/* Pair Objects
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

#ifndef _VM_OBJECT_PAIR_H_
#define _VM_OBJECT_PAIR_H_

#include <stdio.h>
#include <stdint.h>

#include "vm_infrastructure.h"
#include "vm_err.h"

struct virtual_machine_object;

/* Internal Part */

struct virtual_machine_object_pair_internal_node
{
    struct virtual_machine_object *ptr;
};

struct virtual_machine_object_pair_internal
{
    struct virtual_machine_object_pair_internal_node *car;
    struct virtual_machine_object_pair_internal_node *cdr;

    struct virtual_machine *vm;
};

/* Internal Marker */
int virtual_machine_object_pair_internal_marker(void *object_internal);
/* Internal Collector */
int virtual_machine_object_pair_internal_collector(void *object_internal, int *confirm);


/* Shell Part */

struct virtual_machine_object_pair
{
    /* Pointer to internal (the kernel part) */
    struct virtual_machine_object_pair_internal *ptr_internal;
};

struct virtual_machine_object *virtual_machine_object_pair_new(struct virtual_machine *vm);
int virtual_machine_object_pair_destroy(struct virtual_machine *vm, struct virtual_machine_object *object);
struct virtual_machine_object *virtual_machine_object_pair_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object);
int virtual_machine_object_pair_print(const struct virtual_machine_object *object);

struct virtual_machine_object *virtual_machine_object_pair_make(struct virtual_machine *vm, \
        struct virtual_machine_object *object_car, \
        struct virtual_machine_object *object_cdr);

int virtual_machine_object_pair_car( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src);
int virtual_machine_object_pair_cdr( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src);

int virtual_machine_object_pair_car_set( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        struct virtual_machine_object *object_src, \
        struct virtual_machine_object *object_member);
int virtual_machine_object_pair_cdr_set( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        struct virtual_machine_object *object_src, \
        struct virtual_machine_object *object_member);

#endif

