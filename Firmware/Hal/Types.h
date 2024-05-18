/*
 * Types.h
 *
 *  Created on: May 12, 2024
 *      Author: Developer
 */

#pragma once

#include "stdint.h"
#include "stdio.h"

typedef uint32_t 	usart_t;
typedef uint32_t 	timer_t;
typedef uint32_t 	irq_t;
typedef uint32_t	baud_t;
typedef uint32_t	time_ms_t;
typedef uint32_t	int_lock_t;

typedef uint32_t	gpio_port_t;
typedef uint16_t	gpio_pin_t;

typedef struct {
	gpio_port_t port;
	gpio_pin_t pin;
} gpio_t;

