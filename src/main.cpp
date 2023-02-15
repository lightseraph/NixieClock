#include <Arduino.h>
#include <AutoConnect.h>
#include <DS3231.h>
#include <FastLED.h>

/*----------LED PARAM-----------*/
#define NUM_LEDS 2
#define LED_TYPE WS2812
#define COLOR_ORDER GRB
#define DATA_PIN 16
#define LED_BRIGHTNESS 128

WebServer Server;
AutoConnect Portal(Server);
AutoConnectConfig config;

CRGB leds[NUM_LEDS];

void rootPage()
{
  char content[] = "Hello World";
  Server.send(200, "text/plain", content);
}

void setup()
{
  // put your setup code here, to run once:
  delay(1000);
  Serial.begin(115200);
  config.ota = AC_OTA_BUILTIN;
  Portal.config(config);

  Server.on("/", rootPage);
  Portal.begin();
  // Serial.println("Web server started: " + Wifi.localIP().toString());

  LEDS.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(LED_BRIGHTNESS);
}

void loop()
{
  // put your main code here, to run repeatedly:
  Portal.handleClient();
  fill_solid(leds, 2, CRGB::OrangeRed);
  FastLED.show();
  delay(25);
}