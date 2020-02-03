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
 */
void mode_switch(runtime_config_t *rc) {
	RTC_TimeTypeDef t;
	rtc_timestamp_t ts;

	switch (rc->mode) {
	case MODE_OFF: // no break
	case MODE_TRIGGERED:
		init_timetype(&rc->ival);
		init_timetype(&rc->next_alarm);
		HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
		break;

	case MODE_STARTEND:
		t = get_closest_next_alarm(rc);
		rtc_set_alarmA(&t);
		rc->next_alarm = rtc_get_alermAtime();
		wakeup.triggerPin = false;
		break;

	case MODE_ENDLESS:
		ts = rtc_get_now();
		rtc_set_alarmA_by_offset(&ts.time, &rc->ival);
		rc->next_alarm = rtc_get_alermAtime();
		wakeup.triggerPin = false;
		break;

	default:
		break;
	}
}
