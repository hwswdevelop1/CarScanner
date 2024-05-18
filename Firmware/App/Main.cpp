/*
 * main.cpp
 *      Author: Evgeny Sobolev 09.02.1984 y.b.
 *      Tel: +79003030374
 *      e-mail: hwswdevelop@gmail.com
*/



#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/stm32/f1/timer.h>
#include <libopencm3/stm32/st_usbfs.h>
#include <libopencm3/stm32/f1/st_usbfs.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/mpu.h>
#include <libopencm3/stm32/usart.h>
#include "Memory.h"
#include "Usb.h"
#include "Lin.h"


#include "usb_common.h"
#include "UsbPacketBuffer.h"

extern "C" void systemMain() {

	usbdInit();

	Lin::HwConfig hwConfig;


	hwConfig.usart = USART1;
	hwConfig.usart_irq = 37;
	hwConfig.lin_en = { GPIOB, GPIO11 };
	hwConfig.timer = TIM1;
	hwConfig.timer_irq = 25;
	Lin::hwInit( lin0, &hwConfig );

	Lin::Config config;
	config.baud = 9600;
	config.maxRxSize = 16;
	config.rxTimeout = 2000;
	Lin::config(lin0, &config );

	uint32_t counter = 0;
	while( true ) {
		auto p = usbBufferGet( PoolType::UsbToPeriph );
		if ( InvalidIndex != p ) {
			Lin::process( lin0, p );
		}
	}

}

