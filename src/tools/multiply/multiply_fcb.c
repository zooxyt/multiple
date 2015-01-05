/* Multiply -- Multiple IR Generator
 * Floating Code Block
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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "multiple_err.h"
#include "multiple_ir.h"

#include "multiply_num.h"

#include "multiply_fcb.h"


/* Attributes for each line */

struct multiply_fcb_line_attr *multiply_fcb_line_attr_new(uint32_t attr_id, uint32_t res_id)
{
    struct multiply_fcb_line_attr *new_attr = NULL;

    new_attr = (struct multiply_fcb_line_attr *)malloc(sizeof(struct multiply_fcb_line_attr));
    if (new_attr == NULL) { goto fail; }
    new_attr->attr_id = attr_id;
    new_attr->res_id = res_id;
    new_attr->next = NULL;

fail:
    return new_attr;
}

int multiply_fcb_line_attr_destroy(struct multiply_fcb_line_attr *attr)
{
    free(attr);
    return 0;
}

struct multiply_fcb_line_attr_list *multiply_fcb_line_attr_list_new(void)
{
    struct multiply_fcb_line_attr_list *new_attr_list = NULL;

    new_attr_list = (struct multiply_fcb_line_attr_list *)malloc(sizeof(struct multiply_fcb_line_attr_list));
    if (new_attr_list == NULL) { goto fail; }
    new_attr_list->begin = new_attr_list->end = NULL;
    new_attr_list->size = 0;

fail:
    return new_attr_list;
}

int multiply_fcb_line_attr_list_destroy(struct multiply_fcb_line_attr_list *list)
{
    struct multiply_fcb_line_attr *attr_cur, *attr_next;

    attr_cur = list->begin;
    while (attr_cur != NULL)
    {
        attr_next = attr_cur->next; 
        multiply_fcb_line_attr_destroy(attr_cur);
        attr_cur = attr_next;
    }
    free(list);
    return 0;
}

int multiply_fcb_line_attr_list_append(struct multiply_fcb_line_attr_list *list, \
        struct multiply_fcb_line_attr *new_attr)
{
    if (list->begin == NULL)
    {
        list->begin = list->end = new_attr;
    }
    else
    {
        list->end->next = new_attr;
        list->end = new_attr;
    }
    list->size += 1;

    return 0;
}

int multiply_fcb_line_attr_list_append_with_configure(struct multiply_fcb_line_attr_list *list, \
        uint32_t attr_id, uint32_t res_id)
{
    struct multiply_fcb_line_attr *new_attr = NULL;

    new_attr = multiply_fcb_line_attr_new(attr_id, res_id);
    if (new_attr == NULL) return -MULTIPLE_ERR_MALLOC;

    multiply_fcb_line_attr_list_append(list, new_attr);

    return 0;
}


struct multiply_fcb_line *multiply_fcb_line_new(void)
{
    struct multiply_fcb_line *new_icg_fcb_line = NULL;

    new_icg_fcb_line = (struct multiply_fcb_line *)malloc(sizeof(struct multiply_fcb_line));
    if (new_icg_fcb_line == NULL) goto fail;
    new_icg_fcb_line->opcode = new_icg_fcb_line->operand = 0;
    new_icg_fcb_line->type = MULTIPLY_FCB_LINE_TYPE_NORMAL;
    new_icg_fcb_line->attrs = NULL;
    new_icg_fcb_line->prev = new_icg_fcb_line->next = NULL;
    goto done;
fail:
    if (new_icg_fcb_line != NULL) { free(new_icg_fcb_line); new_icg_fcb_line = NULL; }
done:
    return new_icg_fcb_line;
}

int multiply_fcb_line_destroy(struct multiply_fcb_line *icg_fcb_line)
{
    if (icg_fcb_line == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (icg_fcb_line->attrs != NULL)
    {
        multiply_fcb_line_attr_list_destroy(icg_fcb_line->attrs);
    }
    free(icg_fcb_line); 
    return 0;
}

static struct multiply_fcb_line *multiply_fcb_line_new_with_configure_raw(uint32_t opcode, uint32_t operand, int type)
{
    struct multiply_fcb_line *new_icg_fcb_line = NULL;

    new_icg_fcb_line = multiply_fcb_line_new();
    if (new_icg_fcb_line == NULL) { goto fail; }
    new_icg_fcb_line->type = type;
    new_icg_fcb_line->opcode = opcode;
    new_icg_fcb_line->operand = operand;
    new_icg_fcb_line->attrs = NULL;

    goto done;
fail:
    if (new_icg_fcb_line != NULL)
    {
        multiply_fcb_line_destroy(new_icg_fcb_line);
        new_icg_fcb_line = NULL;
    }
done:
    return new_icg_fcb_line;
}


struct multiply_fcb_line *multiply_fcb_line_new_with_configure(uint32_t opcode, uint32_t operand)
{
    return multiply_fcb_line_new_with_configure_raw(opcode, operand, MULTIPLY_FCB_LINE_TYPE_NORMAL);
}

struct multiply_fcb_line *multiply_fcb_line_new_with_configure_type(uint32_t opcode, uint32_t operand, int type)
{
    return multiply_fcb_line_new_with_configure_raw(opcode, operand, type);
}

struct multiply_fcb_block *multiply_fcb_block_new(void)
{
    struct multiply_fcb_block *new_icg_fcb_block = NULL;

    new_icg_fcb_block = (struct multiply_fcb_block *)malloc(sizeof(struct multiply_fcb_block));
    if (new_icg_fcb_block == NULL) goto fail;
    new_icg_fcb_block->begin = new_icg_fcb_block->end = NULL;
    new_icg_fcb_block->prev = new_icg_fcb_block->next = NULL;
    new_icg_fcb_block->size = 0;
    goto done;
fail:
    if (new_icg_fcb_block != NULL) { free(new_icg_fcb_block); }
done:
    return new_icg_fcb_block;
}

int multiply_fcb_block_destroy(struct multiply_fcb_block *icg_fcb_block)
{
    struct multiply_fcb_line *icg_fcb_line_cur, *icg_fcb_line_next;

    if (icg_fcb_block == NULL) return -MULTIPLE_ERR_NULL_PTR;
    icg_fcb_line_cur = icg_fcb_block->begin;
    while (icg_fcb_line_cur != NULL)
    {
        icg_fcb_line_next = icg_fcb_line_cur->next; 
        multiply_fcb_line_destroy(icg_fcb_line_cur);
        icg_fcb_line_cur = icg_fcb_line_next;
    }
    free(icg_fcb_block); 
    return 0;
}

int multiply_fcb_block_append(struct multiply_fcb_block *icg_fcb_block, \
        struct multiply_fcb_line *new_icg_fcb_line)
{
    if (icg_fcb_block == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (new_icg_fcb_line == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (icg_fcb_block->begin == NULL)
    {
        icg_fcb_block->begin = icg_fcb_block->end = new_icg_fcb_line;
    }
    else
    {
        icg_fcb_block->end->next = new_icg_fcb_line;
        new_icg_fcb_line->prev = icg_fcb_block->end;
        icg_fcb_block->end = new_icg_fcb_line;
    }
    icg_fcb_block->size += 1;

    return 0;
}

int multiply_fcb_block_insert(struct multiply_fcb_block *icg_fcb_block, \
        uint32_t instrument_number_insert, \
        struct multiply_fcb_line *new_icg_fcb_line)
{
    struct multiply_fcb_line *icg_fcb_line_cur = NULL, *icg_fcb_line_prev = NULL;
    uint32_t instrument_number = instrument_number_insert;

    /* Locate to the position */
    icg_fcb_line_cur = icg_fcb_block->begin;
    while ((icg_fcb_line_cur != NULL) && (instrument_number-- != 0))
    {
        icg_fcb_line_prev = icg_fcb_line_cur;
        icg_fcb_line_cur = icg_fcb_line_cur->next; 
    }

    if (icg_fcb_line_cur == NULL) { return -MULTIPLE_ERR_INTERNAL; }

    /* Insert */
    if (icg_fcb_line_prev == NULL)
    {
        new_icg_fcb_line->next = icg_fcb_block->begin;
        icg_fcb_block->begin->prev = new_icg_fcb_line;
        icg_fcb_block->begin = new_icg_fcb_line;
    }
    else
    {
        icg_fcb_line_prev->next = new_icg_fcb_line;
        new_icg_fcb_line->prev = icg_fcb_line_prev;
        new_icg_fcb_line->next = icg_fcb_line_cur;
        icg_fcb_line_cur->prev = new_icg_fcb_line;
    }
    icg_fcb_block->size += 1;

    /* Fix */
    icg_fcb_line_cur = icg_fcb_block->begin;
    while (icg_fcb_line_cur != NULL)
    {
        if (icg_fcb_line_cur->type == MULTIPLY_FCB_LINE_TYPE_PC)
        {
            if (icg_fcb_line_cur->operand > instrument_number_insert)
            {
                icg_fcb_line_cur->operand += 1;
            }
        }
        icg_fcb_line_cur = icg_fcb_line_cur->next;
    }

    return 0;
}


int multiply_fcb_block_append_with_configure(struct multiply_fcb_block *icg_fcb_block, \
        uint32_t opcode, uint32_t operand)
{
    int ret;
    struct multiply_fcb_line *new_icg_fcb_line = NULL;

    new_icg_fcb_line = multiply_fcb_line_new_with_configure(opcode, operand);
    if (new_icg_fcb_line == NULL) 
    {
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }
    if ((ret = multiply_fcb_block_append(icg_fcb_block, new_icg_fcb_line)) != 0)
    { goto fail; }

    goto done;
fail:
    if (new_icg_fcb_line != NULL)
    {
        multiply_fcb_line_destroy(new_icg_fcb_line);
        new_icg_fcb_line = NULL;
    }
done:
    return ret;
}

int multiply_fcb_block_append_with_configure_type(struct multiply_fcb_block *icg_fcb_block, \
        uint32_t opcode, uint32_t operand, int type)
{
    int ret;
    struct multiply_fcb_line *new_icg_fcb_line = NULL;

    new_icg_fcb_line = multiply_fcb_line_new_with_configure_type(opcode, operand, type);
    if (new_icg_fcb_line == NULL) 
    {
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }
    if ((ret = multiply_fcb_block_append(icg_fcb_block, new_icg_fcb_line)) != 0)
    { goto fail; }

    goto done;
fail:
    if (new_icg_fcb_line != NULL)
    {
        multiply_fcb_line_destroy(new_icg_fcb_line);
        new_icg_fcb_line = NULL;
    }
done:
    return ret;
}


uint32_t multiply_fcb_block_get_instrument_number(struct multiply_fcb_block *icg_fcb_block)
{
    return (uint32_t)(icg_fcb_block->size);
}

static int multiply_fcb_block_insert_with_configure_raw(struct multiply_fcb_block *icg_fcb_block, \
        uint32_t instrument_number, uint32_t opcode, uint32_t operand, int type)
{
    int ret;
    struct multiply_fcb_line *new_icg_fcb_line = NULL;

    new_icg_fcb_line = multiply_fcb_line_new_with_configure_raw(opcode, operand, type);
    if (new_icg_fcb_line == NULL) 
    {
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }

    if ((ret = multiply_fcb_block_insert(icg_fcb_block, instrument_number, new_icg_fcb_line)) != 0)
    { goto fail; }

    goto done;
fail:
    if (new_icg_fcb_line != NULL)
    {
        multiply_fcb_line_destroy(new_icg_fcb_line);
        new_icg_fcb_line = NULL;
    }
done:
    return ret;
}

int multiply_fcb_block_insert_with_configure(struct multiply_fcb_block *icg_fcb_block, \
        uint32_t instrument_number, uint32_t opcode, uint32_t operand)
{
    return multiply_fcb_block_insert_with_configure_raw(icg_fcb_block, instrument_number, \
            opcode, operand, MULTIPLY_FCB_LINE_TYPE_NORMAL);
}

int multiply_fcb_block_insert_with_configure_type(struct multiply_fcb_block *icg_fcb_block, \
        uint32_t instrument_number, uint32_t opcode, uint32_t operand, int type)
{
    return multiply_fcb_block_insert_with_configure_raw(icg_fcb_block, instrument_number, \
            opcode, operand, type);
}

int multiply_fcb_block_link(struct multiply_fcb_block *icg_fcb_block, \
        uint32_t instrument_number_from, uint32_t instrument_number_to)
{
    struct multiply_fcb_line *icg_fcb_line_cur;
    icg_fcb_line_cur = icg_fcb_block->begin;
    while ((instrument_number_from != 0) && (icg_fcb_line_cur != NULL))
    {
        icg_fcb_line_cur = icg_fcb_line_cur->next; 
        instrument_number_from -= 1;
    }
    if (icg_fcb_line_cur != NULL) 
    {
        icg_fcb_line_cur->operand = instrument_number_to;
        return 0;
    }
    else
    {
        return -1;
    }
}

int multiply_fcb_block_link_relative(struct multiply_fcb_block *icg_fcb_block, \
        uint32_t instrument_number_from, uint32_t instrument_number_to)
{
    struct multiply_fcb_line *icg_fcb_line_cur;
    uint32_t instrument_number_from_bak = instrument_number_from;

    icg_fcb_line_cur = icg_fcb_block->begin;
    while ((instrument_number_from != 0) && (icg_fcb_line_cur != NULL))
    {
        icg_fcb_line_cur = icg_fcb_line_cur->next; 
        instrument_number_from -= 1;
    }
    if (icg_fcb_line_cur != NULL) 
    {
        icg_fcb_line_cur->operand = snr_sam_to_cmp((int32_t)instrument_number_to - (int32_t)instrument_number_from_bak);
        return 0;
    }
    else
    {
        return -1;
    }
}

struct multiply_fcb_block_list *multiply_fcb_block_list_new(void)
{
    struct multiply_fcb_block_list *new_icg_fcb_block_list = NULL;

    new_icg_fcb_block_list = (struct multiply_fcb_block_list *)malloc(sizeof(struct multiply_fcb_block_list));
    if (new_icg_fcb_block_list == NULL) goto fail;
    new_icg_fcb_block_list->begin = new_icg_fcb_block_list->end = NULL;
    new_icg_fcb_block_list->size = 0;
    goto done;
fail:
    if (new_icg_fcb_block_list != NULL) { free(new_icg_fcb_block_list); }
done:
    return new_icg_fcb_block_list;
}

int multiply_fcb_block_list_destroy(struct multiply_fcb_block_list *icg_fcb_block_list)
{
    struct multiply_fcb_block *icg_fcb_block_cur, *icg_fcb_block_next;

    if (icg_fcb_block_list == NULL) return -MULTIPLE_ERR_NULL_PTR;
    icg_fcb_block_cur = icg_fcb_block_list->begin;
    while (icg_fcb_block_cur != NULL)
    {
        icg_fcb_block_next = icg_fcb_block_cur->next; 
        multiply_fcb_block_destroy(icg_fcb_block_cur);
        icg_fcb_block_cur = icg_fcb_block_next;
    }
    free(icg_fcb_block_list); 
    return 0;
}

int multiply_fcb_block_list_append(struct multiply_fcb_block_list *icg_fcb_block_list, \
        struct multiply_fcb_block *new_icg_fcb_block)
{
    if (icg_fcb_block_list == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (new_icg_fcb_block == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (icg_fcb_block_list->begin == NULL)
    {
        icg_fcb_block_list->begin = icg_fcb_block_list->end = new_icg_fcb_block;
    }
    else
    {
        icg_fcb_block_list->end->next = new_icg_fcb_block;
        icg_fcb_block_list->end = new_icg_fcb_block;
    }
    icg_fcb_block_list->size += 1;
    return 0;
}


