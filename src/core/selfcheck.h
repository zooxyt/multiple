/* Self Check
 * Copyright(C) 2013-2014 Cheryl Natsu

 * This file is part of multiple - Multiple Paradigm Language Emulator

 * multiple is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * multiple is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 */

#ifndef _SELFCHECK_H_
#define _SELFCHECK_H_

/* Clang */
#ifdef __clang__
#pragma clang diagnostic ignored "-Wunreachable-code-break"
#pragma clang diagnostic ignored "-Wpadded"
#pragma clang diagnostic ignored "-Wpedantic"
#pragma clang diagnostic ignored "-Wformat-nonliteral"
#pragma clang diagnostic ignored "-Wunreachable-code"
#pragma clang diagnostic ignored "-Wunneeded-internal-declaration"
#endif

/* Microsoft Visual C++ */
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS 1
/* nonstandard extension, function/data pointer conversion in expression */
#pragma warning(disable:4152)
/* disable conditional expression is constant warning */
#pragma warning(disable:4127) 
/* type cast from void * to function pointer */
#pragma warning(disable:4055) 
/* size_t */
#ifndef ssize_t
#define ssize_t signed int
#endif
/* snprintf */
#define snprintf _snprintf
#endif

#if defined (__clang__)
#pragma clang diagnostic ignored "-Wswitch-enum"
#elif defined (__GNUC__)
#pragma GCC diagnostic ignored "-Wswitch-enum"
/*#pragma GCC diagnostic ignored "-Wdate-time"*/
#endif

#endif 

