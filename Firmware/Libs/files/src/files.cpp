
#include <stddef.h>
#include <stdint.h>

extern "C" {

	int __errno;

	void abort(void) { 
		while(true) { 
			asm("nop");
		};
	};

	void* fopen(char file_name[], char mode[]){
		return nullptr;
	};

	int fprintf(void *file, const char *format, ...) {
		return 0;
	};

	unsigned int fwrite(const void *ptr, unsigned int size, unsigned int nmemb, void *file){
		return 0;
	};

	int fread(void *ptr, int size, int nmemb, void *file){
		return 0;
	};

} // extern "C" {