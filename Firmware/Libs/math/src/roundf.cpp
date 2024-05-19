
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "libm.h"
#include "libm_int.h"

float roundf(float x)
{
	float t;

	if (!isfinitef(x))
		return (x);

	if (x >= 0.0) {
		t = floorf(x);
		if (t - x <= -0.5)
			t += 1.0;
		return (t);
	} else {
		t = floorf(-x);
		if (t + x <= -0.5)
			t += 1.0;
		return (-t);
	}
}

#ifdef __cplusplus
}
#endif
