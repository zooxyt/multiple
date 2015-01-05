/* Pair Objects
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


/* Declaration */
int virtual_machine_object_pair_internal_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_pair_internal *object_pair_internal);


/* Internal Marker */
int virtual_machine_object_pair_internal_marker(void *object_internal)
{
    struct virtual_machine_object_pair_internal *object_pair_internal = object_internal;
    struct virtual_machine_object_pair_internal_node *object_pair_internal_node_car = object_pair_internal->car;
    struct virtual_machine_object_pair_internal_node *object_pair_internal_node_cdr = object_pair_internal->cdr;

    if (object_pair_internal_node_car != NULL)
    { virtual_machine_marks_object(object_pair_internal_node_car->ptr, VIRTUAL_MACHINE_GARBAGE_COLLECT_MAJOR); }
    if (object_pair_internal_node_cdr != NULL)
    { virtual_machine_marks_object(object_pair_internal_node_cdr->ptr, VIRTUAL_MACHINE_GARBAGE_COLLECT_MAJOR); }

    return 0;
}

/* Internal Collector */
int virtual_machine_object_pair_internal_collector(void *object_internal, int *confirm)
{
    struct virtual_machine_object_pair_internal *object_pair_internal;

    *confirm = VIRTUAL_MACHINE_GARBAGE_COLLECT_CONFIRM; 

    object_pair_internal = object_internal;
    return virtual_machine_object_pair_internal_destroy(object_pair_internal->vm, object_pair_internal);
}

/* Internal Part */

static struct virtual_machine_object_pair_internal *virtual_machine_object_pair_internal_new( \
        struct virtual_machine *vm)
{
    struct virtual_machine_object_pair_internal *new_object_pair_internal = NULL;

    /* Create the internal part of pair instance */
    new_object_pair_internal = (struct virtual_machine_object_pair_internal *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_object_pair_internal));
    if (new_object_pair_internal == NULL)
    { goto fail; }

    new_object_pair_internal->car = new_object_pair_internal->cdr = NULL;
    new_object_pair_internal->vm = vm;

    goto done;
fail:
    if (new_object_pair_internal != NULL)
    {
        virtual_machine_resource_free(vm->resource, new_object_pair_internal);
		new_object_pair_internal = NULL;
    }
done:
    return new_object_pair_internal;
}

int virtual_machine_object_pair_internal_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_pair_internal *object_pair_internal)
{
    struct virtual_machine_object_pair_internal_node *object_pair_node_internal_car, *object_pair_node_internal_cdr;

    if (object_pair_internal == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_pair_node_internal_car = object_pair_internal->car;
    object_pair_node_internal_cdr = object_pair_internal->cdr;

    if (object_pair_node_internal_car != NULL)
    {
        virtual_machine_object_destroy(vm, object_pair_node_internal_car->ptr);
        virtual_machine_resource_free(vm->resource, object_pair_node_internal_car);
    }
    if (object_pair_node_internal_cdr != NULL)
    {
        virtual_machine_object_destroy(vm, object_pair_node_internal_cdr->ptr);
        virtual_machine_resource_free(vm->resource, object_pair_node_internal_cdr);
    }
    virtual_machine_resource_free(vm->resource, object_pair_internal);

    return 0;
}

/*
static struct virtual_machine_object_pair_internal_node *virtual_machine_object_pair_internal_node_clone(struct virtual_machine *vm, \
        struct virtual_machine_object_pair_internal_node *object_pair_internal_node)
{
    struct virtual_machine_object_pair_internal_node *new_object_pair_internal_node = NULL;

    new_object_pair_internal_node = (struct virtual_machine_object_pair_internal_node *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_object_pair_internal_node));
    if (new_object_pair_internal_node == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        goto fail; 
    }
    new_object_pair_internal_node->ptr = NULL;
    if ((new_object_pair_internal_node->ptr = virtual_machine_object_clone(vm, object_pair_internal_node->ptr)) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        goto fail; 
    }

    goto done;
fail:
    if (new_object_pair_internal_node != NULL)
    {
        if (new_object_pair_internal_node->ptr != NULL) virtual_machine_object_destroy(vm, new_object_pair_internal_node->ptr);
        virtual_machine_resource_free(vm->resource, new_object_pair_internal_node);
		new_object_pair_internal_node = NULL;
    }
done:
    return new_object_pair_internal_node;
}
*/

static struct virtual_machine_object_pair_internal_node *
virtual_machine_object_pair_internal_node_new_with_configure( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    struct virtual_machine_object_pair_internal_node *new_object_pair_internal_node = NULL;

    new_object_pair_internal_node = (struct virtual_machine_object_pair_internal_node *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_object_pair_internal_node));
    if (new_object_pair_internal_node == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        goto fail; 
    }
    new_object_pair_internal_node->ptr = NULL;
    if ((new_object_pair_internal_node->ptr = virtual_machine_object_clone(vm, object)) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        goto fail; 
    }

    goto done;
fail:
    if (new_object_pair_internal_node != NULL)
    {
        if (new_object_pair_internal_node->ptr != NULL) virtual_machine_object_destroy(vm, new_object_pair_internal_node->ptr);
        virtual_machine_resource_free(vm->resource, new_object_pair_internal_node);
		new_object_pair_internal_node = NULL;
    }
done:
    return new_object_pair_internal_node;
}

static int virtual_machine_object_pair_internal_node_destroy(struct virtual_machine *vm, struct virtual_machine_object_pair_internal_node *object_pair_internal_node)
{
    if (object_pair_internal_node->ptr != NULL) virtual_machine_object_destroy(vm, object_pair_internal_node->ptr);
    virtual_machine_resource_free(vm->resource, object_pair_internal_node);
    return 0;
}

/*
static struct virtual_machine_object_pair_internal *virtual_machine_object_pair_internal_clone(struct virtual_machine *vm, \
        const struct virtual_machine_object_pair_internal *object_pair_internal)
{
    struct virtual_machine_object_pair_internal *new_object_pair_internal = NULL;

    if (object_pair_internal == NULL) return NULL;

    if ((new_object_pair_internal = virtual_machine_object_pair_internal_new(vm)) == NULL)
    { goto fail; }

    if (object_pair_internal->car != NULL)
    {
        if ((new_object_pair_internal->car = virtual_machine_object_pair_internal_node_clone(vm, \
                        object_pair_internal->car)) == NULL)
        { goto fail; }
    }
    if (object_pair_internal->cdr != NULL)
    {
        if ((new_object_pair_internal->cdr = virtual_machine_object_pair_internal_node_clone(vm, \
                        object_pair_internal->cdr)) == NULL)
        { goto fail; }
    }

    goto done;
fail:
    if (new_object_pair_internal != NULL) { virtual_machine_object_pair_internal_destroy(vm, new_object_pair_internal); }
    new_object_pair_internal = NULL;
done:
    return new_object_pair_internal;
}
*/

static int virtual_machine_object_pair_internal_print(const struct virtual_machine_object_pair_internal *object_pair_internal)
{
    if (object_pair_internal == NULL) return -MULTIPLE_ERR_NULL_PTR;

    printf("(");
    if (object_pair_internal->car != NULL) { virtual_machine_object_print(object_pair_internal->car->ptr); }
    printf(" . ");
    if (object_pair_internal->cdr != NULL) { virtual_machine_object_print(object_pair_internal->cdr->ptr); }
    printf(")");

    return 0;
}


/* Shell Part */

struct virtual_machine_object *virtual_machine_object_pair_new(struct virtual_machine *vm)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_pair *new_object_pair = NULL;

    /* Create pair object */
    if ((new_object_pair = (struct virtual_machine_object_pair *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_pair))) == NULL)
    { goto fail; }
    new_object_pair->ptr_internal = NULL;
    new_object_pair->ptr_internal = virtual_machine_object_pair_internal_new(vm);
    if (new_object_pair->ptr_internal == NULL) { goto fail; }

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_PAIR)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_pair) != 0)
    { goto fail; }

    if (virtual_machine_resource_reference_register( \
                vm->gc_stub, \
                new_object, \
                new_object_pair->ptr_internal, \
                &virtual_machine_object_pair_internal_marker, \
                &virtual_machine_object_pair_internal_collector) != 0)
    { goto fail; }

    new_object_pair = NULL;

    return new_object;
fail:
    if (new_object_pair != NULL) 
    {
        if (new_object_pair->ptr_internal != NULL) virtual_machine_object_pair_internal_destroy(vm, new_object_pair->ptr_internal);
        virtual_machine_resource_free(vm->resource, new_object_pair);
    }
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);

    return NULL;
}

int virtual_machine_object_pair_destroy(struct virtual_machine *vm, struct virtual_machine_object *object)
{
    struct virtual_machine_object_pair *object_pair = NULL;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_pair = (struct virtual_machine_object_pair *)object->ptr;
    if (object_pair != NULL)
    {
        virtual_machine_resource_free(vm->resource, object_pair);
    }
    _virtual_machine_object_destroy(vm, object);

    return 0;
}

struct virtual_machine_object *virtual_machine_object_pair_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_pair *object_pair = NULL;
    struct virtual_machine_object_pair *new_object_pair = NULL;

    if ((object == NULL) || (object->ptr == NULL)) return NULL;

    object_pair = object->ptr;

    if ((new_object_pair = (struct virtual_machine_object_pair *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_pair))) == NULL)
    { goto fail; }
    new_object_pair->ptr_internal = NULL;
    new_object_pair->ptr_internal = object_pair->ptr_internal;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_PAIR)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_pair) != 0)
    { goto fail; }
    new_object_pair = NULL;

    return new_object;
fail:
    return NULL;
}

int virtual_machine_object_pair_print(const struct virtual_machine_object *object)
{
    struct virtual_machine_object_pair *object_pair;

    object_pair = object->ptr;
    virtual_machine_object_pair_internal_print(object_pair->ptr_internal);

    return 0;
}

struct virtual_machine_object *virtual_machine_object_pair_make(struct virtual_machine *vm, \
        struct virtual_machine_object *object_car, \
        struct virtual_machine_object *object_cdr)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_pair *object_pair = NULL;
    struct virtual_machine_object_pair_internal *object_pair_internal = NULL;

    if ((new_object = virtual_machine_object_pair_new(vm)) == NULL)
    { goto fail; }
    object_pair = new_object->ptr;
    object_pair_internal = object_pair->ptr_internal;

    if ((object_pair_internal->car = virtual_machine_object_pair_internal_node_new_with_configure( \
            vm, \
            object_car)) == NULL)
    { goto fail; }

    if ((object_pair_internal->cdr = virtual_machine_object_pair_internal_node_new_with_configure( \
            vm, \
            object_cdr)) == NULL)
    { goto fail; }

    goto done;
fail:
    if (new_object != NULL)
    {
        virtual_machine_object_destroy(vm, new_object);
        new_object = NULL;
    }
done:
    return new_object;
}

int virtual_machine_object_pair_car( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src)
{
    int ret = 0;

    struct virtual_machine_object *object_src_solved = NULL;
    struct virtual_machine_object_pair *object_pair = NULL;
    struct virtual_machine_object_pair_internal *object_pair_internal = NULL;
    struct virtual_machine_object *new_object = NULL;

    *object_out = NULL;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }

    if (object_src_solved->type != OBJECT_TYPE_PAIR)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }
    object_pair = object_src_solved->ptr;
    object_pair_internal = object_pair->ptr_internal; 

    if ((new_object = virtual_machine_object_clone(vm, object_pair_internal->car->ptr)) == NULL)
    { goto fail; }
    *object_out = new_object; new_object = NULL;
fail:
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved);
    return ret;
}

int virtual_machine_object_pair_cdr( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src)
{
    int ret = 0;

    struct virtual_machine_object *object_src_solved = NULL;
    struct virtual_machine_object_pair *object_pair = NULL;
    struct virtual_machine_object_pair_internal *object_pair_internal = NULL;
    struct virtual_machine_object *new_object = NULL;

    *object_out = NULL;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }

    if (object_src_solved->type != OBJECT_TYPE_PAIR)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }
    object_pair = object_src_solved->ptr;
    object_pair_internal = object_pair->ptr_internal; 

    if ((new_object = virtual_machine_object_clone(vm, object_pair_internal->cdr->ptr)) == NULL)
    { goto fail; }
    *object_out = new_object; new_object = NULL;
fail:
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved);
    return ret;
}

int virtual_machine_object_pair_car_set( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        struct virtual_machine_object *object_src, \
        struct virtual_machine_object *object_member)
{
    int ret = 0;

    struct virtual_machine_object *object_src_solved = NULL;
    struct virtual_machine_object *object_member_solved = NULL;
    struct virtual_machine_object_pair *object_pair = NULL;
    struct virtual_machine_object_pair_internal *object_pair_internal = NULL;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }
    if ((ret = virtual_machine_variable_solve(&object_member_solved, (struct virtual_machine_object *)object_member, NULL, 1, vm)) != 0)
    { goto fail; }

    if (object_src_solved->type != OBJECT_TYPE_PAIR)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }
    object_pair = object_src_solved->ptr;
    object_pair_internal = object_pair->ptr_internal; 

    if (object_pair_internal->car != NULL)
    {
        virtual_machine_object_pair_internal_node_destroy(vm, object_pair_internal->car);
        object_pair_internal->car = NULL;
    }
    if ((object_pair_internal->car = virtual_machine_object_pair_internal_node_new_with_configure(vm, \
                    object_member_solved)) == NULL)
    { goto fail; }
    *object_out = object_src_solved; object_src_solved = NULL;

fail:
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved);
    if (object_member_solved != NULL) virtual_machine_object_destroy(vm, object_member_solved);
    return ret;
}

int virtual_machine_object_pair_cdr_set( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        struct virtual_machine_object *object_src, \
        struct virtual_machine_object *object_member)
{
    int ret = 0;

    struct virtual_machine_object *object_src_solved = NULL;
    struct virtual_machine_object *object_member_solved = NULL;
    struct virtual_machine_object_pair *object_pair = NULL;
    struct virtual_machine_object_pair_internal *object_pair_internal = NULL;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, (struct virtual_machine_object *)object_src, NULL, 1, vm)) != 0)
    { goto fail; }
    if ((ret = virtual_machine_variable_solve(&object_member_solved, (struct virtual_machine_object *)object_member, NULL, 1, vm)) != 0)
    { goto fail; }

    if (object_src_solved->type != OBJECT_TYPE_PAIR)
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }
    object_pair = object_src_solved->ptr;
    object_pair_internal = object_pair->ptr_internal; 

    if (object_pair_internal->cdr != NULL)
    {
        virtual_machine_object_pair_internal_node_destroy(vm, object_pair_internal->cdr);
        object_pair_internal->cdr = NULL;
    }
    if ((object_pair_internal->cdr = virtual_machine_object_pair_internal_node_new_with_configure(vm, \
                    object_member_solved)) == NULL)
    { goto fail; }
    *object_out = object_src_solved; object_src_solved = NULL;

fail:
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved);
    if (object_member_solved != NULL) virtual_machine_object_destroy(vm, object_member_solved);
    return ret;
}

