/*
 * rtc_usr.c
 *
 *  Created on: Mar 6, 2019
 *      Author: palmb_ubu
 */

#include "global_config.h"
#include "rtc_usr.h"
#include "string.h"
#include "stdio.h"

/* Holds the interval that updates alarmA.*/
RTC_TimeTypeDef rtc_ival;
volatile bool rtc_alarmA_occured = 0;

/**
 * Parse a string to a date and time object.
 * The datetime string should be in ISO 8601 format and should look like this:
 * 20YY-MM-DDTHH:mm:ss
 * example: 2099-12-05T23:59:59
 *
 * Return 0 on success, non-zero otherwise
 */
uint8_t rtc_parse_datetime(char* str, RTC_TimeTypeDef *sTime,
		RTC_DateTypeDef *sDate) {

	char *p = str;
	uint16_t cc;
	uint8_t c;

	/* Scan year.
	 * %hu means scan to unsigned(u) short(h) */
	sscanf(p, "%hu", &cc);
	cc = cc > 2000 ? cc - 2000 : cc;
	c = (uint8_t) cc;
	if (!IS_RTC_YEAR(c)) {
		return 1;
	}
	sDate->Year = c;

	/* Scan month
	 * search the '-', and let str point to the char right after it. */
	p = (char*) memchr(p, '-', 10) + 1;
	/* %hhui means scan to unsigned(u) char(hh) */
	sscanf(p, "%hhui", &c);
	if (!IS_RTC_MONTH(c)) {
		return 2;
	}
	sDate->Month = c;

	/* search the '-', and let str point to the char right after it. */
	p = (char*) memchr(p, '-', 10) + 1;
	sscanf(p, "%hhui", &c);
	if (!IS_RTC_DATE(c)) {
		return 3;
	}
	sDate->Date = c;

	/* search the 'T', and let str point to the char right after it. */
	p = (char*) memchr(p, 'T', 10) + 1;
	sscanf(p, "%hhui", &c);
	if (!IS_RTC_HOUR24(c)) {
		return 4;
	}
	sTime->Hours = c;

	/* search the ':', and let str point to the char right after it. */
	p = (char*) memchr(p, ':', 10) + 1;
	sscanf(p, "%hhui", &c);
	if (!IS_RTC_MINUTES(c)) {
		return 5;
	}
	sTime->Minutes = c;

	/* search the ':', and let str point to the char right after it. */
	p = (char*) memchr(p, ':', 10) + 1;
	sscanf(p, "%hhui", &c);
	if (!IS_RTC_SECONDS(c)) {
		return 6;
	}
	sTime->Seconds = c;

	return 0;
}

/**
 * Parse a interval string to a time object
 * The interval string should be in like ISO 8601 Time format and should look like this:
 * HH:mm:ss, example: 23:59:59
 *
 * Return 0 on success, non-zero otherwise
 */
uint8_t rtc_parse_interval(char *str, RTC_TimeTypeDef *sTime) {

	char *p = str;
	uint8_t c;

	/* Parse the first number*/
	sscanf(p, "%hhui", &c);
	if (!IS_RTC_HOUR24(c)) {
		return 4;
	}
	sTime->Hours = c;

	/* search the ':', and let str point to the char right after it. */
	p = (char*) memchr(p, ':', 10) + 1;
	sscanf(p, "%hhui", &c);
	if (!IS_RTC_MINUTES(c)) {
		return 5;
	}
	sTime->Minutes = c;

	/* search the ':', and let str point to the char right after it. */
	p = (char*) memchr(p, ':', 10) + 1;
	sscanf(p, "%hhui", &c);
	if (!IS_RTC_SECONDS(c)) {
		return 6;
	}
	sTime->Seconds = c;

	return 0;
}

/**
 * Set Alarm a based on a given time.
 *
 */
uint8_t rtc_set_alarmA_interval(RTC_TimeTypeDef *time, RTC_TimeTypeDef *ival) {

	uint8_t sum, carry;
	RTC_AlarmTypeDef a;

	/*First deactivate alarm, to write alarm register.*/
	HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);

	a.Alarm = RTC_ALARM_A;
	a.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
	a.AlarmDateWeekDay = 1; // is masked anyway
	a.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;
	a.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;

	/*set all fields that are not overwritten to original struct,
	 * e.g. subseconds are taken from time struct. */
	memcpy(&a.AlarmTime, time, sizeof(RTC_TimeTypeDef));

	if (ival->Seconds == 0 && ival->Minutes == 0 && ival->Hours == 0 ){
		/*Special case: Alarm off. Alarm is already deactivated. So we are done.*/
		return 0;
	}

	if (ival->Hours == 24) {
		/*Special case one day interval.*/
		a.AlarmTime.Seconds = time->Seconds;
		a.AlarmTime.Minutes = time->Minutes;
		a.AlarmTime.Hours = time->Hours;

	} else {

		sum = time->Seconds + ival->Seconds;
		if (sum < 60) {
			a.AlarmTime.Seconds = sum;
			carry = 0;
		} else {
			a.AlarmTime.Seconds = sum - 60;
			carry = 1;
		}

		sum = time->Minutes + ival->Minutes + carry;
		if (sum < 60) {
			a.AlarmTime.Minutes = sum;
			carry = 0;
		} else {
			a.AlarmTime.Minutes = sum - 60;
			carry = 1;
		}

		sum = time->Hours + ival->Hours + carry;
		if (sum < 24) {
			a.AlarmTime.Hours = sum;
		} else {
			a.AlarmTime.Hours = sum - 24;
		}

	}

	return HAL_RTC_SetAlarm_IT(&hrtc, &a, RTC_FORMAT_BIN);
}

/**
 * Note: Overwrite __weak function in stm32l4xx_hal_rtc.c
 * Note: This is called from within a interrupt.
 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
	UNUSED(hrtc);

	rtc_alarmA_occured = 1;

	cpu_enter_run_mode();
}
