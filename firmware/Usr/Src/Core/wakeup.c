
#include "wakeup.h"
#include "cmd_handler.h"
#include "cmd_parser.h"
#include "datetime.h"
#include "alarm.h"
#include "measurements.h"
#include "logging.h"

wakeup_t wakeup = { .alarmA = false, .triggerPin = false, .cmd = false, };

void wakeup_alarm_handler(void) {

	RTC_TimeTypeDef new;
	rtc_timestamp_t now = rtc_get_now();
	uint32_t n, i, a;

	debug(1, "(alarm): alarm \n");
	debug(2, "(alarm): now: 20%02i-%02i-%02iT%02i:%02i:%02i\n", now.date.Year, now.date.Month, now.date.Date,
			now.time.Hours, now.time.Minutes, now.time.Seconds);

	/* Do the measurement */
	multimeasure(true);

	if (rc.mode == MODE_STARTEND){
		new = get_closest_next_alarm(&rc);
	}

	if (rc.mode == MODE_ENDLESS) {
		// mini-algo: while( alarm <= now ) alarm += ival
		now = rtc_get_now();
		a = rtc_time2seconds(&rc.next_alarm);
		i = rtc_time2seconds(&rc.ival);
		n = rtc_time2seconds(&now.time);
		while (a <= n){ a += i; }
		new = rtc_seconds2time(a);
		new.Hours -= new.Hours > 24 ? 24 : 0;
	}
	rtc_set_alarmA(&new);
	rc.next_alarm = rtc_get_alermAtime();

	debug(2, "(alarm): next: %02i:%02i:%02i\n", rc.next_alarm.Hours, rc.next_alarm.Minutes, rc.next_alarm.Seconds);
}

void wakeup_pintrigger_handler(void) {
	multimeasure(true);
}

void wakeup_cmd_handler(void) {

	parse_extcmd(rxtx_rxbuffer.base, rxtx_rxbuffer.size);
	rxtx.cmd_bytes = 0;
	rxtx_restart_listening();
	cmd_handler();

}
