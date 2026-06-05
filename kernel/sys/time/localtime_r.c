#include "sys/time.h"
#include "stdbool.h"
#include <stdint.h>

struct tm *localtime_r(const time_t *timep, struct tm *result) {
    if (!timep || !result) return NULL;

    time_t t = *timep;
    if (t < 0) {
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

    #define IS_LEAP(y) (((y) % 4 == 0 && (y) % 100 != 0) || ((y) % 400 == 0))
    static const int month_days[12] = {
        31, 28, 31, 30, 31, 30,
        31, 31, 30, 31, 30, 31
    };

    time_t days = t / 86400;
    time_t seconds_of_day = t % 86400;
    if (seconds_of_day < 0) seconds_of_day += 86400;

    int year = 1970;
    while (1) {
        int days_in_year = IS_LEAP(year) ? 366 : 365;
        if (days < days_in_year) break;
        days -= days_in_year;
        year++;
    }

    result->tm_year = year - 1900;
    result->tm_yday = (int)days;
    result->tm_wday = (int)((days + 4) % 7);
    if (result->tm_wday < 0) result->tm_wday += 7;

    int month = 0;
    int day_of_year = (int)days;
    while (month < 12) {
        int dim = month_days[month];
        if (month == 1 && IS_LEAP(year)) dim++;
        if (day_of_year < dim) break;
        day_of_year -= dim;
        month++;
    }

    result->tm_mon = month;
    result->tm_mday = day_of_year + 1;
    result->tm_hour = (int)(seconds_of_day / 3600);
    result->tm_min = (int)((seconds_of_day % 3600) / 60);
    result->tm_sec = (int)(seconds_of_day % 60);
    result->tm_isdst = -1;
    return result;
}
