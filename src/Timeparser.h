#pragma once
#ifndef TIMEPARSER_H
#define TIMEPARSER_H

#ifdef __cplusplus
extern "C" {
#endif

/* Palautusarvot:
 * >= 0 : aika sekunteina
 * <  0 : virhekoodit
 */
#define TIME_NULL_ERROR   -1   /* NULL pointteri */
#define TIME_LEN_ERROR    -2   /* väärä pituus */
#define TIME_CHAR_ERROR   -3   /* ei-numero merkkejä */
#define TIME_HOUR_ERROR   -4   /* tunnit virheelliset */
#define TIME_MIN_ERROR    -5   /* minuutit virheelliset */
#define TIME_SEC_ERROR    -6   /* sekunnit virheelliset */
#define TIME_ZERO_ERROR   -7   /* aika-arvo ei saa olla 0 */

/* Funktio: parsii merkkijonon muodossa "HHMMSS" */
int time_parse(const char *time);

#ifdef __cplusplus
}
#endif

#endif /* TIMEPARSER_H */
