/* Semaphore Objects
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

#ifndef _VM_OBJECT_SEMAPHORE_H_
#define _VM_OBJECT_SEMAPHORE_H_

#include <stdint.h>

struct virtual_machine_object;


struct virtual_machine_object_semaphore
{
    uint32_t semaphore_id;
};

/* new */
struct virtual_machine_object *virtual_machine_object_semaphore_new_with_semaphore_id( \
        struct virtual_machine *vm, \
        const uint32_t semaphore_id);
/* destroy */
int virtual_machine_object_semaphore_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);
/* clone */
struct virtual_machine_object *virtual_machine_object_semaphore_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object);
/* print */
int virtual_machine_object_semaphore_print( \
        const struct virtual_machine_object *object);

/* get id */
uint32_t virtual_machine_object_semaphore_get_id( \
        const struct virtual_machine_object *object);

/* value */
int virtual_machine_object_semaphore_value( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);
/* P (wait) */
int virtual_machine_object_semaphore_p( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);
/* try P (try_wait) 
 * Atomic try operation, if resource avaliable, 
 * perform P, or just return -1 */
int virtual_machine_object_semaphore_try_p( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);
/* V (signal) */
int virtual_machine_object_semaphore_v( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);

#endif


