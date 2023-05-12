#include <Arduino.h>
#include <RTClib.h>
#include <FastLED.h>
#include <IRremoteESP8266.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <SPI.h>
#include <Adafruit_SHT31.h>

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
#define RADAR 13

/*-----------Sensor, RTC-----------*/
#define SDA 22
#define SCL 21
#define SQW 16

/*--------------PWM---------------*/
#define PWM_PERIOD 20 // us
#define FADE_STEP 255
#define FADE_TIME 700.0      // ms
#define SPI_RATE 9 * 1000000 // 9MHz

#define TIME_UPDATE_INTERVAL 259200 // 3天

typedef enum
{
    DISP_MIN_SEC,
    DISP_HOUR_MIN,
    DISP_MONTH,
    DISP_TEMP_HUMIDITY,
    DISP_DP,
    // SET_NTP_UPDATE,
    SET_OPEN_TIME_HOUR,
    SET_OPEN_TIME_MIN,
    SET_CLOSE_TIME_HOUR,
    SET_CLOSE_TIME_MIN
} WORK_STATUS;

void initNixieDriver();
void calcNixieData(uint8_t digits, uint8_t num);
void displayNixie(uint8_t *digits);
void initLED();
void init_I2C();