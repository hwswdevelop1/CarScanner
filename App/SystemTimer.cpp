/*
 * Time.cpp
 *
 *  Created on: May 24, 2024
 *      Author: Evgeny Sobolev. 02/09/1984y.b
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/timer.h>
#include "Types.h"
#include "SystemTimer.h"
#include "IntLock.h"

#include "UsbPackets.h"


namespace SystemTimer {

static constexpr uint32_t SystemTimerPeriod = 0x10000;

struct HwConfig {
	timer_t		timer;
	irq_t		irq;
};

struct Module {
	HwConfig	hwConfig;
	volatile time_us_t	time;
};

static Module gTim2;

Status init() {
	Module* const module = &gTim2;
	module->hwConfig.timer = TIM2;
	module->hwConfig.irq = 28;
	nvic_disable_irq(module->hwConfig.irq);
	TIM_CR1(module->hwConfig.timer) = 0;
	TIM_CR2(module->hwConfig.timer) = 0;
	TIM_SMCR(module->hwConfig.timer) = 0;
	TIM_DIER(module->hwConfig.timer) = TIM_DIER_UIE;
	TIM_CNT(module->hwConfig.timer) = 0;
	TIM_PSC(module->hwConfig.timer) = 71;
	TIM_ARR(module->hwConfig.timer) = ( SystemTimerPeriod - 1);
	TIM_EGR(module->hwConfig.timer) |= TIM_EGR_UG;
	while( (TIM_EGR(module->hwConfig.timer) && (TIM_EGR_UG)) );
	TIM_SR(module->hwConfig.timer) = 0;
	nvic_clear_pending_irq(module->hwConfig.irq);
	nvic_enable_irq(module->hwConfig.irq);
	TIM_CR1(module->hwConfig.timer) = TIM_CR1_CEN;
	return Status::Success;
}

time_us_t getTimeUs() {
	Module* const module = &gTim2;
	int_lock_t key = intLock();
	volatile const uint32_t firstTs = TIM_CNT(module->hwConfig.timer);
	volatile time_us_t time = module->time | firstTs;
	volatile const uint32_t secondTs = TIM_CNT(module->hwConfig.timer);
	if ( secondTs < firstTs ) {
		time = ( module->time + SystemTimerPeriod ) | secondTs;
	}
	intUnlock(key);
	return time;
}

void wait( const time_us_t delay ) {
	static constexpr time_us_t minDelay = 3;
	if ( delay < minDelay ) return;
	const time_us_t startTime = SystemTimer::getTimeUs();
	do {
		volatile const time_us_t nowTime = SystemTimer::getTimeUs();
		volatile const time_us_t diff = ( nowTime - startTime );
		if ( diff >= delay ) {
			break;
		}
	} while(true);
}

void onTimerIrq( Module* const module ) {
	module->time += SystemTimerPeriod;
	TIM_SR(module->hwConfig.timer) = 0;
	nvic_clear_pending_irq(module->hwConfig.irq);
}

Status process( const IndexType p ) {
	Module* const module =  &gTim2;
	int_lock_t key = intLock();
	uint8_t* const buf = usbBufferGetDataPtr( PoolType::UsbToPeriph, p );
	usbBufferCheckout( PoolType::UsbToPeriph, p );
	const UsbToTimerHead* const head = reinterpret_cast<const UsbToTimerHead* const>(buf);
	const time_us_t ts = head->ts;
	usbBufferFree( PoolType::UsbToPeriph, p );
	TIM_CR1(module->hwConfig.timer) &= ~(TIM_CR1_CEN);
	TIM_CNT(module->hwConfig.timer) = (ts & 0xFFFF);
	module->time = (ts & 0xFFFF0000);
	TIM_CR1(module->hwConfig.timer) |= (TIM_CR1_CEN);
	intUnlock(key);
	return Status::Success;
}

}

SystemTimer::Module* const tim2 = &SystemTimer::gTim2;

extern "C" void TIM2_IRQHandler(void) {
	SystemTimer::onTimerIrq( tim2 );
}


