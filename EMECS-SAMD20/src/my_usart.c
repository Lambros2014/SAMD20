/*
 * my_usart.c
 *
 * Created: 01.02.2014 12:09:19
 *  Author: student
 */ 

#include <asf.h>
#include "my_usart.h"

struct usart_module usart_instance;

void configure_usart(void)
{
	struct
	usart_config config_usart;
	usart_get_config_defaults(&config_usart);
	config_usart.baudrate = 9200;
	config_usart.mux_setting = EDBG_CDC_SERCOM_MUX_SETTING;
	config_usart.pinmux_pad0 = EDBG_CDC_SERCOM_PINMUX_PAD0;
	config_usart.pinmux_pad1 = EDBG_CDC_SERCOM_PINMUX_PAD1;
	config_usart.pinmux_pad2 = EDBG_CDC_SERCOM_PINMUX_PAD2;
	config_usart.pinmux_pad3 = EDBG_CDC_SERCOM_PINMUX_PAD3;
	while(usart_init(&usart_instance, EDBG_CDC_MODULE, &config_usart) != STATUS_OK)
	{
		//wait
	}
	usart_enable(&usart_instance);
	
	uint8_t	string[] = "usart OK\n   ";
	usart_write_buffer_job(&usart_instance,	string, sizeof(string));
}