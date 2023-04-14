#include <Hardware.h>

const uint8_t digit_offset[4][10] = {
    {41, 38, 7, 23, 39, 8, 24, 40, 9, 25},
    {22, 19, 35, 4, 20, 36, 5, 21, 37, 6},
    {3, 32, 16, 0, 1, 17, 33, 2, 18, 34},
    {10, 44, 13, 28, 12, 43, 27, 11, 42, 26}};

uint32_t nixieBitData_H = 0;
uint32_t nixieBitData_L = 0;
uint64_t nixieBitData = 0;

CRGB leds[NUM_LEDS];
CHSV color;

void initLED()
{
    LEDS.addLeds<LED_TYPE, LED_DATA, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(LED_BRIGHTNESS);
    fill_solid(leds, 4, CRGB::Black);
    FastLED.show();
}

void initNixieDriver()
{
    pinMode(32, INPUT_PULLDOWN);
    pinMode(NIXIE_CLK, OUTPUT);
    pinMode(NIXIE_DATA, OUTPUT);
    pinMode(NIXIE_LATCH, OUTPUT);
    digitalWrite(NIXIE_LATCH, HIGH);

    SPI.begin(NIXIE_CLK, 32, NIXIE_DATA, NIXIE_LATCH);
    SPI.beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
    SPI.setHwCs(false);
}

void calcNixieData(uint8_t digits, uint8_t num)
{
    uint8_t offset = digit_offset[digits - 1][num];
    nixieBitData = 0;

    nixieBitData |= ((uint64_t)1) << offset;
    nixieBitData_H = (nixieBitData >> 32); //& 0x0000FFFF;
    nixieBitData_L = (nixieBitData & 0x00000000FFFFFFFF);
}

void displayNixie()
{
    digitalWrite(NIXIE_LATCH, LOW);
    delayMicroseconds(1);

    SPI.transfer32(nixieBitData_H);
    SPI.transfer32(nixieBitData_L);

    digitalWrite(NIXIE_LATCH, HIGH);
}
