#include <Arduino.h>
#include <DS3231.h>
#include <FastLED.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <SPI.h>

/*---------------LED----------------*/

#define NUM_LEDS 4
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define LED_DATA 27
#define LED_BRIGHTNESS 128

/*-------------Nixie---------------*/

#define HV_ENABLE 26
#define DOT 33
#define NIXIE_DATA 19
#define NIXIE_CLK 17
#define NIXIE_LATCH 18
#define IN3_COMMA 25
#define IN2_COMMA 14

/*--------------IR-----------------*/

#define IR_PIN 23

/*-----------Sensor, RTC-----------*/
#define SDA 22
#define SCL 21
#define SQW 16

void initNixieDriver();
void calcNixieData(uint8_t digits, uint8_t num);
void displayNixie();
void initLED();