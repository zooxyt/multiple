/* Object Types
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
#include <stdint.h>

#include "multiple_err.h"

#include "vm_opcode.h"
#include "vm_types.h"
#include "vm_object_aio.h"
#include "vm_err.h"

/* Create a new type object with specified value */
static struct virtual_machine_object *virtual_machine_object_type_new_with_value_and_raw_name( \
        struct virtual_machine *vm, const uint32_t value, char *raw_name, size_t raw_len)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_type *new_object_type = NULL;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_TYPE)) == NULL)
    {
        return NULL;
    }

    if ((new_object_type = (struct virtual_machine_object_type *)virtual_machine_resource_malloc_primitive( \
                    vm->resource, sizeof(struct virtual_machine_object_type))) == NULL)
    {
        goto fail;
    }
    new_object_type->name = NULL;
    new_object_type->len = 0;

    new_object_type->value = value;

    if (raw_name != NULL)
    {
        new_object_type->name = (char *)virtual_machine_resource_malloc_primitive( \
                vm->resource, sizeof(char) * (raw_len + 1));
        if (new_object_type->name == NULL)
        {
            goto fail;
        }
        memcpy(new_object_type->name, raw_name, raw_len);
        new_object_type->name[raw_len] = '\0';
        new_object_type->len = raw_len;
    }

    if (_virtual_machine_object_ptr_set(new_object, new_object_type) != 0)
    {
        goto fail;
    }
    new_object_type = NULL;

    return new_object;
fail:
    if (new_object_type != NULL)
    {
        if (new_object_type->name != NULL)
        {
            virtual_machine_resource_free_primitive(vm->resource, new_object_type->name);
        }
        virtual_machine_resource_free_primitive(vm->resource, new_object_type);
    }
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
    return NULL;
}

struct virtual_machine_object *virtual_machine_object_type_new_with_value(struct virtual_machine *vm, const uint32_t value)
{
    return virtual_machine_object_type_new_with_value_and_raw_name(vm, value, NULL, 0);
}

struct virtual_machine_object *virtual_machine_object_type_from_object( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_type *new_object_type = NULL;
    struct virtual_machine_object_raw *object_raw = NULL;
    struct virtual_machine_object_class *object_class = NULL;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_TYPE)) == NULL)
    { goto fail; }

    /* Create a new type object */
    if ((new_object_type = (struct virtual_machine_object_type *)virtual_machine_resource_malloc_primitive( \
                    vm->resource, sizeof(struct virtual_machine_object_type))) == NULL)
    { goto fail; }
    new_object_type->value = OBJECT_TYPE_UNKNOWN;
    new_object_type->name = NULL;
    new_object_type->len = 0;

    /* Create specified core part */
    if (object->type == OBJECT_TYPE_RAW)
    {
        object_raw = (struct virtual_machine_object_raw *)object->ptr;
        new_object_type->value = OBJECT_TYPE_RAW;
        new_object_type->len = object_raw->len; 
        new_object_type->name = (char *)virtual_machine_resource_malloc_primitive( \
                vm->resource, sizeof(char) * (new_object_type->len + 1));
        if (new_object_type->name == NULL) goto fail;
        memcpy(new_object_type->name, object_raw->name, new_object_type->len);
        new_object_type->name[new_object_type->len] = '\0';
        if (_virtual_machine_object_ptr_set(new_object, new_object_type) != 0)
        { goto fail; }
        new_object_type = NULL;
    }
    else if (object->type == OBJECT_TYPE_CLASS)
    {
        object_class = (struct virtual_machine_object_class *)object->ptr;
        new_object_type->value = OBJECT_TYPE_CLASS;
        new_object_type->len = object_class->ptr_internal->data_type->name_len; 
        new_object_type->name = (char *)virtual_machine_resource_malloc_primitive( \
                vm->resource, sizeof(char) * (new_object_type->len + 1));
        if (new_object_type->name == NULL) goto fail;
        memcpy(new_object_type->name, object_class->ptr_internal->data_type->name, new_object_type->len);
        new_object_type->name[new_object_type->len] = '\0';
        if (_virtual_machine_object_ptr_set(new_object, new_object_type) != 0)
        { goto fail; }
        new_object_type = NULL;
    }
    else
    {
        if (virtual_machine_object_id_to_type_name(NULL, NULL, object->type) == 0)
        {
            if ((new_object = virtual_machine_object_type_new_with_value(vm, object->type)) == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                goto fail;
            }
        }
        else
        {
            vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OBJECT, \
                    "runtime error: objects in type id \'%u\' don't support");
            goto fail;
        }
    }


    return new_object;
fail:
    if (new_object != NULL) _virtual_machine_object_destroy(vm, object);
    if (new_object_type != NULL)
    {
        if (new_object_type->name != NULL) virtual_machine_resource_free_primitive(vm->resource, new_object_type->name);
        virtual_machine_resource_free_primitive(vm->resource, new_object_type);
    }
    return NULL;
}

int virtual_machine_object_type_destroy(struct virtual_machine *vm, struct virtual_machine_object *object)
{
    struct virtual_machine_object_type *object_type = NULL;
    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_type = (struct virtual_machine_object_type *)object->ptr;
    if (object_type != NULL)
    {
        if (object_type->name != NULL) { virtual_machine_resource_free_primitive(vm->resource, object_type->name); }
        virtual_machine_resource_free_primitive(vm->resource, object_type);
    }
    _virtual_machine_object_destroy(vm, object);

    return 0;
}

struct virtual_machine_object *virtual_machine_object_type_clone(struct virtual_machine *vm, const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;

    if (object == NULL) return NULL;
    if (object->ptr == NULL) return NULL;
    if (((struct virtual_machine_object_type *)(object->ptr))->value == OBJECT_TYPE_RAW)
    {
        if ((new_object = virtual_machine_object_type_new_with_value_and_raw_name(vm, \
                        ((struct virtual_machine_object_type *)(object->ptr))->value,
                        ((struct virtual_machine_object_type *)(object->ptr))->name,
                        ((struct virtual_machine_object_type *)(object->ptr))->len
                        )) == NULL)
        {
            return NULL;
        }
    }
    else
    {
        if ((new_object = virtual_machine_object_type_new_with_value(vm, \
                        ((struct virtual_machine_object_type *)(object->ptr))->value)) == NULL)
        {
            return NULL;
        }
    }

    return new_object;
}

int virtual_machine_object_type_print(const struct virtual_machine_object *object)
{
    uint32_t value;
    struct virtual_machine_object_type *object_type = NULL;
    char *type_name;
    size_t type_name_len;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_type = (struct virtual_machine_object_type *)(object->ptr);
    value = object_type->value;
    if (value == OBJECT_TYPE_RAW)
    {
        fwrite(object_type->name, object_type->len, 1, stdout);
        return 0;
    }
    else
    {
        if (virtual_machine_object_id_to_type_name( \
                    &type_name, \
                    &type_name_len, \
                    value) == 0)
        {
            fwrite(type_name, type_name_len, 1, stdout);
            return 0;
        }
    }
    return -MULTIPLE_ERR_INTERNAL;
}

/* convert */
int virtual_machine_object_type_convert(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const uint32_t type)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_type *object_type = NULL;

    char *type_name;
    size_t type_len;

    (void)vm;

    if (object_src == NULL) return -MULTIPLE_ERR_NULL_PTR;

    *object_out = NULL;

    object_type = (struct virtual_machine_object_type *)object_src->ptr;

    switch (type)
    {
        case OBJECT_TYPE_STR:
            if (virtual_machine_object_id_to_type_name(&type_name, &type_len, object_type->value) != 0)
            {
                VM_ERR_INTERNAL(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail; 
            }
            if ((new_object = virtual_machine_object_str_new_with_value(vm, type_name, type_len)) == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            break;
    }
    if (new_object == NULL) 
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    *object_out = new_object;
    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
done:
    return ret;
}

/* eq, ne */
int virtual_machine_object_type_binary_equality(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    uint32_t value_left, value_right;

    (void)vm;

    if (object_left == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (object_right == NULL) return -MULTIPLE_ERR_NULL_PTR;

    *object_out = NULL;

    /* Type check */
    if (object_right->type != OBJECT_TYPE_TYPE)
    {
        switch (opcode)
        {
            case OP_EQ:
                new_object = virtual_machine_object_bool_new_with_value(vm, VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_FALSE);
                goto finish;
            case OP_NE:
                new_object = virtual_machine_object_bool_new_with_value(vm, VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE);
                goto finish;
        }
    }

    value_left = ((struct virtual_machine_object_type *)(object_left->ptr))->value;
    value_right = ((struct virtual_machine_object_type *)(object_right->ptr))->value;

    switch (opcode)
    {
        case OP_EQ:
            new_object = virtual_machine_object_bool_new_with_value(vm, TO_BOOL_VALUE(value_left == value_right));
            break;
        case OP_NE:
            new_object = virtual_machine_object_bool_new_with_value(vm, TO_BOOL_VALUE(value_left != value_right));
            break;
    }
finish:
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

