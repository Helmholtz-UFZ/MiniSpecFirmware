/*
 * rtc_usr.c
 *
 *  Created on: Mar 6, 2019
 *      Author: Bert Palm
 */

#include "datetime.h"

#include "string.h"
#include "stdio.h"

rtc_t rtc = { .alarmA_wakeup = false };

/**
 * Parse a string to a date and time object.
 * The datetime string should be in ISO 8601 format and should look like this:
 * 20YY-MM-DDTHH:mm:ss
 * example: 2099-12-05T23:59:59
 *
 * Return 0 on success, non-zero otherwise
 */
uint8_t rtc_parsecheck_datetime(char* str, RTC_TimeTypeDef *sTime, RTC_DateTypeDef *sDate) {
	// todo change to take a timestamp instead of time and date

	char *p = str;
	uint c;
	uint16_t len;

	/* Scan year. */
	len = sscanf(p, "%u", &c);
	c = c >= 2000 ? c - 2000 : c;
	if (len == 1 && IS_RTC_YEAR(c)) {
		sDate->Year = c;

		/* Scan month
		 * search the '-', and let str point to the char right after it. */
		p = (char*) memchr(p, '-', 10);
		if (p) {
			p++;
			len = sscanf(p, "%u", &c);
			if (len == 1 && IS_RTC_MONTH(c)) {
				sDate->Month = c;

				/* Scan day of month*/
				/* search the '-', and let str point to the char right after it. */
				p = (char*) memchr(p, '-', 10);
				if (p) {
					p++;
					len = sscanf(p, "%u", &c);
					if (len == 1 && IS_RTC_DATE(c)) {
						sDate->Date = c;

						/* Parse the first time number*/
						/* search the 'T', and let str point to the char right after it. */
						p = (char*) memchr(p, 'T', 10);
						if (p) {
							p++;
							len = sscanf(p, "%u", &c);
							if (len == 1 && IS_RTC_HOUR24(c)) {
								sTime->Hours = c;

								/* Parse the second time number*/
								/* search the ':' */
								p = (char*) memchr(p, ':', 10);
								if (p) {
									p++;
									/* set pointer after ':' and scan number*/
									len = sscanf(p, "%u", &c);
									if (len == 1 && IS_RTC_MINUTES(c)) {
										sTime->Minutes = c;

										/* Parse the third time number*/
										/* search the ':' */
										p = (char*) memchr(p, ':', 10);
										if (p) {
											p++;
											/* set pointer after ':' and scan number*/
											len = sscanf(p, "%u", &c);
											if (len == 1 && IS_RTC_SECONDS(c)) {
												sTime->Seconds = c;
												return 0;
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	return 1;
}

/**
 * Parse a time string to a time object
 * The time string should be in like ISO 8601 Time format and should look like this:
 * HH:mm:ss, example: 23:59:59
 *
 * Return 0 on success, non-zero otherwise
 */
uint8_t rtc_parse_time(char *str, RTC_TimeTypeDef *sTime) {
	char *p = str;
	uint c;
	int16_t len;

	/* Parse the first time number */
	len = sscanf(p, "%u", &c);
	if (len == 1 && c < 24) {
		sTime->Hours = c;

		/* Parse the second time number*/
		/* search the ':' */
		p = (char*) memchr(p, ':', 10);
		if (p) {
			p++;
			/* set pointer after ':' and scan number*/
			len = sscanf(p, "%u", &c);
			if (len == 1 && c < 60) {
				sTime->Minutes = c;

				/* Parse the third time number*/
				/* search the ':' */
				p = (char*) memchr(p, ':', 10);
				if (p) {
					p++;
					/* set pointer after ':' and scan number*/
					len = sscanf(p, "%u", &c);
					if (len == 1 && c < 60) {
						sTime->Seconds = c;
						return 0;
					}
				}
			}
		}
	}
	return 1;
}

/**
 * Fill the given timestamp with today and now values.
 *
 * Note: use this for conversion to string:
 * sprintf(buffer, "20%02i-%02i-%02iT%02i:%02i:%02i", d.Year, d.Month, d.Date, t.Hours, t.Minutes, t.Seconds);
 */
rtc_timestamp_t rtc_get_now(void) {
	rtc_timestamp_t ts;
	HAL_RTC_GetTime(&hrtc, &ts.time, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &ts.date, RTC_FORMAT_BIN);
	return ts;
}

/** time '<=' time */
bool rtc_time_leq(RTC_TimeTypeDef *a, RTC_TimeTypeDef *b) {
	return ((a->Hours > b->Hours) ? 0 : (a->Hours < b->Hours) ? 1 : (a->Minutes > b->Minutes) ? 0 :
			(a->Minutes < b->Minutes) ? 1 : (a->Seconds > b->Seconds) ? 0 : 1);
}

/** time '==' time */
bool rtc_time_eq(RTC_TimeTypeDef *a, RTC_TimeTypeDef *b) {
	return (a->Hours == b->Hours && a->Minutes == b->Minutes && a->Seconds == b->Seconds);
}

/** time '<' time */
bool rtc_time_lt(RTC_TimeTypeDef *a, RTC_TimeTypeDef *b) {
	return (rtc_time_leq(a, b) && !rtc_time_eq(a, b));
}

RTC_TimeTypeDef rtc_time_add(RTC_TimeTypeDef *a, RTC_TimeTypeDef *b) {
	RTC_TimeTypeDef c;
	uint carry = 0; // this is += or ++ do not work, because of optimization (?)
	c.Hours = 0;
	c.Minutes = 0;
	c.Seconds = 0;
	c.Seconds = a->Seconds + b->Seconds;
	if (c.Seconds > 59) {
		c.Seconds -= 60;
		carry = 1;
	}
	c.Minutes = a->Minutes + b->Minutes + carry;
	carry = 0;
	if (c.Minutes > 59) {
		c.Minutes -= 60;
		carry = 1;
	}
	c.Hours = a->Hours + b->Hours + carry;
	if (c.Hours > 24) {
		c.Hours -= 24;
	}
	return c;
}

uint32_t rtc_time2seconds(RTC_TimeTypeDef *t) {
	return (t->Hours * 60 + t->Minutes) * 60 + t->Seconds;
}

RTC_TimeTypeDef rtc_seconds2time(uint32_t s) {
	uint32_t x = s;
	RTC_TimeTypeDef t;
	t.Hours = x / 3600;
	x -= t.Hours * 3600;
	t.Minutes = x / 60;
	x -= t.Minutes * 60;
	t.Seconds = x;
	return t;
}


void init_timetype(RTC_TimeTypeDef *time) {
	time->Hours = 99;
	time->Minutes = 99;
	time->Seconds = 99;
}

