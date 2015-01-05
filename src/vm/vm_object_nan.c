/* Not a Number Objects
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

/* A Not a Number Object is used to represents the meaning of the invalid 
 * result of some calculation such a 1 / 0 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "multiple_err.h"

#include "vm_opcode.h"
#include "vm_types.h"
#include "vm_object_aio.h"
#include "vm_err.h"

struct virtual_machine_object *virtual_machine_object_nan_new( \
        struct virtual_machine *vm, int sign)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_nan *new_object_nan = NULL;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_NAN)) == NULL)
    {
        return NULL;
    }

    if ((new_object_nan = (struct virtual_machine_object_nan *)virtual_machine_resource_malloc_primitive( \
                    vm->resource, sizeof(struct virtual_machine_object_nan))) == NULL)
    {
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }
    new_object_nan->sign = sign;

    if (_virtual_machine_object_ptr_set(new_object, new_object_nan) != 0)
    {
        virtual_machine_resource_free_primitive(vm->resource, new_object_nan);
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }


    return new_object;
}

int virtual_machine_object_nan_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (object->ptr != NULL) virtual_machine_resource_free_primitive(vm->resource, object->ptr);
    _virtual_machine_object_destroy(vm, object);

    return 0;
}

struct virtual_machine_object *virtual_machine_object_nan_clone( \
        struct virtual_machine *vm, 
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_nan *object_nan = NULL;

    if (object == NULL) return NULL;
    if (object->ptr == NULL) return NULL;
    object_nan = object->ptr;
    if ((new_object = virtual_machine_object_nan_new(vm, object_nan->sign)) == NULL)
    {
        return NULL;
    }

    return new_object;
}

int virtual_machine_object_nan_print(const struct virtual_machine_object *object)
{
    struct virtual_machine_object_nan *object_nan;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_nan = object->ptr;
    if (object_nan->sign != 0) { fputc('-', stdout); }
    printf("nan");

    return 0;
}

/* eq, ne */
int virtual_machine_object_nan_binary(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    (void)vm;

    if (object_left == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (object_right == NULL) return -MULTIPLE_ERR_NULL_PTR;

    *object_out = NULL;

    /* Type check */
    if (object_right->type != OBJECT_TYPE_NAN)
    {
        switch (opcode)
        {
            case OP_EQ:
                new_object = virtual_machine_object_bool_new_with_value(vm, VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_FALSE);
                break;
            case OP_NE:
                new_object = virtual_machine_object_bool_new_with_value(vm, VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE);
                break;
        }
    }
    else
    {
        switch (opcode)
        {
            case OP_EQ:
                new_object = virtual_machine_object_bool_new_with_value(vm, VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE);
                break;
            case OP_NE:
                new_object = virtual_machine_object_bool_new_with_value(vm, VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_FALSE);
                break;
        }
    }

    if (new_object == NULL) 
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    *object_out = new_object;

fail:

    return ret;
}

/* neg */
int virtual_machine_object_nan_unary(struct virtual_machine *vm, struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const uint32_t opcode)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    (void)vm;

    if (object_src == NULL) return -MULTIPLE_ERR_NULL_PTR;

    *object_out = NULL;

    switch (opcode)
    {
        case OP_NEG:
            new_object = virtual_machine_object_nan_new( \
                    vm, \
                    (((struct virtual_machine_object_nan *)(object_src->ptr))->sign) == 0 ? 1 : 0);
            break;
    }
    if (new_object == NULL) 
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    *object_out = new_object;

fail:

    return ret;
}

