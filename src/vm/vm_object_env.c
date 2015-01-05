/* Environment Objects
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
#include "vm_env.h"
#include "vm_object_aio.h"
#include "vm_gc.h"
#include "vm_err.h"

int virtual_machine_object_environment_internal_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_environment_internal *object_environment_internal);

/* Internal Marker */

static int virtual_machine_object_environment_marker(void *object_internal)
{
    struct virtual_machine_object_environment_internal *object_environment_internal = object_internal;
    struct virtual_machine_environment_stack *environment; 

    environment = object_environment_internal->environment;
    virtual_machine_marks_environment_stack(environment, VIRTUAL_MACHINE_GARBAGE_COLLECT_MAJOR); 

    return 0;
}

/* Internal Collector */

static int virtual_machine_object_environment_collector(void *object_internal, int *confirm)
{
    struct virtual_machine_object_environment_internal *object_environment_internal;

    *confirm = VIRTUAL_MACHINE_GARBAGE_COLLECT_CONFIRM; 

    object_environment_internal = object_internal;
    return virtual_machine_object_environment_internal_destroy(object_environment_internal->vm, object_environment_internal);
}


/* Internal Part */

static struct virtual_machine_object_environment_internal *virtual_machine_object_environment_internal_new( \
        struct virtual_machine *vm, \
        struct virtual_machine_running_stack *running_stack)
{
    struct virtual_machine_object_environment_internal *new_object_environment_internal = NULL;
    struct virtual_machine_environment_stack *new_environment_stack = NULL;

    new_environment_stack = virtual_machine_environment_stack_new_from_running_stack(vm, running_stack);
    if (new_environment_stack == NULL) { goto fail; }

    /* Create the internal part of environment instance */
    new_object_environment_internal = (struct virtual_machine_object_environment_internal *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_object_environment_internal));
    if (new_object_environment_internal == NULL)
    { goto fail; }
    new_object_environment_internal->environment = new_environment_stack;
    new_object_environment_internal->vm = vm;

    goto done;
fail:
    if (new_environment_stack != NULL) virtual_machine_environment_stack_destroy(vm, new_environment_stack);
    if (new_object_environment_internal != NULL)
    {
        virtual_machine_resource_free(vm->resource, new_object_environment_internal);
        new_object_environment_internal = NULL;
    }
done:
    return new_object_environment_internal;
}

int virtual_machine_object_environment_internal_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_environment_internal *object_environment_internal)
{
    virtual_machine_environment_stack_destroy(vm, object_environment_internal->environment); 

    virtual_machine_resource_free(vm->resource, object_environment_internal);

    return 0;
}

/* Shell Part */

/* new */
struct virtual_machine_object *virtual_machine_object_environment_new( \
        struct virtual_machine *vm, \
        struct virtual_machine_running_stack *running_stack)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_environment *new_object_environment = NULL;

    /* Create environment object */
    if ((new_object_environment = (struct virtual_machine_object_environment *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_environment))) == NULL)
    { goto fail; }
    new_object_environment->ptr_internal = NULL;
    new_object_environment->ptr_internal = virtual_machine_object_environment_internal_new(vm, running_stack);
    if (new_object_environment->ptr_internal == NULL) { goto fail; }

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_ENV)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_environment) != 0)
    { goto fail; }

    if (virtual_machine_resource_reference_register( \
                vm->gc_stub, \
                new_object, \
                new_object_environment->ptr_internal, \
                &virtual_machine_object_environment_marker, \
                &virtual_machine_object_environment_collector) != 0)
    { goto fail; }

    new_object_environment = NULL;

    return new_object;
fail:
    if (new_object_environment != NULL) 
    {
        if (new_object_environment->ptr_internal != NULL) virtual_machine_object_environment_internal_destroy(vm, new_object_environment->ptr_internal);
        virtual_machine_resource_free(vm->resource, new_object_environment);
    }
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);

    return NULL;
}

/* destroy */
int virtual_machine_object_environment_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    struct virtual_machine_object_environment *object_environment = NULL;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_environment = (struct virtual_machine_object_environment *)object->ptr;
    if (object_environment != NULL)
    {
        virtual_machine_resource_free(vm->resource, object_environment);
    }
    _virtual_machine_object_destroy(vm, object);

    return 0;
}

struct virtual_machine_object *virtual_machine_object_environment_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_environment *object_environment = NULL;
    struct virtual_machine_object_environment *new_object_environment = NULL;

    if ((object == NULL) || (object->ptr == NULL)) return NULL;

    object_environment = object->ptr;

    if ((new_object_environment = (struct virtual_machine_object_environment *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_environment))) == NULL)
    { goto fail; }
    new_object_environment->ptr_internal = NULL;
    new_object_environment->ptr_internal = object_environment->ptr_internal;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_ENV)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_environment) != 0)
    { goto fail; }
    new_object_environment = NULL;

    return new_object;
fail:
    return NULL;
}

/* print */
int virtual_machine_object_environment_print( \
        const struct virtual_machine_object *object)
{
    (void)object;

    return 0;
}

int virtual_machine_object_environment_extract_environment( \
        struct virtual_machine_environment_stack **environment_stack_out, \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object_src)
{
    int ret = 0;
    char *bad_type_name;
    struct virtual_machine_object_environment *object_environment;
    struct virtual_machine_object_environment_internal *object_environment_internal;

    if (object_src == NULL) return -VM_ERR_NULL_PTR;

    if (object_src->type != OBJECT_TYPE_ENV)
    {
        ret = virtual_machine_object_id_to_type_name(&bad_type_name, NULL, object_src->type);
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type, " \
                 "\'%s\'",  
                 ret == 0 ? bad_type_name : "undefined type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    object_environment = object_src->ptr;
    object_environment_internal = object_environment->ptr_internal;

    *environment_stack_out = object_environment_internal->environment;
fail:
    return ret;
}

