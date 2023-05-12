#include <Hardware.h>
#include <network.h>
#include <Preferences.h>
#include <global_var.h>
#include <time.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com", 28800, 86400000);
extern AutoConnect Portal;
extern CRGB leds[NUM_LEDS];
CHSV color;
extern RTC_DS3231 rtc;
Settings settings;

IRrecv irrecv(IR_PIN);

decode_results results;

hw_timer_t *timer_PWM = NULL;
hw_timer_t *timer_fade = NULL;
hw_timer_t *timer_incremental = NULL;
uint16_t fade_pwm = 0;
uint8_t fadein_num[4] = {0};
uint8_t fadeout_num[4] = {0};
const uint8_t fadein_blank[4] = {10};
uint8_t set_num[4] = {0};
uint8_t flag = 0;
WORK_STATUS work_status = DISP_HOUR_MIN;
uint8_t set_hue; // 0:fixed, 1:increase, 2:decrease
uint8_t dp_count = 0;
bool halfSQW = false;

void flash_led();
void updateRTC();
void startDP();
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

void IRAM_ATTR onTimer_incremental()
{
  for (int i = 0; i < 4; i++)
  {
    fadein_num[i]++;
    fadein_num[i] %= 10;
  }
  fade_pwm = 255;
  flag = 0;
  dp_count++;
}

void IRAM_ATTR onSQW()
{
  timerAlarmEnable(timer_fade);

  if (work_status == SET_CLOSE_TIME_HOUR || work_status == SET_OPEN_TIME_HOUR)
  {
    fade_pwm = 255;
    digitalWrite(DOT, HIGH);
    flag = 0;
    if (digitalRead(SQW) == HIGH)
      memcpy(fadein_num, set_num, 4 * sizeof(uint8_t));
    else
    {
      fadein_num[2] = 10;
      fadein_num[3] = 10;
    }
  }
  else if (work_status == SET_CLOSE_TIME_MIN || work_status == SET_OPEN_TIME_MIN)
  {
    fade_pwm = 255;
    digitalWrite(DOT, HIGH);
    flag = 0;
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
    digitalWrite(DOT, !digitalRead(DOT));
    flag = 1;
  }
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(IR_PIN, INPUT);
  // pinMode(IR_PIN, INPUT_PULLUP);
  // pinMode(RADAR, INPUT);

  pinMode(HV_ENABLE, OUTPUT);
  pinMode(DOT, OUTPUT);
  pinMode(IN2_COMMA, OUTPUT);
  pinMode(IN3_COMMA, OUTPUT);

  digitalWrite(HV_ENABLE, LOW); // 低电平有效
  digitalWrite(DOT, HIGH);
  digitalWrite(IN2_COMMA, LOW);
  digitalWrite(IN3_COMMA, LOW);

  initNixieDriver();
  initLED();
  init_I2C();
  settings.init();

  attachInterrupt(digitalPinToInterrupt(SQW), onSQW, RISING);
  // attachInterrupt(digitalPinToInterrupt(RADAR), onRADAR_ACT, CHANGE);
  color = settings.getColor();

  timer_PWM = timerBegin(1, 80, true);
  timerAttachInterrupt(timer_PWM, onTimer_PWM, true);
  timerAlarmWrite(timer_PWM, PWM_PERIOD, true); // PWM周期 20us

  timer_fade = timerBegin(2, 80, true);
  timerAttachInterrupt(timer_fade, onTimer_Fade, true);
  timerAlarmWrite(timer_fade, FADE_TIME / FADE_STEP * 1000, true);

  timer_incremental = timerBegin(0, 80, true);
  timerAttachInterrupt(timer_incremental, onTimer_incremental, true);
  timerAlarmWrite(timer_incremental, 250000, true); // 解毒数字循环周期 250us

  startWifiWithWebServer();
  irrecv.enableIRIn();
  timeClient.begin();
  set_hue = 0;
  timerAlarmEnable(timer_PWM);
  flash_led();
  Serial.println("hardware setup success!");
}

void loop()
{
  Portal.handleClient();
  // put your main code here, to run repeatedly:
  if (dp_count > 10)
  {
    timerAlarmDisable(timer_incremental);
    attachInterrupt(digitalPinToInterrupt(SQW), onSQW, RISING);
    dp_count = 0;
    fade_pwm = 0;
  }

  if (fade_pwm > FADE_STEP)
  {
    timerAlarmDisable(timer_fade);
    fade_pwm = FADE_STEP;
    for (int i = 0; i < 4; i++)
    {
      fadeout_num[i] = fadein_num[i];
    }
  }

  if (flag)
  {
    char date[15] = "MM-DD hh:mm:ss";
    DateTime rtc_dt = rtc.now();
    rtc_dt.toString(date);
    Serial.println(date);

    if (rtc_dt.unixtime() - settings.mLastupdate > TIME_UPDATE_INTERVAL)
      updateRTC();

    if (!(settings.mOpen_time[0] == 0 &&
          settings.mOpen_time[1] == 0 &&
          settings.mOpen_time[2] == 0 &&
          settings.mOpen_time[3] == 0))
    {
      if (settings.mOpen_time[0] == (date[13] - '0') &&
          settings.mOpen_time[1] == (date[12] - '0') &&
          settings.mOpen_time[2] == (date[10] - '0') &&
          settings.mOpen_time[3] == (date[9] - '0'))
      {
        if (digitalRead(HV_ENABLE) == HIGH)
          digitalWrite(HV_ENABLE, LOW);
      }
    }

    if (!(settings.mClose_time[0] == 0 &&
          settings.mClose_time[1] == 0 &&
          settings.mClose_time[2] == 0 &&
          settings.mClose_time[3] == 0))
    {
      if (settings.mClose_time[0] == (date[13] - '0') &&
          settings.mClose_time[1] == (date[12] - '0') &&
          settings.mClose_time[2] == (date[10] - '0') &&
          settings.mClose_time[3] == (date[9] - '0'))
      {
        if (digitalRead(HV_ENABLE) == LOW)
          digitalWrite(HV_ENABLE, HIGH);
      }
    }

    switch (work_status)
    {
    case DISP_MIN_SEC:
      fadein_num[1] = date[9] - '0';
      fadein_num[0] = date[10] - '0';
      fadein_num[3] = date[12] - '0';
      fadein_num[2] = date[13] - '0';
      break;
    case DISP_HOUR_MIN:
      fadein_num[1] = date[6] - '0';
      fadein_num[0] = date[7] - '0';
      fadein_num[3] = date[9] - '0';
      fadein_num[2] = date[10] - '0';
      break;
    case DISP_MONTH:
      fadein_num[1] = date[0] - '0';
      fadein_num[0] = date[1] - '0';
      fadein_num[3] = date[3] - '0';
      fadein_num[2] = date[4] - '0';
      break;
    }
    flag = 0;

    if ((date[10] - '0') % 5 == 0 && (date[12] - '0') == 0 && (date[13] - '0') == 0)
      startDP();
  }

  if (irrecv.decode(&results))
  {
    serialPrintUint64(results.command, HEX);

    Serial.println("");
    if (results.command == 0x43) // use NTP update rtc
    {
      updateRTC();
      flash_led();
    }
    if (results.command == 0x44) // prev
    {
      if (set_hue == 0)
        set_hue = 2;
      else
        set_hue = 0;
    }
    if (results.command == 0x40) // next
    {
      if (set_hue == 0)
        set_hue = 1;
      else
        set_hue = 0;
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
        uint8_t temp = set_num[3] * 10 + set_num[2];
        temp++;
        if (temp == 24)
          temp = 0;
        set_num[3] = temp / 10;
        set_num[2] = temp % 10;
      }
      else if (work_status == SET_CLOSE_TIME_MIN || work_status == SET_OPEN_TIME_MIN)
      {
        uint8_t temp = set_num[1] * 10 + set_num[0];
        temp++;
        if (temp == 60)
          temp = 0;
        set_num[1] = temp / 10;
        set_num[0] = temp % 10;
      }
      else
      {
        if (color.val < 245)
          color.val += 10;
        else
          color.val = 255;
      }
    }
    if (results.command == 0x07) // ----
    {
      if (work_status == SET_CLOSE_TIME_HOUR || work_status == SET_OPEN_TIME_HOUR)
      {
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
        uint8_t temp = set_num[1] * 10 + set_num[0];
        if (temp == 0)
          temp = 59;
        else
          temp--;
        set_num[1] = temp / 10;
        set_num[0] = temp % 10;
      }
      else
      {
        if (color.val > 10)
          color.val -= 10;
        else
          color.val = 0;
      }
    }
    if (results.command == 0x47) // CH-
    {
      if (color.sat < 245)
        color.sat += 10;
      else
        color.sat = 255;
    }
    if (results.command == 0x45) // CH+
    {
      if (color.sat > 10)
        color.sat -= 10;
      else
        color.sat = 0;
    }
    if (results.command == 0x46) // CH
    {
      color = settings.getColor();
      flash_led();
    }
    if (results.command == 0x4a) // 9 --- HV
    {
      digitalWrite(HV_ENABLE, !digitalRead(HV_ENABLE));
      digitalWrite(DOT, !digitalRead(DOT));
      // delay(45);
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
      if (halfSQW)
        useHalfSQW(false);
    }

    if (results.command == 0x08) // 4 ---
    {
      work_status = DISP_TEMP_HUMIDITY;
      if (halfSQW)
        useHalfSQW(false);
    }

    if (results.command == 0x5A) // 6 ---
    {
      if (work_status == SET_OPEN_TIME_MIN)
      {
        work_status = DISP_HOUR_MIN;
        settings.putTime(set_num, OPEN_TIME);
        useHalfSQW(false);
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

    if (results.command == 0x42) // 7 ---
    {
      if (work_status == SET_CLOSE_TIME_MIN)
      {
        work_status = DISP_HOUR_MIN;
        settings.putTime(set_num, CLOSE_TIME);
        useHalfSQW(false);
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

    if (results.command == 0x16) // 0 ---
    {
      startDP();
    }

    fill_solid(leds, 4, CRGB::Black);
    FastLED.show();
    delay(20);
    irrecv.resume(); // Receive the next value
  }

  switch (set_hue)
  {
  case 0:
    break;
  case 1:
    color.hue++;
    break;
  case 2:
    color.hue--;
    break;
  }

  fill_solid(leds, 4, color);
  FastLED.show();

  delay(20);
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
  timeClient.forceUpdate();
  Serial.println(timeClient.getFormattedTime());
  DateTime dt(timeClient.getEpochTime());
  rtc.adjust(dt);
  settings.putLastUpdate(timeClient.getEpochTime());
}

void startDP()
{
  dp_count = 0;
  for (int i = 0; i < 4; i++)
  {
    fadein_num[i] = 255;
  }
  timerAlarmEnable(timer_incremental);
  detachInterrupt(digitalPinToInterrupt(SQW));
}

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