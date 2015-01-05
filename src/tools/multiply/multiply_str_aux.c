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

#include <stdio.h>
#include <stdint.h>

#include <stdlib.h>
#include <string.h>

/* Multiple */
#include "multiple_err.h"

/* Multiply */
#include "multiply_str_aux.h"


#define CONVERT_STR_TO_FLOAT_INTEGER 0
#define CONVERT_STR_TO_FLOAT_DECIMAL 1

int multiply_convert_str_to_float(double *value_out, \
        const char *str, const size_t len)
{
    int sign = 0;
    int part = CONVERT_STR_TO_FLOAT_INTEGER;
    double value_integer = 0.0;
    double value_decimal = 0.0;
    double value_decimal_idx = 0.0;
    int base = 10;
    int digit = 0;
    const char *str_p = str, *str_endp = str + len;
    char ch;

    if ((len >= 1) && (*str_p == '-'))
    { sign = 1; str_p++; }
    while (str_p != str_endp)
    {
        ch = *str_p;
        if (ch == '.')
        {
            part = CONVERT_STR_TO_FLOAT_DECIMAL;
            switch (base)
            {
                case 2: value_decimal_idx = 1.0 / 2; break;
                case 8: value_decimal_idx = 1.0 / 8; break;
                case 10: value_decimal_idx = 1.0 / 10; break;
                case 16: value_decimal_idx = 1.0 / 16; break;
            }
        }
        else
        {
            switch (base)
            {
                case 2:
                case 8:
                    if (ch < '0' && ch >= base)
                    {
                        return -1;
                    }
                    else
                    {
                        if (part == CONVERT_STR_TO_FLOAT_INTEGER)
                        {
                            value_integer = (value_integer * base) + (ch - (int)('0')); 
                        }
                        else
                        {
                            value_decimal = value_decimal + value_decimal_idx * (ch - (int)('0'));
                            value_decimal_idx /= base;
                        }
                    }
                    break;
                case 10:
                    if (ch < '0' && ch >= base)
                    {
                        return -1;
                    } else
                    {
                        if (part == CONVERT_STR_TO_FLOAT_INTEGER)
                        {
                            value_integer = (value_integer * 10) + (ch - (int)('0')); 
                        }
                        else
                        {
                            value_decimal = value_decimal + value_decimal_idx * (ch - (int)('0'));
                            value_decimal_idx /= base;
                        }
                    }
                    break;
                case 16:
                    if (ch >= '0' && ch <= '9')
                    { digit = ch - (int)('0'); }
                    else if (ch >= 'a' && ch <= 'f')
                    { digit = ch - (int)('a') + 10; }
                    else if (ch >= 'A' && ch <= 'F')
                    { digit = ch - (int)('A') + 10; }
                    else
                    {
                        return -1;
                    }
                    if (part == CONVERT_STR_TO_FLOAT_INTEGER)
                    {
                        value_integer = value_integer * base + digit;
                    }
                    else
                    {
                        value_decimal = value_decimal + value_decimal_idx * (ch - (int)('0'));
                        value_decimal_idx /= base;
                    }
                    break;
                default:
                    break;
            }
        }
        str_p++;
    }
    if (sign != 0) *value_out = -(value_integer + value_decimal);
    else *value_out = value_integer + value_decimal;
    return 0;
}

int multiply_convert_str_to_int(int *value_out, \
        const char *str, const size_t len)
{
    int sign = 0;
    int value = 0;
    int base = 10;
    int digit = 0;
    const char *str_p = str, *str_endp = str + len;
    char ch;
    static const int base_shift[17] = 
    { 0, 0, 1, 0, 2, 0, 0, 0, 3, 0,  0,  0,  0,  0,  0,  0,  4, };
    if ((len >= 1) && (*str_p == '-'))
    { sign = 1; str_p++; }
    if (len >= 2)
    {
        if (*str_p == '0' && (*(str_p + 1) == 'x' || *(str_p + 1) == 'X'))
        { base = 16; str_p += 2; }
        else if (*str_p == '0' && (*(str_p + 1) == 'b' || *(str_p + 1) == 'B'))
        { base = 2; str_p += 2; }
        else if (*str_p == '0')
        { base = 8; str_p += 1; }
        else
        { base = 10; }
    }
    else if (len >= 1)
    {
        if (*str_p == '0')
        { base = 8; str_p += 1; }
        else
        { base = 10; }
    }
    else
    {
        /* zero length */
        return -1;
    }
    while (str_p != str_endp)
    {
        ch = *str_p;
        switch (base)
        {
            case 2:
            case 8:
                if (ch < '0' && ch >= base)
                {
                    return -1;
                }
                else
                { value = (value << base_shift[base]) | (ch - (int)('0')); }
                break;
            case 10:
                if (ch < '0' && ch >= base)
                {
                    return -1;
                }
                else
                { value = (value * 10) + (ch - (int)('0')); }
                break;
            case 16:
                if (ch >= '0' && ch <= '9')
                { digit = ch - (int)('0'); }
                else if (ch >= 'a' && ch <= 'f')
                { digit = ch - (int)('a') + 10; }
                else if (ch >= 'A' && ch <= 'F')
                { digit = ch - (int)('A') + 10; }
                else
                {
                    return -1;
                }
                value = value << base_shift[base] | digit;
                break;
            default:
                break;
        }
        str_p++;
    }
    if (sign != 0) value = -value;
    *value_out = value;
    return 0;
}

int multiply_convert_str_to_uint(unsigned int *value_out, \
        const char *str, const size_t len)
{
    int ret = 0;
    int value;
    ret = multiply_convert_str_to_int(&value, str, len);
    *value_out = (unsigned int)value;
    return ret;
}

int multiply_convert_str_to_uint32_t(uint32_t *value_out, \
        const char *str, const size_t len)
{
    int ret = 0;
    int value;
    ret = multiply_convert_str_to_int(&value, str, len);
    *value_out = (uint32_t)value;
    return ret;
}

#define UTF8_NEXT_BYTE()\
{\
    ch = (unsigned char)*str_original_p;\
    if ((ch & 0xc0) != 0x80)\
    {\
        return -1; \
    } \
    new_unicode_char = (new_unicode_char << 6) | (ch & 0x3f); \
    str_original_p++;  \
}

int multiply_convert_utf_8_to_uint32(uint32_t *value_out, \
        const char *str, const size_t len)
{
    int bytes_number;
    uint32_t new_unicode_char = 0;
    char *str_original_p;
    unsigned char ch;

    /* Bytes Detection */
    ch = (unsigned char)*str;
    if ((ch & 0x80) == 0) bytes_number = 1; /* 0xxxxxxx */
    else if ((ch & 0xe0) == 0xc0) bytes_number = 2; /* 110xxxxx, 10xxxxxx */
    else if ((ch & 0xf0) == 0xe0) bytes_number = 3; /* 1110xxxx, 10xxxxxx, 10xxxxxx */
    else if ((ch & 0xf8) == 0xf0) bytes_number = 4; /* 11110xxx, 10xxxxxx, 10xxxxxx, 10xxxxxx */
    else if ((ch & 0xfc) == 0xf8) bytes_number = 5; /* 111110xx, 10xxxxxx, 10xxxxxx, 10xxxxxx, 10xxxxxx */
    else if ((ch & 0xfe) == 0xfc) bytes_number = 6; /* 1111110x, 10xxxxxx, 10xxxxxx, 10xxxxxx, 10xxxxxx, 10xxxxxx */
    else { return -1; }

    /* Length Checking */
    if (len < (size_t)bytes_number) { return -1; }

    str_original_p = (char *)str;
    switch (bytes_number)
    {
        case 1:
            new_unicode_char = ch & 0x7f;
            str_original_p++;
            break;
        case 2:
            new_unicode_char = (ch & 0x1f);
            str_original_p++;
            UTF8_NEXT_BYTE();
            break;
        case 3:
            new_unicode_char = (ch & 0x1f);
            str_original_p++;
            UTF8_NEXT_BYTE();
            UTF8_NEXT_BYTE();
            break;
        case 4:
            new_unicode_char = (ch & 0x1f);
            str_original_p++;
            UTF8_NEXT_BYTE();
            UTF8_NEXT_BYTE();
            UTF8_NEXT_BYTE();
            break;
        case 5:
            new_unicode_char = (ch & 0x1f);
            str_original_p++;
            UTF8_NEXT_BYTE();
            UTF8_NEXT_BYTE();
            UTF8_NEXT_BYTE();
            UTF8_NEXT_BYTE();
            break;
        case 6:
            new_unicode_char = (ch & 0x1f);
            str_original_p++;
            UTF8_NEXT_BYTE();
            UTF8_NEXT_BYTE();
            UTF8_NEXT_BYTE();
            UTF8_NEXT_BYTE();
            UTF8_NEXT_BYTE();
            break;
    }
    *value_out = new_unicode_char;
    return 0;
}

int multiply_concat_prefix_and_name( \
        char **full_name_out, size_t *full_name_len_out, \
        const char *prefix, const size_t prefix_len, \
        const char *name, const size_t name_len)
{
    int ret = 0;
    char *full_name = NULL, *full_name_p;
    size_t full_name_len = 0;

    *full_name_out = NULL;
    *full_name_len_out = 0;

    /* Prefix + "::" + Original Token */
    if (prefix != NULL)
    {
        full_name_len = name_len + 2 + prefix_len;
        full_name = (char *)malloc(sizeof(char) * (full_name_len + 1));
        if (full_name == NULL)
        { goto fail; }
        full_name_p = full_name;
        memcpy(full_name_p, prefix, prefix_len); full_name_p += prefix_len;
        *full_name_p++ = ':'; *full_name_p++ = ':';
        memcpy(full_name_p, name, name_len); full_name_p += name_len;
        *full_name_p++ = '\0';
    }
    else
    {
        full_name_len = name_len;
        full_name = (char *)malloc(sizeof(char) * full_name_len + 1);
        if (full_name == NULL)
        { goto fail; }
        full_name_p = full_name;
        memcpy(full_name_p, name, name_len); full_name_p += name_len;
        *full_name_p++ = '\0';
    }

    *full_name_out = full_name;
    *full_name_len_out = full_name_len;
fail:
    return ret;
}

/* Replace escapes in string with real chars */
int multiply_replace_escape_chars(char *str, size_t *len)
{
    char *str_src_p = str, *str_src_endp = str + *len;
    char *str_dst_p = str;
    while (str_src_p != str_src_endp)
    {
        if ((*str_src_p == '\\') && (str_src_endp - str_src_p >= 2))
        {
            switch (*(str_src_p + 1))
            {
                case 'a': *str_dst_p = '\a'; break;
                case 'b': *str_dst_p = '\b'; break;
                case 'f': *str_dst_p = '\f'; break;
                case 'n': *str_dst_p = '\n'; break;
                case 'r': *str_dst_p = '\r'; break;
                case 't': *str_dst_p = '\t'; break;
                case 'v': *str_dst_p = '\v'; break;
                case '\\': *str_dst_p = '\\'; break;
                case '\?': *str_dst_p = '\?'; break;
                case '\'': *str_dst_p = '\''; break;
                case '\"': *str_dst_p = '\"'; break;
                case '\0': *str_dst_p = '\0'; break;
                default: *str_dst_p = *(str_src_p + 1); break;
            }
            str_src_p += 2;
        }
        else
        {
            *str_dst_p = *str_src_p;
            str_src_p += 1;
        }
        str_dst_p += 1;
    }
    *str_dst_p = '\0';
    *len = (size_t)(str_dst_p - str);
    return 0;
}

