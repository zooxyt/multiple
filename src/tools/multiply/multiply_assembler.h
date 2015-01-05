/* Multiply -- Multiple IR Generator
 * Assembler
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

#ifndef _MULTIPLY_ASSEMBLER_H_
#define _MULTIPLY_ASSEMBLER_H_

#include <stdint.h>
#include <stdarg.h>

/* Multiple */
#include "multiple_ir.h"
#include "multiple_err.h"

#include "vm_opcode.h"

/* Multiply */
#include "multiply.h"

enum 
{
    MULTIPLY_ASM_OP_ID = 0, 
    MULTIPLY_ASM_OP_STR = 1, 
    MULTIPLY_ASM_OP_INT = 2, 
    MULTIPLY_ASM_OP_FLOAT = 3, 
    MULTIPLY_ASM_OP_NONE = 4,
    MULTIPLY_ASM_OP_FALSE = 5,
    MULTIPLY_ASM_OP_TRUE = 6,
    MULTIPLY_ASM_OP_NAN = 7,
    MULTIPLY_ASM_OP_INF = 8,
    MULTIPLY_ASM_OP_LBL = 9,
    MULTIPLY_ASM_OP_LBLR = 10,
    MULTIPLY_ASM_OP_RAW = 11,
    MULTIPLY_ASM_OP_TYPE = 12,
    MULTIPLY_ASM_OP = 13,
    MULTIPLY_ASM_LABEL = 14, 
    MULTIPLY_ASM_EXPORT = 15, 
    MULTIPLY_ASM_FINISH = 16, 
};


enum
{ 
    MULTIPLY_ASM_LINE_TYPE_OP_ID = 0, 
    MULTIPLY_ASM_LINE_TYPE_OP_STR = 1, 
    MULTIPLY_ASM_LINE_TYPE_OP_INT = 2, 
    MULTIPLY_ASM_LINE_TYPE_OP_FLOAT = 3, 
    MULTIPLY_ASM_LINE_TYPE_OP_NONE = 4,
    MULTIPLY_ASM_LINE_TYPE_OP_FALSE = 5,
    MULTIPLY_ASM_LINE_TYPE_OP_TRUE = 6,
    MULTIPLY_ASM_LINE_TYPE_OP_NAN = 7,
    MULTIPLY_ASM_LINE_TYPE_OP_INF = 8,
    MULTIPLY_ASM_LINE_TYPE_OP_LBL = 9,
    MULTIPLY_ASM_LINE_TYPE_OP_LBLR = 10,
    MULTIPLY_ASM_LINE_TYPE_OP_RAW = 11,
    MULTIPLY_ASM_LINE_TYPE_OP_TYPE = 12,
    MULTIPLY_ASM_LINE_TYPE_OP = 13,
    MULTIPLY_ASM_LINE_TYPE_LABEL = 14, 
};

/* Example 'abs'
 * Converted from moo code: 
 *   function abs(x) = if x < 0 then -x else x
 *
 * #define LBL_PUSH 1
 * #define LBL_RETURN 2
 *
 * multiple_icg_asm(err, icode, 
 *   MULTIPLY_ASM_EXPORT , "abs", 1, "x", 
 *   MULTIPLY_ASM_OP_ID  , OP_DEF     , "abs"      , 3,
 *   MULTIPLY_ASM_OP_ID  , OP_ARG     , "x"        , 1,
 *   MULTIPLY_ASM_OP_ID  , OP_PUSH    , "x"        , 1,
 *   MULTIPLY_ASM_OP_INT , OP_PUSH    , 0          ,
 *   MULTIPLY_ASM_OP     , OP_L       ,
 *   MULTIPLY_ASM_OP     , OP_NOTL    ,
 *   MULTIPLY_ASM_OP_LBL , OP_JMPC    , LBL_PUSH   ,
 *   MULTIPLY_ASM_OP_ID  , OP_PUSH    , "x"        , 1,
 *   MULTIPLY_ASM_OP     , OP_NEG     ,
 *   MULTIPLY_ASM_OP_LBL , OP_JMP     , LBL_RETURN ,
 *   MULTIPLY_ASM_LABEL  , LBL_PUSH   ,
 *   MULTIPLY_ASM_OP_ID  , OP_PUSH    , "x"        , 1,
 *   MULTIPLY_ASM_LABEL  , LBL_RETURN ,
 *   MULTIPLY_ASM_OP     , OP_RETURN  ,
 *   MULTIPLY_ASM_FINISH
 *   );
 *
 */


/* Precompiled text */

struct multiply_text_precompiled_line
{
    uint32_t opcode;
    uint32_t operand;
};

struct multiply_text_precompiled
{
    struct multiply_text_precompiled_line *lines;
    size_t size;
};

struct multiply_text_precompiled *multiply_text_precompiled_new(size_t size);
int multiply_text_precompiled_destroy(struct multiply_text_precompiled *text_precompiled);

/* Assemble to precompiled text */
int multiply_asm_precompile( \
        struct multiple_error *err, \
        struct multiple_ir *icode, \
        struct multiply_resource_id_pool *res_id, \
        struct multiply_text_precompiled **text_precompiled_out, \
        ...);

/* Assemble to icode directly */
int multiply_asm( \
        struct multiple_error *err, \
        struct multiple_ir *icode, \
        struct multiply_resource_id_pool *res_id, \
        ...);

/* A group of macro for making the assembly language
 * easier to draft */

/* TODO: Not yet started */

#endif

