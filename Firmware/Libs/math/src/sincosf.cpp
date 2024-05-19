
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "libm.h"
#include "libm_int.h"


static const float
one =  1.0,
C0  = -0x1ffffffd0c5e81.0p-54,	/* -0.499999997251031003120 */
C1  =  0x155553e1053a42.0p-57,	/*  0.0416666233237390631894 */
C2  = -0x16c087e80f1e27.0p-62,	/* -0.00138867637746099294692 */
C3  =  0x199342e0ee5069.0p-68,	/*  0.0000243904487962774090654 */
S1 = -0x15555554cbac77.0p-55,	/* -0.166666666416265235595 */
S2 =  0x111110896efbb2.0p-59,	/*  0.0083333293858894631756 */
S3 = -0x1a00f9e2cae774.0p-65,	/* -0.000198393348360966317347 */
S4 =  0x16cd878c3b46a7.0p-71,	/*  0.0000027183114939898219064 */
pi  =  3.14159259,
twopi = pi * 2,
halfpi = pi /2;



/* Teilor sin function aproximation */

float sinf_int(float x)
{
	float r, s, w, z;
	/* Try to optimize for parallel evaluation as in k_tanf.c. */
	z = x*x;
	w = z*z;
	r = S3+z*S4;
	s = z*x;
	return (x + s*(S1+z*S2)) + s*w*r;
}

/* Teilor sin function aproximation */
float cosf_int(float x)
{
	float r, w, z;
	/* Try to optimize for parallel evaluation as in k_tanf.c. */
	z = x*x;
	w = z*z;
	r = C2+z*C3;
	return ((one+z*C0) + w*C1) + (w*z)*r;
}


float cosf(float x) {
	float int_x = (x >= 0.0) ? (x) : (-x);
	
	if ( int_x > (2.0 * pi) ) {
		float delta = floorf( int_x / (2.0 * pi) ) * (2.0 * pi);
		int_x = int_x - delta;
	}	
	if ( int_x < ( pi / 2.0 )  ) return cosf_int( int_x );	
	if ( int_x <= pi ) return  -cosf_int( pi - int_x );
	return -cosf( int_x - pi );
}

float sinf(float x) {
	return cosf( x - ( pi/2.0 ) );
}


#ifdef __cplusplus
}
#endif

