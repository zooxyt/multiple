/* Char Types
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

#include "selfcheck.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "multiple_err.h"

#include "vm_opcode.h"
#include "vm_types.h"

#include "vm_infrastructure.h"
#include "vm_object_aio.h"
#include "vm_err.h"

#define PRINTABLE_CHAR_LOWER_BOUND 32
#define PRINTABLE_CHAR_HIGHER_BOUND 127

/* new */
struct virtual_machine_object *virtual_machine_object_char_new_with_value( \
        struct virtual_machine *vm, \
        const uint32_t value)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_char *new_object_char = NULL;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_CHAR)) == NULL)
    {
        return NULL;
    }

    if ((new_object_char = (struct virtual_machine_object_char *)virtual_machine_resource_malloc_primitive( \
                    vm->resource, sizeof(struct virtual_machine_object_char))) == NULL)
    {
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }

    if (_virtual_machine_object_ptr_set(new_object, new_object_char) != 0)
    {
        virtual_machine_resource_free_primitive(vm->resource, new_object_char);
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }
    new_object_char->value = value;
    return new_object;
}

/* destroy */
int virtual_machine_object_char_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (object->ptr != NULL) virtual_machine_resource_free_primitive(vm->resource, object->ptr);
    _virtual_machine_object_destroy(vm, object);

    return 0;
}

/* clone */
struct virtual_machine_object *virtual_machine_object_char_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;

    if ((object == NULL) || (object->ptr == NULL)) return NULL;

    if ((new_object = virtual_machine_object_char_new_with_value(vm, ((struct virtual_machine_object_char *)(object->ptr))->value)) == NULL)
    {
        return NULL;
    }

    return new_object;
}

/* print */
int virtual_machine_object_char_print(const struct virtual_machine_object *object)
{
    uint32_t value;

    if ((object == NULL) || (object->ptr == NULL)) return -MULTIPLE_ERR_NULL_PTR;

    value = ((struct virtual_machine_object_char *)(object->ptr))->value;

	if ((value >= PRINTABLE_CHAR_LOWER_BOUND) && (value <= PRINTABLE_CHAR_HIGHER_BOUND))
    {
        printf("%c", (char)value);
    }
    else
    {
        printf("\\%u", value);
    }

    return 0;
}

/* get primitive value */
uint32_t virtual_machine_object_char_get_primitive_value(const struct virtual_machine_object *object)
{
    return ((struct virtual_machine_object_char *)(object->ptr))->value;
}

/* set primitive value */
int virtual_machine_object_char_set_primitive_value(const struct virtual_machine_object *object, uint32_t value)
{
    struct virtual_machine_object_char *object_char;

    object_char = object->ptr;

    object_char->value = value; 
    return 0;
}

/* convert */
int virtual_machine_object_char_convert( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const uint32_t type)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;
    char new_str[6]; /* A char's maximum length */
    char *new_str_p;
    uint32_t value;

    *object_out = NULL;

    switch (type)
    {
        case OBJECT_TYPE_INT:
            value = (uint32_t)(((struct virtual_machine_object_char *)(object_src->ptr))->value);
            new_object = virtual_machine_object_int_new_with_value(
                    vm, (int)value);
            break;
        case OBJECT_TYPE_STR:
            value = (uint32_t)(((struct virtual_machine_object_char *)(object_src->ptr))->value);
            new_str_p = new_str;
            if (value <= 0x7F) *new_str_p++ = (char)(value);
            else if (value <= 0x7FF)
            {
                *new_str_p++ = (char)(0xc0 | ((value) >> 6));
                *new_str_p++ = (char)(0x80 | ((value) & 0x3f));
            }
            else if (value <= 0xFFFF)
            {
                *new_str_p++ = (char)(0xe0 | ((value) >> 12));
                *new_str_p++ = (char)(0x80 | (((value) >> 6) & 0x3f));
                *new_str_p++ = (char)(0x80 | ((value) & 0x3f));
            }
            else if (value <= 0x1FFFFF)
            {
                *new_str_p++ = (char)(0xf0 | ((value) >> 18));
                *new_str_p++ = (char)(0x80 | (((value) >> 12) & 0x3f));
                *new_str_p++ = (char)(0x80 | (((value) >> 6) & 0x3f));
                *new_str_p++ = (char)(0x80 | ((value) & 0x3f));
            }
            else if (value <= 0x3FFFFFF)
            {
                *new_str_p++ = (char)(0xf8 | ((value) >> 24));
                *new_str_p++ = (char)(0x80 | (((value) >> 18) & 0x3f));
                *new_str_p++ = (char)(0x80 | (((value) >> 12) & 0x3f));
                *new_str_p++ = (char)(0x80 | (((value) >> 6) & 0x3f));
                *new_str_p++ = (char)(0x80 | ((value) & 0x3f));
            }
            else if (value <= 0x7FFFFFFF)
            {
                *new_str_p++ = (char)(0xfc | ((value) >> 30));
                *new_str_p++ = (char)(0x80 | (((value) >> 24) & 0x3f));
                *new_str_p++ = (char)(0x80 | (((value) >> 18) & 0x3f));
                *new_str_p++ = (char)(0x80 | (((value) >> 12) & 0x3f));
                *new_str_p++ = (char)(0x80 | (((value) >> 6) & 0x3f));
                *new_str_p++ = (char)(0x80 | ((value) & 0x3f));
            }
            else
            {
                vm_err_update(vm->r, -VM_ERR_CONVERT_FAILED, \
                        "runtime error: unable to convert from \'char\' to \'str\'");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            new_object = virtual_machine_object_str_new_with_value(
                    vm, \
                    new_str, (size_t)(new_str_p - new_str));
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

/* eq, ne, l, g, le, ge */
int virtual_machine_object_char_binary_equality_relational( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    uint32_t value_left, value_right;

    (void)vm;

    if ((object_left == NULL) || (object_right == NULL))
    {
        return -MULTIPLE_ERR_NULL_PTR;
    }

    *object_out = NULL;

    /* Type check */
    if (object_right->type != OBJECT_TYPE_INT)
    {
        switch (opcode)
        {
            case OP_EQ:
                new_object = virtual_machine_object_bool_new_with_value(vm, VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_FALSE);
                goto finish;
                break;
            case OP_NE:
                new_object = virtual_machine_object_bool_new_with_value(vm, VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE);
                goto finish;
                break;
            default:
                VM_ERR_INTERNAL(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
        }
    }

    value_left = ((struct virtual_machine_object_char *)(object_left->ptr))->value;
    value_right = ((struct virtual_machine_object_char *)(object_right->ptr))->value;

    /* Perform operation */
    switch (opcode)
    {
        case OP_EQ: new_object = virtual_machine_object_bool_new_with_value(vm, TO_BOOL_VALUE(value_left == value_right));break;
        case OP_NE: new_object = virtual_machine_object_bool_new_with_value(vm, TO_BOOL_VALUE(value_left != value_right));break;
        case OP_L: new_object = virtual_machine_object_bool_new_with_value(vm, TO_BOOL_VALUE(value_left < value_right));break;
        case OP_G: new_object = virtual_machine_object_bool_new_with_value(vm, TO_BOOL_VALUE(value_left > value_right));break;
        case OP_LE: new_object = virtual_machine_object_bool_new_with_value(vm, TO_BOOL_VALUE(value_left <= value_right));break;
        case OP_GE: new_object = virtual_machine_object_bool_new_with_value(vm, TO_BOOL_VALUE(value_left >= value_right));break;
        default:
                      VM_ERR_INTERNAL(vm->r);
                      ret = -MULTIPLE_ERR_VM;
                      goto fail;
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

