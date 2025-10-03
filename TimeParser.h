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
#define TIME_NULL_ERROR  -1  /* NULL pointteri */
#define TIME_LEN_ERROR   -2  /* pituus != 6 */
#define TIME_HOUR_ERROR  -3  /* tunti out of range */
#define TIME_MIN_ERROR   -4  /* minuutti out of range */
#define TIME_SEC_ERROR   -5  /* sekunti out of range */
#define TIME_CHAR_ERROR  -6  /* virheellinen merkki */

/* Funktio: parsii merkkijonon muodossa "HHMMSS" */
int time_parse(const char *time);

#ifdef __cplusplus
}
#endif

#endif /* TIMEPARSER_H */
