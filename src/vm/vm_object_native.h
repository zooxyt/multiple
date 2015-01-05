/* Low-level Native Objects
 * Copyright(C) 2013-2014 Cheryl Natsu

 * This file is part of multiple - Multiple Paradigm Language Emulator

 * multiple is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * multiple is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>. 
 */

#ifndef _VM_OBJECT_NATIVE_H_
#define _VM_OBJECT_NATIVE_H_

#include <stdint.h>

struct virtual_machine_object;

struct virtual_machine_object_int
{
    int value;
};

/* new */
struct virtual_machine_object *virtual_machine_object_int_new_with_value( \
        struct virtual_machine *vm, \
        const int value);
/* destroy */
int virtual_machine_object_int_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);
/* clone */
struct virtual_machine_object *virtual_machine_object_int_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object);
/* print */
int virtual_machine_object_int_print(const struct virtual_machine_object *object);

/* get primitive value */
int virtual_machine_object_int_get_primitive_value(const struct virtual_machine_object *object);

/* set primitive value */
int virtual_machine_object_int_set_primitive_value(const struct virtual_machine_object *object, int value);

/* convert */
int virtual_machine_object_int_convert( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const uint32_t type);

/* add, sub, mul, div, mod */
/* lshift, rshift */
/* anda, ora, xora */
int virtual_machine_object_int_binary_arithmetic_shift_bitwise( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode);

/* eq, ne, l, g, le, ge */
int virtual_machine_object_int_binary_equality_relational( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode);

/* neg, nota */
int virtual_machine_object_int_unary( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const uint32_t opcode);

#endif

