
#include "../App/Debug.h"
#include "../App/memorymap.h"
#include "libopencm3/cm3/assert.h"
#include "libopencm3/stm32/f1/rcc.h"
#include "libopencm3/stm32/gpio.h"

// Called to initialize peripheral blocks
extern "C" void systemInit(void) {
	// Enable debugging
	DHCSR = DHCSR_DBGKEY | DHCSR_C_DEBUGEN;
	// Enable debugging
	DEMCR = DEMCR_MON_EN;
}

enum class PowerMode {
	Active,
	Sleep,
	Idle
};


static inline void disableInterupts(){
	asm("cpsid i");
}

static inline void enableInterrups(){
	asm("cpsie i");
}


static void setActiveMode( void ) {
	// Disable interrupts
	disableInterupts();
	// Confiure clock system
	rcc_clock_setup_pll( &rcc_hse_configs[RCC_CLOCK_HSE16_72MHZ] );
	// Enable LED GPIO port
	rcc_periph_clock_enable( RCC_GPIOC );
	// Enable CAN1, CAN2, LIN/USART1 GPIO port
	rcc_periph_clock_enable( RCC_GPIOB );
	// Enable alternate function
	rcc_periph_clock_enable( RCC_AFIO );
	// Enable CAN1
	rcc_periph_clock_enable( RCC_CAN1 );
	// Enable CAN2
	rcc_periph_clock_enable( RCC_CAN2 );
	// USART1
	rcc_periph_clock_enable( RCC_USART1 );
	// USB
	rcc_periph_clock_enable( RCC_OTGFS );

	// Reconfigure GPIOC mode
	gpio_set_mode( GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO6 | GPIO7);

	// Enable USB GPIO
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO11);
	gpio_set_mode(GPIOA, GPIO_MODE_OUTPUT_50_MHZ, GPIO_CNF_OUTPUT_ALTFN_PUSHPULL, GPIO12);

	// Enable interrupts
	enableInterrups();
}

static void setSleepMode( void ) {

}

static void setIdleMode( void ) {
}


extern "C" void setPowerMode( PowerMode newPowerMode ) {
	static PowerMode currentPowerMode = PowerMode::Sleep;

	if ( currentPowerMode != newPowerMode ) {
		switch (newPowerMode) {
		case PowerMode::Active: setActiveMode(); break;
		case PowerMode::Sleep: setSleepMode(); break;
		case PowerMode::Idle: setIdleMode(); break;
		default: break;
		}
		currentPowerMode = newPowerMode;
	}

}


extern "C" void wakeUp(void) {
	rcc_clock_setup_pll( &rcc_hse_configs[RCC_CLOCK_HSE16_72MHZ] );

	rcc_periph_clock_enable( RCC_GPIOC );
	rcc_periph_reset_pulse( RST_GPIOC );

	rcc_periph_clock_enable(RCC_OTGFS);



	gpio_set_mode( GPIOC, GPIO_MODE_OUTPUT_2_MHZ, GPIO_CNF_OUTPUT_PUSHPULL, GPIO6 | GPIO7);

}

extern "C" void DebugHandler(ArmRegisters* regs){
	static volatile bool exit_disable = true;
	//gpio_set(GPIOC, GPIO7);
	while( exit_disable ) {
		asm("nop");
	}
}

// Default code
extern "C" void systemMain(void) {
	systemInit();
	wakeUp();
	//gpio_set(GPIOC, GPIO7);
	//gpio_set(GPIOC, GPIO6);
	//powerDown();

	rcc_periph_clock_enable( RCC_GPIOA );
	gpio_set_mode( GPIOA, GPIO_MODE_INPUT, GPIO_CNF_INPUT_PULL_UPDOWN, GPIO3 );

	int b = 0;
	int c = 0;

	while(true) {


	}

	while(true) {
		asm("bkpt");
	}
}
