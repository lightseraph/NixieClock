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

// 设置LED灯色
void Settings::putColor(CHSV color)
{
    putUChar("hue", color.hue);
    putUChar("sat", color.sat);
    putUChar("val", color.val);
    mColor = color;
}

// 读取LED灯色
CHSV Settings::getColor(void)
{
    mColor.hue = getUChar("hue", 0);
    mColor.sat = getUChar("sat", 255);
    mColor.val = getUChar("val", 128);
    return mColor;
}

// 读取定时开关机时间
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

// 设置定时开关机时间
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

// 读取阴级解毒间隔时间
uint8_t Settings::getDPTime(void)
{
    return getUChar("dp_time", 5);
}

// 设置阴级解毒间隔时间
void Settings::putDPTime(const uint8_t interval)
{
    putUChar("dp_time", interval);
}

// 读取显示模式
// 0：淡入淡出切换数字
// 1：直接切换数字
uint8_t Settings::getDisplayMode(void)
{
    return getUChar("disp_mode", 0);
}

// 设置显示模式
void Settings::putDisplayMode(uint8_t mode)
{
    putUChar("disp_mode", mode);
}

// 读取最后一次NTP更新时间
uint32_t Settings::getLastUpdate(void)
{
    return getUInt("last_update", 0);
}

// 设置最后一次NTP更新时间
void Settings::putLastUpdate(uint32_t secondstime)
{
    putUInt("last_update", secondstime);
}