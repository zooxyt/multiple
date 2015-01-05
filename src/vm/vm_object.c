/* Virtual Machine Objects 
   Copyright(C) 2013-2014 Cheryl Natsu

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

#include "selfcheck.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "multiple_ir.h"
#include "multiple_err.h"

#include "vm.h"
#include "vm_opcode.h"
#include "vm_types.h"
#include "vm_infrastructure.h"
#include "vm_object_aio.h"

#include "vm_err.h"

#include <float.h> /* double compare */

struct virtual_machine_object_general_interface
{
    struct virtual_machine_object *(*func_clone)(struct virtual_machine *vm, const struct virtual_machine_object *obj);
    int (*func_destroy)(struct virtual_machine *vm, struct virtual_machine_object *obj);
    int (*func_print)(const struct virtual_machine_object *obj);
};

#define DEF_INTERFACE(name) \
{ \
    &virtual_machine_object##name##clone, \
    &virtual_machine_object##name##destroy, \
    &virtual_machine_object##name##print, \
}

static struct virtual_machine_object_general_interface virtual_machine_object_general_interfaces[] =
{
    { NULL, NULL, NULL, }, /* Unknown */
    DEF_INTERFACE(_identifier_),
    DEF_INTERFACE(_int_),
    DEF_INTERFACE(_str_),
    DEF_INTERFACE(_bool_),
    DEF_INTERFACE(_none_),
    DEF_INTERFACE(_undef_),
    DEF_INTERFACE(_nan_),
    DEF_INTERFACE(_inf_),
    DEF_INTERFACE(_float_),
    DEF_INTERFACE(_rational_),
    DEF_INTERFACE(_complex_),
    DEF_INTERFACE(_type_),
    DEF_INTERFACE(_list_),
    DEF_INTERFACE(_array_),
    DEF_INTERFACE(_tuple_),
    DEF_INTERFACE(_hash_),
    DEF_INTERFACE(_thread_),
    DEF_INTERFACE(_mutex_),
    DEF_INTERFACE(_semaphore_),
    DEF_INTERFACE(_func_),
    DEF_INTERFACE(_raw_),
    DEF_INTERFACE(_class_),
    DEF_INTERFACE(_pair_),
    DEF_INTERFACE(_char_),
    DEF_INTERFACE(_symbol_),
    DEF_INTERFACE(_environment_),
    DEF_INTERFACE(_environment_entrance_),
};
#define VIRTUAL_MACHINE_OBJECT_GENERAL_INTERFACES_COUNT \
    sizeof(virtual_machine_object_general_interfaces)/sizeof(struct virtual_machine_object_general_interface)

/* Generic "clone" interface */
struct virtual_machine_object *virtual_machine_object_clone(struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;
    size_t type_idx;

    if (object == NULL)
    { VM_ERR_INTERNAL(vm->r); return NULL; }

    type_idx = (size_t)object->type;
    if ((type_idx == OBJECT_TYPE_UNKNOWN) && \
            (type_idx >= VIRTUAL_MACHINE_OBJECT_GENERAL_INTERFACES_COUNT))
    {
        VM_ERR_INTERNAL(vm->r);
        return NULL;
    }

    if ((new_object = virtual_machine_object_general_interfaces[type_idx].func_clone(vm, object)) == NULL)
    { goto fail; }
    new_object->object_table_item_ptr = object->object_table_item_ptr;

    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
    new_object = NULL;
done:
    return new_object;
}

/* Generic "new_from_data_section_item" interface */
struct virtual_machine_object *virtual_machine_object_new_from_data_section_item(struct virtual_machine *vm, \
        const struct virtual_machine_data_section_item *item)
{
    struct virtual_machine_object *new_object = NULL;

    if (item == NULL) return NULL;

    /* Create object body */
    switch (item->type)
    {
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER:
            new_object = virtual_machine_object_identifier_new_with_value(vm, \
                    item->ptr, (size_t)item->size, item->module->id, item->id, \
                    NULL, 0, 0, 0);
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INT:
            new_object = virtual_machine_object_int_new_with_value(vm, *((int *)item->ptr));
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_FLOAT:
            new_object = virtual_machine_object_float_new_with_value(vm, *((double *)item->ptr));
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_STR:
            new_object = virtual_machine_object_str_new_with_value(vm, item->ptr, (size_t)item->size);
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL:
            new_object = virtual_machine_object_bool_new_with_value(vm, *((int *)item->ptr));
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NONE:
            new_object = virtual_machine_object_none_new(vm);
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NAN:
            new_object = virtual_machine_object_nan_new(vm, *((int *)item->ptr));
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INF:
            new_object = virtual_machine_object_inf_new(vm, *((int *)item->ptr));
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_CHAR:
            new_object = virtual_machine_object_char_new_with_value(vm, *((uint32_t *)item->ptr));
            break;
        default:
            return NULL;
            break;
    }
    if (new_object == NULL) goto fail;
    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
    new_object = NULL;
done:
    return new_object;
}
/* Generic "new_from_data_section_item" interface (symbol version) */
struct virtual_machine_object *virtual_machine_object_new_symbol_from_data_section_item(struct virtual_machine *vm, \
        const struct virtual_machine_data_section_item *item)
{
    struct virtual_machine_object *new_object = NULL;

    if (item == NULL) return NULL;

    /* Create object body */
    switch (item->type)
    {
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER:
            new_object = virtual_machine_object_symbol_new_with_value(vm, \
                    item->ptr, (size_t)item->size);
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INT:
            new_object = virtual_machine_object_int_new_with_value(vm, *((int *)item->ptr));
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_CHAR:
            new_object = virtual_machine_object_char_new_with_value(vm, *((uint32_t *)item->ptr));
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_FLOAT:
            new_object = virtual_machine_object_float_new_with_value(vm, *((double *)item->ptr));
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_STR:
            new_object = virtual_machine_object_str_new_with_value(vm, \
				item->ptr, (size_t)item->size);
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL:
            new_object = virtual_machine_object_bool_new_with_value(vm, *((int *)item->ptr));
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NONE:
            new_object = virtual_machine_object_none_new(vm);
            break;
        default:
            return NULL;
            break;
    }
    if (new_object == NULL) goto fail;
    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
    new_object = NULL;
done:
    return new_object;
}

/* destroy */
int virtual_machine_object_destroy(struct virtual_machine *vm, struct virtual_machine_object *object)
{
    int ret = 0;
    size_t type_idx;

    if (object == NULL) 
    { VM_ERR_INTERNAL(vm->r); return -MULTIPLE_ERR_INTERNAL; }

    type_idx = (size_t)object->type;
    if ((type_idx == OBJECT_TYPE_UNKNOWN) && \
            (type_idx >= VIRTUAL_MACHINE_OBJECT_GENERAL_INTERFACES_COUNT))
    {
        VM_ERR_INTERNAL(vm->r);
        return -MULTIPLE_ERR_INTERNAL;
    }

    if ((ret = virtual_machine_object_general_interfaces[type_idx].func_destroy(vm, object)) != 0)
    { goto fail; }

fail:
    return ret;
}

/* print */
int virtual_machine_object_print(const struct virtual_machine_object *object)
{
    int ret = 0;

    size_t type_idx;

    type_idx = (size_t)object->type;
    if ((type_idx == OBJECT_TYPE_UNKNOWN) && \
            (type_idx >= VIRTUAL_MACHINE_OBJECT_GENERAL_INTERFACES_COUNT))
    {
        return -MULTIPLE_ERR_INTERNAL;
    }

    if ((ret = virtual_machine_object_general_interfaces[type_idx].func_print(object)) != 0)
    { goto fail; }

    fflush(stdout);

fail:
    return ret;
}

/* size */
int virtual_machine_object_size(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src)
{
    int ret = 0;
    char *type_name;

    if (object_src == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (object_src->type == OBJECT_TYPE_STR)
    {
        if ((ret = virtual_machine_object_str_size(vm, object_out, object_src)) != 0)
        { goto fail; }
    }
    else if (object_src->type == OBJECT_TYPE_LIST)
    {
        if ((ret = virtual_machine_object_list_size(vm, object_out, object_src)) != 0)
        { goto fail; }
    }
    else if (object_src->type == OBJECT_TYPE_ARRAY)
    {
        if ((ret = virtual_machine_object_array_size(vm, object_out, object_src)) != 0)
        { goto fail; }
    }
    else if (object_src->type == OBJECT_TYPE_TUPLE)
    {
        if ((ret = virtual_machine_object_tuple_size(vm, object_out, object_src)) != 0)
        { goto fail; }
    }
    else if (object_src->type == OBJECT_TYPE_HASH)
    {
        if ((ret = virtual_machine_object_hash_size(vm, object_out, object_src)) != 0)
        { goto fail; }
    }
    else
    {
        virtual_machine_object_id_to_type_name(&type_name, NULL, object_src->type);
        if (type_name != NULL)
        {
            vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OBJECT, \
                    "runtime error: objects in type \'%s\' don't have size attribute", \
                    type_name);
        }
        else
        {
            vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OBJECT, \
                    "runtime error: objects in type id \'%s\' don't have size attribute", \
                    object_src->type);
        }
        ret = -MULTIPLE_ERR_VM;
    }

fail:
    return ret;
}

/* type */
int virtual_machine_object_type(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    if ((new_object = virtual_machine_object_type_from_object( \
                    vm, \
                    object_src)) == NULL)
    { goto fail; }
    /*virtual_machine_object *object);*/


    /*switch (object_src->type)*/
    /*{*/
    /*case OBJECT_TYPE_RAW: */
    /*if ((new_object = virtual_machine_object_type_raw_new_with_value(vm, object_src)) == NULL)*/
    /*{*/
    /*VM_ERR_MALLOC(vm->r);*/
    /*ret = -MULTIPLE_ERR_VM;*/
    /*goto fail;*/
    /*}*/
    /*break;*/
    /*default:*/
    /*if (virtual_machine_object_id_to_type_name(NULL, NULL, object_src->type) == 0)*/
    /*{*/
    /*if ((new_object = virtual_machine_object_type_new_with_value(vm, object_src->type)) == NULL)*/
    /*{*/
    /*VM_ERR_MALLOC(vm->r);*/
    /*ret = -MULTIPLE_ERR_VM;*/
    /*goto fail;*/
    /*}*/
    /*}*/
    /*else*/
    /*{*/
    /*vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OBJECT, \*/
    /*"runtime error: objects in type id \'%u\' don't support");*/
    /*ret = -MULTIPLE_ERR_VM;*/
    /*}*/
    /*break;*/
    /*}*/
    *object_out = new_object;
    goto done;
fail:
    *object_out = NULL;
done:
    return ret;
}

/* property method */
int virtual_machine_object_method_invoke(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_method, \
        const struct virtual_machine_object *object_args, unsigned int object_args_count, \
        struct virtual_machine *vm)
{
    int ret = 0;
    struct virtual_machine_object *new_object_src = NULL;
    /*char *type_name = NULL;*/

    if (object_src == NULL)
    {
        VM_ERR_NULL_PTR(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    if ((ret = virtual_machine_variable_solve(&new_object_src, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }

    if (new_object_src->type == OBJECT_TYPE_CLASS)
    {
        if ((ret = virtual_machine_object_class_method(object_out, \
                        new_object_src, \
                        object_method, object_args, object_args_count, \
                        vm)) != 0)
        { goto fail; }
    }
    else
    {
        if ((ret = virtual_machine_object_class_built_in_types_method(object_out, \
                new_object_src, \
                object_method, object_args, object_args_count, \
                vm)) != 0)
        { goto fail; }
        /*break;*/
        /*case OBJECT_TYPE_CLASS:*/
        /*break;*/
        /*default:*/
        /*virtual_machine_object_id_to_type_name(&type_name, NULL, new_object_src->type);*/
        /*if (type_name != NULL)*/
        /*{*/
        /*vm_err_update(vm->r, -VM_ERR_NO_METHOD, \*/
        /*"runtime error: objects in type \'%s\' doesn't have methods", type_name);*/
        /*}*/
        /*else*/
        /*{*/
        /*vm_err_update(vm->r, -VM_ERR_NO_METHOD, \*/
        /*"runtime error: objects in type id \'%d\' doesn't have methods", new_object_src->type);*/
        /*}*/
        /*ret = -MULTIPLE_ERR_VM;*/
        /*goto fail;*/
        /*break;*/
    }

fail:
    if (new_object_src != NULL) virtual_machine_object_destroy(vm, new_object_src);

    return ret;
}

/* property get */
int virtual_machine_object_property_get(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_property, \
        struct virtual_machine *vm)
{
    int ret = 0;
    struct virtual_machine_object *new_object_src = NULL;
    char *type_name;

    if (object_src == NULL)
    {
        VM_ERR_NULL_PTR(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    if ((ret = virtual_machine_variable_solve(&new_object_src, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }

    if (new_object_src->type == OBJECT_TYPE_CLASS)
    {
        if ((ret = virtual_machine_object_class_property_get(object_out, \
                        new_object_src, object_property, \
                        vm)) != 0)
        { goto fail; }
    }
    else
    {
        virtual_machine_object_id_to_type_name(&type_name, NULL, new_object_src->type);
        if (type_name != NULL)
        {
            vm_err_update(vm->r, -VM_ERR_NO_PROPERTY, \
                    "runtime error: objects in type \'%s\' don't have properties", type_name);
        }
        else
        {
            vm_err_update(vm->r, -VM_ERR_NO_PROPERTY, \
                    "runtime error: objects in type id \'%d\' don't have properties", new_object_src->type);
        }
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
fail:
    if (new_object_src != NULL) virtual_machine_object_destroy(vm, new_object_src);

    return ret;
}

int virtual_machine_object_property_set(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_property, \
        const struct virtual_machine_object *object_value, \
        struct virtual_machine *vm)
{
    int ret = 0;
    struct virtual_machine_object *new_object_src = NULL;
    struct virtual_machine_object *new_object_result = NULL;
    char *type_name = NULL;

    if (object_src == NULL)
    {
        VM_ERR_NULL_PTR(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    if ((ret = virtual_machine_variable_solve(&new_object_src, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }

    if (new_object_src->type == OBJECT_TYPE_CLASS)
    {
        if ((ret = virtual_machine_object_class_property_set(&new_object_result, \
                        new_object_src, \
                        object_property, object_value, \
                        vm)) != 0)
        { goto fail; }
    }
    else
    {
        virtual_machine_object_id_to_type_name(&type_name, NULL, new_object_src->type);
        if (type_name != NULL)
        {
            vm_err_update(vm->r, -VM_ERR_NO_METHOD, \
                    "runtime error: object in type \'%s\' doesn't have methods", type_name);
        }
        else
        {
            vm_err_update(vm->r, -VM_ERR_NO_METHOD, \
                    "runtime error: objectin type id \'%d\' doesn't have methods", new_object_src->type);
        }
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    *object_out = new_object_result;
    new_object_result = NULL;
    goto done;
fail:
done:
    if (new_object_src != NULL) virtual_machine_object_destroy(vm, new_object_src);
    if (new_object_result != NULL) virtual_machine_object_destroy(vm, new_object_result);

    return ret;
}

struct virtual_machine_opcode_to_unary_func_tbl_item
{
    const uint32_t opcode;
    const uint32_t object_src_type;
    const int solve_identifier_src;
    int (*virtual_machine_object_func)( \
            struct virtual_machine *vm, \
            struct virtual_machine_object **object_dst, \
            const struct virtual_machine_object *object_src, \
            const uint32_t opcode);
};

static struct virtual_machine_opcode_to_unary_func_tbl_item virtual_machine_opcode_to_unary_func_tbl_item[] =
{
    {OP_NEG, OBJECT_TYPE_FLOAT, 1, &virtual_machine_object_float_unary},
    {OP_NEG, OBJECT_TYPE_INT, 1, &virtual_machine_object_int_unary},
    {OP_NEG, OBJECT_TYPE_NAN, 1, &virtual_machine_object_nan_unary},
    {OP_NEG, OBJECT_TYPE_INF, 1, &virtual_machine_object_inf_unary},
    {OP_NOTA, OBJECT_TYPE_INT, 1, &virtual_machine_object_int_unary},
    {OP_NOTL, OBJECT_TYPE_BOOL, 1, &virtual_machine_object_bool_unary},
};

#define VIRTUAL_MACHINE_OPCODE_TO_UNARY_FUNC_TBL_SIZE (sizeof(virtual_machine_opcode_to_unary_func_tbl_item)/sizeof(struct virtual_machine_opcode_to_unary_func_tbl_item))

/* unary operate */
int virtual_machine_object_unary_operate(struct virtual_machine_object **object_dst, \
        const struct virtual_machine_object *object_src, \
        const uint32_t opcode, struct virtual_machine *vm)
{
    int ret = 0;
    int opcode_exists = 0;
    unsigned int count = VIRTUAL_MACHINE_OPCODE_TO_UNARY_FUNC_TBL_SIZE;
    struct virtual_machine_opcode_to_unary_func_tbl_item *item_cur = virtual_machine_opcode_to_unary_func_tbl_item;
    struct virtual_machine_object *new_object_src[2] = {NULL, NULL};
    char *type_name;
    char *instrument;

    new_object_src[0] = (struct virtual_machine_object *)object_src;
    if ((ret = virtual_machine_variable_solve(&new_object_src[1], (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }

    while (count-- != 0)
    {
        if (opcode == item_cur->opcode) opcode_exists = 1;
        if (opcode == item_cur->opcode && new_object_src[item_cur->solve_identifier_src]->type == item_cur->object_src_type)
        {
            ret = (item_cur->virtual_machine_object_func)(vm, object_dst, new_object_src[item_cur->solve_identifier_src], opcode);
            goto done;
        }
        item_cur++;
    }

    if (opcode_exists == 0) 
    {
        VM_ERR_INTERNAL(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    else 
    {
        virtual_machine_object_id_to_type_name(&type_name, NULL, new_object_src[1]->type);
        virtual_machine_opcode_to_instrument(&instrument, NULL, opcode);
        if (type_name != NULL)
        {
            vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                    "runtime error: objects in type \'%s\' don't match instrument \'%s\'", \
                    type_name, instrument);
        }
        else
        {
            vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                    "runtime error: objects in type id \'%u\' don't match instrument \'%s\'", \
                    new_object_src[1]->type, instrument);
        }
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

fail:
done:
    if (new_object_src[1] != NULL) virtual_machine_object_destroy(vm, new_object_src[1]);

    return ret;
}

struct virtual_machine_opcode_to_binary_func_tbl_item
{
    const uint32_t opcode;
    const uint32_t object_left_type;
    const int solve_identifier_left;
    const int solve_identifier_right;
    int (*virtual_machine_object_func)( \
            struct virtual_machine *vm, \
            struct virtual_machine_object **object_dst, \
            const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
            const uint32_t opcode);
};

static int virtual_machine_object_generic_equality(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode);

static struct virtual_machine_opcode_to_binary_func_tbl_item virtual_machine_opcode_to_binary_func_tbl_item[] =
{
    {OP_ADD, OBJECT_TYPE_INT, 1, 1, &virtual_machine_object_int_binary_arithmetic_shift_bitwise},
    {OP_ADD, OBJECT_TYPE_STR, 1, 1, &virtual_machine_object_str_add},
    {OP_ADD, OBJECT_TYPE_FLOAT, 1, 1, &virtual_machine_object_float_binary_arithmetic},

    {OP_SUB, OBJECT_TYPE_INT, 1, 1, &virtual_machine_object_int_binary_arithmetic_shift_bitwise},
    {OP_SUB, OBJECT_TYPE_FLOAT, 1, 1, &virtual_machine_object_float_binary_arithmetic},

    {OP_MUL, OBJECT_TYPE_INT, 1, 1, &virtual_machine_object_int_binary_arithmetic_shift_bitwise},
    {OP_MUL, OBJECT_TYPE_STR, 1, 1, &virtual_machine_object_str_mul},
    {OP_MUL, OBJECT_TYPE_FLOAT, 1, 1, &virtual_machine_object_float_binary_arithmetic},

    {OP_DIV, OBJECT_TYPE_INT, 1, 1, &virtual_machine_object_int_binary_arithmetic_shift_bitwise},
    {OP_DIV, OBJECT_TYPE_FLOAT, 1, 1, &virtual_machine_object_float_binary_arithmetic},

    {OP_MOD, OBJECT_TYPE_INT, 1, 1, &virtual_machine_object_int_binary_arithmetic_shift_bitwise},

    {OP_LSHIFT, OBJECT_TYPE_INT, 1, 1, &virtual_machine_object_int_binary_arithmetic_shift_bitwise},

    {OP_RSHIFT, OBJECT_TYPE_INT, 1, 1, &virtual_machine_object_int_binary_arithmetic_shift_bitwise},

    {OP_ANDA, OBJECT_TYPE_INT, 1, 1, &virtual_machine_object_int_binary_arithmetic_shift_bitwise},
    {OP_ORA, OBJECT_TYPE_INT, 1, 1, &virtual_machine_object_int_binary_arithmetic_shift_bitwise},
    {OP_XORA, OBJECT_TYPE_INT, 1, 1, &virtual_machine_object_int_binary_arithmetic_shift_bitwise},

    {OP_ANDL, OBJECT_TYPE_BOOL, 1, 1, &virtual_machine_object_bool_binary_logical},
    {OP_ORL, OBJECT_TYPE_BOOL, 1, 1, &virtual_machine_object_bool_binary_logical},
    {OP_XORL, OBJECT_TYPE_BOOL, 1, 1, &virtual_machine_object_bool_binary_logical},

    {OP_EQ, OBJECT_TYPE_INT, 1, 1, &virtual_machine_object_int_binary_equality_relational},
    {OP_EQ, OBJECT_TYPE_NONE, 1, 1, &virtual_machine_object_none_binary},
    {OP_EQ, OBJECT_TYPE_NAN, 1, 1, &virtual_machine_object_nan_binary},
    {OP_EQ, OBJECT_TYPE_INF, 1, 1, &virtual_machine_object_inf_binary},
    {OP_EQ, OBJECT_TYPE_BOOL, 1, 1, &virtual_machine_object_bool_binary_equality},
    {OP_EQ, OBJECT_TYPE_STR, 1, 1, &virtual_machine_object_str_binary_equality},
    {OP_EQ, OBJECT_TYPE_FLOAT, 1, 1, & virtual_machine_object_float_binary_equality_relational},
    {OP_EQ, OBJECT_TYPE_TYPE, 1, 1, &virtual_machine_object_type_binary_equality},
    {OP_EQ, OBJECT_TYPE_THREAD, 1, 1, &virtual_machine_object_generic_equality},
    {OP_EQ, OBJECT_TYPE_LIST, 1, 1, &virtual_machine_object_generic_equality},
    {OP_EQ, OBJECT_TYPE_TUPLE, 1, 1, &virtual_machine_object_generic_equality},
    {OP_EQ, OBJECT_TYPE_HASH, 1, 1, &virtual_machine_object_generic_equality},
    {OP_EQ, OBJECT_TYPE_FUNCTION, 1, 1, &virtual_machine_object_generic_equality},
    {OP_EQ, OBJECT_TYPE_RAW, 1, 1, &virtual_machine_object_generic_equality},
    {OP_EQ, OBJECT_TYPE_CLASS, 1, 1, &virtual_machine_object_generic_equality},

    {OP_NE, OBJECT_TYPE_INT, 1, 1, &virtual_machine_object_int_binary_equality_relational},
    {OP_NE, OBJECT_TYPE_NONE, 1, 1, &virtual_machine_object_none_binary},
    {OP_NE, OBJECT_TYPE_NAN, 1, 1, &virtual_machine_object_nan_binary},
    {OP_NE, OBJECT_TYPE_INF, 1, 1, &virtual_machine_object_inf_binary},
    {OP_NE, OBJECT_TYPE_BOOL, 1, 1, &virtual_machine_object_bool_binary_equality},
    {OP_NE, OBJECT_TYPE_STR, 1, 1, &virtual_machine_object_str_binary_equality},
    {OP_NE, OBJECT_TYPE_FLOAT, 1, 1, & virtual_machine_object_float_binary_equality_relational},
    {OP_NE, OBJECT_TYPE_TYPE, 1, 1, &virtual_machine_object_type_binary_equality},
    {OP_NE, OBJECT_TYPE_THREAD, 1, 1, &virtual_machine_object_generic_equality},
    {OP_NE, OBJECT_TYPE_LIST, 1, 1, &virtual_machine_object_generic_equality},
    {OP_NE, OBJECT_TYPE_TUPLE, 1, 1, &virtual_machine_object_generic_equality},
    {OP_NE, OBJECT_TYPE_HASH, 1, 1, &virtual_machine_object_generic_equality},
    {OP_NE, OBJECT_TYPE_FUNCTION, 1, 1, &virtual_machine_object_generic_equality},
    {OP_NE, OBJECT_TYPE_RAW, 1, 1, &virtual_machine_object_generic_equality},
    {OP_NE, OBJECT_TYPE_CLASS, 1, 1, &virtual_machine_object_generic_equality},

    {OP_L, OBJECT_TYPE_INT, 1, 1, &virtual_machine_object_int_binary_equality_relational},
    {OP_G, OBJECT_TYPE_INT, 1, 1, &virtual_machine_object_int_binary_equality_relational},
    {OP_LE, OBJECT_TYPE_INT, 1, 1, &virtual_machine_object_int_binary_equality_relational},
    {OP_GE, OBJECT_TYPE_INT, 1, 1, &virtual_machine_object_int_binary_equality_relational},
    {OP_L, OBJECT_TYPE_CHAR, 1, 1, &virtual_machine_object_char_binary_equality_relational},
    {OP_G, OBJECT_TYPE_CHAR, 1, 1, &virtual_machine_object_char_binary_equality_relational},
    {OP_LE, OBJECT_TYPE_CHAR, 1, 1, &virtual_machine_object_char_binary_equality_relational},
    {OP_GE, OBJECT_TYPE_CHAR, 1, 1, &virtual_machine_object_char_binary_equality_relational},
    {OP_L, OBJECT_TYPE_FLOAT, 1, 1, &virtual_machine_object_float_binary_equality_relational},
    {OP_G, OBJECT_TYPE_FLOAT, 1, 1, &virtual_machine_object_float_binary_equality_relational},
    {OP_LE, OBJECT_TYPE_FLOAT, 1, 1, &virtual_machine_object_float_binary_equality_relational},
    {OP_GE, OBJECT_TYPE_FLOAT, 1, 1, &virtual_machine_object_float_binary_equality_relational},
};

#define VIRTUAL_MACHINE_OPCODE_TO_BINARY_FUNC_TBL_SIZE (sizeof(virtual_machine_opcode_to_binary_func_tbl_item)/sizeof(struct virtual_machine_opcode_to_binary_func_tbl_item))

/* binary operate */
int virtual_machine_object_binary_operate(struct virtual_machine_object **object_dst, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode, \
        struct virtual_machine *vm)
{
    int ret = 0;
    int opcode_exists = 0;
    unsigned int count = VIRTUAL_MACHINE_OPCODE_TO_BINARY_FUNC_TBL_SIZE;
    struct virtual_machine_opcode_to_binary_func_tbl_item *item_cur = virtual_machine_opcode_to_binary_func_tbl_item;
    struct virtual_machine_object *new_object_left[2] = {NULL, NULL}, *new_object_right[2] = {NULL, NULL};
    char *type_name_left, *type_name_right;
    char *instrument;

    new_object_left[0] = (struct virtual_machine_object *)object_left;
    new_object_right[0] = (struct virtual_machine_object *)object_right;

    if ((ret = virtual_machine_variable_solve(&new_object_left[1], (struct virtual_machine_object *)object_left, NULL, 1, vm)) != 0)
    { goto fail; }
    if ((ret = virtual_machine_variable_solve(&new_object_right[1], (struct virtual_machine_object *)object_right, NULL, 1, vm)) != 0)
    { goto fail; }

    while (count-- != 0)
    {
        if (opcode == item_cur->opcode) opcode_exists = 1;
        if (opcode == item_cur->opcode && new_object_left[item_cur->solve_identifier_left]->type == item_cur->object_left_type)
        {
            ret = (item_cur->virtual_machine_object_func)(
                    vm, \
                    object_dst, \
                    new_object_left[item_cur->solve_identifier_left], \
                    new_object_right[item_cur->solve_identifier_right], \
                    opcode);
            goto done;
        }
        item_cur++;
    }
    if (opcode_exists == 0) 
    {
        VM_ERR_INTERNAL(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    else 
    {
        virtual_machine_object_id_to_type_name(&type_name_left, NULL, new_object_left[1]->type);
        virtual_machine_object_id_to_type_name(&type_name_right, NULL, new_object_right[1]->type);
        virtual_machine_opcode_to_instrument(&instrument, NULL, opcode);
        if ((type_name_left != NULL)
                && (type_name_right != NULL))
        {
            vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                    "runtime error: objects in type \'%s\' and \'%s\' don't match instrument \'%s\'", \
                    type_name_left, type_name_right, instrument);
        }
        else
        {
            vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                    "runtime error: objects in type id \'%u\' and \'%s\' don't match instrument \'%s\'", \
                    new_object_left[1]->type, new_object_right[1]->type, instrument);
        }
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

fail:
done:
    if (new_object_left[1] != NULL) virtual_machine_object_destroy(vm, new_object_left[1]);
    if (new_object_right[1] != NULL) virtual_machine_object_destroy(vm, new_object_right[1]);

    return ret;
}

/* polynary operate */
int virtual_machine_object_polynary_operate(struct virtual_machine_object **object_dst, \
        struct virtual_machine_object *object_top, \
        uint32_t opcode, uint32_t operand, \
        struct virtual_machine *vm)
{
    int ret = 0;
    if (opcode == OP_LSTMK)
    {
        if ((ret = virtual_machine_object_list_make(vm, \
                        object_dst, \
                        object_top, (size_t)operand, VIRTUAL_MACHINE_OBJECT_LIST_MAKE_ORDER_DEFAULT, \
                        NULL)) != 0)
        { goto fail; }
        goto done;
    }
    else
    {
        VM_ERR_INTERNAL(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

fail:
done:
    return ret;
}

struct virtual_machine_convert_tbl_item
{
    const uint32_t object_src_type;
    const uint32_t object_dest_type;
    int (*virtual_machine_object_func)( \
            struct virtual_machine *vm, \
            struct virtual_machine_object **object_dst, \
            const struct virtual_machine_object *object_src, const uint32_t type);
};

static struct virtual_machine_convert_tbl_item virtual_machine_convert_tbl_item[] =
{
    {OBJECT_TYPE_INT, OBJECT_TYPE_INT, &virtual_machine_object_int_convert},
    {OBJECT_TYPE_INT, OBJECT_TYPE_BOOL, &virtual_machine_object_int_convert},
    {OBJECT_TYPE_INT, OBJECT_TYPE_STR, &virtual_machine_object_int_convert},
    {OBJECT_TYPE_INT, OBJECT_TYPE_FLOAT, &virtual_machine_object_int_convert},
    {OBJECT_TYPE_INT, OBJECT_TYPE_CHAR, &virtual_machine_object_int_convert},
    {OBJECT_TYPE_FLOAT, OBJECT_TYPE_INT, &virtual_machine_object_float_convert},
    {OBJECT_TYPE_FLOAT, OBJECT_TYPE_BOOL, &virtual_machine_object_float_convert},
    {OBJECT_TYPE_FLOAT, OBJECT_TYPE_STR, &virtual_machine_object_float_convert},
    {OBJECT_TYPE_FLOAT, OBJECT_TYPE_FLOAT, &virtual_machine_object_float_convert},
    {OBJECT_TYPE_BOOL, OBJECT_TYPE_BOOL, &virtual_machine_object_bool_convert},
    {OBJECT_TYPE_BOOL, OBJECT_TYPE_INT, &virtual_machine_object_bool_convert},
    {OBJECT_TYPE_BOOL, OBJECT_TYPE_STR, &virtual_machine_object_bool_convert},
    {OBJECT_TYPE_STR, OBJECT_TYPE_STR, &virtual_machine_object_str_convert},
    {OBJECT_TYPE_STR, OBJECT_TYPE_INT, &virtual_machine_object_str_convert},
    {OBJECT_TYPE_STR, OBJECT_TYPE_BOOL, &virtual_machine_object_str_convert},
    {OBJECT_TYPE_STR, OBJECT_TYPE_SYMBOL, &virtual_machine_object_str_convert},
    {OBJECT_TYPE_TYPE, OBJECT_TYPE_STR, &virtual_machine_object_type_convert},
    {OBJECT_TYPE_CHAR, OBJECT_TYPE_INT, &virtual_machine_object_char_convert},
    {OBJECT_TYPE_CHAR, OBJECT_TYPE_STR, &virtual_machine_object_char_convert},
    {OBJECT_TYPE_SYMBOL, OBJECT_TYPE_STR, &virtual_machine_object_symbol_convert},
};
#define VIRTUAL_MACHINE_CONVERT_TBL_ITEM (sizeof(virtual_machine_convert_tbl_item)/sizeof(struct virtual_machine_convert_tbl_item))

/* convert */
int virtual_machine_object_convert(struct virtual_machine_object **object_out, const struct virtual_machine_object *object_src, const uint32_t type, struct virtual_machine *vm)
{
    int ret = 0;
    unsigned int count = VIRTUAL_MACHINE_CONVERT_TBL_ITEM;
    struct virtual_machine_convert_tbl_item *item_cur = virtual_machine_convert_tbl_item;
    struct virtual_machine_object *new_object_src = NULL;
    char *type_name_src, *type_name_dst;

    new_object_src = (struct virtual_machine_object *)object_src;
    if ((ret = virtual_machine_variable_solve(&new_object_src, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }

    while (count-- != 0)
    {
        if ((new_object_src->type == item_cur->object_src_type) && \
                (type == item_cur->object_dest_type))
        {
            ret = (item_cur->virtual_machine_object_func)(vm, object_out, new_object_src, type);
            goto finish;
        }
        item_cur++;
    }
    virtual_machine_object_id_to_type_name(&type_name_src, NULL, new_object_src->type);
    virtual_machine_object_id_to_type_name(&type_name_dst, NULL, type);
    if ((type_name_src != NULL)
            && (type_name_dst != NULL))
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: invalid type cast from type \'%s\' to \'%s\'", \
                type_name_src, type_name_dst);
    }
    else
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: invalid type cast from type \'%u\' to \'%u\'", \
                new_object_src->type, type);
    }
    ret = -MULTIPLE_ERR_VM;
finish:
    goto done;
fail:
done:
    if (new_object_src != NULL) virtual_machine_object_destroy(vm, new_object_src);

    return ret;
}


static int virtual_machine_object_type_upgrade_int_to_float( \
        struct virtual_machine_object **object_dst, \
        struct virtual_machine_object *object_src, \
        struct virtual_machine *vm)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    new_object = virtual_machine_object_float_new_with_value(vm, (double)(((struct virtual_machine_object_int *)(object_src->ptr))->value));
    if (new_object == NULL) { goto fail; }
    *object_dst = new_object;

    goto done;
fail:
    if (new_object == NULL) virtual_machine_object_destroy(vm, new_object);
done:
    return ret;
}

struct virtual_machine_object_type_upgrade_convertion_matrix_item
{
    uint32_t type_small;
    uint32_t type_large;
    int (*func)(struct virtual_machine_object **object_dst, \
        struct virtual_machine_object *object_src, \
        struct virtual_machine *vm);
};
static const struct virtual_machine_object_type_upgrade_convertion_matrix_item \
          virtual_machine_object_type_upgrade_convertion_matrix_items[] = 
{
    { OBJECT_TYPE_INT, OBJECT_TYPE_FLOAT, &virtual_machine_object_type_upgrade_int_to_float },
};
#define VIRTUAL_MACHINE_OBJECT_TYPE_UPGRADE_CONVERTION_MATRIX_ITEMS_COUNT \
    (sizeof(virtual_machine_object_type_upgrade_convertion_matrix_items)/sizeof(struct virtual_machine_object_type_upgrade_convertion_matrix_item))

/* type upgrade */
int virtual_machine_object_type_upgrade( \

        struct virtual_machine_object **object_out_left, \
        struct virtual_machine_object **object_out_right, \
        const struct virtual_machine_object *object_src_left, \
        const struct virtual_machine_object *object_src_right, \
        struct virtual_machine *vm)
{
    int ret = 0;
    unsigned int i;
    struct virtual_machine_object *object_src_left_solved = NULL, *object_src_right_solved = NULL;
    struct virtual_machine_object *object_src_left_converted = NULL, *object_src_right_converted = NULL;

    if ((ret = virtual_machine_variable_solve(&object_src_left_solved , (struct virtual_machine_object *)object_src_left, NULL, 1, vm)) != 0)
    { goto fail; }
    if ((ret = virtual_machine_variable_solve(&object_src_right_solved , (struct virtual_machine_object *)object_src_right, NULL, 1, vm)) != 0)
    { goto fail; }

    *object_out_left = object_src_left_solved;
    *object_out_right = object_src_right_solved;

    for (i = 0; i != VIRTUAL_MACHINE_OBJECT_TYPE_UPGRADE_CONVERTION_MATRIX_ITEMS_COUNT; i++)
    {
        if ((virtual_machine_object_type_upgrade_convertion_matrix_items[i].type_large == object_src_left_solved->type) && \
                (virtual_machine_object_type_upgrade_convertion_matrix_items[i].type_small == object_src_right_solved->type))
        {
            if ((ret = virtual_machine_object_type_upgrade_convertion_matrix_items[i].func(&object_src_right_converted, object_src_right_solved, vm)) != 0)
            { goto fail; }
            *object_out_right = object_src_right_converted;
            *object_out_left = object_src_left_solved;
            virtual_machine_object_destroy(vm, object_src_right_solved); object_src_right_solved = NULL;
        }
        else if ((virtual_machine_object_type_upgrade_convertion_matrix_items[i].type_large == object_src_right_solved->type) && \
                (virtual_machine_object_type_upgrade_convertion_matrix_items[i].type_small == object_src_left_solved->type))
        {
            if ((ret = virtual_machine_object_type_upgrade_convertion_matrix_items[i].func(&object_src_left_converted, object_src_left_solved, vm)) != 0)
            { goto fail; }
            *object_out_left = object_src_left_converted;
            *object_out_right = object_src_right_solved;
            virtual_machine_object_destroy(vm, object_src_left_solved); object_src_left_solved = NULL;
        }
    }

    goto done;
fail:
    if (object_src_left_solved != NULL) virtual_machine_object_destroy(vm, object_src_left_solved);
    if (object_src_right_solved != NULL) virtual_machine_object_destroy(vm, object_src_right_solved);
    if (object_src_left_converted != NULL) virtual_machine_object_destroy(vm, object_src_left_converted);
    if (object_src_right_converted != NULL) virtual_machine_object_destroy(vm, object_src_right_converted);
done:
    return ret;
}

#ifndef DBL_EPSILON
#define DBL_EPSILON 0.00001
#endif
#ifndef ABS
#define ABS(x) ((x)>0?(x):(-(x)))
#endif

int objects_eq(const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right)
{
    /* Identifier */
    struct virtual_machine_object_identifier *object_id_left = NULL, *object_id_right = NULL;
    /* String */
    struct virtual_machine_object_str *object_str_left = NULL, *object_str_right = NULL;
    struct virtual_machine_object_str_internal *object_str_left_internal = NULL, *object_str_right_internal = NULL;
    /* Raw */
    struct virtual_machine_object_raw *object_raw_left = NULL, *object_raw_right = NULL;
    /* Class */
    struct virtual_machine_object_class *object_class_left = NULL, *object_class_right = NULL;
    /* Function */
    /*struct virtual_machine_object_func *object_function_left = NULL, *object_function_right = NULL;*/
    /* Symbol */
    struct virtual_machine_object_symbol *object_symbol_left = NULL, *object_symbol_right = NULL;

    /* Type Check */
    if (object_left->type != object_right->type)
    {
        return OBJECTS_NE;
    }
    switch (object_left->type)
    {
        case OBJECT_TYPE_IDENTIFIER:
            object_id_left = object_left->ptr;
            object_id_right = object_right->ptr;
            if ((object_id_left->id_len != object_id_right->id_len) || \
                    (strncmp(object_id_left->id, object_id_right->id, object_id_right->id_len) != 0))
            { return OBJECTS_NE; }
            else
            { return OBJECTS_EQ; }
            break;
        case OBJECT_TYPE_INT:
            return (((struct virtual_machine_object_int *)(object_left->ptr))->value == \
                    ((struct virtual_machine_object_int *)(object_right->ptr))->value) ? OBJECTS_EQ : OBJECTS_NE;
            break;
        case OBJECT_TYPE_CHAR:
            return (((struct virtual_machine_object_char *)(object_left->ptr))->value == \
                    ((struct virtual_machine_object_char *)(object_right->ptr))->value) ? OBJECTS_EQ : OBJECTS_NE;
            break;
        case OBJECT_TYPE_STR:
            object_str_left = object_left->ptr;
            object_str_left_internal = object_str_left->ptr_internal;
            object_str_right = object_right->ptr;
            object_str_right_internal = object_str_right->ptr_internal;
            if ((object_str_left_internal->len != object_str_right_internal->len) || \
                    (object_str_left_internal->checksum_crc32 != object_str_right_internal->checksum_crc32) || \
                    (strncmp(object_str_left_internal->str, object_str_right_internal->str, object_str_left_internal->len) != 0))
            {
                return OBJECTS_NE;
            }
            else
            {
                return OBJECTS_EQ;
            }
            break;
        case OBJECT_TYPE_BOOL:
            return (((struct virtual_machine_object_bool *)(object_left->ptr))->value == \
                    ((struct virtual_machine_object_bool *)(object_right->ptr))->value) ? OBJECTS_EQ : OBJECTS_NE;
            break;
        case OBJECT_TYPE_NONE:
            return OBJECTS_EQ;
            break;
        case OBJECT_TYPE_UNDEF:
            return OBJECTS_EQ;
            break;
        case OBJECT_TYPE_FLOAT:
            return (ABS(((struct virtual_machine_object_float *)(object_left->ptr))->value -
                    ((struct virtual_machine_object_float *)(object_right->ptr))->value) < DBL_EPSILON) ? OBJECTS_EQ : OBJECTS_NE;
            break;
        case OBJECT_TYPE_TYPE:
            return (((struct virtual_machine_object_type *)(object_left->ptr))->value == \
                    ((struct virtual_machine_object_type *)(object_right->ptr))->value) ? OBJECTS_EQ : OBJECTS_NE;
            break;
        case OBJECT_TYPE_THREAD:
            return (((struct virtual_machine_object_thread *)(object_left->ptr))->tid == \
                    ((struct virtual_machine_object_thread *)(object_right->ptr))->tid) ? OBJECTS_EQ : OBJECTS_NE;
            break;
        case OBJECT_TYPE_FUNCTION:
            return OBJECTS_NE;
            break;
        case OBJECT_TYPE_RAW:
            object_raw_left = (struct virtual_machine_object_raw *)(object_left->ptr);
            object_raw_right = (struct virtual_machine_object_raw *)(object_right->ptr);
            /* Raw object name length */
            if (object_raw_left->len != object_raw_right->len)
            {
                return OBJECTS_NE;
            }
            /* Raw object name content */
            if (strncmp(object_raw_left->name, object_raw_right->name, object_raw_right->len) != 0)
            {
                return OBJECTS_NE;
            }
            /* Raw object name pointers */
            if (object_raw_left->func_eq(object_raw_left, object_raw_right) == 0)
            {
                return OBJECTS_NE;
            }
            else
            {
                return OBJECTS_EQ;
            }
            break;
        case OBJECT_TYPE_CLASS:
            object_class_left = (struct virtual_machine_object_class *)(object_left->ptr);
            object_class_right = (struct virtual_machine_object_class *)(object_right->ptr);
            if (object_class_left->ptr_internal != object_class_right->ptr_internal)
            {
                return OBJECTS_NE;
            }
            else
            {
                return OBJECTS_EQ;
            }
            break;
        case OBJECT_TYPE_SYMBOL:
            object_symbol_left = object_left->ptr;
            object_symbol_right = object_right->ptr;
            if ((object_symbol_left->id_len != object_symbol_right->id_len) || \
                    (strncmp(object_symbol_left->id, object_symbol_right->id, object_symbol_right->id_len) != 0))
            {
                return OBJECTS_NE;
            }
            else
            {
                return OBJECTS_EQ;
            }
            break;
        default:
            break;
    }
    return OBJECTS_NE;
}

/* eq, ne */
static int virtual_machine_object_generic_equality(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode)
{
    struct virtual_machine_object *new_object = NULL;

    if ((object_left == NULL) || (object_right == NULL)) return -MULTIPLE_ERR_NULL_PTR;

    *object_out = NULL;

    if (objects_eq(object_left, object_right) == OBJECTS_NE)
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
    else
    {
        switch (opcode)
        {
            case OP_EQ:
                new_object = virtual_machine_object_bool_new_with_value(vm, VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE);
                goto finish;
            case OP_NE:
                new_object = virtual_machine_object_bool_new_with_value(vm, VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_FALSE);
                goto finish;
        }
    }

finish:

    *object_out = new_object;

    return 0;
}

