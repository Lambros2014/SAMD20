/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * This is a bare minimum user application template.
 *
 * For documentation of the board, go \ref group_common_boards "here" for a link
 * to the board-specific documentation.
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to system_init()
 * -# Basic usage of on-board LED and button
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
#include <asf.h>
#include <stdio.h>

enum app_state {
	APP_STATE_LIGHTSENSOR_FILTER,
	APP_STATE_TEMPERATURE_READ,
};

volatile uint16_t app_state_flags = 0;

static bool is_app_state_set(enum app_state state)
{
	bool retval;
	system_interrupt_enter_critical_section();
	
	if (app_state_flags & (1 << state)) {
		retval = true;
		} else {
		retval = false;
	}
	system_interrupt_leave_critical_section();
	return retval;
}

static void set_app_state(enum app_state state)
{
	system_interrupt_enter_critical_section();
	/* Set corresponding flag */
	app_state_flags |= (1 << state);
	system_interrupt_leave_critical_section();
}
static void clear_app_state(enum app_state state)
{
	system_interrupt_enter_critical_section();
	/* Clear corresponding flag */
	app_state_flags &= ~(1 << state);
	system_interrupt_leave_critical_section();
}

static void rtc_setup(void)
{
	struct rtc_count_config conf;
	rtc_count_get_config_defaults(&conf);
	conf.prescaler = RTC_COUNT_PRESCALER_DIV_1;
	conf.mode = RTC_COUNT_MODE_16BIT;
	conf.continuously_update = true;
	rtc_count_init(&conf);
	rtc_count_enable();
}
static void rtc_callback(void)
{
	port_pin_toggle_output_level(LED_0_PIN);
	set_app_state(APP_STATE_TEMPERATURE_READ);
}
static void rtc_callback_setup(void)
{
	/* Set period */
	rtc_count_set_period(1024);
	/* Restart counter */
	rtc_count_set_count(0);
	/* Register and enable callback */
	rtc_count_register_callback(rtc_callback, RTC_COUNT_CALLBACK_OVERFLOW);
	rtc_count_enable_callback(RTC_COUNT_CALLBACK_OVERFLOW);
}

#define APP_ADC_SAMPLES 128
#define APP_ADC_PLOT_START 16
uint16_t adc_sample_buffer[2][APP_ADC_SAMPLES];
volatile uint8_t adc_buffer_number = 0;

struct adc_module adc_light_module;

static void adc_sample_done_callback(
const struct adc_module *const module)
{
	/* Swap sampling buffer */
	adc_buffer_number ^= 1;
	/* signal the application that the ADC data can be processed */
	set_app_state(APP_STATE_LIGHTSENSOR_FILTER);
	/* start a new conversion on the next buffer */
	adc_read_buffer_job(&adc_light_module, adc_sample_buffer[adc_buffer_number],
	APP_ADC_SAMPLES);
}

static void lightsensor_setup(void)
{
	struct adc_config config;
	adc_get_config_defaults(&config);
	config.gain_factor = ADC_GAIN_FACTOR_DIV2;
	config.clock_prescaler = ADC_CLOCK_PRESCALER_DIV512;
	config.reference = ADC_REFERENCE_INTVCC1;
	config.positive_input = ADC_POSITIVE_INPUT_PIN8;
	config.resolution = ADC_RESOLUTION_12BIT;
	config.clock_source = GCLK_GENERATOR_3;
	adc_init(&adc_light_module, ADC, &config);
	adc_enable(&adc_light_module);
	adc_register_callback(&adc_light_module, adc_sample_done_callback,
	ADC_CALLBACK_READ_BUFFER);
	adc_enable_callback(&adc_light_module, ADC_CALLBACK_READ_BUFFER);
}

static void temp_sensor_setup(void)
{
	/* Init and enable temperature sensor */
	at30tse_init();
	/* Set 12-bit resolution mode */
	at30tse_write_config_register(
	AT30TSE_CONFIG_RES(AT30TSE_CONFIG_RES_12_bit));
}

#define APP_TEMP_STRING_LENGTH 20
#define APP_TEMP_POSITION_X 0
#define APP_TEMP_POSITION_Y 0

static uint16_t buffer_average(
uint16_t *buffer,
uint16_t buffer_size)
{
	uint8_t i;
	uint32_t acc = 0;
	for (i = 0; i < buffer_size; i++) {
		acc += buffer[i];
	}
	/* Returns average */
	return (acc/buffer_size);
}

static void highpass_filter(
uint16_t *source,
int16_t *dest,
uint8_t buffer_size,
uint16_t offset)
{
	uint8_t i;
	for (i = 0; i < buffer_size; i++) {
		/* Subtract offset from each sample in the raw signal */
		dest[i] = (source[i] - offset);
	}
}

#define APP_ADC_SNR 300
static uint16_t calc_freq_from_buffer(
int16_t *buffer,
uint16_t buffer_size,
uint16_t sampling_frequency)
{
	/* Variables */
	uint16_t acc_samples = 0;
	uint16_t last_sample;
	uint16_t half_periods = 0;
	uint16_t i = 0;
	uint8_t signbit;
	uint16_t freq = 0;
	int32_t abs_sum = 0;
	/* Find the first non-zero sample */
	while (buffer[i] == 0 && i < buffer_size) {
		i++;
	}
	/* Return from function if it contains only zeros */
	if (i > (buffer_size - 1)) {
		return freq;
	}
	signbit = buffer[i] > 0;
	last_sample = i;
	while (i < buffer_size) {
		if ((buffer[i] > 0) != signbit) {
			/* Zero-crossing, a half period is passed */
			half_periods++;
			acc_samples += i - last_sample;
			last_sample = i;
			signbit ^= 1;
		}
		i++;
		abs_sum += abs(buffer[i]);
	}
	if ((abs_sum / buffer_size) > APP_ADC_SNR) {
		/* Calculate average frequency */
		freq = (half_periods * sampling_frequency) / (acc_samples * 2);
		} else {
		/* The signal contains only noise */
		freq = 0;
	}
	return freq;
}

#define APP_ADC_SAMPLE_FREQ 70
#define SPOKES 24
#define WHEEL_DIAMETER_CM 66
#define PI 3.14

static void process_data(
uint16_t *source,
uint16_t *speed,
uint16_t *offset,
uint16_t size)
{
	/* Temp variables */
	int16_t hp_buffer[APP_ADC_SAMPLES];
	uint16_t spoke_freq;
	/* Find the offset of the given signal */
	*offset = buffer_average(source, size);
	/* Filtrate away the offset */
	highpass_filter(source, hp_buffer, size, *offset);
	/* Calculate spokes frequency and speed */
	spoke_freq = calc_freq_from_buffer(hp_buffer, APP_ADC_SAMPLES,
	APP_ADC_SAMPLE_FREQ);
	*speed = ((WHEEL_DIAMETER_CM * PI) * spoke_freq) / SPOKES;
}

#define APP_SPEED_POSITION_X 0
#define APP_SPEED_POSITION_Y 8



int main (void)
{
	uint8_t xpos = 0;
	double temp_result;
	char temp_string[APP_TEMP_STRING_LENGTH];
	
	uint16_t speed;
	uint16_t offset;
	
	system_init();
	
	rtc_setup();
	rtc_callback_setup();
	
	gfx_mono_init();
	gfx_mono_draw_string("Hello world!", 0, 0, &sysfont);	
	
	lightsensor_setup();
	adc_read_buffer_job(&adc_light_module, adc_sample_buffer[adc_buffer_number],
	APP_ADC_SAMPLES);
	
	temp_sensor_setup();
	temp_result = at30tse_read_temperature();
	
	snprintf(temp_string, APP_TEMP_STRING_LENGTH, 	"Temp: %d.%dC\n",	(int)temp_result, 	((int)(temp_result * 10)) % 10);
	
	gfx_mono_draw_string(temp_string, APP_TEMP_POSITION_X, APP_TEMP_POSITION_Y, &sysfont);

	while (true) {
		
		if (is_app_state_set(APP_STATE_LIGHTSENSOR_FILTER)) {
			clear_app_state(APP_STATE_LIGHTSENSOR_FILTER);
			
			process_data(adc_sample_buffer[adc_buffer_number^1],
			&speed, &offset, APP_ADC_SAMPLES);
			snprintf(temp_string, APP_TEMP_STRING_LENGTH,
			"S: %4u L: %3u \n",
			speed,
			offset);
			gfx_mono_draw_string(temp_string,
			APP_SPEED_POSITION_X, APP_SPEED_POSITION_Y, &sysfont);
			
			for (xpos = 0; xpos < APP_ADC_SAMPLES; xpos++) {
				/* Draw white line */
				gfx_mono_draw_vertical_line(xpos, APP_ADC_PLOT_START,
				GFX_MONO_LCD_HEIGHT - APP_ADC_PLOT_START,
				GFX_PIXEL_SET);
				/* Draw black line according to ADC sample value */
				gfx_mono_draw_vertical_line(xpos, APP_ADC_PLOT_START,
				(adc_sample_buffer[adc_buffer_number^1][xpos] /
				(0xFFF / (GFX_MONO_LCD_HEIGHT - APP_ADC_PLOT_START))),
				GFX_PIXEL_CLR);
			}
		}
		
		if (is_app_state_set(APP_STATE_TEMPERATURE_READ)) {
			clear_app_state(APP_STATE_TEMPERATURE_READ);
			/* Read temperature result */
			temp_result = at30tse_read_temperature();
			/* Convert result to a string and write it to the display */
			snprintf(temp_string, APP_TEMP_STRING_LENGTH,
			"Temp: %d.%dC\n",
			(int)temp_result,
			((int)(temp_result * 10)) % 10);
			gfx_mono_draw_string(temp_string, APP_TEMP_POSITION_X, APP_TEMP_POSITION_Y,
			&sysfont);
		}
	}
}
