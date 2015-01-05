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

#include "selfcheck.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "multiple_err.h"
#include "multiple_misc.h"

struct err_desc_by_number_item
{
    int err_no;
    const char *desc;
};
static struct err_desc_by_number_item err_desc_by_number[] =
{
    {MULTIPLE_ERR_UNDEFINED, "error: undefined error"},
    {MULTIPLE_ERR_UNKNOWN, "error: unknown error"},
    {MULTIPLE_ERR_INTERNAL, "error: internal error"},
    {MULTIPLE_ERR_NOT_IMPLEMENTED, "error: not implemented"},
    {MULTIPLE_ERR_NULL_PTR, "error: null pointer error"},
    {MULTIPLE_ERR_MALLOC, "error: out of memory"},
    {MULTIPLE_ERR_ATOMIC, "error: atomic error"},

    {MULTIPLE_ERR_INVALID_ARG, "error: invalid argument"},
    {MULTIPLE_ERR_STUB, "error: stub error"},
    {MULTIPLE_ERR_LEXICAL, "error: lexical analysis error"},
    {MULTIPLE_ERR_PARSING, "error: parsing error"},
    {MULTIPLE_ERR_SEMANTIC, "error: semantic analysis error"},
    {MULTIPLE_ERR_ICODEGEN, "error: intermediate code generation error"},
    {MULTIPLE_ERR_BYTECODE, "error: bytecode generation error"},
    {MULTIPLE_ERR_ASM, "error: assembly source code generation error"},
    {MULTIPLE_ERR_VM, "error: virtual machine error"},
};
#define MULTIPLE_ERR_DESC_BY_NUMBER_COUNT (sizeof(err_desc_by_number)/sizeof(struct err_desc_by_number_item))

static int multiple_copy_error_by_number(char *buf, size_t buf_size, int err_no)
{
    int idx;
    err_no = -err_no;
    for (idx = 0; idx != MULTIPLE_ERR_DESC_BY_NUMBER_COUNT; idx++)
    {
        if (err_desc_by_number[idx].err_no == err_no)
        {
            strncpy(buf, err_desc_by_number[idx].desc, buf_size);
            return 0;
        }
    }

    for (idx = 0; idx != MULTIPLE_ERR_DESC_BY_NUMBER_COUNT; idx++)
    {
        if (err_desc_by_number[idx].err_no == MULTIPLE_ERR_UNDEFINED)
        {
            strncpy(buf, err_desc_by_number[idx].desc, buf_size);
            return 0;
        }
    }

    return 0;
}

struct multiple_error *multiple_error_new(void)
{
    struct multiple_error *new_err = NULL;

    new_err = (struct multiple_error *)malloc(sizeof(struct multiple_error));
    if (new_err == NULL) return NULL;
    memset(new_err, 0, sizeof(struct multiple_error));

    new_err->description = (char *)malloc(sizeof(char) * ERR_DESCRIPTION_LEN);
    if (new_err->description == NULL) goto fail;
    new_err->description[0] = '\0';
    new_err->description_enabled = 0;

    new_err->filepath = (char *)malloc(sizeof(char) * ERR_PATHNAME_LEN);
    if (new_err->filepath == NULL) goto fail;
    new_err->filepath[0] = '\0';
    new_err->filepath_enabled = 0;

    return new_err;

fail:
    if (new_err != NULL)
    {
        if (new_err->description != NULL) free(new_err->description);
        if (new_err->filepath != NULL) free(new_err->filepath);
        free(new_err);
    }
    return NULL;
}

int multiple_error_destroy(struct multiple_error *err)
{
    if (err == NULL) return 0;

    if (err->description != NULL) free(err->description);
    if (err->filepath != NULL) free(err->filepath);
    free(err);

    return 0;
}

int multiple_error_clear(struct multiple_error *err)
{
    err->occurred = 0;
    err->number_enabled = 0;
    err->number = 0;
    err->filepath_enabled = 0;
    err->filepath[0] = '\0';
    err->description_enabled = 0;
    err->description[0] = '\0';

    return 0;
}

int multiple_error_update_pathname(struct multiple_error *err, const char *pathname)
{
    size_t pathname_len;
    char *basename = NULL;
    size_t basename_len;

    if (pathname == NULL)
    {
        err->filepath_enabled = 0;
        err->filepath[0] = '\0';
    }
    else
    {
        pathname_len = strlen(pathname);
        basename_get(&basename, &basename_len, pathname, pathname_len);
        if (basename_len != 0)
        {
            err->filepath_enabled = 1;
            memcpy(err->filepath, basename, basename_len);
        }
        err->filepath[basename_len] = '\0';
    }

    return 0;
}

int multiple_error_update(struct multiple_error *err, int number, const char *fmt, ...)
{
    va_list args;

    if (err->number_enabled == 0)
    {
        if (number != 0)
        {
            err->number = number;
            err->number_enabled = 1;
            err->occurred = 1;
        }
    }

    if (err->description_enabled == 0)
    {
        if (fmt != NULL)
        {
            va_start(args, fmt);
            vsprintf(err->description, fmt, args);
            /*vsnprintf(err->description, ERR_DESCRIPTION_LEN, fmt, args);*/
            va_end(args);
            err->description_enabled = 1;
            err->occurred = 1;
        }
    }

    return 0;
}

int multiple_error_occurred(struct multiple_error *err)
{
    return err->occurred;
}

int multiple_error_final(struct multiple_error *err, int ret)
{
    if (ret != 0)
    {
        err->number = ret;
        err->number_enabled = 1;
        err->occurred = 1;
    }

    return 0;
}

int multiple_error_print(struct multiple_error *err)
{
    if (err->occurred == 0) return 0;

    if (err->filepath_enabled != 0)
    {
        fprintf(stderr, "%s: ", err->filepath);
    }

    if (err->description_enabled != 0)
    {
        fputs(err->description, stderr);
        fprintf(stderr, "\n");
    }
    else if (err->number != 0)
    {
        multiple_copy_error_by_number(err->description, ERR_DESCRIPTION_LEN, err->number);
        fputs(err->description, stderr);
        fprintf(stderr, "\n");
    }

    return 0;
}

