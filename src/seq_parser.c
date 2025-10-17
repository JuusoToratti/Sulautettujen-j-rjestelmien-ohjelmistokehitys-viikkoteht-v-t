#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include "seq_parser.h"
#include <string.h>
#include <ctype.h>

#define SEQ_INVALID_CHAR_ERROR  -7
#define SEQ_EMPTY_ERROR         -8
#define SEQ_TOO_LONG_ERROR      -9

#define VALID_CHARS "RYG"
#define MAX_SEQ_LEN 20

int parse_sequence(const char *input, struct seq_item *items, int max_items, int *out_count) {
    *out_count = 0;

    if (input == NULL || strlen(input) == 0) {
        printk("%dX", SEQ_EMPTY_ERROR);
        return SEQ_EMPTY_ERROR;
    }

    if (strlen(input) > MAX_SEQ_LEN) {
        printk("%dX", SEQ_TOO_LONG_ERROR);
        return SEQ_TOO_LONG_ERROR;
    }

    const char *p = input;
    while (*p && *out_count < max_items) {
        if (strchr(VALID_CHARS, *p) == NULL) {
            printk("%dX", SEQ_INVALID_CHAR_ERROR);
            return SEQ_INVALID_CHAR_ERROR;
        }
        items[*out_count].color = *p;
        items[*out_count].duration_ms = 500;
        (*out_count)++;
        p++;
    }

    printk("0X");
    return 0;
}
