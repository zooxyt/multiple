/* Raw Objects
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
#include <string.h>
#include <stdio.h>

#include "multiple_err.h"

#include "vm_opcode.h"
#include "vm_types.h"

#include "vm_infrastructure.h"
#include "vm_object_aio.h"
#include "vm_err.h"


/* Create a new raw object with specified value */
struct virtual_machine_object *virtual_machine_object_raw_new_with_value(struct virtual_machine *vm, const char *name, const size_t len, const void *ptr)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_raw *new_object_raw = NULL;
    char *new_name;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_RAW)) == NULL)
    { goto fail; }

    if ((new_object_raw = (struct virtual_machine_object_raw *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_raw))) == NULL)
    { goto fail; }
    new_object_raw->name = NULL;

    new_name = (char *)virtual_machine_resource_malloc(vm->resource, sizeof(char) * (len + 1));
    if (new_name== NULL) { goto fail; }

    memcpy(new_name, name, len);
    new_name[len] = '\0';

    new_object_raw->name = new_name; new_name = NULL;
    new_object_raw->ptr = (void *)ptr;
    new_object_raw->len = len;

    new_object_raw->func_destroy = NULL;
    new_object_raw->func_clone = NULL;
    new_object_raw->func_print = NULL;
    new_object_raw->func_eq = NULL;

    if (_virtual_machine_object_ptr_set(new_object, new_object_raw) != 0)
    { goto fail; }
    new_object_raw = NULL;

    return new_object;
fail:
    if (new_object_raw != NULL) 
    {
        if (new_object_raw->name != NULL) virtual_machine_resource_free(vm->resource, new_object_raw->name);
        virtual_machine_resource_free(vm->resource, new_object_raw);
    }
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);

    return NULL;
}

/* Destroy a raw object */
int virtual_machine_object_raw_destroy(struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    struct virtual_machine_object_raw *object_raw = NULL;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_raw = (struct virtual_machine_object_raw *)object->ptr;
    if (object_raw->func_destroy == NULL) return -MULTIPLE_ERR_NULL_PTR;
    object_raw->func_destroy(object_raw->ptr);
    if (object_raw->name != NULL) virtual_machine_resource_free(vm->resource, object_raw->name);
    virtual_machine_resource_free(vm->resource, object_raw);
    _virtual_machine_object_destroy(vm, object);

    return 0;
}

/* Clone a raw object */
struct virtual_machine_object *virtual_machine_object_raw_clone( \
        struct virtual_machine *vm, const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_raw *object_raw = NULL;
    struct virtual_machine_object_raw *new_object_raw = NULL;
    void *new_ptr = NULL;

    if ((object == NULL) || (object->ptr == NULL)) return NULL;

    object_raw = (struct virtual_machine_object_raw *)object->ptr;
    if (object_raw->func_destroy == NULL) return NULL;
    if ((new_ptr = object_raw->func_clone(object_raw->ptr)) == NULL) return NULL;
    new_object = virtual_machine_object_raw_new_with_value(vm, object_raw->name, object_raw->len, new_ptr);
    if (new_object == NULL) 
    {
        object_raw->func_destroy(object_raw->ptr);
        return NULL;
    }
    new_object_raw = (struct virtual_machine_object_raw *)new_object->ptr;

    new_object_raw->func_destroy = object_raw->func_destroy;
    new_object_raw->func_clone = object_raw->func_clone;
    new_object_raw->func_print = object_raw->func_print;
    new_object_raw->func_eq = object_raw->func_eq;

    return new_object;
}

/* print */
int virtual_machine_object_raw_print(const struct virtual_machine_object *object)
{
    struct virtual_machine_object_raw *object_raw = NULL;

    if ((object == NULL) || (object->ptr == NULL)) return -MULTIPLE_ERR_NULL_PTR;

    object_raw = (struct virtual_machine_object_raw *)object->ptr;
    object_raw->func_print(object_raw->ptr);

    return 0;
}

