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
	uint32_t     baud;
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

	if ( 100000 == module->config.baud ){
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
				40,
				false,
				false);

		//nvic_enable_irq(63);
		nvic_enable_irq(64);
		nvic_enable_irq(65);
		//nvic_enable_irq(66);


	} else {
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
				8,
				false,
				false);

		//nvic_enable_irq(18);
		nvic_enable_irq(19);
		nvic_enable_irq(20);
		//nvic_enable_irq(21);

	}

	can_filter_init( 0, true, false,  0x00, 0x00, 0, true );
	can_filter_init( 14, true, false,  0x00, 0x00, 0, true );

	can_enable_irq(module->hwConfig.can, (1 << 1) | (1 << 4) );

	return Status::Success;
}

Status init() {
	can1->hwConfig.can = CAN1;
	can1->id = 0;
	can1->config.baud = 500000;

	can2->hwConfig.can = CAN2;
	can2->config.baud = 100000;
	can2->id = 1;

	Status status = hwInit( can1 );
	if ( Status::Success != status ) return status;
	status = hwInit( can2 );
	return status;
}


struct UsbToCanBuffer {
	CanDataToUsbHead head;

};


Status process( Module* const module, IndexType p, const UsbPacketId packetId ) {
	if ( nullptr == module ) return Status::WrongArgument;
	if ( InvalidIndex == p ) return Status::WrongArgument;


	uint8_t* const data = usbBufferGetDataPtr( PoolType::UsbToPeriph, p );

	if ( UsbPacketId::UsbDataToCan == packetId ) {
		UsbDataToCanHead* const head = reinterpret_cast<UsbDataToCanHead* const>(data);
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
		}
	}

	return Status::Success;
}

void canRxIrq( Can::Module* const module, const uint8_t fifoId ) {
	auto p = usbBufferAlloc( PoolType::PeriphToUsb, sizeof(CanDataToUsbHead) );
	if (InvalidIndex != p ) {
		uint8_t* const data = usbBufferGetDataPtr( PoolType::PeriphToUsb, p);
		CanDataToUsbHead* const head = reinterpret_cast<CanDataToUsbHead* const>(data);
		head->id = UsbPacketId::CanDataToUsb;
		head->ts = SystemTimer::getTimeUs();
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
