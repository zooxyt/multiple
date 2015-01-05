/* Multiply -- Multiple IR Generator
 * String Auxiliary
   Copyright(C) 2013-2014 Cheryl Natsu

   This file is part of multiple - Multiple Paradigm Language Emulator

   multiple is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   multiple is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>. 
   */

#ifndef _MULTIPLY_STR_AUX_H_
#define _MULTIPLY_STR_AUX_H_

#include <stdio.h>
#include <stdint.h>

/* Converting between number and string */
int multiply_convert_str_to_float(double *value_out, \
        const char *str, const size_t len);
int multiply_convert_str_to_int(int *value_out, \
        const char *str, const size_t len);
int multiply_convert_str_to_uint(unsigned int *value_out, \
        const char *str, const size_t len);
int multiply_convert_str_to_uint32_t(uint32_t *value_out, \
        const char *str, const size_t len);
int multiply_convert_utf_8_to_uint32(uint32_t *value_out, \
        const char *str, const size_t len);

/* Concatenate prefix and name with '::' */
int multiply_concat_prefix_and_name( \
        char **full_name_out, size_t *full_name_len_out, \
        const char *prefix, const size_t prefix_len, \
        const char *name, const size_t name_len);

/* Replace escapes in string with real chars */
int multiply_replace_escape_chars(char *str, size_t *len);

#endif

