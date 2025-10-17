#include <stdlib.h>
#include <string.h>
#include "TimeParser.h"
#include <ctype.h>
#include <zephyr/sys/printk.h>

// Virhekoodit:
#define TIME_LEN_ERROR   -1
#define TIME_ZERO_ERROR  -2
#define TIME_CHAR_ERROR  -3
#define TIME_HOUR_ERROR  -4
#define TIME_MIN_ERROR   -5
#define TIME_SEC_ERROR   -6
#define TIME_NULL_ERROR  -10  // ei käytetä testeissä, mutta mukana varmuuden vuoksi

// --- time_parse ---
// Palauttaa sekuntimäärän (0–86399) tai negatiivisen virhekoodin
int time_parse(const char *time)
{
    if (time == NULL) {
        return TIME_NULL_ERROR;
    }

    int len = strlen(time);
    if (len != 6) {
        return TIME_LEN_ERROR;  // liian lyhyt tai pitkä
    }

    // Tarkista että kaikki merkit ovat numeroita
    for (int i = 0; i < 6; i++) {
        if (!isdigit((unsigned char)time[i])) {
            return TIME_CHAR_ERROR;
        }
    }

    char buf[3];
    buf[2] = '\0';

    strncpy(buf, time, 2);
    int hh = atoi(buf);

    strncpy(buf, time + 2, 2);
    int mm = atoi(buf);

    strncpy(buf, time + 4, 2);
    int ss = atoi(buf);

    // Tarkista aikakomponentit
    if (hh < 0 || hh > 23) {
        return TIME_HOUR_ERROR;
    }
    if (mm < 0 || mm > 59) {
        return TIME_MIN_ERROR;
    }
    if (ss < 0 || ss > 59) {
        return TIME_SEC_ERROR;
    }

    // Tarkista nolla-aika
    if (hh == 0 && mm == 0 && ss == 0) {
        return TIME_ZERO_ERROR;
    }

    // Lasketaan sekunnit
    int total = hh * 3600 + mm * 60 + ss;
    return total;
}
