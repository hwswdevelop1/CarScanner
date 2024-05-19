
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "libm.h"
#include "libm_int.h"

float fabsf(float in) {
	return __builtin_fabsf(in);
}

#ifdef __cplusplus
}
#endif
