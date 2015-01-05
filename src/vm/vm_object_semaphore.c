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

#include <stdlib.h>
#include <stdio.h>

#include "multiple_err.h"

#include "vm_opcode.h"
#include "vm_types.h"

#include "vm_infrastructure.h"
#include "vm_object_aio.h"
#include "vm_err.h"


/* new */
struct virtual_machine_object *virtual_machine_object_semaphore_new_with_semaphore_id( \
        struct virtual_machine *vm, \
        const uint32_t semaphore_id)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_semaphore *new_object_semaphore = NULL;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_SEMAPHORE)) == NULL)
    {
        return NULL;
    }

    if ((new_object_semaphore = (struct virtual_machine_object_semaphore *)virtual_machine_resource_malloc_primitive( \
                    vm->resource, sizeof(struct virtual_machine_object_semaphore))) == NULL)
    {
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }
    if (_virtual_machine_object_ptr_set(new_object, new_object_semaphore) != 0)
    {
        virtual_machine_resource_free_primitive(vm->resource, new_object_semaphore);
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }
    new_object_semaphore->semaphore_id = semaphore_id;

    return new_object;
}

/* destroy */
int virtual_machine_object_semaphore_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (object->ptr != NULL) virtual_machine_resource_free_primitive(vm->resource, object->ptr);
    _virtual_machine_object_destroy(vm, object);
    return 0;
}

/* clone */
struct virtual_machine_object *virtual_machine_object_semaphore_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;

    if (object == NULL) return NULL;
    if (object->ptr == NULL) return NULL;

    if ((new_object = virtual_machine_object_semaphore_new_with_semaphore_id( \
                    vm, \
                    ((struct virtual_machine_object_semaphore *)(object->ptr))->semaphore_id)) == NULL)
    {
        return NULL;
    }

    return new_object;
}

/* print */
int virtual_machine_object_semaphore_print(const struct virtual_machine_object *object)
{
    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (object->ptr == NULL) return -MULTIPLE_ERR_NULL_PTR;

    printf("%u", (unsigned int)(((struct virtual_machine_object_semaphore *)(object->ptr))->semaphore_id));

    return 0;
}

/* get id */
uint32_t virtual_machine_object_semaphore_get_id( \
        const struct virtual_machine_object *object)
{
    return ((struct virtual_machine_object_semaphore *)(object->ptr))->semaphore_id;
}

/* value */
int virtual_machine_object_semaphore_value( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    return virtual_machine_semaphore_list_value( \
            vm->semaphores, \
            ((struct virtual_machine_object_semaphore *)(object->ptr))->semaphore_id);
}

/* P (wait) */
/*int virtual_machine_object_semaphore_p( \*/
/*struct virtual_machine *vm, \*/
/*struct virtual_machine_object *object)*/
/*{*/
/*return virtual_machine_semaphore_list_p( \*/
/*vm, \*/
/*vm->semaphores, \*/
/*((struct virtual_machine_object_semaphore *)(object->ptr))->semaphore_id);*/
/*}*/

/* try P (try_wait) 
 * Atomic try operation, if resource avaliable, 
 * perform P, or just return -1 */
int virtual_machine_object_semaphore_try_p( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    return  virtual_machine_semaphore_list_try_p( \
            vm, \
            vm->tp, \
            vm->semaphores, \
            ((struct virtual_machine_object_semaphore *)(object->ptr))->semaphore_id);
}

/* V (signal) */
int virtual_machine_object_semaphore_v( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    return virtual_machine_semaphore_list_v( \
            vm, \
            vm->semaphores, \
            ((struct virtual_machine_object_semaphore *)(object->ptr))->semaphore_id);
}

