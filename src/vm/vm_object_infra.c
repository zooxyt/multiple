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

#include <stdlib.h>

#include "multiple_err.h"

#include "vm_infrastructure.h"
#include "vm_object_infra.h"


/* A Object is a container to hold different kinds of Objects */

/* Objects */

/* Create the basement (pure) object */
struct virtual_machine_object *_virtual_machine_object_new(struct virtual_machine *vm, \
        const uint32_t type)
{
    struct virtual_machine_object *new_object = NULL;

    if ((new_object = (struct virtual_machine_object *)virtual_machine_resource_malloc(\
                    vm->resource, \
                    sizeof(struct virtual_machine_object))) == NULL)
    { return NULL; }
    new_object->type = type;
    new_object->ptr = NULL;
    new_object->object_table_item_ptr = NULL;
    new_object->prev = new_object->next = NULL;

    return new_object;
}

int _virtual_machine_object_destroy(struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    virtual_machine_resource_free(vm->resource, ((void *)object));

    return 0;
}

int _virtual_machine_object_ptr_set(struct virtual_machine_object *object, const void *ptr)
{
    if ((object == NULL) || (ptr == NULL)) return -MULTIPLE_ERR_NULL_PTR;

    object->ptr = (void *)ptr;

    return 0;
}

