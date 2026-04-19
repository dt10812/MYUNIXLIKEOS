#include "stdint.h"
#include "stdbool.h"
#include "io.h"
#include "sys/time.h"
#include "nvram.h"

#define CMOS_ADDR_PORT 0x70
#define CMOS_DATA_PORT 0x71
#define NMI_DISABLE    0x80
#define RTC_STATUS_A   0x0A
#define RTC_UIP        0x80  /* Update In Progress bit */

static uint8_t rtc_read(uint8_t reg) {
    outb(CMOS_ADDR_PORT, NMI_DISABLE | reg);
    io_wait();
    return inb(CMOS_DATA_PORT);
}

static uint8_t rtc_bcd_to_bin(uint8_t value) {
    return (value & 0x0F) + ((value >> 4) * 10);
}

static bool rtc_is_valid_time(uint8_t sec, uint8_t min, uint8_t hour,
                              uint8_t day, uint8_t month, uint8_t year) {
    if (sec > 59 || min > 59 || hour > 23 || month < 1 || month > 12 || day < 1 || day > 31 || year > 99)
        return false;

    if (month == 2)
        return day <= 29;

    if (month == 4 || month == 6 || month == 9 || month == 11)
        return day <= 30;

    return true;
}

void rtc_get_time(uint8_t *sec, uint8_t *min, uint8_t *hour,
                  uint8_t *day, uint8_t *month, uint8_t *year,
                  uint8_t *century) {
    uint8_t raw1[7];
    uint8_t raw2[7];
    uint8_t statusB1;
    uint8_t statusB2;

    do {
        while (rtc_read(RTC_STATUS_A) & RTC_UIP)
            io_wait();

        raw1[0] = rtc_read(0x00);
        raw1[1] = rtc_read(0x02);
        raw1[2] = rtc_read(0x04);
        raw1[3] = rtc_read(0x07);
        raw1[4] = rtc_read(0x08);
        raw1[5] = rtc_read(0x09);
        raw1[6] = rtc_read(0x32);
        statusB1 = rtc_read(0x0B);

        while (rtc_read(RTC_STATUS_A) & RTC_UIP)
            io_wait();

        raw2[0] = rtc_read(0x00);
        raw2[1] = rtc_read(0x02);
        raw2[2] = rtc_read(0x04);
        raw2[3] = rtc_read(0x07);
        raw2[4] = rtc_read(0x08);
        raw2[5] = rtc_read(0x09);
        raw2[6] = rtc_read(0x32);
        statusB2 = rtc_read(0x0B);
    } while (statusB1 != statusB2 || raw1[0] != raw2[0] || raw1[1] != raw2[1] ||
             raw1[2] != raw2[2] || raw1[3] != raw2[3] || raw1[4] != raw2[4] ||
             raw1[5] != raw2[5] || raw1[6] != raw2[6]);

    uint8_t century_raw = raw1[6];
    if (century_raw == 0 || century_raw == 0xFF)
        century_raw = 20;

    bool is_bcd = !(statusB1 & 0x04);
    bool is_24h = statusB1 & 0x02;
    bool is_pm = raw1[2] & 0x80;
    uint8_t hour_raw = raw1[2] & 0x7F;

    if (is_bcd)
        hour_raw = rtc_bcd_to_bin(hour_raw);

    if (!is_24h) {
        if (is_pm) {
            if (hour_raw != 12)
                hour_raw += 12;
        } else if (hour_raw == 12) {
            hour_raw = 0;
        }
    }

    if (is_bcd) {
        *sec = rtc_bcd_to_bin(raw1[0]);
        *min = rtc_bcd_to_bin(raw1[1]);
        *hour = hour_raw;
        *day = rtc_bcd_to_bin(raw1[3]);
        *month = rtc_bcd_to_bin(raw1[4]);
        *year = rtc_bcd_to_bin(raw1[5]);
        *century = rtc_bcd_to_bin(century_raw);
    } else {
        *sec = raw1[0];
        *min = raw1[1];
        *hour = hour_raw;
        *day = raw1[3];
        *month = raw1[4];
        *year = raw1[5];
        *century = century_raw;
    }

    if (!rtc_is_valid_time(*sec, *min, *hour, *day, *month, *year)) {
        *sec = 0;
        *min = 0;
        *hour = 12;
        *day = 18;
        *month = 4;
        *year = 24;
        *century = 20;
    }
}
