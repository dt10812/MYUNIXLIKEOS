#include "commands.h"
#include "stdio.h"
#include "sys/time.h"

int cmd_date(int argc, char** argv) {
    (void)argc;
    (void)argv;
    time_t t;
    time(&t);
    struct tm bt;
    
    const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    
    localtime_r(&t, &bt);
    
    printf("%s %s %2d %02d:%02d:%02d UTC %d\n", 
           days[bt.tm_wday],
           months[bt.tm_mon],
           bt.tm_mday,
           bt.tm_hour,
           bt.tm_min,
           bt.tm_sec,
           bt.tm_year);
    
    return 0;
}
