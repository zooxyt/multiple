/* Multiply -- Multiple IR Generator
 * General
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

#include "selfcheck.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Multiple */
#include "multiple_ir.h"
#include "multiple_err.h"

/* Multiply */
#include "multiply.h"
#include "multiply_str_aux.h"


/* Resource ID */

struct multiply_resource_id_pool *multiply_resource_id_pool_new(void)
{
    struct multiply_resource_id_pool *new_resource_id_pool;
    if ((new_resource_id_pool = (struct multiply_resource_id_pool *)malloc( \
                    sizeof(struct multiply_resource_id_pool))) == NULL)
    { return NULL; }
    new_resource_id_pool->id = MULTIPLY_RESOURCE_ID_POOL_START;
    return new_resource_id_pool;
}

int multiply_resource_id_pool_destroy(struct multiply_resource_id_pool *id)
{
    free(id);
    return 0;
}


/* Low-level */

/* resource */

/* none */
int multiply_resource_get_none( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out)
{
    int ret = 0;
    struct multiple_ir_data_section_item *item_cur;
    struct multiple_ir_data_section_item *new_item;
    uint32_t id;

    for (item_cur = ir->data_section->begin; item_cur != NULL; item_cur = item_cur->next)
    {
        if (item_cur->type == MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NONE)
        { *id_out = item_cur->id; return 0; }
    }

    if ((new_item = multiple_ir_data_section_item_new(MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NONE)) == NULL)
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
    id = res_id->id++;
    new_item->id = id;
    new_item->size = sizeof(int);
    multiple_ir_data_section_append(ir->data_section, \
            new_item);

    *id_out = id;

fail:
    return ret;
}

/* int */
int multiply_resource_get_int( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out, \
        int value)
{
    int ret = 0;
    struct multiple_ir_data_section_item *item_cur;
    struct multiple_ir_data_section_item *new_item;
    uint32_t id;

    for (item_cur = ir->data_section->begin; item_cur != NULL; item_cur = item_cur->next)
    {
        if (item_cur->type == MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INT)
        {
            if (item_cur->u.value_int == value)
            { *id_out = item_cur->id; return 0; }
        }
    }

    if ((new_item = multiple_ir_data_section_item_new(MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INT)) == NULL)
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
    id = res_id->id++;
    new_item->id = id;
    new_item->size = sizeof(int);
    new_item->u.value_int = value;
    multiple_ir_data_section_append(ir->data_section, \
            new_item);

    *id_out = id;

fail:
    return ret;
}

/* float */
#ifndef DBL_EPSILON
#define DBL_EPSILON 0.00001
#endif
int multiply_resource_get_float( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out, \
        double value)
{
    int ret = 0;
    struct multiple_ir_data_section_item *item_cur;
    struct multiple_ir_data_section_item *new_item;
    uint32_t id;

    for (item_cur = ir->data_section->begin; item_cur != NULL; item_cur = item_cur->next)
    {
        if (item_cur->type == MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_FLOAT)
        {
            if (item_cur->u.value_float - value <= DBL_EPSILON)
            { *id_out = item_cur->id; return 0; }
        }
    }

    if ((new_item = multiple_ir_data_section_item_new(MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_FLOAT)) == NULL)
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
    id = res_id->id++;
    new_item->id = id;
    new_item->size = sizeof(double);
    new_item->u.value_float = value;
    multiple_ir_data_section_append(ir->data_section, \
            new_item);

    *id_out = id;

fail:
    return ret;
}

/* char */
int multiply_resource_get_char( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out, \
        uint32_t value)
{
    int ret = 0;
    struct multiple_ir_data_section_item *item_cur;
    struct multiple_ir_data_section_item *new_item;
    uint32_t id;

    for (item_cur = ir->data_section->begin; item_cur != NULL; item_cur = item_cur->next)
    {
        if (item_cur->type == MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_CHAR)
        {
            if (item_cur->u.value_char == value)
            { *id_out = item_cur->id; return 0; }
        }
    }

    if ((new_item = multiple_ir_data_section_item_new(MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_CHAR)) == NULL)
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
    id = res_id->id++;
    new_item->id = id;
    new_item->size = sizeof(uint32_t);
    new_item->u.value_char = value;
    multiple_ir_data_section_append(ir->data_section, \
            new_item);

    *id_out = id;

fail:
    return ret;
}

/* bool */
#define MULTIPLY_RESOURCE_BOOL_FALSE 0
#define MULTIPLY_RESOURCE_BOOL_TRUE 1
int multiply_resource_get_bool( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out, \
        int value)
{
    int ret = 0;
    struct multiple_ir_data_section_item *item_cur;
    struct multiple_ir_data_section_item *new_item;
    uint32_t id;

    for (item_cur = ir->data_section->begin; item_cur != NULL; item_cur = item_cur->next)
    {
        if (item_cur->type == MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL)
        {
            if (item_cur->u.value_bool == value)
            { *id_out = item_cur->id; return 0; }
        }
    }

    if ((new_item = multiple_ir_data_section_item_new(MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL)) == NULL)
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
    id = res_id->id++;
    new_item->id = id;
    new_item->size = sizeof(int);
    new_item->u.value_bool = value;
    multiple_ir_data_section_append(ir->data_section, \
            new_item);

    *id_out = id;

fail:
    return ret;
}
int multiply_resource_get_false( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out)
{
    return multiply_resource_get_bool( \
            err, ir, res_id, \
            id_out, \
            MULTIPLY_RESOURCE_BOOL_FALSE);
}
int multiply_resource_get_true( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out)
{
    return multiply_resource_get_bool( \
            err, ir, res_id, \
            id_out, \
            MULTIPLY_RESOURCE_BOOL_TRUE);
}

/* nan */
int multiply_resource_get_nan( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out, \
        int is_negative)
{
    int ret = 0;
    struct multiple_ir_data_section_item *item_cur;
    struct multiple_ir_data_section_item *new_item;
    uint32_t id;

    for (item_cur = ir->data_section->begin; item_cur != NULL; item_cur = item_cur->next)
    {
        if (item_cur->type == MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NAN)
        {
            if (item_cur->u.signed_nan == is_negative)
            { *id_out = item_cur->id; return 0; }
        }
    }

    if ((new_item = multiple_ir_data_section_item_new(MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NAN)) == NULL)
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
    id = res_id->id++;
    new_item->id = id;
    new_item->size = sizeof(int);
    new_item->u.signed_nan = is_negative;
    multiple_ir_data_section_append(ir->data_section, \
            new_item);

    *id_out = id;

fail:
    return ret;
}

/* inf */
int multiply_resource_get_inf( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out, \
        int is_negative)
{
    int ret = 0;
    struct multiple_ir_data_section_item *item_cur;
    struct multiple_ir_data_section_item *new_item;
    uint32_t id;

    for (item_cur = ir->data_section->begin; item_cur != NULL; item_cur = item_cur->next)
    {
        if (item_cur->type == MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INF)
        {
            if (item_cur->u.signed_inf == is_negative)
            { *id_out = item_cur->id; return 0; }
        }
    }

    if ((new_item = multiple_ir_data_section_item_new(MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INF)) == NULL)
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
    id = res_id->id++;
    new_item->id = id;
    new_item->size = sizeof(int);
    new_item->u.signed_inf = is_negative;
    multiple_ir_data_section_append(ir->data_section, \
            new_item);

    *id_out = id;

fail:
    return ret;
}

/* id */
int multiply_resource_get_id( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out, \
        const char *str, const size_t len)
{
    int ret = 0;
    struct multiple_ir_data_section_item *item_cur;
    struct multiple_ir_data_section_item *new_item;
    uint32_t id;

    for (item_cur = ir->data_section->begin; item_cur != NULL; item_cur = item_cur->next)
    {
        if (item_cur->type == MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER)
        {
            if ((item_cur->u.value_str.len == len) && \
                    (strncmp(item_cur->u.value_str.str, str, len) == 0))
            { *id_out = item_cur->id; return 0; }
        }
    }

    if ((new_item = multiple_ir_data_section_item_new(MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER)) == NULL)
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
    id = res_id->id++;
    new_item->id = id;
    new_item->size = (uint32_t)len;
    new_item->u.value_str.len = len;
    new_item->u.value_str.str = (char *)malloc(sizeof(char) * (len + 1));
    if (new_item->u.value_str.str == NULL)
    { goto fail; }
    memcpy(new_item->u.value_str.str, str, len);
    new_item->u.value_str.str[len] = '\0';

    multiple_ir_data_section_append(ir->data_section, \
            new_item);

    *id_out = id;

    goto done;
fail:
    if (new_item != NULL)
    {
        multiple_ir_data_section_item_destroy(new_item);
    }
done:
    return ret;
}

/* str */
int multiply_resource_get_str( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out, \
        const char *str, const size_t len)
{
    int ret = 0;
    struct multiple_ir_data_section_item *item_cur;
    struct multiple_ir_data_section_item *new_item;
    uint32_t id;

    for (item_cur = ir->data_section->begin; item_cur != NULL; item_cur = item_cur->next)
    {
        if (item_cur->type == MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_STR)
        {
            if ((item_cur->u.value_str.len == len) && \
                    (strncmp(item_cur->u.value_str.str, str, len) == 0))
            { *id_out = item_cur->id; return 0; }
        }
    }

    if ((new_item = multiple_ir_data_section_item_new(MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_STR)) == NULL)
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
    id = res_id->id++;
    new_item->id = id;
    new_item->size = (uint32_t)len;
    new_item->u.value_str.len = len;
    new_item->u.value_str.str = (char *)malloc(sizeof(char) * (len + 1));
    if (new_item->u.value_str.str == NULL)
    { goto fail; }
    memcpy(new_item->u.value_str.str, str, len);
    new_item->u.value_str.str[len] = '\0';

    multiple_ir_data_section_append(ir->data_section, \
            new_item);

    *id_out = id;

    goto done;
fail:
    if (new_item != NULL)
    {
        multiple_ir_data_section_item_destroy(new_item);
    }
done:
    return ret;
}


/* .text */

int multiply_icodegen_text_section_append( \
        struct multiple_error *err, \
        struct multiple_ir *ir, \
        const uint32_t opcode, \
        const uint32_t operand)
{
    int ret = 0;
    struct multiple_ir_text_section_item *new_text_section_item = NULL;

    if ((new_text_section_item = multiple_ir_text_section_item_new()) == NULL)
    { MULTIPLE_ERROR_MALLOC(); ret = -MULTIPLE_ERR_MALLOC; goto fail; }
    new_text_section_item->opcode = opcode;
    new_text_section_item->operand = operand;
    multiple_ir_text_section_append(ir->text_section, new_text_section_item);

    goto done;
fail:
    if (new_text_section_item != NULL) multiple_ir_text_section_item_destroy(new_text_section_item);
done:
    return ret;
}


uint32_t multiply_icodegen_text_section_current_offset(struct multiple_ir *ir)
{
    return (uint32_t)ir->text_section->size;
}


int multiply_icodegen_text_fill_jmp(struct multiple_error *err, \
        struct multiple_ir *ir, \
        uint32_t offset_jmpc, uint32_t offset_dest)
{
    struct multiple_ir_text_section_item *item_cur;

    item_cur = ir->text_section->begin;
    while (item_cur != NULL)
    {
        if (offset_jmpc == 0)
        {
            item_cur->operand = offset_dest;
            return 0;
        }
        offset_jmpc--;
        item_cur = item_cur->next;
    }

    MULTIPLE_ERROR_INTERNAL();
    return -MULTIPLE_ERR_INTERNAL;
}

