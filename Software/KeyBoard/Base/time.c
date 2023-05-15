#include "time.h"
#include "log.h"
#define YEAR_START (1970)
#define SECOND_OF_DAY (24 * 60 * 60)
static const uint8_t DayOfMon[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
#define HEX2BCD(hex) ((uint8_t)((hex / 10) << 4 | (hex % 10)))
#define BCD2HEX(bcd) ((uint8_t)((bcd >> 4) * 10 + (bcd & 0x0f)))
void TimeFormatConvert(struct Time* pTime, enum TimeDispFormat format)
{
    if (pTime->format == format) {
        return;
    }
    pTime->format = format;
    if (format == TIME_HEX) {
        pTime->year = BCD2HEX(pTime->year);
        pTime->month = BCD2HEX(pTime->month);
        pTime->date = BCD2HEX(pTime->date);
        pTime->hour = BCD2HEX(pTime->hour);
        pTime->minute = BCD2HEX(pTime->minute);
        pTime->second = BCD2HEX(pTime->second);
    } else if(format == TIME_BCD){
        pTime->year = HEX2BCD(pTime->year);
        pTime->month = HEX2BCD(pTime->month);
        pTime->date = HEX2BCD(pTime->date);
        pTime->hour = HEX2BCD(pTime->hour);
        pTime->minute = HEX2BCD(pTime->minute);
        pTime->second = HEX2BCD(pTime->second);
    }else {
			#ifdef FUNC_LOG
        PRINT("Time format illegal!!!\n");
			#endif
    }
    
}
void OutputTime(const struct Time* pTime)
{
	#ifdef FUNC_LOG
    if (pTime->format == TIME_HEX) {
        PRINT("time:%02d-%02d-%02d,%02d:%02d:%02d\n", pTime->year, pTime->month, pTime->date, pTime->hour, pTime->minute, pTime->second);
    } else {
        PRINT("time:%02x-%02x-%02x,%02x:%02x:%02x\n", pTime->year, pTime->month, pTime->date, pTime->hour, pTime->minute, pTime->second);
    }
	#endif
}
struct Time GetDateTimeFromSecond(const int secTotal)
{
    uint16_t year, j, iDay;
    struct Time tempTime;
    int lDay = secTotal / SECOND_OF_DAY;
    int lSec = secTotal % SECOND_OF_DAY;
    // __DATE__
    // __TIME__
    year = YEAR_START;
    while (lDay > 365) {
        if (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0)) /* ???? */
            lDay -= 366;
        else
            lDay -= 365;
        year++;
    }
    if ((lDay == 365) && !(((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0))) /* ??? */
    {
        lDay -= 365;
        year++;
    }
    tempTime.year = year % 100;
    for (j = 0; j < 12; j++) {
        if ((j == 1) && (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0)))
            iDay = 29;
        else
            iDay = DayOfMon[j];
        if (lDay >= iDay)
            lDay -= iDay;
        else
            break;
    }
    tempTime.format = TIME_HEX;
    tempTime.month = j + 1;
    tempTime.date = lDay + 1;
    tempTime.hour = (lSec / 3600) % 24 + 8;
    tempTime.minute = lSec % 3600 / 60;
    tempTime.second = lSec % 3600 % 60;
    return tempTime;
}
int GetSecFromTime(const struct Time* pTime)
{   
    struct Time tempTime = *pTime;
    TimeFormatConvert(&tempTime,TIME_HEX);
    return 0;
}
