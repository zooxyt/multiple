/* Object Types
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

#ifndef _VM_OBJECT_TYPE_H_
#define _VM_OBJECT_TYPE_H_

#include <stdio.h>
#include <stdint.h>

#include "vm_types.h"

struct virtual_machine_object;

struct virtual_machine_object_type
{
    uint32_t value;

    /* Addition information for raw */
    char *name;
    size_t len;
};

struct virtual_machine;

struct virtual_machine_object *virtual_machine_object_type_new_with_value( \
        struct virtual_machine *vm, \
        const uint32_t value);
struct virtual_machine_object *virtual_machine_object_type_from_object( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object);
int virtual_machine_object_type_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);
struct virtual_machine_object *virtual_machine_object_type_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object);
int virtual_machine_object_type_print(const struct virtual_machine_object *object);

struct virtual_machine;

int virtual_machine_object_type_convert( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, const uint32_t type);

/* eq, ne */
int virtual_machine_object_type_binary_equality( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode);

#endif

