#pragma once

/* uncomment the next line if you're on a big-endian system */
/* #define _OBH_BIG_ENDIAN */

/* uncomment the next line if you can not assume double-precision FPU or IEEE754
 */
/* #define NO_FAST_TRUNCATE */

/* we need to do some custom hacking for MSVC */

#include <emmintrin.h>
#include <math.h>
#include <stddef.h>
#include <stdio.h>
#ifdef _MSC_VER
typedef __int32 int32_t;
#else
#include <stdint.h>
#endif

// #define NO_FAST_TRUNCATE

// with this hacks in fast_round fast_sin work faster then native sinf
inline int32_t fast_round(double x)
{

#ifndef NO_FAST_TRUNCATE

    const double MAGIC_ROUND =
        6755399441055744.0; /* http://stereopsis.com/sree/fpu2006.html */

    // yeah undefined behaviors hacks thats significanlty speed up code
    union
    {
        double d;

        struct
        {
#ifndef _OBH_BIG_ENDIAN
            int32_t lw;
            int32_t hw;
#elif
            int32_t hw;
            int32_t lw;
#endif
        };
    } fast_trunc;

    fast_trunc.d = x;
    fast_trunc.d += MAGIC_ROUND;

    return fast_trunc.lw;

#else

    if (x < 0)
    {
        return (int32_t)(x - 0.5);
    }
    else
    {
        return (int32_t)(x + 0.5);
    }

#endif
}

inline double
fast_sin(double x)
{
    const double PI = 3.14159265358979323846264338327950288;
    const double INVPI = 0.31830988618379067153776752674502872;
    const double A = 0.00735246819687011731341356165096815;
    const double B = -0.16528911397014738207016302002888890;
    const double C = 0.99969198629596757779830113868360584;

    /* find offset of x from the range -pi/2 to pi/2 */
    int32_t k = fast_round(INVPI * x);

    /* bring x into range */
    x -= k * PI;

    /* calculate sine */
    double x2 = x * x;
    x = x * (C + x2 * (B + A * x2));

    /* if x is in an odd pi count we must flip */
    return k & 1 ? -x : x;
}

inline double fast_cos(double x) { 
    // `fast_sin` magic faster on ~40% then `cosf`
    return fast_sin(1.57079632 - x);
}