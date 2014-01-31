/*
 * rtc.c
 *
 * Created: 31.01.2014 10:54:09
 *  Author: student
 */ 

#include <asf.h>
#include "rtc.h"
#include <stdbool.h>



// Modefied from Application note AT03266
void configure_rtc_calendar( void )
{ /* Initialize RTC in calendar mode. */
	struct rtc_calendar_config config_rtc_calendar;
	rtc_calendar_get_config_defaults(&config_rtc_calendar);
	
	struct rtc_calendar_time alarm;
	rtc_calendar_get_time_defaults(&alarm);
	alarm.year = 2014;
	alarm.month = 2;
	alarm.day = 1;
	alarm.hour = 0;
	alarm.minute = 0;
	alarm.second = 4;
	
	config_rtc_calendar.clock_24h = true ;
	config_rtc_calendar.alarm[0].time = alarm;
	config_rtc_calendar.alarm[0].mask = RTC_CALENDAR_ALARM_MASK_YEAR;
	
	rtc_calendar_init(&config_rtc_calendar);
	
	rtc_calendar_enable();
}

 