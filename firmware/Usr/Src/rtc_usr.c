/*
 * rtc_usr.c
 *
 *  Created on: Mar 6, 2019
 *      Author: palmb_ubu
 */

#include "global_config.h"
#include "main_usr.h"
#include "rtc_usr.h"
#include "string.h"
#include "stdio.h"

rtc_t rtc;

void rtc_init(void){
	rtc.alarmA_wakeup = false;
}

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
	uint c;
	uint16_t len;

	/* Scan year. */
	len = sscanf(p, "%u", &c);
	c = c > 2000 ? c - 2000 : c;
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
 * Parse a interval string to a time object
 * The interval string should be in like ISO 8601 Time format and should look like this:
 * HH:mm:ss, example: 23:59:59
 *
 * Return 0 on success, non-zero otherwise
 */
uint8_t rtc_parse_interval(char *str, RTC_TimeTypeDef *sTime) {

	char *p = str;
	uint c;
	int16_t len;

	/* Parse the first time number */
	len = sscanf(p, "%u", &c);
	if(c == 24){
		/* 24h is allowed as a special case.*/
		sTime->Hours = c;
		sTime->Minutes = 0;
		sTime->Seconds = 0;
		return 0;
	}
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
	return 1;
}

/**
 * Set or deactivate AlarmA, based on a given time.
 *
 * param time: The base time. The interval is set relative to that.
 * So next alarm = base time + interval
 *
 * param ival: the interval.
 *
 * Note: Only Hours, Minutes and Seconds from both parameter are taken in
 * account, other fields are ignored.
 *
 * Note: If Hours, Minutes and Seconds of ival is set to zero, the alarm is
 * deactivated. Also if Hours is set to 24, Minutes and Seconds are ignored
 * and the alarm is set to 1day aka. 24h.
 *
 */
uint8_t rtc_set_alarmA_by_offset(RTC_TimeTypeDef *time, RTC_TimeTypeDef *offset) {

	uint8_t sum, carry;
	RTC_AlarmTypeDef a;

	/*First deactivate alarm, to write alarm register.*/
	HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);

	a.Alarm = RTC_ALARM_A;
	a.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
	a.AlarmDateWeekDay = 1; // is masked anyway
	a.AlarmMask = RTC_ALARMMASK_DATEWEEKDAY;
	a.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;

	/* For Futur Use...
	 * Set all fields that are not overwritten to original struct,
	 * e.g. subseconds are taken from time struct. */
//	memcpy(&a.AlarmTime, time, sizeof(RTC_TimeTypeDef));
	a.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	a.AlarmTime.SecondFraction = 0;
	a.AlarmTime.SubSeconds = 0;
	a.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
	a.AlarmTime.TimeFormat = RTC_HOURFORMAT_24;

	if (offset->Seconds == 0 && offset->Minutes == 0 && offset->Hours == 0) {
		/*Special case: Alarm off. Alarm is already deactivated. So we are done.*/
		return 0;
	}

	if (offset->Hours == 24) {
		/*Special case one day interval.*/
		a.AlarmTime.Seconds = time->Seconds;
		a.AlarmTime.Minutes = time->Minutes;
		a.AlarmTime.Hours = time->Hours;

	} else {

		sum = time->Seconds + offset->Seconds;
		if (sum < 60) {
			a.AlarmTime.Seconds = sum;
			carry = 0;
		} else {
			a.AlarmTime.Seconds = sum - 60;
			carry = 1;
		}

		sum = time->Minutes + offset->Minutes + carry;
		if (sum < 60) {
			a.AlarmTime.Minutes = sum;
			carry = 0;
		} else {
			a.AlarmTime.Minutes = sum - 60;
			carry = 1;
		}

		sum = time->Hours + offset->Hours + carry;
		if (sum < 24) {
			a.AlarmTime.Hours = sum;
		} else {
			a.AlarmTime.Hours = sum - 24;
		}

	}

	return HAL_RTC_SetAlarm_IT(&hrtc, &a, RTC_FORMAT_BIN);
}

/*
 * Return a string representation of the current RTC time.
 * In the ISO 8601 Format 20YY-MM-DDTHH:mm:ss.
 *
 * The param buffer must hold enough space to store the
 * 20 bytes string including the null terminator.
 * */
void rtc_get_now_str(char *buffer, uint32_t sz) {
	RTC_TimeTypeDef t;
	RTC_DateTypeDef d;

	if (sz < 20) {
		return;
	}

	HAL_RTC_GetTime(&hrtc, &t, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &d, RTC_FORMAT_BIN);

	sprintf(buffer, "20%02i-%02i-%02iT%02i:%02i:%02i", d.Year, d.Month, d.Date, t.Hours, t.Minutes,
			t.Seconds);
}

/**
 * Note: Overwrite __weak function in stm32l4xx_hal_rtc.c
 * Note: This is called from within a interrupt.
 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{
	UNUSED(hrtc);

	rtc.alarmA_wakeup = true;

	cpu_enter_run_mode();
}
