/*
 * Time.h
 *
 *  Created on: May 24, 2024
 *      Author: Evgeny Sobolev. 02/09/1984y.b
 */
#pragma once

#include "Types.h"
#include "UsbPacketBuffer.h"

namespace SystemTimer {

Status init();
time_us_t getTimeUs();
void wait( time_us_t delay );
Status process( const IndexType p );

};


