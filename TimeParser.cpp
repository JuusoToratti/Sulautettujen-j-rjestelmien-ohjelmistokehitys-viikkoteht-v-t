#include <stdlib.h>
#include <string.h>
#include "TimeParser.h"
#include <ctype.h>

// time format: HHMMSS (6 characters)
int time_parse(const char *time) {

    if (time == NULL) {
        return TIME_NULL_ERROR;   
    }

    if (strlen(time) != 6) {
        return TIME_LEN_ERROR;    
    }

    // Tarkistetaan että kaikki merkit ovat numeroita
    for (int i = 0; i < 6; i++) {
        if (time[i] < '0' || time[i] > '9') {
            return TIME_CHAR_ERROR;   
        }
    }

    // Parsitaan luvut ilman että muokataan alkuperäistä merkkijonoa
    char buf[3];
    buf[2] = '\0';

    strncpy(buf, time, 2);   // HH
    int hh = atoi(buf);

    strncpy(buf, time+2, 2); // MM
    int mm = atoi(buf);

    strncpy(buf, time+4, 2); // SS
    int ss = atoi(buf);

    // boundary checks
    if (hh < 0 || hh > 23) {
        return TIME_HOUR_ERROR;
    }
    if (mm < 0 || mm > 59) {
        return TIME_MIN_ERROR;
    }
    if (ss < 0 || ss > 59) {
        return TIME_SEC_ERROR;
    }

    int total = hh * 3600 + mm * 60 + ss;

     if (total == 0) {
        return TIME_ZERO_ERROR;
    }
     return total;
}
