#include "gtest/gtest.h"
#include "TimeParser.h"

/*1p suoritus: Yksinkertaiset testit ja parseri sulautetussa ohjelmassa

Kooditiedostossa TimeParser.cpp on siis funktio time_parse(char *time), jossa merkkijono time pitäisi tarkistaa. Funktio palauttaa joko virhekoodin (katso TimeParser.h) tai aikamerkkijonossa annetun ajan sekunteina, johon arvoon siis ajastin alustetaan. Virhekoodit ovat kaikki negatiivisia, jotta ne erottuvat aika-arvosta. 

Mutta, siitä puuttuu ainakin boundary value-testit, jotka pitäisi itse lisätä koodiin (hakemistossa test_cases/TimeParserTests.cpp). Eli siis testata että puretut aika-arvot ovat laillisia: mikään aika-arvo ei ole negatiivinen, sekuntien arvo ei ole yli 59, tuntien arvo yli 23, jne. (Tarkistukset tehdään vaikka näitä arvoja nyt ei liikennevalo-ohjelmassa käytetäkään). 

Esimerkki: jos merkkijono on 000120 -> saadaan 1 minuutti ja 20 sekuntia -> yhteensä 80 sekuntia mikä arvo palautetaan funktiosta ja asetetaan ajastimeen.

Kun time_parse-funktio on muokattu ja vastaavat testikeissit tehty ja katsottu että testit menevät läpi,*/




TEST(TimeParserTests, ValidTimes) {
    EXPECT_EQ(time_parse("000000"), 0);
    EXPECT_EQ(time_parse("235959"), 23*3600 + 59*60 + 59);
    EXPECT_EQ(time_parse("124033"), 12*3600 + 40*60 + 33);
}

TEST(TimeParserTests, InvalidLength) {
    EXPECT_EQ(time_parse("12345"), TIME_LEN_ERROR);
    EXPECT_EQ(time_parse("1234567"), TIME_LEN_ERROR);
}

TEST(TimeParserTests, NullPointer) {
    EXPECT_EQ(time_parse(NULL), TIME_NULL_ERROR);
}

TEST(TimeParserTests, InvalidHours) {
    EXPECT_EQ(time_parse("246000"), TIME_HOUR_ERROR);
    EXPECT_EQ(time_parse("240000"), TIME_HOUR_ERROR);
}

TEST(TimeParserTests, InvalidMinutes) {
    EXPECT_EQ(time_parse("126000"), TIME_MIN_ERROR);
}

TEST(TimeParserTests, InvalidSeconds) {
    EXPECT_EQ(time_parse("125960"), TIME_SEC_ERROR);
}

TEST(TimeParserTests, InvalidCharacters) {
    EXPECT_EQ(time_parse("12AB30"), TIME_CHAR_ERROR);
}
