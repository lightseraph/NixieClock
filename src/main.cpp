#include <Hardware.h>
#include <network.h>
#include <Preferences.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com", 28800, 86400000);
extern AutoConnect Portal;
CRGB leds[NUM_LEDS];
extern AutoConnect Portal;
IRrecv irrecv(IR_PIN);

decode_results results;
Preferences pref;

CHSV color;

void flash_led();
void fade_off(int channel);
void fade_on(int channel);
void cross_fade(int in_channel, int out_channel);

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  LEDS.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);

  pinMode(IR_PIN, INPUT_PULLUP);
  pinMode(NUM_PIN_A, OUTPUT);
  pinMode(NUM_PIN_B, OUTPUT);
  pinMode(HV_ENABLE, OUTPUT);

  digitalWrite(HV_ENABLE, HIGH); // 低电平有效
  digitalWrite(NUM_PIN_A, LOW);
  digitalWrite(NUM_PIN_B, LOW);

  pref.begin("pref");
  color.hue = pref.getUChar("hue", 0);
  color.sat = pref.getUChar("sat", 255);
  color.val = pref.getUChar("val", 128);

  ledcSetup(8, 1000, 8);
  ledcSetup(9, 1000, 8);
  ledcAttachPin(NUM_PIN_A, 8);
  ledcAttachPin(NUM_PIN_B, 9);

  startWifiWithWebServer();
  irrecv.enableIRIn();
}
void loop()
{
  Portal.handleClient();
  // put your main code here, to run repeatedly:
  static uint8_t work_status = 1;

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
      delay(20);
    }
    if (results.command == 0x0C) // 1 ---
    {
      cross_fade(8, 9);

      // digitalWrite(NUM_PIN_A, HIGH);
      // digitalWrite(NUM_PIN_B, LOW);
    }

    if (results.command == 0x18) // 2 ---
    {
      cross_fade(9, 8);
      // digitalWrite(NUM_PIN_A, LOW);
      // digitalWrite(NUM_PIN_B, HIGH);
    }

    if (results.command == 0x16) // 0 ---
    {
      fade_off(8);
      fade_off(9);
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

  fill_solid(leds, 4, color);
  FastLED.show();

  delay(100);
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

void fade_off(int channel)
{
  for (int i = ledcRead(channel); i != 0; i--)
  {
    ledcWrite(channel, i);
    delay(4);
  }
}

void fade_on(int channel)
{
  for (int i = ledcRead(channel); i != 255; i++)
  {
    ledcWrite(channel, i);
    delay(4);
  }
}

void cross_fade(int in_channel, int out_channel)
{
  for (int i = ledcRead(in_channel), j = ledcRead(out_channel); i != 255; i++, j--)
  {
    ledcWrite(in_channel, i);
    ledcWrite(out_channel, j);
    delay(4);
  }
}