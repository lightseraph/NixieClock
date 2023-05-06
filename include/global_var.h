#include <Arduino.h>
#include "Preferences.h"
#include <FastLED.h>
#include <RTClib.h>

#define CLOSE_TIME 1
#define OPEN_TIME 2

class Settings : public Preferences
{
public:
    void init();

    void putColor(CHSV color);
    CHSV getColor(void);
    void getTime(uint8_t *time, uint8_t on_off);
    void putTime(const uint8_t *time, uint8_t on_off);
    void getDPTime(uint8_t *time);
    void putDPTime(const uint8_t *time);
    uint8_t getDisplayMode(void);
    void putDisplayMode(uint8_t mode);
    uint32_t getLastUpdate(void);
    void putLastUpdate(uint32_t secondstime);

private:
    CHSV mColor;
    uint8_t mClose_time[4];
    uint8_t mOpen_time[4];
};