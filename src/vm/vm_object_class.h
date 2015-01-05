/* Class Instance Objects
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

#ifndef _VM_OBJECT_CLASS_H_
#define _VM_OBJECT_CLASS_H_

#include <stdio.h>
#include <stdint.h>

#include "vm_infrastructure.h"

struct virtual_machine_object;
struct virtual_machine_object_class;

struct virtual_machine_object_class_internal
{
    struct virtual_machine_data_type *data_type;
    struct virtual_machine *vm;

    /* Properties hold by class */
    struct virtual_machine_variable_list *properties;

    int destructed; /* Confirmed executed destructor */
    int destructing; /* Created destructing thread, not yet finished executing */
};

struct virtual_machine_object_class_internal *virtual_machine_object_class_internal_new( \
        struct virtual_machine *vm, \
        struct virtual_machine_data_type *data_type);
int virtual_machine_object_class_internal_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_class_internal *object_class_internal, int *confirm);
int virtual_machine_object_class_internal_properties_set( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_class_internal *object_class_internal, \
        uint32_t module_id, uint32_t id, \
        struct virtual_machine_object *value);
int virtual_machine_object_class_internal_properties_get( \
        struct virtual_machine *vm, 
        struct virtual_machine_object_class_internal *object_class_internal, \
        struct virtual_machine_object **object_value_out, \
        uint32_t module_id, uint32_t id);

/* Internal Marker */
int virtual_machine_object_class_internal_marker(void *object_internal);

/* Internal Collector */
int virtual_machine_object_class_internal_collector(void *object_internal, int *confirm);


struct virtual_machine_object_class
{
    /* Pointer to internal (the kernel part) */
    struct virtual_machine_object_class_internal *ptr_internal;
};

/* new */
struct virtual_machine_object *virtual_machine_object_class_new_with_value( \
        struct virtual_machine *vm, \
        struct virtual_machine_data_type *data_type);
/* destroy */
int virtual_machine_object_class_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);
/* clone */
struct virtual_machine_object *virtual_machine_object_class_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object);
/* print */
int virtual_machine_object_class_print(const struct virtual_machine_object *object);

/* Methods */
int virtual_machine_object_class_method(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_method, \
        const struct virtual_machine_object *object_args, unsigned int object_args_count, \
        struct virtual_machine *vm);

/* Methods for built-in types */
int virtual_machine_object_class_built_in_types_method(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_method, \
        const struct virtual_machine_object *object_args, unsigned int object_args_count, \
        struct virtual_machine *vm);

/* Confirm the execution of a destructor */
int virtual_machine_object_destructor_confirm(struct virtual_machine_object *object_this, \
        struct virtual_machine *vm);

/* Property */
int virtual_machine_object_class_property_get(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_property, \
        struct virtual_machine *vm);
int virtual_machine_object_class_property_set(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_property, \
        const struct virtual_machine_object *object_value, \
        struct virtual_machine *vm);

#endif

