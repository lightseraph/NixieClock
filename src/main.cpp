#include <Hardware.h>
#include <network.h>
#include <Preferences.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com", 28800, 86400000);
extern AutoConnect Portal;
extern CRGB leds[NUM_LEDS];
extern CHSV color;
extern RTC_DS3231 rtc;

IRrecv irrecv(IR_PIN);

decode_results results;
Preferences pref;

hw_timer_t *timer_PWM = NULL;
hw_timer_t *timer_fade = NULL;
hw_timer_t *timer_incremental = NULL;
uint16_t fade_pwm = 0;
uint8_t fadein_num[4] = {0};
uint8_t fadeout_num[4] = {0};
uint8_t flag = 0;

void flash_led();

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

void IRAM_ATTR onTimer_incremental()
{
  for (int i = 0; i < 4; i++)
  {
    fadein_num[i]++;
    fadein_num[i] %= 10;
  }
  flag = 1;
}

void IRAM_ATTR onSQW()
{
  for (int i = 0; i < 4; i++)
  {
    fadein_num[i]++;
    fadein_num[i] %= 10;
  }
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

  pinMode(HV_ENABLE, OUTPUT);
  pinMode(DOT, OUTPUT);
  pinMode(IN2_COMMA, OUTPUT);
  pinMode(IN3_COMMA, OUTPUT);

  digitalWrite(HV_ENABLE, LOW); // 低电平有效
  digitalWrite(DOT, HIGH);
  digitalWrite(IN2_COMMA, LOW);
  digitalWrite(IN3_COMMA, LOW);

  pref.begin("pref");
  color.hue = pref.getUChar("hue", 0);
  color.sat = pref.getUChar("sat", 255);
  color.val = pref.getUChar("val", 128);

  initNixieDriver();
  initLED();
  init_I2C();
  attachInterrupt(digitalPinToInterrupt(SQW), onSQW, RISING);

  timer_PWM = timerBegin(1, 80, true);
  timerAttachInterrupt(timer_PWM, onTimer_PWM, true);
  timerAlarmWrite(timer_PWM, PWM_PERIOD, true); // PWM周期 20us
  timerAlarmEnable(timer_PWM);

  timer_fade = timerBegin(2, 80, true);
  timerAttachInterrupt(timer_fade, onTimer_Fade, true);
  timerAlarmWrite(timer_fade, FADE_TIME / FADE_STEP * 1000, true);

  timer_incremental = timerBegin(0, 80, true);
  timerAttachInterrupt(timer_incremental, onTimer_incremental, true);
  timerAlarmWrite(timer_incremental, 200000, true);
  // timerAlarmEnable(timer_incremental);

  startWifiWithWebServer();
  irrecv.enableIRIn();
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
  static uint8_t work_status = 2;

  if (flag)
  {
    char date[10] = "hh:mm:ss";
    rtc.now().toString(date);
    Serial.println(date);
    // Serial.println(rtc.getTemperature());
    flag = 0;
  }

  if (irrecv.decode(&results))
  {
    // print() & println() can't handle printing long longs. (uint64_t)
    serialPrintUint64(results.command, HEX);

    Serial.println("");
    if (results.command == 0x43) // play/pause
    {
      work_status = 1;
    }
    if (results.command == 0x44) // prev
    {
      if (work_status == 1)
        work_status = 2;
      else
        work_status = 1;
    }
    if (results.command == 0x40) // next
    {
      if (work_status == 1)
        work_status = 3;
      else
        work_status = 1;
    }
    if (results.command == 0x09) // EQ
    {
      pref.putUChar("hue", color.hue);
      pref.putUChar("sat", color.sat);
      pref.putUChar("val", color.val);
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
      color.hue = pref.getUChar("hue", 0);
      color.sat = pref.getUChar("sat", 255);
      color.val = pref.getUChar("val", 255);
    }
    if (results.command == 0x4a) // 9 --- HV
    {
      digitalWrite(HV_ENABLE, !digitalRead(HV_ENABLE));
      digitalWrite(DOT, !digitalRead(DOT));
      delay(45);
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
      if (timerAlarmEnabled(timer_incremental))
      {
        timerAlarmDisable(timer_incremental);
        attachInterrupt(digitalPinToInterrupt(SQW), onSQW, RISING);
      }
      else
      {
        timerAlarmEnable(timer_incremental);
        detachInterrupt(digitalPinToInterrupt(SQW));
        digitalWrite(IN3_COMMA, HIGH);
      }
    }

    fill_solid(leds, 4, CRGB::Black);
    FastLED.show();
    delay(30);
    irrecv.resume(); // Receive the next value
  }

  switch (work_status)
  {
  case 0:
    break;
  case 1:
    break;
  case 2:
    color.hue--;
    break;
  case 3:
    color.hue++;
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

  delay(50);
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
