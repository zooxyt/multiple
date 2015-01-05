/* Environment Entrance Objects
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

#ifndef _VM_OBJECT_ENV_ENT_H_
#define _VM_OBJECT_ENV_ENT_H_

#include <stdint.h>

struct virtual_machine_module;
struct virtual_machine_object;

/* Internal Part */

struct virtual_machine_object_environment_entrance_internal
{
    struct virtual_machine_environment_stack_frame *entrance;

    struct virtual_machine *vm;
};

/* Internal Marker */
int virtual_machine_object_environment_entrance_internal_marker(void *object_internal);
/* Internal Collector */
int virtual_machine_object_environment_entrance_internal_collector(void *object_internal, int *confirm);

/* Shell Part */

struct virtual_machine_object_environment_entrance
{
    struct virtual_machine_object_environment_entrance_internal *ptr_internal;
};

/* new */
struct virtual_machine_object *virtual_machine_object_environment_entrance_new( \
        struct virtual_machine *vm, \
        struct virtual_machine_environment_stack_frame *entrance);
/* destroy */
int virtual_machine_object_environment_entrance_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);
/* clone */
struct virtual_machine_object *virtual_machine_object_environment_entrance_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object);
/* print */
int virtual_machine_object_environment_entrance_print( \
        const struct virtual_machine_object *object);

/* Make a blank entrance, used in the bottom */
struct virtual_machine_object *virtual_machine_object_environment_entrance_make_blank( \
        struct virtual_machine *vm);

/* Make a linked entrance with the given previous environment */
struct virtual_machine_object *virtual_machine_object_environment_entrance_make_linked( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object_environment_entrance_prev);

int virtual_machine_object_environment_entrance_extract_environment_stack_frame( \
        struct virtual_machine_environment_stack_frame **environment_stack_frame_out, \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object_src);

#endif


