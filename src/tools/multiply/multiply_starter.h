/* Multiply -- Multiple IR Generator
 * Starter - A Backend Framework for Multiple
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

#ifndef _MULTIPLY_STARTER_H_
#define _MULTIPLY_STARTER_H_

#include <stdint.h>

/* Low-level things */
#include "multiply.h"
/* Floating Code Block */
#include "multiply_fcb.h"
/* Built-in Procedure */
#include "multiply_bip.h"
/* Offset */
#include "multiply_offset.h"
/* Labeled Offset */
#include "multiply_lbl_offset.h"
/* Assembler */
#include "multiply_assembler.h"


/* Purpose of this project is to create a one station 
 * backend framework for IR generating */

/* Usage:
 * the "multiply_starter.h" is the only file to be included
 * in all parts of the IR generating, and the data structure 
 * 'multiply_starter_t' should also be passed in all functions of
 * IR generating. */

/* Features */
/* Features enabled in the starter */
enum
{
    MULTIPLY_STARTER_FEATURE_BIP = (1 << 0),   /* Build in Procedure */
    MULTIPLY_STARTER_FEATURE_BREAK = (1 << 1), /* Break offset pack */
    MULTIPLY_STARTER_FEATURE_LABEL = (1 << 2), /* Labeled offset pack */
};
/* None feature enabled */
#define MULTIPLE_STARTER_FEATURE_NONE (0)
/* None feature enabled */
#define MULTIPLE_STARTER_FEATURE_ALL \
    (MULTIPLY_STARTER_FEATURE_BIP | \
     MULTIPLY_STARTER_FEATURE_BREAK | \
     MULTIPLY_STARTER_FEATURE_LABEL)

typedef uint32_t multiply_starter_feature_t;

struct multiply_starter
{
    struct multiply_resource_id_pool *res_id;
    struct multiply_fcb_block_list *fcb_list;

    /* Features */
    struct multiply_customizable_built_in_procedure_list *bip_list;

    struct multiply_map_offset_label_list *map_offset_label_list;
    struct multiply_offset_item_pack_stack *offset_item_pack_stack;

    multiply_starter_feature_t feature;
};
typedef struct multiply_starter multiply_starter_t;

/* Create and destroy starter context 
 * (the data structure should be pass across all parts
 * of IR generating functions) */
multiply_starter_t *multiply_starter_new(multiply_starter_feature_t feature);
int multiply_starter_destroy(multiply_starter_t *starter);
int multiply_starter_generate_ir( \
        struct multiple_error *err, \
        struct multiple_ir **ir_out, \
        multiply_starter_t *starter);


/* Line */
/* A Line is a 'macro' instruction to represent a
 * structure in the program */
struct multiple_starter_line
{
};
typedef struct multiple_starter_line multiple_starter_line_t;


/* Block */
/* A block is a serial of instruments with attributes that
 * represents a procedure or a function */
struct multiple_starter_block
{
    size_t size;
};
typedef struct multiple_starter_block multiple_starter_block_t;


#endif

