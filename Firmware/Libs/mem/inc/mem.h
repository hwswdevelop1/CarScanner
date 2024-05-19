/*
 * mem.h
 *
 *  Created on: 7 мар. 2024 г.
 *      Author: Evgeny
 */


#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>

void* malloc(size_t bytes);
void free (void *mem);
void* memset( void *s , int value, size_t n );
void* memcpy( void *dst , const void* src, size_t n );

#ifdef __cplusplus
}
#endif

