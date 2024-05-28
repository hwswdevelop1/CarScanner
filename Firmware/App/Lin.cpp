/*
 * Lin.cpp
 *
 *  Created on: May 11, 2024
 *      Author: Evgeny Sobolev 09.02.1984 y.b.
 *      Tel: +79003030374
 *      e-mail: hwswdevelop@gmail.com
 *
 */

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/f1/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/dma.h>
#include <libopencm3/stm32/timer.h>
#include "Memory.h"
#include "Lin.h"
#include "UsbPacketBuffer.h"
#include "IntLock.h"
#include "SystemTimer.h"


#define USART_CR2_LINEN 	(1 << 14)
static void usart_send_break( const uint32_t usart );
static void usart_set_lin_mode( uint32_t usart );



namespace Lin {

static constexpr const uint8_t 	LinSyncByte = 0x55;
static constexpr const uint8_t 	LinMasterFrameRequestSize = 2;
static constexpr const uint8_t  LinSlaveAnswerCount = 8;

static constexpr const uint8_t  LinSlaveAnswerMaxSize = 9;
static constexpr const uint16_t LinMinTimeoutUs = 100;
static constexpr const uint16_t LinMaxTimeoutUs = 50000;
static constexpr const uint16_t LinMinBaud = 300;
static constexpr const uint16_t LinMaxBaud = 57600;


enum class TxState :uint8_t {
	Idle,
	SendData,
	SendAnswer
};

enum class RxState : uint8_t {
	Idle,
	WaitLinData,
	WaitData
};


struct LinSlaveAnswer {
	uint8_t		id;
	uint8_t		size;
	LinCrcType	crc;
	uint8_t		data[LinSlaveAnswerMaxSize];
	uint8_t		incr[LinSlaveAnswerMaxSize];
};


struct RxContext {
	uint8_t*			data;
	uint32_t			index;
	uint32_t			size;
	IndexType			p;
	RxState				state;
	bool				enabled;
};

struct TxContext {
	uint8_t*			data;
	uint32_t			index;
	uint32_t			size;
	IndexType			p;
	TxState				state;
};

struct Module {
	HwConfig			hwConfig;
	Config				config;
	TxContext			tx;
	RxContext			rx;
	LinSlaveAnswer		answer[LinSlaveAnswerCount];
};

constexpr const PoolType pool = PoolType::PeriphToUsb;
constexpr const uint32_t LinDataToUsbHeadSize = sizeof(LinDataToUsbHead);
constexpr const uint32_t UsbToLinDataHeadSize = sizeof(UsbToLinDataHead);

static Module gLin0;

Status sendAnswer( Module* const module, uint8_t answerIndex );

void updateRxTimeout(  Module* const module ) {
	TIM_ARR(module->hwConfig.timer) = module->config.rx.timeout;
	TIM_EGR(module->hwConfig.timer) |= TIM_EGR_UG;
	while( (TIM_EGR(module->hwConfig.timer) && TIM_EGR_UG) );
}

void startRxTimer( Module* const module ) {
	TIM_CR1(module->hwConfig.timer) &= ~(TIM_CR1_CEN);
	TIM_SR(module->hwConfig.timer) = 0;
	TIM_CNT(module->hwConfig.timer) = 0;
	asm("isb; dsb");
	TIM_CR1(module->hwConfig.timer) |= TIM_CR1_CEN;
}

void stopRxTimer( Module* const module ) {
	TIM_CR1(module->hwConfig.timer) &= ~(TIM_CR1_CEN);
	asm("isb; dsb");
	TIM_SR(module->hwConfig.timer) = 0;
	nvic_clear_pending_irq( module->hwConfig.timer_irq );
}

Status init( Module* const module ) {

	if ( nullptr == module ) return Status::Error;

	module->tx.state = TxState::Idle;
	module->rx.state = RxState::Idle;
	module->hwConfig.usart = USART1;
	module->hwConfig.timer = TIM1;
	module->hwConfig.usart_irq = 37;
	module->hwConfig.timer_irq = 25;

	module->hwConfig.lin_en = { GPIOB, GPIO11 };
	module->config.baud = 9600;
	module->config.rx.size = 16;
	module->config.rx.timeout = 2000;

	nvic_disable_irq( module->hwConfig.timer_irq );
	nvic_disable_irq( module->hwConfig.usart_irq );

	// Enable LIN port
	gpio_set( module->hwConfig.lin_en.port, module->hwConfig.lin_en.pin );

	// Enable USART module
	usart_enable( module->hwConfig.usart );

	// Initialize hardware
	usart_set_mode( module->hwConfig.usart, USART_MODE_TX_RX );
	usart_set_lin_mode( module->hwConfig.usart );
	usart_set_baudrate( module->hwConfig.usart, module->config.baud );
	usart_set_databits( module->hwConfig.usart, 8 );
	usart_set_stopbits( module->hwConfig.usart, USART_CR2_STOPBITS_1 );
	usart_set_flow_control( module->hwConfig.usart, USART_FLOWCONTROL_NONE );

	TIM_CR1( module->hwConfig.timer ) = TIM_CR1_OPM;
	TIM_CR2( module->hwConfig.timer ) = 0;
	TIM_SMCR( module->hwConfig.timer ) = 0;
	TIM_DIER( module->hwConfig.timer ) = TIM_DIER_UIE;
	TIM_PSC( module->hwConfig.timer ) = 72;
	updateRxTimeout( module );
	TIM_CNT( module->hwConfig.timer ) = 0;
	TIM_SR( module->hwConfig.timer ) = 0;
	nvic_clear_pending_irq( module->hwConfig.timer_irq );

	// Clear RX interrupt, by reading register
	usart_recv( module->hwConfig.usart );
	// Clear all interrupts
	USART_SR(module->hwConfig.usart) = 0;
	// Enable interrupts
	USART_CR2(module->hwConfig.usart) |= USART_CR2_LBDIE;
	USART_CR1(module->hwConfig.usart) |= USART_CR1_TCIE | USART_CR1_RXNEIE;
	// Setup interrupt priority
	nvic_set_priority( module->hwConfig.usart_irq, 0x10 );
	nvic_set_priority( module->hwConfig.timer_irq, 0x10 );
	// Clear interrupts if any pending
	nvic_clear_pending_irq( module->hwConfig.usart_irq );
	nvic_clear_pending_irq( module->hwConfig.timer_irq );
	// Enable interrups
	nvic_enable_irq( module->hwConfig.usart_irq  );
	nvic_enable_irq( module->hwConfig.timer_irq  );

	return Status::Success;
}

Status prepareNewReceive( Module* const module, const RxState newRxState  ) {
	if ( nullptr == module ) return Status::WrongArgument;
	if ( !module->rx.enabled ) {
		module->rx.state = RxState::Idle;
		return Status::Success;
	}
	// Allocate new USB buffer
	const size_t TotalBufferSize = LinDataToUsbHeadSize + module->config.rx.size;
	module->rx.p = usbBufferAlloc( pool, TotalBufferSize );
	if ( InvalidIndex == module->rx.p ) return Status::NoMemory;
	// Fill USB buffer head
	const time_us_t startTs = SystemTimer::getTimeUs();
	uint8_t* const data = usbBufferGetDataPtr( pool, module->rx.p );
	LinDataToUsbHead* const head = reinterpret_cast<LinDataToUsbHead* const>(data);
	head->id = UsbPacketId::LinDataToUsb;
	head->startTs = startTs;
	// Update data indexes and pointers
	module->rx.data = LinDataToUsbHeadSize + data;
	module->rx.size = module->config.rx.size;
	module->rx.index = 0;
	module->rx.state = newRxState;
	startRxTimer( module );
	return Status::Success;
}

Status completeReceive( Module* const module, bool onTimeout = false) {
	if ( nullptr == module ) return Status::WrongArgument;
	if ( InvalidIndex == module->rx.p ) return Status::Error;
	stopRxTimer(module);

	// Update head
	const time_us_t endTs = SystemTimer::getTimeUs();
	uint8_t* const data = usbBufferGetDataPtr( pool, module->rx.p );
	LinDataToUsbHead* const head = reinterpret_cast<LinDataToUsbHead* const>(data);
	head->endTs = endTs;

	if ( ( onTimeout ) &&
		 ( RxState::WaitLinData == module->rx.state ) &&
		 ( LinMasterFrameRequestSize == module->rx.index ) &&
		 ( LinSyncByte == module->rx.data[0] ) ) {

		const uint8_t protectedId = module->rx.data[1];
		const Status status = sendAnswer( module, protectedId );
		if ( ( Status::Pending == status ) ||
			 ( Status::Success == status ) ) {
			return status;
		}

	}

	if ( RxState::WaitLinData == module->rx.state ) {
		head->frameType = LinFrameType::BreakAndData;
	} else {
		head->frameType = LinFrameType::Data;
	}
	usbBufferUpdateSize( pool, module->rx.p, module->rx.index + LinDataToUsbHeadSize );
	usbBufferCommit( pool, module->rx.p );
	module->rx.state = RxState::Idle;
	return Status::Success;
}

void onBreakIrq( Module* const module ) {
	if ( nullptr == module ) return;
	stopRxTimer(module);

	if ( RxState::Idle != module->rx.state ) {

		if ( ( module->rx.index > 0) &&
			 ( 0 == module->rx.data[ (module->rx.index - 1) ] ) ) {
			module->rx.index--;
		}

		if ( module->rx.index > 0 ) {
			completeReceive(module);
		} else {
			module->rx.state = RxState::WaitLinData;
			startRxTimer(module);
		}

	}

	if ( RxState::Idle == module->rx.state ) {
		prepareNewReceive( module,  RxState::WaitLinData );
	}
}

void onRxReadyIrq( Module* const module ) {
	if ( nullptr == module ) return;
	stopRxTimer(module);

	const uint8_t data = usart_recv( module->hwConfig.usart );

	if ( RxState::Idle == module->rx.state ) {
		prepareNewReceive( module,  RxState::WaitData );
	}

	if ( ( RxState::WaitData == module->rx.state ) ||
		 ( RxState::WaitLinData == module->rx.state ) ) {

		module->rx.data[module->rx.index] = data;
		module->rx.index++;

		if  ( ( RxState::WaitLinData == module->rx.state) &&
			  ( LinMasterFrameRequestSize == module->rx.index ) &&
			  ( LinSyncByte == module->rx.data[0] ) ) {
			// Send answer frame if possible :-)
			const uint8_t protectedId = module->rx.data[1];
			sendAnswer( module, protectedId );
		}

		if (module->rx.index >= module->rx.size ) {
			completeReceive(module);
		} else {
			startRxTimer(module);
		}

	}

}

void onTimerIrq( Module* const module ) {
	if ( nullptr == module ) return;
	if ( ( RxState::WaitData ==  module->rx.state ) ||
		 ( RxState::WaitLinData ==  module->rx.state ) ) {
			completeReceive(module, true);
	}
}

void onAnswerSent( Module* const module );

void onTxCompleteIrq( Module* const module ) {
	if ( nullptr == module ) return;
	if ( TxState::Idle == module->tx.state ) return;

	if (  module->tx.index >= module->tx.size ) {

		if ( TxState::SendData == module->tx.state ) {
			usbBufferFree( PoolType::UsbToPeriph, module->tx.p );
		} else if ( TxState::SendAnswer == module->tx.state ) {
			onAnswerSent( module );
		}
		module->tx.state = TxState::Idle;

	} else {
		usart_send( module->hwConfig.usart, module->tx.data[module->tx.index] );
		module->tx.index++;
	}
}

void onAnswerSent( Module* const module ) {
	if ( nullptr == module ) return;
	const uint8_t answerIndex = module->tx.p;
	if ( LinSlaveAnswerCount <= answerIndex ) return;

	LinSlaveAnswer* const answer = &(module->answer[answerIndex]);

	if ( ( nullptr != answer ) &&
		 ( answer->size > 0 ) &&
		  (answer->size <= LinSlaveAnswerMaxSize) ) {

		uint16_t crc = 0;

		// Update CRC
		if ( LinCrcType::Std2x == answer->crc ) {
			crc += answer->id;
		}

		// Update value by increment/decremnet
		for( uint8_t byteInd = 0; byteInd < answer->size; byteInd++  ){
			answer->data[byteInd] += answer->incr[byteInd];
			if ( byteInd < (answer->size -1) ) {
				crc += answer->data[byteInd];
				if  (crc >= 0x100) crc -= 0xFF;
			}
		}

		if  ( ( LinCrcType::Std1x == answer->crc ) ||
			  ( LinCrcType::Std2x == answer->crc ) ) {
			// Update CRC
			answer->data[answer->size - 1] = ~crc;
		}

	}

}

uint8_t calcLinId( const uint8_t id ) {
	const uint8_t newId = id & 0x3F;
	const uint8_t p0 = (id ^ (id >> 1) ^ (id >> 2) ^ (id >> 4)) & 0x1;
	const uint8_t p1 = ~(((id >> 1) ^ (id >> 3) ^ (id >> 4) ^ (id >> 5))) & 0x1;
	const uint8_t resId =  (p1 << 7) | (p0 << 6) | newId;
	return resId;
}

Status sendAnswer( Module* const module, uint8_t protectedId ) {
	if ( nullptr == module ) return Status::WrongArgument;
	if ( TxState::Idle != module->tx.state ) return Status::Busy;

	for( uint8_t ind = 0; ind < LinSlaveAnswerCount; ind++ ) {
		if ( ( protectedId == module->answer[ind].id ) &&
			 ( module->answer[ind].size  > 0) ) {
				module->tx.state = TxState::SendAnswer;
				module->tx.p = ind;
				module->tx.data = module->answer[ind].data;
				module->tx.size =  module->answer[ind].size;
				module->tx.index = 0;
				usart_send( module->hwConfig.usart, module->tx.data[module->tx.index] );
				module->tx.index++;
				return Status::Pending;
			}
	}

	return Status::Busy;
}


Status send(  Module* const module, const IndexType p ) {

	Status status = Status::Error;
	if ( nullptr == module ) return Status::WrongArgument;
	if ( TxState::Idle != module->tx.state ) return Status::Busy;

	const uint32_t bufSize = usbBufferGetSize( PoolType::UsbToPeriph, p );
	uint8_t* const buf = usbBufferGetDataPtr( PoolType::UsbToPeriph, p );

	const UsbToLinDataHead* const head = reinterpret_cast<const UsbToLinDataHead* const>(buf);
	if ( WaitMode::WaitTimestamp == head->mode ) {
		const time_us_t ts = SystemTimer::getTimeUs();
		CircularIndex<time_us_t> gotTs(head->ts);
		CircularIndex<time_us_t> curTs(ts);
		if ( curTs < gotTs ) {
			return Status::Busy;
		}
	}

	usbBufferCheckout( PoolType::UsbToPeriph, p );
	module->tx.p = p;
	module->tx.data = buf + UsbToLinDataHeadSize;
	module->tx.size = bufSize - UsbToLinDataHeadSize;
	module->tx.index = 0;

	switch( head->frameType ) {

	case LinFrameType::BreakAndData:
	{
		usart_send_break( module->hwConfig.usart );

		if ( 0 == module->tx.size ) {
			usbBufferFree( PoolType::UsbToPeriph, module->tx.p );
			return Status::Success;
		}

		module->tx.state = TxState::SendData;
		usart_send( module->hwConfig.usart, module->tx.data[module->tx.index] );
		module->tx.index++;
		status = Status::Pending;
	}
	break;

	case LinFrameType::Data:
	{
		if ( 0 == module->tx.size ) {
			usbBufferFree( PoolType::UsbToPeriph, module->tx.p );
			return Status::Error;
		}
		module->tx.state = TxState::SendData;
		usart_send( module->hwConfig.usart, module->tx.data[module->tx.index] );
		module->tx.index++;
		status = Status::Pending;
	}
	break;

	default:
	{
		usbBufferFree( PoolType::UsbToPeriph, module->tx.p );
	}
	break;

	}

	return status;
}

Status rxOnOff( Module* const module, const IndexType p ) {
	uint8_t* const buf = usbBufferGetDataPtr( PoolType::UsbToPeriph, p );
	const UsbToLinRxOnOffHead* const head = reinterpret_cast<const UsbToLinRxOnOffHead* const>(buf);
	usbBufferCheckout(PoolType::UsbToPeriph, p);
	module->rx.enabled = (LinRxOnOff::On == head->rx);
	usbBufferFree( PoolType::UsbToPeriph, p );
	return Status::Success;
}


Status setBaud( Module* const module, const IndexType p ) {
	uint8_t* const buf = usbBufferGetDataPtr( PoolType::UsbToPeriph, p );
	const UsbToLinBaudHead* const head = reinterpret_cast<const UsbToLinBaudHead* const>(buf);
	module->config.baud = head->baud;
	usart_set_baudrate( module->hwConfig.usart, module->config.baud);
	usbBufferCheckout( PoolType::UsbToPeriph, p);
	usbBufferFree( PoolType::UsbToPeriph, p);
	return Status::Success;
}

Status setTimeout( Module* const module, const IndexType p ) {
	uint8_t* const buf = usbBufferGetDataPtr( PoolType::UsbToPeriph, p );
	const UsbToLinTimeoutHead* const head = reinterpret_cast<const UsbToLinTimeoutHead* const>(buf);
	module->config.rx.timeout = head->timeout;
	usbBufferCheckout( PoolType::UsbToPeriph, p);
	usbBufferFree( PoolType::UsbToPeriph, p);
	updateRxTimeout(module);
	return Status::Success;
}

Status setRxSize( Module* const module, const IndexType p ) {
	uint8_t* const buf = usbBufferGetDataPtr( PoolType::UsbToPeriph, p );
	const UsbToLinMaxRxSizeHead* const head = reinterpret_cast<const UsbToLinMaxRxSizeHead* const>(buf);
	module->config.rx.size = head->size;
	usbBufferCheckout( PoolType::UsbToPeriph, p);
	usbBufferFree( PoolType::UsbToPeriph, p);
	updateRxTimeout(module);
	return Status::Success;
}

Status updateLinAnswer( Module* const module, const IndexType p ) {

	uint8_t* const buf = usbBufferGetDataPtr( PoolType::UsbToPeriph, p );
	const UsbToLinAnswerHead* const head = reinterpret_cast<const UsbToLinAnswerHead* const>(buf);
	if ( ( head->index >= LinSlaveAnswerCount) ||
		 ( LinSlaveAnswerMaxSize < head->size ) ) {
		usbBufferCheckout(PoolType::UsbToPeriph, p);
		usbBufferFree( PoolType::UsbToPeriph, p );
		return Status::Error;
	}

	const uint8_t answerIndex = head->index;
	LinSlaveAnswer* const answer = &(module->answer[answerIndex]);
	answer->crc = head->crc;
	answer->size = head->size;
	answer->id = head->protectedId;
	uint8_t* ptr = buf + sizeof(UsbToLinAnswerHead);
	memcpy( &(answer->data[0]), ptr, answer->size);
	ptr += answer->size;
	memcpy( &(answer->incr[0]), ptr, answer->size);

	usbBufferCheckout(PoolType::UsbToPeriph, p);
	usbBufferFree( PoolType::UsbToPeriph, p );
	return Status::Success;

}

Status process( Module* const module, const IndexType p, const UsbPacketId packetId ) {
	if ( nullptr == module ) return Status::WrongArgument;
	if ( InvalidIndex == p ) return Status::WrongArgument;

	if ( UsbPacketId::UsbToLinData == packetId ) {
		return send(module, p);
	}

	if ( UsbPacketId::UsbToLinRxOnOff == packetId ) {
		return rxOnOff(module, p);
	}

	if ( UsbPacketId::UsbToLinBaud == packetId ) {
		return setBaud(module, p);
	}

	if ( UsbPacketId::UsbToLinTimeout == packetId ) {
		return setTimeout(module, p);
	}

	if ( UsbPacketId::UsbToLinAnswer == packetId ) {
		return updateLinAnswer(module, p);
	}

	if ( UsbPacketId::UsbToLinRxSize == packetId ) {
		return setRxSize(module, p);
	}


	return Status::WrongArgument;
}


}


Lin::Module* lin0 = &Lin::gLin0;

void usart_send_break( const uint32_t usart ) {
	USART_CR1(usart) |= USART_CR1_SBK;
}

void usart_set_lin_mode( uint32_t usart  ) {
	USART_CR2(usart) |=  USART_CR2_LINEN;
}


extern "C" void TIM1_IRQHandler(void) {
	TIM_SR(lin0->hwConfig.timer) &= ~(TIM_SR_UIF);
	nvic_clear_pending_irq( lin0->hwConfig.timer_irq );
	Lin::onTimerIrq(lin0);
}

extern "C" void USART1_IRQHandler(void) {

	uint16_t status = USART_SR(USART1);

	do {

		if ( status & USART_SR_TC ){
			Lin::onTxCompleteIrq(lin0);
			USART_SR(USART1) &= (~(USART_SR_TC));
		}

		if ( status & USART_SR_RXNE ) {
			Lin::onRxReadyIrq(lin0);
			USART_SR(USART1) &= (~(USART_SR_RXNE));
		}

		if ( status & USART_SR_LBD ) {
#if 0
			if ( status & USART_SR_FE ) {
				volatile uint16_t localStatus = USART_SR(USART1);
				volatile uint16_t localData = USART_DR(USART1);
				localStatus = localStatus;
				localData = localData;
			}
#endif
			Lin::onBreakIrq(lin0);
			USART_SR(USART1) &= (~(USART_SR_LBD));
		}

		status = USART_SR(USART1);

	} while(status & ( USART_SR_LBD | USART_SR_TC | USART_SR_RXNE) );


	nvic_clear_pending_irq( lin0->hwConfig.usart_irq );
}




