
#include "wakeup.h"
#include "cmd_handler.h"
#include "cmd_parser.h"

wakeup_t wakeup = { .alarmA = false, .triggerPin = false, .cmd = false, };

void wakeup_alarm_handler(void) {

	RTC_TimeTypeDef new;
	rtc_timestamp_t ts = rtc_get_now();

	debug(1, "(alarm): alarm \n");
	debug(2, "(alarm): now: 20%02i-%02i-%02iT%02i:%02i:%02i\n", ts.date.Year, ts.date.Month, ts.date.Date,
			ts.time.Hours, ts.time.Minutes, ts.time.Seconds);

	/* set new alarm */
	new = rtc_time_add(&rc.next_alarm, &rc.ival);
	if (rc.mode == MODE_ENDLESS) {
		rtc_set_alarmA(&new);

	} else { /* rc.mode == IVAL_STARTEND */
		/* if new <= end */
		if (rtc_time_leq(&new, &rc.end)) {
			rtc_set_alarmA(&new);
		} else {
			rtc_set_alarmA(&rc.start);
		}
	}
	rc.next_alarm = rtc_get_alermAtime();

	/* Do the measurement */
	multimeasure(true);

	debug(2, "(alarm): next: %02i:%02i:%02i\n", rc.next_alarm.Hours, rc.next_alarm.Minutes, rc.next_alarm.Seconds);
}

void wakeup_pintrigger_handler(void) {
	multimeasure(true);
}

void wakeup_cmd_handler(void) {

	parse_extcmd(rxtx_rxbuffer.base, rxtx_rxbuffer.size);
	rxtx.cmd_bytes = 0;
	rxtx_restart_listening();
	extcmd_handler();

}
