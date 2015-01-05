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

#include <stdlib.h>
#include <stdio.h>

#include "multiple_err.h"

#include "vm_opcode.h"
#include "vm_types.h"

#include "vm_infrastructure.h"
#include "vm_object_aio.h"
#include "vm_err.h"

/* new */
struct virtual_machine_object *virtual_machine_object_mutex_new_with_mutex_id( \
        struct virtual_machine *vm, \
        const uint32_t mutex_id)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_mutex *new_object_mutex = NULL;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_MUTEX)) == NULL)
    {
        return NULL;
    }

    if ((new_object_mutex = (struct virtual_machine_object_mutex *)virtual_machine_resource_malloc_primitive( \
                    vm->resource, sizeof(struct virtual_machine_object_mutex))) == NULL)
    {
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }
    if (_virtual_machine_object_ptr_set(new_object, new_object_mutex) != 0)
    {
        virtual_machine_resource_free_primitive(vm->resource, new_object_mutex);
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }
    new_object_mutex->mutex_id = mutex_id;

    return new_object;
}

/* destroy */
int virtual_machine_object_mutex_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (object->ptr != NULL) virtual_machine_resource_free_primitive(vm->resource, object->ptr);
    _virtual_machine_object_destroy(vm, object);
    return 0;
}

/* clone */
struct virtual_machine_object *virtual_machine_object_mutex_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;

    if (object == NULL) return NULL;
    if (object->ptr == NULL) return NULL;

    if ((new_object = virtual_machine_object_mutex_new_with_mutex_id( \
                    vm, \
                    ((struct virtual_machine_object_mutex *)(object->ptr))->mutex_id)) == NULL)
    {
        return NULL;
    }

    return new_object;
}

/* print */
int virtual_machine_object_mutex_print(const struct virtual_machine_object *object)
{
    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (object->ptr == NULL) return -MULTIPLE_ERR_NULL_PTR;

    printf("%u", (unsigned int)(((struct virtual_machine_object_mutex *)(object->ptr))->mutex_id));

    return 0;
}

/* state */
int virtual_machine_object_mutex_state( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    return virtual_machine_mutex_list_state_by_id( \
            vm->mutexes, \
            ((struct virtual_machine_object_mutex *)(object->ptr))->mutex_id);
}

/* lock */
int virtual_machine_object_mutex_lock( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    return virtual_machine_mutex_list_lock_by_id( \
            vm->mutexes, \
            ((struct virtual_machine_object_mutex *)(object->ptr))->mutex_id);
}

/* unlock */
int virtual_machine_object_mutex_unlock( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    return virtual_machine_mutex_list_unlock_by_id( \
            vm->mutexes, \
            ((struct virtual_machine_object_mutex *)(object->ptr))->mutex_id);
}

