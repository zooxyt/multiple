/* Virtual Machine CPU : Core
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

#include <stdlib.h>
#include <string.h>

#include "multiple_err.h"
#include "multiple_tunnel.h"

#include "vm_opcode.h"
#include "vm_types.h"
#include "vm_object.h"
#include "vm_object_aio.h"
#include "vm_infrastructure.h"
#include "vm_opcode.h"

#include "vm_cpu.h"
#include "vm_cpu_fastlib.h"
#include "vm_cpu_alu.h"
#include "vm_cpu_control.h"
#include "vm_cpu_thread.h"
#include "vm_cpu_composite_ds.h"
#include "vm_cpu_dlcall.h"
#include "vm_cpu_func.h"
#include "vm_cpu_int.h"
#include "vm_cpu_class.h"

#include "vm_err.h"


static int virtual_machine_locate_target_frame(struct multiple_error *err, \
        struct virtual_machine *vm, \
        struct virtual_machine_running_stack_frame **target_frame_out, \
        struct virtual_machine_computing_stack **target_computing_stack_out)
{
    int ret = 0;
    uint32_t opcode;
    struct virtual_machine_thread *thread;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_running_stack_frame *previous_frame;

    (void)err;

	thread = vm->tp;
    current_running_stack = thread->running_stack;
    current_frame = current_running_stack->top;
    if (thread->running_stack->top->prev != NULL) previous_frame = current_frame->prev;
    else previous_frame = NULL;

    opcode = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].opcode;
    switch (opcode)
    {
        case OP_ARGCS:
        case OP_LSTARGCS:
        case OP_LSTRARGCS:
        case OP_ARG: 
        case OP_LSTARG: 
        case OP_LSTRARG: 
        case OP_ARGC: 
        case OP_ARGCL: 
        case OP_LSTARGC: 
        case OP_LSTRARGC: 
            if (thread->running_stack->top->arguments->size != 0)
            {
                *target_frame_out = NULL;
                *target_computing_stack_out = thread->running_stack->top->arguments;
            }
            else
            {
                if (previous_frame == NULL)
                {
                    VM_ERR_INTERNAL(vm->r);
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }
                *target_frame_out = previous_frame; 
                *target_computing_stack_out =  previous_frame->computing_stack;
            }
            break;
        case OP_POP: 
        case OP_POPC: 
        case OP_POPCL: 
        case OP_POPCX: 
        case OP_POPM: 
        case OP_POPG: 
            *target_frame_out = current_frame; 
            *target_computing_stack_out = current_frame->computing_stack;
            break;
        default: 
            VM_ERR_INTERNAL(vm->r);
            ret = -MULTIPLE_ERR_VM;
            goto fail;
    }

    goto done;
fail:
done:
    return ret;
}

static int virtual_machine_merge_composite_arguments(struct multiple_error *err, \
        struct virtual_machine *vm, \
        struct virtual_machine_running_stack_frame *target_frame, \
        struct virtual_machine_computing_stack *target_computing_stack)
{
    int ret = 0;
    uint32_t opcode;
    struct virtual_machine_thread *thread;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_object *new_object = NULL;

    (void)err;

	thread = vm->tp;
    current_running_stack = thread->running_stack;
    current_frame = current_running_stack->top;

    opcode = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].opcode;
    switch (opcode)
    {

        case OP_LSTARGCS: 
        case OP_LSTRARGCS: 
        case OP_LSTARG: 
        case OP_LSTRARG: 
        case OP_LSTARGC: 
        case OP_LSTRARGC: 

            if (target_computing_stack->size < current_frame->args_count)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            switch (opcode)
            {
                case OP_LSTARGCS:
                case OP_LSTARG:
                case OP_LSTARGC:
                    /* Make list with remain arguments */
                    if ((ret = virtual_machine_object_list_make(vm, \
                                    &new_object, \
                                    target_computing_stack->top, \
                                    current_frame->args_count, VIRTUAL_MACHINE_OBJECT_LIST_MAKE_ORDER_DEFAULT, \
                                    target_frame)) != 0)
                    { goto fail; }
                    break;
                case OP_LSTRARGCS: 
                case OP_LSTRARG: 
                case OP_LSTRARGC: 
                    /* Make list with remain arguments in reverse order */
                    if ((ret = virtual_machine_object_list_make(vm, \
                                    &new_object, \
                                    target_computing_stack->top, \
                                    current_frame->args_count, VIRTUAL_MACHINE_OBJECT_LIST_MAKE_ORDER_REVERSE, \
                                    target_frame)) != 0)
                    { goto fail; }
                    break;
            }

            /* Pop the elements of list */
            while (current_frame->args_count-- != 0)
            {
                ret = virtual_machine_computing_stack_pop(vm, target_computing_stack);
                if (ret != 0) { goto fail; }
            }

            /* Push the result object into computing stack */
            ret = virtual_machine_computing_stack_push(target_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            /* Reset the number of arguments */
            if (target_frame != NULL)
            {
                target_frame->args_count = 1;
            }

            break;

        default:
            break;
    }


    goto done;
fail:
done:
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
    return ret;
}

static struct virtual_machine_data_section_item *virtual_machine_retrive_data_section( \
        struct virtual_machine_data_section *data_section, \
        uint32_t data_id, \
        uint32_t opcode)
{
    struct virtual_machine_data_section_item *data_section_item_operand = NULL;

    switch (opcode)
    {
        case OP_DEF:
        case OP_ARG:
        case OP_LSTARG:
        case OP_LSTRARG:
        case OP_ARGC:
        case OP_ARGCL:
        case OP_LSTARGC:
        case OP_LSTRARGC:
        case OP_PUSH:
        case OP_MSPUSH:
        case OP_POP:
        case OP_POPC:
        case OP_POPCL:
        case OP_POPCX:
        case OP_PUSHM:
        case OP_POPM:
        case OP_PUSHG:
        case OP_POPG:
        case OP_MSPOP:
        case OP_SYMMK:
        case OP_VARP:
        case OP_VARCP:
        case OP_VARCLP:
            data_section_item_operand = data_section->items + (size_t)data_id;
            break;
        default:
            data_section_item_operand = NULL;
            break;
    }

    return data_section_item_operand;
}

int virtual_machine_thread_step(struct multiple_error *err, struct virtual_machine *vm)
{
    int ret = 0;

    struct virtual_machine_thread *thread;
    uint32_t opcode, operand, data_id;

    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_running_stack_frame *target_frame;

    struct virtual_machine_module *current_module;

    struct virtual_machine_computing_stack *current_computing_stack;
    struct virtual_machine_computing_stack *target_computing_stack;

    struct virtual_machine_object_identifier *object_id_domain = NULL;
    struct virtual_machine_object_identifier *object_id_id = NULL;
    struct virtual_machine_data_section_item *data_section_item_operand = NULL;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object *new_object_2 = NULL;
    struct virtual_machine_object *object_solved = NULL;

    struct virtual_machine_variable *var = NULL;
    struct virtual_machine_variable_list *var_list = NULL;
    struct virtual_machine_object *object_environment_entrance;
    struct virtual_machine_object_environment_entrance *environment_entrance;
    struct virtual_machine_object_environment_entrance_internal *environment_entrance_internal;

    char *str_extract = NULL;
    size_t str_extract_len;

    char *instrument_str;
    size_t instrument_len;

    (void)err;

    /* Current Running variable */
    if (vm->tp == NULL)	{ VM_ERR_INTERNAL(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
    thread = vm->tp;
    current_running_stack = thread->running_stack;
    current_frame = current_running_stack->top;
    if (current_frame == NULL) { return 0; }
    current_module = current_frame->module;
    current_computing_stack = current_frame->computing_stack;
    if (current_running_stack == NULL) { return 0; }

    /* PC checking */
    if ((size_t)current_frame->pc >= vm->tp->running_stack->top->module->text_section->size) 
    {
        vm_err_update(vm->r, -VM_ERR_ACCESS_VIOLATION, \
                "runtime error: " 
                "access violation occurred while thread \'%u\' "\
                "executes the code of module \'%s\'", vm->tp->tid, vm->tp->running_stack->top->module->name);
        ret = -MULTIPLE_ERR_VM;
        return ret;
    }

    /* Record step */
    vm->step_in_time_slice += 1;

    /* Fetch instrument */
    opcode = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].opcode;
    operand = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].operand;
    data_id = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].data_id;

    /* Error passing */
    vm->r->opcode = opcode;
    vm->r->operand = operand;
    vm->r->pc = current_frame->pc;
    vm->r->module = current_module;

    /* Checking Computing Stack */
    if (virtual_machine_computing_stack_check(opcode, current_frame->computing_stack->size) != 0)
    {
        vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                "runtime error: computing stack empty");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    /* Retrieve Data section item with operand */
    data_section_item_operand = virtual_machine_retrive_data_section( \
            current_module->data_section, data_id, opcode);

    /* Execute instrument */
    switch (opcode)
    {
        case OP_NOP:
        case OP_DEF:
            /* Nothing needed to do */
            thread->running_stack->top->pc++;
            break;

        case OP_HALT:
            /* Terminate all threads */
            virtual_machine_thread_list_clear(vm, vm->threads);
            vm->tp = NULL;
            break;

        case OP_FASTLIB:
            ret = virtual_machine_thread_step_fastlib(vm);
            if (ret != 0) goto fail;
            break;

        case OP_IEGC:
            virtual_machine_interrupt_gc_enable(vm);
            thread->running_stack->top->pc++;
            break;

        case OP_IDGC:
            virtual_machine_interrupt_gc_disable(vm);
            thread->running_stack->top->pc++;
            break;

        case OP_INT:
            ret = virtual_machine_interrupt(vm);
            if (ret != 0) goto fail;
            break;

        case OP_DOMAIN:

            target_frame = current_frame;

            object_id_domain = target_frame->computing_stack->top->ptr;
            object_id_id = target_frame->computing_stack->top->prev->ptr;

            new_object = virtual_machine_object_identifier_new_with_value(vm, \
                    object_id_id->id, object_id_id->id_len, object_id_id->id_module_id, object_id_id->id_data_id, \
                    object_id_domain->id, object_id_domain->id_len, object_id_domain->id_module_id, object_id_domain->id_data_id);
            if (new_object == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            ret = virtual_machine_computing_stack_pop(vm, target_frame->computing_stack);
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, target_frame->computing_stack);
            if (ret != 0) { goto fail; }

            if ((ret = virtual_machine_computing_stack_push(current_computing_stack, new_object)) != 0)
            { VM_ERR_INTERNAL(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
            new_object = NULL;

            /* Update PC */
            thread->running_stack->top->pc++;
            break;

        case OP_MSCALL:
        case OP_MSRETURN:
        case OP_MSREM:
            { VM_ERR_INTERNAL(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
            break;

        case OP_PUSH:
        case OP_PUSHG:
        case OP_PUSHM:
        case OP_MSPUSH:
        case OP_SYMMK:

            /* Push value into computing stack */
            switch (opcode)
            {
                case OP_PUSH:
                case OP_PUSHG:
                case OP_PUSHM:
                case OP_MSPUSH:
                    if ((new_object = virtual_machine_object_new_from_data_section_item(vm, data_section_item_operand)) == NULL)
                    { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
                    if ((ret = virtual_machine_computing_stack_push(current_computing_stack, new_object)) != 0)
                    { VM_ERR_INTERNAL(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
                    new_object = NULL;
                    break;
                case OP_SYMMK:
                    if ((new_object = virtual_machine_object_new_symbol_from_data_section_item(vm, data_section_item_operand)) == NULL)
                    { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
                    if ((ret = virtual_machine_computing_stack_push(current_computing_stack, new_object)) != 0)
                    { VM_ERR_INTERNAL(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
                    new_object = NULL;
                    break;
            }

            /* Update PC */
            thread->running_stack->top->pc++;
            break;


        case OP_ARGP:
            if ((new_object = virtual_machine_object_bool_new_with_value(vm, 
                            ((thread->running_stack->top->arguments->size != 0) || \
                             ((current_running_stack->top != NULL) && \
                              (current_running_stack->top->prev != NULL) && \
                              (current_running_stack->top->prev->computing_stack->size != 0))) ? 
                            VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE :
                            VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_FALSE)) == NULL)
            { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
            /* Push the result object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            /* Update PC */
            thread->running_stack->top->pc++;
            break;

        case OP_ARGCS:
        case OP_LSTARGCS:
        case OP_LSTRARGCS:
        case OP_ARG:
        case OP_LSTARG:
        case OP_LSTRARG:
        case OP_ARGC:
        case OP_ARGCL:
        case OP_LSTARGC:
        case OP_LSTRARGC:
        case OP_POP:
        case OP_POPC:
        case OP_POPCL:
        case OP_POPCX:
        case OP_POPM:
        case OP_POPG:

            /* ARG and POP are similar, but:
             * 'pop' gets value from current computing stack,
             * 'arg' gets value from previous computing stack. */
            if ((ret = virtual_machine_locate_target_frame(err, vm, \
                            &target_frame, \
                            &target_computing_stack)) != 0)
            { goto fail; }

            /* Compact Composite Arguments */
            if ((ret = virtual_machine_merge_composite_arguments(err, vm, \
                            target_frame, \
                            target_computing_stack)) != 0)
            { goto fail; }

            /* Decrease the number of arguments */
            current_frame->args_count -=1;
            /* Stack Top element */
            if (target_computing_stack->size == 0)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((opcode == OP_ARGCS)||(opcode == OP_LSTARGCS))
            {
                /* Solving */
                if ((ret = virtual_machine_variable_solve(&new_object, target_computing_stack->top, target_frame, 1, vm)) != 0)
                { goto fail; }
                /* Push on the current computing stack */
                ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
                if (ret != 0) { goto fail; }
                new_object = NULL;
                goto lbl_to_pop;
            }

            if (data_section_item_operand == NULL)
            { VM_ERR_INTERNAL(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }

            /* Operand type */
            if (data_section_item_operand->type == MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NONE)
            {
                /* Directly remove the top element */ 
                goto lbl_to_pop;
            }

            if (data_section_item_operand->type != MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER)
                /*if (!(OBJECT_IS_IDENTIFIER(data_section_item_operand->type)))*/
            {
                vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                        "runtime error: unsupported operand type");
                ret = -MULTIPLE_ERR_VM;
                goto fail; 
            }

            /* Update to Variable List */

            if ((opcode == OP_POP) || (opcode == OP_ARG) || \
                    (opcode == OP_LSTARG) || (opcode == OP_LSTRARG))
            {
                /* Solving */
                if ((ret = virtual_machine_variable_solve(&new_object, target_computing_stack->top, target_frame, 1, vm)) != 0)
                { goto fail; }
                /* Variable exists? */
                if (virtual_machine_variable_list_lookup(&var, current_frame->variables, current_module->id, operand) == LOOKUP_NOT_FOUND)
                { /* Not exists, create directly */ }
                else
                {
                    /* Destroy the variable */
                    if ((ret = virtual_machine_variable_list_remove(vm, current_frame->variables, var)) != 0) { goto fail; }
                }
                /* Append the new variable */
                if ((ret = virtual_machine_variable_list_append_with_configure(vm, \
                                current_frame->variables, current_module->id, operand, \
                                new_object)) != 0)
                { goto fail; }

                goto lbl_to_pop;
            }

            if ((opcode == OP_ARGCL) || (opcode == OP_POPCL))
            {
                /* Lookup on target frame */
                switch (opcode)
                {
                    case OP_ARGCL:
                        if ((ret = virtual_machine_variable_list_lookup_from_environment_entrance( \
                                        vm, 
                                        &var, &var_list, \
                                        current_running_stack->top->prev->environment_entrance, \
                                        current_module->id, operand, \
                                        0 /* 1 frame limit */
                                        )) != 0)
                        { goto fail; }
                        break;
                    case OP_POPCL:
                        if ((ret = virtual_machine_variable_list_lookup_from_environment_entrance( \
                                        vm, 
                                        &var, &var_list, \
                                        current_running_stack->top->environment_entrance, \
                                        current_module->id, operand, \
                                        1 /* 1 frame limit */
                                        )) != 0)
                        { goto fail; }
                        break;
                }
                if (var != NULL)
                {
                    /* Exists, remove it */

                    /* Destroy the variable */
                    if ((ret = virtual_machine_variable_list_remove(vm, var_list, var)) != 0) { goto fail; }
                    /* Solving */
                    if ((ret = virtual_machine_variable_solve(&new_object, target_computing_stack->top, target_frame, 1, vm)) != 0)
                    { goto fail; }
                    /* Append the new variable */
                    if ((ret = virtual_machine_variable_list_append_with_configure(vm, \
                                    var_list, current_module->id, operand, \
                                    new_object)) != 0)
                    { goto fail; }
                }
                else
                {
                    /* Not exists */

                    /* Solving */
                    if ((ret = virtual_machine_variable_solve(&new_object, target_computing_stack->top, target_frame, 1, vm)) != 0)
                    { goto fail; }
                    object_environment_entrance = current_running_stack->top->environment_entrance;
                    if (object_environment_entrance->type != OBJECT_TYPE_ENV_ENT)
                    { VM_ERR_INTERNAL(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
                    environment_entrance = object_environment_entrance->ptr;
                    environment_entrance_internal = environment_entrance->ptr_internal;
                    if (environment_entrance_internal->entrance == NULL)
                    { VM_ERR_INTERNAL(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
                    /* Append the new variable */
                    if ((ret = virtual_machine_variable_list_append_with_configure(vm, \
                                    environment_entrance_internal->entrance->variables, \
                                    current_module->id, operand, \
                                    new_object)) != 0)
                    { goto fail; }
                }

                goto lbl_to_pop;
            }

            switch (opcode)
            {
                case OP_POPC:
                case OP_POPCX:
                case OP_ARGC:
                case OP_LSTARGC:
                case OP_LSTRARGC:
                    if ((ret = virtual_machine_variable_list_lookup_from_environment_entrance( \
                                    vm, 
                                    &var, \
                                    &var_list, \
                                    current_running_stack->top->environment_entrance, \
                                    current_module->id, operand, \
                                    0 /* no frame limit */
                                    )) != 0)
                    { goto fail; }
                    if (var == NULL)
                    {
                        switch (opcode)
                        {
                            case OP_POPC:
                            case OP_ARGC:
                            case OP_LSTARGC:
                            case OP_LSTRARGC:
                                /* Solving */
                                if ((ret = virtual_machine_variable_solve(&new_object, target_computing_stack->top, target_frame, 1, vm)) != 0)
                                { goto fail; }
                                object_environment_entrance = current_running_stack->top->environment_entrance;
                                if (object_environment_entrance->type != OBJECT_TYPE_ENV_ENT)
                                { VM_ERR_INTERNAL(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
                                environment_entrance = object_environment_entrance->ptr;
                                environment_entrance_internal = environment_entrance->ptr_internal;
                                if (environment_entrance_internal->entrance == NULL)
                                { VM_ERR_INTERNAL(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
                                /* Append the new variable */
                                if ((ret = virtual_machine_variable_list_append_with_configure(vm, \
                                                environment_entrance_internal->entrance->variables, \
                                                current_module->id, operand, \
                                                new_object)) != 0)
                                { goto fail; }
                                break;

                            case OP_POPCX:
                                /* Not exists */
                                vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                                        "runtime error: variable \'%s\' not exists", \
                                        (char *)(data_section_item_operand->ptr));
                                ret = -MULTIPLE_ERR_VM;
                                goto fail;
                        }
                    }
                    else
                    {
                        /* Solving */
                        if ((ret = virtual_machine_variable_solve(&new_object, target_computing_stack->top, target_frame, 1, vm)) != 0)
                        { goto fail; }
                        /* Destroy the variable */
                        if ((ret = virtual_machine_variable_list_remove(vm, var_list, var)) != 0) 
                        { goto fail; }
                        /* Append the new variable */
                        if ((ret = virtual_machine_variable_list_append_with_configure(vm, \
                                        var_list, current_module->id, operand, \
                                        new_object)) != 0)
                        { goto fail; }
                    }
                    break;

                case OP_POPM:
                    /* Module Level variable exists? */
                    if (virtual_machine_variable_list_lookup(&var, current_frame->module->variables, current_module->id, operand) == LOOKUP_NOT_FOUND)
                    { /* Not exists, create directly */ }
                    else
                    {
                        /* Destroy the variable */
                        if ((ret = virtual_machine_variable_list_remove(vm, current_frame->module->variables, var)) != 0) { goto fail; }
                    }
                    /* Solving */
                    if ((ret = virtual_machine_variable_solve(&new_object, target_computing_stack->top, target_frame, 1, vm)) != 0)
                    { goto fail; }
                    /* Append the new variable */
                    if ((ret = virtual_machine_variable_list_append_with_configure(vm, \
                                    current_frame->module->variables, current_module->id, operand, \
                                    new_object)) != 0)
                    { goto fail; }
                    break;
                case OP_POPG:
                    /* Global Variable exists? */
                    if (virtual_machine_variable_list_lookup(&var, vm->variables_global, current_module->id, operand) == LOOKUP_NOT_FOUND)
                    { /* Not exists, create directly */ }
                    else
                    {
                        /* Destroy the variable */
                        if ((ret = virtual_machine_variable_list_remove(vm, vm->variables_global, var)) != 0) { goto fail; }
                    }
                    /* Solving */
                    if ((ret = virtual_machine_variable_solve(&new_object, target_computing_stack->top, target_frame, 1, vm)) != 0)
                    { goto fail; }
                    /* Append the new variable */
                    if ((ret = virtual_machine_variable_list_append_with_configure(vm, \
                                    vm->variables_global, current_module->id, operand, \
                                    new_object)) != 0)
                    { goto fail; }
                    break;
            }

            /* Destroy solved object */
            if (new_object != NULL)
            {
                virtual_machine_object_destroy(vm, new_object); new_object = NULL;
            }

lbl_to_pop:

            /* Pop the top element */
            ret = virtual_machine_computing_stack_pop(vm, target_computing_stack);
            if (ret != 0) { goto fail; }
            /* Update PC */
            thread->running_stack->top->pc++;

            break;

        case OP_PRINT:
        case OP_SIZE:
        case OP_TYPE:

            /* Stack Top element */
            if (current_computing_stack->size == 0)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Print the element */
            object_solved = NULL;
            if (current_computing_stack->top->type == OBJECT_TYPE_IDENTIFIER)
            {
                /* Variable */
                if ((ret = virtual_machine_variable_solve(&object_solved, current_computing_stack->top, current_frame, 1, vm)) != 0)
                { goto fail; }

                switch (opcode)
                {
                    case OP_PRINT:
                        ret = virtual_machine_object_print(object_solved);
                        break;
                    case OP_SIZE:
                        ret = virtual_machine_object_size(vm, &new_object, object_solved);
                        break;
                    case OP_TYPE:
                        ret = virtual_machine_object_type(vm, &new_object, object_solved);
                        break;
                }
                if (ret != 0) goto fail;

                virtual_machine_object_destroy(vm, object_solved); object_solved = NULL;
            }
            else
            {
                switch (opcode)
                {
                    case OP_PRINT:
                        ret = virtual_machine_object_print(current_computing_stack->top);
                        break;
                    case OP_SIZE:
                        ret = virtual_machine_object_size(vm, &new_object, current_computing_stack->top);
                        break;
                    case OP_TYPE:
                        ret = virtual_machine_object_type(vm, &new_object, current_computing_stack->top);
                        break;
                }
                if (ret != 0) goto fail;
            }

            /* Pop the top element */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            switch (opcode)
            {
                case OP_PRINT:
                    break;
                case OP_SIZE:
                case OP_TYPE:
                    /* Push the result object into computing stack */
                    ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
                    if (ret != 0) { goto fail; }
                    new_object = NULL;
                    break;
                default:
                    break;
            }

            /* Update PC */
            thread->running_stack->top->pc++;
            break;

        case OP_TYPEUP:

            /* Stack Top element */
            if (current_computing_stack->size < 2)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((ret = virtual_machine_object_type_upgrade( \
                            &new_object, &new_object_2, \
                            current_computing_stack->top->prev, \
                            current_computing_stack->top, \
                            vm)) != 0)
            { goto fail; }

            /* Pop the top 2 elements */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object_2);
            if (ret != 0) { goto fail; }
            new_object_2 = NULL;

            /* Update PC */
            thread->running_stack->top->pc++;
            break;

        case OP_CONVERT:

            /* At least 1 object */
            if (current_computing_stack->size < 1)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Perform the conversion */
            ret = virtual_machine_object_convert(&new_object, current_computing_stack->top, \
                    operand, vm);
            if (ret != 0) goto fail;

            /* Pop the original object */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Push the converted object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            thread->running_stack->top->pc++;
            break;

        case OP_TYPEP:

            /* At least 1 object */
            if (current_computing_stack->size < 1)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((ret = virtual_machine_variable_solve(&object_solved, current_frame->computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }

            if ((new_object = virtual_machine_object_bool_new_with_value(vm, \
                            object_solved->type == operand ? \
                            VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE: \
                            VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_FALSE)) == NULL)
            { goto fail; }
            virtual_machine_object_destroy(vm, object_solved); object_solved = NULL;

            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            thread->running_stack->top->pc++;
            break;

        case OP_STI:
        case OP_ITS:

            /* At least 1 object */
            if (current_computing_stack->size < 1)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            switch (opcode)
            {
                case OP_STI:
                    if (current_computing_stack->top->type != OBJECT_TYPE_STR)
                    {
                        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                                "runtime error: unsupported operand type");
                        ret = -MULTIPLE_ERR_VM;
                        goto fail; 
                    }
                    virtual_machine_object_str_extract(&str_extract, &str_extract_len, current_computing_stack->top);
                    new_object = virtual_machine_object_identifier_new_with_value( \
                            vm, \
                            str_extract, str_extract_len, \
                            current_module->id,
                            0, \
                            NULL, 0, 0, 0);
                    if (new_object == NULL)
                    {
                        VM_ERR_MALLOC(vm->r);
                        ret = -MULTIPLE_ERR_VM;
                        goto fail; 
                    }
                    break;
                case OP_ITS:
                    if (current_computing_stack->top->type != OBJECT_TYPE_IDENTIFIER)
                    {
                        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                                "runtime error: unsupported operand type");
                        ret = -MULTIPLE_ERR_VM;
                        goto fail; 
                    }
                    virtual_machine_object_str_extract(&str_extract, &str_extract_len, current_computing_stack->top);
                    new_object = virtual_machine_object_str_new_with_value(\
                            vm, 
                            str_extract, str_extract_len);
                    if (new_object == NULL)
                    {
                        VM_ERR_MALLOC(vm->r);
                        ret = -MULTIPLE_ERR_VM;
                        goto fail; 
                    }
                    break;
            }

            /* Pop the original object */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Push the converted object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            thread->running_stack->top->pc++;
            break;

        case OP_SLV:
        case OP_TRYSLV:

            /* At least 1 object */
            if (current_computing_stack->size < 1)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if (current_computing_stack->top->type == OBJECT_TYPE_IDENTIFIER)
            {
                object_id_id = current_frame->computing_stack->top->ptr;
                if ((ret = virtual_machine_variable_list_lookup_from_environment_entrance( \
                                vm, 
                                &var, \
                                &var_list, \
                                current_running_stack->top->environment_entrance, \
                                current_module->id, object_id_id->id_data_id, \
                                0 /* no frame limit */
                                )) != 0)
                { goto fail; }
                if (var != NULL)
                {
                    /* In Environment */
                    switch (opcode)
                    {
                        case OP_SLV:
                            new_object = virtual_machine_object_clone(vm, var->ptr);
                            if (new_object == NULL) { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
                            break;
                        case OP_TRYSLV:
                            new_object = virtual_machine_object_bool_new_with_value(vm, \
                                    VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE);
                            if (new_object == NULL) { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
                            break;
                    }
                }
                else
                {
                    /* Normal Variable */
                    if ((ret = virtual_machine_variable_solve(&new_object, current_computing_stack->top, current_frame, 1, vm)) != 0)
                    {
                        /* Can't solve */
                        switch (opcode)
                        {
                            case OP_SLV:
                                break;
                            case OP_TRYSLV:
                                /* Clear the runtime error */
                                vm_err_clear(vm->r);
                                /* Push a false */
                                new_object = virtual_machine_object_bool_new_with_value(vm, \
                                        VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_FALSE);
                                if (new_object == NULL) { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
                                break;
                        }
                    }
                    else
                    {
                        switch (opcode)
                        {
                            case OP_SLV:
                                break;
                            case OP_TRYSLV:
                                virtual_machine_object_destroy(vm, new_object);
                                new_object = virtual_machine_object_bool_new_with_value(vm, \
                                        VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE);
                                if (new_object == NULL) { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
                        }
                    }
                }

                /* Pop the original object */
                ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
                if (ret != 0) { goto fail; }

                /* Push the converted object into computing stack */
                ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
                if (ret != 0) { goto fail; }
                new_object = NULL;
            }
            else
            {
                if (opcode == OP_TRYSLV)
                {
                    new_object = virtual_machine_object_bool_new_with_value(vm, \
                            VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE);
                    if (new_object == NULL) { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
                    ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
                    if (ret != 0) { goto fail; }
                    new_object = NULL;
                }
            }

            thread->running_stack->top->pc++;
            break;

        case OP_FUNCMK:
        case OP_NFUNCMK:
        case OP_LAMBDAMK:
        case OP_PROMMK:
        case OP_CONTMK:
            ret = virtual_machine_thread_step_func(vm);
            if (ret != 0) goto fail;
            break;

        case OP_RETURN:
        case OP_RETNONE:
        case OP_RETURNTO:
        case OP_YIELD:
        case OP_LIFT:
        case OP_CALL: case OP_CALLC:
        case OP_TAILCALL: case OP_TAILCALLC:
        case OP_TRAPSET:
        case OP_TRAP:
        case OP_PROMC:
            ret = virtual_machine_thread_step_control(vm);
            if (ret != 0) goto fail;
            break;

        case OP_DLCALL:
            ret = virtual_machine_thread_step_dlcall(vm);
            if (ret != 0) goto fail;
            break;

        case OP_DUP:
        case OP_DROP:
        case OP_PICK: case OP_PICKCP:
        case OP_INSERT: case OP_INSERTCP:
        case OP_REVERSE: case OP_REVERSEP:
        case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV: case OP_MOD:
        case OP_LSHIFT: case OP_RSHIFT:
        case OP_ANDA: case OP_ORA: case OP_XORA:
        case OP_ANDL: case OP_ORL: case OP_XORL:
        case OP_EQ: case OP_NE: 
        case OP_L: case OP_G: case OP_LE: case OP_GE:
        case OP_NEG:
        case OP_NOTA:
        case OP_NOTL:
        case OP_JMP:
        case OP_JMPC:
        case OP_JMPR:
        case OP_JMPCR:
            ret = virtual_machine_thread_step_alu(vm);
            if (ret != 0) goto fail;
            break;

        case OP_TFK:
        case OP_TWAIT:
        case OP_TEXIT:
        case OP_TCUR:
        case OP_TALIVE:
        case OP_TYIELD:
        case OP_TSENDMSG: case OP_TRECVMSG: case OP_TISEMPTY:
        case OP_TSUSPEND: case OP_TRESUME:
        case OP_MTXMK: case OP_MTXLCK: case OP_MTXUNLCK:
        case OP_SEMMK: case OP_SEMP: case OP_SEMV:
            ret = virtual_machine_thread_step_thread(vm);
            if (ret != 0) goto fail;
            break;


        case OP_LSTMK: case OP_ARRMK: case OP_TUPMK: case OP_HASHMK: case OP_PAIRMK:

        case OP_LSTCAR: case OP_LSTCDR: 
        case OP_ARRCAR: case OP_ARRCDR: 
        case OP_TUPCAR: case OP_TUPCDR:
        case OP_PAIRCAR: case OP_PAIRCDR:
        case OP_HASHCAR: case OP_HASHCDR:

        case OP_LSTUNPACK: case OP_LSTUNPACKR:

        case OP_LSTCDRSET:
        case OP_PAIRCARSET: case OP_PAIRCDRSET:

        case OP_LSTADD:  case OP_LSTADDH:
        case OP_ARRADD:
        case OP_HASHADD: case OP_HASHDEL: case OP_HASHHASKEY:

        case OP_REFGET: case OP_REFSET: 
            ret = virtual_machine_thread_step_composite_ds(vm);
            if (ret != 0) goto fail;
            break;


        case OP_CLSTYPEREG: case OP_CLSINSTMK: case OP_CLSINSTRM: 
        case OP_CLSPGET: case OP_CLSPSET: 
        case OP_CLSMADD: 
        case OP_CLSCTORADD: 
        case OP_CLSDTORADD: 
        case OP_CLSDTOR: 
        case OP_CLSMINVOKE:
            ret = virtual_machine_thread_step_class(vm);
            if (ret != 0) goto fail;
            break;

        default:
            /* Check if the instrument is available */
            if (virtual_machine_opcode_to_instrument(&instrument_str, &instrument_len, opcode) == 0)
            {
                vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPCODE, \
                        "runtime error: unsupported instrument \'%s\'", instrument_str);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            else
            {
                vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPCODE, \
                        "runtime error: unsupported opcode %u", opcode);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            break;
    }

    ret = 0;
    goto done;
fail:
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
    if (new_object_2 != NULL) _virtual_machine_object_destroy(vm, new_object_2);
done:
    return ret;
}

