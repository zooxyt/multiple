/* Error Handling
 * Copyright(C) 2013 Cheryl Natsu

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

#ifndef _MULTIPLE_ERR_H_
#define _MULTIPLE_ERR_H_

#include "selfcheck.h"

#include <stdarg.h>

/* Error Types */
enum 
{
    MULTIPLE_ERR_UNKNOWN = 1,
    MULTIPLE_ERR_UNDEFINED = 2,
    MULTIPLE_ERR_INTERNAL = 3,
    MULTIPLE_ERR_NOT_IMPLEMENTED = 4,
    MULTIPLE_ERR_NULL_PTR = 5,
    MULTIPLE_ERR_MALLOC = 6,
    MULTIPLE_ERR_ATOMIC = 7,

    MULTIPLE_ERR_INVALID_ARG = 8,
    MULTIPLE_ERR_STUB = 9,
    MULTIPLE_ERR_LEXICAL = 10,
    MULTIPLE_ERR_PARSING = 11,
    MULTIPLE_ERR_SEMANTIC = 12,
    MULTIPLE_ERR_ICODEGEN = 13,
    MULTIPLE_ERR_BYTECODE = 14,
    MULTIPLE_ERR_ASM = 15,
    MULTIPLE_ERR_VM = 16,
};

/* Print error description */
int multiple_print_error(int err_no);

#define ERR_PATHNAME_LEN 256
#define ERR_DESCRIPTION_LEN 512

struct multiple_error
{
    int occurred;

    int number_enabled;
	int number;
    int filepath_enabled;
	int description_enabled;
	char *filepath;
	char *description;
};

/* Could cause problem when put error message directly on stack */
struct multiple_error *multiple_error_new(void);
int multiple_error_destroy(struct multiple_error *err);

int multiple_error_clear(struct multiple_error *err);
int multiple_error_occurred(struct multiple_error *err);
int multiple_error_update_pathname(struct multiple_error *err, const char *pathname);
int multiple_error_update(struct multiple_error *err, int number, const char *fmt, ...);
int multiple_error_final(struct multiple_error *err, int ret);
int multiple_error_print(struct multiple_error *err);

#ifndef __FUNC_NAME__
#if defined(__GNUC__)
# define __FUNC_NAME__ __FUNCTION__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
# define __FUNC_NAME__ __func__
#else
# define __FUNC_NAME__ ("??")
#endif
#endif

#define MULTIPLE_ERROR_UNKNOWN() \
    multiple_error_update(err, -MULTIPLE_ERR_UNKNOWN, "error: unknown error in %s() %s:%d", __FUNC_NAME_, __FILE__, __LINE__)

#define MULTIPLE_ERROR_UNDEFINED() \
    multiple_error_update(err, -MULTIPLE_ERR_UNDEFINED, "error undefined error in %s() %s:%d", __FUNC_NAME_, __FILE__, __LINE__)

#define MULTIPLE_ERROR_INTERNAL() \
    multiple_error_update(err, -MULTIPLE_ERR_INTERNAL, "error: internal error in %s() %s:%d", __FUNC_NAME__, __FILE__, __LINE__)

#define MULTIPLE_ERROR_NOT_IMPLEMENTED() \
    multiple_error_update(err, -MULTIPLE_ERR_NOT_IMPLEMENTED, "error: feature not implemented yet in %s() %s:%d",  __FUNC_NAME__, __FILE__, __LINE__)

#define MULTIPLE_ERROR_NULL_PTR() \
    multiple_error_update(err, -MULTIPLE_ERR_NULL_PTR, "error: null pointer in %s() %s:%d",  __FUNC_NAME__, __FILE__, __LINE__)

#define MULTIPLE_ERROR_MALLOC() \
    multiple_error_update(err, -MULTIPLE_ERR_MALLOC, "error: out of memory in %s() %s:%d",  __FUNC_NAME__, __FILE__, __LINE__)

#endif

