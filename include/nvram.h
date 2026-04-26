#ifndef _SYS_NVRAM_H_
#define _SYS_NVRAM_H_

#include "stdint.h"

struct nvram_t {
    uint8_t rtc_sec;
    uint8_t rtc_sec_alarm;
    uint8_t rtc_min;
    uint8_t rtc_min_alarm;
    uint8_t rtc_hour;
    uint8_t rtc_hour_alarm;
    uint8_t rtc_weekday;
    uint8_t rtc_day;
    uint8_t rtc_month;
    uint8_t rtc_year;
    uint8_t status_reg_a;
    uint8_t status_reg_b;
    uint8_t status_reg_c;
    uint8_t status_reg_d;
    uint8_t diag_status;
    uint8_t cmos_shutdown;
    uint8_t floppy_drive;
    uint8_t system_flags;
    uint8_t checksum_hi;
    uint8_t checksum_lo;
    uint8_t base_mem_lo;
    uint8_t base_mem_hi;
    uint8_t ext_mem_lo;
    uint8_t ext_mem_hi;
    uint8_t ext_mem2_lo;
    uint8_t ext_mem2_hi;
    uint8_t reserved[8];
    uint8_t ps2_century;
    uint8_t cksum[2];
    uint8_t least_sig_byte_ext_actual;
    uint8_t most_sig_byte_ext_actual;
    uint8_t century_BCD;
    uint8_t post_flags;
    uint8_t bios_opt_flags[2];
    uint8_t rtc_alarm_status;
    uint8_t century;
} __attribute__((packed));

void readNVRAM(uint8_t *out);
void writeNVRAM(uint8_t *in);
void rtc_get_time(uint8_t *sec, uint8_t *min, uint8_t *hour,
                  uint8_t *day, uint8_t *month, uint8_t *year,
                  uint8_t *century);
uint8_t bcd(uint8_t v);

#endif
