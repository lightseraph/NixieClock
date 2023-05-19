#include <Hardware.h>

const uint8_t digit_offset[4][11] = {
    {41, 38, 7, 23, 39, 8, 24, 40, 9, 25, 14},
    {22, 19, 35, 4, 20, 36, 5, 21, 37, 6, 14},
    {3, 32, 16, 0, 1, 17, 33, 2, 18, 34, 14},
    {10, 44, 13, 28, 12, 43, 27, 11, 42, 26, 14}};

uint32_t nixieBitData_H = 0;
uint32_t nixieBitData_L = 0;
uint64_t nixieBitData = 0;

CRGB leds[NUM_LEDS];
// CHSV color;

RTC_DS3231 rtc;
Adafruit_SHT31 sht31 = Adafruit_SHT31();

void init_I2C()
{
    pinMode(SQW, INPUT_PULLUP);
    Wire.begin(SDA, SCL, 400000);
    if (!rtc.begin())
    {
        Serial.println("Couldn't find RTC");
        Serial.flush();
        while (1)
            delay(100);
    }
    rtc.disable32K();
    rtc.writeSqwPinMode(DS3231_SquareWave1Hz);

    if (!sht31.begin())
    {
        Serial.println("Couldn't find SHT");
        Serial.flush();
        while (1)
            delay(100);
    }
}

void initLED()
{
    LEDS.addLeds<LED_TYPE, LED_DATA, COLOR_ORDER>(leds, NUM_LEDS);
    FastLED.setBrightness(LED_BRIGHTNESS);
    fill_solid(leds, 4, CRGB::Black);
    FastLED.show();
}

void initNixieDriver()
{
    pinMode(32, INPUT_PULLDOWN); // SPI输入脚空置，拉低
    pinMode(NIXIE_CLK, OUTPUT);
    pinMode(NIXIE_DATA, OUTPUT);
    pinMode(NIXIE_LATCH, OUTPUT);
    digitalWrite(NIXIE_LATCH, HIGH);

    SPI.begin(NIXIE_CLK, 32, NIXIE_DATA, NIXIE_LATCH);
    SPI.beginTransaction(SPISettings(SPI_RATE, MSBFIRST, SPI_MODE0));
    SPI.setHwCs(false);
    displayNixie(fadein_blank);
}

void displayNixie(const uint8_t *digits)
{
    nixieBitData = 0;
    for (int i = 0; i < 4; i++)
    {
        uint8_t offset = digit_offset[i][digits[i]];
        nixieBitData |= ((uint64_t)1) << offset;
    }
    nixieBitData_H = (nixieBitData >> 32);
    nixieBitData_L = (nixieBitData & 0x00000000FFFFFFFF);

    digitalWrite(NIXIE_LATCH, LOW);
    delayMicroseconds(1);

    SPI.transfer32(nixieBitData_H);
    SPI.transfer32(nixieBitData_L);

    digitalWrite(NIXIE_LATCH, HIGH);
}
