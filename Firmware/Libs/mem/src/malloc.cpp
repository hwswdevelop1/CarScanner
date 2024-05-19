
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

extern void* __HeapStart;
extern void* __HeapEnd;


__attribute__((weak)) void __heap_alloc_cb( size_t req, size_t total, size_t used ) {
}


/*
 * Don't use libc for now. Use malloc for now, and replace it later by libc
 * Worry: this malloc function doesn't support free of allocated memory
*/
void* malloc(size_t bytes) {
	static uint8_t* heap_ptr = reinterpret_cast<uint8_t*>(&__HeapStart);
	uint8_t* heap_start_ptr = reinterpret_cast<uint8_t*>(&__HeapStart);
	uint8_t* heap_end_ptr   = reinterpret_cast<uint8_t*>(&__HeapEnd);
	uint8_t* ret_ptr = heap_ptr;
	
	__heap_alloc_cb(
		bytes,
		(heap_end_ptr - heap_start_ptr),
		(heap_ptr - heap_start_ptr)
	);
	
	if ( heap_start_ptr >= heap_end_ptr ) return nullptr; 
	if ( heap_ptr >= heap_end_ptr ) return nullptr;
	if ( (heap_ptr + bytes) > heap_end_ptr ) return nullptr;
	heap_ptr += bytes;
	return (void*)(ret_ptr);
}

/*
* Worry: this free function doesn't support free of allocated memory
*/
void free(void *mem) {
	asm("bkpt");
	while(true) {
		asm("nop");
	}
}

#ifdef __cplusplus
}
#endif

