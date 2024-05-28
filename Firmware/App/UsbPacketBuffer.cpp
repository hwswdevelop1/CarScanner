/*
 * UsbPacketBuffer.cpp
 *
 *  Created on: May 16, 2024
 *      Author: Evgeny Sobolev. 02/09/1984y.b
 */


#include "UsbPacketBuffer.h"
#include "IntLock.h"

static CircularPacketBuffer<4 * 1024, 256>   usbToPeriph;
static CircularPacketBuffer<4 * 1024, 256>   periphToUsb;

void usbSendPacket();
void usbSetAck();


IndexType usbBufferAlloc( const PoolType pool, const size_t size, bool allocPossibleCheck  ) {

	IndexType  id = InvalidIndex;

	switch(pool){
	case PoolType::UsbToPeriph:
	{
		int_lock_t key = intLock();
		id =  usbToPeriph.alloc(size, allocPossibleCheck);
		intUnlock(key);
	}
	break;

	case PoolType::PeriphToUsb:
	{
		usbSendPacket();
		int_lock_t key = intLock();
		id = periphToUsb.alloc(size, allocPossibleCheck);
		intUnlock(key);
	}
	break;

	default: break;
	}

	return id;
}

IndexType usbBufferGet( const PoolType pool ) {
	IndexType id = InvalidIndex;

	int_lock_t key = intLock();
	switch(pool){

	case PoolType::UsbToPeriph:
	{
		id = usbToPeriph.get();
		if ( InvalidIndex == id ) {
			usbSetAck();
		}
	}

	break;

	case PoolType::PeriphToUsb:
		id = periphToUsb.get();
	break;

	default: break;
	}

	intUnlock(key);
	return id;
}

void usbBufferCheckout( const PoolType pool, IndexType id ) {

	int_lock_t key = intLock();

	switch(pool){

	case PoolType::UsbToPeriph:
		usbToPeriph.checkout(id);
	break;

	case PoolType::PeriphToUsb:
		periphToUsb.checkout(id);
	break;

	default: break;
	}

	intUnlock(key);
}

void usbBufferFree( const PoolType pool, IndexType id ) {

	int_lock_t key = intLock();

	switch(pool){

	case PoolType::UsbToPeriph:
	{
		usbToPeriph.free(id);
		if ( InvalidIndex != usbToPeriph.alloc( 64, true ) ) {
			usbSetAck();
		}
	}
	break;

	case PoolType::PeriphToUsb:
		periphToUsb.free(id);
	break;


	default: break;
	}

	intUnlock(key);

}

void usbBufferCommit( const PoolType pool, IndexType id ) {

	switch(pool){

	case PoolType::UsbToPeriph:
	{
		int_lock_t key = intLock();
		usbToPeriph.commit(id);
		intUnlock(key);
	}
	break;

	case PoolType::PeriphToUsb:
	{
		int_lock_t key = intLock();
		periphToUsb.commit(id);
		intUnlock(key);
		usbSendPacket();
	}
	break;

	default: break;
	}

}

uint8_t* usbBufferGetDataPtr( const PoolType pool, IndexType id ) {

	int_lock_t key = intLock();

	uint8_t* data = nullptr;
	switch(pool){

	case PoolType::UsbToPeriph:
		data = usbToPeriph.data(id);
	break;

	case PoolType::PeriphToUsb:
		data = periphToUsb.data(id);
	break;


	default: break;
	}

	intUnlock(key);

	return data;

}

size_t usbBufferGetSize( const PoolType pool, IndexType id ) {

	int_lock_t key = intLock();

	size_t  dataSize = 0;
	switch(pool){

	case PoolType::UsbToPeriph:
		dataSize = usbToPeriph.size(id);
	break;

	case PoolType::PeriphToUsb:
		dataSize = periphToUsb.size(id);
	break;

	default: break;
	}

	intUnlock(key);

	return dataSize;

}

void usbBufferUpdateSize( const PoolType pool, IndexType id, const size_t size ) {

	int_lock_t key = intLock();

	switch(pool){

	case PoolType::UsbToPeriph:
		usbToPeriph.trunc(id, size);
	break;

	case PoolType::PeriphToUsb:
		periphToUsb.trunc(id, size);
	break;


	default: break;
	}

	intUnlock(key);

}



