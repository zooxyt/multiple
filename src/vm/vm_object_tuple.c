/* Tuple Objects
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "multiple_err.h"

#include "vm_opcode.h"
#include "vm_types.h"
#include "vm_object_aio.h"
#include "vm_err.h"

int virtual_machine_object_tuple_append(struct virtual_machine_object *object, \
        struct virtual_machine_object *object_new_sub, \
        struct virtual_machine *vm);

struct virtual_machine_object *virtual_machine_object_tuple_new( \
        struct virtual_machine *vm)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_tuple *new_object_tuple = NULL;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_TUPLE)) == NULL)
    {
        return NULL;
    }

    if ((new_object_tuple = (struct virtual_machine_object_tuple *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_tuple))) == NULL)
    {
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }

    if (_virtual_machine_object_ptr_set(new_object, new_object_tuple) != 0)
    {
        virtual_machine_resource_free(vm->resource, new_object_tuple);
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }

    new_object_tuple->begin = new_object_tuple->end = NULL;
    new_object_tuple->size = 0;

    return new_object;
}

int virtual_machine_object_tuple_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    struct virtual_machine_object_tuple *object_tuple;
    struct virtual_machine_object_tuple_node *object_tuple_node_cur, *object_tuple_node_next;
    struct virtual_machine_object *virtual_machine_object_tuple_node_object;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_tuple = (struct virtual_machine_object_tuple *)object->ptr;
    object_tuple_node_cur = object_tuple->begin;

    while (object_tuple_node_cur != NULL)
    {
        object_tuple_node_next = object_tuple_node_cur->next; 
        virtual_machine_object_tuple_node_object = object_tuple_node_cur->ptr;
        virtual_machine_object_destroy(vm, virtual_machine_object_tuple_node_object);
        virtual_machine_resource_free(vm->resource, object_tuple_node_cur);
        object_tuple_node_cur = object_tuple_node_next;
    }

    if (object->ptr != NULL) virtual_machine_resource_free(vm->resource, object->ptr);
    _virtual_machine_object_destroy(vm, object);

    return 0;
}

struct virtual_machine_object *virtual_machine_object_tuple_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL, *new_sub_object = NULL;
    struct virtual_machine_object_tuple *object_tuple;
    struct virtual_machine_object_tuple_node *object_tuple_node_cur, *object_tuple_node_next;
    struct virtual_machine_object *virtual_machine_object_tuple_node_object;

    if (object == NULL) return NULL;

    if ((new_object = virtual_machine_object_tuple_new(vm)) == NULL)
    {
        goto fail;
    }

    object_tuple = (struct virtual_machine_object_tuple *)object->ptr;
    object_tuple_node_cur = object_tuple->begin;

    while (object_tuple_node_cur != NULL)
    {
        object_tuple_node_next = object_tuple_node_cur->next; 
        virtual_machine_object_tuple_node_object = object_tuple_node_cur->ptr;
        if ((new_sub_object = virtual_machine_object_clone(vm, virtual_machine_object_tuple_node_object)) == NULL)
        { goto fail; }
        if (virtual_machine_object_tuple_append(new_object, new_sub_object, vm) != 0)
        { goto fail; }
        new_sub_object = NULL;
        object_tuple_node_cur = object_tuple_node_next;
    }

    goto done;
fail:
    if (new_sub_object != NULL) virtual_machine_object_destroy(vm, new_sub_object);
    new_sub_object = NULL;
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
    new_object = NULL;
done:
    return new_object;
}

int virtual_machine_object_tuple_print(const struct virtual_machine_object *object)
{
    struct virtual_machine_object_tuple *object_tuple;
    struct virtual_machine_object_tuple_node *object_tuple_node_cur, *object_tuple_node_next;
    struct virtual_machine_object *virtual_machine_object_tuple_node_object;
    int first = 0;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_tuple = (struct virtual_machine_object_tuple *)object->ptr;
    object_tuple_node_cur = object_tuple->begin;

    printf("(");
    while (object_tuple_node_cur != NULL)
    {
        object_tuple_node_next = object_tuple_node_cur->next; 
        virtual_machine_object_tuple_node_object = object_tuple_node_cur->ptr;

        if (first == 0) { first = 1; }
        else { printf(", "); }

        virtual_machine_object_print(virtual_machine_object_tuple_node_object);
        object_tuple_node_cur = object_tuple_node_next;
    }

    printf(")");

    return 0;
}

/* size */
int virtual_machine_object_tuple_size(struct virtual_machine *vm, struct virtual_machine_object **object_out, const struct virtual_machine_object *object_src)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_tuple *tuple_object = NULL;

    tuple_object = (struct virtual_machine_object_tuple *)object_src->ptr;
    if ((new_object = virtual_machine_object_int_new_with_value(vm, (int)(tuple_object->size))) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    *object_out = new_object;
fail:
    return ret;
}

int virtual_machine_object_tuple_append(struct virtual_machine_object *object, \
        struct virtual_machine_object *object_new_sub, \
        struct virtual_machine *vm)
{
    int ret = 0;
    struct virtual_machine_object_tuple *object_tuple;
    struct virtual_machine_object_tuple_node *new_object_tuple_node;

    struct virtual_machine_object *object_new_sub_solved = NULL;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if ((ret = virtual_machine_variable_solve(&object_new_sub_solved, (struct virtual_machine_object *)object_new_sub, NULL, 1, vm)) != 0)
    { goto fail; }

    new_object_tuple_node = (struct virtual_machine_object_tuple_node *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_object_tuple_node));
    if (new_object_tuple_node == NULL)
    {
        if (object_new_sub_solved != NULL) virtual_machine_object_destroy(vm, object_new_sub_solved);
        return -MULTIPLE_ERR_NULL_PTR;
    }
    new_object_tuple_node->ptr = (struct virtual_machine_object *)object_new_sub_solved;
    object_new_sub_solved = NULL;
    new_object_tuple_node->next = NULL;

    object_tuple = (struct virtual_machine_object_tuple *)object->ptr;

    if (object_tuple->begin == NULL)
    {
        object_tuple->begin = object_tuple->end = new_object_tuple_node;
    }
    else
    {
        object_tuple->end->next = new_object_tuple_node;
        object_tuple->end = new_object_tuple_node;
    }
    object_tuple->size++;
    if (object_new_sub != NULL) {virtual_machine_object_destroy(vm, object_new_sub);object_new_sub = NULL;}


    goto done;
fail:
    if (object_new_sub_solved != NULL) virtual_machine_object_destroy(vm, object_new_sub_solved);
done:
    return ret;
}

int virtual_machine_object_tuple_make(struct virtual_machine *vm, struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_top, \
        const size_t count)
{
    int ret = 0;

    int count_copy = (int)count;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object *new_sub_object = NULL;
    struct virtual_machine_object *object_cur = NULL;

    *object_out = NULL;
    if ((new_object = virtual_machine_object_tuple_new(vm)) == NULL) { return -MULTIPLE_ERR_MALLOC; }

    object_cur = (struct virtual_machine_object *)object_top;
    while (count_copy-- != 0)
    {
        if ((ret = virtual_machine_variable_solve(&new_sub_object, (struct virtual_machine_object *)object_cur, NULL, 1, vm)) != 0)
        { goto fail; }
        if ((ret = virtual_machine_object_tuple_append(new_object, new_sub_object, vm)) != 0) 
        { 
            goto fail; 
        }
        new_sub_object = NULL;
        object_cur = object_cur->prev;
    }

    *object_out = new_object;

    ret = 0;
    goto done;
fail:
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
done:
    if (new_sub_object != NULL) virtual_machine_object_destroy(vm, new_sub_object);
    return ret;
}

static int _virtual_machine_object_tuple_ref_get_by_raw_index(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, int ref_index)
{
    int ret = 0;
    struct virtual_machine_object_tuple_node *tuple_node_cur = NULL;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_tuple *object_tuple = NULL;

    (void)vm;

    *object_out = NULL;

    object_tuple = (struct virtual_machine_object_tuple *)object_src->ptr;
    if (object_tuple->size == 0) 
    {
        vm_err_update(vm->r, -VM_ERR_OUT_OF_BOUNDS, \
                "runtime error: out of bounds, there is no element in tuple");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    if ((ref_index < 0) || (ref_index >= (signed int)object_tuple->size))
    {
        vm_err_update(vm->r, -VM_ERR_OUT_OF_BOUNDS, \
                "runtime error: out of bounds, reference of element \'%d\' isn't in bound of %u to %u", \
                ref_index, 0, object_tuple->size);
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    tuple_node_cur = object_tuple->begin;
    while (tuple_node_cur != NULL)
    {
        if (ref_index == 0)
        {
            if ((new_object = virtual_machine_object_clone(vm, tuple_node_cur->ptr)) == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            *object_out = new_object;
            goto finish;
        }
        tuple_node_cur = tuple_node_cur->next; 
        ref_index -= 1;
    }
finish:
fail:
    return ret;
}

int virtual_machine_object_tuple_ref_get(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, const struct virtual_machine_object *object_idx)
{
    int ret = 0;

    struct virtual_machine_object *object_src_solved= NULL;
    struct virtual_machine_object *object_idx_solved= NULL;
    struct virtual_machine_object_int *object_ref_index = NULL;
    int ref_index;

    *object_out = NULL;

    if ((ret = virtual_machine_variable_solve(&object_idx_solved, (struct virtual_machine_object *)object_idx, NULL, 1, vm)) != 0)
    { goto fail; }
    if ((ret = virtual_machine_variable_solve(&object_src_solved, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }

    if (object_idx_solved->type != OBJECT_TYPE_INT)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type, reference of tuple should be in type \'int\'");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }
    if (object_src_solved->type != OBJECT_TYPE_TUPLE)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    object_ref_index = (struct virtual_machine_object_int *)object_idx_solved->ptr;
    ref_index = object_ref_index->value;

    if ((ret = _virtual_machine_object_tuple_ref_get_by_raw_index(vm, object_out, object_src_solved, ref_index)) != 0)
    { goto fail; }

fail:
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved);
    if (object_idx_solved != NULL) virtual_machine_object_destroy(vm, object_idx_solved);
    return ret;
}

static int _virtual_machine_object_tuple_ref_set_by_raw_index(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        int ref_index, \
        struct virtual_machine_object *object_value, \
        struct virtual_machine *vm)
{
    int ret = 0;
    struct virtual_machine_object_tuple_node *tuple_node_cur = NULL;
    struct virtual_machine_object_tuple *object_tuple = NULL;

    (void)vm;

    *object_out = NULL;

    object_tuple = (struct virtual_machine_object_tuple *)object_src->ptr;
    if (object_tuple->size == 0) 
    {
        vm_err_update(vm->r, -VM_ERR_OUT_OF_BOUNDS, \
                "runtime error: out of bounds, there is no element in tuple");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    if ((ref_index < 0) || (ref_index >= (signed int)object_tuple->size))
    {
        vm_err_update(vm->r, -VM_ERR_OUT_OF_BOUNDS, \
                "runtime error: out of bounds, reference of element \'%d\' isn't in bound of %u to %u", \
                ref_index, 0, object_tuple->size);
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    tuple_node_cur = object_tuple->begin;
    while (tuple_node_cur != NULL)
    {
        if (ref_index == 0)
        {
            virtual_machine_object_destroy(vm, tuple_node_cur->ptr);
            tuple_node_cur->ptr = NULL;
            tuple_node_cur->ptr = virtual_machine_object_clone(vm, object_value);
            if (tuple_node_cur->ptr == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            goto finish;
        }
        tuple_node_cur = tuple_node_cur->next; 
        ref_index -= 1;
    }
finish:
fail:
    return ret;
}

int virtual_machine_object_tuple_ref_set(struct virtual_machine *vm, struct virtual_machine_object **object_out, const struct virtual_machine_object *object_src, const struct virtual_machine_object *object_idx, const struct virtual_machine_object *object_value)
{
    int ret = 0;

    struct virtual_machine_object *object_src_solved= NULL;
    struct virtual_machine_object *object_idx_solved= NULL;
    struct virtual_machine_object *object_value_solved = NULL;
    struct virtual_machine_object_int *object_ref_index = NULL;
    int ref_index;

    *object_out = NULL;

    if ((ret = virtual_machine_variable_solve(&object_idx_solved, (struct virtual_machine_object *)object_idx, NULL, 1, vm)) != 0)
    { goto fail; }
    if ((ret = virtual_machine_variable_solve(&object_src_solved, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }
    if ((ret = virtual_machine_variable_solve(&object_value_solved, (struct virtual_machine_object *)object_value, NULL, 1, vm)) != 0)
    { goto fail; }

    if (object_idx_solved->type != OBJECT_TYPE_INT)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type, reference of tuple should be in type \'int\'");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }
    if (object_src_solved->type != OBJECT_TYPE_TUPLE)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    object_ref_index = (struct virtual_machine_object_int *)object_idx_solved->ptr;
    ref_index = object_ref_index->value;

    if ((ret = _virtual_machine_object_tuple_ref_set_by_raw_index(object_out, object_src_solved, ref_index, object_value_solved, vm)) != 0)
    { goto fail; }

    *object_out = object_src_solved; object_src_solved = NULL;
    goto done;
fail:
done:
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved);
    if (object_idx_solved != NULL) virtual_machine_object_destroy(vm, object_idx_solved);
    if (object_value_solved != NULL) virtual_machine_object_destroy(vm, object_value_solved);
    return ret;
}

