/*
 * UsbPacketBuffer.cpp
 *
 *  Created on: May 16, 2024
 *      Author: Developer
 */


#include "UsbPacketBuffer.h"
#include "IntLock.h"


static CircularPacketBuffer<4 * 1024, 256>   usbToPeriph;
static CircularPacketBuffer<4 * 1024, 128>   can1ToUsb;
static CircularPacketBuffer<4 * 1024, 128>   can2ToUsb;
static CircularPacketBuffer<256, 16>  		 linToUsb;

void usbSendPacket();
void usbSetAck();


IndexType usbBufferAlloc( const PoolType pool, const size_t size ) {

	IndexType  id = InvalidIndex;

	switch(pool){
	case PoolType::UsbToPeriph:
	{
		int_lock_t key = intLock();
		id =  usbToPeriph.alloc(size);
		intUnlock(key);
	}
	break;

	case PoolType::LinToUsb:
	{
		usbSendPacket();
		int_lock_t key = intLock();
		id = linToUsb.alloc(size);
		intUnlock(key);
	}
	break;

	case PoolType::Can1ToUsb:
	{
		usbSendPacket();
		int_lock_t key = intLock();
		id = can1ToUsb.alloc(size);
		intUnlock(key);
	}
	break;

	case PoolType::Can2ToUsb:
	{
		usbSendPacket();
		int_lock_t key = intLock();
		id = can2ToUsb.alloc(size);
		intUnlock(key);
	}
	break;

	default: break;
	}

	return id;
}

IndexType usbBufferGet( const PoolType pool ) {
	int id = InvalidIndex;

	int_lock_t key = intLock();
	switch(pool){

	case PoolType::UsbToPeriph:
		id =  usbToPeriph.get();
	break;

	case PoolType::LinToUsb:
		id = linToUsb.get();
	break;

	case PoolType::Can1ToUsb:
		id = can1ToUsb.get();
	break;

	case PoolType::Can2ToUsb:
		id = can2ToUsb.get();
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

	case PoolType::LinToUsb:
		linToUsb.checkout(id);
	break;

	case PoolType::Can1ToUsb:
		can1ToUsb.checkout(id);
	break;

	case PoolType::Can2ToUsb:
		can2ToUsb.checkout(id);
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
		const size_t count = usbBufferFreeElementCount( pool );
		if ( 1 == count ) {
			usbSetAck();
		}
	}
	break;

	case PoolType::LinToUsb:
		linToUsb.free(id);
	break;

	case PoolType::Can1ToUsb:
		can1ToUsb.free(id);
	break;

	case PoolType::Can2ToUsb:
		can2ToUsb.free(id);
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

	case PoolType::LinToUsb:
	{
		int_lock_t key = intLock();
		linToUsb.commit(id);
		intUnlock(key);
		usbSendPacket();
	}
	break;

	case PoolType::Can1ToUsb:
	{
		int_lock_t key = intLock();
		can1ToUsb.commit(id);
		intUnlock(key);
		usbSendPacket();
	}
	break;

	case PoolType::Can2ToUsb:
	{
		int_lock_t key = intLock();
		can2ToUsb.commit(id);
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

	case PoolType::LinToUsb:
		data = linToUsb.data(id);
	break;

	case PoolType::Can1ToUsb:
		data = can1ToUsb.data(id);
	break;

	case PoolType::Can2ToUsb:
		data = can2ToUsb.data(id);
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

	case PoolType::LinToUsb:
		dataSize = linToUsb.size(id);
	break;

	case PoolType::Can1ToUsb:
		dataSize = can1ToUsb.size(id);
	break;

	case PoolType::Can2ToUsb:
		dataSize = can2ToUsb.size(id);
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

	case PoolType::LinToUsb:
		linToUsb.trunc(id, size);
	break;

	case PoolType::Can1ToUsb:
		can1ToUsb.trunc(id, size);
	break;

	case PoolType::Can2ToUsb:
		can2ToUsb.trunc(id, size);
	break;

	default: break;
	}

	intUnlock(key);

}

size_t usbBufferElementCount( const PoolType pool ){

	int_lock_t key = intLock();

	size_t  count = 0;
	switch(pool){

	case PoolType::UsbToPeriph:
		count = usbToPeriph.packetCount();
	break;

	case PoolType::LinToUsb:
		count = linToUsb.packetCount();
	break;

	case PoolType::Can1ToUsb:
		count = can1ToUsb.packetCount();
	break;

	case PoolType::Can2ToUsb:
		count = can2ToUsb.packetCount();
	break;

	default: break;
	}

	intUnlock(key);

	return count;
}


size_t usbBufferFreeElementCount( const PoolType pool ){

	int_lock_t key = intLock();

	size_t  count = 0;
	switch(pool){

	case PoolType::UsbToPeriph:
		count = usbToPeriph.emptyCount();
	break;

	case PoolType::LinToUsb:
		count = linToUsb.emptyCount();
	break;

	case PoolType::Can1ToUsb:
		count = can1ToUsb.emptyCount();
	break;

	case PoolType::Can2ToUsb:
		count = can2ToUsb.emptyCount();
	break;

	default: break;
	}

	intUnlock(key);

	return count;
}


