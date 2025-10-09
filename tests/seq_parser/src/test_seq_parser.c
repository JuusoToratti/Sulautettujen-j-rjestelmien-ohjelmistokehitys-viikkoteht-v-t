#include <zephyr/ztest.h>
#include "seq_parser.h"

ZTEST(seq_parser, test_simple_letters)
{
    struct seq_item items[10];
    int count;
    int ret = parse_sequence("RYG", items, 10, &count);

    zassert_equal(ret, 0, "Parser failed");
    zassert_equal(count, 3, "Wrong count");
    zassert_equal(items[0].color, 'R', "Wrong color");
    zassert_equal(items[1].color, 'Y', "Wrong color");
    zassert_equal(items[2].color, 'G', "Wrong color");
}

ZTEST(seq_parser, test_with_durations)
{
    struct seq_item items[10];
    int count;
    int ret = parse_sequence("R,1000,Y,500,G,1000", items, 10, &count);

    zassert_equal(ret, 0, "Parser failed");
    zassert_equal(count, 3, "Wrong count");
    zassert_equal(items[0].duration_ms, 1000, "Wrong duration");
    zassert_equal(items[1].duration_ms, 500, "Wrong duration");
    zassert_equal(items[2].duration_ms, 1000, "Wrong duration");
}

ZTEST(seq_parser, test_invalid_char)
{
    struct seq_item items[10];
    int count;
    int ret = parse_sequence("RXG", items, 10, &count);

    zassert_not_equal(ret, 0, "Should fail for invalid char");
}

ZTEST_SUITE(seq_parser, NULL, NULL, NULL, NULL, NULL);
