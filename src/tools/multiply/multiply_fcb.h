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

#ifndef _MULTIPLY_FCB_H_
#define _MULTIPLY_FCB_H_

#include <stdio.h>
#include <stdint.h>

/* Attributes for each line */

struct multiply_fcb_line_attr
{
    uint32_t attr_id;
    uint32_t res_id;
    struct multiply_fcb_line_attr *next;
};

struct multiply_fcb_line_attr *multiply_fcb_line_attr_new(uint32_t attr_id, \
        uint32_t res_id); 
int multiply_fcb_line_attr_destroy(struct multiply_fcb_line_attr *attr);

struct multiply_fcb_line_attr_list
{
    struct multiply_fcb_line_attr *begin;
    struct multiply_fcb_line_attr *end;
    size_t size;
};

struct multiply_fcb_line_attr_list *multiply_fcb_line_attr_list_new(void);
int multiply_fcb_line_attr_list_destroy(struct multiply_fcb_line_attr_list *list);
int multiply_fcb_line_attr_list_append(struct multiply_fcb_line_attr_list *list, \
        struct multiply_fcb_line_attr *new_attr);
int multiply_fcb_line_attr_list_append_with_configure(struct multiply_fcb_line_attr_list *list, \
        uint32_t attr_id, uint32_t res_id);


enum
{
    /* No needed to do anything to operand */
    MULTIPLY_FCB_LINE_TYPE_NORMAL = 0,        

    /* operand = global_start + operand */
    MULTIPLY_FCB_LINE_TYPE_PC = 1,            

    /* operand = global_offsets_of_lambda_procs[res_id] */
    MULTIPLY_FCB_LINE_TYPE_LAMBDA_MK = 2,     

    /* operand = global_offsets_of_built_in_proces[res_id] 
     * At the beginning of __init__ */
    MULTIPLY_FCB_LINE_TYPE_BLTIN_PROC_MK = 3, 
};

struct multiply_fcb_line
{
    uint32_t opcode;
    uint32_t operand;
    int type;
    struct multiply_fcb_line_attr_list *attrs;

    struct multiply_fcb_line *prev;
    struct multiply_fcb_line *next;
};
struct multiply_fcb_line *multiply_fcb_line_new(void);
int multiply_fcb_line_destroy(struct multiply_fcb_line *icg_fcb_line);
struct multiply_fcb_line *multiply_fcb_line_new_with_configure(uint32_t opcode, uint32_t operand);
struct multiply_fcb_line *multiply_fcb_line_new_with_configure_type(uint32_t opcode, uint32_t operand, int type);

struct multiply_fcb_block
{
    struct multiply_fcb_line *begin;
    struct multiply_fcb_line *end;
    size_t size;

    struct multiply_fcb_block *prev;
    struct multiply_fcb_block *next;
};
struct multiply_fcb_block *multiply_fcb_block_new(void);
int multiply_fcb_block_destroy(struct multiply_fcb_block *icg_fcb_block);
int multiply_fcb_block_append(struct multiply_fcb_block *icg_fcb_block, \
        struct multiply_fcb_line *new_icg_fcb_line);
int multiply_fcb_block_insert(struct multiply_fcb_block *icg_fcb_block, \
        uint32_t instrument_number_insert, \
        struct multiply_fcb_line *new_icg_fcb_line);

int multiply_fcb_block_append_with_configure(struct multiply_fcb_block *icg_fcb_block, \
        uint32_t opcode, uint32_t operand);
int multiply_fcb_block_append_with_configure_type(struct multiply_fcb_block *icg_fcb_block, \
        uint32_t opcode, uint32_t operand, int type);

uint32_t multiply_fcb_block_get_instrument_number(struct multiply_fcb_block *icg_fcb_block);

int multiply_fcb_block_insert_with_configure(struct multiply_fcb_block *icg_fcb_block, \
        uint32_t instrument, uint32_t opcode, uint32_t operand);
int multiply_fcb_block_insert_with_configure_type(struct multiply_fcb_block *icg_fcb_block, \
        uint32_t instrument, uint32_t opcode, uint32_t operand, int type);

int multiply_fcb_block_link(struct multiply_fcb_block *icg_fcb_block, \
        uint32_t instrument_number_from, uint32_t instrument_number_to);
int multiply_fcb_block_link_relative(struct multiply_fcb_block *icg_fcb_block, \
        uint32_t instrument_number_from, uint32_t instrument_number_to);

struct multiply_fcb_block_list
{
    struct multiply_fcb_block *begin;
    struct multiply_fcb_block *end;
    size_t size;
};
struct multiply_fcb_block_list *multiply_fcb_block_list_new(void);
int multiply_fcb_block_list_destroy(struct multiply_fcb_block_list *icg_fcb_block_list);
int multiply_fcb_block_list_append(struct multiply_fcb_block_list *icg_fcb_block_list, \
        struct multiply_fcb_block *new_icg_fcb_block);


#endif

