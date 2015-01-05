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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "multiple_err.h"

#include "multiply_fcb.h"
#include "multiply_lbl_offset.h"


struct multiply_map_offset_label *multiply_map_offset_label_new( \
        uint32_t offset, \
        char *str, size_t len, \
        uint32_t pos_ln, uint32_t pos_col)
{
    struct multiply_map_offset_label *new_multiply_map_offset_label = NULL;

    new_multiply_map_offset_label = (struct multiply_map_offset_label *)malloc(sizeof(struct multiply_map_offset_label));
    if (new_multiply_map_offset_label == NULL) { goto fail; }
    new_multiply_map_offset_label->prev = new_multiply_map_offset_label->next = NULL;
    new_multiply_map_offset_label->offset = offset;
    new_multiply_map_offset_label->len = len;
    new_multiply_map_offset_label->str = (char *)malloc(sizeof(char) * (len + 1));
    if (new_multiply_map_offset_label->str == NULL) { goto fail; }
    memcpy(new_multiply_map_offset_label->str, str, len);
    new_multiply_map_offset_label->str[len] = '\0';
    new_multiply_map_offset_label->pos_ln = pos_ln;
    new_multiply_map_offset_label->pos_col = pos_col;

    goto done;
fail:
    if (new_multiply_map_offset_label != NULL)
    {
        multiply_map_offset_label_destroy(new_multiply_map_offset_label);
        new_multiply_map_offset_label = NULL;
    }
done:
    return new_multiply_map_offset_label;
}

int multiply_map_offset_label_destroy(struct multiply_map_offset_label *multiply_map_offset_label)
{
    if (multiply_map_offset_label->str != NULL) free(multiply_map_offset_label->str);
    free(multiply_map_offset_label);

    return 0;
}

struct multiply_map_offset_label_list *multiply_map_offset_label_list_new(void)
{
    struct multiply_map_offset_label_list *new_list = NULL;

    new_list = (struct multiply_map_offset_label_list *)malloc(sizeof(struct multiply_map_offset_label_list));
    if (new_list == NULL) { goto fail; }
    new_list->begin = new_list->end = NULL;

    goto done;
fail:
    if (new_list != NULL)
    {
         multiply_map_offset_label_list_destroy(new_list);
         new_list = NULL;
    }
done:
    return new_list;
}

int multiply_map_offset_label_list_destroy(struct multiply_map_offset_label_list *list)
{
    struct multiply_map_offset_label *multiply_map_offset_label_cur, *multiply_map_offset_label_next;

    multiply_map_offset_label_cur = list->begin;
    while (multiply_map_offset_label_cur != NULL)
    {
        multiply_map_offset_label_next = multiply_map_offset_label_cur->next; 
        multiply_map_offset_label_destroy(multiply_map_offset_label_cur);
        multiply_map_offset_label_cur = multiply_map_offset_label_next; 
    }
    free(list);

    return 0;
}

int multiply_map_offset_label_list_append(struct multiply_map_offset_label_list *list, \
        struct multiply_map_offset_label *new_multiply_map_offset_label)
{
    if (list->begin == NULL)
    {
        list->begin = list->end = new_multiply_map_offset_label;
    }
    else
    {
        new_multiply_map_offset_label->prev = list->end;
        list->end->next = new_multiply_map_offset_label;
        list->end = new_multiply_map_offset_label;
    }

    return 0;
}

int multiply_map_offset_label_list_append_with_configure( \
        struct multiply_map_offset_label_list *list, \
        uint32_t offset, \
        char *str, size_t len, \
        uint32_t pos_ln, uint32_t pos_col)
{
    struct multiply_map_offset_label *new_multiply_map_offset_label = NULL;

    new_multiply_map_offset_label = multiply_map_offset_label_new( \
            offset, \
            str, len, \
            pos_ln, pos_col);
    if (new_multiply_map_offset_label == NULL) { return -1; }

    multiply_map_offset_label_list_append(list, new_multiply_map_offset_label);

    return 0;
}


int multiply_statement_list_apply_goto( \
        struct multiple_error *err, \
        struct multiply_offset_item_pack_stack *offset_item_pack_stack, \
        struct multiply_fcb_block *icg_fcb_block, \
        struct multiply_map_offset_label_list *map_offset_label_list)
{
    int ret = 0;
    struct multiply_map_offset_label *map_offset_label_cur;
    struct multiply_offset_item *offset_item_target;
    uint32_t offset_label;
    uint32_t offset_goto;

    /* Apply goto to labels */
    map_offset_label_cur = map_offset_label_list->begin;
    while (map_offset_label_cur != NULL)
    {
        /* Locate to the label definition */
        offset_item_target = multiply_offset_item_pack_stack_lookup_by_label( \
                offset_item_pack_stack, \
                map_offset_label_cur->str, \
                map_offset_label_cur->len, \
                icg_fcb_block);
        if (offset_item_target == NULL)
        {
            multiple_error_update(err, -MULTIPLE_ERR_ICODEGEN, \
                    "%d:%d: error: label '%s' not found", \
                    map_offset_label_cur->pos_ln, map_offset_label_cur->pos_col, \
                    map_offset_label_cur->str);
            ret = -MULTIPLE_ERR_ICODEGEN;
            goto fail;
        }

        offset_label = offset_item_target->offset;
        offset_goto = map_offset_label_cur->offset;
        if ((ret = multiply_fcb_block_link_relative(icg_fcb_block, offset_goto, offset_label)) != 0) { goto fail; }

        map_offset_label_cur = map_offset_label_cur->next;
    }

    goto done;
fail:
done:
    return ret;
}

