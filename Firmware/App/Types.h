/*
 * Types.h
 *
 *  Created on: May 12, 2024
 *      Author: Evgeny Sobolev. 02/09/1984y.b
 */

#pragma once

#include "stdint.h"
#include "stddef.h"

typedef uint32_t 	usart_t;
typedef uint32_t 	timer_t;
typedef uint32_t	can_t;
typedef uint32_t 	irq_t;
typedef uint32_t	gpio_port_t;
typedef uint16_t	gpio_pin_t;

typedef struct _gpio_t {
	gpio_port_t port;
	gpio_pin_t pin;
} gpio_t;

typedef uint32_t	time_ms_t;
typedef uint32_t	time_us_t;
typedef uint32_t	frame_size_t;
typedef uint32_t	int_lock_t;
typedef uint32_t	baud_t;


typedef struct _buffer_t {
	uint8_t*		ptr;
	size_t			size;
} buffer_t;



enum class Status : uint8_t {
	Success = 0x00,
	Pending,
	Busy,
	NoMemory,
	Error = 0x80,
	WrongArgument,
};

typedef void (*onReqCmplt)( struct irp*, Status );

typedef struct _request {
	buffer_t 	data;
	onReqCmplt	complete;
} request_t;

