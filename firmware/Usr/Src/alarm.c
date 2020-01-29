/*
 * alarm.c
 *
 *  Created on: Jul 5, 2019
 *      Author: palmb_ubu
 */

#include "alarm.h"

static RTC_TimeTypeDef get_closest_next_alarm(runtime_config_t *rc);

/**
 * Calculate and return the next alarm closest to now.
 *
 * Note: Call this ONLY in IVAL_STARTEND mode, otherwise
 * stuff break !
 */
static RTC_TimeTypeDef get_closest_next_alarm(runtime_config_t *rc) {
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

/*
 * Set the alarm if the user was submitting a new value.
 */
void set_initial_alarm(runtime_config_t *rc) {
	RTC_TimeTypeDef t;
	rtc_timestamp_t ts;
	if (rc->mode == IVAL_OFF || rc->mode == TRIGGERED) {
		init_timetype(&rc->ival);
		init_timetype(&rc->next_alarm);
		HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);

	} else if (rc->mode == IVAL_STARTEND) {
		t = get_closest_next_alarm(rc);
		rtc_set_alarmA(&t);
		rc->next_alarm = rtc_get_alermAtime();

	} else if (rc->mode == IVAL_ENDLESS) {
		ts = rtc_get_now();
		rtc_set_alarmA_by_offset(&ts.time, &rc->ival);
		rc->next_alarm = rtc_get_alermAtime();
	}
}


