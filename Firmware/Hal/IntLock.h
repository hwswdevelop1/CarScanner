/*
 * IntLock.h
 *
 *  Created on: May 12, 2024
 *      Author: Developer
 */

#pragma once

#include "Types.h"

#ifdef __cplusplus
extern "C" {
#endif


extern "C" int_lock_t intLock();
extern "C" void intUnlock( int_lock_t prevLockState );

struct IntLock {
	IntLock() {
		_lock = intLock();
	}
	~IntLock() {
		intUnlock(_lock);
	}
private:
	int_lock_t _lock;
};


#ifdef __cplusplus
}
#else

typedef int IntLockState;
IntLockState intLock();
void intUnlock( IntLockState prevLockState );

#endif


