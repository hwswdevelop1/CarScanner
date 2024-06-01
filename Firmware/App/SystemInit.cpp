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
	//rcc_clock_setup_in_hse_8mhz_out_72mhz();
	rcc_clock_setup_pll( &rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ] );
	rcc_periph_clock_enable(RCC_GPIOA);
	rcc_periph_clock_enable(RCC_GPIOB);
	rcc_periph_clock_enable(RCC_GPIOC);
	rcc_periph_clock_enable(RCC_AFIO);
	rcc_periph_clock_enable(RCC_USART1);
	rcc_periph_clock_enable(RCC_USB);
	rcc_periph_clock_enable(RCC_OTGFS);
	rcc_periph_clock_enable(RCC_TIM1);
	rcc_periph_clock_enable(RCC_TIM2);
	rcc_periph_clock_enable(RCC_CAN1);
	rcc_periph_clock_enable(RCC_CAN2);




	// Configure USB pins
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO11 | GPIO12);
	gpio_primary_remap( AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON, AFIO_MAPR_USART1_REMAP );

	// Configure CAN1 pins
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO9);
	gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO8);
	// Configure Rs pin of CAN1 module
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO10);
	// Can High Speed mode
	gpio_clear(GPIOB, GPIO10);

	// Configure CAN2 pins
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO13);
	gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO12);
	// Enable CAN PHY
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO15);
	// Can High Speed mode
	gpio_set(GPIOB, GPIO15);
	// Rx Error signal input
	gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO14);
	// Can High Speed mode
	gpio_set(GPIOB, GPIO14);



	// Configure REMAP....
	gpio_primary_remap( AFIO_MAPR_SWJ_CFG_JTAG_OFF_SW_ON, AFIO_MAPR_CAN1_REMAP_PORTB );

	// Configure UART/LIN pins
	gpio_set_mode(GPIOB, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO6 );
	gpio_set_mode(GPIOB, GPIO_MODE_INPUT, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO7);
	gpio_set(GPIOB, GPIO7);

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

	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO8);
	rcc_set_mco(RCC_CFGR_MCO_HSE);

}


extern "C" void SysTick_Handler() {
	int_lock_t key = intLock();
	nvic_clear_pending_irq(NVIC_SYSTICK_IRQ);
	intUnlock(key);
}



