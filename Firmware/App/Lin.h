
#pragma once

#include "Types.h"
#include "UsbPacketBuffer.h"

namespace Lin {

	struct HwConfig {
		usart_t 	usart;
		usart_t		timer;
		gpio_t		lin_en;
		irq_t		usart_irq;
		irq_t		timer_irq;
	};

	enum class RxEnable {
		Disabled,
		Enabled
	};

	struct Module;

	struct RxConfig {
		time_us_t 	timeout;
		uint32_t	size;
	};

	struct Config {
		baud_t		baud;
		RxConfig	rx;
		//TxConfig	tx;
	};


	enum class FrameType {
		DataWithBreak,
		Data
	};

	Status init( Module* const module ) ;
	Status process( Module* const module, const IndexType p, const UsbPacketId packetId );

}

extern Lin::Module* lin0;

