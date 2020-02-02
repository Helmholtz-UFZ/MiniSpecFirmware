/*
 * alarm.c
 *
 *  Created on: Jul 5, 2019
 *      Author: Bert Palm
 */

#include "alarm.h"
#include "cpu.h"
#include "datetime.h"

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
