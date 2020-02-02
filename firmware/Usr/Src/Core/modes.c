/*
 * modes.c
 *
 *  Created on: Feb 1, 2020
 *      Author: Bert Palm
 */

#include "sysrc.h"
#include "alarm.h"
#include "wakeup.h"
#include "datetime.h"

/*
 * Switch the mode
 *
 * todo rename to switch mode
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
		wakeup.triggerPin = false;

	} else if (rc->mode == MODE_ENDLESS) {
		ts = rtc_get_now();
		rtc_set_alarmA_by_offset(&ts.time, &rc->ival);
		rc->next_alarm = rtc_get_alermAtime();
		wakeup.triggerPin = false;
	}
}
