/*
 * new.cpp
 *
 *  Created on: 19 мар. 2024 г.
 *      Author: Evgeny
 */

#include <stddef.h>
#include <stdint.h>

#include "mem.h"

void* operator new( unsigned int size ) {
	void* res = malloc(size);
	return res;
}

void operator delete( void* ptr, unsigned int size ) {
	free(ptr);
}


