/*
 * SystemInit.cpp
 *
 *  Created on: 1 мар. 2021 г.
 *      Author: Evgeny
 */
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/flash.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/st_usbfs.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/mpu.h>
#include <libopencm3/cm3/systick.h>

#include "Usb.h"
#include "IntLock.h"


extern "C" void systemInit(){
	rcc_clock_setup_pll( &rcc_hse_configs[RCC_CLOCK_HSE16_72MHZ] );
	//rcc_clock_setup_in_hse_16mhz_out_72mhz();
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_AFIO);
	rcc_periph_clock_enable(RCC_USART1);
	rcc_periph_clock_enable(RCC_USB);
	rcc_periph_clock_enable(RCC_OTGFS);
	rcc_periph_clock_enable(RCC_TIM1);
	rcc_periph_clock_enable(RCC_TIM2);


	// Configure USB pins
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO11 | GPIO12);



	// Configure UART/LIN pins
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO6 );
	gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO7);
	gpio_port_write(GPIOB, GPIO7);
	gpio_set(GPIOB, GPIO7);

	gpio_primary_remap( AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON, AFIO_MAPR_USART1_REMAP );

	// LIN control pins
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO11);

	// Configure LED pins
	gpio_set_mode(GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO6 | GPIO7  );

	//AFIO_MAPR = AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON;

	systick_set_frequency(1000, 72000000);
	nvic_set_priority( NVIC_SYSTICK_IRQ, 0x10);
	nvic_enable_irq(NVIC_SYSTICK_IRQ);
	systick_interrupt_enable();
	systick_counter_enable();

}


extern "C" void SysTick_Handler() {
	int_lock_t key = intLock();
	nvic_clear_pending_irq(NVIC_SYSTICK_IRQ);
	intUnlock(key);
}



