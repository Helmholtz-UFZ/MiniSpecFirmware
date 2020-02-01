/*
 * pretty_print.c
 *
 *  Created on: Feb 1, 2020
 *      Author: Bert Palm
 */


#include "pretty_print.h"
#include "stdio_usr.h"
#include "stdio.h"
#include "lib_spectrometer.h"


/** print 'ok' */
void ok(void) {
	if (rc.format == DATA_FORMAT_ASCII) {
		reply("ok\n");
	}
}

/** print 'fail' */
void argerr(void) {
	if (rc.format == DATA_FORMAT_ASCII) {
		errreply("argument error\n");
	}
}


void print_config(runtime_config_t *rc, char *name){
	rtc_timestamp_t ts;

	reply("%s:\n", name);
	printf("mode:        %u\n", rc->mode);
	printf("format:      %u\n", rc->format);
	printf("dbglvl:      %u\n", rc->debuglevel);
	printf("lower/upper: %lu, %lu\n", rc->aa_lower, rc->aa_upper);
	for (int i = 0; i < RCCONF_MAX_ITIMES; ++i) {
		if (i == rc->itime_index) {
			printf("itime[%u] =   %ld  <---- ii (index)\n", i, rc->itime[i]);
			continue;
		}
		if (rc->itime[i] != 0) {
			printf("itime[%u] =   %ld\n", i, rc->itime[i]);
		}
	}
	printf("ii(index):   %u\n", rc->itime_index);
	printf("N:           %u\n", rc->iterations);
	printf("start time:  %02i:%02i:%02i\n", rc->start.Hours, rc->start.Minutes, rc->start.Seconds);
	printf("end time:    %02i:%02i:%02i\n", rc->end.Hours, rc->end.Minutes, rc->end.Seconds);
	printf("interval:    %02i:%02i:%02i\n", rc->ival.Hours, rc->ival.Minutes, rc->ival.Seconds);
	printf("next alarm:  %02i:%02i:%02i\n", rc->next_alarm.Hours, rc->next_alarm.Minutes, rc->next_alarm.Seconds);
	ts = rtc_get_now();
	debug(1," now: 20%02i-%02i-%02iT%02i:%02i:%02i\n", ts.date.Year, ts.date.Month, ts.date.Date, ts.time.Hours,
			ts.time.Minutes, ts.time.Seconds);
}


/** helper for sending data via the uart interface. */
void send_data(void) {
	char *errstr;
	uint32_t *rptr;
	uint16_t i = 0;
	uint8_t errcode = sens1.errc;

	switch (errcode) {
	case ERRC_NO_ERROR:
		errstr = "";
		break;
	case ERRC_TIMEOUT:
		errstr = "ERR: TIMEOUT. Is sensor plugged in ?\n";
		/* otehr reasons: "ADC/sensor not powered, physical connections bad\n"; */
		break;
	case ERRC_NO_EOS:
		errstr = "ERR: NO EOS. Something went wrong, please debug manually.\n";
		break;
	case ERRC_EOS_EARLY:
		errstr = "ERR: EOS EARLY. Something went wrong, please debug manually.\n";
		break;
	case ERRC_UNKNOWN:
		errstr = "ERR: UNKNOWN. Unknown error occurred.\n";
		break;
	default:
		errstr = "ERR: Not implemented.\n";
		break;
	}

	if (rc.format == DATA_FORMAT_BIN) {
		/*Send the errorcode nevertheless an error occurred or not.*/
		HAL_UART_Transmit(&hrxtx, (uint8_t *) &errcode, 2, 200);
		if (!errcode) {
			/* send data */
			HAL_UART_Transmit(&hrxtx, (uint8_t *) (sens1.data->wptr - MSPARAM_PIXEL),
			MSPARAM_PIXEL * 4,
			MSPARAM_PIXEL * 4 * 100);
		}
	} else { /* DATA_FORMAT_ASCII */
		if (errcode) {
			printf(errstr);
		} else {
#if DBG_SEND_ALL
			rptr = sens1.data->base;
#else
			rptr = (uint32_t *) (sens1.data->wptr - MSPARAM_PIXEL);
#endif
			while (rptr < sens1.data->wptr) {
				if (rptr == (sens1.data->wptr - MSPARAM_PIXEL)) {
					/* Pretty print some lines..*/
					printf("\n"DELIMITER_STR);
					printf("\n"HEADER_STR);
					i = 0;
				}

				/* Break and enumerate line after 10 values.*/
				if (i % 10 == 0) {
					printf("\n%03d   %05d ", i, (int) *rptr);
				} else {
					printf("%05d ", (int) *rptr);
				}
				rptr++;
				i++;
			}
			printf("\n"DELIMITER_STR"\n\n");
		}
	}
}
