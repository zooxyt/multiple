/* Function Objects
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


int virtual_machine_object_func_internal_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_func_internal *object_func_internal);
int virtual_machine_object_func_internal_print( \
        const struct virtual_machine_object_func_internal *object_func_internal);

/* Internal Marker */
static int virtual_machine_object_func_marker(void *object_internal)
{
    struct virtual_machine_object_func_internal *object_func_internal = object_internal;

    if (object_func_internal->environment != NULL)
    {
        virtual_machine_marks_object(object_func_internal->environment, \
                VIRTUAL_MACHINE_GARBAGE_COLLECT_MAJOR);
    }

    if (object_func_internal->environment_entrance != NULL)
    {
        virtual_machine_marks_object(object_func_internal->environment_entrance, \
                VIRTUAL_MACHINE_GARBAGE_COLLECT_MAJOR);
    }

    if (object_func_internal->cached_promise != NULL)
    {
        virtual_machine_marks_object(object_func_internal->cached_promise, \
                VIRTUAL_MACHINE_GARBAGE_COLLECT_MAJOR);
    }

    return 0;
}

/* Internal Collector */
static int virtual_machine_object_func_collector(void *object_internal, int *confirm)
{
    struct virtual_machine_object_func_internal *object_func_internal;

    *confirm = VIRTUAL_MACHINE_GARBAGE_COLLECT_CONFIRM; 

    object_func_internal = object_internal;
    return virtual_machine_object_func_internal_destroy(object_func_internal->vm, object_func_internal);
}


/* Internal Part */

static struct virtual_machine_object_func_internal *virtual_machine_object_func_internal_new( \
        struct virtual_machine *vm)
{
    struct virtual_machine_object_func_internal *new_object_func_internal = NULL;

    /* Create the internal part of 'function' instance */
    new_object_func_internal = (struct virtual_machine_object_func_internal *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_object_func_internal));
    if (new_object_func_internal == NULL)
    { goto fail; }

    new_object_func_internal->module = NULL;
    new_object_func_internal->pc = 0;
    new_object_func_internal->cont = 0;
    new_object_func_internal->extern_func = NULL;
    new_object_func_internal->extern_func_args = NULL;
    new_object_func_internal->environment = NULL;
    new_object_func_internal->turning_point = vm->tp->running_stack->size;
    new_object_func_internal->continuation_tracing_item = NULL;
    new_object_func_internal->environment_entrance = NULL;
    new_object_func_internal->promise = 0;
    new_object_func_internal->cached_promise = NULL;
    new_object_func_internal->vm = vm;

    goto done;
fail:
    if (new_object_func_internal != NULL)
    {
        virtual_machine_resource_free(vm->resource, new_object_func_internal);
		new_object_func_internal = NULL;
    }
done:
    return new_object_func_internal;
}

int virtual_machine_object_func_internal_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_func_internal *object_func_internal)
{

    if (object_func_internal->environment != NULL)
    {
        virtual_machine_object_destroy(vm, object_func_internal->environment);
    }
    if (object_func_internal->continuation_tracing_item != NULL)
    {
        continuation_list_remove(vm, \
                vm->tp->continuations, \
                object_func_internal->continuation_tracing_item);
    }
    if (object_func_internal->environment_entrance != NULL)
    {
        virtual_machine_object_destroy(vm, object_func_internal->environment_entrance);
    }
    if (object_func_internal->cached_promise != NULL)
    {
        virtual_machine_object_destroy(vm, object_func_internal->cached_promise);
    }
    virtual_machine_resource_free(vm->resource, object_func_internal);

    return 0;
}

/* print */
int virtual_machine_object_func_internal_print(const struct virtual_machine_object_func_internal *object_func_internal)
{
    if (object_func_internal == NULL) return -MULTIPLE_ERR_NULL_PTR;

    printf("function");

    if ((object_func_internal->module != NULL) && (object_func_internal->module->name != NULL))
    {
        printf(" ");
        fwrite(object_func_internal->module->name, object_func_internal->module->name_len, 1, stdout);
        printf("::");
    }
    printf("%u", object_func_internal->pc);

    return 0;
}


/* Shell Part */

struct virtual_machine_object *virtual_machine_object_func_new( \
        struct virtual_machine *vm)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_func *new_object_func = NULL;

    /* Create function object */
    if ((new_object_func = (struct virtual_machine_object_func *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_func))) == NULL)
    { goto fail; }
    new_object_func->ptr_internal = NULL;
    new_object_func->ptr_internal = virtual_machine_object_func_internal_new(vm);
    if (new_object_func->ptr_internal == NULL) { goto fail; }

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_FUNCTION)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_func) != 0)
    { goto fail; }
    new_object_func = NULL;

    return new_object;
fail:
    if (new_object_func != NULL) 
    {
        if (new_object_func->ptr_internal != NULL) virtual_machine_object_func_internal_destroy(vm, new_object_func->ptr_internal);
        virtual_machine_resource_free(vm->resource, new_object_func);
    }
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);

    return NULL;
}

struct virtual_machine_object *virtual_machine_object_func_make_internal( \
        struct virtual_machine_module *module, \
        uint32_t pc, \
        struct virtual_machine *vm)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_func *new_object_func = NULL;

    /* Create func object */
    if ((new_object_func = (struct virtual_machine_object_func *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_func))) == NULL)
    { goto fail; }
    new_object_func->ptr_internal = NULL;
    new_object_func->ptr_internal = virtual_machine_object_func_internal_new(vm);
    if (new_object_func->ptr_internal == NULL) { goto fail; }

    new_object_func->ptr_internal->module = module;
    new_object_func->ptr_internal->pc = pc;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_FUNCTION)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_func) != 0)
    { goto fail; }

    if (virtual_machine_resource_reference_register( \
                vm->gc_stub, \
                new_object, \
                new_object_func->ptr_internal, \
                &virtual_machine_object_func_marker, \
                &virtual_machine_object_func_collector) != 0)
    { goto fail; }

    new_object_func = NULL;

    return new_object;
fail:
    if (new_object_func != NULL) 
    {
        if (new_object_func->ptr_internal != NULL) virtual_machine_object_func_internal_destroy(vm, new_object_func->ptr_internal);
        virtual_machine_resource_free(vm->resource, new_object_func);
    }
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);

    return NULL;
}

struct virtual_machine_object *virtual_machine_object_func_make_external( \
        int (*extern_func)(struct multiple_stub_function_args *args), \
        struct multiple_stub_function_args *extern_func_args, \
        struct virtual_machine *vm)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_func *new_object_func = NULL;

    /* Create func object */
    if ((new_object_func = (struct virtual_machine_object_func *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_func))) == NULL)
    { goto fail; }
    new_object_func->ptr_internal = NULL;
    new_object_func->ptr_internal = virtual_machine_object_func_internal_new(vm);
    if (new_object_func->ptr_internal == NULL) { goto fail; }

    new_object_func->ptr_internal->extern_func = extern_func;
    new_object_func->ptr_internal->extern_func_args = extern_func_args;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_FUNCTION)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_func) != 0)
    { goto fail; }


    if (virtual_machine_resource_reference_register( \
                vm->gc_stub, \
                new_object, \
                new_object_func->ptr_internal, \
                &virtual_machine_object_func_marker, \
                &virtual_machine_object_func_collector) != 0)
    { goto fail; }

    new_object_func = NULL;

    return new_object;
fail:
    if (new_object_func != NULL) 
    {
        if (new_object_func->ptr_internal != NULL) virtual_machine_object_func_internal_destroy(vm, new_object_func->ptr_internal);
        virtual_machine_resource_free(vm->resource, new_object_func);
    }
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);

    return NULL;
}

int virtual_machine_object_func_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    struct virtual_machine_object_func *object_func = NULL;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_func = (struct virtual_machine_object_func *)object->ptr;
    if (object_func != NULL)
    {
        virtual_machine_resource_free(vm->resource, object_func);
    }
    _virtual_machine_object_destroy(vm, object);

    return 0;
}

struct virtual_machine_object *virtual_machine_object_func_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_func *object_func = NULL;
    struct virtual_machine_object_func *new_object_func = NULL;

    if ((object == NULL) || (object->ptr == NULL)) return NULL;

    object_func = object->ptr;

    if ((new_object_func = (struct virtual_machine_object_func *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_object_func))) == NULL)
    { goto fail; }
    new_object_func->ptr_internal = NULL;
    new_object_func->ptr_internal = object_func->ptr_internal;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_FUNCTION)) == NULL)
    { goto fail; }
    if (_virtual_machine_object_ptr_set(new_object, new_object_func) != 0)
    { goto fail; }
    new_object_func = NULL;

    return new_object;
fail:
    return NULL;
}

int virtual_machine_object_func_print(const struct virtual_machine_object *object)
{
    struct virtual_machine_object_func *object_func;

    object_func = object->ptr;
    virtual_machine_object_func_internal_print(object_func->ptr_internal);

    return 0;
}

/* value */
int virtual_machine_object_func_get_value_internal(const struct virtual_machine_object *object, \
        struct virtual_machine_module **module, \
        uint32_t *pc)
{
    struct virtual_machine_object_func *object_func;
    struct virtual_machine_object_func_internal *object_func_internal;

    object_func = object->ptr;
    object_func_internal = object_func->ptr_internal;

    *module = object_func_internal->module;
    *pc= object_func_internal->pc;

    return 0;
}

int virtual_machine_object_func_get_value_external(const struct virtual_machine_object *object, \
        int (**extern_func)(struct multiple_stub_function_args *args), \
        struct multiple_stub_function_args **extern_func_args)
{
    struct virtual_machine_object_func *object_func;
    struct virtual_machine_object_func_internal *object_func_internal;

    object_func = object->ptr;
    object_func_internal = object_func->ptr_internal;

    *extern_func = object_func_internal->extern_func;
    *extern_func_args = object_func_internal->extern_func_args;

    return 0;
}

int virtual_machine_object_func_make(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        struct virtual_machine *vm)
{
    int ret = 0;

    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object *object_src_solved = NULL;
    struct virtual_machine_module *module_target;
    uint32_t function_instrument_number;
    char *bad_type_name;

    struct virtual_machine_object_identifier *object_id_src = NULL;

    int (*extern_func)(struct multiple_stub_function_args *args) = NULL;
    struct multiple_stub_function_args *extern_func_args = NULL;

    if (object_src == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (object_src->type == OBJECT_TYPE_FUNCTION)
    {
        if ((new_object = virtual_machine_object_func_clone(vm, object_src)) == NULL)
        {
            VM_ERR_MALLOC(vm->r);
            ret = -MULTIPLE_ERR_VM;
            goto fail;
        }
    }
    else if (object_src->type == OBJECT_TYPE_IDENTIFIER)
    {
        object_id_src = object_src->ptr;

        /* Lookup in all modules */
        if (object_id_src->domain_id != NULL)
        {
            if (virtual_machine_module_lookup_by_name(&module_target, \
                        vm, \
                        object_id_src->domain_id, object_id_src->domain_id_len) == LOOKUP_NOT_FOUND)
            {
                vm_err_update(vm->r, -VM_ERR_JUMP_TARGET_NOT_FOUND, \
                        "runtime error: module \'%s\' not imported", \
                        object_id_src->domain_id);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
        }
        else
        {
            module_target = vm->tp->running_stack->top->module;
        }

        /* Lookup instrument */
        if (virtual_machine_module_function_lookup_by_name( \
                    &function_instrument_number, \
                    module_target, \
                    object_id_src->id, object_id_src->id_len) != LOOKUP_NOT_FOUND)
        {

            if ((new_object = virtual_machine_object_func_make_internal( \
                            module_target, function_instrument_number, \
                            vm)) == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
        }
        else if (virtual_machine_function_lookup_external_function( \
                    vm, \
                    &extern_func, \
                    &extern_func_args, \
                    object_id_src->id, object_id_src->id_len) != LOOKUP_NOT_FOUND)
        {
            if ((new_object = virtual_machine_object_func_make_external( \
                            extern_func, extern_func_args, \
                            vm)) == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
        }
        else
        {
            /* Solve */
            if ((ret = virtual_machine_variable_solve(&object_src_solved, object_src, vm->tp->running_stack->top, 1, vm)) != 0)
            { goto fail; }
            if (object_src_solved->type == OBJECT_TYPE_FUNCTION)
            {
                if ((new_object = virtual_machine_object_func_clone(vm, object_src_solved)) == NULL)
                {
                    VM_ERR_MALLOC(vm->r);
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }
                virtual_machine_object_destroy(vm, object_src_solved);
                object_src_solved = NULL;
            }
            else
            {
                /* Not found */
                vm_err_update(vm->r, -VM_ERR_JUMP_TARGET_NOT_FOUND, \
                        "runtime error: function \'%s\' not found", \
                        object_id_src->id);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
        }
    }
    else
    {
        ret = virtual_machine_object_id_to_type_name(&bad_type_name, NULL, object_src->type);
        vm_err_update(vm->r, -VM_ERR_DATA_TYPE, \
                "runtime error: invalid object type, " \
                "\'%s\' given",  \
                ret == 0 ? bad_type_name : "undefined type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    *object_out = new_object;

    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_func_destroy(vm, new_object);
    if (object_src_solved != NULL) virtual_machine_object_func_destroy(vm, object_src_solved);
done:
    return ret;
}

int virtual_machine_object_func_make_normal(struct virtual_machine_object **object_out, \
        const struct virtual_machine_module *module, \
        const uint32_t pc, \
        struct virtual_machine *vm)
{
    int ret = 0;

    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_func *new_object_func = NULL;
    struct virtual_machine_object_func_internal *new_object_func_internal = NULL;

    /* Create Function Object */
    if ((new_object = virtual_machine_object_func_new(vm)) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    new_object_func = (struct virtual_machine_object_func *)new_object->ptr;
    new_object_func_internal = new_object_func->ptr_internal; 
    new_object_func_internal->module = (struct virtual_machine_module *)module;
    new_object_func_internal->pc = pc;
    new_object_func_internal->cont = 0;
    new_object_func_internal->extern_func = NULL;
    new_object_func_internal->extern_func_args = NULL;
    new_object_func_internal->environment = NULL;
    if (virtual_machine_resource_reference_register( \
                vm->gc_stub, \
                new_object, \
                new_object_func->ptr_internal, \
                &virtual_machine_object_func_marker, \
                &virtual_machine_object_func_collector) != 0)
    { goto fail; }

    *object_out = new_object;

    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_func_destroy(vm, new_object);
done:
    return ret;
}

int virtual_machine_object_func_make_lambda(struct virtual_machine_object **object_out, \
        const struct virtual_machine_module *module, \
        const uint32_t pc, \
        struct virtual_machine *vm)
{
    int ret = 0;

    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_func *new_object_func = NULL;
    struct virtual_machine_object_func_internal *new_object_func_internal = NULL;

    /* Create Function Object */
    if ((new_object = virtual_machine_object_func_new(vm)) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    /*printf("Make Lambda: (%u) ", (unsigned int)pc);*/

    new_object_func = (struct virtual_machine_object_func *)new_object->ptr;
    new_object_func_internal = new_object_func->ptr_internal; 
    new_object_func_internal->module = (struct virtual_machine_module *)module;
    new_object_func_internal->pc = pc;
    new_object_func_internal->cont = 0;
    new_object_func_internal->extern_func = NULL;
    new_object_func_internal->extern_func_args = NULL;
    new_object_func_internal->environment_entrance = virtual_machine_object_clone(vm, vm->tp->running_stack->top->environment_entrance);
    if (new_object_func_internal->environment_entrance == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    if (virtual_machine_resource_reference_register( \
                vm->gc_stub, \
                new_object, \
                new_object_func->ptr_internal, \
                &virtual_machine_object_func_marker, \
                &virtual_machine_object_func_collector) != 0)
    { goto fail; }

    *object_out = new_object;

    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_func_destroy(vm, new_object);
done:
    return ret;
}

int virtual_machine_object_func_make_promise(struct virtual_machine_object **object_out, \
        const struct virtual_machine_module *module, \
        const uint32_t pc, \
        struct virtual_machine *vm)
{
    int ret = 0;

    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_func *new_object_func = NULL;
    struct virtual_machine_object_func_internal *new_object_func_internal = NULL;

    /* Create Function Object */
    if ((new_object = virtual_machine_object_func_new(vm)) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    /*printf("Make Promise: (%u) ", (unsigned int)pc);*/

    new_object_func = (struct virtual_machine_object_func *)new_object->ptr;
    new_object_func_internal = new_object_func->ptr_internal; 
    new_object_func_internal->module = (struct virtual_machine_module *)module;
    new_object_func_internal->pc = pc;
    new_object_func_internal->promise = 1;
    new_object_func_internal->extern_func = NULL;
    new_object_func_internal->extern_func_args = NULL;
    new_object_func_internal->environment_entrance = virtual_machine_object_clone(vm, vm->tp->running_stack->top->environment_entrance);
    if (new_object_func_internal->environment_entrance == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    if (virtual_machine_resource_reference_register( \
                vm->gc_stub, \
                new_object, \
                new_object_func->ptr_internal, \
                &virtual_machine_object_func_marker, \
                &virtual_machine_object_func_collector) != 0)
    { goto fail; }

    *object_out = new_object;

    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_func_destroy(vm, new_object);
done:
    return ret;
}

int virtual_machine_object_func_make_cont(struct virtual_machine_object **object_out, \
        const struct virtual_machine_module *module, \
        const uint32_t pc, \
        struct virtual_machine *vm)
{
    int ret = 0;

    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_func *new_object_func = NULL;
    struct virtual_machine_object_func_internal *new_object_func_internal = NULL;
    struct continuation_list_item *new_continuation_list_item = NULL;

    /* Create Function Object */
    if ((new_object = virtual_machine_object_func_new(vm)) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    /*printf("Make continuation: (%u) ", (unsigned int)pc);*/

    new_object_func = (struct virtual_machine_object_func *)new_object->ptr;
    new_object_func_internal = new_object_func->ptr_internal;
    new_object_func_internal->module = (struct virtual_machine_module *)module;
    new_object_func_internal->pc = pc;
    new_object_func_internal->cont = 1;
    new_object_func_internal->extern_func = NULL;
    new_object_func_internal->extern_func_args = NULL;
    new_object_func_internal->environment = virtual_machine_object_environment_new(vm, vm->tp->running_stack);
    if (new_object_func_internal->environment == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    /* Add the continuation to the continuation list for updating the turning point */
    new_continuation_list_item = continuation_list_item_new(vm, new_object_func_internal);
    if (new_continuation_list_item == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    continuation_list_append(vm->tp->continuations, new_continuation_list_item);

    if (virtual_machine_resource_reference_register( \
                vm->gc_stub, \
                new_object, \
                new_object_func->ptr_internal, \
                &virtual_machine_object_func_marker, \
                &virtual_machine_object_func_collector) != 0)
    { goto fail; }

    *object_out = new_object;

    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_func_destroy(vm, new_object);
done:
    return ret;
}

struct virtual_machine_environment_stack *virtual_machine_object_func_extract_environment( \
        struct virtual_machine_object *object_src)
{
    struct virtual_machine_object_func *object_func = object_src->ptr;
    struct virtual_machine_object_func_internal *object_func_internal;
    struct virtual_machine_object *object_src_environment;
    struct virtual_machine_object_environment *object_environment;
    struct virtual_machine_object_environment_internal *object_environment_internal;

    object_func_internal = object_func->ptr_internal;
    object_src_environment = object_func_internal->environment;
    if (object_src_environment == NULL) return NULL;

    object_environment = object_src_environment->ptr;
    object_environment_internal = object_environment->ptr_internal;
    return object_environment_internal->environment;
}

int virtual_machine_object_func_type( \
        struct virtual_machine_object *object_src)
{
    struct virtual_machine_object_func *object_function = NULL;
    struct virtual_machine_object_func_internal *object_function_internal = NULL;

    if (object_src->type != OBJECT_TYPE_FUNCTION)
    { return VIRTUAL_MACHINE_OBJECT_FUNC_TYPE_UNKNOWN; }

    object_function = object_src->ptr;
    object_function_internal = object_function->ptr_internal; 
    if (object_function_internal->extern_func != NULL)
    { return VIRTUAL_MACHINE_OBJECT_FUNC_TYPE_EXTERNAL; }

    if (object_function_internal->cont != 0)
    { return VIRTUAL_MACHINE_OBJECT_FUNC_TYPE_CONT; }

    if (object_function_internal->promise != 0)
    { return VIRTUAL_MACHINE_OBJECT_FUNC_TYPE_PROMISE; }

    if (object_function_internal->environment_entrance != NULL)
    { return VIRTUAL_MACHINE_OBJECT_FUNC_TYPE_LAMBDA; }

    return VIRTUAL_MACHINE_OBJECT_FUNC_TYPE_NORMAL;
}

