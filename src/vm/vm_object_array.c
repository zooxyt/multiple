/* Array Objects
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
#include "vm_gc.h"
#include "vm_err.h"


/* Declarations */
int virtual_machine_object_array_internal_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_array_internal *object_array_internal);

#define VIRTUAL_MACHINE_OBJECT_ARRAY_INTERNAL_APPEND_TO_TAIL 0
#define VIRTUAL_MACHINE_OBJECT_ARRAY_INTERNAL_APPEND_TO_HEAD 1
int virtual_machine_object_array_internal_append(struct virtual_machine *vm, \
        struct virtual_machine_object_array_internal *object_array_internal, \
        struct virtual_machine_object *object_new_sub, int order);


/* Internal Marker */
int virtual_machine_object_array_internal_marker(void *object_internal)
{
    struct virtual_machine_object_array_internal *object_array_internal = object_internal;
    struct virtual_machine_object_array_internal_node *object_array_internal_node;
    size_t idx;

    for (idx = object_array_internal->pos; idx != object_array_internal->pos + object_array_internal->size; idx++)
    {
        object_array_internal_node = object_array_internal->nodes + idx;
        virtual_machine_marks_object(object_array_internal_node->ptr, VIRTUAL_MACHINE_GARBAGE_COLLECT_MAJOR);
    }

    return 0;
}

/* Internal Collector */
int virtual_machine_object_array_internal_collector(void *object_internal, int *confirm)
{
    struct virtual_machine_object_array_internal *object_array_internal;

    *confirm = VIRTUAL_MACHINE_GARBAGE_COLLECT_CONFIRM; 

    object_array_internal = object_internal;
    return virtual_machine_object_array_internal_destroy(object_array_internal->vm, object_array_internal);
}

/* Internal Part */

static struct virtual_machine_object_array_internal *virtual_machine_object_array_internal_new( \
        struct virtual_machine *vm, size_t count)
{
    struct virtual_machine_object_array_internal *new_object_array_internal = NULL;
    size_t idx;

    /* Create the internal part of array instance */
    new_object_array_internal = (struct virtual_machine_object_array_internal *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_object_array_internal));
    if (new_object_array_internal == NULL)
    { goto fail; }

    new_object_array_internal->vm = vm;
    /* <Reserved> <Allocated> <Reserved> */
    new_object_array_internal->size = count;
    new_object_array_internal->capacity = new_object_array_internal->size * 3;
    new_object_array_internal->pos = new_object_array_internal->size;
    new_object_array_internal->nodes = NULL;
    new_object_array_internal->nodes = (struct virtual_machine_object_array_internal_node *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_object_array_internal_node) * new_object_array_internal->capacity);
    if (new_object_array_internal->nodes == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        goto fail; 
    }
    for (idx = 0; idx != new_object_array_internal->capacity; idx++)
    {
        new_object_array_internal->nodes[idx].ptr = NULL;
    }
        

    goto done;
fail:
    if (new_object_array_internal != NULL)
    {
        if (new_object_array_internal->nodes != NULL) 
        {
            virtual_machine_resource_free(vm->resource, new_object_array_internal->nodes);
        }
        virtual_machine_resource_free(vm->resource, new_object_array_internal);
		new_object_array_internal = NULL;
    }
done:
    return new_object_array_internal;
}

int virtual_machine_object_array_internal_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_array_internal *object_array_internal)
{
    struct virtual_machine_object *virtual_machine_object_array_node_object;
    size_t idx;

    if (object_array_internal == NULL) return -MULTIPLE_ERR_NULL_PTR;

    for (idx = object_array_internal->pos; idx != object_array_internal->pos + object_array_internal->size; idx++)
    {
        virtual_machine_object_array_node_object = object_array_internal->nodes[idx].ptr;
        virtual_machine_object_destroy(vm, virtual_machine_object_array_node_object);
        object_array_internal->nodes[idx].ptr = NULL;
    }

    virtual_machine_resource_free(vm->resource, object_array_internal->nodes);
    virtual_machine_resource_free(vm->resource, object_array_internal);

    return 0;
}

static struct virtual_machine_object_array_internal *virtual_machine_object_array_internal_clone(struct virtual_machine *vm, \
        const struct virtual_machine_object_array_internal *object_array_internal)
{
    struct vm_err r_local;
    struct virtual_machine_object *new_sub_object = NULL;
    struct virtual_machine_object_array_internal *new_object_array_internal = NULL;
    size_t idx;

    if (object_array_internal == NULL) return NULL;

    vm_err_clear(&r_local);

    if ((new_object_array_internal = virtual_machine_object_array_internal_new(vm, object_array_internal->size)) == NULL)
    { goto fail; }
    new_object_array_internal->pos = object_array_internal->pos;

    for (idx = object_array_internal->pos; idx != object_array_internal->pos + object_array_internal->size; idx++)
    {
        if ((new_sub_object = virtual_machine_object_clone(vm, object_array_internal->nodes[idx].ptr)) == NULL)
        { goto fail; }
        new_object_array_internal->nodes[idx].ptr = new_sub_object; new_sub_object = NULL;
    }

    goto done;
fail:
    if (new_sub_object != NULL) virtual_machine_object_destroy(vm, new_sub_object);
    new_sub_object = NULL;
    if (new_object_array_internal != NULL) virtual_machine_object_array_internal_destroy(vm, new_object_array_internal);
    new_object_array_internal = NULL;
done:
    return new_object_array_internal;
}

static int virtual_machine_object_array_internal_print(const struct virtual_machine_object_array_internal *object_array_internal)
{
    int first = 0;
    size_t idx;

    if (object_array_internal == NULL) return -MULTIPLE_ERR_NULL_PTR;

    printf("[");

    for (idx = object_array_internal->pos; idx != object_array_internal->pos + object_array_internal->size; idx++)
    {
        if (first == 0) { first = 1; }
        else { printf(", "); }
        virtual_machine_object_print(object_array_internal->nodes[idx].ptr);
    }

    printf("]");

    return 0;
}

/* size */
static int virtual_machine_object_array_internal_size(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object_array_internal *object_array_internal_src)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    if ((new_object = virtual_machine_object_int_new_with_value(vm, (int)(object_array_internal_src->size))) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    *object_out = new_object;
fail:
    return ret;
}

static int virtual_machine_object_array_internal_make(struct virtual_machine *vm, \
        struct virtual_machine_object_array_internal **object_out, \
        const struct virtual_machine_object *object_top, const size_t count, int order, \
        struct virtual_machine_running_stack_frame *target_frame)
{
    int ret = 0;
    int count_copy = (int)count;
    struct virtual_machine_object_array_internal *new_object_array_internal = NULL;
    struct virtual_machine_object *new_sub_object = NULL;
    struct virtual_machine_object *object_cur = NULL;
    size_t idx;

    *object_out = NULL;
    if ((new_object_array_internal = virtual_machine_object_array_internal_new(vm, count)) == NULL) { return -MULTIPLE_ERR_MALLOC; }

    switch (order)
    {
        case VIRTUAL_MACHINE_OBJECT_ARRAY_MAKE_ORDER_DEFAULT: 
            idx = new_object_array_internal->pos; 
            break;
        case VIRTUAL_MACHINE_OBJECT_ARRAY_MAKE_ORDER_REVERSE: 
            idx = new_object_array_internal->pos + new_object_array_internal->size - 1; 
            break;
        default:
            ret = -MULTIPLE_ERR_MALLOC;
            goto fail;
            break;
    }

    object_cur = (struct virtual_machine_object *)object_top;
    while (count_copy-- != 0)
    {
        if ((ret = virtual_machine_variable_solve(&new_sub_object, (struct virtual_machine_object *)object_cur, target_frame, 1, vm)) != 0)
        { goto fail; }

        new_object_array_internal->nodes[idx].ptr = new_sub_object; new_sub_object = NULL;
        switch (order)
        {
            case VIRTUAL_MACHINE_OBJECT_ARRAY_MAKE_ORDER_DEFAULT:
                idx++;
                break;
            case VIRTUAL_MACHINE_OBJECT_ARRAY_MAKE_ORDER_REVERSE:
                idx--;
                break;
        }
        object_cur = object_cur->prev;
    }

    *object_out = new_object_array_internal;

    ret = 0;
    goto done;
fail:
    if (new_object_array_internal != NULL) virtual_machine_object_array_internal_destroy(vm, new_object_array_internal);
done:
    if (new_sub_object != NULL) virtual_machine_object_destroy(vm, new_sub_object);
    return ret;
}

static int _virtual_machine_object_array_internal_ref_get_by_raw_index(struct virtual_machine_object **object_out,\
        const struct virtual_machine_object_array_internal *object_array_internal_src, \
        int ref_index, \
        struct virtual_machine *vm)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    (void)vm;

    *object_out = NULL;

    if (object_array_internal_src->size == 0) 
    {
        vm_err_update(vm->r, -VM_ERR_OUT_OF_BOUNDS, \
                "runtime error: out of bounds, there is no element in array");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    if ((ref_index < 0) || (ref_index >= (signed int)(object_array_internal_src->size)))
    {
        vm_err_update(vm->r, -VM_ERR_OUT_OF_BOUNDS, \
                "runtime error: out of bounds, reference of element \'%d\' isn't in bound of %u to %u", \
                ref_index, 0, (unsigned int)object_array_internal_src->size);
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    if ((new_object = virtual_machine_object_clone(vm, \
                    object_array_internal_src->nodes[object_array_internal_src->pos + (size_t)ref_index].ptr)) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    *object_out = new_object;

fail:
    return ret;
}

int virtual_machine_object_array_ref_get(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_idx)
{
    int ret = 0;

    struct virtual_machine_object *object_src_solved= NULL;
    struct virtual_machine_object_array *object_src_solved_array = NULL;
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
                "runtime error: unsupported operand type, reference of array should be in type \'int\'");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }
    if (object_src_solved->type != OBJECT_TYPE_ARRAY)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    object_ref_index = (struct virtual_machine_object_int *)object_idx_solved->ptr;
    ref_index = object_ref_index->value;
    object_src_solved_array = object_src_solved->ptr;

    if ((ret = _virtual_machine_object_array_internal_ref_get_by_raw_index(object_out, object_src_solved_array->ptr_internal, ref_index, vm)) != 0)
    { goto fail; }

fail:
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved);
    if (object_idx_solved != NULL) virtual_machine_object_destroy(vm, object_idx_solved);
    return ret;
}

static int _virtual_machine_object_array_internal_ref_set_by_raw_index(const struct virtual_machine_object_array_internal *object_array_internal_src, \
        int ref_index, const struct virtual_machine_object *object_value, struct virtual_machine *vm)
{
    int ret = 0;

    (void)vm;

    if (object_array_internal_src->size == 0) 
    {
        vm_err_update(vm->r, -VM_ERR_OUT_OF_BOUNDS, \
                "runtime error: out of bounds, there is no element in array");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    if ((ref_index < 0) || (ref_index >= (signed int)(object_array_internal_src->size)))
    {
        vm_err_update(vm->r, -VM_ERR_OUT_OF_BOUNDS, \
                "runtime error: out of bounds, reference of element \'%d\' isn't in bound of %u to %u", \
                ref_index, 0, (unsigned int)object_array_internal_src->size);
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    if (object_array_internal_src->nodes[object_array_internal_src->pos + (size_t)ref_index].ptr != NULL)
    {
        virtual_machine_object_destroy(vm, \
                object_array_internal_src->nodes[object_array_internal_src->pos + (size_t)ref_index].ptr);
		object_array_internal_src->nodes[object_array_internal_src->pos + (size_t)ref_index].ptr = NULL;
    }

	object_array_internal_src->nodes[object_array_internal_src->pos + (size_t)ref_index].ptr = virtual_machine_object_clone(vm, object_value);
	if (object_array_internal_src->nodes[object_array_internal_src->pos + (size_t)ref_index].ptr == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
fail:
    return ret;
}

int virtual_machine_object_array_ref_set(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_idx, \
        const struct virtual_machine_object *object_value)
{
    int ret = 0;

    struct virtual_machine_object *object_src_solved= NULL;
    struct virtual_machine_object_array *object_src_solved_array = NULL;
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
                "runtime error: unsupported operand type, reference of array should be in type \'int\'");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }
    if (object_src_solved->type != OBJECT_TYPE_ARRAY)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    object_ref_index = (struct virtual_machine_object_int *)object_idx_solved->ptr;
    ref_index = object_ref_index->value;

    object_src_solved_array = object_src_solved->ptr;
    if ((ret = _virtual_machine_object_array_internal_ref_set_by_raw_index(object_src_solved_array->ptr_internal, ref_index, object_value_solved, vm)) != 0)
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

int virtual_machine_object_array_car(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src)
{
    int ret = 0;

    struct virtual_machine_object *object_src_solved = NULL;
    struct virtual_machine_object_array *object_array = NULL;

    *object_out = NULL;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }

    if (object_src_solved->type != OBJECT_TYPE_ARRAY)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }
    object_array = object_src_solved->ptr;
    if ((ret = _virtual_machine_object_array_internal_ref_get_by_raw_index(object_out, object_array->ptr_internal, 0, vm)) != 0)
    { goto fail; }

fail:
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved);
    return ret;
}

static int virtual_machine_object_array_internal_cdr( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_array_internal **object_array_internal_out, \
        const struct virtual_machine_object_array_internal *object_array_internal_src)
{
    int ret = 0;

    struct virtual_machine_object_array_internal *new_object_array_internal = NULL;

    *object_array_internal_out = NULL;

    /* Element number checking */
    if (object_array_internal_src->size == 0)
    {
        vm_err_update(vm->r, -VM_ERR_EMPTY_LIST, \
                "runtime error: empty array has no element");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    /* Complete clone the internal of original array */
    if ((new_object_array_internal = virtual_machine_object_array_internal_clone(vm, object_array_internal_src)) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }
    
    if (new_object_array_internal->size != 0)
    {

        if ((ret = virtual_machine_object_destroy(vm, new_object_array_internal->nodes[new_object_array_internal->pos].ptr)) != 0) 
        {
            VM_ERR_INTERNAL(vm->r);
            ret = -MULTIPLE_ERR_VM;
            goto fail; 
        }
        
        new_object_array_internal->pos += 1;
        new_object_array_internal->size -= 1;
    }

    *object_array_internal_out = new_object_array_internal;
    new_object_array_internal = NULL;

    ret = 0;
fail:
    if (new_object_array_internal != NULL) virtual_machine_object_array_internal_destroy(vm, new_object_array_internal);
    return ret;
}

int virtual_machine_object_array_cdr( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_array *new_object_array = NULL;
    struct virtual_machine_object_array_internal *new_object_array_internal = NULL;

    struct virtual_machine_object *object_src_solved = NULL;
    struct virtual_machine_object_array *object_array_src = NULL;
    struct virtual_machine_object_array_internal *object_array_internal_src = NULL;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }

    object_array_src = object_src_solved->ptr;
    object_array_internal_src = object_array_src->ptr_internal;

    /* Create a 'cdred' internal */
    ret = virtual_machine_object_array_internal_cdr( \
            vm, \
            &new_object_array_internal, \
            object_array_internal_src);
    if (ret != 0) { goto fail; }

    /* Create the shell part */
    if ((new_object_array = (struct virtual_machine_object_array *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_array))) == NULL)
    { goto fail; }
    new_object_array->ptr_internal = new_object_array_internal; new_object_array_internal = NULL;

    /* Create the array's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_ARRAY)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_array) != 0)
    { goto fail; }

    if (virtual_machine_resource_reference_register( \
                vm->gc_stub, \
                new_object, \
                new_object_array->ptr_internal, \
                &virtual_machine_object_array_internal_marker, \
                &virtual_machine_object_array_internal_collector) != 0)
    { goto fail; }

    new_object_array = NULL;

    *object_out = new_object; new_object = NULL;

fail:
    if (new_object_array_internal != NULL) virtual_machine_object_array_internal_destroy(vm, new_object_array_internal);
    if (new_object_array != NULL) virtual_machine_resource_free(vm->resource, new_object_array);
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved);
    return ret;
}

#ifndef MAX
#define MAX(a, b) ((a)>(b)?(a):(b))
#endif
#define VIRTUAL_MACHINE_OBJECT_ARRAY_CAPACITY_MINIMUM_REQUIRED 9

static int virtual_machine_object_array_internal_extend(struct virtual_machine *vm, \
        struct virtual_machine_object_array_internal *object_array_internal)
{
    int ret = 0;
    struct virtual_machine_object_array_internal_node *new_nodes = NULL;
    size_t idx_dst, idx_src, idx_src_end;
    size_t new_capacity, new_size, new_pos;

    new_size = object_array_internal->size;
    new_capacity = MAX(new_size * 3, VIRTUAL_MACHINE_OBJECT_ARRAY_CAPACITY_MINIMUM_REQUIRED);
    new_pos = new_capacity / 3;

    new_nodes = virtual_machine_resource_malloc(vm->resource, \
            sizeof(struct virtual_machine_object_array_internal_node) * new_capacity);
    if (new_nodes == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    idx_src = object_array_internal->pos;
    idx_src_end = object_array_internal->pos + object_array_internal->size;
    idx_dst = new_pos;

    while (idx_src != idx_src_end)
    {
        new_nodes[idx_dst].ptr = object_array_internal->nodes[idx_src].ptr;
        idx_src++;
        idx_dst++;
    }

    object_array_internal->nodes = new_nodes;
    object_array_internal->pos = new_pos;
    object_array_internal->size = new_size;
    object_array_internal->capacity = new_capacity;

    goto done;
fail:
    if (new_nodes != NULL)
    {
        virtual_machine_resource_free(vm->resource, new_nodes);
    }
done:
    return ret;
}

int virtual_machine_object_array_internal_append(struct virtual_machine *vm, \
        struct virtual_machine_object_array_internal *object_array_internal, \
        struct virtual_machine_object *object_new_sub, int order)
{
    int ret = 0;
    struct virtual_machine_object *object_new_sub_solved = NULL;

    if (object_array_internal == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if ((order == VIRTUAL_MACHINE_OBJECT_ARRAY_INTERNAL_APPEND_TO_TAIL) && \
            (object_array_internal->pos + object_array_internal->size >= object_array_internal->capacity))
    {
        virtual_machine_object_array_internal_extend(vm, object_array_internal);
    }
    else if ((order == VIRTUAL_MACHINE_OBJECT_ARRAY_INTERNAL_APPEND_TO_HEAD) && \
            (object_array_internal->pos == 0))
    {
        virtual_machine_object_array_internal_extend(vm, object_array_internal);
    }

    if ((ret = virtual_machine_variable_solve(&object_new_sub_solved, (struct virtual_machine_object *)object_new_sub, NULL, 1, vm)) != 0)
    { goto fail; }

    switch (order)
    {
        case VIRTUAL_MACHINE_OBJECT_ARRAY_INTERNAL_APPEND_TO_TAIL:
            object_array_internal->nodes[object_array_internal->pos + object_array_internal->size].ptr = object_new_sub_solved;
            break;
        case VIRTUAL_MACHINE_OBJECT_ARRAY_INTERNAL_APPEND_TO_HEAD:
            object_array_internal->pos -= 1;
            object_array_internal->nodes[object_array_internal->pos].ptr = object_new_sub_solved;
            break;
    }
    object_array_internal->size += 1;

    if (object_new_sub != NULL) {virtual_machine_object_destroy(vm, object_new_sub);object_new_sub = NULL;}

    goto done;
fail:
    if (object_new_sub_solved != NULL) virtual_machine_object_destroy(vm, object_new_sub_solved);
done:
    return ret;
}


/* Basic */

struct virtual_machine_object *virtual_machine_object_array_new( \
        struct virtual_machine *vm)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_array *new_object_array = NULL;

    /* Create array object */
    if ((new_object_array = (struct virtual_machine_object_array *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_array))) == NULL)
    { goto fail; }
    new_object_array->ptr_internal = NULL;
    new_object_array->ptr_internal = virtual_machine_object_array_internal_new(vm, 0);
    if (new_object_array->ptr_internal == NULL) { goto fail; }

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_ARRAY)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_array) != 0)
    { goto fail; }
    new_object_array = NULL;

    return new_object;
fail:
    if (new_object_array != NULL) 
    {
        if (new_object_array->ptr_internal != NULL) virtual_machine_object_array_internal_destroy(vm, new_object_array->ptr_internal);
        virtual_machine_resource_free(vm->resource, new_object_array);
    }
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);

    return NULL;
}

int virtual_machine_object_array_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    struct virtual_machine_object_array *object_array = NULL;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_array = (struct virtual_machine_object_array *)object->ptr;
    if (object_array != NULL)
    {
        virtual_machine_resource_free(vm->resource, object_array);
    }
    _virtual_machine_object_destroy(vm, object);

    return 0;
}

struct virtual_machine_object *virtual_machine_object_array_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_array *object_array = NULL;
    struct virtual_machine_object_array *new_object_array = NULL;

    if ((object == NULL) || (object->ptr == NULL)) return NULL;

    object_array = object->ptr;

    if ((new_object_array = (struct virtual_machine_object_array *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_array))) == NULL)
    { goto fail; }
    new_object_array->ptr_internal = NULL;
    new_object_array->ptr_internal = object_array->ptr_internal;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_ARRAY)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_array) != 0)
    { goto fail; }
    new_object_array = NULL;

    return new_object;
fail:
    return NULL;
}

int virtual_machine_object_array_print(const struct virtual_machine_object *object)
{
    struct virtual_machine_object_array *object_array;

    object_array = object->ptr;
    virtual_machine_object_array_internal_print(object_array->ptr_internal);

    return 0;
}

/* Size */
int virtual_machine_object_array_size( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, const struct virtual_machine_object *object_src)
{
    struct virtual_machine_object_array *object_array = object_src->ptr;
    return virtual_machine_object_array_internal_size(vm, object_out, object_array->ptr_internal);
}

/* Make */
int virtual_machine_object_array_make( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_top, const size_t count, int order, \
        struct virtual_machine_running_stack_frame *target_frame)
{
    int ret = 0;

    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_array *new_object_array = NULL;
    struct virtual_machine_object_array_internal *new_object_array_internal = NULL;

    /* Create array object */
    if ((new_object_array = (struct virtual_machine_object_array *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_array))) == NULL)
    { goto fail; }
    new_object_array->ptr_internal = NULL;

    if ((ret = virtual_machine_object_array_internal_make(vm, \
                    &new_object_array_internal, \
                    object_top, count, order, \
                    target_frame)) != 0)
    { goto fail; }

    new_object_array->ptr_internal = new_object_array_internal; new_object_array_internal = NULL;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_ARRAY)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_array) != 0)
    { goto fail; }

    if (virtual_machine_resource_reference_register( \
                vm->gc_stub, \
                new_object, \
                new_object_array->ptr_internal, \
                &virtual_machine_object_array_internal_marker, \
                &virtual_machine_object_array_internal_collector) != 0)
    { goto fail; }

    new_object_array = NULL;

    *object_out = new_object; new_object = NULL;
fail:
    if (new_object_array_internal)
    {
		virtual_machine_object_array_internal_destroy(vm, new_object_array_internal);
    }
    if (new_object_array != NULL) 
    {
        if (new_object_array->ptr_internal != NULL) virtual_machine_object_array_internal_destroy(vm, new_object_array->ptr_internal);
        virtual_machine_resource_free(vm->resource, new_object_array);
    }
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
    return ret;
}

/* Append */
int virtual_machine_object_array_append( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object, struct virtual_machine_object *object_new_sub)
{
    int ret = 0;
    struct virtual_machine_object_array *object_array = NULL;
    struct virtual_machine_object_array_internal *object_array_internal = NULL;

    object_array = object->ptr;
    object_array_internal = object_array->ptr_internal;

    if ((ret = virtual_machine_object_array_internal_append(vm, \
            object_array_internal, object_new_sub, VIRTUAL_MACHINE_OBJECT_ARRAY_INTERNAL_APPEND_TO_TAIL)) != 0)
    { goto fail; }

fail:
    return ret;
}

