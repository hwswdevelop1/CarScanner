
#include <stddef.h>
#include <stdint.h>



void exit(int){
	volatile bool debugLoop = true;
	while( debugLoop ){
		asm("bkpt");
	}
}

extern "C" int __aeabi_atexit( void *arg, void (*func)(void*), void* d) {
	volatile bool debugLoop = true;
	while( debugLoop ){
		asm("bkpt");
	}
	return 0;
}

extern "C" void __cxa_pure_virtual() {
	volatile bool debugLoop = true;
	while( debugLoop ){
		asm("bkpt");
	}
}

