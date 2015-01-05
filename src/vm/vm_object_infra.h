/* Infrastructure of Virtual Machine's Object 
   Copyright(C) 2013 Cheryl Natsu

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

#ifndef _VM_OBJECT_INFRA_H_
#define _VM_OBJECT_INFRA_H_

#include "selfcheck.h"

#include <stdint.h>

#include "vm_res.h"
#include "vm_types.h"
#include "gc.h"

/* Objects */

/* Object for contain data in computing stack */
struct virtual_machine_object
{
    enum virtual_machine_object_type_id type;
    void *ptr;

    struct gc_object_table_item *object_table_item_ptr;

    struct virtual_machine_object *prev;
    struct virtual_machine_object *next;
};

struct virtual_machine;

struct virtual_machine_object *_virtual_machine_object_new(struct virtual_machine *vm, const uint32_t type);
int _virtual_machine_object_destroy(struct virtual_machine *vm, const struct virtual_machine_object *object);
int _virtual_machine_object_ptr_set(struct virtual_machine_object *object, const void *ptr);

int _virtual_machine_object_add_reference(struct virtual_machine *vm, struct virtual_machine_object *object);

#endif

