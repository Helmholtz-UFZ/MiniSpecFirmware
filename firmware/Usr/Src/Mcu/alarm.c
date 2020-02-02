/*
 * alarm.c
 *
 *  Created on: Jul 5, 2019
 *      Author: Bert Palm
 */

#include "alarm.h"
#include "datetime.h"
#include "cpu.h"


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
 * set to 24h aka. every day.
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

	return HAL_RTC_SetAlarm_IT(&hrtc, &a, RTC_FORMAT_BIN);
}



RTC_TimeTypeDef rtc_get_alermAtime(void) {
	RTC_AlarmTypeDef a;
	HAL_RTC_GetAlarm(&hrtc, &a, RTC_ALARM_A, RTC_FORMAT_BIN);
	return a.AlarmTime;
}

/**
 * Set AlarmA
 *
 * param time: time to set the alarm
 *
 * Note: Only Hours, Minutes and Seconds time are taken in
 * account, other fields are ignored.
 */
uint8_t rtc_set_alarmA(RTC_TimeTypeDef *time) {
	RTC_TimeTypeDef zero;
	zero.Hours = 0;
	zero.Minutes = 0;
	zero.Seconds = 0;
	return rtc_set_alarmA_by_offset(time, &zero);
}
/**
 * Calculate and return the next alarm closest to now.
 *
 * Note: Call this ONLY in IVAL_STARTEND mode, otherwise
 * stuff break !
 */
RTC_TimeTypeDef get_closest_next_alarm(runtime_config_t *rc) {
	uint32_t start, end, now, ival, N, x;
	rtc_timestamp_t t;
	t = rtc_get_now();
	now = rtc_time2seconds(&t.time);
	ival = rtc_time2seconds(&rc->ival);
	start = rtc_time2seconds(&rc->start);
	end = rtc_time2seconds(&rc->end);
	/*Safety window of 2 seconds. One is not enough
	 * because we could be very close to a second transition.*/
	now = now + 2;

	if (now < start || now > end){
		return rtc_seconds2time(start);
	}

	/* N <= n < N+1, where n is the correct float-value */
	N = (now - start) / ival;
	x = (start + N * ival > now) ? N * ival : (N + 1) * ival;
	x = (start + x <= end) ? start + x : start;
	return rtc_seconds2time(x);
}

/**
 * Note: Overwrite __weak function in stm32l4xx_hal_rtc.c
 * Note: This is called from within a interrupt.
 */
void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc) {
	UNUSED(hrtc);

	rtc.alarmA_wakeup = true;

	leave_LPM_from_ISR();
}
