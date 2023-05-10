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
uint8_t flag = 0;
WORK_STATUS work_status;
uint8_t set_hue; // 0:fixed, 1:increase, 2:decrease
uint8_t dp_count = 0;

void flash_led();
void updateRTC();

void IRAM_ATTR onTimer_PWM()
{
  static uint8_t counter = 0;
  counter++;

  if ((counter % FADE_STEP) < fade_pwm)
  {
    displayNixie(fadein_num);
  }
  else
  {
    displayNixie(fadeout_num);
  }
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
  fade_pwm = 256;
  flag = 0;
  dp_count++;
}

void IRAM_ATTR onSQW()
{
  /* for (int i = 0; i < 4; i++)
  {
    fadein_num[i]++;
    fadein_num[i] %= 10;
  } */
  fade_pwm = 0;
  timerAlarmEnable(timer_fade);
  digitalWrite(DOT, !digitalRead(DOT));
  digitalWrite(IN3_COMMA, !digitalRead(IN3_COMMA));
  flag = 1;
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
  timerAlarmWrite(timer_incremental, 200000, true);
  // timerAlarmEnable(timer_incremental);

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
  if (fade_pwm > FADE_STEP)
  {
    timerAlarmDisable(timer_fade);
    fade_pwm = FADE_STEP;
    for (int i = 0; i < 4; i++)
    {
      fadeout_num[i] = fadein_num[i];
    }
  }

  if (rtc.now().unixtime() - settings.mLastupdate > TIME_UPDATE_INTERVAL)
  {
    updateRTC();
  }

  if (dp_count > 10)
  {
    timerAlarmDisable(timer_incremental);
    attachInterrupt(digitalPinToInterrupt(SQW), onSQW, RISING);
    dp_count = 0;
  }

  if (flag)
  {
    char date[15] = "MM-DD hh:mm:ss";
    rtc.now().toString(date);
    Serial.println(date);

    fadein_num[0] = date[6] - '0';
    fadein_num[1] = date[7] - '0';
    fadein_num[3] = date[12] - '0';
    fadein_num[2] = date[13] - '0';
    flag = 0;
  }

  if (irrecv.decode(&results))
  {
    // print() & println() can't handle printing long longs. (uint64_t)
    serialPrintUint64(results.command, HEX);

    Serial.println("");
    if (results.command == 0x43) // play/pause
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
      if (color.val < 245)
        color.val += 10;
      else
        color.val = 255;
    }
    if (results.command == 0x07) // ----
    {
      if (color.val > 10)
        color.val -= 10;
      else
        color.val = 0;
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
    }
    if (results.command == 0x4a) // 9 --- HV
    {
      digitalWrite(HV_ENABLE, !digitalRead(HV_ENABLE));
      digitalWrite(DOT, !digitalRead(DOT));
      // delay(45);
    }
    if (results.command == 0x0C) // 1 ---
    {
      fadein_num[3] = 1;
      fade_pwm = 0;
      timerAlarmEnable(timer_fade);
      detachInterrupt(digitalPinToInterrupt(SQW));
      timerAlarmDisable(timer_incremental);
      digitalWrite(IN3_COMMA, HIGH);
    }

    if (results.command == 0x18) // 2 ---
    {
      fadein_num[3] = 2;
      fade_pwm = 0;
      timerAlarmEnable(timer_fade);
      detachInterrupt(digitalPinToInterrupt(SQW));
      timerAlarmDisable(timer_incremental);
      digitalWrite(IN3_COMMA, HIGH);
    }

    if (results.command == 0x5E) // 3 ---
    {
      fadein_num[3] = 3;
      fade_pwm = 0;
      timerAlarmEnable(timer_fade);
      detachInterrupt(digitalPinToInterrupt(SQW));
      timerAlarmDisable(timer_incremental);
      digitalWrite(IN3_COMMA, HIGH);
    }

    if (results.command == 0x08) // 4 ---
    {
      fadein_num[3] = 4;
      fade_pwm = 0;
      timerAlarmEnable(timer_fade);
      detachInterrupt(digitalPinToInterrupt(SQW));
      timerAlarmDisable(timer_incremental);
      digitalWrite(IN3_COMMA, HIGH);
    }

    if (results.command == 0x16) // 0 ---
    {
      dp_count = 0;
      timerAlarmEnable(timer_incremental);
      detachInterrupt(digitalPinToInterrupt(SQW));
    }

    fill_solid(leds, 4, CRGB::Black);
    FastLED.show();
    delay(30);
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

  /* if (counter > 50)
  {
    if (ledcRead(8) > 250)
      cross_fade(9, 8);
    else
      cross_fade(8, 9);
    counter = 0;
  } */

  fill_solid(leds, 4, color);
  FastLED.show();

  delay(25);
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