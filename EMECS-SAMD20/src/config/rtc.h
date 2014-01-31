/*
 * rtc.h
 *
 * Created: 31.01.2014 10:54:23
 *  Author: student
 */ 


#ifndef RTC_H_
#define RTC_H_



struct Alarm 
{
		struct rtc_calendar_time alarm_time;
		void *ptr;
};
void configure_rtc_calendar( void );

//struct Alarm a1 = {time, param, &d0_this)

#endif /* RTC_H_ */