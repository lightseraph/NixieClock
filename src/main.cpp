#include <Arduino.h>
#include <AutoConnect.h>
#include <DS3231.h>

WebServer Server;
AutoConnect Portal(Server);
AutoConnectConfig config;

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
}

void loop()
{
  // put your main code here, to run repeatedly:
  Portal.handleClient();
}