/* Boolean Objects
   Copyright(C) 2013 Cheryl Natsu

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

#ifndef _VM_OBJECT_BOOL_H_
#define _VM_OBJECT_BOOL_H_

#include <stdint.h>

#define VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_FALSE 0
#define VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE 1

#define TO_BOOL_VALUE(x) ((x)?VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE:VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_FALSE)


struct virtual_machine_object;

struct virtual_machine_object_bool
{
    int value;
};

struct virtual_machine_object *virtual_machine_object_bool_new_with_value( \
        struct virtual_machine *vm, \
        const int value);
int virtual_machine_object_bool_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);
struct virtual_machine_object *virtual_machine_object_bool_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object);
int virtual_machine_object_bool_print(const struct virtual_machine_object *object);

int virtual_machine_object_bool_valid(struct virtual_machine_object *object);
int virtual_machine_object_bool_get_value(const struct virtual_machine_object *object, int *value);

/* convert */
int virtual_machine_object_bool_convert( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, const struct virtual_machine_object *object_src, const uint32_t type);

/* andl, orl, xorl */
int virtual_machine_object_bool_binary_logical( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode);

/* eq, ne */
int virtual_machine_object_bool_binary_equality( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, const struct virtual_machine_object *object_left, \
        const struct virtual_machine_object *object_right, \
        const uint32_t opcode);

/* notl */
int virtual_machine_object_bool_unary( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const uint32_t opcode);

#endif

