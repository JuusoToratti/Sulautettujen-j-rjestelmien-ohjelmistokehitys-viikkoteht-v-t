#include "gtest/gtest.h"
#include "TimeParser.h"

// validit ajat
TEST(TimeParserTests, ValidTimes) {
    EXPECT_EQ(time_parse("235959"), 23*3600 + 59*60 + 59);
    EXPECT_EQ(time_parse("124033"), 12*3600 + 40*60 + 33);
}

// väärä pituus
TEST(TimeParserTests, InvalidLength) {
    EXPECT_EQ(time_parse("12345"), TIME_LEN_ERROR);
    EXPECT_EQ(time_parse("1234567"), TIME_LEN_ERROR);
}

// null pointteri
TEST(TimeParserTests, NullPointer) {
    EXPECT_EQ(time_parse(NULL), TIME_NULL_ERROR);
}

// tunnit virheelliset
TEST(TimeParserTests, InvalidHours) {
    EXPECT_EQ(time_parse("246000"), TIME_HOUR_ERROR);
    EXPECT_EQ(time_parse("240000"), TIME_HOUR_ERROR);
}

// minuutit virheelliset
TEST(TimeParserTests, InvalidMinutes) {
    EXPECT_EQ(time_parse("126000"), TIME_MIN_ERROR);
}

// sekunnit virheelliset
TEST(TimeParserTests, InvalidSeconds) {
    EXPECT_EQ(time_parse("125960"), TIME_SEC_ERROR);
}

// väärät merkit
TEST(TimeParserTests, InvalidCharacters) {
    EXPECT_EQ(time_parse("12AB30"), TIME_CHAR_ERROR);
    EXPECT_EQ(time_parse("12AB56"), TIME_CHAR_ERROR);
    EXPECT_EQ(time_parse("123 56"), TIME_CHAR_ERROR);
}

// nolla-aika (ei hyväksytty)
TEST(TimeParserTests, ZeroTime) {
    EXPECT_EQ(time_parse("000000"), TIME_ZERO_ERROR);
}

// laajennetut validit ajat
TEST(TimeParserTests, ValidTimesExtended) {
    EXPECT_EQ(time_parse("000001"), 1);    // 1 sekunti
    EXPECT_EQ(time_parse("010000"), 3600); // 1h
}
