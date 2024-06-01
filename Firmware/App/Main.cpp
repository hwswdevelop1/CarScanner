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
#include <libopencm3/stm32/can.h>
#include <libopencm3/usb/usbd.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/scb.h>
#include <libopencm3/cm3/mpu.h>
#include <libopencm3/stm32/usart.h>
#include "Memory.h"
#include "Usb.h"
#include "Lin.h"
#include "Can.h"


#include "SystemTimer.h"

#include "usb_common.h"
#include "UsbPacketBuffer.h"

extern "C" void systemMain() {
	SystemTimer::init();
	usbdInit();

	Lin::init( lin0 );
	Can::init();

	while( true ) {

		auto p = usbBufferGet( PoolType::UsbToPeriph );
		if ( InvalidIndex != p ) {

			// So, if I got packet, process it
			uint8_t* const data = usbBufferGetDataPtr( PoolType::UsbToPeriph, p );
			const size_t size = usbBufferGetSize( PoolType::UsbToPeriph, p );

			// Check if, buffer size is lower then UsbPacketId
			if ( size < sizeof(UsbPacketId) ) {
				usbBufferCheckout( PoolType::UsbToPeriph, p );
				usbBufferFree( PoolType::UsbToPeriph, p );
				continue;
			}

			// Check usb packet head size
			const UsbPacketId* const usbPacketId = reinterpret_cast<const UsbPacketId* const>(data);
			const size_t headSize = usbPacketHeadSize(*usbPacketId);

			// Check packet type
			if ( ( 0 == headSize ) || ( size < headSize ) ) {
				usbBufferCheckout( PoolType::UsbToPeriph, p );
				usbBufferFree( PoolType::UsbToPeriph, p );
				continue;
			}

			// Select packet type
			switch( *usbPacketId ) {

			case UsbPacketId::UsbToTimer:
			{
				SystemTimer::process(p);
			}
			break;

			// Lin data packet
			case UsbPacketId::UsbToLinData:
			case UsbPacketId::UsbToLinRxOnOff:
			case UsbPacketId::UsbToLinTimeout:
			case UsbPacketId::UsbToLinBaud:
			case UsbPacketId::UsbToLinAnswer:
			case UsbPacketId::UsbToLinRxSize:
			{
				Lin::process( lin0, p,  *usbPacketId );
			}
			break;

			case UsbPacketId::UsbDataToCan:
			case UsbPacketId::UsbToCanBaud:
			case UsbPacketId::UsbToCanEcho:
			{
				UsbDataToCanHead* const head = reinterpret_cast<UsbDataToCanHead* const>(data);
				Can::process( p, *usbPacketId );
			}
			break;

			// Any other packets
			default:
			{
				// Clear unused packets
				usbBufferCheckout( PoolType::UsbToPeriph, p );
				usbBufferFree( PoolType::UsbToPeriph, p );
			}
			break;
			}

		}
	}

}

