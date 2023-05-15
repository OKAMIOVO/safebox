#ifndef __TIME_H__
#define __TIME_H__
#include <stdint.h>
enum TimeDispFormat {
    TIME_HEX,
    TIME_BCD,
};
struct Time {
    enum TimeDispFormat format;
    uint8_t year;
    uint8_t month;
    uint8_t date;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};
struct Time GetDateTimeFromSecond(const int secTotal);
void OutputTime(const struct Time* pTime);
void TimeFormatConvert(struct Time* pTime, enum TimeDispFormat format);
#endif
