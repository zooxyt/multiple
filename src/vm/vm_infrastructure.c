/* Infrastructure of Virtual Machine
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

#include "multiple_ir.h"
#include "multiple_err.h"
#include "multiple_tunnel.h"
#include "multiple_misc.h"
#include "vm_predef.h"
#include "vm.h"
#include "vm_startup.h"
#include "vm_types.h"
#include "vm_res.h"
#include "vm_object_type.h"
#include "vm_object.h"
#include "vm_object_aio.h"
#include "vm_err.h"
#include "vm_dynlib.h"

#include "gc.h"

#include "spinlock.h"


/* Computing Stack */

struct virtual_machine_computing_stack *virtual_machine_computing_stack_new(struct virtual_machine *vm)
{
    struct virtual_machine_computing_stack *new_stack = NULL;

    if ((new_stack = (struct virtual_machine_computing_stack *)virtual_machine_resource_malloc(\
                    vm->resource, \
                    sizeof(struct virtual_machine_computing_stack))) == NULL) 
    { return NULL; }
    new_stack->bottom = new_stack->top = NULL;
    new_stack->size = 0;
    return new_stack;
}

int virtual_machine_computing_stack_destroy(struct virtual_machine *vm, struct virtual_machine_computing_stack *stack)
{
    int ret = 0;
    struct virtual_machine_object *obj_cur, *obj_next;

    if (stack == NULL) return -MULTIPLE_ERR_NULL_PTR;

    obj_cur = stack->bottom;
    while (obj_cur != NULL)
    {
        obj_next = obj_cur->next;
        if ((ret = virtual_machine_object_destroy(vm, obj_cur)) != 0) return ret;
        obj_cur = obj_next;
    }
    virtual_machine_resource_free(vm->resource, stack);

    return 0;
}

int virtual_machine_computing_stack_clear(struct virtual_machine *vm, struct virtual_machine_computing_stack *stack)
{
    int ret = 0;
    struct virtual_machine_object *obj_cur, *obj_next;

    if (stack == NULL) return -MULTIPLE_ERR_NULL_PTR;

    obj_cur = stack->bottom;
    while (obj_cur != NULL)
    {
        obj_next = obj_cur->next;
        if ((ret = virtual_machine_object_destroy(vm, obj_cur)) != 0) return ret;
        obj_cur = obj_next;
    }
    stack->bottom = stack->top = NULL;
    stack->size = 0;

    return 0;
}

int virtual_machine_computing_stack_push(struct virtual_machine_computing_stack *stack, \
        struct virtual_machine_object *object)
{
    if (stack->bottom == NULL)
    {
        stack->bottom = stack->top = object;
    }
    else
    {
        stack->top->next = object;
        object->prev = stack->top;
        stack->top = object;
    }
    stack->size++;

    return 0;
}

int virtual_machine_computing_stack_pop(struct virtual_machine *vm, struct virtual_machine_computing_stack *stack) 
{
    int ret = 0;
    struct virtual_machine_object *new_top = NULL;

    if (stack == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (stack->top == NULL)
    {
        vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                "runtime error: computing stack empty");
        return -MULTIPLE_ERR_VM;
    }

    new_top = stack->top->prev;
    if ((ret = virtual_machine_object_destroy(vm, stack->top)) != 0) return ret;
    stack->top = new_top;
    if (new_top != NULL) new_top->next = NULL;
    stack->size--;
    if (stack->size == 0) stack->bottom = NULL;

    return 0;
}

int virtual_machine_computing_stack_transport(struct virtual_machine *vm, \
        struct virtual_machine_computing_stack *stack_dst, \
        struct virtual_machine_computing_stack *stack_src, \
        size_t size)
{
    struct virtual_machine_object *new_object = NULL;

    if (stack_dst == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (stack_src == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (stack_src->size < size) 
    {
        vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                "runtime error: computing stack empty");
        return -MULTIPLE_ERR_VM;
    }

    while (size > 0)
    {
        if ((new_object = virtual_machine_object_clone(vm, stack_src->top)) == NULL)
        {
            return -MULTIPLE_ERR_MALLOC;
        }
        virtual_machine_computing_stack_push(stack_dst, new_object);
        size--;
    }
    return 0;
}

int virtual_machine_computing_stack_transport_reversely(struct virtual_machine *vm, \
        struct virtual_machine_computing_stack *stack_dst, \
        struct virtual_machine_computing_stack *stack_src, \
        size_t size)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object *object_cur = NULL;
    size_t idx;

    if (stack_dst == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (stack_src == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (stack_src->size < size) 
    {
        vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                "runtime error: computing stack empty");
        return -MULTIPLE_ERR_VM;
    }

    object_cur = stack_src->bottom;
    for (idx = 0; idx != size; idx++)
    {
        if ((new_object = virtual_machine_object_clone(vm, object_cur)) == NULL)
        {
            return -MULTIPLE_ERR_MALLOC;
        }
        virtual_machine_computing_stack_push(stack_dst, new_object);
        object_cur = object_cur->next;
    }

    return 0;
}

struct virtual_machine_running_stack_frame *virtual_machine_running_stack_frame_new(struct virtual_machine *vm)
{
    struct virtual_machine_running_stack_frame *new_stack_frame = NULL;

    if ((new_stack_frame = (struct virtual_machine_running_stack_frame *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_running_stack_frame))) == NULL)
    { return NULL; }

    new_stack_frame->computing_stack = NULL;
    new_stack_frame->arguments = NULL;
    new_stack_frame->variables = NULL;
    new_stack_frame->generators = NULL;
    new_stack_frame->environment_entrance = NULL;

    if ((new_stack_frame->computing_stack = virtual_machine_computing_stack_new(vm)) == NULL)
    { goto fail; }
    if ((new_stack_frame->arguments = virtual_machine_computing_stack_new(vm)) == NULL)
    { goto fail; }
    if ((new_stack_frame->variables = virtual_machine_variable_list_new(vm)) == NULL)
    { goto fail; }
    if ((new_stack_frame->generators = virtual_machine_running_stack_frame_list_new(vm)) == NULL)
    { goto fail; }
    new_stack_frame->module = 0;
    new_stack_frame->pc_start = 0;
    new_stack_frame->pc = 0;
    new_stack_frame->args_count = 0;
    new_stack_frame->closure = 0;
    new_stack_frame->trap_pc = 0;
    new_stack_frame->trap_enabled = 0;
    new_stack_frame->next = new_stack_frame->prev = NULL;
    goto done;
fail:
    if (new_stack_frame != NULL)
    {
        virtual_machine_running_stack_frame_destroy(vm, new_stack_frame);
        new_stack_frame = NULL;
    }
done:
    return new_stack_frame;
}

int virtual_machine_running_stack_frame_destroy(struct virtual_machine *vm, struct virtual_machine_running_stack_frame *stack_frame)
{
    if (stack_frame == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (stack_frame->variables != NULL) virtual_machine_variable_list_destroy(vm, stack_frame->variables);
    if (stack_frame->computing_stack != NULL) virtual_machine_computing_stack_destroy(vm, stack_frame->computing_stack);
    if (stack_frame->arguments != NULL) virtual_machine_computing_stack_destroy(vm, stack_frame->arguments);
    if (stack_frame->generators != NULL) virtual_machine_running_stack_frame_list_destroy(vm, stack_frame->generators);
    if (stack_frame->environment_entrance != NULL) virtual_machine_object_destroy(vm, stack_frame->environment_entrance);
    virtual_machine_resource_free(vm->resource, stack_frame);

    return 0;
}

static struct virtual_machine_running_stack_frame *virtual_machine_running_stack_frame_clone(struct virtual_machine *vm, struct virtual_machine_running_stack_frame *frame)
{
    int ret = 0;
    struct virtual_machine_running_stack_frame *new_frame = NULL;

    struct virtual_machine_variable *variable_cur = NULL, *new_variable = NULL;
    struct virtual_machine_object *object_cur = NULL, *new_object = NULL;
    struct virtual_machine_running_stack_frame *generator_cur = NULL, *new_generator = NULL;

    new_frame = virtual_machine_running_stack_frame_new(vm);
    if (new_frame == NULL) goto fail;

    new_frame->module = frame->module;
    new_frame->pc_start = frame->pc_start;
    new_frame->pc = frame->pc;
    new_frame->args_count = frame->args_count;
    new_frame->closure = frame->closure;
    new_frame->trap_pc = frame->trap_pc;
    new_frame->trap_enabled = frame->trap_enabled;

    variable_cur = frame->variables->begin;
    while (variable_cur != NULL)
    {
        new_variable = virtual_machine_variable_clone(vm, variable_cur);
        if (new_variable == NULL) goto fail;
        ret = virtual_machine_variable_list_append(new_frame->variables, new_variable);
        if (ret != 0) goto fail;
        new_variable = NULL;

        variable_cur = variable_cur->next;
    }

    object_cur = frame->computing_stack->bottom;
    while (object_cur != NULL)
    {
        new_object = virtual_machine_object_clone(vm, object_cur);
        if (new_object == NULL) goto fail;
        virtual_machine_computing_stack_push(new_frame->computing_stack, new_object);
        new_object = NULL;

        object_cur = object_cur->next;
    }

    object_cur = frame->arguments->bottom;
    while (object_cur != NULL)
    {
        new_object = virtual_machine_object_clone(vm, object_cur);
        if (new_object == NULL) goto fail;
        virtual_machine_computing_stack_push(new_frame->arguments, new_object);
        new_object = NULL;

        object_cur = object_cur->next;
    }

    generator_cur = frame->generators->begin;
    while (generator_cur != NULL)
    {
        new_generator = virtual_machine_running_stack_frame_clone(vm, generator_cur);
        if (new_generator != NULL) goto fail;
        virtual_machine_running_stack_frame_list_append(new_frame->generators, new_generator);
        new_generator = NULL;
        generator_cur = generator_cur->next;
    }

    if (frame->environment_entrance != NULL)
    {
        if ((new_frame->environment_entrance = virtual_machine_object_clone(vm, \
                        frame->environment_entrance)) == NULL)
        { goto fail; }
    }

    return new_frame;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
    if (new_variable != NULL) virtual_machine_variable_destroy(vm, new_variable);
    if (new_frame != NULL) virtual_machine_running_stack_frame_destroy(vm, new_frame);
    return NULL;
}

struct virtual_machine_running_stack_frame *virtual_machine_running_stack_frame_new_with_configure(struct virtual_machine *vm, \
        struct virtual_machine_module *module, uint32_t pc, \
        size_t args_count, int closure, uint32_t trap_pc, int trap_enabled, \
        struct virtual_machine_object *environment_entrance, \
        struct virtual_machine_variable_list *variables)
{
    int ret = 0;
    struct virtual_machine_running_stack_frame *new_stack_frame = NULL;
    struct virtual_machine_variable *variable_cur = NULL, *new_variable = NULL;
    char *bad_type_name;

    if ((new_stack_frame = virtual_machine_running_stack_frame_new(vm)) == NULL)
    { goto fail; }
    new_stack_frame->module = module;
    new_stack_frame->pc_start = pc;
    new_stack_frame->pc = pc;
    new_stack_frame->args_count = args_count;
    new_stack_frame->closure = closure;
    new_stack_frame->trap_pc = trap_pc;
    new_stack_frame->trap_enabled = trap_enabled;

    if (environment_entrance != NULL)
    {
        if (environment_entrance->type != OBJECT_TYPE_ENV_ENT)
        {
            ret = virtual_machine_object_id_to_type_name(&bad_type_name, NULL, environment_entrance->type);
            vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                    "runtime error: unsupported operand type, " \
                    "\'%s\'",  
                    ret == 0 ? bad_type_name : "undefined type");
            goto fail;
        }
        new_stack_frame->environment_entrance = virtual_machine_object_clone(vm, environment_entrance);
    }
    else
    {
        new_stack_frame->environment_entrance = virtual_machine_object_environment_entrance_make_blank(vm);
    }
    if (new_stack_frame->environment_entrance == NULL) { goto fail; }

    if (variables != NULL)
    {
        variable_cur = variables->begin;
        while (variable_cur != NULL)
        {
            new_variable = virtual_machine_variable_clone(vm, variable_cur);
            if (new_variable == NULL) goto fail;
            ret = virtual_machine_variable_list_append(new_stack_frame->variables, new_variable);
            if (ret != 0) goto fail;
            new_variable = NULL;

            variable_cur = variable_cur->next;
        }
    }

    goto done;
fail:
    if (new_stack_frame != NULL)  virtual_machine_running_stack_frame_destroy(vm, new_stack_frame);
done:
    return new_stack_frame;
}

struct virtual_machine_running_stack_frame *virtual_machine_running_stack_frame_get_generator(struct virtual_machine_running_stack_frame *current_frame, \
        struct virtual_machine_module *module, \
        uint32_t generator_pc)
{
    struct virtual_machine_running_stack_frame *frame_cur;

    if (current_frame == NULL) return NULL;

    frame_cur = current_frame->generators->begin;
    while (frame_cur != NULL)
    {
        if ((frame_cur->module == module) && (frame_cur->pc_start == generator_pc))
        {
            return frame_cur;
        }
        frame_cur = frame_cur->next;
    }

    return NULL;
}


/* Variable & Variable List */

struct virtual_machine_variable *virtual_machine_variable_new(struct virtual_machine *vm, uint32_t module_id, uint32_t id)
{
    struct virtual_machine_variable *new_variable = NULL;

    if ((new_variable = (struct virtual_machine_variable *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_variable))) == NULL)
    {
        return NULL;
    }
    new_variable->module_id = module_id;
    new_variable->id = id;
    new_variable->ptr = NULL;
    new_variable->prev = new_variable->next = NULL;

    return new_variable;
}

int virtual_machine_variable_destroy(struct virtual_machine *vm, struct virtual_machine_variable *variable)
{
    if (variable == NULL) return -MULTIPLE_ERR_NULL_PTR;

    virtual_machine_object_destroy(vm, variable->ptr);
    virtual_machine_resource_free(vm->resource, variable);

    return 0;
}

int virtual_machine_variable_ptr_set(struct virtual_machine_variable *variable, struct virtual_machine_object *ptr)
{
    if (variable == NULL) return -MULTIPLE_ERR_NULL_PTR;
    variable->ptr = ptr;
    return 0;
}

struct virtual_machine_variable *virtual_machine_variable_new_with_configure(struct virtual_machine *vm, \
        uint32_t module_id, uint32_t id, struct virtual_machine_object *object_ptr)
{
    struct virtual_machine_variable *new_variable = NULL;
    struct virtual_machine_object *new_object = NULL;

    if ((new_object = virtual_machine_object_clone(vm, object_ptr)) == NULL)
    { goto fail; }
    if ((new_variable = virtual_machine_variable_new(vm, module_id, id)) == NULL) 
    { goto fail; }
    if (virtual_machine_variable_ptr_set(new_variable, new_object) != 0)
    { goto fail; }
    new_object = NULL;

    return new_variable;
    goto done;
fail:
    if (new_variable != NULL)
    {
        if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
        if (new_variable != NULL) virtual_machine_variable_destroy(vm, new_variable);
        new_variable = NULL;
    }
done:
    return new_variable;
}

struct virtual_machine_variable *virtual_machine_variable_clone(struct virtual_machine *vm, struct virtual_machine_variable *variable)
{
    struct virtual_machine_variable *new_variable = NULL;

    if ((new_variable = virtual_machine_variable_new(vm, variable->module_id, variable->id)) == NULL)
    { goto fail; }
    if ((new_variable->ptr = virtual_machine_object_clone(vm, variable->ptr)) == NULL)
    { goto fail; }
    return new_variable;
fail:
    if (new_variable != NULL) virtual_machine_variable_destroy(vm, new_variable);
    return NULL;
}

struct virtual_machine_variable_list *virtual_machine_variable_list_new(struct virtual_machine *vm)
{
    struct virtual_machine_variable_list *new_virtual_machine_variable_list = NULL;

    if ((new_virtual_machine_variable_list = (struct virtual_machine_variable_list *)virtual_machine_resource_malloc( \
                    vm->resource, 
                    sizeof(struct virtual_machine_variable_list))) == NULL)
    {
        return NULL;
    }
    new_virtual_machine_variable_list->begin = new_virtual_machine_variable_list->end = NULL;
    new_virtual_machine_variable_list->size = 0;

    return new_virtual_machine_variable_list;
}

int virtual_machine_variable_list_destroy(struct virtual_machine *vm, struct virtual_machine_variable_list *list)
{
    struct virtual_machine_variable *variable_cur, *variable_next;

    if (list == NULL) return -MULTIPLE_ERR_NULL_PTR;
    variable_cur = list->begin;
    while (variable_cur != NULL)
    {
        variable_next = variable_cur->next;
        virtual_machine_variable_destroy(vm, variable_cur);
        variable_cur = variable_next;
    }
    virtual_machine_resource_free(vm->resource, list);

    return 0;
}

int virtual_machine_variable_list_append(struct virtual_machine_variable_list *list, struct virtual_machine_variable * new_variable)
{
    if (list == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (new_variable == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (list->begin == NULL)
    {
        list->begin = list->end = new_variable;
    }
    else
    {
        list->end->next = new_variable;
        new_variable->prev = list->end;
        list->end = new_variable;
    }
    list->size++;

    return 0;
}

int virtual_machine_variable_list_remove(struct virtual_machine *vm, struct virtual_machine_variable_list *list, struct virtual_machine_variable *variable)
{
    struct virtual_machine_variable *variable_cur, *variable_prev, *variable_next;
    char *var_name = NULL;
    struct virtual_machine_object *variable_object;
    struct virtual_machine_object_identifier *variable_object_identifier;

    if (list == NULL) return -MULTIPLE_ERR_NULL_PTR;

    variable_object = variable->ptr;
    if (variable_object->type == OBJECT_TYPE_IDENTIFIER)
    {
        variable_object_identifier = (struct virtual_machine_object_identifier *)variable_object->ptr;
        if (variable_object_identifier != NULL)
        {
            var_name = variable_object_identifier->id;
        }
    }

    if (list->size == 0) 
    {
        vm_err_update(vm->r, -VM_ERR_OBJECT_NOT_FOUND, \
                "runtime error: variable \'%s\' not found", \
                var_name != NULL ? var_name : "unknown");
        return -MULTIPLE_ERR_VM;
    }
    else if (list->size == 1) 
    {
        if (variable == list->begin)
        {
            virtual_machine_variable_destroy(vm, list->begin);
            list->begin = list->end = NULL;
            list->size = 0;
            return 0;
        }
        else
        {
            vm_err_update(vm->r, -VM_ERR_OBJECT_NOT_FOUND, \
                    "runtime error: variable \'%s\' not found", \
                    var_name != NULL ? var_name : "unknown");
            return -MULTIPLE_ERR_VM;
        }
    }
    else 
    {
        if (variable == list->begin)
        {
            variable_next = variable->next;
            virtual_machine_variable_destroy(vm, list->begin);
            list->begin = variable_next;
            list->size--;
            return 0;
        }
        else if (variable == list->end)
        {
            variable_prev = variable->prev;
            virtual_machine_variable_destroy(vm, list->end);
            list->end = variable_prev;
            list->size--;
            return 0;
        }
        else
        {
            variable_cur = list->begin;
            while (variable_cur != NULL)
            {
                variable_prev = variable_cur->prev;
                variable_next = variable_cur->next;
                if (variable_cur == variable)
                {
                    variable_prev->next = variable_next;
                    variable_next->prev = variable_prev;
                    virtual_machine_variable_destroy(vm, variable_cur);
                    list->size--;
                    return 0;
                }
                variable_cur = variable_next;
            }
            vm_err_update(vm->r, -VM_ERR_OBJECT_NOT_FOUND, \
                    "runtime error: variable \'%s\' not found", \
                    var_name != NULL ? var_name : "unknown");
            return -MULTIPLE_ERR_VM;
        }
    }

}

int virtual_machine_variable_list_append_with_configure(struct virtual_machine *vm, \
        struct virtual_machine_variable_list *list, \
        uint32_t module_id, uint32_t id, \
        struct virtual_machine_object *object_ptr)
{
    int ret;
    struct virtual_machine_variable *new_virtual_machine_variable = NULL;

    if ((list == NULL) || (object_ptr == NULL))
    {
        VM_ERR_NULL_PTR(vm->r);
        ret = -MULTIPLE_ERR_NULL_PTR;
        goto fail;
    }

    if ((new_virtual_machine_variable = virtual_machine_variable_new_with_configure(vm, module_id, id, object_ptr)) == NULL)
    {
        vm_err_update(vm->r, -VM_ERR_MALLOC, \
                "runtime error: out of memory while creating new variable");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    if ((ret = virtual_machine_variable_list_append(list, new_virtual_machine_variable)) != 0)
    {
        VM_ERR_INTERNAL(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    goto done;
fail:
    if (new_virtual_machine_variable != NULL)
    {
        virtual_machine_variable_destroy(vm, new_virtual_machine_variable);
    }
done:
    return ret;
}

int virtual_machine_variable_list_update_with_configure(struct virtual_machine *vm, \
        struct virtual_machine_variable_list *list, uint32_t module_id, uint32_t id, struct virtual_machine_object *object_ptr)
{
    int ret = 0;
    struct virtual_machine_variable *var = NULL;
    struct vm_err r_local;

    vm_err_init(&r_local);

    if ((list == NULL) || (object_ptr == NULL))
    {
        VM_ERR_NULL_PTR(vm->r);
        return -MULTIPLE_ERR_NULL_PTR;
    }

    /* Variable? */
    if ((ret = virtual_machine_variable_list_lookup(&var, list, module_id, id)) != LOOKUP_FOUND)
    {
        /* Not exists, create directly */
    }
    else
    {
        /* Destroy the variable */
        if ((ret = virtual_machine_variable_list_remove(vm, list, var)) != 0) 
        { goto fail; }
    }

    ret = virtual_machine_variable_list_append_with_configure(vm, list, module_id, id, object_ptr);

fail:
    return ret;
}

int virtual_machine_variable_list_lookup(struct virtual_machine_variable **variable_out, \
        struct virtual_machine_variable_list *list, \
        uint32_t module_id, uint32_t id)
{
    struct virtual_machine_variable *variable_cur;

    if (list == NULL) return -MULTIPLE_ERR_NULL_PTR;

    *variable_out = NULL;

    variable_cur = list->begin;
    while (variable_cur != NULL)
    {
        if ((variable_cur->id == id) && (variable_cur->module_id == module_id))
        {
            *variable_out = variable_cur;
            return LOOKUP_FOUND;
        }
        variable_cur = variable_cur->next;
    }

    return LOOKUP_NOT_FOUND;
}

int virtual_machine_variable_list_lookup_from_environment_entrance( \
        struct virtual_machine *vm, \
        struct virtual_machine_variable **variable_out, \
        struct virtual_machine_variable_list **variable_list_out, \
        struct virtual_machine_object *object_environment_entrance, \
        uint32_t module_id, uint32_t id, int one_frame_limit)
{
    int ret = 0;
    struct virtual_machine_variable *variable_cur;
    struct virtual_machine_environment_stack_frame *environment_stack_frame_cur;

    *variable_out = NULL;
    *variable_list_out = NULL;

    if (object_environment_entrance == NULL) 
    {
        VM_ERR_INTERNAL(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    if ((virtual_machine_object_environment_entrance_extract_environment_stack_frame( \
                    &environment_stack_frame_cur, \
                    vm, \
                    object_environment_entrance)) != 0)
    {
        /* Not Found */
        goto finish;
    }
    while (environment_stack_frame_cur != NULL)
    {
        variable_cur = environment_stack_frame_cur->variables->begin;
        while (variable_cur != NULL)
        {
            if ((variable_cur->id == id) && (variable_cur->module_id == module_id))
            {
                *variable_out = variable_cur;
                *variable_list_out = environment_stack_frame_cur->variables;
                goto finish;
            }
            variable_cur = variable_cur->next;
        }
        if (one_frame_limit != 0) 
        {
            /* Hit the one frame limit */
            break;
        }
        environment_stack_frame_cur = environment_stack_frame_cur->prev;
    }

finish:
fail:
    return ret;
}

static int module_lookup_by_id(struct virtual_machine_module **target_module, uint32_t module_id, struct virtual_machine *vm)
{
    struct virtual_machine_module *module_cur;
    module_cur = vm->modules->begin;
    while (module_cur != NULL)
    {
        if (module_cur->id == module_id)
        {
            *target_module = module_cur;
            return LOOKUP_FOUND;
        }
        module_cur = module_cur->next;
    }
    *target_module = NULL;
    return LOOKUP_NOT_FOUND;
}

static int virtual_machine_variable_list_lookup_global(struct virtual_machine_variable **variable_out, struct virtual_machine_variable_list *list, const char *name, const size_t len, struct virtual_machine *vm)
{
    struct virtual_machine_variable *variable_cur;
    struct virtual_machine_data_section_item *data_section_item_operand = NULL;
    struct virtual_machine_module *target_module;

    if (list == NULL) return -MULTIPLE_ERR_NULL_PTR;

    *variable_out = NULL;

    variable_cur = list->begin;
    while (variable_cur != NULL)
    {
        /* Get pointer to target module with module id */
        if (module_lookup_by_id(&target_module, variable_cur->module_id, vm) == LOOKUP_NOT_FOUND)
        {
            VM_ERR_INTERNAL(vm->r);
            return -MULTIPLE_ERR_VM;
        }
        if (virtual_machine_module_lookup_data_section_items(target_module, &data_section_item_operand, variable_cur->id) != LOOKUP_FOUND)
        {
            VM_ERR_INTERNAL(vm->r);
            return -MULTIPLE_ERR_VM;
        }
        if (((size_t)data_section_item_operand->size == len) && (strncmp(data_section_item_operand->ptr, name, len) == 0))
        {
            *variable_out = variable_cur;
            return LOOKUP_FOUND;
        }
        variable_cur = variable_cur->next;
    }

    return LOOKUP_NOT_FOUND;
}


/* Stack Frame List */

struct virtual_machine_running_stack_frame_list *virtual_machine_running_stack_frame_list_new(struct virtual_machine *vm)
{
    struct virtual_machine_running_stack_frame_list *new_stack_frame_list = NULL;

    if ((new_stack_frame_list = (struct virtual_machine_running_stack_frame_list *) \
                virtual_machine_resource_malloc(vm->resource, sizeof(struct virtual_machine_running_stack_frame_list))) == NULL)
    {
        return NULL;
    }
    new_stack_frame_list->begin = new_stack_frame_list->end = NULL;
    new_stack_frame_list->size = 0;
    return new_stack_frame_list;
}

int virtual_machine_running_stack_frame_list_destroy(struct virtual_machine *vm, struct virtual_machine_running_stack_frame_list *list)
{
    struct virtual_machine_running_stack_frame *frame_cur, *frame_next;

    if (list == NULL) return -MULTIPLE_ERR_NULL_PTR;
    frame_cur = list->begin;
    while (frame_cur != NULL)
    {
        frame_next = frame_cur->next;
        virtual_machine_running_stack_frame_destroy(vm, frame_cur);
        frame_cur = frame_next;
    }
    virtual_machine_resource_free(vm->resource, list);
    return 0;
}

int virtual_machine_running_stack_frame_list_append(struct virtual_machine_running_stack_frame_list *stack_frame_list, \
        struct virtual_machine_running_stack_frame *new_frame)
{

    if (stack_frame_list->begin == NULL)
    {
        stack_frame_list->begin = stack_frame_list->end = new_frame;
    }
    else
    {
        stack_frame_list->end->next = new_frame;
        new_frame->prev = stack_frame_list->end;
        stack_frame_list->end = new_frame;
    }
    stack_frame_list->size++;

    return 0;
}

struct virtual_machine_variable_list *virtual_machine_variable_list_clone(struct virtual_machine *vm, \
        struct virtual_machine_variable_list *variable_list)
{
    int ret;
    struct virtual_machine_variable *variable_cur;
    struct virtual_machine_variable *new_variable = NULL;
    struct virtual_machine_variable_list *new_variable_list = NULL;

    new_variable_list = virtual_machine_variable_list_new(vm);
    if (new_variable_list == NULL) goto fail;

    variable_cur = variable_list->begin;
    while (variable_cur != NULL)
    {
        new_variable = virtual_machine_variable_clone(vm, variable_cur);
        if (new_variable == NULL) goto fail;
        ret = virtual_machine_variable_list_append(new_variable_list, new_variable);
        if (ret != 0) goto fail;
        new_variable = NULL;

        variable_cur = variable_cur->next;
    }
    goto done;
fail:
    if (new_variable_list != NULL)
    {
        virtual_machine_variable_list_destroy(vm, new_variable_list);
        new_variable_list = NULL;
    }
    if (new_variable != NULL)
    {
        virtual_machine_variable_destroy(vm, new_variable);
    }
done:
    return new_variable_list;
}


/* Environment Stack */

struct virtual_machine_environment_stack_frame *virtual_machine_environment_stack_frame_new_blank( \
        struct virtual_machine *vm)
{
    struct virtual_machine_environment_stack_frame *new_environment_stack_frame;

    if ((new_environment_stack_frame = (struct virtual_machine_environment_stack_frame *) \
                virtual_machine_resource_malloc(vm->resource, sizeof(struct virtual_machine_environment_stack_frame))) == NULL)
    { return NULL; }
    new_environment_stack_frame->variables = NULL;
    new_environment_stack_frame->computing_stack = NULL;
    new_environment_stack_frame->environment_entrance = NULL;

    new_environment_stack_frame->variables = virtual_machine_variable_list_new(vm);
    if (new_environment_stack_frame->variables == NULL)
    { goto fail; }

    new_environment_stack_frame->computing_stack = virtual_machine_computing_stack_new(vm);
    if (new_environment_stack_frame->computing_stack == NULL)
    { goto fail; }

    new_environment_stack_frame->environment_entrance = NULL;

    new_environment_stack_frame->pc = 0;
    new_environment_stack_frame->module = NULL;
    new_environment_stack_frame->args_count = 0;
    new_environment_stack_frame->closure = 0;
    new_environment_stack_frame->trap_pc = 0;
    new_environment_stack_frame->trap_enabled = 0;
    new_environment_stack_frame->next = NULL;
    new_environment_stack_frame->prev = NULL;
    goto done;
fail:
    if (new_environment_stack_frame != NULL)
    {
        virtual_machine_environment_stack_frame_destroy(vm, new_environment_stack_frame);
        new_environment_stack_frame = NULL;
    }
done:
    return new_environment_stack_frame;
}

struct virtual_machine_environment_stack_frame *virtual_machine_environment_stack_frame_new_with_configure( \
        struct virtual_machine *vm, \
        struct virtual_machine_variable_list *variables, \
        struct virtual_machine_computing_stack *computing_stack, \
        struct virtual_machine_object *environment_entrance, \
        uint32_t pc, \
        struct virtual_machine_module *module, \
        size_t args_count, \
        int closure, \
        uint32_t trap_pc, \
        int trap_enabled)
{
    struct virtual_machine_environment_stack_frame *new_environment_stack_frame;

    if ((new_environment_stack_frame = (struct virtual_machine_environment_stack_frame *) \
                virtual_machine_resource_malloc(vm->resource, sizeof(struct virtual_machine_environment_stack_frame))) == NULL)
    { return NULL; }
    new_environment_stack_frame->variables = NULL;
    new_environment_stack_frame->computing_stack = NULL;
    new_environment_stack_frame->environment_entrance = NULL;

    if (variables != NULL)
    {
        new_environment_stack_frame->variables = virtual_machine_variable_list_clone(vm, variables);
        if (new_environment_stack_frame->variables == NULL)
        { goto fail; }
    }

    new_environment_stack_frame->computing_stack = virtual_machine_computing_stack_new(vm);
    if (new_environment_stack_frame->computing_stack == NULL)
    { goto fail; }


    if (environment_entrance != NULL)
    {
        new_environment_stack_frame->environment_entrance = virtual_machine_object_clone(vm, environment_entrance);
    }
    else
    {
        new_environment_stack_frame->environment_entrance = virtual_machine_object_environment_entrance_make_blank(vm);
    }
    if (new_environment_stack_frame->environment_entrance == NULL) { goto fail; }

    if (virtual_machine_computing_stack_transport(vm, \
            new_environment_stack_frame->computing_stack, \
            computing_stack, \
            computing_stack->size \
            ) != 0)
    {
        goto fail;
    }

    new_environment_stack_frame->pc = pc;
    new_environment_stack_frame->module = module;
    new_environment_stack_frame->args_count = args_count;
    new_environment_stack_frame->closure = closure;
    new_environment_stack_frame->trap_pc = trap_pc;
    new_environment_stack_frame->trap_enabled = trap_enabled;
    new_environment_stack_frame->next = NULL;
    new_environment_stack_frame->prev = NULL;
    goto done;
fail:
    if (new_environment_stack_frame != NULL)
    {
        virtual_machine_environment_stack_frame_destroy(vm, new_environment_stack_frame);
        new_environment_stack_frame = NULL;
    }
done:
    return new_environment_stack_frame;
}

int virtual_machine_environment_stack_frame_destroy(struct virtual_machine *vm, \
        struct virtual_machine_environment_stack_frame *environment_stack_frame)
{
    if (environment_stack_frame->variables != NULL)
    {
        virtual_machine_variable_list_destroy(vm, environment_stack_frame->variables);
    }
    if (environment_stack_frame->computing_stack != NULL) 
    {
        virtual_machine_computing_stack_destroy(vm, environment_stack_frame->computing_stack);
    }
    if (environment_stack_frame->environment_entrance != NULL) 
    {
        virtual_machine_object_destroy(vm, environment_stack_frame->environment_entrance);
    }
    virtual_machine_resource_free(vm->resource, environment_stack_frame);

    return 0;
}

struct virtual_machine_environment_stack_frame *virtual_machine_environment_stack_frame_clone(struct virtual_machine *vm, \
        struct virtual_machine_environment_stack_frame *environment_stack_frame)
{
    struct virtual_machine_environment_stack_frame *new_environment_stack_frame = NULL;

    new_environment_stack_frame = virtual_machine_environment_stack_frame_new_with_configure( \
            vm, \
            environment_stack_frame->variables, \
            environment_stack_frame->computing_stack, \
            environment_stack_frame->environment_entrance, \
            environment_stack_frame->pc, \
            environment_stack_frame->module, \
            environment_stack_frame->args_count, \
            environment_stack_frame->closure, \
            environment_stack_frame->trap_pc, \
            environment_stack_frame->trap_enabled);
    if (new_environment_stack_frame == NULL) { goto fail; }

    goto done;
fail:
    if (new_environment_stack_frame != NULL)
    {
        virtual_machine_environment_stack_frame_destroy(vm, new_environment_stack_frame);
        new_environment_stack_frame = NULL;
    }
done:
    return new_environment_stack_frame;
}

struct virtual_machine_environment_stack *virtual_machine_environment_stack_new(struct virtual_machine *vm)
{
    struct virtual_machine_environment_stack *new_environment_stack;

    if ((new_environment_stack = (struct virtual_machine_environment_stack *) \
                virtual_machine_resource_malloc(vm->resource, sizeof(struct virtual_machine_environment_stack))) == NULL)
    {
        return NULL;
    }
    new_environment_stack->begin = new_environment_stack->end = NULL;
    new_environment_stack->size = 0;

    return new_environment_stack;
}

int virtual_machine_environment_stack_destroy(struct virtual_machine *vm, \
        struct virtual_machine_environment_stack *environment_stack)
{
    struct virtual_machine_environment_stack_frame *environment_stack_frame_cur, *environment_stack_frame_next;

    environment_stack_frame_cur = environment_stack->begin;
    while (environment_stack_frame_cur != NULL)
    {
        environment_stack_frame_next = environment_stack_frame_cur->next;
        virtual_machine_environment_stack_frame_destroy(vm, environment_stack_frame_cur);
        environment_stack_frame_cur = environment_stack_frame_next;
    }

    virtual_machine_resource_free(vm->resource, environment_stack);
    return 0;
}

int virtual_machine_environment_stack_append(struct virtual_machine_environment_stack *environment_stack, \
        struct virtual_machine_environment_stack_frame *new_environment_stack_frame)
{
    if (environment_stack->begin == NULL)
    {
        environment_stack->begin = environment_stack->end = new_environment_stack_frame;
    }
    else
    {
        new_environment_stack_frame->prev = environment_stack->end;
        environment_stack->end->next = new_environment_stack_frame;
        environment_stack->end = new_environment_stack_frame;
    }
    environment_stack->size += 1;
    return 0;
}

struct virtual_machine_environment_stack *virtual_machine_environment_stack_clone(struct virtual_machine *vm, \
        struct virtual_machine_environment_stack *environment_stack)
{
    struct virtual_machine_environment_stack *new_environment_stack;
    struct virtual_machine_environment_stack_frame *environment_stack_frame_cur;
    struct virtual_machine_environment_stack_frame *new_environment_stack_frame = NULL;

    new_environment_stack = virtual_machine_environment_stack_new(vm); 
    if (new_environment_stack == NULL) 
    {
        goto fail;
    }

    environment_stack_frame_cur = environment_stack->begin;
    while (environment_stack_frame_cur != NULL)
    {
        new_environment_stack_frame = virtual_machine_environment_stack_frame_clone(vm, environment_stack_frame_cur);
        if (new_environment_stack_frame == NULL) { goto fail; }
        virtual_machine_environment_stack_append(new_environment_stack, new_environment_stack_frame);
        new_environment_stack_frame = NULL;
        environment_stack_frame_cur = environment_stack_frame_cur->next; 
    }

    goto done;
fail:
    if (new_environment_stack != NULL)
    {
        virtual_machine_environment_stack_destroy(vm, new_environment_stack);
        new_environment_stack = NULL;
    }
    if (new_environment_stack_frame != NULL)
    {
        virtual_machine_environment_stack_frame_destroy(vm, new_environment_stack_frame);
        new_environment_stack_frame = NULL;
    }
done:
    return new_environment_stack;
}

/* Running Stack*/

struct virtual_machine_mask_stack_frame *virtual_machine_mask_stack_frame_new(struct virtual_machine *vm)
{
    struct virtual_machine_mask_stack_frame *new_mask_stack_frame = NULL;

    if ((new_mask_stack_frame = (struct virtual_machine_mask_stack_frame *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_mask_stack_frame))) == NULL)
    { goto fail; }
    new_mask_stack_frame->prev = NULL;
    new_mask_stack_frame->next = NULL;
    new_mask_stack_frame->variables = virtual_machine_variable_list_new(vm);
    if (new_mask_stack_frame->variables == NULL)
    { goto fail; }

    goto done;
fail:
    if (new_mask_stack_frame != NULL)
    {
        virtual_machine_mask_stack_frame_destroy(vm, new_mask_stack_frame);
        new_mask_stack_frame = NULL;
    }
done:
    return new_mask_stack_frame;
}

int virtual_machine_mask_stack_frame_destroy(struct virtual_machine *vm, \
        struct virtual_machine_mask_stack_frame *mask_stack_frame)
{
    if (mask_stack_frame->variables != NULL) virtual_machine_variable_list_destroy(vm, mask_stack_frame->variables);
    virtual_machine_resource_free(vm->resource, mask_stack_frame);

    return 0;
}

struct virtual_machine_mask_stack *virtual_machine_mask_stack_new(struct virtual_machine *vm)
{
    struct virtual_machine_mask_stack *new_mask_stack = NULL;

    if ((new_mask_stack = (struct virtual_machine_mask_stack *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_mask_stack))) == NULL)
    { goto fail; }
    new_mask_stack->bottom = NULL;
    new_mask_stack->top = NULL;
    new_mask_stack->size = 0;
    { goto fail; }

    goto done;
fail:
    if (new_mask_stack != NULL)
    {
        virtual_machine_mask_stack_destroy(vm, new_mask_stack);
        new_mask_stack = NULL;
    }
done:
    return new_mask_stack;
}

int virtual_machine_mask_stack_destroy(struct virtual_machine *vm, \
        struct virtual_machine_mask_stack *mask_stack)
{
    struct virtual_machine_mask_stack_frame *mask_stack_frame_cur, *mask_stack_frame_next;

    mask_stack_frame_cur = mask_stack->bottom;
    while (mask_stack_frame_cur != NULL)
    {
        mask_stack_frame_next = mask_stack_frame_cur->next;
        virtual_machine_mask_stack_frame_destroy(vm, mask_stack_frame_cur);
        mask_stack_frame_cur = mask_stack_frame_next;
    }
    virtual_machine_resource_free(vm->resource, mask_stack);

    return 0;
}

int virtual_machine_mask_stack_push(struct virtual_machine_mask_stack *mask_stack, \
        struct virtual_machine_mask_stack_frame *new_mask_stack_frame)
{
    if (mask_stack->bottom == NULL)
    {
        mask_stack->bottom = mask_stack->top = new_mask_stack_frame;
    }
    else
    {
        new_mask_stack_frame->prev = mask_stack->top;
        mask_stack->top->next = new_mask_stack_frame;
        mask_stack->top = new_mask_stack_frame;
    }
    mask_stack->size += 1;

    return 0;
}

int virtual_machine_mask_stack_pop(struct virtual_machine *vm, \
        struct virtual_machine_mask_stack *mask_stack)
{
    struct virtual_machine_mask_stack_frame *mask_stack_frame_target;

    if (mask_stack->size != 0)
    {
        mask_stack_frame_target = mask_stack->top;
        if (mask_stack->size == 1)
        {
            mask_stack->top = mask_stack->bottom = NULL;
        }
        else
        {
            mask_stack->top->prev->next = NULL;
            mask_stack->top = mask_stack->top->prev;
        }
        virtual_machine_mask_stack_frame_destroy(vm, mask_stack_frame_target);
        mask_stack->size -= 1;
    }

    return 0;
}

int virtual_machine_mask_stack_push_blank(struct virtual_machine *vm, \
        struct virtual_machine_mask_stack *mask_stack)
{
    int ret = 0;
    struct virtual_machine_mask_stack_frame *new_mask_stack_frame = NULL;

    if ((new_mask_stack_frame = virtual_machine_mask_stack_frame_new(vm)) == NULL)
    { goto fail; }
    virtual_machine_mask_stack_push(mask_stack, new_mask_stack_frame);

    goto done;
fail:
    if (new_mask_stack_frame != NULL)
    {
        virtual_machine_mask_stack_frame_destroy(vm, new_mask_stack_frame);
    }
done:
    return ret;
}


struct virtual_machine_running_stack *virtual_machine_running_stack_new(struct virtual_machine *vm)
{
    struct virtual_machine_running_stack *new_stack = NULL;

    if ((new_stack = (struct virtual_machine_running_stack *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_running_stack))) == NULL)
    {
        return NULL;
    }
    new_stack->bottom = new_stack->top = NULL;
    new_stack->size = 0;
    return new_stack;
}

int virtual_machine_running_stack_destroy(struct virtual_machine *vm, struct virtual_machine_running_stack *stack)
{
    struct virtual_machine_running_stack_frame *frame_cur, *frame_next;

    if (stack == NULL) return -MULTIPLE_ERR_NULL_PTR;
    frame_cur = stack->bottom;
    while (frame_cur != NULL)
    {
        frame_next = frame_cur->next;
        virtual_machine_running_stack_frame_destroy(vm, frame_cur);
        frame_cur = frame_next;
    }
    virtual_machine_resource_free(vm->resource, stack);
    return 0;
}

int virtual_machine_running_stack_push(struct virtual_machine_running_stack *stack, \
        struct virtual_machine_running_stack_frame *new_frame)
{
    if (stack == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (new_frame == NULL) return -MULTIPLE_ERR_NULL_PTR; 
    if (stack->bottom == NULL)
    {
        stack->bottom = stack->top = new_frame;
    }
    else
    {
        /* Save the module of the top frame */
        stack->top->next = new_frame;
        new_frame->prev = stack->top;
        stack->top = new_frame;
    }
    stack->size++;
    return 0;
}

int virtual_machine_running_stack_pop(struct virtual_machine *vm, struct virtual_machine_running_stack *stack)
{
    int ret = 0;
    struct virtual_machine_running_stack_frame *new_top = NULL;

    if (stack == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (stack->top == NULL) 
    {
        vm_err_update(vm->r, -VM_ERR_RUNNING_STACK_EMPTY, \
                "runtime error: running stack empty");
        return -MULTIPLE_ERR_VM;
    }
    new_top = stack->top->prev;
    if ((ret = virtual_machine_running_stack_frame_destroy(vm, stack->top)) != 0)
    {
        return ret;
    }
    stack->top = new_top;
    if (new_top != NULL)
    {
        new_top->next = NULL;
    }
    stack->size--;

    if (stack->size == 0) stack->bottom = NULL;

    return 0;
}

int virtual_machine_running_stack_push_with_configure(struct virtual_machine *vm, \
        struct virtual_machine_running_stack *stack, \
        struct virtual_machine_module *module, uint32_t pc, \
        size_t args_count, int closure, \
        uint32_t trap_pc, int trap_enabled, \
        struct virtual_machine_object *environment_entrance, \
        struct virtual_machine_variable_list *variables)
{
    int ret = 0;
    struct virtual_machine_running_stack_frame *new_frame = NULL;

    if (stack == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if ((new_frame = virtual_machine_running_stack_frame_new_with_configure(vm, \
                    module, pc, args_count, closure, \
                    trap_pc, trap_enabled, \
                    environment_entrance, \
                    variables)) == NULL) 
    { ret = -MULTIPLE_ERR_MALLOC; goto fail; }


    if ((ret = virtual_machine_running_stack_push(stack, new_frame)) != 0)
    { goto fail; }

    ret = 0;
    goto done;
fail:
    if (new_frame != NULL) virtual_machine_running_stack_frame_destroy(vm, new_frame);
done:
    return ret;
}

int virtual_machine_running_stack_lift_merge(struct virtual_machine *vm, struct virtual_machine_running_stack *stack)
{
    int ret = 0;
    struct virtual_machine_running_stack_frame *prev_frame = NULL;

    if (stack == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (stack->size <= 1)
    {
        vm_err_update(vm->r, -VM_ERR_RUNNING_STACK_EMPTY, \
                "running error: stack lift merging requires the running stack has at least 2 frames");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    else if (stack->size == 2)
    {
        virtual_machine_running_stack_frame_destroy(vm, stack->top->prev);
        stack->bottom = stack->top;
        stack->top->prev = NULL;
        stack->size = 1;
    }
    else
    {
        prev_frame = stack->top->prev;
        prev_frame->prev->next = stack->top;
        stack->top->prev = prev_frame->prev;
        virtual_machine_running_stack_frame_destroy(vm, prev_frame);
        stack->size -= 1;
    }
fail:
    return ret;
}


/* Message */

struct virtual_machine_message *virtual_machine_message_new(struct virtual_machine *vm)
{
    struct virtual_machine_message *new_message = NULL;

    new_message = (struct virtual_machine_message *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_message));
    if (new_message == NULL) return NULL;
    new_message->tid_from = 0;
    new_message->object = 0;
    new_message->next = 0;
    new_message->prev = 0;

    return new_message;
}

int virtual_machine_message_destroy(struct virtual_machine *vm, struct virtual_machine_message *message)
{
    if (message == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (message->object) virtual_machine_object_destroy(vm, message->object);
    virtual_machine_resource_free(vm->resource, message);

    return 0;
}


struct virtual_machine_message_queue *virtual_machine_message_queue_new(struct virtual_machine *vm)
{
    struct virtual_machine_message_queue *new_queue = NULL;

    new_queue = (struct virtual_machine_message_queue *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_message_queue));
    if (new_queue == NULL) return NULL;
    new_queue->begin = new_queue->end = NULL;
    new_queue->size = 0;

    return new_queue;
}

int virtual_machine_message_queue_destroy(struct virtual_machine *vm, struct virtual_machine_message_queue *queue)
{
    struct virtual_machine_message *message_cur, *message_next;

    if (queue == NULL) return -MULTIPLE_ERR_NULL_PTR;

    message_cur = queue->begin;
    while (message_cur != NULL)
    {
        message_next = message_cur->next;
        virtual_machine_message_destroy(vm, message_cur);

        message_cur = message_next;
    }
    virtual_machine_resource_free(vm->resource, queue);

    return 0;
}

int virtual_machine_message_queue_push(struct virtual_machine_message_queue *queue, struct virtual_machine_message *new_message)
{
    if ((queue == NULL) || (new_message == NULL)) return -MULTIPLE_ERR_NULL_PTR;

    if (queue->begin == NULL)
    {
        queue->begin = queue->end = new_message;
    }
    else
    {
        new_message->prev = queue->end;
        queue->end->next = new_message;
        queue->end = new_message;
    }

    queue->size += 1;

    return 0;

}

int virtual_machine_message_queue_push_with_configure(struct virtual_machine *vm, struct virtual_machine_message_queue *queue, \
        struct virtual_machine_object *new_object, uint32_t tid)
{
    int ret = 0;
    struct virtual_machine_message *new_message = NULL;

    if ((queue == NULL) || (new_object == NULL)) return -MULTIPLE_ERR_NULL_PTR;

    new_message = virtual_machine_message_new(vm);
    if (new_message == NULL)
    {
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }
    new_message->object = new_object;
    new_message->tid_from = tid;

    virtual_machine_message_queue_push(queue, new_message);

    goto done;
fail:
done:
    return ret;
}

int virtual_machine_message_queue_pop(struct virtual_machine_message_queue *queue, struct virtual_machine_message **message_out)
{
    struct virtual_machine_message *new_begin = NULL;

    if (queue == NULL) return -MULTIPLE_ERR_NULL_PTR;


    if (queue->begin != NULL)
    {
        /* Available */
        new_begin = queue->begin->next;

        *message_out = queue->begin;
        queue->begin->next = NULL;
        queue->begin->prev = NULL;

        if (new_begin != NULL)
        { new_begin->prev = NULL; }
        queue->begin = new_begin;

        if (new_begin == NULL) queue->end = NULL;
    }
    else
    {
        *message_out = NULL;
    }

    queue->size -= 1;

    return 0;
}


/* Threading */
struct continuation_list;
struct continuation_list *continuation_list_new( \
        struct virtual_machine *vm);
int continuation_list_destroy( \
        struct virtual_machine *vm, \
        struct continuation_list *list);
struct virtual_machine_thread *virtual_machine_thread_new(struct virtual_machine *vm)
{
    struct virtual_machine_thread *new_thread = NULL;

    if ((new_thread = (struct virtual_machine_thread *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_thread))) == NULL)
    { return NULL; }

    new_thread->next = new_thread->prev = NULL;
    new_thread->zombie = 0;
    new_thread->state = VIRTUAL_MACHINE_THREAD_STATE_NORMAL;
    new_thread->type = VIRTUAL_MACHINE_THREAD_TYPE_NORMAL;
    new_thread->tid = 0;
    new_thread->continuations = NULL;
    new_thread->messages = NULL;
    new_thread->running_stack = NULL;
    if ((new_thread->messages = virtual_machine_message_queue_new(vm)) == NULL)
    { goto fail; }
    if ((new_thread->running_stack = virtual_machine_running_stack_new(vm)) == NULL)
    { goto fail; }
    if ((new_thread->continuations = continuation_list_new(vm)) == NULL)
    { goto fail; }

    return new_thread;
fail:
    if (new_thread != NULL)
    {
        virtual_machine_thread_destroy(vm, new_thread);
    }
    return NULL;
}

int virtual_machine_thread_destroy(struct virtual_machine *vm, struct virtual_machine_thread *thread)
{
    if (thread == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (thread->continuations != NULL) continuation_list_destroy(vm, thread->continuations);
    if (thread->running_stack != NULL) virtual_machine_running_stack_destroy(vm, thread->running_stack);
    if (thread->messages != NULL) virtual_machine_message_queue_destroy(vm, thread->messages);
    virtual_machine_resource_free(vm->resource, thread);

    return 0;
}

struct virtual_machine_thread *virtual_machine_thread_clone(struct virtual_machine *vm, struct virtual_machine_thread *thread)
{
    int ret = 0;
    struct virtual_machine_thread *new_thread = NULL;
    struct virtual_machine_running_stack_frame *stack_frame_cur = NULL, *stack_frame_next = NULL;
    struct virtual_machine_running_stack_frame *new_stack_frame = NULL;

    if ((new_thread = virtual_machine_thread_new(vm)) == NULL)
    {
        goto fail;
    }
    new_thread->zombie = thread->zombie;
    new_thread->state = thread->state;
    new_thread->type = thread->type;
    new_thread->tid = thread->tid;

    stack_frame_cur = thread->running_stack->bottom;
    while (stack_frame_cur != NULL)
    {
        stack_frame_next = stack_frame_cur->next; 
        new_stack_frame = virtual_machine_running_stack_frame_clone(vm, stack_frame_cur);
        if (new_stack_frame == NULL) { goto fail; }
        ret = virtual_machine_running_stack_push(new_thread->running_stack, new_stack_frame);
        if (ret != 0) { goto fail; }
        new_stack_frame = NULL;

        stack_frame_cur = stack_frame_next; 
    }

    return new_thread;
fail:
    if (new_stack_frame != NULL)
    {
        virtual_machine_running_stack_frame_destroy(vm, new_stack_frame);
    }
    if (new_thread != NULL)
    {
        virtual_machine_thread_destroy(vm, new_thread);
    }
    return NULL;
}

struct virtual_machine_thread *virtual_machine_thread_new_with_configure(struct virtual_machine *vm, \
        struct virtual_machine_module *module, uint32_t pc, \
        size_t args_count)
{
    struct virtual_machine_thread *new_thread = NULL;
    struct virtual_machine_object *new_object_environment_entrance = NULL;

    new_object_environment_entrance = virtual_machine_object_environment_entrance_make_blank(vm);
    if (new_object_environment_entrance == NULL) { goto fail; }

    if ((new_thread = virtual_machine_thread_new(vm)) == NULL)
    { goto fail; }
    if (virtual_machine_running_stack_push_with_configure(vm, new_thread->running_stack, \
                module, pc, \
                args_count, 0, \
                0, 0, \
                new_object_environment_entrance, /* Environment */
                NULL /* Variable */ \
                ))
    { goto fail; }

    goto done;
fail:
    if (new_thread != NULL) virtual_machine_thread_destroy(vm, new_thread);
    new_thread = NULL;
done:
    if (new_object_environment_entrance != NULL) { virtual_machine_object_destroy(vm, new_object_environment_entrance); }
    return new_thread;
}

struct virtual_machine_thread_list *virtual_machine_thread_list_new(struct virtual_machine *vm)
{
    struct virtual_machine_thread_list *new_list = NULL;

    if ((new_list = (struct virtual_machine_thread_list *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_thread_list))) == NULL)
    {
        return NULL;
    }
    new_list->begin = new_list->end = NULL;
    new_list->size = 0;
    new_list->tid_pool = 0;
    return new_list;
}

int virtual_machine_thread_list_clear(struct virtual_machine *vm, struct virtual_machine_thread_list *list)
{
    struct virtual_machine_thread *thread_cur, *thread_next;
    if (list == NULL) return -MULTIPLE_ERR_NULL_PTR;

    thread_cur = list->begin;
    while (thread_cur != NULL)
    {
        thread_next = thread_cur->next;
        virtual_machine_thread_destroy(vm, thread_cur);
        thread_cur = thread_next;
    }

    list->begin = list->end = NULL;
    list->size = 0;

    return 0;
}

int virtual_machine_thread_list_print(struct virtual_machine *vm)
{
    struct virtual_machine_thread *thread_cur;
    if (vm == NULL) return -MULTIPLE_ERR_NULL_PTR;

    printf("%u threads at total, current thread is %u\n", (unsigned int)vm->threads->size, (unsigned int)vm->tp->tid);
    thread_cur = vm->threads->begin;
    while (thread_cur != NULL)
    {
        printf("%u (%s)", (unsigned int)thread_cur->tid, thread_cur->state == VIRTUAL_MACHINE_THREAD_STATE_NORMAL ? "normal" : "suspend");
        thread_cur = thread_cur->next;
    }
    printf("\n");
    fflush(stdout);

    return 0;
}

int virtual_machine_thread_list_destroy(struct virtual_machine *vm, struct virtual_machine_thread_list *list)
{
    virtual_machine_thread_list_clear(vm, list);
    virtual_machine_resource_free(vm->resource, list);
    return 0;
}

int virtual_machine_thread_list_append(struct virtual_machine_thread_list *list, struct virtual_machine_thread *new_thread)
{
    if (list == NULL) return -MULTIPLE_ERR_NULL_PTR;

    /* Assign with new thread id */
    new_thread->tid = list->tid_pool++;

    if (list->begin == NULL)
    {
        list->begin = list->end = new_thread;
    }
    else
    {
        list->end->next = new_thread;
        new_thread->prev = list->end;
        list->end = new_thread;
    }
    list->size++;
    return 0;
}

int virtual_machine_thread_list_append_with_configure(struct virtual_machine *vm, \
        struct virtual_machine_thread_list *list, \
        struct virtual_machine_module *module, uint32_t pc, \
        size_t args_count)
{
    int ret = 0;
    struct virtual_machine_thread *new_thread = NULL;

    if ((new_thread = virtual_machine_thread_new_with_configure(vm, \
                    module, pc, \
                    args_count)) == NULL)
    {
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }

    if ((ret = virtual_machine_thread_list_append(list, new_thread)) != 0)
    { goto fail; }

    ret = 0;
    goto done;
fail:
    if (new_thread != NULL) virtual_machine_thread_destroy(vm, new_thread);
    new_thread = NULL;
done:
    return ret;
}

int virtual_machine_thread_list_remove(struct virtual_machine *vm, struct virtual_machine_thread_list *list, struct virtual_machine_thread *thread)
{
    struct virtual_machine_thread *thread_cur = NULL;

    thread_cur = list->begin;
    while (thread_cur != NULL)
    {
        if (thread_cur == thread)
        {
            if (thread_cur->prev != NULL)
            { thread_cur->prev->next = thread_cur->next; }
            else
            { list->begin = thread_cur->next; }
            if (thread_cur->next != NULL)
            { thread_cur->next->prev = thread_cur->prev; }
            else
            { list->end = thread_cur->prev; }
            list->size--;
            virtual_machine_thread_destroy(vm, thread_cur);

            return 0;
        }
        thread_cur = thread_cur->next;
    }
    return 0;
}

int virtual_machine_thread_list_remove_by_tid(struct virtual_machine *vm, \
        struct virtual_machine_thread_list *list, uint32_t tid)
{
    struct virtual_machine_thread *thread_cur = NULL;

    thread_cur = list->begin;
    while (thread_cur != NULL)
    {
        if (thread_cur->tid == tid)
        {
            virtual_machine_thread_list_remove(vm, list, thread_cur);
            return 0;
        }
        thread_cur = thread_cur->next;
    }

    vm_err_update(vm->r, -VM_ERR_OBJECT_NOT_FOUND, \
            "runtime error: thread which id is %u doesn't exist", tid);
    return -MULTIPLE_ERR_VM;
}

int virtual_machine_thread_list_set_state_by_tid(struct virtual_machine *vm, \
        struct virtual_machine_thread_list *list, uint32_t tid, int state)
{
    struct virtual_machine_thread *thread_cur = NULL;

    thread_cur = list->begin;
    while (thread_cur != NULL)
    {
        if (thread_cur->tid == tid)
        {
            thread_cur->state = state;
            return 0;
        }
        thread_cur = thread_cur->next;
    }

    vm_err_update(vm->r, -VM_ERR_OBJECT_NOT_FOUND, \
            "runtime error: thread which id is %u doesn't exist", tid);
    return -MULTIPLE_ERR_VM;
}

int virtual_machine_thread_list_is_tid_exists(struct virtual_machine_thread_list *list, uint32_t tid)
{
    struct virtual_machine_thread *thread_cur = NULL;

    thread_cur = list->begin;
    while (thread_cur != NULL)
    {
        if (thread_cur->tid == tid)
        {
            return 1;
        }
        thread_cur = thread_cur->next; 
    }
    return 0;
}


/* Mutex */

struct virtual_machine_mutex *virtual_machine_mutex_new(struct virtual_machine *vm)
{
    struct virtual_machine_mutex *new_mutex;

    new_mutex = (struct virtual_machine_mutex *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_mutex));
    if (new_mutex == NULL) { goto fail; }
    new_mutex->state = VIRTUAL_MACHINE_MUTEX_UNLOCKED;
    new_mutex->prev = NULL;
    new_mutex->next = NULL;

    goto done;
fail:
    if (new_mutex != NULL)
    {
        virtual_machine_resource_free(vm->resource, new_mutex);
        new_mutex = NULL;
    }
done:
    return new_mutex;
}

int virtual_machine_mutex_destroy(struct virtual_machine *vm, struct virtual_machine_mutex *mutex)
{
    virtual_machine_resource_free(vm->resource, mutex);

    return 0;
}

struct virtual_machine_mutex_list *virtual_machine_mutex_list_new(struct virtual_machine *vm)
{
    struct virtual_machine_mutex_list *new_mutex_list;

    new_mutex_list = (struct virtual_machine_mutex_list *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_mutex_list));
    if (new_mutex_list == NULL) { goto fail; }
    new_mutex_list->begin = NULL;
    new_mutex_list->end = NULL;
    new_mutex_list->size = 0;
    new_mutex_list->mutex_id_pool = 0;

    goto done;
fail:
    if (new_mutex_list != NULL)
    {
        virtual_machine_resource_free(vm->resource, new_mutex_list);
        new_mutex_list = NULL;
    }
done:
    return new_mutex_list;
}

int virtual_machine_mutex_list_destroy(struct virtual_machine *vm, struct virtual_machine_mutex_list *list)
{
    struct virtual_machine_mutex *mutex_cur, *mutex_next;

    mutex_cur = list->begin;
    while (mutex_cur != NULL)
    {
        mutex_next = mutex_cur->next;
        virtual_machine_mutex_destroy(vm, mutex_cur);
        mutex_cur = mutex_next;
    }
    virtual_machine_resource_free(vm->resource, list);

    return 0;
}

int virtual_machine_mutex_list_append(struct virtual_machine_mutex_list *list, \
        struct virtual_machine_mutex *new_mutex)
{
    new_mutex->id = list->mutex_id_pool++;

    if (list->begin == NULL)
    {
        list->begin = list->end = new_mutex;
    }
    else
    {
        new_mutex->prev = list->end;
        list->end->next = new_mutex;
        list->end = new_mutex;
    }
    list->size += 1;

    return 0;
}

int virtual_machine_mutex_list_remove_by_id( \
        struct virtual_machine *vm, \
        struct virtual_machine_mutex_list *list, \
        uint32_t id)
{
    struct virtual_machine_mutex *mutex_cur;

    mutex_cur = list->begin;
    while (mutex_cur != NULL)
    {
        if (mutex_cur->id == id)
        {
            if (mutex_cur->prev != NULL)
            { mutex_cur->prev->next = mutex_cur->next; }
            else
            { list->begin = mutex_cur->next; }
            if (mutex_cur->next != NULL)
            { mutex_cur->next->prev = mutex_cur->prev; }
            else
            { list->end = mutex_cur->prev; }
            list->size--;
            virtual_machine_mutex_destroy(vm, mutex_cur);

            return 0;
        }
        mutex_cur = mutex_cur->next;
    }

    return 0;
}

int virtual_machine_mutex_list_state_by_id( \
        struct virtual_machine_mutex_list *list, \
        uint32_t id)
{
    struct virtual_machine_mutex *mutex_cur;

    mutex_cur = list->begin;
    while (mutex_cur != NULL)
    {
        if (mutex_cur->id == id)
        {
            return mutex_cur->state;
        }
        mutex_cur = mutex_cur->next;
    }

    return -1;
}

int virtual_machine_mutex_list_lock_by_id( \
        struct virtual_machine_mutex_list *list, \
        uint32_t id)
{
    struct virtual_machine_mutex *mutex_cur;

    mutex_cur = list->begin;
    while (mutex_cur != NULL)
    {
        if (mutex_cur->id == id)
        {
            mutex_cur->state = VIRTUAL_MACHINE_MUTEX_LOCKED;
            return 0;
        }
        mutex_cur = mutex_cur->next;
    }

    return -1;
}

int virtual_machine_mutex_list_unlock_by_id( \
        struct virtual_machine_mutex_list *list, \
        uint32_t id)
{
    struct virtual_machine_mutex *mutex_cur;

    mutex_cur = list->begin;
    while (mutex_cur != NULL)
    {
        if (mutex_cur->id == id)
        {
            mutex_cur->state = VIRTUAL_MACHINE_MUTEX_UNLOCKED;
            return 0;
        }
        mutex_cur = mutex_cur->next;
    }

    return -1;
}


/* Semaphore */

/* Semaphore thread queue */

struct virtual_machine_semaphore_thread_queue_item *
virtual_machine_semaphore_thread_queue_item_new( \
        struct virtual_machine *vm, \
        int type, \
        void *ptr_thread)
{
    struct virtual_machine_semaphore_thread_queue_item *new_item = NULL;

    new_item = (struct virtual_machine_semaphore_thread_queue_item *) \
               virtual_machine_resource_malloc( \
                       vm->resource, \
                       sizeof(struct virtual_machine_semaphore_thread_queue_item));
    if (new_item == NULL) { goto fail; }
    new_item->type = type;
    new_item->ptr_thread = ptr_thread;
    new_item->next = NULL;

    goto done;
fail:
    if (new_item != NULL)
    {
        virtual_machine_resource_free(vm->resource, new_item);
        new_item = NULL;
    }
done:
    return new_item;
}

int virtual_machine_semaphore_thread_queue_item_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_semaphore_thread_queue_item *item)
{
    virtual_machine_resource_free(vm->resource, item);
    return 0;
}

struct virtual_machine_semaphore_thread_queue *virtual_machine_semaphore_thread_queue_new(struct virtual_machine *vm)
{
    struct virtual_machine_semaphore_thread_queue *new_queue = NULL;

    new_queue = (struct virtual_machine_semaphore_thread_queue *) \
               virtual_machine_resource_malloc( \
                       vm->resource, \
                       sizeof(struct virtual_machine_semaphore_thread_queue));
    if (new_queue == NULL) { goto fail; }
    new_queue->begin = NULL;
    new_queue->end = NULL;
    new_queue->size = 0;

    goto done;
fail:
    if (new_queue != NULL)
    {
        virtual_machine_resource_free(vm->resource, new_queue);
        new_queue = NULL;
    }
done:
    return new_queue;
}

int virtual_machine_semaphore_thread_queue_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_semaphore_thread_queue *queue)
{
    struct virtual_machine_semaphore_thread_queue_item *item_cur, *item_next;

    item_cur = queue->begin;
    while (item_cur != NULL)
    {
        item_next = item_cur->next; 
        virtual_machine_semaphore_thread_queue_item_destroy(vm, item_cur);
        item_cur = item_next; 
    }
    virtual_machine_resource_free(vm->resource, queue);
    return 0;
}

int virtual_machine_semaphore_thread_queue_push( \
        struct virtual_machine *vm, \
        struct virtual_machine_semaphore_thread_queue *queue, \
        void *ptr_thread)
{
    int ret = 0;
    struct virtual_machine_semaphore_thread_queue_item *new_item = NULL;

    new_item = virtual_machine_semaphore_thread_queue_item_new( \
            vm, \
            VIRTUAL_MACHINE_SEMAPHORE_THREAD_QUEUE_ITEM_TYPE_VM, \
            ptr_thread);
    if (new_item == NULL) { goto fail; }

    if (queue->begin == NULL)
    {
        queue->begin = queue->end = new_item;
    }
    else
    {
        queue->end->next = new_item;
        queue->end = new_item;
    }
    queue->size += 1;

    goto done;
fail:
    if (new_item != NULL)
    {
        virtual_machine_resource_free(vm->resource, new_item);
        new_item = NULL;
    }
done:
    return ret;
}

int virtual_machine_semaphore_thread_queue_push_native( \
        struct virtual_machine *vm, \
        struct virtual_machine_semaphore_thread_queue *queue, \
        int *channel)
{
    int ret = 0;
    struct virtual_machine_semaphore_thread_queue_item *new_item = NULL;

    new_item = virtual_machine_semaphore_thread_queue_item_new( \
            vm, \
            VIRTUAL_MACHINE_SEMAPHORE_THREAD_QUEUE_ITEM_TYPE_NATIVE, \
            channel);
    if (new_item == NULL) { goto fail; }

    if (queue->begin == NULL)
    {
        queue->begin = queue->end = new_item;
    }
    else
    {
        queue->end->next = new_item;
        queue->end = new_item;
    }
    queue->size += 1;

    goto done;
fail:
    if (new_item != NULL)
    {
        virtual_machine_resource_free(vm->resource, new_item);
        new_item = NULL;
    }
done:
    return ret;
}

int virtual_machine_semaphore_thread_queue_head_type( \
        struct virtual_machine_semaphore_thread_queue *queue)
{
    if (queue->size == 0) return VIRTUAL_MACHINE_SEMAPHORE_THREAD_QUEUE_ITEM_TYPE_UNKNOWN;

    return queue->begin->type;
}

void *virtual_machine_semaphore_thread_queue_pop( \
        struct virtual_machine *vm, \
        struct virtual_machine_semaphore_thread_queue *queue)
{
    void *ptr_thread = NULL;
    struct virtual_machine_semaphore_thread_queue_item *item_to_pop;


    if (queue->size == 0) return NULL;

    ptr_thread = queue->begin->ptr_thread;

    item_to_pop = queue->begin;
    queue->begin = queue->begin->next;
    if (queue->begin == NULL) queue->end = NULL;
    queue->size -= 1;

    virtual_machine_semaphore_thread_queue_item_destroy(vm, item_to_pop);

    return ptr_thread;
}

int *virtual_machine_semaphore_thread_queue_pop_native( \
        struct virtual_machine *vm, \
        struct virtual_machine_semaphore_thread_queue *queue)
{
    int *ptr_channel = NULL;
    struct virtual_machine_semaphore_thread_queue_item *item_to_pop;


    if (queue->size == 0) return NULL;

    ptr_channel = queue->begin->ptr_thread;

    item_to_pop = queue->begin;
    queue->begin = queue->begin->next;
    if (queue->begin == NULL) queue->end = NULL;
    queue->size -= 1;

    virtual_machine_semaphore_thread_queue_item_destroy(vm, item_to_pop);

    return ptr_channel;
}

struct virtual_machine_semaphore *virtual_machine_semaphore_new(struct virtual_machine *vm, int value)
{
    struct virtual_machine_semaphore *new_semaphore;

    new_semaphore = (struct virtual_machine_semaphore *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_semaphore));
    if (new_semaphore == NULL) { goto fail; }
    new_semaphore->value = value;
    new_semaphore->suspended_threads = virtual_machine_semaphore_thread_queue_new(vm);
    if (new_semaphore->suspended_threads == NULL) { goto fail; }
    new_semaphore->prev = NULL;
    new_semaphore->next = NULL;

    goto done;
fail:
    if (new_semaphore != NULL)
    {
        if (new_semaphore->suspended_threads != NULL) 
        { virtual_machine_semaphore_thread_queue_destroy(vm, new_semaphore->suspended_threads); }
        virtual_machine_resource_free(vm->resource, new_semaphore);
        new_semaphore = NULL;
    }
done:
    return new_semaphore;
}

int virtual_machine_semaphore_destroy(struct virtual_machine *vm, struct virtual_machine_semaphore *semaphore)
{
    if (semaphore->suspended_threads != NULL) 
    { virtual_machine_semaphore_thread_queue_destroy(vm, semaphore->suspended_threads); }
    virtual_machine_resource_free(vm->resource, semaphore);

    return 0;
}

struct virtual_machine_semaphore_list *virtual_machine_semaphore_list_new(struct virtual_machine *vm)
{
    struct virtual_machine_semaphore_list *new_semaphore_list;

    new_semaphore_list = (struct virtual_machine_semaphore_list *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_semaphore_list));
    if (new_semaphore_list == NULL) { goto fail; }
    new_semaphore_list->begin = NULL;
    new_semaphore_list->end = NULL;
    new_semaphore_list->size = 0;
    new_semaphore_list->semaphore_id_pool = 0;
    new_semaphore_list->confirm_terminated = 0;
    thread_mutex_init(&new_semaphore_list->lock);

    goto done;
fail:
    if (new_semaphore_list != NULL)
    {
        virtual_machine_resource_free(vm->resource, new_semaphore_list);
        new_semaphore_list = NULL;
    }
done:
    return new_semaphore_list;
}

int virtual_machine_semaphore_list_destroy(struct virtual_machine *vm, struct virtual_machine_semaphore_list *list)
{
    struct virtual_machine_semaphore *semaphore_cur, *semaphore_next;

    semaphore_cur = list->begin;
    while (semaphore_cur != NULL)
    {
        semaphore_next = semaphore_cur->next;
        virtual_machine_semaphore_destroy(vm, semaphore_cur);
        semaphore_cur = semaphore_next;
    }
    thread_mutex_uninit(&list->lock);
    virtual_machine_resource_free(vm->resource, list);

    return 0;
}

int virtual_machine_semaphore_list_append(struct virtual_machine_semaphore_list *list, \
        struct virtual_machine_semaphore *new_semaphore)
{
    thread_mutex_lock(&list->lock);

    new_semaphore->id = list->semaphore_id_pool++;

    if (list->begin == NULL)
    {
        list->begin = list->end = new_semaphore;
    }
    else
    {
        new_semaphore->prev = list->end;
        list->end->next = new_semaphore;
        list->end = new_semaphore;
    }
    list->size += 1;

    thread_mutex_unlock(&list->lock);

    return 0;
}

int virtual_machine_semaphore_list_remove_by_id( \
        struct virtual_machine *vm, \
        struct virtual_machine_semaphore_list *list, \
        uint32_t id)
{
    struct virtual_machine_semaphore *semaphore_cur;

    thread_mutex_lock(&list->lock);

    semaphore_cur = list->begin;
    while (semaphore_cur != NULL)
    {
        if (semaphore_cur->id == id)
        {
            if (semaphore_cur->prev != NULL)
            { semaphore_cur->prev->next = semaphore_cur->next; }
            else
            { list->begin = semaphore_cur->next; }
            if (semaphore_cur->next != NULL)
            { semaphore_cur->next->prev = semaphore_cur->prev; }
            else
            { list->end = semaphore_cur->prev; }
            list->size--;
            virtual_machine_semaphore_destroy(vm, semaphore_cur);

            thread_mutex_unlock(&list->lock);

            return 0;
        }
        semaphore_cur = semaphore_cur->next;
    }

    thread_mutex_unlock(&list->lock);

    return 0;
}

int virtual_machine_semaphore_list_value( \
        struct virtual_machine_semaphore_list *list, \
        uint32_t id)
{
    struct virtual_machine_semaphore *semaphore_cur;

    thread_mutex_lock(&list->lock);

    semaphore_cur = list->begin;
    while (semaphore_cur != NULL)
    {
        if (semaphore_cur->id == id)
        {
            thread_mutex_unlock(&list->lock);

            return semaphore_cur->value;
        }
        semaphore_cur = semaphore_cur->next;
    }

    thread_mutex_unlock(&list->lock);

    return -1;
}

/* P Operation (By Virtual Machine) 
 * If resource avaliable perform P operation, 
 * or just return VIRTUAL_MACHINE_SEMAPHORE_TRY_BUSY */
int virtual_machine_semaphore_list_try_p( \
        struct virtual_machine *vm, \
        struct virtual_machine_thread *vm_thread, \
        struct virtual_machine_semaphore_list *list, \
        uint32_t id)
{
    struct virtual_machine_semaphore *semaphore_cur;

    thread_mutex_lock(&list->lock);

    semaphore_cur = list->begin;
    while (semaphore_cur != NULL)
    {
        if (semaphore_cur->id == id)
        {
            /* Found matched id */

            if (semaphore_cur->value > 0)
            {
                /* Resource avaliable, 'p' it */

                /*semaphore_cur->value--;*/
                atomic_dec(&semaphore_cur->value);

                thread_mutex_unlock(&list->lock);
                return 0;
            }
            else
            {
                /* Add thread into suspended thread queue 
                 * to be wake up in the future */
                virtual_machine_semaphore_thread_queue_push( \
                        vm, \
                        semaphore_cur->suspended_threads, \
                        vm_thread);
                vm_thread->state = VIRTUAL_MACHINE_THREAD_STATE_SUSPENDED;

                /* Unlock and return BUSY */
                thread_mutex_unlock(&list->lock);
                return VIRTUAL_MACHINE_SEMAPHORE_TRY_BUSY;
            }
        }
        semaphore_cur = semaphore_cur->next;
    }

    thread_mutex_unlock(&list->lock);

    return VIRTUAL_MACHINE_SEMAPHORE_NO_FOUND;
}

/* P Operation (By Native) */
int virtual_machine_semaphore_list_p( \
        struct virtual_machine *vm, \
        struct virtual_machine_semaphore_list *list, \
        uint32_t id)
{
    struct virtual_machine_semaphore *semaphore_cur;
    int channel;

    thread_mutex_lock(&list->lock);

    semaphore_cur = list->begin;
    while (semaphore_cur != NULL)
    {
        if (semaphore_cur->id == id)
        {
            /* Found matched id */

            if (semaphore_cur->value > 0)
            {
                /* Resource avaliable, 'p' it */
                /*semaphore_cur->value--;*/
                atomic_dec(&semaphore_cur->value);
                thread_mutex_unlock(&list->lock);
                return 0;
            }
            else
            {

                /* Set up a channel for checking if the resource is avaliable */
                channel = VIRTUAL_MACHINE_SEMAPHORE_CHANNEL_UNAVALIABLE;

                /* Add NATIVE thread into suspended thread queue 
                 * to be wake up in the future */
                virtual_machine_semaphore_thread_queue_push_native( \
                        vm, \
                        semaphore_cur->suspended_threads, \
                        &channel);

                /* Wait for it */
                thread_mutex_unlock(&list->lock);

                for (;;)
                {
                    thread_mutex_lock(&list->lock);

                    if (channel == VIRTUAL_MACHINE_SEMAPHORE_CHANNEL_AVALIABLE)
                    {
                        /* Resource avaliable, 'p' it */
                        /*semaphore_cur->value--;*/
                        atomic_dec(&semaphore_cur->value);
                        thread_mutex_unlock(&list->lock);
                        return 0;
                    }

                    /* Do some rest and try again */
                    do_rest();

                    thread_mutex_unlock(&list->lock);
                }
            }
        }
        semaphore_cur = semaphore_cur->next;
    }

    thread_mutex_unlock(&list->lock);

    return VIRTUAL_MACHINE_SEMAPHORE_NO_FOUND;
}

/* V Operation (By Virtual Machine) */
int virtual_machine_semaphore_list_v( \
        struct virtual_machine *vm, \
        struct virtual_machine_semaphore_list *list, \
        uint32_t id)
{
    struct virtual_machine_semaphore *semaphore_cur;
    int thread_type;
    int *ptr_channel;
    struct virtual_machine_thread *ptr_thread;

    thread_mutex_lock(&list->lock);

    semaphore_cur = list->begin;
    while (semaphore_cur != NULL)
    {
        if (semaphore_cur->id == id)
        {
            /*semaphore_cur->value++;*/
            atomic_inc(&semaphore_cur->value);

            /* Resource avaliable */
            if((semaphore_cur->value > 0) && (semaphore_cur->suspended_threads->size != 0))
            {
                /* Suspended thread avaliable, wake it up */
                thread_type = virtual_machine_semaphore_thread_queue_head_type(semaphore_cur->suspended_threads);
                switch (thread_type)
                {
                    case VIRTUAL_MACHINE_SEMAPHORE_THREAD_QUEUE_ITEM_TYPE_UNKNOWN:
                        break;

                    case VIRTUAL_MACHINE_SEMAPHORE_THREAD_QUEUE_ITEM_TYPE_VM:
                        ptr_thread = virtual_machine_semaphore_thread_queue_pop( \
                                vm, \
                                semaphore_cur->suspended_threads);
                        ptr_thread->state = VIRTUAL_MACHINE_THREAD_STATE_NORMAL;
                        break;

                    case VIRTUAL_MACHINE_SEMAPHORE_THREAD_QUEUE_ITEM_TYPE_NATIVE:
                        ptr_channel = virtual_machine_semaphore_thread_queue_pop_native( \
                                vm, \
                                semaphore_cur->suspended_threads);
                        *ptr_channel = VIRTUAL_MACHINE_SEMAPHORE_CHANNEL_AVALIABLE;

                        while (*ptr_channel != VIRTUAL_MACHINE_SEMAPHORE_CHANNEL_AVALIABLE);

                        break;

                    default:
                        break;
                }
            }

            thread_mutex_unlock(&list->lock);

            return 0;
        }
        semaphore_cur = semaphore_cur->next;
    }

    thread_mutex_unlock(&list->lock);

    return VIRTUAL_MACHINE_SEMAPHORE_NO_FOUND;
}


/* Program Loading */

/* Data and code loading routines used while loading program into virtual machine */

struct virtual_machine_data_section *virtual_machine_data_section_new(struct virtual_machine *vm, size_t size)
{
    struct virtual_machine_data_section *new_data_section = NULL;
    size_t idx;

    if ((new_data_section = (struct virtual_machine_data_section *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_data_section))) == NULL)
    {
        return NULL;
    }
    if ((new_data_section->items = (struct virtual_machine_data_section_item *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_data_section_item) * size)) == NULL)
    {
        virtual_machine_resource_free(vm->resource, new_data_section);
        return NULL;
    }
    idx = 0;
    while (idx != size)
    {
        new_data_section->items[idx].ptr = NULL;
        idx++;
    }
    new_data_section->size = size;
    return new_data_section;
}

int virtual_machine_data_section_destroy(struct virtual_machine *vm, struct virtual_machine_data_section *data_section)
{
    size_t idx;

    if (data_section == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (data_section->items != NULL)
    {
        idx = 0;
        while (idx != data_section->size)
        {
            if (data_section->items[idx].ptr != NULL) virtual_machine_resource_free(vm->resource, \
                    data_section->items[idx].ptr);
            idx++;
        }
        virtual_machine_resource_free(vm->resource, data_section->items);
    }
    virtual_machine_resource_free(vm->resource, data_section);
    return 0;
}

struct virtual_machine_text_section *virtual_machine_text_section_new(struct virtual_machine *vm, size_t size)
{
    struct virtual_machine_text_section *new_virtual_machine_text_section = NULL;

    if ((new_virtual_machine_text_section = (struct virtual_machine_text_section *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_text_section))) == NULL)
    {
        return NULL;
    }
    if ((new_virtual_machine_text_section->instruments = (struct virtual_machine_text_section_instrument *)virtual_machine_resource_malloc( \
                    vm->resource, sizeof(struct virtual_machine_text_section_instrument) * size)) == NULL)
    {
        virtual_machine_resource_free(vm->resource, new_virtual_machine_text_section);
        return NULL;
    }
    return new_virtual_machine_text_section;
}

int virtual_machine_text_section_destroy(struct virtual_machine *vm, struct virtual_machine_text_section *text_section)
{
    if (text_section == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (text_section->instruments != NULL) virtual_machine_resource_free(vm->resource, text_section->instruments);
    virtual_machine_resource_free(vm->resource, text_section);
    return 0;
}

struct virtual_machine_export_section *virtual_machine_export_section_new(struct virtual_machine *vm)
{
    struct virtual_machine_export_section *new_export_section = NULL;

    if ((new_export_section = (struct virtual_machine_export_section *)virtual_machine_resource_malloc(vm->resource, sizeof(struct virtual_machine_export_section))) == NULL)
    {
        return NULL;
    }
    new_export_section->exports = NULL;
    new_export_section->size = 0;

    return new_export_section;
}

int virtual_machine_export_section_destroy(struct virtual_machine *vm, struct virtual_machine_export_section *export_section)
{
    size_t index;

    if (export_section == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (export_section->exports != NULL)
    {
        for (index = 0; index != export_section->size; index++)
        {
            if (export_section->exports[index].args != NULL) 
            {
                virtual_machine_resource_free(vm->resource, export_section->exports[index].args);
            }
        }
        virtual_machine_resource_free(vm->resource, export_section->exports);
    }
    virtual_machine_resource_free(vm->resource, export_section);
    return 0;
}

struct virtual_machine_debug_section *virtual_machine_debug_section_new(struct virtual_machine *vm)
{
    struct virtual_machine_debug_section *new_debug_section = NULL;

    new_debug_section = (struct virtual_machine_debug_section *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_debug_section));
    if (new_debug_section == NULL) return NULL;
    new_debug_section->debugs = NULL;
    new_debug_section->size = 0;

    return new_debug_section;
}

int virtual_machine_debug_section_destroy(struct virtual_machine *vm, struct virtual_machine_debug_section *debug_section)
{
    if (debug_section == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (debug_section->debugs != NULL) virtual_machine_resource_free(vm->resource, debug_section->debugs);
    virtual_machine_resource_free(vm->resource, debug_section);

    return 0;
}


struct virtual_machine_source_section *virtual_machine_source_section_new(struct virtual_machine *vm)
{
    struct virtual_machine_source_section *new_source_section = NULL;

    new_source_section = (struct virtual_machine_source_section *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_source_section));
    if (new_source_section == NULL) return NULL;
    new_source_section->code = NULL;
    new_source_section->len = 0;

    return new_source_section;
}

int virtual_machine_source_section_destroy(struct virtual_machine *vm, struct virtual_machine_source_section *source_section)
{
    if (source_section == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (source_section->code != NULL) virtual_machine_resource_free(vm->resource, source_section->code);
    virtual_machine_resource_free(vm->resource, source_section);

    return 0;
}


struct virtual_machine_module *virtual_machine_module_new(struct virtual_machine *vm, uint32_t id)
{
    struct virtual_machine_module *new_module;

    new_module = (struct virtual_machine_module *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_module));
    if (new_module == NULL) return NULL;

    new_module->id = id;
    new_module->data_types = NULL;
    new_module->data_section = NULL;
    new_module->text_section = NULL;
    new_module->export_section = NULL;
    new_module->debug_section = NULL;
    new_module->source_section = NULL;
    new_module->name = NULL;
    new_module->name_len = 0;
    new_module->variables = NULL;
    new_module->next = NULL;

    if ((new_module->data_types = virtual_machine_data_type_list_new(vm)) == NULL)
    { goto fail; }

    if ((new_module->variables = virtual_machine_variable_list_new(vm)) == NULL)
    { goto fail; }

    return new_module;
fail:

    if (new_module != NULL)
    {
        if (new_module->data_types != NULL) virtual_machine_data_type_list_destroy(vm, new_module->data_types);
        if (new_module->variables != NULL) virtual_machine_variable_list_destroy(vm, new_module->variables);
        virtual_machine_resource_free(vm->resource, new_module);
    }

    return NULL;
}

int virtual_machine_module_destroy(struct virtual_machine *vm, struct virtual_machine_module *module)
{
    if (module == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (module->data_section != NULL) virtual_machine_data_section_destroy(vm, module->data_section);
    if (module->text_section != NULL) virtual_machine_text_section_destroy(vm, module->text_section);
    if (module->export_section != NULL) virtual_machine_export_section_destroy(vm, module->export_section);
    if (module->debug_section != NULL) virtual_machine_debug_section_destroy(vm, module->debug_section);
    if (module->source_section != NULL) virtual_machine_source_section_destroy(vm, module->source_section);
    if (module->name != NULL) virtual_machine_resource_free(vm->resource, module->name);
    if (module->data_types != NULL) virtual_machine_data_type_list_destroy(vm, module->data_types);
    if (module->variables != NULL) virtual_machine_variable_list_destroy(vm, module->variables);
    virtual_machine_resource_free(vm->resource, module);

    return 0;
}

struct virtual_machine_module_list *virtual_machine_module_list_new(struct virtual_machine *vm)
{
    struct virtual_machine_module_list *new_list;

    new_list = (struct virtual_machine_module_list *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_module_list));
    if (new_list == NULL) return NULL;
    new_list->begin = new_list->end = NULL;
    new_list->size = 0;

    return new_list;
}

int virtual_machine_module_list_destroy(struct virtual_machine *vm, struct virtual_machine_module_list *list)
{
    struct virtual_machine_module *module_cur, *module_next;

    module_cur = list->begin;
    while (module_cur != NULL)
    {
        module_next = module_cur->next;
        virtual_machine_module_destroy(vm, module_cur);
        module_cur = module_next;
    }
    virtual_machine_resource_free(vm->resource, list);

    return 0;
}

int virtual_machine_module_list_append(struct virtual_machine_module_list *list, struct virtual_machine_module *new_module)
{
    if (list == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (new_module == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (list->begin == NULL)
    {
        list->begin = list->end = new_module;
    }
    else
    {
        list->end->next = new_module;
        list->end = new_module;
    }
    list->size += 1;

    return 0;
}


/* Virtual Machine Shared Library */
struct virtual_machine_shared_library *virtual_machine_shared_library_new(struct virtual_machine *vm)
{
    struct virtual_machine_shared_library *new_shared_library;

    (void)vm;

    new_shared_library = (struct virtual_machine_shared_library *)malloc( \
            sizeof(struct virtual_machine_shared_library));
    if (new_shared_library == NULL) return NULL;

    new_shared_library->handle = NULL;
    new_shared_library->name = NULL;
    new_shared_library->len = 0;

    new_shared_library->next = NULL;

    return new_shared_library;
}

int virtual_machine_shared_library_destroy(struct virtual_machine *vm, struct virtual_machine_shared_library *shared_library)
{
    (void)vm;

    if (shared_library == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (shared_library->name != NULL) free(shared_library->name);
    free(shared_library);

    return 0;
}

struct virtual_machine_shared_library *virtual_machine_shared_library_new_with_configure( \
        struct virtual_machine *vm, char *name, size_t len, void *handle)
{
    struct virtual_machine_shared_library *new_shared_library;

    new_shared_library = virtual_machine_shared_library_new(vm);
    if (new_shared_library == NULL) return NULL;

    new_shared_library->name = (char *)malloc( \
            sizeof(char) * (len + 1));
    if (new_shared_library->name == NULL) goto fail;
    memcpy(new_shared_library->name, name, len);
    new_shared_library->name[len] = '\0';

    new_shared_library->len = len;
    new_shared_library->handle = handle;

    return new_shared_library;
fail:
    if (new_shared_library != NULL) virtual_machine_shared_library_destroy(vm, new_shared_library);
    return NULL;
}

struct virtual_machine_shared_library_list *virtual_machine_shared_library_list_new(struct virtual_machine *vm)
{
    struct virtual_machine_shared_library_list *new_shared_library_list;

    (void)vm;

    new_shared_library_list = (struct virtual_machine_shared_library_list *)malloc( \
            sizeof(struct virtual_machine_shared_library_list));
    if (new_shared_library_list == NULL) return NULL;

    new_shared_library_list->begin = new_shared_library_list->end = NULL;
    new_shared_library_list->size = 0;

    return new_shared_library_list;
}

int virtual_machine_shared_library_list_destroy(struct virtual_machine *vm, struct virtual_machine_shared_library_list *list)
{
    struct virtual_machine_shared_library *shared_library_cur, *shared_library_next;

    if (list == NULL) return -MULTIPLE_ERR_NULL_PTR;

    shared_library_cur = list->begin;
    while (shared_library_cur != NULL)
    {
        shared_library_next = shared_library_cur->next;
        virtual_machine_shared_library_destroy(vm, shared_library_cur);

        shared_library_cur = shared_library_next;
    }
    free(list);

    return 0;
}

int virtual_machine_shared_library_list_append(struct virtual_machine_shared_library_list *list, struct virtual_machine_shared_library *new_lib)
{
    if (list == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (list->begin == NULL)
    {
        list->begin = list->end = new_lib;
    }
    else
    {
        list->end->next = new_lib;
        list->end = new_lib;
    }
    list->size += 1;

    return 0;
}

int virtual_machine_shared_library_list_lookup(struct virtual_machine_shared_library_list *list, void **handle_out, char *name, size_t len)
{
    struct virtual_machine_shared_library *shared_library_cur;

    if (list == NULL) return -MULTIPLE_ERR_NULL_PTR;

    shared_library_cur = list->begin;
    while (shared_library_cur != NULL)
    {
        if ((shared_library_cur->len == len) && (strncmp(shared_library_cur->name, name, len) == 0))
        {
            *handle_out = shared_library_cur->handle;
            return 1;
        } 

        shared_library_cur = shared_library_cur->next;
    }

    *handle_out = NULL;
    return 0;
}


/* External Event */
struct virtual_machine_external_event *virtual_machine_external_event_new(struct virtual_machine *vm, \
        struct virtual_machine_object *object_callback, uint32_t id)
{
    struct virtual_machine_external_event *new_external_event = NULL;

    new_external_event = (struct virtual_machine_external_event *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_external_event));
    if (new_external_event == NULL) { goto fail; }

    new_external_event->id = id;
    new_external_event->object_callback = virtual_machine_object_clone(vm, object_callback);
    if (new_external_event->object_callback == NULL) { goto fail; }

    new_external_event->raised = 0;
    new_external_event->args = virtual_machine_computing_stack_new(vm);
    if (new_external_event->args == NULL) { goto fail; }

    new_external_event->next = NULL;

    goto done;
fail:
    if (new_external_event != NULL)
    {
        if (new_external_event->args) virtual_machine_computing_stack_destroy(vm, new_external_event->args);
        if (new_external_event->object_callback) virtual_machine_object_destroy(vm, new_external_event->object_callback);
        virtual_machine_resource_free(vm->resource, new_external_event);
    }
done:
    return new_external_event;
}

int virtual_machine_external_event_destroy(struct virtual_machine *vm, \
    struct virtual_machine_external_event *external_event)
{
    if (external_event->args) virtual_machine_computing_stack_destroy(vm, external_event->args);
    if (external_event->object_callback) virtual_machine_object_destroy(vm, external_event->object_callback);
    virtual_machine_resource_free(vm->resource, external_event);

    return 0;
}

struct virtual_machine_external_event_list *virtual_machine_external_event_list_new(struct virtual_machine *vm)
{
    struct virtual_machine_external_event_list *new_external_event_list = NULL;

    new_external_event_list = (struct virtual_machine_external_event_list *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct virtual_machine_external_event_list));
    if (new_external_event_list == NULL) { goto fail; }
    new_external_event_list->begin = new_external_event_list->end = NULL;
    new_external_event_list->id = 0;
    thread_mutex_init(&new_external_event_list->lock);

fail:
    return new_external_event_list;
}

int virtual_machine_external_event_list_destroy(struct virtual_machine *vm, \
    struct virtual_machine_external_event_list *external_event_list)
{
    struct virtual_machine_external_event *external_event_cur, *external_event_next;

    external_event_cur = external_event_list->begin;
    while (external_event_cur != NULL)
    {
        external_event_next = external_event_cur->next;
        virtual_machine_external_event_destroy(vm, external_event_cur);
        external_event_cur = external_event_next;
    }

    thread_mutex_uninit(&external_event_list->lock);
    virtual_machine_resource_free(vm->resource, external_event_list);

    return 0;
}

int virtual_machine_external_event_list_append(struct virtual_machine_external_event_list *external_event_list, \
        struct virtual_machine_external_event *new_external_event)
{

    if (external_event_list->begin == NULL)
    {
        external_event_list->begin = external_event_list->end = new_external_event;
    }
    else
    {
        external_event_list->end->next = new_external_event;
        external_event_list->end = new_external_event;
    }

    return 0;
}

uint32_t virtual_machine_external_event_list_get_id( \
        struct virtual_machine_external_event_list *external_event_list)
{
    uint32_t id;

    id = external_event_list->id;
    external_event_list->id++;

    return id;
}

struct virtual_machine_external_event *virtual_machine_external_event_list_lookup( \
        struct virtual_machine *vm, \
        struct virtual_machine_external_event_list *external_event_list, uint32_t id)
{
    struct virtual_machine_external_event *external_event_cur, *external_event_next;

    (void)vm;

    external_event_cur = external_event_list->begin;
    while (external_event_cur != NULL)
    {
        external_event_next = external_event_cur->next;
        if (external_event_cur->id == id) { break; }
        external_event_cur = external_event_next;
    }

    return external_event_cur;
}

int virtual_machine_external_event_list_raise( \
        struct virtual_machine *vm, \
        struct virtual_machine_external_event_list *external_event_list, uint32_t id)
{
    struct virtual_machine_external_event *external_event_target;

    external_event_target = virtual_machine_external_event_list_lookup(vm, external_event_list, id);
    if (external_event_target == NULL) return -1;

    external_event_target->raised += 1;

    return 0;
}

int virtual_machine_external_event_list_count( \
        struct virtual_machine *vm, \
        struct virtual_machine_external_event_list *external_event_list, uint32_t id, size_t *count)
{
    struct virtual_machine_external_event *external_event_target;

    external_event_target = virtual_machine_external_event_list_lookup(vm, external_event_list, id);
    if (external_event_target == NULL) return -1;

    *count = external_event_target->args->size;

    return 0;
}


#define CHAR_TOUPPER(x) ((char)((('a'<=(x))&&((x)<='z'))?((char)((int)(x)-(int)('a'-'A'))):(x)))

/* Initialize Built-in Variables */
static int virtual_machine_initialize_builtin_variables(struct virtual_machine *vm)
{
    int ret = 0;
    struct virtual_machine_variable *new_variable = NULL;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_module *new_module = NULL, *target_module;
    uint32_t value;
    uint32_t module_id;
    size_t data_section_item_count;
    char *type_name;
    size_t type_name_len;
    struct virtual_machine_data_section_item *data_section_item;
    char *p, *endp;

    if (vm == NULL) 
    {
        ret = -MULTIPLE_ERR_NULL_PTR;
        goto fail;
    }

    /* Create a new module for containing module id and res id of variables */
    if ((new_module = virtual_machine_module_new(vm, (uint32_t)(vm->modules->size))) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }
    if ((ret = virtual_machine_module_list_append(vm->modules, new_module)) != 0)
    { goto fail; }
    target_module = new_module; new_module = NULL;
    module_id = target_module->id; 

    /* Data Section */
    data_section_item_count = OBJECT_TYPE_COUNT;
    if ((target_module->data_section = virtual_machine_data_section_new(vm, data_section_item_count)) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }
    data_section_item = target_module->data_section->items;

    /* Types */
    for (value = OBJECT_TYPE_FIRST; value != OBJECT_TYPE_FINAL; value++)
    {
        virtual_machine_object_id_to_type_name(&type_name, &type_name_len, value);
        if (type_name == NULL) break;

        /* Data Section Item */
        data_section_item->module = target_module;
        data_section_item->id = value;
        data_section_item->type = MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER;
        data_section_item->size = (uint32_t)type_name_len;
        if ((data_section_item->ptr = (void *)virtual_machine_resource_malloc(vm->resource, (size_t)data_section_item->size + 1)) == NULL)
        {
            VM_ERR_MALLOC(vm->r);
            ret = -MULTIPLE_ERR_VM;
            goto fail; 
        }
        memcpy(data_section_item->ptr, type_name, type_name_len);
        ((char *)data_section_item->ptr)[(size_t)data_section_item->size] = '\0';
        p = data_section_item->ptr;
        endp = p + type_name_len;
        while (p != endp)
        {
            *p = CHAR_TOUPPER(*p);
            p++;
        }

        /* Variable */
        if ((new_object = virtual_machine_object_type_new_with_value(vm, value)) == NULL)
        { goto fail; }
        if ((new_variable = virtual_machine_variable_new(vm, module_id, value)) == NULL)
        { ret = -MULTIPLE_ERR_MALLOC; goto fail; }
        new_variable->ptr = new_object;
        new_object = NULL;
        if ((ret = virtual_machine_variable_list_append(vm->variables_builtin, new_variable)) != 0)
        { goto fail; }
        new_variable = NULL;

        /* Next Data Section Item */
        data_section_item++;
    }

    /* Stdin */
    /*
       if ((new_object = virtual_machine_object_file_new_from_fp_mode_str(stdin, "stdin", "r")) == NULL) 
       { ret = -MULTIPLE_ERR_MALLOC; goto fail; }
       if ((new_variable = virtual_machine_variable_new(0, OBJECT_TYPE_FILE)) == NULL)
       { ret = -MULTIPLE_ERR_MALLOC; goto fail; }
       new_variable->ptr = new_object;
       new_object = NULL;
       if ((ret = virtual_machine_variable_list_append(vm->variables_builtin, new_variable)) != 0)
       { goto fail; }
       new_variable = NULL;
       */

    /* Stdout */
    /*
       if ((new_object = virtual_machine_object_file_new_from_fp_mode_str(stdout, "stdout", "w")) == NULL) 
       { ret = -MULTIPLE_ERR_MALLOC; goto fail; }
       if ((new_variable = virtual_machine_variable_new(1, OBJECT_TYPE_FILE)) == NULL)
       { ret = -MULTIPLE_ERR_MALLOC; goto fail; }
       new_variable->ptr = new_object;
       new_object = NULL;
       if ((ret = virtual_machine_variable_list_append(vm->variables_builtin, new_variable)) != 0)
       { goto fail; }
       new_variable = NULL;
       */

    ret = 0;
    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
    if (new_variable != NULL) virtual_machine_variable_destroy(vm, new_variable);
    if (new_module != NULL) virtual_machine_module_destroy(vm, new_module);
done:
    return ret;
}

/* Debugger */

struct virtual_machine_debugger *virtual_machine_debugger_new(void)
{
    struct virtual_machine_debugger *new_debugger = NULL;

    new_debugger = (struct virtual_machine_debugger *)malloc(sizeof(struct virtual_machine_debugger));
    if (new_debugger == NULL) { goto fail; }
    new_debugger->tp_debugger = NULL;
    new_debugger->tp_target = NULL;

    goto done;
fail:
    if (new_debugger != NULL)
    {
        free(new_debugger);
        new_debugger = NULL;
    }
done:
    return new_debugger;
}

int virtual_machine_debugger_destroy(struct virtual_machine_debugger *debugger)
{
    if (debugger == NULL) return -MULTIPLE_ERR_NULL_PTR;

    free(debugger);

    return 0;
}


/* Virtual Machine */

/* 2 frames preallocated initially */
#define STACK_SIZE_RESERVED (1)

/* Note: Virtual Machine related routines, entrance of program running */

int virtual_machine_interrupt_init(struct virtual_machine *vm);

/* Create a blank virtual machine */
struct virtual_machine *virtual_machine_new(struct virtual_machine_startup *startup, struct vm_err *r)
{
    struct virtual_machine *new_vm = NULL;

    if ((new_vm = (struct virtual_machine *)malloc( \
                    sizeof(struct virtual_machine))) == NULL)
    { return NULL; }
    new_vm->gc_stub = NULL;
    new_vm->resource = NULL;
    new_vm->modules = NULL;
    new_vm->shared_libraries = NULL;
    new_vm->keep_dll = 0;
    new_vm->tp = NULL;
    new_vm->step_in_time_slice = 0;
    new_vm->time_slice = TIME_SLICE_DEFAULT;
    new_vm->stack_size = STACK_SIZE_DEFAULT + STACK_SIZE_RESERVED;
    new_vm->threads = NULL;
    new_vm->variables_global = NULL;
    new_vm->variables_builtin = NULL;
    new_vm->external_functions = NULL;
    new_vm->external_events = NULL;
    new_vm->mutexes = NULL;
    new_vm->semaphores = NULL;
    new_vm->locked = 0;
    new_vm->data_types = NULL;
    new_vm->r = r;
    new_vm->debugger = NULL;
    new_vm->debug_mode = 0;
    new_vm->debug_info = 0;
    thread_mutex_init(&new_vm->gil);
    virtual_machine_interrupt_init(new_vm);
    /* Resource */
    if ((new_vm->gc_stub = gc_stub_new()) == NULL) goto fail;
    if ((new_vm->resource = virtual_machine_resource_new(startup)) == NULL) goto fail;
    if ((new_vm->modules = virtual_machine_module_list_new(new_vm)) == NULL) goto fail;
    if ((new_vm->shared_libraries = virtual_machine_shared_library_list_new(new_vm)) == NULL) goto fail;
    if ((new_vm->threads = virtual_machine_thread_list_new(new_vm)) == NULL) goto fail;
    if ((new_vm->variables_global = virtual_machine_variable_list_new(new_vm)) == NULL) goto fail;
    if ((new_vm->variables_builtin = virtual_machine_variable_list_new(new_vm)) == NULL) goto fail;
    if (virtual_machine_initialize_builtin_variables(new_vm) != 0) goto fail;
    if ((new_vm->data_types = virtual_machine_data_type_list_new(new_vm)) == NULL) goto fail; 
    if ((new_vm->external_events = virtual_machine_external_event_list_new(new_vm)) == NULL) goto fail; 
    if ((new_vm->mutexes = virtual_machine_mutex_list_new(new_vm)) == NULL) goto fail;
    if ((new_vm->semaphores = virtual_machine_semaphore_list_new(new_vm)) == NULL) goto fail;
    if ((new_vm->debugger = virtual_machine_debugger_new()) == NULL) goto fail;
    goto done;
fail:
    if (new_vm != NULL)
    {
        if (new_vm->threads != NULL) virtual_machine_thread_list_destroy(new_vm, new_vm->threads);
        if (new_vm->variables_global != NULL) virtual_machine_variable_list_destroy(new_vm, new_vm->variables_global);
        if (new_vm->variables_builtin != NULL) virtual_machine_variable_list_destroy(new_vm, new_vm->variables_builtin);
        if (new_vm->external_events != NULL) virtual_machine_external_event_list_destroy(new_vm, new_vm->external_events);
        if (new_vm->mutexes != NULL) virtual_machine_mutex_list_destroy(new_vm, new_vm->mutexes);
        if (new_vm->semaphores != NULL) virtual_machine_semaphore_list_destroy(new_vm, new_vm->semaphores);
        if (new_vm->shared_libraries != NULL) virtual_machine_shared_library_list_destroy(new_vm, new_vm->shared_libraries);
        if (new_vm->modules != NULL) virtual_machine_module_list_destroy(new_vm, new_vm->modules);
        if (new_vm->data_types != NULL) virtual_machine_data_type_list_destroy(new_vm, new_vm->data_types);
        if (new_vm->resource != NULL) virtual_machine_resource_destroy(new_vm->resource);
        if (new_vm->debugger != NULL) virtual_machine_debugger_destroy(new_vm->debugger);
        free(new_vm);
        new_vm = NULL;
    }
done:
    return new_vm;
}

static int vm_shared_libraries_close(struct virtual_machine *vm)
{
    int ret = 0;
    struct virtual_machine_shared_library *shared_library_cur;

    shared_library_cur = vm->shared_libraries->begin;
    while (shared_library_cur != NULL)
    {
        if (shared_library_cur->handle != NULL) 
        {
            ret = virtual_machine_dynlib_close_handle(vm, shared_library_cur->handle);
            if (ret != 0) goto fail;
        }

        shared_library_cur = shared_library_cur->next;
    }
fail:
    return ret;
}

/* Destroy an existent virtual machine */
int virtual_machine_destroy(struct virtual_machine *vm, int error_occurred)
{
    if (vm == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (vm->threads != NULL) virtual_machine_thread_list_destroy(vm, vm->threads);
    if (vm->modules != NULL) virtual_machine_module_list_destroy(vm, vm->modules);
    if (vm->variables_global != NULL) virtual_machine_variable_list_destroy(vm, vm->variables_global);
    if (vm->variables_builtin != NULL) virtual_machine_variable_list_destroy(vm, vm->variables_builtin);
    if (vm->external_events != NULL) virtual_machine_external_event_list_destroy(vm, vm->external_events);

    if (error_occurred == 0)
    {
        if (vm->mutexes != NULL) virtual_machine_mutex_list_destroy(vm, vm->mutexes);
        if (vm->semaphores != NULL) virtual_machine_semaphore_list_destroy(vm, vm->semaphores);
        if (vm->gc_stub != NULL) gc_stub_destroy(vm->gc_stub);
        if (vm->resource != NULL) virtual_machine_resource_destroy(vm->resource);
        vm_shared_libraries_close(vm);
    }

    if (vm->data_types != NULL) virtual_machine_data_type_list_destroy(vm, vm->data_types);

    if (vm->shared_libraries != NULL) virtual_machine_shared_library_list_destroy(vm, vm->shared_libraries);
    if (vm->debugger != NULL) virtual_machine_debugger_destroy(vm->debugger);
    thread_mutex_uninit(&vm->gil);

    free(vm);

    return 0;
}

/* Interrupt */
int virtual_machine_interrupt_init(struct virtual_machine *vm)
{
    vm->interrupt_enabled = VIRTUAL_MACHINE_IE_ALL;
    return 0;
}

int virtual_machine_interrupt_gc_enable(struct virtual_machine *vm)
{
    vm->interrupt_enabled |= VIRTUAL_MACHINE_IE_GC;
    return 0;
}

int virtual_machine_interrupt_gc_disable(struct virtual_machine *vm)
{
    vm->interrupt_enabled &= ~VIRTUAL_MACHINE_IE_GC;
    return 0;
}

/* GIL */
int virtual_machine_gil_lock(struct virtual_machine *vm)
{
    thread_mutex_lock(&vm->gil);
    return 0;
}

int virtual_machine_gil_unlock(struct virtual_machine *vm)
{
    thread_mutex_unlock(&vm->gil);
    return 0;
}


/* Utilities */
int virtual_machine_module_lookup_data_section_items(struct virtual_machine_module *module, \
        struct virtual_machine_data_section_item **item_out, uint32_t id)
{
    size_t idx;

    for (idx = 0; idx != module->data_section->size; idx++)
    {
        if (module->data_section->items[idx].id == id)
        {
            *item_out = &module->data_section->items[idx];
            return LOOKUP_FOUND;
        }
    }
    return LOOKUP_NOT_FOUND;
}

int virtual_machine_module_lookup_data_section_id(uint32_t *id_out, struct virtual_machine_module *module, const char *name, const size_t len, const uint32_t type)
{
    size_t idx;

    *id_out = 0;
    if (module->data_section != NULL)
    {
        for (idx = 0; idx != module->data_section->size; idx++)
        {
            if ((module->data_section->items[idx].type == type) &&
                    ((size_t)(module->data_section->items[idx].size) == len) && 
                    (strncmp((const char *)(module->data_section->items[idx].ptr), name, len) == 0))
            {
                *id_out = module->data_section->items[idx].id;
                return LOOKUP_FOUND;
            }
        }
    }
    return LOOKUP_NOT_FOUND;
}

int virtual_machine_module_lookup_by_name(struct virtual_machine_module **module_out, \
        struct virtual_machine *vm, \
        const char *module_name, const size_t module_name_len)
{
    struct virtual_machine_module *module_cur;

    *module_out = NULL;

    if (vm->modules == NULL) return -VM_ERR_NULL_PTR;

    module_cur = vm->modules->begin;
    while (module_cur != NULL)
    {
        if ((module_cur->name_len == module_name_len) && \
                (strncmp(module_cur->name, module_name, module_name_len) == 0))
        {
            *module_out = module_cur;
            return LOOKUP_FOUND;
        }
        module_cur = module_cur->next;
    }
    return LOOKUP_NOT_FOUND;
}

int virtual_machine_module_lookup_by_id(struct virtual_machine_module **module_out, \
        struct virtual_machine *vm, uint32_t id)
{
    struct virtual_machine_module *module_cur = vm->modules->begin;

    *module_out = NULL;
    while (module_cur != NULL)
    {
        if (module_cur->id == id)
        {
            *module_out = module_cur;
            return LOOKUP_FOUND;
        }
        module_cur = module_cur->next;
    }
    return LOOKUP_NOT_FOUND;
}


int virtual_machine_module_function_lookup_by_id(uint32_t *function_instrument_number_out, struct virtual_machine_module *module, const uint32_t function_id)
{
    uint32_t function_name_data_section_id = function_id;
    size_t idx;

    for (idx = 0; idx != module->export_section->size; idx++)
    {
        if (module->export_section->exports[idx].name == function_name_data_section_id)
        {
            *function_instrument_number_out = module->export_section->exports[idx].instrument_number;
            return LOOKUP_FOUND;
        }
    }

    return LOOKUP_NOT_FOUND;
}

int virtual_machine_module_function_lookup_by_name( \
        uint32_t *function_instrument_number_out, \
        struct virtual_machine_module *module, \
        const char *function_name, const size_t function_name_len)
{
    int ret;
    uint32_t function_name_data_section_id = 0;

    ret = virtual_machine_module_lookup_data_section_id(&function_name_data_section_id, module, \
            function_name, function_name_len, MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER);
    if (ret == LOOKUP_NOT_FOUND) return LOOKUP_NOT_FOUND;

    ret = virtual_machine_module_function_lookup_by_id(function_instrument_number_out, module, function_name_data_section_id);
    if (ret == LOOKUP_NOT_FOUND) return LOOKUP_NOT_FOUND;

    return LOOKUP_FOUND;
}

int virtual_machine_function_lookup_external_function( \
        struct virtual_machine *vm, \
        int (**func_out)(struct multiple_stub_function_args *args), \
        struct multiple_stub_function_args **args_out, \
        char *function_name, size_t function_name_len)
{
    struct multiple_stub_function *function_cur;

    function_cur = vm->external_functions->begin;
    while (function_cur != NULL)
    {
        if ((function_cur->name_len == function_name_len) && \
                (strncmp(function_cur->name, function_name, function_name_len) == 0))
        {
            (*func_out) = function_cur->func;
            (*args_out) = function_cur->args;
            return LOOKUP_FOUND;
        }
        function_cur = function_cur->next;
    }
    return LOOKUP_NOT_FOUND;
}

static int virtual_machine_variable_list_lookup_closure(struct virtual_machine_variable **variable_out, \
        const struct virtual_machine_running_stack_frame *running_stack_frame_start, \
        uint32_t module_id, uint32_t id)
{
    int ret = 0;
    struct virtual_machine_running_stack_frame *running_stack_frame_cur;

    if ((ret = virtual_machine_variable_list_lookup(variable_out, \
                    running_stack_frame_start->variables, module_id, id)) == LOOKUP_FOUND)
    { return LOOKUP_FOUND; }

    running_stack_frame_cur = (struct virtual_machine_running_stack_frame *)running_stack_frame_start;
    while ((running_stack_frame_cur != NULL) && (running_stack_frame_cur->closure != 0))
    {
        running_stack_frame_cur = running_stack_frame_cur->prev; 
        if ((ret = virtual_machine_variable_list_lookup(variable_out, \
                        running_stack_frame_cur->variables, module_id, id)) == LOOKUP_FOUND)
        { return LOOKUP_FOUND; }
    }
    return LOOKUP_NOT_FOUND;
}

static int virtual_machine_variable_list_lookup_environment( \
        struct virtual_machine *vm, \
        struct virtual_machine_variable **variable_out, \
        const struct virtual_machine_running_stack_frame *running_stack_frame_cur, \
        uint32_t module_id, uint32_t id)
{
    int ret = 0;
    /*struct virtual_machine_running_stack_frame *running_stack_frame_cur;*/

    struct virtual_machine_environment_stack_frame *environment_stack_frame_cur;

    /*running_stack_frame_cur = running_stack->top;*/
    while (running_stack_frame_cur != NULL)
    {
        if ((ret = virtual_machine_object_environment_entrance_extract_environment_stack_frame( \
                        &environment_stack_frame_cur, vm, running_stack_frame_cur->environment_entrance)) != 0)
        { goto fail; }

        while (environment_stack_frame_cur != NULL)
        {
            if ((environment_stack_frame_cur->variables != NULL) && \
                    ((ret = virtual_machine_variable_list_lookup(variable_out, \
                                                                 environment_stack_frame_cur->variables, \
                                                                 module_id, \
                                                                 id)) == LOOKUP_FOUND))
            { return LOOKUP_FOUND; }
            environment_stack_frame_cur = environment_stack_frame_cur->prev; 
        }

        running_stack_frame_cur = running_stack_frame_cur->prev;
    }

fail:
    return LOOKUP_NOT_FOUND;
}

/* Solve variables into specific values */
int virtual_machine_variable_solve( \
        struct virtual_machine_object **object_dst, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_running_stack_frame *target_frame, \
        const int marking_error, /* While solving failed, runtime error happens */ \
        struct virtual_machine *vm)
{
    int ret = 0;
    struct virtual_machine_thread *thread = vm->tp;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_variable *var = NULL;
    struct virtual_machine_object_identifier *object_identifier = NULL;
    uint32_t operand;
    uint32_t module_id;
    int (*extern_func)(struct multiple_stub_function_args *args);
    struct multiple_stub_function_args *extern_func_args;

    struct virtual_machine_module *module_target = NULL;
    uint32_t instrument_number;

    struct virtual_machine_object *new_object;

    *object_dst = NULL;
    current_running_stack = thread->running_stack;
    current_frame = current_running_stack->top;
    if (target_frame == NULL) target_frame = current_frame;

    if (object_src->type == OBJECT_TYPE_IDENTIFIER)
    {
        object_identifier = (struct virtual_machine_object_identifier *)object_src->ptr;
        operand = object_identifier->id_data_id;
        module_id = object_identifier->id_module_id;


        /* If module specified */
        if (object_identifier->domain_id != NULL)
        {
            /* Lookup domain */
            if ((virtual_machine_module_lookup_by_name(&module_target, \
                            vm, \
                            object_identifier->domain_id, \
                            object_identifier->domain_id_len) == LOOKUP_NOT_FOUND))
            { module_target = NULL; }
        }
        else { module_target = NULL; }

        if (module_target != NULL)
        {
            /* Domain specified */
            if ((ret = virtual_machine_variable_list_lookup(&var, \
                            module_target->variables, module_id, operand)) == LOOKUP_FOUND)
            {
                /* Module Variable */
                if ((new_object = virtual_machine_object_clone(vm, var->ptr)) == NULL)
                { ret = -MULTIPLE_ERR_MALLOC; goto fail; }
            }
            else if ((virtual_machine_module_function_lookup_by_name( \
                            &instrument_number, \
                            module_target, \
                            object_identifier->id, object_identifier->id_len)) == LOOKUP_FOUND)
            {
                /* Internal Function */
                if ((new_object = virtual_machine_object_func_make_internal( \
                                module_target, instrument_number, \
                                vm)) == NULL)
                { goto fail; }
            }
            else
            {
                /* Not found */
                if (marking_error != 0)
                {
                    vm_err_update(vm->r, -VM_ERR_UNDEFINED_SYMBOL, \
                            "runtime error: variable \'%s:%s' not found", \
                            object_identifier->domain_id, object_identifier->id);
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }
                else
                {
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }
            }
        }
        else
        {
            /* domain not specified */
            /*
               if ((ret = virtual_machine_object_environment_entrance_extract_environment_stack_frame( \
               &environment_stack_frame, vm, target_frame->environment_entrance)) != 0)
               { goto fail; }
               */

            /* If it is a variable, push the value of variable to the top of the stack,*/
            /* or push the symbol directly */
            if ((ret = virtual_machine_variable_list_lookup(&var, \
                            target_frame->variables, \
                            module_id, \
                            operand)) == LOOKUP_FOUND)
            {
                /* Local Variable */
                if ((new_object = virtual_machine_object_clone(vm, var->ptr)) == NULL)
                { ret = -MULTIPLE_ERR_MALLOC; goto fail; }
            }
            else if ((ret = virtual_machine_variable_list_lookup_environment(vm, \
                            &var, \
                            target_frame, \
                            module_id, \
                            operand)) == LOOKUP_FOUND)

            {
                /* Environment */
                if ((new_object = virtual_machine_object_clone(vm, var->ptr)) == NULL)
                { ret = -MULTIPLE_ERR_MALLOC; goto fail; }
            }
            else if ((ret = virtual_machine_variable_list_lookup_closure(&var, \
                            target_frame, module_id, operand)) == LOOKUP_FOUND)
            {
                /* Local Variable */
                if ((new_object = virtual_machine_object_clone(vm, var->ptr)) == NULL)
                { ret = -MULTIPLE_ERR_MALLOC; goto fail; }
            }
            else if ((ret = virtual_machine_variable_list_lookup(&var, \
                            current_frame->module->variables, module_id, operand)) == LOOKUP_FOUND)
            {
                /* Module Variable */
                if ((new_object = virtual_machine_object_clone(vm, var->ptr)) == NULL)
                { ret = -MULTIPLE_ERR_MALLOC; goto fail; }
            }
            else if ((ret = virtual_machine_variable_list_lookup_global(&var, \
                            vm->variables_global, object_identifier->id, object_identifier->id_len, vm)) == LOOKUP_FOUND)
            {
                /* Global Variable */
                if ((new_object = virtual_machine_object_clone(vm, var->ptr)) == NULL)
                { ret = -MULTIPLE_ERR_MALLOC; goto fail; }
            }
            else if ((ret = virtual_machine_variable_list_lookup_global(&var, \
                            vm->variables_builtin, object_identifier->id, object_identifier->id_len, vm)) == LOOKUP_FOUND)
            {
                /* Built-in Variable */
                if ((new_object = virtual_machine_object_clone(vm, var->ptr)) == NULL)
                { ret = -MULTIPLE_ERR_MALLOC; goto fail; }
            }
            else if (virtual_machine_function_lookup_external_function(vm, &extern_func, &extern_func_args, \
                        object_identifier->id, \
                        object_identifier->id_len) == LOOKUP_FOUND)
            {
                /* Extern Function */
                if ((new_object = virtual_machine_object_func_make_external(extern_func, extern_func_args, vm)) == NULL)
                { ret = -MULTIPLE_ERR_MALLOC; goto fail; }
            }
            else
            {
                /* Not found */
                if (marking_error != 0)
                {
                    vm_err_update(vm->r, -VM_ERR_UNDEFINED_SYMBOL, \
                            "runtime error: variable \'%s\' not found", \
                            object_identifier->id);
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }
                else
                {
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }
            }
        }
    }
    else
    {
        if ((new_object = virtual_machine_object_clone(vm, object_src)) == NULL)
        { goto fail; }
    }
    ret = 0;
    *object_dst = new_object;
    goto done;
fail:
    new_object = NULL;
done:
    return ret;
}


/* Loading queue */

struct virtual_machine_ir_loading_queue_item *icode_loading_queue_item_new(void)
{
    struct virtual_machine_ir_loading_queue_item *new_item;

    new_item = (struct virtual_machine_ir_loading_queue_item *)malloc(sizeof(struct virtual_machine_ir_loading_queue_item));
    if (new_item == NULL) return NULL;
    new_item->loaded = 0;
    new_item->module_name = NULL;
    new_item->module_name_len = 0;
    new_item->pathname = NULL;
    new_item->pathname_len = 0;
    new_item->next = NULL;

    return new_item;
}

int icode_loading_queue_item_destroy(struct virtual_machine_ir_loading_queue_item *item)
{
    if (item == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (item->pathname != NULL) free(item->pathname);
    if (item->module_name != NULL) free(item->module_name);
    free(item);

    return 0;
}

struct virtual_machine_ir_loading_queue_item *icode_loading_queue_item_new_with_configure( \
        char *module_name, size_t module_name_len, char *pathname, size_t pathname_len)
{
    struct virtual_machine_ir_loading_queue_item *new_item = NULL;

    if (module_name == NULL) return NULL;

    new_item = icode_loading_queue_item_new();
    if (new_item == NULL) return NULL;
    new_item->module_name = NULL;
    new_item->pathname = NULL;

    if (module_name != NULL)
    {
        new_item->module_name = (char *)malloc( \
                sizeof(char) * (module_name_len + 1));
        if (new_item->module_name == NULL) { goto fail; }
        new_item->module_name_len = module_name_len;
        memcpy(new_item->module_name, module_name, module_name_len);
        new_item->module_name[module_name_len] = '\0';
    }

    if (pathname != NULL)
    {
        new_item->pathname = (char *)malloc( \
                sizeof(char) * (pathname_len + 1));
        if (new_item->pathname == NULL) { goto fail; }
        new_item->pathname_len = pathname_len;
        memcpy(new_item->pathname, pathname, pathname_len);
        new_item->pathname[pathname_len] = '\0';
    }

    return new_item;
fail:
    if (new_item != NULL) 
    {
        icode_loading_queue_item_destroy(new_item);
    }
    return NULL;
}


struct virtual_machine_ir_loading_queue *virtual_machine_ir_loading_queue_new(void)
{
    struct virtual_machine_ir_loading_queue *new_queue;
    new_queue = (struct virtual_machine_ir_loading_queue *)malloc(sizeof(struct virtual_machine_ir_loading_queue));
    if (new_queue == NULL) return NULL;

    new_queue->begin = new_queue->end = NULL;
    new_queue->size = 0;

    return new_queue;
}

int virtual_machine_ir_loading_queue_destroy(struct virtual_machine_ir_loading_queue *queue)
{
    struct virtual_machine_ir_loading_queue_item *item_cur, *item_next;

    if (queue == NULL) return -MULTIPLE_ERR_NULL_PTR;

    item_cur = queue->begin;
    while (item_cur != NULL)
    {
        item_next = item_cur->next;
        icode_loading_queue_item_destroy(item_cur);
        item_cur = item_next;
    }
    free(queue);

    return 0;
}

int icode_loading_queue_is_exists(struct virtual_machine_ir_loading_queue *queue, char *module_name, size_t module_name_len)
{
    struct virtual_machine_ir_loading_queue_item *item_cur;

    if (queue == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (module_name == NULL) return -MULTIPLE_ERR_NULL_PTR;

    item_cur = queue->begin;
    while (item_cur != NULL)
    {
        if ((item_cur->module_name_len == module_name_len) && 
                strncmp(item_cur->module_name, module_name, module_name_len) == 0)
        {
            /* Exists, not needed to append anymore */
            return 1;
        }
        item_cur = item_cur->next;
    }

    return 0;
}

int virtual_machine_ir_loading_queue_is_unloaded(struct virtual_machine_ir_loading_queue_item **queue_item_out, struct virtual_machine_ir_loading_queue *queue)
{
    struct virtual_machine_ir_loading_queue_item *item_cur;

    if (queue == NULL) return -MULTIPLE_ERR_NULL_PTR;

    item_cur = queue->begin;
    while (item_cur != NULL)
    {
        if (item_cur->loaded == 0)
        {
            *queue_item_out = item_cur;
            return 1;
        }
        item_cur = item_cur->next;
    }
    *queue_item_out = NULL;

    return 0;
}
int virtual_machine_ir_loading_queue_append(struct virtual_machine_ir_loading_queue *queue, \
        char *module_name, size_t module_name_len, char *pathname, size_t pathname_len, int loaded)
{
    int ret = 0;
    struct virtual_machine_ir_loading_queue_item *new_item = NULL;

    if (queue == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (module_name == NULL) return -MULTIPLE_ERR_NULL_PTR;

    ret = icode_loading_queue_is_exists(queue, module_name, module_name_len);
    if (ret == 1) 
    {
        /* Exists, not needed to append */
        return 0;
    }
    else if (ret < 0)
    {
        /* Error */
        return ret;
    }

    /* Not exists*/

    /* Create a new queue item */
    new_item = icode_loading_queue_item_new_with_configure(module_name, module_name_len, pathname, pathname_len);
    if (new_item == NULL) return -MULTIPLE_ERR_MALLOC;
    new_item->loaded = loaded;

    /* Append */
    if (queue->begin == NULL)
    {
        queue->begin = queue->end = new_item;
    }
    else 
    {
        queue->end->next = new_item;
        queue->end = new_item;
    }
    queue->size += 1;

    return 0;
}

struct virtual_machine_data_type *virtual_machine_data_type_new(
        struct virtual_machine *vm, 
        const char *name, const size_t len)
{
    struct virtual_machine_data_type *new_data_type = NULL;

    new_data_type = (struct virtual_machine_data_type *)malloc( \
            sizeof(struct virtual_machine_data_type));
    if (new_data_type == NULL) goto fail;
    new_data_type->name = NULL;
    new_data_type->name_len = 0;
    new_data_type->methods = NULL;
    new_data_type->native = 0;
    new_data_type->next = NULL;

    new_data_type->func_clone = NULL;
    new_data_type->func_print = NULL;
    new_data_type->func_destroy = NULL;
    new_data_type->func_eq = NULL;

    new_data_type->name = (char *)malloc( \
            sizeof(char) * (len + 1));
    if (new_data_type->name == NULL) goto fail;
    memcpy(new_data_type->name, name, len);
    new_data_type->name[len] = '\0';
    new_data_type->name_len = len;

    new_data_type->methods = virtual_machine_data_type_method_list_new(vm);
    if (new_data_type->methods == NULL) goto fail;

    return new_data_type;
fail:
    if (new_data_type != NULL) 
    {
        if (new_data_type->name != NULL) free(new_data_type->name);
        if (new_data_type->methods != NULL) virtual_machine_data_type_method_list_destroy(vm, new_data_type->methods);
        free(new_data_type);
    }

    return NULL;
}

int virtual_machine_data_type_set_native( \
        struct virtual_machine_data_type *data_type, int native)
{
    if (data_type == NULL) return -MULTIPLE_ERR_NULL_PTR;

    data_type->native = native;

    return 0;
}

int virtual_machine_data_type_destroy(
        struct virtual_machine *vm, 
        struct virtual_machine_data_type *data_type)
{
    if (data_type == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (data_type->methods != NULL) virtual_machine_data_type_method_list_destroy(vm, data_type->methods);
    if (data_type->name != NULL) free(data_type->name);

    free(data_type);

    return 0;
}

struct virtual_machine_data_type_list *virtual_machine_data_type_list_new(struct virtual_machine *vm)
{
    struct virtual_machine_data_type_list *new_list = NULL;

    (void)vm;

    new_list = (struct virtual_machine_data_type_list *)malloc( \
            sizeof(struct virtual_machine_data_type_list));
    if (new_list == NULL) 
    {
        return NULL;
    }
    new_list->begin = new_list->end = NULL;
    new_list->size = 0;

    return new_list;
}

int virtual_machine_data_type_list_destroy(struct virtual_machine *vm, struct virtual_machine_data_type_list *list)
{
    struct virtual_machine_data_type *data_type_cur, *data_type_next;

    if (list == NULL) return -MULTIPLE_ERR_NULL_PTR;

    data_type_cur = list->begin;
    while (data_type_cur != NULL)
    {
        data_type_next = data_type_cur->next;
        virtual_machine_data_type_destroy(vm, data_type_cur);
        data_type_cur = data_type_next;
    }
    free(list);

    return 0;
}

int virtual_machine_data_type_list_append(struct virtual_machine_data_type_list *list, struct virtual_machine_data_type *new_data_type)
{
    if ((list == NULL) || (new_data_type == NULL)) return -MULTIPLE_ERR_MALLOC;

    if (list->begin == NULL)
    {
        list->begin = list->end = new_data_type;
    }
    else
    {
        list->end->next = new_data_type;
        list->end = new_data_type;
    }
    list->size += 1;

    return 0;
}

int virtual_machine_data_type_list_append_with_configure(struct virtual_machine *vm, struct virtual_machine_data_type_list *list, \
        const char *name, const size_t len)
{
    int ret = 0;
    struct virtual_machine_data_type *new_data_type = NULL;

    new_data_type = virtual_machine_data_type_new(vm, name, len);
    if (new_data_type == NULL) { goto fail; }
    if ((ret = virtual_machine_data_type_list_append(list, new_data_type)) != 0)
    { goto fail; }
    new_data_type = NULL;

    goto done;
fail:
    if (new_data_type != NULL) virtual_machine_data_type_destroy(vm, new_data_type);
done:
    return ret;
}


struct virtual_machine_data_type_method *virtual_machine_data_type_method_new(\
        struct virtual_machine *vm, \
        const char *method_name, const size_t method_name_len, \
        const char *def_name, const size_t def_name_len, const uint32_t def_name_module_id, const uint32_t def_name_data_id, \
        const char *module_name, const size_t module_name_len, const uint32_t module_name_module_id, const uint32_t module_name_data_id, \
        int method_type)
{
    struct virtual_machine_data_type_method *new_data_type_method = NULL;

    (void)vm;

    new_data_type_method = (struct virtual_machine_data_type_method *)malloc( \
            sizeof(struct virtual_machine_data_type_method));
    if (new_data_type_method == NULL) return NULL;

    new_data_type_method->method_name = NULL;
    new_data_type_method->def_name = NULL;
    new_data_type_method->module_name = NULL;
    new_data_type_method->method_type = method_type;
    new_data_type_method->next = NULL;

    /* Method Name */
    new_data_type_method->method_name = (char *)malloc( \
            sizeof(char) * (method_name_len + 1));
    if (new_data_type_method->method_name == NULL)
    { goto fail; }
    memcpy(new_data_type_method->method_name, method_name, method_name_len);
    new_data_type_method->method_name[method_name_len] = '\0';
    new_data_type_method->method_name_len = method_name_len;

    /* Subroutine Name */
    new_data_type_method->def_name = (char *)malloc( \
            sizeof(char) * (def_name_len + 1));
    if (new_data_type_method->def_name == NULL)
    { goto fail; }
    memcpy(new_data_type_method->def_name, def_name, def_name_len);
    new_data_type_method->def_name[def_name_len] = '\0';
    new_data_type_method->def_name_len = def_name_len;
    new_data_type_method->def_name_module_id = def_name_module_id;
    new_data_type_method->def_name_data_id = def_name_data_id;

    /* Method Name */
    new_data_type_method->module_name = (char *)malloc( \
            sizeof(char) * (module_name_len + 1));
    if (new_data_type_method->module_name == NULL)
    { goto fail; }
    memcpy(new_data_type_method->module_name, module_name, module_name_len);
    new_data_type_method->module_name[module_name_len] = '\0';
    new_data_type_method->module_name_len = module_name_len;
    new_data_type_method->module_name_module_id = module_name_module_id;
    new_data_type_method->module_name_data_id = module_name_data_id;

    goto done;
fail:
    if (new_data_type_method != NULL)
    {
        if (new_data_type_method->module_name != NULL) free(new_data_type_method->module_name);
        if (new_data_type_method->def_name != NULL) free(new_data_type_method->def_name);
        if (new_data_type_method->method_name != NULL) free(new_data_type_method->method_name);
        free(new_data_type_method);
        new_data_type_method = NULL;
    }
done:
    return new_data_type_method;
}

int virtual_machine_data_type_method_destroy(struct virtual_machine *vm, struct virtual_machine_data_type_method *method)
{

    (void)vm;

    if (method == NULL) return -MULTIPLE_ERR_MALLOC;

    if (method->module_name != NULL) free(method->module_name);
    if (method->def_name != NULL) free(method->def_name);
    if (method->method_name != NULL) free(method->method_name);
    free(method);
    return 0;
}

struct virtual_machine_data_type_method_list *virtual_machine_data_type_method_list_new(struct virtual_machine *vm)
{
    struct virtual_machine_data_type_method_list *new_list = NULL;

    (void)vm;

    new_list = (struct virtual_machine_data_type_method_list *)malloc( \
            sizeof(struct virtual_machine_data_type_method_list));
    if (new_list == NULL) goto fail;
    new_list->begin = new_list->end = NULL;
    new_list->size = 0;

    /* Special Method */
    new_list->constructor = NULL;
    new_list->destructor = NULL;

    goto done;
fail:
    if (new_list != NULL)
    {
        free(new_list);
        new_list = NULL;
    }
done:
    return new_list;
}

int virtual_machine_data_type_method_list_destroy(struct virtual_machine *vm, struct virtual_machine_data_type_method_list *list)
{
    struct virtual_machine_data_type_method *method_cur, *method_next;

    if (list == NULL) return -MULTIPLE_ERR_NULL_PTR;

    method_cur = list->begin;
    while (method_cur != NULL)
    {
        method_next = method_cur->next;
        virtual_machine_data_type_method_destroy(vm, method_cur);
        method_cur = method_next;
    }
    free(list);

    return 0;
}

int virtual_machine_data_type_method_list_append( \
        struct virtual_machine_data_type_method_list *list, \
        struct virtual_machine_data_type_method *new_method)
{
    if ((list == NULL) || (new_method == NULL)) return -MULTIPLE_ERR_NULL_PTR;

    if (list->begin == NULL)
    {
        list->begin = list->end = new_method;
    }
    else
    {
        list->end->next = new_method;
        list->end = new_method;
    }
    switch (new_method->method_type)
    {
        case VIRTUAL_MACHINE_DATA_TYPE_METHOD_TYPE_NORMAL:
            break;
        case VIRTUAL_MACHINE_DATA_TYPE_METHOD_TYPE_CONSTRUCTOR:
            list->constructor = new_method;
            break;
        case VIRTUAL_MACHINE_DATA_TYPE_METHOD_TYPE_DESTRUCTOR:
            list->destructor = new_method;
            break;
    }
    list->size += 1;

    return 0;
}

int virtual_machine_data_type_method_list_lookup( \
        struct virtual_machine_data_type_method_list *list, \
        struct virtual_machine_data_type_method **method_out, const char *method_name, const size_t method_name_len)
{
    struct virtual_machine_data_type_method *method_cur;

    *method_out = NULL;
    method_cur = list->begin;
    while (method_cur != NULL)
    {
        if ((method_cur->method_name_len == method_name_len) && \
                (strncmp(method_cur->method_name, method_name, method_name_len) == 0))
        {
            *method_out = method_cur;
            return LOOKUP_FOUND;
        }
        method_cur = method_cur->next; 
    }
    return LOOKUP_NOT_FOUND;
}


int virtual_machine_data_type_list_lookup( \
        struct virtual_machine_data_type **data_type_out, \
        struct virtual_machine_data_type_list *data_type_list, const char *name, const size_t name_len)
{
    struct virtual_machine_data_type *data_type_cur = NULL;

    *data_type_out = NULL;

    data_type_cur = data_type_list->begin;
    while (data_type_cur != NULL)
    {
        if ((data_type_cur->name_len == name_len) && \
                (strncmp(data_type_cur->name, name, name_len) == 0))
        {
            *data_type_out = data_type_cur;
            return LOOKUP_FOUND;
        }
        data_type_cur = data_type_cur->next;
    }
    return LOOKUP_NOT_FOUND;
}

int virtual_machine_data_type_list_method_add_with_configure(struct virtual_machine *vm, \
        struct virtual_machine_data_type_list *list, \
        const char *method_name, const size_t method_name_len, \
        const char *data_type_name, const size_t data_type_len, \
        const char *def_name, const size_t def_name_len, const uint32_t def_name_module_id, const uint32_t def_name_data_id, \
        const char *module_name, const size_t module_name_len, const uint32_t module_name_module_id, const uint32_t module_name_data_id, \
        int method_type)
{
    int ret = 0;
    struct virtual_machine_data_type *data_type_target;
    struct virtual_machine_data_type_method *new_method = NULL;

    if (virtual_machine_data_type_list_lookup(&data_type_target, list, data_type_name, data_type_len) != LOOKUP_FOUND)
    {
        /* Not found ? */
        vm_err_update(vm->r, -VM_ERR_DATA_TYPE, \
                "runtime error: data type \'%s\' is undefined", data_type_name);
        return -MULTIPLE_ERR_VM;
        goto fail;
    }
    if ((new_method = virtual_machine_data_type_method_new(vm, method_name, method_name_len, \
                    def_name, def_name_len, def_name_module_id, def_name_data_id, \
                    module_name, module_name_len, module_name_module_id, module_name_data_id, \
                    method_type)) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    if ((ret = virtual_machine_data_type_method_list_append(data_type_target->methods, new_method)) != 0)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    new_method = NULL;
fail:
    if (new_method != NULL) virtual_machine_data_type_method_destroy(vm, new_method);
    return ret;
}

