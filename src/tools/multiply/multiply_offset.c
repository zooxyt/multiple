/* Multiply -- Multiple IR Generator
 * Offset
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

#include "multiple_err.h"
#include "multiply_offset.h"


/* Offset Pack */

/* For recording offsets while generating code for if..elif..else..end */

struct multiply_offset_item *multiply_offset_item_new_with_value(uint32_t value)
{
    struct multiply_offset_item *new_item = NULL;

    if ((new_item = (struct multiply_offset_item *)malloc(sizeof(struct multiply_offset_item))) == NULL)
    { return NULL; }
    new_item->next = NULL;
    new_item->offset = value;
    new_item->str = NULL;
    new_item->len = 0;

    return new_item;
}

struct multiply_offset_item *multiply_offset_item_new_with_value_label(uint32_t value, \
        char *str, size_t len, void *owner_block)
{
    struct multiply_offset_item *new_item = NULL;

    if ((new_item = (struct multiply_offset_item *)malloc(sizeof(struct multiply_offset_item))) == NULL)
    { return NULL; }
    new_item->next = NULL;
    new_item->offset = value;
    new_item->owner_block = owner_block;
    new_item->str = (char *)malloc(sizeof(char) * (len + 1));
    if (new_item->str == NULL) { goto fail; }
    memcpy(new_item->str, str, len);
    new_item->str[len] = '\0';
    new_item->len = len;

    goto done;
fail:
    if (new_item != NULL)
    {
        multiply_offset_item_destroy(new_item);
        new_item = NULL;
    }
done:
    return new_item;
}

int multiply_offset_item_destroy(struct multiply_offset_item *item)
{
    if (item->str != NULL) free(item->str);
    free(item);

    return 0;
}

struct multiply_offset_item_pack *multiply_offset_item_pack_new(void)
{
    struct multiply_offset_item_pack *new_pack = NULL;

    if ((new_pack = (struct multiply_offset_item_pack *)malloc(sizeof(struct multiply_offset_item_pack))) == NULL)
    { return NULL; }
    new_pack->begin = new_pack->end = NULL;
    new_pack->prev = new_pack->next = NULL;

    return new_pack;
}

int multiply_offset_item_pack_destroy(struct multiply_offset_item_pack *pack)
{
    struct multiply_offset_item *cur_item, *next_item;

    cur_item = pack->begin;
    while (cur_item != NULL)
    {
        next_item = cur_item->next;
        multiply_offset_item_destroy(cur_item);
        cur_item = next_item;
    }
    free(pack);

    return 0;
}

int multiply_offset_item_pack_push_back(struct multiply_offset_item_pack *pack, uint32_t multiply_offset)
{
    struct multiply_offset_item *new_item = NULL;

    if ((new_item = multiply_offset_item_new_with_value(multiply_offset)) == NULL) 
    {
        return -MULTIPLE_ERR_MALLOC;
    }
    if (pack->begin == NULL)
    {
        pack->begin = pack->end = new_item;
    }
    else
    {
        pack->end->next = new_item;
        pack->end = new_item;
    }

    return 0;
}

int multiply_offset_item_pack_push_back_label(struct multiply_offset_item_pack *pack, \
        uint32_t multiply_offset, char *str, size_t len, void *owner_block)
{
    struct multiply_offset_item *new_item = NULL;

    if ((new_item = multiply_offset_item_new_with_value_label(multiply_offset, str, len, owner_block)) == NULL) 
    {
        return -MULTIPLE_ERR_MALLOC;
    }
    if (pack->begin == NULL)
    {
        pack->begin = pack->end = new_item;
    }
    else
    {
        pack->end->next = new_item;
        pack->end = new_item;
    }

    return 0;
}


struct multiply_offset_item_pack_stack *multiply_offset_item_pack_stack_new(void)
{
    struct multiply_offset_item_pack_stack *new_stack = NULL;

    new_stack = (struct multiply_offset_item_pack_stack *)malloc(sizeof(struct multiply_offset_item_pack_stack));
    if (new_stack == NULL) { goto fail; }
    new_stack->bottom = new_stack->top = NULL;

    goto done;
fail:
    if (new_stack != NULL)
    {
        free(new_stack);
        new_stack = NULL;
    }
done:
    return new_stack;
}

int multiply_offset_item_pack_stack_destroy(struct multiply_offset_item_pack_stack *stack)
{
    struct multiply_offset_item_pack *pack_cur, *pack_next;

    pack_cur = stack->bottom;
    while (pack_cur != NULL)
    {
        pack_next = pack_cur->next; 
        multiply_offset_item_pack_destroy(pack_cur);
        pack_cur = pack_next; 
    }
    free(stack);

    return 0;
}

int multiply_offset_item_pack_stack_push(struct multiply_offset_item_pack_stack *stack, \
        struct multiply_offset_item_pack *new_pack)
{
    if (stack->bottom == NULL)
    {
        stack->bottom = stack->top = new_pack;
    }
    else
    {
        new_pack->prev = stack->top;
        stack->top->next = new_pack;
        stack->top = new_pack;
    }

    return 0;
}

int multiply_offset_item_pack_stack_pop(struct multiply_offset_item_pack_stack *stack)
{
    struct multiply_offset_item_pack *pack_to_remove, *pack_prev;

    if (stack->top == NULL) { return 0; }

    pack_to_remove = stack->top;
    pack_prev = stack->top->prev;

    if (stack->top->prev != NULL) stack->top->prev->next = NULL;
    stack->top = pack_prev;
    if (stack->top == NULL) stack->bottom = NULL;

    multiply_offset_item_pack_destroy(pack_to_remove);

    return 0;
}

struct multiply_offset_item *multiply_offset_item_pack_stack_lookup_by_label( \
        struct multiply_offset_item_pack_stack *stack, \
        char *str, size_t len, void *owner_block)
{
    struct multiply_offset_item_pack *multiply_offset_item_pack_cur = NULL;
    struct multiply_offset_item *multiply_offset_item_cur;

    multiply_offset_item_pack_cur = stack->top;
    while (multiply_offset_item_pack_cur != NULL)
    {
        multiply_offset_item_cur = multiply_offset_item_pack_cur->begin;
        while (multiply_offset_item_cur != NULL)
        {
            if (multiply_offset_item_cur->owner_block != owner_block)
            {
                return NULL;
            }
            if ((multiply_offset_item_cur->len == len) && \
                    (strncmp(multiply_offset_item_cur->str, str, len) == 0))
            {
                return multiply_offset_item_cur;
            }
            multiply_offset_item_cur = multiply_offset_item_cur->next;
        }
        multiply_offset_item_pack_cur = multiply_offset_item_pack_cur->prev;
    }

    return NULL;
}

