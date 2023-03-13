#include "Hardware.h"
#include "network.h"

WebServer httpServer;
AutoConnectConfig Config;
AutoConnect Portal(httpServer);

HTTPUpdateServer httpUpdater;
AutoConnectAux update("/update", "UPDATE");
AutoConnectAux hello;

int wifiTimeout = 0;
bool isFirmwareMode = false;

static const char AUX_AppPage[] PROGMEM = R"(
{
  "title": "Software Info",
  "uri": "/",
  "menu": true,
  "element": [
    {
      "name": "caption",
      "type": "ACText",
      "value": "<h2>ESPnixie</h2>",
      "style": "text-align:center;color:#2f4f4f;padding:10px;"
    },
    {
      "name": "content",
      "type": "ACText",
      "value": "NixieClock based on IN-8-2 , Modified By Light , rev 2"
    }
  ]
}
)";

void startWifiWithWebServer()
{
  Serial.println("WebServer start!");

  // WiFi.mode(WIFI_MODE_APSTA);
  httpUpdater.setup(&httpServer);
  // Load a custom web page for a sketch and a dummy page for the updater.
  hello.load(AUX_AppPage);
  Portal.join({hello, update});

  Config.autoReconnect = true;
  Config.immediateStart = false;
  Config.apid = DEFAULT_AP_SSID;
  Config.psk = DEFAULT_AP_PASSWORD;
  Config.hostName = HOST_NAME;
  Config.homeUri = "/_ac";
  Config.bootUri = AC_ONBOOTURI_ROOT;
  Portal.config(Config);
  Portal.begin();
  // while (true)
  //  Portal.handleClient();
}

void startWifiWithoutWebServer()
{
  WiFi.mode(WIFI_MODE_STA);
  WiFi.begin();
  // WiFi.waitForConnectResult();
  for (wifiTimeout = 0; wifiTimeout < 20; wifiTimeout++)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.println("Success!");
      Serial.println(WiFi.localIP());
      break;
    }
    Serial.print(".");
    delay(500);
  }
  if (wifiTimeout >= 20)
    Serial.println("Failed!");
}
