/*
 * iotask.c
 *
 * Created: 31.01.2014 12:16:30
 *  Author: magnealv
 */ 

#include <asf.h>
#include <conf_demo.h>
#include <inttypes.h> 

#include "rtc.h"
#include "uitask.h"

#define UI_TASK_PRIORITY      (tskIDLE_PRIORITY + 3)
#define UI_TASK_DELAY         (10 / portTICK_RATE_MS)
#define UI_TASK_INIT_DELAY	  ( 1000 / portTICK_RATE_MS)

static OLED1_CREATE_INSTANCE(oled1, OLED1_EXT_HEADER);

//! Handle for about screen task
static xTaskHandle ui_task_handle;

static void ui_task(void *params);

void uitask_init (void)
{
	oled1_init(&oled1);
		
	xTaskCreate(ui_task,
	(signed char *)"About",
	configMINIMAL_STACK_SIZE,
	NULL,
	UI_TASK_PRIORITY,
	&ui_task_handle);
}

void ui_print_date(void)
{
	struct rtc_calendar_time temp;
	rtc_calendar_get_time(&temp);

	char str[24];
	sprintf(str, "%i:%i:%"PRIu16"\n", temp.day, temp.month, temp.year);

	//oled1_set_led_state(&oled1, OLED1_LED2_ID, true);

	gfx_mono_draw_string(str, 0, 0, &sysfont);

	//oled1_set_led_state(&oled1, OLED1_LED2_ID, false);
}

void ui_print_time(void)
{
	struct rtc_calendar_time temp;
	rtc_calendar_get_time(&temp);

	char str[10];
	sprintf(str, "%i:%i:%i", temp.hour, temp.minute, temp.second);

	//oled1_set_led_state(&oled1, OLED1_LED2_ID, true);

	gfx_mono_draw_string(str, 0, 20, &sysfont);

	//oled1_set_led_state(&oled1, OLED1_LED2_ID, false);
}


static void ui_task(void *params)
{
	portTickType last_wake_time;
	last_wake_time = xTaskGetTickCount ();
	
	vTaskDelay (UI_TASK_INIT_DELAY);
	
	while (1)
	{
		ui_print_time();
		
		ui_print_date();
		
		vTaskDelayUntil (&last_wake_time, UI_TASK_DELAY);
	}
}