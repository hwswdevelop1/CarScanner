
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

	extern int __errno;
	void abort(void);
	void* fopen(char file_name[], char mode[]);
	int fprintf(void *file, const char *format, ...);
	unsigned int fwrite(const void *ptr, unsigned int size, unsigned int nmemb, void *file);
	int fread(void *ptr, int size, int nmemb, void *file);

#ifdef __cplusplus
}
#endif
