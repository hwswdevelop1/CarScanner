
#pragma once

#include "Types.h"
#include "UsbPacketBuffer.h"

namespace Lin {

	enum class Status {
		Success,
		Pending,
		Error,
		Busy
	};

	struct HwConfig {
		usart_t 	usart;
		usart_t		timer;
		gpio_t		lin_en;
		irq_t		usart_irq;
		irq_t		timer_irq;
	};

	struct Config {
		baud_t		baud;
		uint8_t     maxRxSize;
		uint16_t	rxTimeout;
	};


	enum class LinFrameType : uint8_t {
		LinBreak,
		LinNoBreak,
		LinSetBaud,
		LinSetTimeout,
		LinSetAutoAnswer
	};



	enum class ModuleId : uint8_t {
		Lin1,
		Can1,
		Can2
	};

	struct UsbPacketHead {
		ModuleId 	 id;
		union ModuleSpecific {
			LinFrameType	lin;
		} type;
	};


	struct Module;

	Status hwInit( Module* const module, const HwConfig* const config );
	Status config( Module* const module, const Config* const config );
	Status process( Module* const module, IndexType p );
	// void startTimer( Module* const module );

}

extern Lin::Module* lin0;

