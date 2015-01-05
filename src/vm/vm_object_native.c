/* Low-level Native Objects
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

/* Machine related Low-level data type, just an interface to C's data type
 * for providing interface to 'native' programming languages(C/C++?) and 
 * do low-level works such binary file operating */

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

/* Compatible */
#if defined(_MSC_VER)
#define snprintf _snprintf
#elif defined(WIN32)
#define snprintf _snprintf
#endif

/* Create a new native object with specified value */
struct virtual_machine_object *virtual_machine_object_int_new_with_value( \
        struct virtual_machine *vm, \
        const int value)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_int *new_object_int = NULL;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_INT)) == NULL)
    {
        return NULL;
    }

    if ((new_object_int = (struct virtual_machine_object_int *)virtual_machine_resource_malloc_primitive( \
                    vm->resource, sizeof(struct virtual_machine_object_int))) == NULL)
    {
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }

    if (_virtual_machine_object_ptr_set(new_object, new_object_int) != 0)
    {
        virtual_machine_resource_free_primitive(vm->resource, new_object_int);
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }
    new_object_int->value = value;
    return new_object;
}

/* Destroy a native object */
int virtual_machine_object_int_destroy(struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (object->ptr != NULL) virtual_machine_resource_free_primitive(vm->resource, object->ptr);
    _virtual_machine_object_destroy(vm, object);

    return 0;
}

/* Clone a native object */
struct virtual_machine_object *virtual_machine_object_int_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;

    if ((object == NULL) || (object->ptr == NULL)) return NULL;

    if ((new_object = virtual_machine_object_int_new_with_value(vm, ((struct virtual_machine_object_int *)(object->ptr))->value)) == NULL)
    {
        return NULL;
    }

    return new_object;
}

/* print */
int virtual_machine_object_int_print(const struct virtual_machine_object *object)
{
    if ((object == NULL) || (object->ptr == NULL)) return -MULTIPLE_ERR_NULL_PTR;

    printf("%d", ((struct virtual_machine_object_int *)(object->ptr))->value);

    return 0;
}

/* get primitive value */
int virtual_machine_object_int_get_primitive_value(const struct virtual_machine_object *object)
{
    struct virtual_machine_object_int *object_int;

    object_int = object->ptr;

    return object_int->value; 
}

/* set primitive value */
int virtual_machine_object_int_set_primitive_value(const struct virtual_machine_object *object, int value)
{
    struct virtual_machine_object_int *object_int;

    object_int = object->ptr;

    object_int->value = value; 
    return 0;
}


#define OP_LITERAL_LEN 32

/* add, sub, mul, div, mod */
/* lshift, rshift */
/* anda, ora, xora */
int virtual_machine_object_int_binary_arithmetic_shift_bitwise(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;
    char *bad_type_name;
    char op_literal[OP_LITERAL_LEN];

    int value_left, value_right;

    (void)vm;

    if ((object_left == NULL) || (object_right == NULL)) return -MULTIPLE_ERR_NULL_PTR;

    *object_out = NULL;

    /* Type check */
    if (object_right->type != OBJECT_TYPE_INT)
    {
        op_literal[0] = '\0';
        switch (opcode)
        {
            case OP_ADD: snprintf(op_literal, OP_LITERAL_LEN, "+"); break;
            case OP_SUB: snprintf(op_literal, OP_LITERAL_LEN, "-"); break;
            case OP_MUL: snprintf(op_literal, OP_LITERAL_LEN, "*"); break;
            case OP_DIV: snprintf(op_literal, OP_LITERAL_LEN, "/"); break;
            case OP_MOD: snprintf(op_literal, OP_LITERAL_LEN, "%%"); break;
            case OP_LSHIFT: snprintf(op_literal, OP_LITERAL_LEN, "<<"); break;
            case OP_RSHIFT: snprintf(op_literal, OP_LITERAL_LEN, ">>"); break;
            case OP_ANDA: snprintf(op_literal, OP_LITERAL_LEN, "bit::and"); break;
            case OP_ORA: snprintf(op_literal, OP_LITERAL_LEN, "bit::or"); break;
            case OP_XORA: snprintf(op_literal, OP_LITERAL_LEN, "bit::xor"); break;
            default: snprintf(op_literal, OP_LITERAL_LEN, "unknown op"); break;
        }
        ret = virtual_machine_object_id_to_type_name(&bad_type_name, NULL, object_right->type);
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type, " \
                "\'int\' %s \'%s\'",  \
                op_literal, ret == 0 ? bad_type_name : "undefined type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    if (opcode == OP_DIV || opcode == OP_MOD)
    {
        /* 0 can't be divided by */
        if (((struct virtual_machine_object_int *)(object_right->ptr))->value == 0)
        {
            vm_err_update(vm->r, -VM_ERR_DIVIDE_BY_ZERO, \
                    "runtime error: divide by zero");
            ret = -VM_ERR_DIVIDE_BY_ZERO; 
            goto fail; 
        }
    }

    value_left = ((struct virtual_machine_object_int *)(object_left->ptr))->value;
    value_right = ((struct virtual_machine_object_int *)(object_right->ptr))->value;

    /* Perform operation */
    switch (opcode)
    {
        case OP_ADD: new_object = virtual_machine_object_int_new_with_value(vm, value_left + value_right); break;
        case OP_SUB: new_object = virtual_machine_object_int_new_with_value(vm, value_left - value_right); break;
        case OP_MUL: new_object = virtual_machine_object_int_new_with_value(vm, value_left * value_right); break;
        case OP_DIV: new_object = virtual_machine_object_int_new_with_value(vm, value_left / value_right); break;
        case OP_MOD: new_object = virtual_machine_object_int_new_with_value(vm, value_left % value_right); break;
        case OP_LSHIFT: new_object = virtual_machine_object_int_new_with_value(vm, value_left << value_right); break;
        case OP_RSHIFT: new_object = virtual_machine_object_int_new_with_value(vm, value_left >> value_right); break;
        case OP_ANDA: new_object = virtual_machine_object_int_new_with_value(vm, value_left & value_right); break;
        case OP_ORA: new_object = virtual_machine_object_int_new_with_value(vm, value_left | value_right); break;
        case OP_XORA: new_object = virtual_machine_object_int_new_with_value(vm, value_left ^ value_right); break;
        default:
                      VM_ERR_INTERNAL(vm->r);
                      ret = -MULTIPLE_ERR_VM;
                      goto fail;
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
int virtual_machine_object_int_binary_equality_relational(struct virtual_machine *vm, struct virtual_machine_object **object_out, const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, const uint32_t opcode)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    int value_left, value_right;

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

    value_left = ((struct virtual_machine_object_int *)(object_left->ptr))->value;
    value_right = ((struct virtual_machine_object_int *)(object_right->ptr))->value;

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

/* neg, nota */
int virtual_machine_object_int_unary(struct virtual_machine *vm, struct virtual_machine_object **object_out, \
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
            new_object = virtual_machine_object_int_new_with_value( \
                    vm, \
                    -(((struct virtual_machine_object_int *)(object_src->ptr))->value));
            break;
        case OP_NOTA:
            new_object = virtual_machine_object_int_new_with_value(
                    vm, \
                    ~(((struct virtual_machine_object_int *)(object_src->ptr))->value));
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

#define CONVERTED_STR_LEN_MAX (50)
/* convert */
int virtual_machine_object_int_convert( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const uint32_t type)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    char str[CONVERTED_STR_LEN_MAX]; /* For store temp value of str */
    int str_len;

    (void)vm;

    if (object_src == NULL) return -MULTIPLE_ERR_NULL_PTR;

    *object_out = NULL;

    switch (type)
    {
        case OBJECT_TYPE_INT:
            new_object = virtual_machine_object_int_new_with_value(
                    vm, \
                    (((struct virtual_machine_object_int *)(object_src->ptr))->value));
            break;
        case OBJECT_TYPE_CHAR:
            new_object = virtual_machine_object_char_new_with_value(
                    vm, \
                    (uint32_t)(((struct virtual_machine_object_int *)(object_src->ptr))->value));
            break;
        case OBJECT_TYPE_FLOAT:
            new_object = virtual_machine_object_float_new_with_value(
                    vm, \
                    (double)(((struct virtual_machine_object_int *)(object_src->ptr))->value));
            break;
        case OBJECT_TYPE_BOOL:
            new_object = virtual_machine_object_bool_new_with_value( \
                    vm, \
                    (((struct virtual_machine_object_int *)(object_src->ptr))->value == 0) ? \
                    VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_FALSE : \
                    VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE);
            break;
        case OBJECT_TYPE_STR:
            str_len = sprintf(str, "%d", ((struct virtual_machine_object_int *)(object_src->ptr))->value);
            new_object = virtual_machine_object_str_new_with_value( \
                    vm, \
                    str, (size_t)str_len);
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

