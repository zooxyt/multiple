/* Tuple Objects
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

#ifndef _VM_OBJECT_TUPLE_H_
#define _VM_OBJECT_TUPLE_H_

#include <stdio.h>

struct virtual_machine_object;

struct virtual_machine_object_tuple_node
{
    struct virtual_machine_object *ptr;
    struct virtual_machine_object_tuple_node *next;
};

struct virtual_machine_object_tuple
{
    struct virtual_machine_object_tuple_node *begin;
    struct virtual_machine_object_tuple_node *end;
    size_t size;
};

struct virtual_machine_object *virtual_machine_object_tuple_new( \
        struct virtual_machine *vm);
int virtual_machine_object_tuple_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);
struct virtual_machine_object *virtual_machine_object_tuple_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object);
int virtual_machine_object_tuple_print(const struct virtual_machine_object *object);

int virtual_machine_object_tuple_size( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src);

int virtual_machine_object_tuple_make(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_top, \
        const size_t count);
int virtual_machine_object_tuple_ref_get(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_idx);
int virtual_machine_object_tuple_ref_set(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_idx, \
        const struct virtual_machine_object *object_value);

#endif

