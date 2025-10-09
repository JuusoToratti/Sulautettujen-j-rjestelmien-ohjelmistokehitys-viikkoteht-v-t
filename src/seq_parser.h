// src/seq_parser.h
#pragma once
#include <zephyr/kernel.h>

struct seq_item {
    char color;         // 'R','Y','G'
    int duration_ms;    // millisekunneissa
};

int parse_sequence(const char *input, struct seq_item *items, int max_items, int *out_count);
