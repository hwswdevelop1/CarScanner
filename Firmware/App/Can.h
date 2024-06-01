/*
 * Can.h
 *
 *  Created on: May 20, 2024
 *      Author: Evgeny Sobolev. 02/09/1984y.b
 */


#pragma once

#include "Types.h"
#include "UsbPacketBuffer.h"


namespace Can {

struct Module;

Status init();
Status process( const IndexType p, const UsbPacketId packetId );

}

extern Can::Module* const can1;
extern Can::Module* const can2;

