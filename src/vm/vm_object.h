/* Virtual Machine Objects 
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


/* Object is a generic container for containing data */

#ifndef _VM_OBJECT_H_
#define _VM_OBJECT_H_

#include <stdint.h>

#include "vm_object_infra.h"
#include "vm.h"

/* Declarations */
struct virtual_machine_data_section_item;

/* new (from data section item) */
struct virtual_machine_object *virtual_machine_object_new_from_data_section_item(struct virtual_machine *vm, \
        const struct virtual_machine_data_section_item *item);

/* Generic "new_from_data_section_item" interface (symbol version) */
struct virtual_machine_object *virtual_machine_object_new_symbol_from_data_section_item(struct virtual_machine *vm, \
        const struct virtual_machine_data_section_item *item);

/* destroy */
int virtual_machine_object_destroy(struct virtual_machine *vm, \
        struct virtual_machine_object *object);
/* clone */
struct virtual_machine_object *virtual_machine_object_clone(struct virtual_machine *vm, \
        const struct virtual_machine_object *obj);
/* print */
int virtual_machine_object_print(const struct virtual_machine_object *object);

/* size */
int virtual_machine_object_size( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src);
/* type */
int virtual_machine_object_type( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src);

/* type upgrade */
int virtual_machine_object_type_upgrade( \
        struct virtual_machine_object **object_out_left, \
        struct virtual_machine_object **object_out_right, \
        const struct virtual_machine_object *object_src_left, \
        const struct virtual_machine_object *object_src_right, \
        struct virtual_machine *vm);

/* convert */
int virtual_machine_object_convert(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const uint32_t type, struct virtual_machine *vm);

/* property get */
int virtual_machine_object_property_get(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_property, \
        struct virtual_machine *vm);
/* property set */
int virtual_machine_object_property_set(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_property, \
        const struct virtual_machine_object *object_value, \
        struct virtual_machine *vm);

/* method */
int virtual_machine_object_method_invoke(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_method, \
        const struct virtual_machine_object *object_args, unsigned int object_args_count, \
        struct virtual_machine *vm);

/* unary operate */
int virtual_machine_object_unary_operate(struct virtual_machine_object **object_dst, \
        const struct virtual_machine_object *object_src, \
        const uint32_t opcode, struct virtual_machine *vm);

/* binary operate */
int virtual_machine_object_binary_operate(struct virtual_machine_object **object_dst, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode, struct virtual_machine *vm);

/* polynary operate */
int virtual_machine_object_polynary_operate(struct virtual_machine_object **object_dst, \
        struct virtual_machine_object *object_top, \
        uint32_t opcode, uint32_t operand, struct virtual_machine *vm);

#define OBJECTS_EQ 1
#define OBJECTS_NE 0

/* Return Value : 1 for eq, 0 for ne */
int objects_eq(const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right);

#endif

