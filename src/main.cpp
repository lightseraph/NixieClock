#include <Hardware.h>
#include <network.h>
#include <Preferences.h>
#include <global_var.h>
#include <time.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com", 28800);
extern AutoConnect Portal;
extern CRGB leds[NUM_LEDS];
CHSV color;
extern RTC_DS3231 rtc;
extern Adafruit_SHT31 sht31;
Settings settings;

IRrecv irrecv(IR_PIN);

decode_results results;

hw_timer_t *timer_PWM = NULL;
hw_timer_t *timer_fade = NULL;
hw_timer_t *timer_DP = NULL;
uint16_t fade_pwm = 0;
uint8_t fadein_num[4] = {0, 0, 0, 0};
uint8_t fadeout_num[4] = {0, 0, 0, 0};
uint8_t set_num[4] = {0, 0, 0, 0};
uint8_t displayStatus_Flag = 0;
WORK_STATUS work_status = DISP_HOUR_MIN;
uint8_t set_hue = 0; // 0:fixed, 1:increase, 2:decrease
uint8_t dp_count = 0;
bool halfSQW = false;
bool colorChanged;
uint8_t humidity_count = 0;
uint16_t dp_limit;
float f_hum, f_temp;
uint16_t th_error;
uint8_t set_count = 0;
uint32_t working_time;
uint8_t working_time_count = 0;
uint8_t disp_ip_count = 0;

void flash_led();
void updateRTC();
void startDP(uint8_t repeat);
void useHalfSQW(bool sw);

void IRAM_ATTR onTimer_PWM()
{
  static uint8_t counter = 0;
  counter++;

  if ((counter % FADE_STEP) < fade_pwm)
    displayNixie(fadein_num);
  else
    displayNixie(fadeout_num);
}

void IRAM_ATTR onTimer_Fade()
{
  fade_pwm++;
}

void IRAM_ATTR onRADAR_ACT()
{
  if (digitalRead(RADAR) == HIGH)
  {
    if (digitalRead(HV_ENABLE) == HIGH)
      digitalWrite(HV_ENABLE, LOW);
  }
  else
  {
    if (digitalRead(HV_ENABLE) == LOW)
      digitalWrite(HV_ENABLE, HIGH);
  }
}

void IRAM_ATTR onTimer_DP()
{
  for (int i = 0; i < 4; i++)
  {
    fadein_num[i]++;
    fadein_num[i] %= 10;
  }
  fade_pwm = FADE_STEP;
  displayStatus_Flag = 0;
  dp_count++;
}

void IRAM_ATTR onSQW()
{
  timerAlarmEnable(timer_fade);
  if (digitalRead(HV_ENABLE) == LOW)
    working_time++;

  if (work_status == SET_CLOSE_TIME_HOUR || work_status == SET_OPEN_TIME_HOUR ||
      work_status == SET_TERROR_INT || work_status == SET_HERROR_INT)
  {
    set_count++;
    fade_pwm = FADE_STEP;
    displayStatus_Flag = 0;
    if (digitalRead(SQW) == HIGH)
      memcpy(fadein_num, set_num, 4 * sizeof(uint8_t));
    else
    {
      fadein_num[2] = 10;
      fadein_num[3] = 10;
    }
  }
  else if (work_status == SET_CLOSE_TIME_MIN || work_status == SET_OPEN_TIME_MIN ||
           work_status == SET_TERROR_DEC || work_status == SET_HERROR_DEC)
  {
    set_count++;
    fade_pwm = FADE_STEP;
    displayStatus_Flag = 0;
    if (digitalRead(SQW) == HIGH)
      memcpy(fadein_num, set_num, 4 * sizeof(uint8_t));
    else
    {
      fadein_num[0] = 10;
      fadein_num[1] = 10;
    }
  }
  else
  {
    fade_pwm = 0;
    if (work_status != DISP_TEMP_HUMIDITY)
      digitalWrite(DOT, !digitalRead(DOT));
    if (work_status == DISP_WORKING_TIME)
      digitalWrite(DOT, LOW);
    displayStatus_Flag = 1;
  }
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  // pinMode(IR_PIN, INPUT);
  pinMode(RADAR, INPUT_PULLDOWN);

  pinMode(HV_ENABLE, OUTPUT);
  pinMode(DOT, OUTPUT);
  pinMode(IN2_COMMA, OUTPUT);
  pinMode(IN3_COMMA, OUTPUT);

  digitalWrite(HV_ENABLE, HIGH); // 低电平有效
  digitalWrite(DOT, HIGH);
  digitalWrite(IN2_COMMA, LOW);
  digitalWrite(IN3_COMMA, LOW);

  initNixieDriver();
  initLED();
  init_I2C();
  settings.init();

  attachInterrupt(digitalPinToInterrupt(SQW), onSQW, RISING);
  attachInterrupt(digitalPinToInterrupt(RADAR), onRADAR_ACT, CHANGE);
  color = settings.getColor();
  colorChanged = true;

  timer_PWM = timerBegin(1, 80, true);
  timerAttachInterrupt(timer_PWM, onTimer_PWM, true);
  timerAlarmWrite(timer_PWM, PWM_PERIOD, true); // PWM周期 20us

  timer_fade = timerBegin(2, 80, true);
  timerAttachInterrupt(timer_fade, onTimer_Fade, true);
  timerAlarmWrite(timer_fade, FADE_TIME / FADE_STEP * 1000, true);

  timer_DP = timerBegin(0, 80, true);
  timerAttachInterrupt(timer_DP, onTimer_DP, true);
  timerAlarmWrite(timer_DP, 300000, true); // 解毒数字循环周期 300ms

  startWifiWithWebServer();
  irrecv.enableIRIn(true);
  timeClient.begin();
  timerAlarmEnable(timer_PWM);
  flash_led();
  digitalWrite(HV_ENABLE, LOW);
  working_time = settings.mWorkingTime;
  Serial.println("Hardware setup success!");
}

void loop()
{
  Portal.handleClient();
  // put your main code here, to run repeatedly:

  // 不显示温度湿度状态关闭小数点
  if (work_status != DISP_TEMP_HUMIDITY &&
      work_status != SET_HERROR_DEC && work_status != SET_HERROR_INT &&
      work_status != SET_TERROR_DEC && work_status != SET_TERROR_INT &&
      HIGH == digitalRead(IN3_COMMA))
    digitalWrite(IN3_COMMA, LOW);

  // 主循环里判断PWM占空比是否变到100
  if (fade_pwm > FADE_STEP)
  {
    timerAlarmDisable(timer_fade);
    fade_pwm = FADE_STEP;
    memcpy(fadeout_num, fadein_num, 4 * sizeof(uint8_t));
  }

  // 结束解毒程序，返回正常时间显示
  if (dp_count > dp_limit)
  {
    timerAlarmDisable(timer_DP);
    attachInterrupt(digitalPinToInterrupt(SQW), onSQW, RISING);
    dp_count = 0;
    fade_pwm = 0;
    displayStatus_Flag = 1;
  }

  // 进入设置参数状态后，没有输入20秒后自动返回显示时间
  if (set_count > 40)
  {
    set_count = 0;
    work_status = DISP_HOUR_MIN;
    fade_pwm = 0;
    displayStatus_Flag = 1;
    useHalfSQW(false);
  }

  if (displayStatus_Flag)
  {
    char date[15] = "MM-DD hh:mm:ss";
    DateTime rtc_dt = rtc.now();
    rtc_dt.toString(date);
    Serial.println(date);

    // 从NTP更新时间间隔超过1天，自动更新一次
    if (rtc_dt.unixtime() - settings.mLastupdate > TIME_UPDATE_INTERVAL)
    {
      updateRTC();
      Serial.println("RTC time has been updated");
    }

    // 自动开关机时间设置为00:00时，为无效状态
    if (!(settings.mOpen_time[0] == 0 &&
          settings.mOpen_time[1] == 0 &&
          settings.mOpen_time[2] == 0 &&
          settings.mOpen_time[3] == 0))
    {
      if (settings.mOpen_time[0] == (date[10] - '0') &&
          settings.mOpen_time[1] == (date[9] - '0') &&
          settings.mOpen_time[2] == (date[7] - '0') &&
          settings.mOpen_time[3] == (date[6] - '0') &&
          (date[12] - '0') == 0)
      {
        if (digitalRead(HV_ENABLE) == HIGH)
        {
          attachInterrupt(digitalPinToInterrupt(RADAR), onRADAR_ACT, CHANGE);
          digitalWrite(HV_ENABLE, LOW);
        }
      }
    }

    if (!(settings.mClose_time[0] == 0 &&
          settings.mClose_time[1] == 0 &&
          settings.mClose_time[2] == 0 &&
          settings.mClose_time[3] == 0))
    {
      if (settings.mClose_time[0] == (date[10] - '0') &&
          settings.mClose_time[1] == (date[9] - '0') &&
          settings.mClose_time[2] == (date[7] - '0') &&
          settings.mClose_time[3] == (date[6] - '0') &&
          (date[12] - '0') == 0)
      {
        if (digitalRead(HV_ENABLE) == LOW)
        {
          digitalWrite(HV_ENABLE, HIGH);
          detachInterrupt(digitalPinToInterrupt(RADAR));
          settings.putWorkingTime(working_time);
        }
      }
    }

    switch (work_status)
    {
    case DISP_MIN_SEC:
      fadein_num[3] = date[9] - '0';
      fadein_num[2] = date[10] - '0';
      fadein_num[1] = date[12] - '0';
      fadein_num[0] = date[13] - '0';
      break;
    case DISP_HOUR_MIN:
      fadein_num[3] = date[6] - '0';
      fadein_num[2] = date[7] - '0';
      fadein_num[1] = date[9] - '0';
      fadein_num[0] = date[10] - '0';
      break;
    case DISP_MONTH:
      fadein_num[3] = date[0] - '0';
      fadein_num[2] = date[1] - '0';
      fadein_num[1] = date[3] - '0';
      fadein_num[0] = date[4] - '0';
      humidity_count++;
      if (humidity_count == 10) // 持续显示10秒后返回显示时间, 借用humidity_count变量
        work_status = DISP_HOUR_MIN;
      break;
    case DISP_TEMP_HUMIDITY:
      digitalWrite(IN3_COMMA, HIGH);
      float fTemp_hum;
      if (((humidity_count / 5) % 2) == 0) // 每5秒切换一次温度湿度显示，温度显示冒号亮，湿度显示冒号灭
      {
        fTemp_hum = f_temp;
        digitalWrite(DOT, HIGH);
      }
      else
      {
        fTemp_hum = f_hum;
        digitalWrite(DOT, LOW);
      }

      fadein_num[3] = (int)fTemp_hum / 10;
      fadein_num[2] = (int)fTemp_hum % 10;
      fadein_num[1] = (((int)(fTemp_hum * 100)) % 100) / 10;
      fadein_num[0] = (((int)(fTemp_hum * 100)) % 100) % 10;

      humidity_count++;
      if (humidity_count == 20) // 持续显示20秒后返回显示时间
        work_status = DISP_HOUR_MIN;
      break;

    case DISP_WORKING_TIME:
    {
      uint16_t working_hours = working_time / 3600;
      fadein_num[3] = working_hours / 1000;
      fadein_num[2] = working_hours % 1000 / 100;
      fadein_num[1] = working_hours % 1000 % 100 / 10;
      fadein_num[0] = working_hours % 1000 % 100 % 10;

      working_time_count++;
      if (working_time_count == 6)
        work_status = DISP_HOUR_MIN;
      break;
    }

    case DISP_IP:
    {
      uint8_t ip = WiFi.localIP()[disp_ip_count / 2];
      fadein_num[3] = 0;
      fadein_num[2] = ip / 100;
      fadein_num[1] = ip % 100 / 10;
      fadein_num[0] = ip % 10;
      if (disp_ip_count > 6)
      {
        work_status = DISP_HOUR_MIN;
        disp_ip_count = 0;
      }
      else
        disp_ip_count++;
      break;
    }
    }
    displayStatus_Flag = 0;

    // 分钟数字被5整除调用一次解毒
    if (((date[10] - '0') + 1) % 5 == 0 && (date[12] - '0') == 5 && (date[13] - '0') == 9 &&
        (work_status == DISP_HOUR_MIN || work_status == DISP_MIN_SEC))
      startDP(1);
  }

  // 红外接收处理
  if (irrecv.decode(&results))
  {
    serialPrintUint64(results.command, HEX);

    Serial.println("");
    if (results.command == 0x43) // play键  use NTP update rtc
    {
      updateRTC();
      flash_led();
    }
    if (results.command == 0x44) // prev
    {
      if (set_hue == 0)
        set_hue = 2;
      else
      {
        set_hue = 0;
        colorChanged = true;
      }
    }
    if (results.command == 0x40) // next
    {
      if (set_hue == 0)
        set_hue = 1;
      else
      {
        set_hue = 0;
        colorChanged = true;
      }
    }
    if (results.command == 0x09) // EQ
    {
      settings.putColor(color);
      flash_led();
    }
    if (results.command == 0x15) // ++++
    {
      if (work_status == SET_CLOSE_TIME_HOUR || work_status == SET_OPEN_TIME_HOUR)
      {
        set_count = 0;
        uint8_t temp = set_num[3] * 10 + set_num[2];
        temp++;
        if (temp == 24)
          temp = 0;
        set_num[3] = temp / 10;
        set_num[2] = temp % 10;
      }
      else if (work_status == SET_CLOSE_TIME_MIN || work_status == SET_OPEN_TIME_MIN)
      {
        set_count = 0;
        uint8_t temp = set_num[1] * 10 + set_num[0];
        temp++;
        if (temp == 60)
          temp = 0;
        set_num[1] = temp / 10;
        set_num[0] = temp % 10;
      }
      else if (work_status == SET_HERROR_DEC || work_status == SET_TERROR_DEC)
      {
        set_count = 0;
        th_error++;
        set_num[3] = th_error / 100;
        set_num[2] = th_error % 100 / 10;
        set_num[1] = th_error % 10;
        set_num[0] = 0;
      }
      else if (work_status == SET_HERROR_INT || work_status == SET_TERROR_INT)
      {
        set_count = 0;
        th_error += 10;
        set_num[3] = th_error / 100;
        set_num[2] = th_error % 100 / 10;
        set_num[1] = th_error % 10;
        set_num[0] = 0;
      }
      else
      {
        if (color.val < 245)
          color.val += 10;
        else
          color.val = 255;
        colorChanged = true;
      }
    }
    if (results.command == 0x07) // ----
    {
      if (work_status == SET_CLOSE_TIME_HOUR || work_status == SET_OPEN_TIME_HOUR)
      {
        set_count = 0;
        uint8_t temp = set_num[3] * 10 + set_num[2];
        if (temp == 0)
          temp = 23;
        else
          temp--;
        set_num[3] = temp / 10;
        set_num[2] = temp % 10;
      }
      else if (work_status == SET_CLOSE_TIME_MIN || work_status == SET_OPEN_TIME_MIN)
      {
        set_count = 0;
        uint8_t temp = set_num[1] * 10 + set_num[0];
        if (temp == 0)
          temp = 59;
        else
          temp--;
        set_num[1] = temp / 10;
        set_num[0] = temp % 10;
      }
      else if (work_status == SET_HERROR_DEC || work_status == SET_TERROR_DEC)
      {
        set_count = 0;
        th_error--;
        set_num[3] = th_error / 100;
        set_num[2] = th_error % 100 / 10;
        set_num[1] = th_error % 10;
        set_num[0] = 0;
      }
      else if (work_status == SET_HERROR_INT || work_status == SET_TERROR_INT)
      {
        set_count = 0;
        th_error -= 10;
        set_num[3] = th_error / 100;
        set_num[2] = th_error % 100 / 10;
        set_num[1] = th_error % 10;
        set_num[0] = 0;
      }

      else
      {
        if (color.val > 10)
          color.val -= 10;
        else
          color.val = 0;
        colorChanged = true;
      }
    }
    if (results.command == 0x47) // CH-
    {
      if (color.sat < 245)
        color.sat += 10;
      else
        color.sat = 255;
      colorChanged = true;
    }
    if (results.command == 0x45) // CH+
    {
      if (color.sat > 10)
        color.sat -= 10;
      else
        color.sat = 0;
      colorChanged = true;
    }
    if (results.command == 0x46) // CH
    {
      color = settings.getColor();
      colorChanged = true;
      flash_led();
    }

    if (results.command == 0x4a) // 9 --- HV
    {
      if (digitalRead(HV_ENABLE) == LOW)
      {
        digitalWrite(HV_ENABLE, HIGH);
        detachInterrupt(digitalPinToInterrupt(RADAR));
      }
      else
      {
        attachInterrupt(digitalPinToInterrupt(RADAR), onRADAR_ACT, CHANGE);
        digitalWrite(HV_ENABLE, LOW);
      }
    }

    if (results.command == 0x0C) // 1 ---
    {
      work_status = DISP_MIN_SEC;
      if (halfSQW)
        useHalfSQW(false);
    }

    if (results.command == 0x18) // 2 ---
    {
      work_status = DISP_HOUR_MIN;
      if (halfSQW)
        useHalfSQW(false);
    }

    if (results.command == 0x5E) // 3 ---
    {
      work_status = DISP_MONTH;
      humidity_count = 0;
      if (halfSQW)
        useHalfSQW(false);
    }

    if (results.command == 0x08) // 4 ---
    {
      work_status = DISP_TEMP_HUMIDITY;
      f_hum = sht31.readHumidity() + settings.mHerror / 10.0f;
      f_temp = sht31.readTemperature() - settings.mTerror / 10.0f;
      if (halfSQW)
        useHalfSQW(false);
      humidity_count = 0;
    }

    if (results.command == 0x1C) // 5 ---设置温度校正
    {
      set_count = 0;
      digitalWrite(IN3_COMMA, HIGH);
      digitalWrite(DOT, HIGH);
      if (work_status == SET_TERROR_DEC)
      {
        work_status = DISP_TEMP_HUMIDITY;
        humidity_count = 0;
        settings.putTerror(th_error);
        settings.putWorkingTime(working_time);
        flash_led();
        useHalfSQW(false);
      }
      else if (work_status == SET_TERROR_INT)
      {
        work_status = SET_TERROR_DEC;
      }
      else
      {
        work_status = SET_TERROR_INT;
        th_error = settings.getTerror();
        set_num[3] = th_error / 100;
        set_num[2] = th_error % 100 / 10;
        set_num[1] = th_error % 10;
        set_num[0] = 0;
        useHalfSQW(true);
      }
    }

    if (results.command == 0x5A) // 6 ---设置湿度校正
    {
      set_count = 0;
      digitalWrite(IN3_COMMA, HIGH);
      digitalWrite(DOT, LOW);
      if (work_status == SET_HERROR_DEC)
      {
        work_status = DISP_TEMP_HUMIDITY;
        humidity_count = 0;
        settings.putHerror(th_error);
        settings.putWorkingTime(working_time);
        flash_led();
        digitalWrite(IN3_COMMA, LOW);
        useHalfSQW(false);
      }
      else if (work_status == SET_HERROR_INT)
      {
        work_status = SET_HERROR_DEC;
      }
      else
      {
        work_status = SET_HERROR_INT;
        th_error = settings.getHerror();
        set_num[3] = th_error / 100;
        set_num[2] = th_error % 100 / 10;
        set_num[1] = th_error % 10;
        set_num[0] = 0;
        useHalfSQW(true);
      }
    }

    if (results.command == 0x42) // 7 ---设置自动开机时间
    {
      set_count = 0;
      digitalWrite(DOT, HIGH);
      if (work_status == SET_OPEN_TIME_MIN)
      {
        work_status = DISP_HOUR_MIN;
        settings.putTime(set_num, OPEN_TIME);
        useHalfSQW(false);
        flash_led();
        digitalWrite(IN3_COMMA, LOW);
      }
      else if (work_status == SET_OPEN_TIME_HOUR)
      {
        work_status = SET_OPEN_TIME_MIN;
      }
      else
      {
        work_status = SET_OPEN_TIME_HOUR;
        useHalfSQW(true);
        settings.getTime(set_num, OPEN_TIME);
      }
    }

    if (results.command == 0x52) // 8 ---设置自动关机时间
    {
      set_count = 0;
      digitalWrite(DOT, LOW);
      if (work_status == SET_CLOSE_TIME_MIN)
      {
        work_status = DISP_HOUR_MIN;
        settings.putTime(set_num, CLOSE_TIME);
        useHalfSQW(false);
        flash_led();
      }
      else if (work_status == SET_CLOSE_TIME_HOUR)
      {
        work_status = SET_CLOSE_TIME_MIN;
      }
      else
      {
        work_status = SET_CLOSE_TIME_HOUR;
        useHalfSQW(true);
        settings.getTime(set_num, CLOSE_TIME);
      }
    }

    if (results.command == 0x16) // 0 ---手动DP，持续十次
    {
      // 设置开关机时间模式时，按0复位设置
      if (work_status == SET_CLOSE_TIME_HOUR ||
          work_status == SET_OPEN_TIME_HOUR ||
          work_status == SET_CLOSE_TIME_MIN ||
          work_status == SET_OPEN_TIME_MIN)
      {
        for (int i = 0; i < 4; i++)
          set_num[i] = 0;
      }
      else
        startDP(10);
    }

    if (results.command == 0x19) // 100+ ---显示辉光管总工作时间
    {
      work_status = DISP_WORKING_TIME;
      working_time_count = 0;
      if (halfSQW)
        useHalfSQW(false);
    }

    if (results.command == 0x0D) // 200+ ---显示IP
    {
      work_status = DISP_IP;
      disp_ip_count = 0;
    }

    // 有红外信号时，闪烁LED
    fill_solid(leds, 4, CRGB::Black);
    FastLED.show();
    delay(30);
    fill_solid(leds, 4, color);
    FastLED.show();
    irrecv.resume(); // Receive the next value
  }

  switch (set_hue)
  {
  case 0:
    break;
  case 1:
    color.hue++;
    colorChanged = true;
    break;
  case 2:
    color.hue--;
    colorChanged = true;
    break;
  }

  if (colorChanged)
  {
    fill_solid(leds, 4, color);
    FastLED.show();
    colorChanged = false;
    delay(35);
  }

  if (digitalRead(HV_ENABLE) == HIGH)
  {
    fill_solid(leds, 4, CRGB::Black);
    FastLED.show();
  }
  else
  {
    fill_solid(leds, 4, color);
    FastLED.show();
  }
  // 主循环所有分支均依靠条件分支，无常态延时
}

void flash_led()
{
  for (int i = 0; i < 5; i++)
  {
    fill_solid(leds, 4, CRGB::Black);
    FastLED.show();
    delay(50);
    fill_solid(leds, 4, color);
    FastLED.show();
    delay(50);
  }
}

void updateRTC()
{
  do
  {
    timeClient.forceUpdate();
    DateTime dt(timeClient.getEpochTime());
    rtc.adjust(dt);
    delay(1000);
  } while (rtc.now().year() == 2006); // NTP更新时间时，不明原因导致更新结果为2006-2-6 14：28,检查更新结果

  settings.putLastUpdate(timeClient.getEpochTime());
  settings.putWorkingTime(working_time);
}

void startDP(uint8_t repeat)
{
  dp_count = 0;
  for (int i = 0; i < 4; i++)
    fadein_num[i] = 255;

  dp_limit = 10 * repeat;

  timerAlarmEnable(timer_DP);
  detachInterrupt(digitalPinToInterrupt(SQW));
  digitalWrite(DOT, HIGH);
}

// 用于设置模式时，闪烁频率提高一倍，即使用 SQW 秒脉冲在上下沿都触发
void useHalfSQW(bool sw)
{
  if (sw)
  {
    detachInterrupt(digitalPinToInterrupt(SQW));
    attachInterrupt(digitalPinToInterrupt(SQW), onSQW, CHANGE);
    halfSQW = true;
  }
  else
  {
    detachInterrupt(digitalPinToInterrupt(SQW));
    attachInterrupt(digitalPinToInterrupt(SQW), onSQW, RISING);
    halfSQW = false;
  }
}