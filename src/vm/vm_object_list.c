/* List Objects
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
int virtual_machine_object_list_internal_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_list_internal *object_list_internal);

#define VIRTUAL_MACHINE_OBJECT_LIST_INTERNAL_APPEND_TO_TAIL 0
#define VIRTUAL_MACHINE_OBJECT_LIST_INTERNAL_APPEND_TO_HEAD 1
int virtual_machine_object_list_internal_append(struct virtual_machine *vm, \
        struct virtual_machine_object_list_internal *object_list_internal, \
        struct virtual_machine_object *object_new_sub, int order);


/* Internal Marker */
int virtual_machine_object_list_internal_marker(void *object_internal)
{
    struct virtual_machine_object_list_internal *object_list_internal = object_internal;
    struct virtual_machine_object_list_internal_node *object_list_internal_node = object_list_internal->begin;

    while (object_list_internal_node != NULL)
    {
         virtual_machine_marks_object(object_list_internal_node->ptr, VIRTUAL_MACHINE_GARBAGE_COLLECT_MAJOR);
         object_list_internal_node = object_list_internal_node->next;
    }

    return 0;
}

/* Internal Collector */
int virtual_machine_object_list_internal_collector(void *object_internal, int *confirm)
{
    struct virtual_machine_object_list_internal *object_list_internal;

    *confirm = VIRTUAL_MACHINE_GARBAGE_COLLECT_CONFIRM; 

    object_list_internal = object_internal;
    return virtual_machine_object_list_internal_destroy(object_list_internal->vm, object_list_internal);
}


/* Index */

static struct virtual_machine_object_list_internal_index *virtual_machine_object_list_internal_index_new( \
        struct virtual_machine_object_list_internal *list_internal)
{
    struct virtual_machine_object_list_internal_index *new_index = NULL;
    struct virtual_machine_object_list_internal_node *list_internal_node_cur = NULL;
    struct virtual_machine_object_list_internal_node **internal_node_p;

    new_index = (struct virtual_machine_object_list_internal_index *)malloc \
                (sizeof(struct virtual_machine_object_list_internal_index));
    if (new_index == NULL) { goto fail; }
    new_index->size = list_internal->size;

    new_index->arr = (struct virtual_machine_object_list_internal_node **)malloc \
                     (sizeof(struct virtual_machine_object_list_internal_node *) * list_internal->size);
    if (new_index->arr == NULL) { goto fail; }

    internal_node_p = new_index->arr;
    list_internal_node_cur = list_internal->begin;
    while (list_internal_node_cur != NULL)
    {
        *internal_node_p = list_internal_node_cur;

        internal_node_p++;
        list_internal_node_cur = list_internal_node_cur->next; 
    }

fail:
    if (new_index != NULL)
    {
        if (new_index->arr != NULL) free(new_index->arr);
        free(new_index);
        new_index = NULL;
    }
    return new_index;
}

static int virtual_machine_object_list_internal_index_destroy(struct virtual_machine_object_list_internal_index *index)
{
    if (index->arr != NULL) free(index->arr);
    free(index);

    return 0;
}


/* Internal Part */

static struct virtual_machine_object_list_internal *virtual_machine_object_list_internal_new( \
        struct virtual_machine *vm)
{
    struct virtual_machine_object_list_internal *new_object_list_internal = NULL;

    /* Create the internal part of list instance */
    new_object_list_internal = (struct virtual_machine_object_list_internal *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_object_list_internal));
    if (new_object_list_internal == NULL)
    { goto fail; }

    new_object_list_internal->begin = new_object_list_internal->end = NULL;
    new_object_list_internal->size = 0;
    new_object_list_internal->index = NULL;
    new_object_list_internal->ref_by_idx_count = 0;
    new_object_list_internal->vm = vm;

    goto done;
fail:
    if (new_object_list_internal != NULL)
    {
        virtual_machine_resource_free(vm->resource, new_object_list_internal);
		new_object_list_internal = NULL;
    }
done:
    return new_object_list_internal;
}

int virtual_machine_object_list_internal_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_list_internal *object_list_internal)
{
    struct virtual_machine_object_list_internal_node *object_list_node_cur, *object_list_node_next;
    struct virtual_machine_object *virtual_machine_object_list_node_object;

    if (object_list_internal == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_list_node_cur = object_list_internal->begin;

    while (object_list_node_cur != NULL)
    {
        object_list_node_next = object_list_node_cur->next; 
        virtual_machine_object_list_node_object = object_list_node_cur->ptr;
        virtual_machine_object_destroy(vm, virtual_machine_object_list_node_object);
        virtual_machine_resource_free(vm->resource, object_list_node_cur);
        object_list_node_cur = object_list_node_next;
    }

    if (object_list_internal->index != NULL) 
    {
        virtual_machine_object_list_internal_index_destroy(object_list_internal->index);
    }

    virtual_machine_resource_free(vm->resource, object_list_internal);

    return 0;
}

static struct virtual_machine_object_list_internal *virtual_machine_object_list_internal_clone(struct virtual_machine *vm, \
        const struct virtual_machine_object_list_internal *object_list_internal)
{
    struct vm_err r_local;
    struct virtual_machine_object *new_sub_object = NULL;
    struct virtual_machine_object_list_internal *new_object_list_internal = NULL;
    struct virtual_machine_object_list_internal_node *object_list_internal_node_cur, *object_list_internal_node_next;
    struct virtual_machine_object *virtual_machine_object_list_node_object;

    if (object_list_internal == NULL) return NULL;

    if ((new_object_list_internal = virtual_machine_object_list_internal_new(vm)) == NULL)
    { goto fail; }

    object_list_internal_node_cur = object_list_internal->begin;

    vm_err_clear(&r_local);

    while (object_list_internal_node_cur != NULL)
    {
        object_list_internal_node_next = object_list_internal_node_cur->next; 
        virtual_machine_object_list_node_object = object_list_internal_node_cur->ptr;
        if ((new_sub_object = virtual_machine_object_clone(vm, virtual_machine_object_list_node_object)) == NULL)
        { goto fail; }
        if (virtual_machine_object_list_internal_append(vm, \
                    new_object_list_internal, new_sub_object, \
                    VIRTUAL_MACHINE_OBJECT_LIST_INTERNAL_APPEND_TO_TAIL) != 0)
        { goto fail; }
        new_sub_object = NULL;
        object_list_internal_node_cur = object_list_internal_node_next;
    }

    goto done;
fail:
    if (new_sub_object != NULL) virtual_machine_object_destroy(vm, new_sub_object);
    new_sub_object = NULL;
    if (new_object_list_internal != NULL) virtual_machine_object_list_internal_destroy(vm, new_object_list_internal);
    new_object_list_internal = NULL;
done:
    return new_object_list_internal;
}

static int virtual_machine_object_list_internal_print(const struct virtual_machine_object_list_internal *object_list_internal)
{
    struct virtual_machine_object_list_internal_node *object_list_internal_node_cur, *object_list_internal_node_next;
    struct virtual_machine_object *virtual_machine_object_list_internal_node_object;
    int first = 0;

    if (object_list_internal == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_list_internal_node_cur = object_list_internal->begin;

    printf("[");
    while (object_list_internal_node_cur != NULL)
    {
        object_list_internal_node_next = object_list_internal_node_cur->next; 
        virtual_machine_object_list_internal_node_object = object_list_internal_node_cur->ptr;

        if (first == 0) { first = 1; }
        else { printf(", "); }

        virtual_machine_object_print(virtual_machine_object_list_internal_node_object);
        object_list_internal_node_cur = object_list_internal_node_next;
    }

    printf("]");

    return 0;
}

/* size */
static int virtual_machine_object_list_internal_size(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object_list_internal *object_list_internal_src)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    if ((new_object = virtual_machine_object_int_new_with_value(vm, (int)(object_list_internal_src->size))) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    *object_out = new_object;
fail:
    return ret;
}

static int virtual_machine_object_list_internal_make(struct virtual_machine *vm, \
        struct virtual_machine_object_list_internal **object_out, \
        const struct virtual_machine_object *object_top, const size_t count, int order, \
        struct virtual_machine_running_stack_frame *target_frame)
{
    int ret = 0;

    int count_copy = (int)count;
    struct virtual_machine_object_list_internal *new_object_list_internal = NULL;
    struct virtual_machine_object *new_sub_object = NULL;
    struct virtual_machine_object *object_cur = NULL;

    *object_out = NULL;
    if ((new_object_list_internal = virtual_machine_object_list_internal_new(vm)) == NULL) { return -MULTIPLE_ERR_MALLOC; }

    object_cur = (struct virtual_machine_object *)object_top;
    while (count_copy-- != 0)
    {
        if ((ret = virtual_machine_variable_solve(&new_sub_object, (struct virtual_machine_object *)object_cur, target_frame, 1, vm)) != 0)
        { goto fail; }
        switch (order)
        {
            case VIRTUAL_MACHINE_OBJECT_LIST_MAKE_ORDER_DEFAULT:
                if ((ret = virtual_machine_object_list_internal_append(vm, \
                                new_object_list_internal, new_sub_object, VIRTUAL_MACHINE_OBJECT_LIST_INTERNAL_APPEND_TO_TAIL)) != 0) 
                { goto fail; }
                break;
            case VIRTUAL_MACHINE_OBJECT_LIST_MAKE_ORDER_REVERSE:
                if ((ret = virtual_machine_object_list_internal_append(vm, \
                                new_object_list_internal, new_sub_object, VIRTUAL_MACHINE_OBJECT_LIST_INTERNAL_APPEND_TO_HEAD)) != 0) 
                { goto fail; }
                break;
        }
        new_sub_object = NULL;
        object_cur = object_cur->prev;
    }

    *object_out = new_object_list_internal;

    ret = 0;
    goto done;
fail:
    if (new_object_list_internal != NULL) virtual_machine_object_list_internal_destroy(vm, new_object_list_internal);
done:
    if (new_sub_object != NULL) virtual_machine_object_destroy(vm, new_sub_object);
    return ret;
}

static int _virtual_machine_object_list_internal_ref_get_by_raw_index(struct virtual_machine_object **object_out,\
        struct virtual_machine_object_list_internal *object_list_internal_src, \
        int ref_index, \
        struct virtual_machine *vm)
{
    int ret = 0;
    struct virtual_machine_object_list_internal_node *list_internal_node_cur = NULL;
    struct virtual_machine_object *new_object = NULL;

    (void)vm;

    *object_out = NULL;

    if (object_list_internal_src->size == 0) 
    {
        vm_err_update(vm->r, -VM_ERR_OUT_OF_BOUNDS, \
                "runtime error: out of bounds, there is no element in list");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    if ((ref_index < 0) || (ref_index >= (signed int)(object_list_internal_src->size)))
    {
        vm_err_update(vm->r, -VM_ERR_OUT_OF_BOUNDS, \
                "runtime error: out of bounds, reference of element \'%d\' isn't in bound of %u to %u", \
                ref_index, 0, object_list_internal_src->size);
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    if (object_list_internal_src->index != NULL)
    {
        if ((new_object = virtual_machine_object_clone(vm, object_list_internal_src->index->arr[(size_t)ref_index]->ptr)) == NULL)
        {
            VM_ERR_MALLOC(vm->r);
            ret = -MULTIPLE_ERR_VM;
            goto fail;
        }
        *object_out = new_object;
        goto finish;
    }
    else
    {
        /* Search */
        list_internal_node_cur = object_list_internal_src->begin;
        while (list_internal_node_cur != NULL)
        {
            if (ref_index == 0)
            {
                if ((new_object = virtual_machine_object_clone(vm, list_internal_node_cur->ptr)) == NULL)
                {
                    VM_ERR_MALLOC(vm->r);
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }
                *object_out = new_object;
                goto finish;
            }
            list_internal_node_cur = list_internal_node_cur->next; 
            ref_index -= 1;
        }

        /* Increase counting for ref by index */
        object_list_internal_src->ref_by_idx_count++;
        /* Create index? */
        if (object_list_internal_src->ref_by_idx_count >= LIST_INTERNAL_INDEX_CREATE_THRESHOLD)
        {
            object_list_internal_src->index = virtual_machine_object_list_internal_index_new(object_list_internal_src);
            if (object_list_internal_src->index == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail; 
            }
        }
    }

finish:
fail:
    return ret;
}

int virtual_machine_object_list_ref_get(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_idx)
{
    int ret = 0;

    struct virtual_machine_object *object_src_solved= NULL;
    struct virtual_machine_object_list *object_src_solved_list = NULL;
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
                "runtime error: unsupported operand type, reference of list should be in type \'int\'");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }
    if (object_src_solved->type != OBJECT_TYPE_LIST)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    object_ref_index = (struct virtual_machine_object_int *)object_idx_solved->ptr;
    ref_index = object_ref_index->value;
    object_src_solved_list = object_src_solved->ptr;

    if ((ret = _virtual_machine_object_list_internal_ref_get_by_raw_index(object_out, object_src_solved_list->ptr_internal, ref_index, vm)) != 0)
    { goto fail; }

fail:
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved);
    if (object_idx_solved != NULL) virtual_machine_object_destroy(vm, object_idx_solved);
    return ret;
}

static int _virtual_machine_object_list_internal_ref_set_by_raw_index(const struct virtual_machine_object_list_internal *object_list_internal_src, \
        int ref_index, const struct virtual_machine_object *object_value, struct virtual_machine *vm)
{
    int ret = 0;
    struct virtual_machine_object_list_internal_node *list_internal_node_cur = NULL;

    (void)vm;

    if (object_list_internal_src->size == 0) 
    {
        vm_err_update(vm->r, -VM_ERR_OUT_OF_BOUNDS, \
                "runtime error: out of bounds, there is no element in list");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    if ((ref_index < 0) || (ref_index >= (signed int)(object_list_internal_src->size)))
    {
        vm_err_update(vm->r, -VM_ERR_OUT_OF_BOUNDS, \
                "runtime error: out of bounds, reference of element \'%d\' isn't in bound of %u to %u", \
                ref_index, 0, object_list_internal_src->size);
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    list_internal_node_cur = object_list_internal_src->begin;
    while (list_internal_node_cur != NULL)
    {
        if (ref_index == 0)
        {
            virtual_machine_object_destroy(vm, list_internal_node_cur->ptr);
            list_internal_node_cur->ptr = NULL;
            list_internal_node_cur->ptr = virtual_machine_object_clone(vm, object_value);
            if (list_internal_node_cur->ptr == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            goto finish;
        }
        list_internal_node_cur = list_internal_node_cur->next; 
        ref_index -= 1;
    }
finish:
fail:
    return ret;
}

int virtual_machine_object_list_ref_set(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_idx, \
        const struct virtual_machine_object *object_value)
{
    int ret = 0;

    struct virtual_machine_object *object_src_solved= NULL;
    struct virtual_machine_object_list *object_src_solved_list = NULL;
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
                "runtime error: unsupported operand type, reference of list should be in type \'int\'");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }
    if (object_src_solved->type != OBJECT_TYPE_LIST)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    object_ref_index = (struct virtual_machine_object_int *)object_idx_solved->ptr;
    ref_index = object_ref_index->value;

    object_src_solved_list = object_src_solved->ptr;
    if ((ret = _virtual_machine_object_list_internal_ref_set_by_raw_index(object_src_solved_list->ptr_internal, ref_index, object_value_solved, vm)) != 0)
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

int virtual_machine_object_list_car(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src)
{
    int ret = 0;

    struct virtual_machine_object *object_src_solved = NULL;
    struct virtual_machine_object_list *object_list = NULL;

    *object_out = NULL;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }

    if (object_src_solved->type != OBJECT_TYPE_LIST)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }
    object_list = object_src_solved->ptr;
    if ((ret = _virtual_machine_object_list_internal_ref_get_by_raw_index(object_out, object_list->ptr_internal, 0, vm)) != 0)
    { goto fail; }

fail:
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved);
    return ret;
}

static int virtual_machine_object_list_internal_cdr( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_list_internal **object_list_internal_out, \
        const struct virtual_machine_object_list_internal *object_list_internal_src)
{
    int ret = 0;

    struct virtual_machine_object_list_internal *new_object_list_internal = NULL;
    struct virtual_machine_object_list_internal_node *object_list_internal_node_first = NULL;

    *object_list_internal_out = NULL;

    /* Element number checking */
    if (object_list_internal_src->size == 0)
    {
        vm_err_update(vm->r, -VM_ERR_EMPTY_LIST, \
                "runtime error: empty list has no element");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    /* Complete clone the internal of original list */
    if ((new_object_list_internal = virtual_machine_object_list_internal_clone(vm, object_list_internal_src)) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }
    
    if (new_object_list_internal->size != 0)
    {

        /* Remove the first one */
        object_list_internal_node_first = new_object_list_internal->begin;
        new_object_list_internal->begin = new_object_list_internal->begin->next;

        if ((ret = virtual_machine_object_destroy(vm, object_list_internal_node_first->ptr)) != 0) 
        {
            VM_ERR_INTERNAL(vm->r);
            ret = -MULTIPLE_ERR_VM;
            goto fail; 
        }
        virtual_machine_resource_free(vm->resource, object_list_internal_node_first);

        /* Decrease size */
        new_object_list_internal->size -= 1;
    }

    *object_list_internal_out = new_object_list_internal;
    new_object_list_internal = NULL;

    ret = 0;
fail:
    if (new_object_list_internal != NULL) virtual_machine_object_list_internal_destroy(vm, new_object_list_internal);
    return ret;
}

int virtual_machine_object_list_cdr( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_list *new_object_list = NULL;
    struct virtual_machine_object_list_internal *new_object_list_internal = NULL;

    struct virtual_machine_object *object_src_solved = NULL;
    struct virtual_machine_object_list *object_list_src = NULL;
    struct virtual_machine_object_list_internal *object_list_internal_src = NULL;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }

    object_list_src = object_src_solved->ptr;
    object_list_internal_src = object_list_src->ptr_internal;

    /* Create a 'cdred' internal */
    ret = virtual_machine_object_list_internal_cdr( \
            vm, \
            &new_object_list_internal, \
            object_list_internal_src);
    if (ret != 0) { goto fail; }

    /* Create the shell part */
    if ((new_object_list = (struct virtual_machine_object_list *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_list))) == NULL)
    { goto fail; }
    new_object_list->ptr_internal = new_object_list_internal; new_object_list_internal = NULL;

    /* Create the list's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_LIST)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_list) != 0)
    { goto fail; }

    if (virtual_machine_resource_reference_register( \
                vm->gc_stub, \
                new_object, \
                new_object_list->ptr_internal, \
                &virtual_machine_object_list_internal_marker, \
                &virtual_machine_object_list_internal_collector) != 0)
    { goto fail; }

    new_object_list = NULL;

    *object_out = new_object; new_object = NULL;

fail:
    if (new_object_list_internal != NULL) virtual_machine_object_list_internal_destroy(vm, new_object_list_internal);
    if (new_object_list != NULL) virtual_machine_resource_free(vm->resource, new_object_list);
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved);
    return ret;
}

int virtual_machine_object_list_cdr_set( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_member)
{
    int ret = 0;

    struct virtual_machine_object *object_src_solved = NULL;
    struct virtual_machine_object *object_member_solved = NULL;
    struct virtual_machine_object *new_object = NULL;

    struct virtual_machine_object_list *object_list = NULL;
    struct virtual_machine_object_list_internal *object_list_internal = NULL;
    struct virtual_machine_object_list_internal_node *object_list_internal_node = NULL;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }
    if ((ret = virtual_machine_variable_solve(&object_member_solved, (struct virtual_machine_object *)object_member, NULL, 1, vm)) != 0)
    { goto fail; }

    if (object_src_solved->type != OBJECT_TYPE_LIST)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    if (object_src_solved->type != OBJECT_TYPE_LIST)
    {
        /* cdr is a list, so the result is a list */

        if ((new_object = virtual_machine_object_list_new(vm)) == NULL)
        { goto fail; }

        /* Append the car from the src */
        object_list = object_src_solved->ptr;
        object_list_internal = object_list->ptr_internal;
        object_list_internal_node = object_list_internal->begin; 
        if ((ret = virtual_machine_object_list_append(vm, \
                        new_object, \
                        object_list_internal_node->ptr)) != 0)
        { goto fail; }

        /* Append the cdr part */
        object_list_internal = object_member_solved->ptr;
        object_list_internal_node = object_list_internal->begin; 
        while (object_list_internal_node != NULL)
        {
            if ((ret = virtual_machine_object_list_append(vm, \
                            new_object, \
                            object_list_internal_node->ptr)) != 0)
            { goto fail; }
            object_list_internal_node = object_list_internal_node->next; 
        }
    }
    else
    {
        /* cdr is not a list, so the result is a pair */

        object_list = object_src_solved->ptr;
        object_list_internal = object_list->ptr_internal;
        if (object_list_internal->size == 0)
        {
            vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                    "runtime error: empty list");
            ret = -MULTIPLE_ERR_VM;
            goto fail; 
        }
        if ((new_object = virtual_machine_object_pair_make(vm, \
                        object_list_internal->begin->ptr, object_member_solved)) == NULL)
        { goto fail; }
    }

    *object_out = new_object; new_object = NULL;

fail:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved);
    if (object_member_solved != NULL) virtual_machine_object_destroy(vm, object_member_solved);
    return ret;
}

int virtual_machine_object_list_internal_append(struct virtual_machine *vm, \
        struct virtual_machine_object_list_internal *object_list_internal, \
        struct virtual_machine_object *object_new_sub, int order)
{
    int ret = 0;
    struct virtual_machine_object_list_internal_node *new_object_list_internal_node;

    struct virtual_machine_object *object_new_sub_solved = NULL;

    if (object_list_internal == NULL) return -MULTIPLE_ERR_NULL_PTR;

    /* Clean index */
    if (object_list_internal->index != NULL) 
    {
        virtual_machine_object_list_internal_index_destroy(object_list_internal->index);
        object_list_internal->index = NULL;
        object_list_internal->ref_by_idx_count = 0;
    }

    if ((ret = virtual_machine_variable_solve(&object_new_sub_solved, (struct virtual_machine_object *)object_new_sub, NULL, 1, vm)) != 0)
    { goto fail; }

    new_object_list_internal_node = (struct virtual_machine_object_list_internal_node *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_object_list_internal_node));
    if (new_object_list_internal_node == NULL)
    {
        ret = -MULTIPLE_ERR_NULL_PTR;
        goto fail;
    }
    new_object_list_internal_node->next = NULL;
    new_object_list_internal_node->prev = NULL;
    new_object_list_internal_node->ptr = (struct virtual_machine_object *)object_new_sub_solved;
    object_new_sub_solved = NULL;

    if (object_list_internal->begin == NULL)
    {
        object_list_internal->begin = object_list_internal->end = new_object_list_internal_node;
    }
    else
    {
        switch (order)
        {
            case VIRTUAL_MACHINE_OBJECT_LIST_INTERNAL_APPEND_TO_TAIL:
                new_object_list_internal_node->prev = object_list_internal->end;
                object_list_internal->end->next = new_object_list_internal_node;
                object_list_internal->end = new_object_list_internal_node;
                break;
            case VIRTUAL_MACHINE_OBJECT_LIST_INTERNAL_APPEND_TO_HEAD:
                object_list_internal->begin->prev = new_object_list_internal_node;
                new_object_list_internal_node->next = object_list_internal->begin;
                object_list_internal->begin = new_object_list_internal_node;
                break;
        }
    }
    object_list_internal->size++;
    new_object_list_internal_node = NULL;
    if (object_new_sub != NULL) {virtual_machine_object_destroy(vm, object_new_sub);object_new_sub = NULL;}

    goto done;
fail:
    if (object_new_sub_solved != NULL) virtual_machine_object_destroy(vm, object_new_sub_solved);
done:
    return ret;
}

static int virtual_machine_object_list_internal_unpack( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_list_internal *object_list_internal, \
        int order)
{
    int ret = 0;
    struct virtual_machine_object_list_internal_node *list_internal_node_cur = NULL;
    struct virtual_machine_object *new_object = NULL;

    switch (order)
    {
        case VIRTUAL_MACHINE_OBJECT_LIST_UNPACK_ORDER_DEFAULT:
            list_internal_node_cur = object_list_internal->end;
            break;
        case VIRTUAL_MACHINE_OBJECT_LIST_UNPACK_ORDER_REVERSE:
            list_internal_node_cur = object_list_internal->begin;
            break;
    }
    while (list_internal_node_cur != NULL)
    {
        if ((new_object = virtual_machine_object_clone(vm, list_internal_node_cur->ptr)) == NULL)
        {
            VM_ERR_MALLOC(vm->r);
            ret = -MULTIPLE_ERR_VM;
            goto fail;
        }

        if ((ret = virtual_machine_computing_stack_push(vm->tp->running_stack->top->computing_stack, \
                        new_object)) != 0)
        { VM_ERR_INTERNAL(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
        new_object = NULL;

        switch (order)
        {
            case VIRTUAL_MACHINE_OBJECT_LIST_UNPACK_ORDER_DEFAULT:
                list_internal_node_cur = list_internal_node_cur->prev; 
                break;
            case VIRTUAL_MACHINE_OBJECT_LIST_UNPACK_ORDER_REVERSE:
                list_internal_node_cur = list_internal_node_cur->next; 
                break;
        }
    }

    if ((new_object = virtual_machine_object_int_new_with_value( \
                    vm, \
                    (int)(object_list_internal->size))) == NULL)
    { goto fail; }
    if ((ret = virtual_machine_computing_stack_push(vm->tp->running_stack->top->computing_stack, \
                    new_object)) != 0)
    { VM_ERR_INTERNAL(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
    new_object = NULL;

    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
done:
    return ret;
}


/* Shell */

struct virtual_machine_object *virtual_machine_object_list_new( \
        struct virtual_machine *vm)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_list *new_object_list = NULL;

    /* Create list object */
    if ((new_object_list = (struct virtual_machine_object_list *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_list))) == NULL)
    { goto fail; }
    new_object_list->ptr_internal = NULL;
    new_object_list->ptr_internal = virtual_machine_object_list_internal_new(vm);
    if (new_object_list->ptr_internal == NULL) { goto fail; }

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_LIST)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_list) != 0)
    { goto fail; }
    new_object_list = NULL;

    return new_object;
fail:
    if (new_object_list != NULL) 
    {
        if (new_object_list->ptr_internal != NULL) virtual_machine_object_list_internal_destroy(vm, new_object_list->ptr_internal);
        virtual_machine_resource_free(vm->resource, new_object_list);
    }
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);

    return NULL;
}

int virtual_machine_object_list_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    struct virtual_machine_object_list *object_list = NULL;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_list = (struct virtual_machine_object_list *)object->ptr;
    if (object_list != NULL)
    {
        virtual_machine_resource_free(vm->resource, object_list);
    }
    _virtual_machine_object_destroy(vm, object);

    return 0;
}

struct virtual_machine_object *virtual_machine_object_list_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_list *object_list = NULL;
    struct virtual_machine_object_list *new_object_list = NULL;

    if ((object == NULL) || (object->ptr == NULL)) return NULL;

    object_list = object->ptr;

    if ((new_object_list = (struct virtual_machine_object_list *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_list))) == NULL)
    { goto fail; }
    new_object_list->ptr_internal = NULL;
    new_object_list->ptr_internal = object_list->ptr_internal;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_LIST)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_list) != 0)
    { goto fail; }
    new_object_list = NULL;

    return new_object;
fail:
    return NULL;
}

int virtual_machine_object_list_print(const struct virtual_machine_object *object)
{
    struct virtual_machine_object_list *object_list;

    object_list = object->ptr;
    virtual_machine_object_list_internal_print(object_list->ptr_internal);

    return 0;
}

/* Size */
int virtual_machine_object_list_size( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, const struct virtual_machine_object *object_src)
{
    struct virtual_machine_object_list *object_list = object_src->ptr;
    return virtual_machine_object_list_internal_size(vm, object_out, object_list->ptr_internal);
}

/* Make */
int virtual_machine_object_list_make( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_top, const size_t count, int order, \
        struct virtual_machine_running_stack_frame *target_frame)
{
    int ret = 0;

    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_list *new_object_list = NULL;
    struct virtual_machine_object_list_internal *new_object_list_internal = NULL;

    /* Create list object */
    if ((new_object_list = (struct virtual_machine_object_list *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_list))) == NULL)
    { goto fail; }
    new_object_list->ptr_internal = NULL;

    if ((ret = virtual_machine_object_list_internal_make(vm, \
                    &new_object_list_internal, \
                    object_top, count, order, \
                    target_frame)) != 0)
    { goto fail; }

    new_object_list->ptr_internal = new_object_list_internal; new_object_list_internal = NULL;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_LIST)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_list) != 0)
    { goto fail; }

    if (virtual_machine_resource_reference_register( \
                vm->gc_stub, \
                new_object, \
                new_object_list->ptr_internal, \
                &virtual_machine_object_list_internal_marker, \
                &virtual_machine_object_list_internal_collector) != 0)
    { goto fail; }

    new_object_list = NULL;

    *object_out = new_object; new_object = NULL;
fail:
    if (new_object_list_internal)
    {
		virtual_machine_object_list_internal_destroy(vm, new_object_list_internal);
    }
    if (new_object_list != NULL) 
    {
        if (new_object_list->ptr_internal != NULL) virtual_machine_object_list_internal_destroy(vm, new_object_list->ptr_internal);
        virtual_machine_resource_free(vm->resource, new_object_list);
    }
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
    return ret;
}

/* Append */
int virtual_machine_object_list_append( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object, struct virtual_machine_object *object_new_sub)
{
    int ret = 0;
    struct virtual_machine_object_list *object_list = NULL;
    struct virtual_machine_object_list_internal *object_list_internal = NULL;

    object_list = object->ptr;
    object_list_internal = object_list->ptr_internal;

    if ((ret = virtual_machine_object_list_internal_append(vm, \
            object_list_internal, object_new_sub, VIRTUAL_MACHINE_OBJECT_LIST_INTERNAL_APPEND_TO_TAIL)) != 0)
    { goto fail; }

fail:
    return ret;
}

int virtual_machine_object_list_append_to_head( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object, struct virtual_machine_object *object_new_sub)
{
    int ret = 0;
    struct virtual_machine_object_list *object_list = NULL;
    struct virtual_machine_object_list_internal *object_list_internal = NULL;

    object_list = object->ptr;
    object_list_internal = object_list->ptr_internal;

    if ((ret = virtual_machine_object_list_internal_append(vm, \
            object_list_internal, object_new_sub, VIRTUAL_MACHINE_OBJECT_LIST_INTERNAL_APPEND_TO_HEAD)) != 0)
    { goto fail; }

fail:
    return ret;
}

/* Unpack */
int virtual_machine_object_list_unpack( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object, \
        int order)
{
    int ret = 0;
    struct virtual_machine_object_list *object_list = NULL;
    struct virtual_machine_object_list_internal *object_list_internal = NULL;

    object_list = object->ptr;
    object_list_internal = object_list->ptr_internal;

    if ((ret = virtual_machine_object_list_internal_unpack( \
                    vm, \
                    object_list_internal, \
                    order)) != 0)
    { goto fail; }

fail:
    return ret;
}

