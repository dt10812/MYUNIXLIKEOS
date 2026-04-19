#include "sys/time.h"
#include "stdbool.h"
#include <stdint.h>

struct tm *localtime_r(const time_t *timep, struct tm *result){
    uint8_t days_per_month[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };
    #define IS_LEAP(y) (((y) % 4 == 0 && (y) % 100 != 0) || ((y) % 400 == 0))

    time_t t = *timep;
    if (t < 0) {
        // For negative time (pre-1970), approximate as 1919-04-19 02:02:02
        result->tm_year = 19;
        result->tm_mon = 3;
        result->tm_mday = 19;
        result->tm_hour = 2;
        result->tm_min = 2;
        result->tm_sec = 2;
        result->tm_wday = 0;
        result->tm_yday = 0;
        result->tm_isdst = -1;
        return result;
    }

    uint32_t total_days = 0;
    time_t sec_of_year = t;
    int y = 1970;
    for (; sec_of_year >= (IS_LEAP(y) ? 31622400 : 31536000); y++) {
        total_days += IS_LEAP(y) ? 366 : 365;
        sec_of_year -= IS_LEAP(y) ? 31622400 : 31536000;
    }
    bool leap = IS_LEAP(y);
    result->tm_year = y - 1900;

    uint32_t soy = (uint32_t)sec_of_year;
    uint32_t yday = soy / 86400;
    total_days += yday;
    result->tm_yday = (int)yday;

    if (leap)
        days_per_month[1] = 29;

    uint8_t m = 0;
    for (; m < 12; m++){
        if (days_per_month[m] > yday)
            break;
        yday -= days_per_month[m];
    }
    result->tm_mon  = m;
    result->tm_mday = (int)yday + 1;

    uint32_t sec_of_day = soy % 86400;
    uint32_t hour = sec_of_day / 3600;
    uint32_t min  = (sec_of_day / 60) - (hour * 60);
    uint32_t sec  = sec_of_day % 60;
    result->tm_hour = (int)hour;
    result->tm_min  = (int)min;
    result->tm_sec  = (int)sec;

    result->tm_wday  = (int)((total_days + 4) % 7);
    result->tm_isdst = -1;
    return result;
}