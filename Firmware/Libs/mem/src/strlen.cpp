/*
 * strlen.cpp
 *
 *  Created on: May 1, 2024
 *      Author: Developer
 */
#include <stdint.h>
#include <stddef.h>

extern "C" unsigned int strlen( const char* buf ) {
	if (nullptr == buf) return 0;
	size_t len = 0;
	while( 0 != *buf++ ) len++;
	return len;
}

