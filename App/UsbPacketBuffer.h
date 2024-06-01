
#pragma once


#include "Types.h"
#include "CircularPacketBuffer.h"
#include "Usb.h"
#include "UsbPackets.h"

enum class PoolType : uint8_t {
	UsbToPeriph,
	PeriphToUsb
};

IndexType usbBufferAlloc( const PoolType pool, const size_t size, bool allocPossibleCheck = false );
IndexType usbBufferGet( const PoolType pool );
void usbBufferCheckout( const PoolType pool, IndexType id );
void usbBufferFree( const PoolType pool, IndexType id );
void usbBufferCommit( const PoolType pool, IndexType id );
uint8_t* usbBufferGetDataPtr( const PoolType pool, IndexType id );
size_t usbBufferGetSize( const PoolType pool, IndexType id );
void usbBufferUpdateSize( const PoolType pool, IndexType id, const size_t size );

