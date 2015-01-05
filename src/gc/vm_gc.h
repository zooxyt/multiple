/* Virtual Machine : Garbage Collection
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

#ifndef _VM_GC_H_
#define _VM_GC_H_

#include "vm_infrastructure.h"
#include "gc.h"

/* GC Type */

#define VIRTUAL_MACHINE_GARBAGE_COLLECT_MINOR 0
#define VIRTUAL_MACHINE_GARBAGE_COLLECT_MAJOR 1

#define VIRTUAL_MACHINE_SURVIVOR_THRESHOLD 10

/* Register Reference Type Object */
int virtual_machine_resource_reference_register( \
        gc_stub_t *gc_stub, \
        struct virtual_machine_object *object_src, \
        void *object_internal_ptr, \
        int (*internal_marker)(void *internal_object_ptr), \
        int (*internal_collector)(void *internal_object_ptr, int *confirm));
int virtual_machine_resource_reference_unregister( \
        gc_stub_t *gc_stub, \
        void *object_internal_ptr);

/* Do complete marking and sweeping */
int virtual_machine_garbage_collect(struct virtual_machine *vm);
int virtual_machine_garbage_collect_and_feedback(struct virtual_machine *vm);

/* Do minor GC
 * 1: Mark objects (only in youth age)
 * 2: Increase 'age' of the marked objects
 * 3: Sweep the rest objects
 * 4: Move objects which of threshold age to survivor items
 * 5: Maintain a table which record objects referenced by survivors
 */
int virtual_machine_garbage_collect_minor(struct virtual_machine *vm);

#endif

