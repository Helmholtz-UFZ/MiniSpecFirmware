/*
 * alarm.c
 *
 *  Created on: Jul 5, 2019
 *      Author: Bert Palm
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
void init_mode(runtime_config_t *rc) {
	RTC_TimeTypeDef t;
	rtc_timestamp_t ts;
	if (rc->mode == MODE_OFF || rc->mode == MODE_TRIGGERED) {
		init_timetype(&rc->ival);
		init_timetype(&rc->next_alarm);
		HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);

	} else if (rc->mode == MODE_STARTEND) {
		t = get_closest_next_alarm(rc);
		rtc_set_alarmA(&t);
		rc->next_alarm = rtc_get_alermAtime();
		rc->trigger = false;

	} else if (rc->mode == MODE_ENDLESS) {
		ts = rtc_get_now();
		rtc_set_alarmA_by_offset(&ts.time, &rc->ival);
		rc->next_alarm = rtc_get_alermAtime();
		rc->trigger = false;
	}
}

void init_timetype(RTC_TimeTypeDef *time) {
	time->Hours = 99;
	time->Minutes = 99;
	time->Seconds = 99;
}