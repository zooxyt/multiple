/* Mutex Objects
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

#ifndef _VM_OBJECT_MUTEX_H_
#define _VM_OBJECT_MUTEX_H_

#include <stdint.h>

struct virtual_machine_object;

struct virtual_machine_object_mutex
{
    uint32_t mutex_id;
};

/* new */
struct virtual_machine_object *virtual_machine_object_mutex_new_with_mutex_id( \
        struct virtual_machine *vm, \
        const uint32_t mutex_id);
/* destroy */
int virtual_machine_object_mutex_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);
/* clone */
struct virtual_machine_object *virtual_machine_object_mutex_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object);
/* print */
int virtual_machine_object_mutex_print( \
        const struct virtual_machine_object *object);

/* state */
int virtual_machine_object_mutex_state( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);
/* lock */
int virtual_machine_object_mutex_lock( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);
/* unlock */
int virtual_machine_object_mutex_unlock( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);

#endif

