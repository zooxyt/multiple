/* Multiply -- Multiple IR Generator
 * Labeled Offset
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

#ifndef _MULTIPLY_LBL_OFFSET_H_
#define _MULTIPLY_LBL_OFFSET_H_

#include <stdint.h>
#include <stdio.h>

#include "multiple_err.h"

#include "multiply_offset.h"
#include "multiply_fcb.h"

/* A data structure for connecting goto's offset and label */

struct multiply_map_offset_label
{
    uint32_t offset;

    char *str;
    size_t len;

    uint32_t pos_ln;
    uint32_t pos_col;

    struct multiply_map_offset_label *next;
    struct multiply_map_offset_label *prev;
};
struct multiply_map_offset_label *multiply_map_offset_label_new( \
        uint32_t offset, \
        char *str, size_t len, \
        uint32_t pos_ln, uint32_t pos_col);
int multiply_map_offset_label_destroy(struct multiply_map_offset_label *multiply_map_offset_label);

struct multiply_map_offset_label_list
{
    struct multiply_map_offset_label *begin;
    struct multiply_map_offset_label *end;
};
struct multiply_map_offset_label_list *multiply_map_offset_label_list_new(void);
int multiply_map_offset_label_list_destroy(struct multiply_map_offset_label_list *list);
int multiply_map_offset_label_list_append(struct multiply_map_offset_label_list *list, \
        struct multiply_map_offset_label *new_multiply_map_offset_label);
int multiply_map_offset_label_list_append_with_configure( \
        struct multiply_map_offset_label_list *list, \
        uint32_t offset, \
        char *str, size_t len, \
        uint32_t pos_ln, uint32_t pos_col);

int multiply_statement_list_apply_goto( \
        struct multiple_error *err, \
        struct multiply_offset_item_pack_stack *offset_item_pack_stack, \
        struct multiply_fcb_block *icg_fcb_block, \
        struct multiply_map_offset_label_list *map_offset_label_list);

#endif

