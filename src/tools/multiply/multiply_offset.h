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

#ifndef _MULTIPLY_OFFSET_H_
#define _MULTIPLY_OFFSET_H_

#include <stdio.h>
#include <stdint.h>

/* Offset Item */

struct multiply_offset_item
{
    uint32_t offset;

    char *str;
    size_t len;

    void *owner_block;

    struct multiply_offset_item *next;
};

struct multiply_offset_item *multiply_offset_item_new_with_value(uint32_t value);
struct multiply_offset_item *multiply_offset_item_new_with_value_label(uint32_t value, \
        char *str, size_t len, void *owner_block);
int multiply_offset_item_destroy(struct multiply_offset_item *item);

struct multiply_offset_item_pack
{
    struct multiply_offset_item *begin;
    struct multiply_offset_item *end;

    struct multiply_offset_item_pack *prev;
    struct multiply_offset_item_pack *next;
};

struct multiply_offset_item_pack *multiply_offset_item_pack_new(void);
int multiply_offset_item_pack_destroy(struct multiply_offset_item_pack *pack);
int multiply_offset_item_pack_push_back(struct multiply_offset_item_pack *pack, uint32_t multiply_offset);
int multiply_offset_item_pack_push_back_label(struct multiply_offset_item_pack *pack, \
        uint32_t multiply_offset, char *str, size_t len, void *owner_block);

struct multiply_offset_item_pack_stack
{
    struct multiply_offset_item_pack *bottom;
    struct multiply_offset_item_pack *top;
};
struct multiply_offset_item_pack_stack *multiply_offset_item_pack_stack_new(void);
int multiply_offset_item_pack_stack_destroy(struct multiply_offset_item_pack_stack *stack);
int multiply_offset_item_pack_stack_push(struct multiply_offset_item_pack_stack *stack, \
        struct multiply_offset_item_pack *new_pack);
int multiply_offset_item_pack_stack_pop(struct multiply_offset_item_pack_stack *stack);
struct multiply_offset_item *multiply_offset_item_pack_stack_lookup_by_label( \
        struct multiply_offset_item_pack_stack *stack, \
        char *str, size_t len, void *owner_block);

#endif

