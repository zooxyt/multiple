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

#ifndef _UMATH_H_
#define _UMATH_H_

/* Basic Functions */

double umath_double_pow(double x, int n);
double umath_double_factorial(int n);


/* Absolute Value */

int umath_int_abs(int value);
float umath_float_abs(float value);
double umath_double_abs(double value);


/* Square Root */

float umath_float_sqrt(float a);
double umath_double_sqrt(double a);
int umath_int_sqrt(int a);
unsigned int umath_uint_sqrt(unsigned int a);


/* Triangle */

double umath_double_sin(double x);
double umath_double_cos(double x);
double umath_double_tan(double x);
double umath_double_cot(double x);
double umath_double_sec(double x);
double umath_double_csc(double x);

double umath_double_asin(double x);
double umath_double_acos(double x);
double umath_double_atan(double x);


/* Miscellaneous */
double umath_double_exp(double x);
double umath_double_log(double x);

#endif

