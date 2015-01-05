/* Function Objects
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

#ifndef _VM_OBJECT_FUNC_H_
#define _VM_OBJECT_FUNC_H_

#include <stdio.h>
#include <stdint.h>

#include "vm_env.h"

struct virtual_machine_module;
struct virtual_machine_object;

struct virtual_machine_object_func_internal
{
	uint32_t pc;
	
	struct virtual_machine_module *module;

    /* 4 Different Types of Function used different part of the the struct
     * TODO: Apply with Union */

    /* External Function */
    int (*extern_func)(struct multiple_stub_function_args *args);
    struct multiple_stub_function_args *extern_func_args;

    /* Continuation */
    int cont;
    struct virtual_machine_object *environment;
    /* The smallest running stack size value since the 
     * continuation has been created */
    size_t turning_point; 
    /* for Turning point tracing */
    struct continuation_list_item *continuation_tracing_item;

    /* Environment */
    struct virtual_machine_object *environment_entrance;

    /* Promise */
    int promise;
    struct virtual_machine_object *cached_promise;

    struct virtual_machine *vm;
};

/* Internal Marker */
int virtual_machine_object_func_internal_marker(void *object_internal);
/* Internal Collector */
int virtual_machine_object_func_internal_collector(void *object_internal, int *confirm);

/* Shell Part */

struct virtual_machine_object_func
{
    /* Pointer to internal (the kernel part) */
    struct virtual_machine_object_func_internal *ptr_internal;
};

/* new */
struct virtual_machine_object *virtual_machine_object_func_new( \
        struct virtual_machine *vm);
/* destroy */
int virtual_machine_object_func_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);
/* clone */
struct virtual_machine_object *virtual_machine_object_func_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object);
/* print */
int virtual_machine_object_func_print(const struct virtual_machine_object *object);

/* value */
int virtual_machine_object_func_get_value_internal(const struct virtual_machine_object *object, \
        struct virtual_machine_module **module, \
        uint32_t *pc);
int virtual_machine_object_func_get_value_external(const struct virtual_machine_object *object, \
        int (**extern_func)(struct multiple_stub_function_args *args), \
        struct multiple_stub_function_args **extern_func_args);

/* new with */
struct virtual_machine_object *virtual_machine_object_func_make_internal( \
        struct virtual_machine_module *module, \
        uint32_t pc, \
        struct virtual_machine *vm);
struct virtual_machine_object *virtual_machine_object_func_make_external( \
        int (*extern_func)(struct multiple_stub_function_args *args), \
        struct multiple_stub_function_args *extern_func_args, \
        struct virtual_machine *vm);

int virtual_machine_object_func_make(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        struct virtual_machine *vm);

int virtual_machine_object_func_make_normal(struct virtual_machine_object **object_out, \
        const struct virtual_machine_module *module, \
        const uint32_t pc, \
        struct virtual_machine *vm);

int virtual_machine_object_func_make_lambda(struct virtual_machine_object **object_out, \
        const struct virtual_machine_module *module, \
        const uint32_t pc, \
        struct virtual_machine *vm);

int virtual_machine_object_func_make_promise(struct virtual_machine_object **object_out, \
        const struct virtual_machine_module *module, \
        const uint32_t pc, \
        struct virtual_machine *vm);

int virtual_machine_object_func_make_cont(struct virtual_machine_object **object_out, \
        const struct virtual_machine_module *module, \
        const uint32_t pc, \
        struct virtual_machine *vm);

struct virtual_machine_environment_stack *virtual_machine_object_func_extract_environment( \
        struct virtual_machine_object *object_src);

enum
{
    VIRTUAL_MACHINE_OBJECT_FUNC_TYPE_UNKNOWN = -1,
    VIRTUAL_MACHINE_OBJECT_FUNC_TYPE_NORMAL = 0,
    VIRTUAL_MACHINE_OBJECT_FUNC_TYPE_LAMBDA,
    VIRTUAL_MACHINE_OBJECT_FUNC_TYPE_PROMISE,
    VIRTUAL_MACHINE_OBJECT_FUNC_TYPE_CONT,
    VIRTUAL_MACHINE_OBJECT_FUNC_TYPE_EXTERNAL,
};
int virtual_machine_object_func_type( \
        struct virtual_machine_object *object_src);

#endif

