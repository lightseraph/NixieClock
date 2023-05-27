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
    uint8_t getDPTime(void);
    void putDPTime(const uint8_t interval);
    uint8_t getDisplayMode(void);
    void putDisplayMode(uint8_t mode);
    uint32_t getLastUpdate(void);
    void putLastUpdate(uint32_t secondstime);
    uint16_t getTerror(void);
    void putTerror(const uint16_t tError);
    uint16_t getHerror(void);
    void putHerror(const uint16_t hError);
    void putWorkingTime(const uint32_t working_time);
    uint32_t getWorkingTime(void);
    uint32_t mLastupdate;
    uint8_t mClose_time[4];
    uint8_t mOpen_time[4];
    uint16_t mTerror;
    uint16_t mHerror;
    uint32_t mWorkingTime;

private:
    CHSV mColor;
};