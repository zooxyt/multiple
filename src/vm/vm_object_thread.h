/* Thread Objects
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

#ifndef _VM_OBJECT_THREAD_H_
#define _VM_OBJECT_THREAD_H_

#include <stdint.h>

struct virtual_machine_object;

struct virtual_machine_object_thread
{
    uint32_t tid;
};

/* new */
struct virtual_machine_object *virtual_machine_object_thread_new_with_tid( \
        struct virtual_machine *vm, \
        const uint32_t tid);
/* destroy */
int virtual_machine_object_thread_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);
/* clone */
struct virtual_machine_object *virtual_machine_object_thread_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object);
/* print */
int virtual_machine_object_thread_print(const struct virtual_machine_object *object);


#endif

