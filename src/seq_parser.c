#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "seq_parser.h"

int parse_sequence(const char *input, struct seq_item *items, int max_items, int *out_count) {
    int count = 0;
    const char *p = input;

    while (*p && count < max_items) {
        char color;
        int duration = 500; // oletusaika, jos ei määritetty

        if (*p == 'R' || *p == 'Y' || *p == 'G') {
            color = *p++;
        } else {
            return -1; // virheellinen merkki
        }

        if (*p == ',') {
            p++;
            if (sscanf(p, "%d", &duration) != 1) {
                return -2; // virheellinen numero
            }
            while (*p >= '0' && *p <= '9') p++;
        }

        items[count].color = color;
        items[count].duration_ms = duration;
        count++;

        if (*p == ',') p++;
    }

    *out_count = count;
    return 0;
}
