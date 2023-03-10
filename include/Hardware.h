#include <Arduino.h>
#include <DS3231.h>
#include <FastLED.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>

/*---------------LED----------------*/

#define NUM_LEDS 4
#define LED_TYPE WS2812B
#define COLOR_ORDER GRB
#define DATA_PIN 26
#define LED_BRIGHTNESS 45

/*-------------Nixie---------------*/

#define NUM_PIN_A 18
#define NUM_PIN_B 19
#define HV_ENABLE 27

/*--------------IR-----------------*/

#define IR_PIN 25

/*-----------Sensor, RTC-----------*/
