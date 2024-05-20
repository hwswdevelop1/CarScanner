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
#include <libopencm3/stm32/timer.h>
#include "Memory.h"
#include "Lin.h"
#include "UsbPacketBuffer.h"
#include "IntLock.h"


#define USART_CR2_LINEN 	(1 << 14)
static void usart_send_break( const uint32_t usart );
static void usart_set_lin_mode( uint32_t usart );



namespace Lin {

static constexpr const uint8_t LinSyncByte = 0x55;
static constexpr const uint8_t LinMaxAutoAnswerCount = 8;
static constexpr const uint8_t LinMaxAnswerSize = 9;
static constexpr const uint16_t LinMinTimeoutUs = 100;
static constexpr const uint16_t LinMaxTimeoutUs = 50000;
static constexpr const uint16_t LinMinBaud = 300;
static constexpr const uint16_t LinMaxBaud = 57600;


enum class TxState :uint8_t {
	Idle,
	SendData
};


enum class RxState : uint8_t {
	Idle,
	WaitLinData,
	WaitData
};

struct LinSlaveAutoAnswer {
	uint8_t 	id = 0;
	uint8_t		size = 0;
	uint8_t		data[LinMaxAnswerSize];
};

struct RxContext {
	size_t				offset;
	size_t				count;
	uint8_t*			ptr;
	IndexType			p;
	RxState				state;
};

struct TxContext {
	uint8_t*			ptr;
	size_t				count;
	IndexType			p;
	TxState				state;
};

struct Module {
	HwConfig			hwConfig;
	Config				config;
	TxContext			tx;
	RxContext			rx;
	LinSlaveAutoAnswer	autoAnswer[LinMaxAutoAnswerCount];
};

void startTimer( Module* const module );

Status hwInit( Module* const module, const HwConfig* const hwConfig ) {

	if ( nullptr == module ) return Status::Error;
	if ( nullptr == hwConfig) return Status::Error;

	nvic_disable_irq( module->hwConfig.timer );
	nvic_disable_irq( module->hwConfig.usart_irq );

	module->hwConfig.usart = hwConfig->usart;
	module->hwConfig.usart_irq = hwConfig->usart_irq;
	module->hwConfig.timer_irq = hwConfig->timer_irq;
	module->hwConfig.timer = hwConfig->timer;
	module->hwConfig.lin_en = hwConfig->lin_en;
	module->tx.state = TxState::Idle;
	module->rx.state = RxState::Idle;

	// Copy handle
	usart_t usart = module->hwConfig.usart;

	gpio_clear( module->hwConfig.lin_en.port, module->hwConfig.lin_en.pin );

	// Enable USART module
	usart_enable( USART1 );
	// Initialize hardware
	usart_set_mode( usart, USART_MODE_TX_RX );
	usart_set_lin_mode(usart);
	usart_set_baudrate( usart, 9600 );
	usart_set_databits(usart, 8 );
	usart_set_stopbits( usart, USART_CR2_STOPBITS_1 );
	usart_set_flow_control(usart, USART_FLOWCONTROL_NONE);

	// Config Timer
	nvic_disable_irq(module->hwConfig.timer);
	TIM_CR1(module->hwConfig.timer) = TIM_CR1_OPM;
	TIM_CR2(module->hwConfig.timer) = 0;
	TIM_SMCR(module->hwConfig.timer) = 0;
	TIM_DIER(module->hwConfig.timer) = TIM_DIER_UIE;
	TIM_PSC(module->hwConfig.timer) = 72;
	TIM_ARR(module->hwConfig.timer) = module->config.rxTimeout;
	TIM_CNT(module->hwConfig.timer) = 0;
	TIM_SR(module->hwConfig.timer) = 0;
	nvic_clear_pending_irq(module->hwConfig.timer_irq);

	// Enable LIN port
	gpio_set( module->hwConfig.lin_en.port, module->hwConfig.lin_en.pin );
	// Clear all interrupts
	USART_SR(usart) = 0;
	// Clear rx interrupt
	usart_recv( usart );
	// Enable interrupts
	USART_CR2(usart) |= USART_CR2_LBDIE;
	usart_enable_tx_complete_interrupt( usart );
	usart_enable_rx_interrupt( usart );

	nvic_set_priority( module->hwConfig.usart_irq, 0x10 );
	nvic_set_priority( module->hwConfig.timer_irq, 0x10 );
	// Clear interrupts if any pending
	nvic_clear_pending_irq( module->hwConfig.usart_irq );
	nvic_enable_irq( module->hwConfig.usart_irq  );

	return Status::Success;
}

Status config( Module* const module, const Config* const config ) {
	if ( nullptr == module ) return Status::Error;
	if ( nullptr == config) return Status::Error;
	module->config.baud = config->baud;
	module->config.maxRxSize = config->maxRxSize;
	module->config.rxTimeout = config->rxTimeout;
	usart_set_baudrate( module->hwConfig.usart, module->config.baud );
	return Status::Success;
}

void startTimer( Module* const module ) {
	nvic_disable_irq(module->hwConfig.timer);
	TIM_ARR(module->hwConfig.timer) = module->config.rxTimeout;
	TIM_EGR(module->hwConfig.timer) |= TIM_EGR_UG;
	while( (TIM_EGR(module->hwConfig.timer) && TIM_EGR_UG) );

	TIM_CNT(module->hwConfig.timer) = 0;
	TIM_SR(module->hwConfig.timer) = 0;
	nvic_clear_pending_irq(module->hwConfig.timer_irq);
	nvic_enable_irq(module->hwConfig.timer_irq);
	TIM_CR1(module->hwConfig.timer) |= TIM_CR1_CEN;
}

void stopTimer( Module* const module ) {
	nvic_disable_irq(module->hwConfig.timer_irq);
	TIM_CR1(module->hwConfig.timer) &= ~(TIM_CR1_CEN);
}

void onTxCompleteIrq( Module* const module ) {
	if ( nullptr == module ) return;

	// Clear Tx complete IRQ
	USART_SR(USART1) &= (~(USART_SR_TC));

	switch( module->tx.state ) {

	case TxState::SendData:
		{
			if ( ( nullptr == module->tx.ptr) ||
				 ( 0 == module->tx.count ) ) {
				gpio_set(GPIOC, GPIO7);
				/// Try to free usbBuffer
				if  ( InvalidIndex != module->tx.p ) {
					usbBufferFree( PoolType::UsbToPeriph, module->tx.p );
				}
				module->tx.ptr = nullptr;
				module->tx.p = InvalidIndex;
				module->tx.state = TxState::Idle;
				gpio_clear(GPIOC, GPIO7);
				break;
			}

			const uint8_t data = *(module->tx.ptr);
			usart_send( module->hwConfig.usart,  data );
			module->tx.ptr++;
			module->tx.count--;
		}
		break;

	default:
		module->tx.state = TxState::Idle;
		break;
	}


}

enum class FrameType : uint8_t {
	StartWithBreak,
	StartWithoutBreak,
	Config
};


bool linAutoAnswer( Module* const module, const uint8_t linId ){
	for( size_t index = 0; index < LinMaxAutoAnswerCount; index++ ) {
		LinSlaveAutoAnswer* answer = &(module->autoAnswer[index]);
		if ( answer->id != linId ) continue;
		if ( 0 == answer->size ) continue;
		if ( answer->size > LinMaxAnswerSize ) continue;
		// Allocate USB buffer and send packet
		module->tx.ptr = &(answer->data[1]);
		module->tx.count = (answer->size - 1);
		usart_send( module->hwConfig.usart, answer->data[0] );
		module->tx.state = TxState::SendData;
		return true;
	}
}

bool linAutoAnswerCheck( Module* const module ){
	// So if tx state is not Idle, it is not possible to send answer
	if ( TxState::Idle != module->tx.state ) return false;
	// So if received size is not equlas two ( sync, id ), it is not master frame
	if ( 2 != module->rx.count) return false;
	// So check is it LIN frame, or data
	const size_t usbHeadOffset = 0;
	UsbPacketHead* head = reinterpret_cast<UsbPacketHead*>( &(module->rx.ptr[usbHeadOffset]) );
	// Check frame type
	if ( LinFrameType::LinBreak != head->type.lin ) return false;
	const size_t dataOffset = module->rx.offset;
	uint8_t* const data = &( module->rx.ptr[dataOffset] );
	// Check Sync byte presence
	if ( LinSyncByte != data[0] ) return false;
	// Get LinId
	const uint8_t linId = data[1];
	// So, try find automatic answer, and send it
	return linAutoAnswer( module, linId );
}

void linCompleteRx( Module* const module, const bool autoAnswerEnabled = false ){
	if ( nullptr == module ) return;
	stopTimer(module);

	if ( autoAnswerEnabled ) {
		const bool answerSent = linAutoAnswerCheck(module);
		if ( answerSent ) {
			// Answer was sent.
			// Don't complete current Lin RX transaction
			return;
		}
	}

	gpio_set(GPIOC, GPIO6);
	module->rx.state = RxState::Idle;
	auto p = module->rx.p;
	if ( InvalidIndex != p ) {
		// Commit USB buffer if exists
		constexpr const size_t UsbHeadSize = sizeof(UsbPacketHead);
		usbBufferUpdateSize( PoolType::LinToUsb, p, (module->rx.count + UsbHeadSize) );
		usbBufferCommit( PoolType::LinToUsb, p );
	}
	module->rx.p = InvalidIndex;
	module->rx.ptr = nullptr;
	module->rx.offset = 0;
	gpio_clear(GPIOC, GPIO6);

}


void linUpdatePacketType( Module* const module, const RxState rxState ){
	if ( nullptr == module ) return;
	UsbPacketHead* head = reinterpret_cast<UsbPacketHead*>(module->rx.ptr);
	module->rx.offset = sizeof(UsbPacketHead);
	module->rx.state = rxState;
	module->rx.count = 0;
	if (rxState == RxState::WaitData) {
		head->type.lin = LinFrameType::LinNoBreak;
	} else if (rxState == RxState::WaitLinData) {
		head->type.lin = LinFrameType::LinBreak;
	}
	startTimer(module);
}

void linStartNewRxTransfer( Module* const module, const RxState rxState ){
	if ( nullptr == module ) return;
	constexpr const size_t UsbHeadSize = sizeof(UsbPacketHead);
	auto p = usbBufferAlloc( PoolType::LinToUsb, module->config.maxRxSize + UsbHeadSize );
	module->rx.p = p;
	if ( InvalidIndex != p ) {
		module->rx.ptr = usbBufferGetDataPtr( PoolType::LinToUsb, p );
		linUpdatePacketType( module, rxState );
	} else {
		module->rx.ptr = nullptr;
		module->rx.offset = 0;
		module->rx.count = 0;
		module->rx.state = RxState::Idle;
	}
}



void onBreakIrq( Module* const module ) {
	if ( nullptr == module ) return;
	if ( !(USART_SR(module->hwConfig.usart) & (USART_SR_LBD)) ) return;

	int_lock_t key = intLock();

	// Anyway stop timer.
	stopTimer(module);

	// Receive data if receiver module got LBD as data
	const uint8_t rxData = usart_recv(module->hwConfig.usart);
	(void)(rxData);
	// Clear LBD interrupt status
	USART_SR(module->hwConfig.usart) &= (~(USART_SR_LBD));
	// Clear RXNE interrupt status
	USART_SR(module->hwConfig.usart) &= (~(USART_SR_RXNE));


	bool startNewTransfer = true;

	if ( RxState::WaitLinData == module->rx.state ) {
		const size_t totalBytesReceived = module->rx.count;
		if ( totalBytesReceived > 0) {
			const size_t dataOffset = (module->rx.offset + totalBytesReceived - 1);
			const uint8_t data = module->rx.ptr[dataOffset];
			if ( 0 == data ) {
				module->rx.count--;
			}
		}
		linCompleteRx(module);
	}

	// Check if previous frame in receive state
	if ( RxState::WaitData == module->rx.state ) {
		// So if it is not Idle state before
		// I have some received data, send it to host, by commit buffer
		const size_t totalBytesReceived = module->rx.count;
		if ( 1 == totalBytesReceived ) {
			const size_t dataOffset = module->rx.offset;
			const uint8_t data = module->rx.ptr[dataOffset];
			if ( 0 == data ) {
				linUpdatePacketType(module, RxState::WaitLinData);
				startNewTransfer = false;
			}
		}

		if ( totalBytesReceived > 1 ) {
			const size_t dataOffset = (module->rx.offset + totalBytesReceived - 1);
			const uint8_t data = module->rx.ptr[dataOffset];
			if ( 0 == data ) {
				module->rx.count--;
			}
			linCompleteRx(module);
		}

	}

	if ( ( startNewTransfer ) &&
		 ( RxState::Idle == module->rx.state ) ) {
		linStartNewRxTransfer( module, RxState::WaitLinData );
	}

	intUnlock(key);

}


void onRxReadyIrq( Module* const module ) {
	if ( nullptr == module ) return;
	if ( !(USART_SR(module->hwConfig.usart) & (USART_SR_RXNE)) ) return;
	if ( (USART_SR(module->hwConfig.usart) & (USART_SR_LBD)) ) return;

	int_lock_t key = intLock();

	stopTimer(module);

	const uint8_t rxData = usart_recv(module->hwConfig.usart);

	if ( RxState::Idle ==  module->rx.state ) {
		linStartNewRxTransfer( module, RxState::WaitData );
	}

	if ( ( RxState::WaitData ==  module->rx.state ) ||
		 ( RxState::WaitLinData ==  module->rx.state ) ) {

		auto p = module->rx.p;
		if ( InvalidIndex != p ) {

			// Append data
			if ( module->rx.count < module->config.maxRxSize ) {
				const size_t offset = module->rx.offset + module->rx.count;
				module->rx.ptr[offset] = rxData;
				module->rx.count++;
				startTimer(module);
			} else {
				linCompleteRx(module);
			}
		}
	}

	intUnlock(key);

}

void onTimerIrq( Module* const module ) {
	if ( nullptr == module ) return;
	int_lock_t key = intLock();
	stopTimer(module);
	if ( ( RxState::WaitData ==  module->rx.state ) ||
		 ( RxState::WaitLinData ==  module->rx.state ) ){
		linCompleteRx(module, true);
	}
	intUnlock(key);

}

Status process( Module* const module,  IndexType p ) {
	if ( nullptr == module ) return Status::Error;
	if ( InvalidIndex == p ) return Status::Error;

	Status status = Status::Busy;

	int_lock_t key = intLock();

	uint8_t* dptr = usbBufferGetDataPtr( PoolType::UsbToPeriph, p );
	const size_t size = usbBufferGetSize( PoolType::UsbToPeriph, p );

	do {
		if ( size < sizeof(UsbPacketHead) ) {
			// Packet too short
			break;
		}

		// Get USB head
		const UsbPacketHead* usbHead = reinterpret_cast<UsbPacketHead*>(dptr);

		if ( ModuleId::Lin1 != usbHead->id ) {
			// Packet is not for LIN module.
			break;
		}

		if ( TxState::Idle !=  module->tx.state ) {
			// It is not appropriate state to get Lin packet now
			break;
		}

		// Process packet
		usbBufferCheckout( PoolType::UsbToPeriph, p );


		if ( size < sizeof(UsbPacketHead) ) {
			usbBufferFree( PoolType::UsbToPeriph, p );
			status = Status::Error;
			break;
		}





		switch( usbHead->type.lin ) {

		case LinFrameType::LinBreak:
		{
			module->tx.p = p;
			module->tx.ptr = ( dptr + sizeof(UsbPacketHead) );
			module->tx.count = ( size - sizeof(UsbPacketHead) );
			module->tx.state = TxState::SendData;
			usart_send_break(module->hwConfig.usart);
			status = Status::Pending;
		}
		break;

		case LinFrameType::LinNoBreak:
		{
			module->tx.p = p;
			module->tx.ptr = ( dptr + sizeof(UsbPacketHead) );
			module->tx.count = ( size - sizeof(UsbPacketHead) );
			if ( module->tx.count > 0 ) {
				module->tx.state = TxState::SendData;
				uint8_t data = *(module->tx.ptr);
				module->tx.ptr++;
				module->tx.count--;
				usart_send(module->hwConfig.usart, data);
				status = Status::Pending;
			} else {
				module->tx.state = TxState::Idle;
				usbBufferFree( PoolType::UsbToPeriph, p );
				status = Status::Error;
			}
		}
		break;

		case LinFrameType::LinSetBaud:
		{
			if ( (sizeof(UsbPacketHead) + 2) == size ) {
				// Get params
				uint8_t* const data = (dptr + sizeof(UsbPacketHead));
				uint16_t baud = (((uint16_t)data[1]) << 8) | data[0];
				if ( ( baud >= LinMinBaud) && ( baud <= LinMaxBaud) ) {
					module->config.baud = baud;
					// Config module settings
					config(module, &module->config);
				}
			}
			usbBufferFree( PoolType::UsbToPeriph, p);
		}
		break;

		case LinFrameType::LinSetTimeout:
		{
			if ( (sizeof(UsbPacketHead) + 2) == size ) {
				// Get params
				uint8_t* const data = (dptr + sizeof(UsbPacketHead));
				uint16_t timeout = (((uint16_t)data[1]) << 8) | data[0];
				if ( ( timeout >= LinMinTimeoutUs ) && (timeout <= LinMaxTimeoutUs ) ) {
					module->config.rxTimeout = timeout;
					// Config module settings
					config(module, &module->config);
				}
			}
			usbBufferFree( PoolType::UsbToPeriph, p);
		}
		break;

		case LinFrameType::LinSetAutoAnswer:
		{
			if ( size >= (sizeof(UsbPacketHead) + 2) ) {
			uint8_t* const data = (dptr + sizeof(UsbPacketHead));
				const uint8_t index = data[0];
				if ( index < LinMaxAutoAnswerCount ) {
					const uint8_t dataSize = data[2];
					const uint8_t remainSize = ( size - sizeof(UsbPacketHead) - 2);
					LinSlaveAutoAnswer* answer = &(module->autoAnswer[index]);
					const uint8_t copySize = ( dataSize < remainSize ) ? dataSize : remainSize;
					answer->id = data[1];
					answer->size = copySize;
					if ( copySize > 0 ) {
						memcpy( &(answer->data[0]), &data[3], copySize );
					}
				}
			}
			usbBufferFree( PoolType::UsbToPeriph, p);
		}
		break;

		default:
			usbBufferFree( PoolType::UsbToPeriph, p);
		break;
		}
	} while(false);

	intUnlock(key);

	return status;

}

}


static Lin::Module gLin0;
Lin::Module* lin0 = &gLin0;

void usart_send_break( const uint32_t usart ) {
	USART_CR1(usart) |= USART_CR1_SBK;
}

void usart_set_lin_mode( uint32_t usart  ) {
	USART_CR2(usart) |=  USART_CR2_LINEN;
}

extern "C" void TIM1_IRQHandler(void) {
	TIM_SR(lin0->hwConfig.timer) &= ~(TIM_SR_UIF);
	Lin::onTimerIrq(lin0);
	nvic_clear_pending_irq( lin0->hwConfig.timer_irq );
}

extern "C" void USART1_IRQHandler(void) {

	uint16_t status = USART_SR(USART1);
	if ( status & USART_SR_TC ){
		Lin::onTxCompleteIrq(lin0);
	}

	status = USART_SR(USART1);
	if ( status & USART_SR_RXNE ) {
		Lin::onRxReadyIrq(lin0);
	}

	status = USART_SR(USART1);
	if ( status & USART_SR_LBD ) {
		Lin::onBreakIrq(lin0);
	}

	nvic_clear_pending_irq( lin0->hwConfig.usart_irq );
}



