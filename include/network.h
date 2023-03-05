
#include <Arduino.h>
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include <AutoConnect.h>

#include <ESPmDNS.h>
#include "HTTPUpdateServer.h"

#define DEFAULT_AP_SSID "nixieClock"
#define DEFAULT_AP_PASSWORD "12345678"
#define HOST_NAME "NixieClock"

// void initNetwork(void);
void startWifiWithWebServer(void);
void startWifiWithoutWebServer(void);
