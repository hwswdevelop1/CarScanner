
#pragma once

/*
 * A union which permits us to convert between a float and a 32 bit
 * int.
 */
#ifdef __cplusplus
extern "C" {
#endif
 
#include <stdint.h>
#include <stddef.h>

float powf(float x, float y);
float log10f(float in);
float log1pf(float in);
float floorf(float in);
float roundf(float in);
float sqrtf(float in);
float fabsf(float in);
float cosf(float in);
float sinf(float in);
float atanf(float in);
float atan2f(float x, float y);
float acosf(float x);
float scalbnf(float x, int n);
int isfinitef(float f);

#ifdef __cplusplus
}
#endif
