/* Environment Entrance Objects
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
#include "vm_gc.h"
#include "vm_err.h"

int virtual_machine_object_environment_entrance_internal_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_environment_entrance_internal *object_environment_entrance_internal);

/* Internal Marker */

static int virtual_machine_object_environment_entrance_marker(void *object_internal)
{
    struct virtual_machine_object_environment_entrance_internal *object_environment_entrance_internal = object_internal;
    struct virtual_machine_environment_stack_frame *environment_stack_frame_cur;

    environment_stack_frame_cur = object_environment_entrance_internal->entrance;
    while (environment_stack_frame_cur != NULL)
    {
        virtual_machine_marks_environment_stack_frame(environment_stack_frame_cur, VIRTUAL_MACHINE_GARBAGE_COLLECT_MAJOR); 
        environment_stack_frame_cur = environment_stack_frame_cur->prev; 
    }

    return 0;
}

/* Internal Collector */

static int virtual_machine_object_environment_entrance_collector(void *object_internal, int *confirm)
{
    struct virtual_machine_object_environment_entrance_internal *object_environment_entrance_internal = object_internal;

    virtual_machine_environment_stack_frame_destroy( \
            object_environment_entrance_internal->vm, \
            object_environment_entrance_internal->entrance);

    *confirm = VIRTUAL_MACHINE_GARBAGE_COLLECT_CONFIRM; 

    return virtual_machine_object_environment_entrance_internal_destroy(object_environment_entrance_internal->vm, \
            object_environment_entrance_internal);
}

/* Internal Part */

static struct virtual_machine_object_environment_entrance_internal *virtual_machine_object_environment_entrance_internal_new( \
        struct virtual_machine *vm, \
        struct virtual_machine_environment_stack_frame *entrance)
{
    struct virtual_machine_object_environment_entrance_internal *new_environment_entrance_internal = NULL;

    new_environment_entrance_internal = (struct virtual_machine_object_environment_entrance_internal *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_object_environment_entrance_internal));
    if (new_environment_entrance_internal == NULL) { goto fail; }
    new_environment_entrance_internal->entrance = entrance;
    new_environment_entrance_internal->vm = vm;

    goto done;
fail:
    if (new_environment_entrance_internal != NULL)
    {
        virtual_machine_resource_free(vm->resource, new_environment_entrance_internal);
        new_environment_entrance_internal = NULL;
    }
done:
    return new_environment_entrance_internal;
}

int virtual_machine_object_environment_entrance_internal_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_environment_entrance_internal *object_environment_entrance_internal)
{
    virtual_machine_resource_free(vm->resource, object_environment_entrance_internal);

    return 0;
}


/* Shell Part */


/* new */
struct virtual_machine_object *virtual_machine_object_environment_entrance_new( \
        struct virtual_machine *vm, \
        struct virtual_machine_environment_stack_frame *entrance)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_environment_entrance *new_object_environment_entrance = NULL;

    /* Create environment object */
    if ((new_object_environment_entrance = (struct virtual_machine_object_environment_entrance *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_environment_entrance))) == NULL)
    { goto fail; }
    new_object_environment_entrance->ptr_internal = NULL;
    new_object_environment_entrance->ptr_internal = virtual_machine_object_environment_entrance_internal_new(vm, entrance);
    if (new_object_environment_entrance->ptr_internal == NULL) { goto fail; }

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_ENV_ENT)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_environment_entrance) != 0)
    { goto fail; }

    if (virtual_machine_resource_reference_register( \
                vm->gc_stub, \
                new_object, \
                new_object_environment_entrance->ptr_internal, \
                &virtual_machine_object_environment_entrance_marker, \
                &virtual_machine_object_environment_entrance_collector) != 0)
    { goto fail; }

    new_object_environment_entrance = NULL;

    return new_object;
fail:
    if (new_object_environment_entrance != NULL) 
    {
        if (new_object_environment_entrance->ptr_internal != NULL) virtual_machine_object_environment_entrance_internal_destroy(vm, new_object_environment_entrance->ptr_internal);
        virtual_machine_resource_free(vm->resource, new_object_environment_entrance);
    }
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);

    return NULL;
}

/* destroy */
int virtual_machine_object_environment_entrance_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    struct virtual_machine_object_environment_entrance *object_environment_entrance = NULL;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_environment_entrance = (struct virtual_machine_object_environment_entrance *)object->ptr;
    if (object_environment_entrance != NULL)
    {
        virtual_machine_resource_free(vm->resource, object_environment_entrance);
    }
    _virtual_machine_object_destroy(vm, object);

    return 0;
}

/* clone */
struct virtual_machine_object *virtual_machine_object_environment_entrance_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_environment_entrance *object_environment_entrance = NULL;
    struct virtual_machine_object_environment_entrance *new_object_environment_entrance  = NULL;

    if ((object == NULL) || (object->ptr == NULL)) return NULL;

    object_environment_entrance = object->ptr;

    if ((new_object_environment_entrance = (struct virtual_machine_object_environment_entrance *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_environment_entrance))) == NULL)
    { goto fail; }
    new_object_environment_entrance->ptr_internal = NULL;
    new_object_environment_entrance->ptr_internal = object_environment_entrance->ptr_internal;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_ENV_ENT)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_environment_entrance) != 0)
    { goto fail; }
    new_object_environment_entrance = NULL;

    return new_object;
fail:
    return NULL;
}

/* print */
int virtual_machine_object_environment_entrance_print( \
        const struct virtual_machine_object *object)
{
    (void)object;

    return 0;
}


/* Make a blank entrance, used in the bottom */
struct virtual_machine_object *virtual_machine_object_environment_entrance_make_blank( \
        struct virtual_machine *vm)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_environment_stack_frame *new_entrance = NULL;

    new_entrance = virtual_machine_environment_stack_frame_new_blank( \
        vm);
    if (new_entrance == NULL) { goto fail; }

    /* Blank Entrance, no previous */
    new_entrance->prev = NULL;

    new_object = virtual_machine_object_environment_entrance_new(vm, new_entrance);
    if (new_object == NULL) { goto fail; }
    new_entrance = NULL;

    goto done;
fail:
    if (new_object != NULL)
    {
        virtual_machine_object_destroy(vm, new_object);
        new_object = NULL;
    }
    if (new_entrance != NULL)
    {
        virtual_machine_environment_stack_frame_destroy(vm, new_entrance);
    }
done:
    return new_object;
}

/* Make a linked entrance with the given previous environment */
struct virtual_machine_object *virtual_machine_object_environment_entrance_make_linked( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object_environment_entrance_prev)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_environment_stack_frame *new_entrance = NULL;
    struct virtual_machine_object *object_environment_entrance;
    struct virtual_machine_object_environment_entrance *environment_entrance;
    struct virtual_machine_object_environment_entrance_internal *environment_entrance_internal;

    new_entrance = virtual_machine_environment_stack_frame_new_blank( \
        vm);
    if (new_entrance == NULL) { goto fail; }

    if (object_environment_entrance_prev != NULL)
    {
        object_environment_entrance = object_environment_entrance_prev;
        if (object_environment_entrance->type != OBJECT_TYPE_ENV_ENT)
        { VM_ERR_INTERNAL(vm->r); goto fail; }
        environment_entrance = object_environment_entrance->ptr;
        environment_entrance_internal = environment_entrance->ptr_internal;
        if (environment_entrance_internal->entrance == NULL)
        { VM_ERR_INTERNAL(vm->r); goto fail; }
        new_entrance->prev = environment_entrance_internal->entrance;
    }

    new_object = virtual_machine_object_environment_entrance_new(vm, new_entrance);
    if (new_object == NULL) { goto fail; }
    new_entrance = NULL;

    goto done;
fail:
    if (new_object != NULL)
    {
        virtual_machine_object_destroy(vm, new_object);
        new_object = NULL;
    }
    if (new_entrance != NULL)
    {
        virtual_machine_environment_stack_frame_destroy(vm, new_entrance);
    }
done:
    return new_object;
}

int virtual_machine_object_environment_entrance_extract_environment_stack_frame( \
        struct virtual_machine_environment_stack_frame **environment_stack_frame_out, \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object_src)
{
    int ret = 0;
    char *bad_type_name;
    struct virtual_machine_object_environment_entrance *object_environment_entrance;
    struct virtual_machine_object_environment_entrance_internal *object_environment_entrance_internal;

    if (object_src == NULL)
    {
        VM_ERR_INTERNAL(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    if (object_src->type != OBJECT_TYPE_ENV_ENT)
    {
        ret = virtual_machine_object_id_to_type_name(&bad_type_name, NULL, object_src->type);
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type, " \
                 "\'%s\'",  
                 ret == 0 ? bad_type_name : "undefined type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    object_environment_entrance = object_src->ptr;
    object_environment_entrance_internal = object_environment_entrance->ptr_internal;

    *environment_stack_frame_out = object_environment_entrance_internal->entrance;
fail:
    return ret;
}

