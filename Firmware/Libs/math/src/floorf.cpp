
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "libm.h"
#include "libm_int.h"

/*
 * floorf(x)
 * Return x rounded toward -inf to integral value
 * Method:
 *	Bit twiddling.
 * Exception:
 *	Inexact flag raised if x not equal to floorf(x).
 */
static const float huge = 1.0e30;

float floorf(float x)
{
	int32_t   i0,j0;
	uint32_t  i;
	GET_FLOAT_WORD(i0,x);
	j0 = ((i0>>23)&0xff)-0x7f;
	if(j0<23) {
	    if(j0<0) { 	/* raise inexact if x != 0 */
		if(huge+x>(float)0.0) {/* return 0*sign(x) if |x|<1 */
		    if(i0>=0) {i0=0;}
		    else if((i0&0x7fffffff)!=0)
			{ i0=0xbf800000;}
		}
	    } else {
		i = (0x007fffff)>>j0;
		if((i0&i)==0) return x; /* x is integral */
		if(huge+x>(float)0.0) {	/* raise inexact flag */
		    if(i0<0) i0 += (0x00800000)>>j0;
		    i0 &= (~i);
		}
	    }
	} else {
	    if(j0==0x80) return x+x;	/* inf or NaN */
	    else return x;		/* x is integral */
	}
	SET_FLOAT_WORD(x,i0);
	return x;
}

#ifdef __cplusplus
}
#endif
