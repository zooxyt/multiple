/*
   Mini Math Library
   Copyright (c) 2013-2014 Cheryl Natsu 
   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:
   1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.
   2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
   3. The name of the authors may not be used to endorse or promote products
   derived from this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ''AS IS'' AND ANY EXPRESS OR
   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
   IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
   THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
   */

#include <stdint.h>

#ifdef _MSC_VER
#define _USE_MATH_DEFINES
#endif
#include <math.h>

#include "umath.h"
#include "umath_lut.h"

/* Constants */

#ifdef M_PI
#define PI M_PI
#else
#define PI (3.14159265)
#endif

#define PI_DBL (PI * 2)
#define PI_HLF (PI / 2)

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef DBL_EPSILON
#define DBL_EPSILON 0.00001
#endif
#ifndef ABS
#define ABS(x) ((x)>0?(x):(-(x)))
#endif

#define SERIES_EXPANSION_LOOP 10

/* Currently Not Been Used
#define SERIES_EXPANSION_LOOP_LONG 1000
#define DELTA_LIMIT (0.00001)
#define e (2.718281828459045235360287471352662498)
*/

#define QUADRANT_1 1
#define QUADRANT_2 2
#define QUADRANT_3 3
#define QUADRANT_4 4

/* Internal Functions */

static double umath_double_normalize_in_2pi(double x)
{
    while (x >= PI_DBL) 
    { x -= PI_DBL; }
    while (x < 0.0) 
    { x += PI_DBL; }
    return x;
}


/* Basic Functions */

double umath_double_pow(double x, int n)
{
    double result = 1.0;
    int neg = FALSE;

    if (n < 0)
    {
        n = -n;
        neg = TRUE;
    }

    while (n-- != 0)
    { result *= x; }

    if (neg != FALSE) { result = 1.0 / result; }

    return result;
}

double umath_double_factorial(int n)
{
    double result = 1.0;
    int i;

    for (i = 1; i <= n; i++)
    {
        result *= (double)i; 
    }

    return result;
}

/* Quadrant */
/*
 * II  |  I
 *     |
 * ----+----
 *     |
 * III |  IV
 */
static int umath_double_normalize_quadrant(double x)
{
    x = umath_double_normalize_in_2pi(x);
	if (x < PI / 2.0) return QUADRANT_1;
	else if (x < PI) return QUADRANT_2;
	else if (x < PI / 2.0 * 3.0) return QUADRANT_3;
	else return QUADRANT_4;
}


/* Absolute Value */

int umath_int_abs(int value) { return value >= 0 ? value : (-value); }
float umath_float_abs(float value) { return value >= 0.0 ? value : (-value); }
double umath_double_abs(double value) { return value >= 0.0 ? value : (-value); }


/* Square Root */

float umath_float_sqrt(float a)
{  
    int i;  
    float x;  
    x=a;  
    for(i=1;i<=10;i++)  
        x=(x+a/x)/2;  
    return x;  
}  

double umath_double_sqrt(double a)
{  
    int i;  
    double x;  
    x=a;  
    for(i=1;(i<=30) && (ABS(x*x-a)>DBL_EPSILON);i++)  
        x=(x+a/x)/2;  
    return x;  
}  

int umath_int_sqrt(int a)
{  
    int i;  
    double x;  
    x=(double)a;  
    for(i=1;(i<=30) && (ABS(x*x-a)>DBL_EPSILON);i++)  
        x=(x+a/x)/2;  
    return (int)x;  
}  

unsigned int umath_uint_sqrt(unsigned int a)
{  
    int i;  
    double x;  
    x=(double)a;  
    for(i=1;(i<=30) && (ABS(x*x-a)>DBL_EPSILON);i++)  
        x=(x+a/x)/2;  
    return (unsigned int)x;  
}  

/* Triangle */

double umath_double_cos(double x)
{
    int q = umath_double_normalize_quadrant(x);
    int neg = FALSE;
    int i;
    double result = 0.0;
    static double arr[4] = {1.0, 0.0, -1.0, 0.0};

    switch (q)
    {
	case QUADRANT_2: x = PI - x; neg = TRUE; break;
	case QUADRANT_3: x = PI - (PI_DBL - x); neg = TRUE; break;
	case QUADRANT_4: x = PI_DBL - x; neg = 1; break;
    }

    i = SERIES_EXPANSION_LOOP;
    while (i != 0)
    {
        result += arr[i & 3] * umath_double_pow(x, i) / ((double)umath_double_factorial(i));
        i--;
    }
    result += 1.0;
    if (neg == TRUE) result = -result;

    return result;
}

double umath_double_sin(double x)
{
    return umath_double_cos(PI_HLF - x);
}

double umath_double_tan(double x)
{
    return umath_double_sin(x) / umath_double_cos(x);
}

double umath_double_cot(double x)
{
    return umath_double_cos(x) / umath_double_sin(x);
}

double umath_double_sec(double x)
{
    return 1 / umath_double_cos(x);
}

double umath_double_csc(double x)
{
    return 1 / umath_double_sin(x);
}

double umath_double_asin(double x)
{
    if (x < 0) return 0.0;
    else if (x > 1) return umath_asin_lut[1024];
    return umath_asin_lut[(int)(x * 1024)];
}

/*
double umath_double_asin(double x)
{
    int i;
    double result = x;
    double sum = 1.0;
    unsigned int numerator_delta = 1;
    unsigned int denominator_delta = 2;
    unsigned int numerator = 1;
    unsigned int denominator = 1;
    unsigned int fix = 3;

    for (i = 1; i != SERIES_EXPANSION_LOOP + 1; i++)
    {
        numerator *= numerator_delta; numerator_delta += 2;
        denominator *= denominator_delta; denominator_delta += 2;
        sum = (double)numerator / (double)denominator / (double)fix * umath_double_pow(x, (int)fix);
        result += sum;
        fix += 2;
    }

    return result;
}
*/

/*
double umath_double_asin(double x)
{
    int i;
    double result = 0.0;
    double inc;
    unsigned int numerator_delta = 1;
    unsigned int denominator_delta = 2;
    unsigned int numerator = 1;
    unsigned int denominator = 1;
    int power_exponent = 3;

    inc = x;
    result = x;

    for (i = 1; i != SERIES_EXPANSION_LOOP; i++)
    {
        numerator *= numerator_delta; 
        denominator *= denominator_delta; 
        inc = ((double)numerator/(double)denominator) * umath_double_pow(x, power_exponent) / ((double)power_exponent);
        numerator_delta += 2;
        denominator_delta += 2;
        power_exponent += 2;
        result += inc;
    }

    return result;
}
*/

double umath_double_acos(double x)
{
    return PI_HLF - umath_double_asin(x);
}

double umath_double_atan(double x)
{
    return umath_double_asin(x/umath_double_sqrt((x*x)+1.0));
}


/* Miscellaneous */
double umath_double_exp(double x)
{
    int i;
    double result = 0.0;
    int neg;

    if (x >= 0)
    {
        neg = 1;
        x = ABS(x);
    }
    else
    {
        neg = 0;
    }

    i = SERIES_EXPANSION_LOOP;
    while (i != 0)
    {
        result += umath_double_pow(x, i) / (double)(umath_double_factorial(i));
        i -= 1;
    }
    result += 1.0;

    return neg != 0 ? result : 1.0 / result;
}

double umath_double_log(double x)
{
    int i;
    double result = 0.0;

    for (i = 1; i != SERIES_EXPANSION_LOOP; i++)
    {
        result += (1.0/i)*(umath_double_pow((x - 1.0)/x, i));
    }

    return result;
}


