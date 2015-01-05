/* Boolean Objects
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

/* A boolean value is used to represent logical value such as 
 * 'true' or 'false' */

#include "selfcheck.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "multiple_err.h"

#include "vm_opcode.h"
#include "vm_types.h"
#include "vm_object_aio.h"
#include "vm_err.h"

/* Compatible */
#if defined(_MSC_VER)
#define snprintf _snprintf
#elif defined(WIN32)
#define snprintf _snprintf
#endif


/* Create a new bool object with specified value */
struct virtual_machine_object *virtual_machine_object_bool_new_with_value(struct virtual_machine *vm, const int value)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_bool *new_object_bool = NULL;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_BOOL)) == NULL)
    {
        return NULL;
    }

    if ((new_object_bool = (struct virtual_machine_object_bool *)virtual_machine_resource_malloc_primitive( \
                    vm->resource, sizeof(struct virtual_machine_object_bool))) == NULL)
    {
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }

    if (_virtual_machine_object_ptr_set(new_object, new_object_bool) != 0)
    {
        virtual_machine_resource_free_primitive(vm->resource, new_object_bool);
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }
    new_object_bool->value = value;
    return new_object;
}

int virtual_machine_object_bool_destroy(struct virtual_machine *vm, struct virtual_machine_object *object)
{
    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (object->ptr != NULL) virtual_machine_resource_free_primitive(vm->resource, object->ptr);
    _virtual_machine_object_destroy(vm, object);

    return 0;
}

struct virtual_machine_object *virtual_machine_object_bool_clone( \
        struct virtual_machine *vm, const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;

    if (object == NULL) return NULL;
    if (object->ptr == NULL) return NULL;
    if ((new_object = virtual_machine_object_bool_new_with_value(vm, ((struct virtual_machine_object_bool *)(object->ptr))->value)) == NULL)
    {
        return NULL;
    }

    return new_object;
}

int virtual_machine_object_bool_print(const struct virtual_machine_object *object)
{
    int value;
    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    value = ((struct virtual_machine_object_bool *)(object->ptr))->value;
    if (value == 0) printf("false");
    else printf("true");

    return 0;
}

int virtual_machine_object_bool_valid(struct virtual_machine_object *object)
{
    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (object->type != OBJECT_TYPE_BOOL)
    {
        return -VM_ERR_UNSUPPORTED_OBJECT;
    }
    return 0;
}

int virtual_machine_object_bool_get_value(const struct virtual_machine_object *object, int *value)
{
    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (value == NULL) return -MULTIPLE_ERR_NULL_PTR;

    *value = *((int *)(object->ptr));
    return 0;
}

#define CONVERTED_STR_LEN_MAX (50)
/* convert */
int virtual_machine_object_bool_convert(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const uint32_t type)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    char str[CONVERTED_STR_LEN_MAX]; /* For store temporary value of string */
    int str_len;

    (void)vm;

    if (object_src == NULL) return -MULTIPLE_ERR_NULL_PTR;

    *object_out = NULL;

    switch (type)
    {
        case OBJECT_TYPE_BOOL:
            new_object = virtual_machine_object_bool_new_with_value(
                    vm, \
                    (((struct virtual_machine_object_bool *)(object_src->ptr))->value));
            break;
        case OBJECT_TYPE_INT:
            new_object = virtual_machine_object_int_new_with_value( \
                    vm, \
                    (((struct virtual_machine_object_bool *)(object_src->ptr))->value == VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_FALSE) ? \
                    0 : 1);
            break;
        case OBJECT_TYPE_STR:
            if (((struct virtual_machine_object_bool *)(object_src->ptr))->value == VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_FALSE)
            { str_len = sprintf(str, "false"); }
            else
            { str_len = sprintf(str, "true"); }
            new_object = virtual_machine_object_str_new_with_value(vm, str, (size_t)str_len);
            break;
    }
    if (new_object == NULL) 
    {
        VM_ERR_MALLOC(vm->r);
        ret = MULTIPLE_ERR_VM;
        goto fail;
    }

    *object_out = new_object;

fail:

    return ret;
}

#define OP_LITERAL_LEN 32

/* andl, orl, xorl */
int virtual_machine_object_bool_binary_logical(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;
    char *bad_type_name;
    char op_literal[OP_LITERAL_LEN];

    (void)vm;

    if (object_left == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (object_right == NULL) return -MULTIPLE_ERR_NULL_PTR;

    *object_out = NULL;

    /* Bool orl Bool = Bool */
    if (object_right->type != OBJECT_TYPE_BOOL)
    {
        op_literal[0] = '\0';
        switch (opcode)
        {
            case OP_ANDA: snprintf(op_literal, OP_LITERAL_LEN, "bitop::and"); break;
            case OP_ORA: snprintf(op_literal, OP_LITERAL_LEN, "bitop::or"); break;
            case OP_XORA: snprintf(op_literal, OP_LITERAL_LEN, "bitop::xor"); break;
            default: snprintf(op_literal, OP_LITERAL_LEN, "unknown op"); break;
        }
        ret = virtual_machine_object_id_to_type_name(&bad_type_name, NULL, object_right->type);
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type, " \
                "\'bool\' %s \'%s\'",  \
                op_literal, ret == 0 ? bad_type_name : "undefined type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    switch (opcode)
    {
        case OP_ANDL:
            new_object = virtual_machine_object_bool_new_with_value(
                    vm, \
                    ((((struct virtual_machine_object_bool *)(object_left->ptr))->value != 0) &&
                     (((struct virtual_machine_object_bool *)(object_right->ptr))->value != 0)) ? 1 : 0);
            break;
        case OP_ORL:
            new_object = virtual_machine_object_bool_new_with_value(
                    vm, \
                    ((((struct virtual_machine_object_bool *)(object_left->ptr))->value != 0) || 
                     (((struct virtual_machine_object_bool *)(object_right->ptr))->value != 0)) ? 1 : 0);
            break;
        case OP_XORL:
            new_object = virtual_machine_object_bool_new_with_value(
                    vm, \
                    ((((struct virtual_machine_object_bool *)(object_left->ptr))->value != 0) != 
                     (((struct virtual_machine_object_bool *)(object_right->ptr))->value != 0)) ? 1 : 0);
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

/* eq, ne */
int virtual_machine_object_bool_binary_equality(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    int value_left, value_right;

    (void)vm;

    if (object_left == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (object_right == NULL) return -MULTIPLE_ERR_NULL_PTR;

    *object_out = NULL;

    /* Type check */
    if (object_right->type != OBJECT_TYPE_BOOL)
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

    value_left = ((struct virtual_machine_object_bool *)(object_left->ptr))->value;
    value_right = ((struct virtual_machine_object_bool *)(object_right->ptr))->value;

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

/* notl */
int virtual_machine_object_bool_unary(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
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
        case OP_NOTL:
            new_object = virtual_machine_object_bool_new_with_value(vm, \
                    ((struct virtual_machine_object_bool *)(object_src->ptr))->value == 0 ? 1 : 0);
            if (new_object == NULL) 
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            break;
    }

    *object_out = new_object;

fail:

    return ret;
}

