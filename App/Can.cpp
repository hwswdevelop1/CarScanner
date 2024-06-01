/*
 * Can.cpp
 *
 *  Created on: May 24, 2024
 *      Author: Evgeny Sobolev. 02/09/1984y.b
 */

#include "libopencm3/stm32/can.h"
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/f1/gpio.h>
#include "SystemTimer.h"
#include "UsbPacketBuffer.h"
#include "Can.h"


extern "C" void can2_filter_init(uint32_t nr, bool scale_32bit,
		     bool id_list_mode, uint32_t fr1, uint32_t fr2,
		     uint32_t fifo, bool enable);

namespace Can {



struct HwConfig {
	can_t 		can;
};

struct Config {
	CanBaud		 baud;
	CanEcho      echo;
};

struct Module {
	HwConfig	hwConfig;
	Config		config;
	uint8_t  id;
};

Can::Module gCan1;
Can::Module gCan2;


Status hwInit( Module* const module ) {

	can_reset( module->hwConfig.can );
	can_disable_irq(module->hwConfig.can, 0xFF);

	bool loopback = ( CanEcho::Enabled == module->config.echo );

	uint32_t div = 120;

#if 0

	switch( module->config.baud ) {
		case CanBaud::Can_33Kbit: div = 120; break;
		case CanBaud::Can_100Kbit: div = 40; break;
		case CanBaud::Can_125Kbit: div = 32; break;
		case CanBaud::Can_250Kbit: div = 16; break;
		case CanBaud::Can_500Kbit: div = 8; break;
		case CanBaud::Can_1Mbit: div = 4; break;
		default: div = 8; break;
	}

#else

	const uint32_t OneMbit = 1000000;
	const uint32_t OneMbitDiv = 4;
	const uint32_t baud = static_cast<uint32_t>( module->config.baud );
	div = ( OneMbit / baud ) * OneMbitDiv;

#endif


	can_init( module->hwConfig.can,
			false,
			false,
			false,
			true,
			true,
			true,
			CAN_BTR_SJW_2TQ,
			CAN_BTR_TS1_6TQ,
			CAN_BTR_TS2_2TQ,
			div,
			loopback,
			false);

	if ( CAN1 == module->hwConfig.can ) {
		nvic_set_priority(19, 0x09);
		nvic_set_priority(20, 0x09);
		nvic_clear_pending_irq(19);
		nvic_clear_pending_irq(20);
		nvic_enable_irq(19);
		nvic_enable_irq(20);
		can_filter_init( 0, true, false,  0x00, 0x00, 0, true );
	} else if ( CAN2 == module->hwConfig.can ) {
		nvic_set_priority(64, 0x09);
		nvic_set_priority(65, 0x09);
		nvic_clear_pending_irq(64);
		nvic_clear_pending_irq(65);
		nvic_enable_irq(64);
		nvic_enable_irq(65);
		can_filter_init( 14, true, false,  0x00, 0x00, 0, true );
	}

	can_enable_irq(module->hwConfig.can, (1 << 1) | (1 << 4) );
	//can_enable_irq(module->hwConfig.can, 0xFF );

	return Status::Success;
}

Status init() {
	can1->hwConfig.can = CAN1;
	can1->id = 0;
	can1->config.baud = CanBaud::Can_500Kbit;
	can1->config.echo = CanEcho::Enabled;

	can2->hwConfig.can = CAN2;
	can2->id = 1;
	can2->config.baud = CanBaud::Can_100Kbit;
	can2->config.echo = CanEcho::Enabled;


	Status status = hwInit( can1 );
	if ( Status::Success != status ) return status;
	status = hwInit( can2 );
	return status;
}


Status process(  IndexType p, const UsbPacketId packetId ) {
	if ( InvalidIndex == p ) return Status::WrongArgument;
	uint8_t* const data = usbBufferGetDataPtr( PoolType::UsbToPeriph, p );


	if ( UsbPacketId::UsbDataToCan == packetId ) {
		UsbDataToCanHead* const head = reinterpret_cast<UsbDataToCanHead* const>(data);
		Module* const module = (can1->id == head->canId) ? can1 : can2;

		if ( WaitMode::WaitTimestamp == head->wait ) {
			const time_us_t ts = SystemTimer::getTimeUs();
			CircularIndex<time_us_t> gotTs(head->ts);
			CircularIndex<time_us_t> curTs(ts);
			if ( curTs < gotTs ) {
				return Status::Busy;
			}
		}

		int success = can_transmit( module->hwConfig.can,
					  	  head->frId,
						  head->ext,
						  head->rtr,
						  head->len,
						  head->data );

		if ( -1 != success ) {
			usbBufferCheckout( PoolType::UsbToPeriph, p );
			usbBufferFree( PoolType::UsbToPeriph, p );
		} else {
			return Status::Busy;
		}
	} else if ( UsbPacketId::UsbToCanBaud == packetId ) {
		UsbToCanBaudHead* const head = reinterpret_cast<UsbToCanBaudHead* const>(data);
		Module* const module = (can1->id == head->canId) ? can1 : can2;
		module->config.baud = head->baud;
		hwInit(module);
		usbBufferCheckout( PoolType::UsbToPeriph, p );
		usbBufferFree( PoolType::UsbToPeriph, p );
	} else if ( UsbPacketId::UsbToCanEcho == packetId ) {
		UsbToCanEchoHead* const head = reinterpret_cast<UsbToCanEchoHead* const>(data);
		Module* const module = (can1->id == head->canId) ? can1 : can2;
		module->config.echo = head->echo;
		hwInit(module);
		usbBufferCheckout( PoolType::UsbToPeriph, p );
		usbBufferFree( PoolType::UsbToPeriph, p );
	} else {
		usbBufferCheckout( PoolType::UsbToPeriph, p );
		usbBufferFree( PoolType::UsbToPeriph, p );
	}

	return Status::Success;
}

void canRxIrq( Can::Module* const module, const uint8_t fifoId ) {
	const time_us_t systemTs = SystemTimer::getTimeUs();
	auto p = usbBufferAlloc( PoolType::PeriphToUsb, sizeof(CanDataToUsbHead) );
	if (InvalidIndex != p ) {
		uint8_t* const data = usbBufferGetDataPtr( PoolType::PeriphToUsb, p);
		CanDataToUsbHead* const head = reinterpret_cast<CanDataToUsbHead* const>(data);
		head->id = UsbPacketId::CanDataToUsb;
		head->ts = systemTs;
		uint16_t ts;
		bool ext = false, rtr = false;
		can_receive( module->hwConfig.can,
					 fifoId,
					 true,
					 &head->frId,
					 &ext,
					 &rtr,
					 &head->fmi,
					 &head->len,
					 &head->data[0],
					 &ts );
		head->ext = ext;
		head->rtr = rtr;
		head->canId = module->id;
		usbBufferCommit(PoolType::PeriphToUsb, p);
	} else {
		uint8_t data[sizeof(CanDataToUsbHead)];
		CanDataToUsbHead* const head = reinterpret_cast<CanDataToUsbHead* const>(data);
		bool ext = false, rtr = false;
		uint16_t ts;
		can_receive( module->hwConfig.can,
					 fifoId,
					 true,
					 &head->frId,
					 &ext,
					 &rtr,
					 &head->fmi,
					 &head->len,
					 &head->data[0],
					 &ts );
	}
}


}

Can::Module* const can1 = &Can::gCan1;
Can::Module* const can2 = &Can::gCan2;


extern "C" void Can1Tx_IRQHandler(void) {
	nvic_clear_pending_irq(18);
}

extern "C" void Can1Rx0_IRQHandler(void) {
	nvic_clear_pending_irq(19);
	canRxIrq(can1, 0);
}

extern "C" void Can1Rx1_IRQHandler(void) {
	nvic_clear_pending_irq(20);
	canRxIrq(can1, 1);
}

extern "C" void Can1Sce_IRQHandler(void) {
	nvic_clear_pending_irq(21);
}

extern "C" void Can2Tx_IRQHandler(void) {
	nvic_clear_pending_irq(63);
}

extern "C" void Can2Rx0_IRQHandler(void) {
	nvic_clear_pending_irq(64);
	canRxIrq(can2, 0);
}

extern "C" void Can2Rx1_IRQHandler(void) {
	nvic_clear_pending_irq(65);
	canRxIrq(can2, 1);
}

extern "C" void Can2Sce_IRQHandler(void) {
	nvic_clear_pending_irq(66);
}
