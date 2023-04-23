#include <global_var.h>

void Settings::init()
{
    begin("pref");
    mColor.hue = getUChar("hue", 0);
    mColor.sat = getUChar("sat", 255);
    mColor.val = getUChar("val", 128);

    mClose_time[0] = getUChar("c_h", 0) / 10;
    mClose_time[1] = getUChar("c_h", 0) % 10;
    mClose_time[2] = getUChar("c_m", 0) / 10;
    mClose_time[3] = getUChar("c_m", 0) % 10;

    mOpen_time[0] = getUChar("o_h", 0) / 10;
    mOpen_time[1] = getUChar("o_h", 0) % 10;
    mOpen_time[2] = getUChar("o_m", 0) / 10;
    mOpen_time[3] = getUChar("o_m", 0) % 10;
}

void Settings::putColor(CHSV color)
{
    putUChar("hue", color.hue);
    putUChar("sat", color.sat);
    putUChar("val", color.val);
    mColor = color;
}

CHSV Settings::getColor(void)
{
    mColor.hue = getUChar("hue", 0);
    mColor.sat = getUChar("sat", 255);
    mColor.val = getUChar("val", 128);
    return mColor;
}

void Settings::getTime(uint8_t *time, uint8_t on_off)
{
    if (on_off == CLOSE_TIME)
    {
        time[0] = getUChar("c_h", 0) / 10;
        time[1] = getUChar("c_h", 0) % 10;
        time[2] = getUChar("c_m", 0) / 10;
        time[3] = getUChar("c_m", 0) % 10;
    }
    else if (on_off == OPEN_TIME)
    {
        time[0] = getUChar("o_h", 0) / 10;
        time[1] = getUChar("o_h", 0) % 10;
        time[2] = getUChar("o_m", 0) / 10;
        time[3] = getUChar("o_m", 0) % 10;
    }
}

void Settings::putTime(const uint8_t *time, uint8_t on_off)
{
    if (on_off == CLOSE_TIME)
    {
        putUChar("c_h", time[0] * 10 + time[1]);
        putUChar("c_m", time[2] * 10 + time[3]);
    }
    else if (on_off == OPEN_TIME)
    {
        putUChar("o_h", time[0] * 10 + time[1]);
        putUChar("o_h", time[2] * 10 + time[3]);
    }
}