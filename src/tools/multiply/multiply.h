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

#ifndef _MULTIPLY_H_
#define _MULTIPLY_H_

#include "selfcheck.h"

#include <stdint.h>
#include <stdio.h>

/* Multiple */
#include "multiple_ir.h"
#include "multiple_err.h"


/* Resource ID Pool */

#define MULTIPLY_RESOURCE_ID_POOL_START 1024
struct multiply_resource_id_pool
{
    uint32_t id;
};
struct multiply_resource_id_pool *multiply_resource_id_pool_new(void);
int multiply_resource_id_pool_destroy(struct multiply_resource_id_pool *id);


/* Low-level */
/* Low-level Interface is NOT recommanded to use directly */


/* resource */

/* none */
int multiply_resource_get_none( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out);

/* int */
int multiply_resource_get_int( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out, \
        int value);

/* float */
int multiply_resource_get_float( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out, \
        double value);

/* char */
int multiply_resource_get_char( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out, \
        uint32_t value);

/* bool */
#define MULTIPLY_RESOURCE_BOOL_FALSE 0
#define MULTIPLY_RESOURCE_BOOL_TRUE 1
int multiply_resource_get_bool( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out, \
        int value);
int multiply_resource_get_false( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out);
int multiply_resource_get_true( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out);

/* nan */
int multiply_resource_get_nan( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out, \
        int is_negative);

/* inf */
int multiply_resource_get_inf( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out, \
        int is_negative);

/* id */
int multiply_resource_get_id( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out, \
        const char *str, const size_t len);

/* str */
int multiply_resource_get_str( \
        struct multiple_error *err, struct multiple_ir *ir, struct multiply_resource_id_pool *res_id, \
        uint32_t *id_out, \
        const char *str, const size_t len);

/* .text */

int multiply_icodegen_text_section_append( \
        struct multiple_error *err, \
        struct multiple_ir *ir, \
        const uint32_t opcode, \
        const uint32_t operand);

uint32_t multiply_icodegen_text_section_current_offset(struct multiple_ir *ir);

int multiply_icodegen_text_fill_jmp(struct multiple_error *err, \
        struct multiple_ir *ir, \
        uint32_t offset_jmpc, uint32_t offset_dest);

#endif

