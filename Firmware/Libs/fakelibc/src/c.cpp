
#include <stddef.h>
#include <stdint.h>

extern "C" void __libc_init_array(void) {
}

extern "C" void exit(int){
	volatile bool debugLoop = true;
	while( debugLoop ){
		asm("bkpt");
	}
}

