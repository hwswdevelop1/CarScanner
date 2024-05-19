

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

void* memset( void *s , int value, size_t n ) {
	uint8_t* ptr = reinterpret_cast<uint8_t*>(s);
	size_t index;
	for(index = 0; index < n; index++){
		ptr[index] = static_cast<uint8_t>(value);
	}
	return s;
}

#ifdef __cplusplus
}
#endif
