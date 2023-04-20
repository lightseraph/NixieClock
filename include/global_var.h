#include <Arduino.h>
#include "Preferences.h"
#include <FastLED.h>

#define CLOSE_TIME 1
#define OPEN_TIME 2

class Settings : public Preferences
{
public:
    Settings();

    uint8_t open_hour;
    uint8_t open_minute;
    uint8_t close_hour;
    uint8_t close_minute;
    uint8_t updated_year;
    uint8_t updated_day;
    uint8_t updated_month;

    void putColor(CHSV color);
    CHSV getColor(void);
    void getTime(uint8_t *time, uint8_t on_off);
    void putTime(const uint8_t *time, uint8_t on_off);

private:
    CHSV mColor;
    uint8_t mClose_time[4];
    uint8_t mOpen_time[4];
};