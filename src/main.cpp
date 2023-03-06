#include <Hardware.h>
#include <network.h>

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp1.aliyun.com", 28800, 86400000);

CRGB leds[NUM_LEDS];

IRrecv irrecv(IR_PIN);

decode_results results;

void setup()
{
  // put your setup code here, to run once:
  delay(1000);
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

  startWifiWithWebServer();
  irrecv.enableIRIn();
}

void loop()
{
  // put your main code here, to run repeatedly:

  fill_solid(leds, 4, CRGB::OrangeRed);
  FastLED.show();
  delay(25);
  if (irrecv.decode(&results))
  {
    // print() & println() can't handle printing long longs. (uint64_t)
    serialPrintUint64(results.value, HEX);
    Serial.println("");
    irrecv.resume(); // Receive the next value
  }

  delay(50);
}
