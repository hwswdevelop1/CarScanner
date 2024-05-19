

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

void* memcpy( void* dst , const void* src, size_t n ) {
	uint8_t* s = (uint8_t*)(src);
	uint8_t* d = reinterpret_cast<uint8_t*>(dst);
	for( size_t index = 0; index < n; index++ ) {
		d[index] = s[index]; 
	}
	return d;
}

#ifdef __cplusplus
}
#endif

