/* Hash Objects
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
static int virtual_machine_object_hash_internal_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_hash_internal *object_hash_internal);

static int virtual_machine_object_hash_internal_append(struct virtual_machine *vm, \
        struct virtual_machine_object_hash_internal *object_hash_internal, \
        struct virtual_machine_object *object_new_sub_key, \
        struct virtual_machine_object *object_new_sub_value);
static int virtual_machine_object_hash_internal_exists( \
        struct virtual_machine_object_hash_internal_node **object_hash_node_out, \
        struct virtual_machine_object_hash_internal *object_hash_internal, \
        const struct virtual_machine_object *object_key);


/* Internal Marker */
int virtual_machine_object_hash_internal_marker(void *object_internal)
{
    struct virtual_machine_object_hash_internal *object_hash_internal = object_internal;
    struct virtual_machine_object_hash_internal_node *object_hash_internal_node = object_hash_internal->begin;

    while (object_hash_internal_node != NULL)
    {
         virtual_machine_marks_object(object_hash_internal_node->ptr_key, VIRTUAL_MACHINE_GARBAGE_COLLECT_MAJOR);
         virtual_machine_marks_object(object_hash_internal_node->ptr_value, VIRTUAL_MACHINE_GARBAGE_COLLECT_MAJOR);

         object_hash_internal_node = object_hash_internal_node->next;
    }

    return 0;
}

/* Internal Collector */
int virtual_machine_object_hash_internal_collector(void *object_internal, int *confirm)
{
    struct virtual_machine_object_hash_internal *object_hash_internal;

    *confirm = VIRTUAL_MACHINE_GARBAGE_COLLECT_CONFIRM; 

    object_hash_internal = object_internal;
    return virtual_machine_object_hash_internal_destroy(object_hash_internal->vm, object_hash_internal);
}


/* Internal Part */

static struct virtual_machine_object_hash_internal *virtual_machine_object_hash_internal_new( \
        struct virtual_machine *vm)
{
    struct virtual_machine_object_hash_internal *new_object_hash_internal = NULL;

    /* Create the internal part of hash instance */
    new_object_hash_internal = (struct virtual_machine_object_hash_internal *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_object_hash_internal));
    if (new_object_hash_internal == NULL)
    { goto fail; }

    new_object_hash_internal->begin = new_object_hash_internal->end = NULL;
    new_object_hash_internal->size = 0;
    new_object_hash_internal->vm = vm;

    goto done;
fail:
    if (new_object_hash_internal != NULL)
    {
        virtual_machine_resource_free(vm->resource, new_object_hash_internal);
		new_object_hash_internal = NULL;
    }
done:
    return new_object_hash_internal;
}

static int virtual_machine_object_hash_internal_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_hash_internal *object_hash_internal)
{
    struct virtual_machine_object_hash_internal_node *object_hash_node_cur, *object_hash_node_next;
    struct virtual_machine_object *virtual_machine_object_hash_node_object;

    if (object_hash_internal == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_hash_node_cur = object_hash_internal->begin;

    while (object_hash_node_cur != NULL)
    {
        object_hash_node_next = object_hash_node_cur->next; 

        virtual_machine_object_hash_node_object = object_hash_node_cur->ptr_key;
        virtual_machine_object_destroy(vm, virtual_machine_object_hash_node_object);
        virtual_machine_object_hash_node_object = object_hash_node_cur->ptr_value;
        virtual_machine_object_destroy(vm, virtual_machine_object_hash_node_object);
        virtual_machine_resource_free(vm->resource, object_hash_node_cur);

        object_hash_node_cur = object_hash_node_next;
    }

    virtual_machine_resource_free(vm->resource, object_hash_internal);

    return 0;
}

/*
static struct virtual_machine_object_hash_internal *virtual_machine_object_hash_internal_clone(struct virtual_machine *vm, \
        const struct virtual_machine_object_hash_internal *object_hash_internal)
{
    struct vm_err r_local;
    struct virtual_machine_object *new_sub_object_key = NULL;
    struct virtual_machine_object *new_sub_object_value = NULL;
    struct virtual_machine_object_hash_internal *new_object_hash_internal = NULL;
    struct virtual_machine_object_hash_internal_node *object_hash_internal_node_cur, *object_hash_internal_node_next;
    struct virtual_machine_object *virtual_machine_object_hash_node_object;

    if (object_hash_internal == NULL) return NULL;

    if ((new_object_hash_internal = virtual_machine_object_hash_internal_new(vm)) == NULL)
    { goto fail; }

    object_hash_internal_node_cur = object_hash_internal->begin;

    vm_err_clear(&r_local);

    while (object_hash_internal_node_cur != NULL)
    {
        object_hash_internal_node_next = object_hash_internal_node_cur->next; 

        virtual_machine_object_hash_node_object = object_hash_internal_node_cur->ptr_key;
        if ((new_sub_object_key = virtual_machine_object_clone(vm, virtual_machine_object_hash_node_object)) == NULL)
        { goto fail; }
        virtual_machine_object_hash_node_object = object_hash_internal_node_cur->ptr_value;
        if ((new_sub_object_value = virtual_machine_object_clone(vm, virtual_machine_object_hash_node_object)) == NULL)
        { goto fail; }
        if (virtual_machine_object_hash_internal_append(vm, \
                    new_object_hash_internal, \
                    new_sub_object_key, \
                    new_sub_object_value) != 0)
        { goto fail; }
        new_sub_object_key = NULL;
        new_sub_object_value = NULL;

        object_hash_internal_node_cur = object_hash_internal_node_next;
    }

    goto done;
fail:
    if (new_sub_object_key != NULL) virtual_machine_object_destroy(vm, new_sub_object_key);
    new_sub_object_key = NULL;
    if (new_sub_object_value != NULL) virtual_machine_object_destroy(vm, new_sub_object_value);
    new_sub_object_value = NULL;
    if (new_object_hash_internal != NULL) virtual_machine_object_hash_internal_destroy(vm, new_object_hash_internal);
    new_object_hash_internal = NULL;
done:
    return new_object_hash_internal;
}
*/

static int virtual_machine_object_hash_internal_print(const struct virtual_machine_object_hash_internal *object_hash_internal)
{
    struct virtual_machine_object_hash_internal_node *object_hash_internal_node_cur, *object_hash_internal_node_next;
    struct virtual_machine_object *virtual_machine_object_hash_internal_node_object;
    int first = 0;

    if (object_hash_internal == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_hash_internal_node_cur = object_hash_internal->begin;

    printf("{");
    while (object_hash_internal_node_cur != NULL)
    {

        if (first == 0) { first = 1; }
        else { printf(", "); }
        object_hash_internal_node_next = object_hash_internal_node_cur->next; 

        virtual_machine_object_hash_internal_node_object = object_hash_internal_node_cur->ptr_key;
        virtual_machine_object_print(virtual_machine_object_hash_internal_node_object);

        printf(" => ");

        virtual_machine_object_hash_internal_node_object = object_hash_internal_node_cur->ptr_value;
        virtual_machine_object_print(virtual_machine_object_hash_internal_node_object);

        object_hash_internal_node_cur = object_hash_internal_node_next;
    }

    printf("}");

    return 0;
}

/* size */
static int virtual_machine_object_hash_internal_size(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object_hash_internal *object_hash_internal_src)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    if ((new_object = virtual_machine_object_int_new_with_value(vm, (int)(object_hash_internal_src->size))) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    *object_out = new_object;
fail:
    return ret;
}

/* make */
static int virtual_machine_object_hash_internal_make( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_hash_internal **object_hash_internal_out, \
        const struct virtual_machine_object *object_top, \
        const size_t count, \
        struct virtual_machine_running_stack_frame *target_frame)
{
    int ret = 0;

    int count_copy = (int)count;
    struct virtual_machine_object_hash_internal *new_object_hash_internal = NULL;
    struct virtual_machine_object *new_sub_object_key = NULL;
    struct virtual_machine_object *new_sub_object_value = NULL;
    struct virtual_machine_object *object_cur = NULL;

    *object_hash_internal_out = NULL;
    if ((new_object_hash_internal = virtual_machine_object_hash_internal_new(vm)) == NULL) { return -MULTIPLE_ERR_MALLOC; }

    object_cur = (struct virtual_machine_object *)object_top;
    while (count_copy-- != 0)
    {
        if ((ret = virtual_machine_variable_solve(&new_sub_object_value, (struct virtual_machine_object *)object_cur, target_frame, 1, vm)) != 0)
        { goto fail; }
        object_cur = object_cur->prev;
        if ((ret = virtual_machine_variable_solve(&new_sub_object_key, (struct virtual_machine_object *)object_cur, target_frame, 1, vm)) != 0)
        { goto fail; }
        object_cur = object_cur->prev;
        if ((ret = virtual_machine_object_hash_internal_append(vm, \
                        new_object_hash_internal, \
                        new_sub_object_key, new_sub_object_value)) != 0) 
        { goto fail; }
        new_sub_object_key = NULL;
        new_sub_object_value = NULL;
    }

    *object_hash_internal_out = new_object_hash_internal;

    ret = 0;
    goto done;
fail:
    if (new_object_hash_internal != NULL) virtual_machine_object_hash_internal_destroy(vm, new_object_hash_internal);
done:
    if (new_sub_object_key != NULL) virtual_machine_object_destroy(vm, new_sub_object_key);
    if (new_sub_object_value != NULL) virtual_machine_object_destroy(vm, new_sub_object_value);
    return ret;
}

static int virtual_machine_object_hash_internal_append( \
        struct virtual_machine *vm, 
        struct virtual_machine_object_hash_internal *object_hash_internal_src, \
        struct virtual_machine_object *object_new_sub_key, \
        struct virtual_machine_object *object_new_sub_value)
{
    int ret = 0;
    struct virtual_machine_object_hash_internal_node *new_object_hash_internal_node = NULL;
    struct virtual_machine_object_hash_internal_node *exists_object_hash_internal_node = NULL;

    if (object_hash_internal_src == NULL) return -MULTIPLE_ERR_NULL_PTR;

    (void)vm;

    if (virtual_machine_object_hash_internal_exists( \
                &exists_object_hash_internal_node, \
                object_hash_internal_src, \
                object_new_sub_key) == 0)
    {
        new_object_hash_internal_node = (struct virtual_machine_object_hash_internal_node *)virtual_machine_resource_malloc( \
                vm->resource, sizeof(struct virtual_machine_object_hash_internal_node));
        if (new_object_hash_internal_node == NULL)
        {
            goto fail;
        }
        new_object_hash_internal_node->ptr_key = (struct virtual_machine_object *)object_new_sub_key;
        new_object_hash_internal_node->ptr_value = (struct virtual_machine_object *)object_new_sub_value;
        new_object_hash_internal_node->next = NULL;
        new_object_hash_internal_node->prev = NULL;

        if (object_hash_internal_src->begin == NULL)
        {
            object_hash_internal_src->begin = object_hash_internal_src->end = new_object_hash_internal_node;
        }
        else
        {
            new_object_hash_internal_node->prev = object_hash_internal_src->end;
            object_hash_internal_src->end->next = new_object_hash_internal_node;
            object_hash_internal_src->end = new_object_hash_internal_node;
        }
        object_hash_internal_src->size++;
    }
    else
    {
        /*virtual_machine_object_destroy(vm, object_new_sub_key);*/
        if (exists_object_hash_internal_node->ptr_value != NULL)
        {
            virtual_machine_object_destroy(vm, exists_object_hash_internal_node->ptr_value);
            exists_object_hash_internal_node->ptr_value = NULL;
        }
        exists_object_hash_internal_node->ptr_value = (struct virtual_machine_object *)object_new_sub_value;
    }

    goto done;
fail:
    if (new_object_hash_internal_node != NULL) virtual_machine_resource_free(vm->resource, new_object_hash_internal_node);
done:
    return ret;
}


struct virtual_machine_object *virtual_machine_object_hash_new( \
        struct virtual_machine *vm)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_hash *new_object_hash = NULL;

    /* Create hash object */
    if ((new_object_hash = (struct virtual_machine_object_hash *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_hash))) == NULL)
    { goto fail; }
    new_object_hash->ptr_internal = NULL;
    new_object_hash->ptr_internal = virtual_machine_object_hash_internal_new(vm);
    if (new_object_hash->ptr_internal == NULL) { goto fail; }

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_HASH)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_hash) != 0)
    { goto fail; }
    new_object_hash = NULL;

    return new_object;
fail:
    if (new_object_hash != NULL) 
    {
        if (new_object_hash->ptr_internal != NULL) virtual_machine_object_hash_internal_destroy(vm, new_object_hash->ptr_internal);
        virtual_machine_resource_free(vm->resource, new_object_hash);
    }
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);

    return NULL;
}

int virtual_machine_object_hash_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    struct virtual_machine_object_hash *object_hash = NULL;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_hash = (struct virtual_machine_object_hash *)object->ptr;
    if (object_hash != NULL)
    {
        virtual_machine_resource_free(vm->resource, object_hash);
    }
    _virtual_machine_object_destroy(vm, object);

    return 0;
}

struct virtual_machine_object *virtual_machine_object_hash_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_hash *object_hash = NULL;
    struct virtual_machine_object_hash *new_object_hash = NULL;

    if ((object == NULL) || (object->ptr == NULL)) return NULL;

    object_hash = object->ptr;

    if ((new_object_hash = (struct virtual_machine_object_hash *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_hash))) == NULL)
    { goto fail; }
    new_object_hash->ptr_internal = NULL;
    new_object_hash->ptr_internal = object_hash->ptr_internal;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_HASH)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_hash) != 0)
    { goto fail; }
    new_object_hash = NULL;

    return new_object;
fail:
    return NULL;
}

int virtual_machine_object_hash_print(const struct virtual_machine_object *object)
{
    struct virtual_machine_object_hash *object_hash;

    object_hash = object->ptr;
    virtual_machine_object_hash_internal_print(object_hash->ptr_internal);

    return 0;
}

/* Size */
int virtual_machine_object_hash_size( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, const struct virtual_machine_object *object_src)
{
    struct virtual_machine_object_hash *object_hash = object_src->ptr;
    return virtual_machine_object_hash_internal_size(vm, object_out, object_hash->ptr_internal);
}

/* Make */
int virtual_machine_object_hash_make( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_top, const size_t count, \
        struct virtual_machine_running_stack_frame *target_frame)
{
    int ret = 0;

    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_hash *new_object_hash = NULL;
    struct virtual_machine_object_hash_internal *new_object_hash_internal = NULL;

    /* Create hash object */
    if ((new_object_hash = (struct virtual_machine_object_hash *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_hash))) == NULL)
    { goto fail; }
    new_object_hash->ptr_internal = NULL;

    if ((ret = virtual_machine_object_hash_internal_make(vm, \
                    &new_object_hash_internal, \
                    object_top, count, \
                    target_frame)) != 0)
    { goto fail; }

    new_object_hash->ptr_internal = new_object_hash_internal; new_object_hash_internal = NULL;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_HASH)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_hash) != 0)
    { goto fail; }

    if (virtual_machine_resource_reference_register( \
                vm->gc_stub, \
                new_object, \
                new_object_hash->ptr_internal, \
                &virtual_machine_object_hash_internal_marker, \
                &virtual_machine_object_hash_internal_collector) != 0)
    { goto fail; }

    new_object_hash = NULL;

    *object_out = new_object; new_object = NULL;
fail:
    if (new_object_hash_internal)
    {
		virtual_machine_object_hash_internal_destroy(vm, new_object_hash_internal);
    }
    if (new_object_hash != NULL) 
    {
        if (new_object_hash->ptr_internal != NULL) virtual_machine_object_hash_internal_destroy(vm, new_object_hash->ptr_internal);
        virtual_machine_resource_free(vm->resource, new_object_hash);
    }
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
    return ret;
}

/* Append */
int virtual_machine_object_hash_append( \
        struct virtual_machine *vm, 
        struct virtual_machine_object *object, \
        struct virtual_machine_object *object_new_sub_key, \
        struct virtual_machine_object *object_new_sub_value)
{
    int ret = 0;
    struct virtual_machine_object_hash *object_hash = NULL;
    struct virtual_machine_object_hash_internal *object_hash_internal = NULL;

    object_hash = object->ptr;
    object_hash_internal = object_hash->ptr_internal;

    if ((ret = virtual_machine_object_hash_internal_append(vm, \
            object_hash_internal, object_new_sub_key, object_new_sub_value)) != 0)
    { goto fail; }

fail:
    return ret;
}

static int virtual_machine_object_hash_internal_exists( \
        struct virtual_machine_object_hash_internal_node **object_hash_node_out, \
        struct virtual_machine_object_hash_internal *object_hash_internal, \
        const struct virtual_machine_object *object_key)
{
    struct virtual_machine_object_hash_internal_node *object_hash_internal_node;

    object_hash_internal_node = object_hash_internal->begin;
    while (object_hash_internal_node != NULL)
    {
        if (objects_eq(object_hash_internal_node->ptr_key, object_key) == OBJECTS_EQ)
        {
            *object_hash_node_out = object_hash_internal_node;
            return 1;
        }
        object_hash_internal_node = object_hash_internal_node->next;
    }
    *object_hash_node_out = NULL;
    return 0;
}

int virtual_machine_object_hash_remove(struct virtual_machine_object *object, \
        struct virtual_machine_object *object_sub_key, \
        struct virtual_machine *vm)
{
    struct virtual_machine_object_hash *exists_object_hash = NULL;
    struct virtual_machine_object_hash_internal *exists_object_hash_internal = NULL;
    struct virtual_machine_object_hash_internal_node *exists_object_hash_internal_node = NULL;
    struct virtual_machine_object_hash_internal_node *hash_internal_node_new_begin = NULL, *hash_internal_node_new_end = NULL;

    (void)vm;
    exists_object_hash = (struct virtual_machine_object_hash *)object->ptr;
    exists_object_hash_internal = exists_object_hash->ptr_internal;
    if (virtual_machine_object_hash_internal_exists(&exists_object_hash_internal_node, exists_object_hash_internal, object_sub_key) != 0)
    {
        if (exists_object_hash_internal_node->prev == NULL) hash_internal_node_new_begin = exists_object_hash_internal_node->next;
        else 
        {
            hash_internal_node_new_begin = exists_object_hash_internal->begin;
            exists_object_hash_internal_node->prev->next = exists_object_hash_internal_node->next;
        }
        if (exists_object_hash_internal_node->next == NULL) hash_internal_node_new_end = exists_object_hash_internal_node->prev;
        else 
        {
            hash_internal_node_new_end = exists_object_hash_internal->end;
            exists_object_hash_internal_node->next->prev = exists_object_hash_internal_node->prev;
        }


        virtual_machine_object_destroy(vm, exists_object_hash_internal_node->ptr_key);
        virtual_machine_object_destroy(vm, exists_object_hash_internal_node->ptr_value);
        virtual_machine_resource_free(vm->resource, exists_object_hash_internal_node);

        exists_object_hash_internal->begin = hash_internal_node_new_begin;
        exists_object_hash_internal->end = hash_internal_node_new_end;
    }

    return 0;
}

static int virtual_machine_object_hash_ref_by_raw_index(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object_hash_internal *object_src_hash_internal, \
        const struct virtual_machine_object *ref_index)
{
    int ret = 0;
    struct virtual_machine_object_hash_internal_node *hash_internal_node_cur = NULL;
    struct virtual_machine_object *new_object = NULL;

    (void)vm;

    *object_out = NULL;

    if (object_src_hash_internal->size == 0) 
    {
        vm_err_update(vm->r, -VM_ERR_OUT_OF_BOUNDS, \
                "runtime error: out of bounds, there is no element in hash");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    hash_internal_node_cur = object_src_hash_internal->begin;
    while (hash_internal_node_cur != NULL)
    {
        if (objects_eq(hash_internal_node_cur->ptr_key, ref_index) == OBJECTS_EQ)
        {
            if ((new_object = virtual_machine_object_clone(vm, hash_internal_node_cur->ptr_value)) == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            *object_out = new_object;
            goto finish;
        }

        hash_internal_node_cur = hash_internal_node_cur->next; 
    }

    virtual_machine_object_print(ref_index);

    vm_err_update(vm->r, -VM_ERR_OUT_OF_BOUNDS, \
            "runtime error: key not found");
    ret = -MULTIPLE_ERR_VM;
    goto fail; 

finish:
fail:
    return ret;
}

int virtual_machine_object_hash_ref_get(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_idx)
{
    int ret = 0;

    struct virtual_machine_object_hash *object_src_hash = NULL;
    struct virtual_machine_object_hash_internal *object_src_hash_internal = NULL;

    *object_out = NULL;

    if (object_src->type != OBJECT_TYPE_HASH)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    object_src_hash = object_src->ptr;
    object_src_hash_internal = object_src_hash->ptr_internal;
    if ((ret = virtual_machine_object_hash_ref_by_raw_index(vm, object_out, object_src_hash_internal, object_idx)) != 0)
    { goto fail; }

fail:
    return ret;
}

int virtual_machine_object_hash_haskey(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        struct virtual_machine_object *object_src, \
        struct virtual_machine_object *object_idx)
{
    int ret = 0;

    struct virtual_machine_object_hash_internal_node *hash_node_cur = NULL;
    struct virtual_machine_object_hash_internal *object_hash_internal = NULL;
    struct virtual_machine_object_hash *object_hash = NULL;
    struct virtual_machine_object *new_object = NULL;

    *object_out = NULL;

    if (object_src->type != OBJECT_TYPE_HASH)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    object_hash = (struct virtual_machine_object_hash *)object_src->ptr;
    object_hash_internal = object_hash->ptr_internal;
    hash_node_cur = object_hash_internal->begin;
    while (hash_node_cur != NULL)
    {
        if (objects_eq(hash_node_cur->ptr_key, object_idx) == OBJECTS_EQ)
        {
            if ((new_object = virtual_machine_object_bool_new_with_value(vm, 1)) == NULL)
            { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
            *object_out = new_object; new_object = NULL;
            goto done;
        }
        hash_node_cur = hash_node_cur->next; 
    }
    if ((new_object = virtual_machine_object_bool_new_with_value(vm, 0)) == NULL)
    { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
    *object_out = new_object; new_object = NULL;
    goto done;
fail:
done:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
    return ret;
}

static int _virtual_machine_object_hash_ref_set_by_raw_index( \
        struct virtual_machine_object *object_src, \
        struct virtual_machine_object *ref_index, \
        struct virtual_machine_object *object_value, \
        struct virtual_machine *vm, int *exists)
{
    int ret = 0;
    struct virtual_machine_object_hash_internal_node *hash_internal_node_cur = NULL;
    struct virtual_machine_object_hash_internal *object_hash_internal = NULL;
    struct virtual_machine_object_hash *object_hash = NULL;

    (void)vm;

    object_hash = (struct virtual_machine_object_hash *)object_src->ptr;
    object_hash_internal = object_hash->ptr_internal; 
    if (object_hash_internal->size == 0) 
    {
        vm_err_update(vm->r, -VM_ERR_OUT_OF_BOUNDS, \
                "runtime error: out of bounds, there is no element in hash");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    hash_internal_node_cur = object_hash_internal->begin;
    while (hash_internal_node_cur != NULL)
    {
        if (objects_eq(hash_internal_node_cur->ptr_key, ref_index) == OBJECTS_EQ)
        {
            *exists = 1;
            virtual_machine_object_destroy(vm, hash_internal_node_cur->ptr_value);
            hash_internal_node_cur->ptr_value = NULL;
            hash_internal_node_cur->ptr_value = virtual_machine_object_clone(vm, object_value);
            if (hash_internal_node_cur->ptr_value == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            goto finish;
        }
        hash_internal_node_cur = hash_internal_node_cur->next; 
    }
    *exists = 0;

finish:
fail:
    return ret;
}

int virtual_machine_object_hash_car(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src)
{
    int ret = 0;
    struct virtual_machine_object *object_src_solved = NULL;
    struct virtual_machine_object_hash *object_src_hash;
    struct virtual_machine_object_hash_internal *object_src_hash_internal;
    struct virtual_machine_object *new_object = NULL;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }
    object_src_hash = object_src_solved->ptr;
    object_src_hash_internal = object_src_hash->ptr_internal;

    *object_out = NULL;

    new_object = virtual_machine_object_clone(vm, object_src_hash_internal->begin->ptr_key);
    if (new_object == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }
    *object_out = new_object;
    new_object = NULL;

    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved);
done:
    return ret;
}

int virtual_machine_object_hash_cdr(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src)
{
    int ret;
    struct virtual_machine_object *object_src_solved = NULL;
    struct virtual_machine_object_hash *object_src_hash;
    struct virtual_machine_object_hash_internal *object_src_hash_internal;
    struct virtual_machine_object_hash_internal_node *object_src_hash_internal_node_target;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }
    object_src_hash = object_src_solved->ptr;
    object_src_hash_internal = object_src_hash->ptr_internal;

    /* Remove the head */
    object_src_hash_internal_node_target = object_src_hash_internal->begin;
    if (object_src_hash_internal_node_target == NULL)
    {
        /* No elements */
        vm_err_update(vm->r, -VM_ERR_EMPTY_HASH, \
                "runtime error: empty hash has no element");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    object_src_hash_internal->begin = object_src_hash_internal_node_target->next;

    object_src_hash_internal->size -= 1;
    if (object_src_hash_internal_node_target->next != NULL) 
    {
        object_src_hash_internal_node_target->next->prev = NULL;
    }
    else
    {
        object_src_hash_internal->end = NULL;
    }
    virtual_machine_object_destroy(vm, object_src_hash_internal_node_target->ptr_key);
    virtual_machine_object_destroy(vm, object_src_hash_internal_node_target->ptr_value);
    virtual_machine_resource_free(vm->resource, object_src_hash_internal_node_target);

    *object_out = object_src_solved;
    object_src_solved = NULL;
fail:
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved);
    return ret;
}

int virtual_machine_object_hash_ref_set(struct virtual_machine *vm, \
        struct virtual_machine_object *object_src, \
        struct virtual_machine_object *object_idx, \
        struct virtual_machine_object *object_value)
{
    int ret = 0;
    int exists = 0;
    struct virtual_machine_object *object_value_solved;

    if ((ret = virtual_machine_variable_solve(&object_value_solved, (struct virtual_machine_object *)object_value, NULL, 1, vm)) != 0)
    { goto fail; }

    if (object_src->type != OBJECT_TYPE_HASH)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    if ((ret = _virtual_machine_object_hash_ref_set_by_raw_index(object_src, object_idx, object_value_solved, vm, &exists)) != 0)
    { goto fail; }
    if (exists == 0)
    {
        if ((ret = virtual_machine_object_hash_append(vm, object_src, object_idx, object_value_solved)) != 0)
        { goto fail; }
        object_value_solved = NULL;
    }

fail:
    if (object_value_solved != NULL) virtual_machine_object_destroy(vm, object_value_solved);
    return ret;
}

