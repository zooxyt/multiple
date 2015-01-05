/* Virtual Machine CPU : Control : Continuation
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

#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "multiple_err.h"

#include "vm_opcode.h"
#include "vm_types.h"
#include "vm_infrastructure.h"
#include "vm_env.h"
#include "vm_object.h"
#include "vm_object_aio.h"
#include "vm_err.h"
#include "vm_cpu_control.h"
#include "vm_cpu_control_cont.h"


static int virtual_machine_thread_step_control_crush( \
        struct virtual_machine *vm, \
        struct virtual_machine_running_stack *running_stack, \
        size_t preserve_running_stack_size)
{
    int ret = 0;
    struct virtual_machine_running_stack_frame *dest_frame, *prev_frame, *next_frame;
    struct virtual_machine_running_stack_frame *current_frame = running_stack->top;
    size_t idx;

    /* First Frame labeled with 0 */
    if (preserve_running_stack_size == 0) { dest_frame = NULL; }
    else
    {
        dest_frame = running_stack->bottom;
        /* Locate to the lower bound */
        for (idx = 1; idx < preserve_running_stack_size; idx++)
        {
            dest_frame = dest_frame->next;
        }
    }

    while ((current_frame != NULL) && (current_frame != dest_frame))
    {
        prev_frame = current_frame->prev; 
        next_frame = current_frame->next;
        if (prev_frame != NULL) prev_frame->next = next_frame; 
        if (next_frame != NULL) next_frame->prev = prev_frame; 
        running_stack->top = prev_frame;

        if ((ret = virtual_machine_running_stack_frame_destroy(vm, current_frame)) != 0)
        { goto fail; }

        running_stack->size -= 1;
        current_frame = prev_frame; 
    }

    if (preserve_running_stack_size == 0) { running_stack->bottom = NULL; }

fail:
    return ret;
}

static int virtual_machine_thread_step_control_patch_trap_to_environment( \
        struct virtual_machine *vm, \
        struct virtual_machine_environment_stack *environment_stack, \
        struct virtual_machine_running_stack *running_stack)
{
    struct virtual_machine_environment_stack_frame *environment_stack_frame_cur;
    struct virtual_machine_running_stack_frame *running_stack_frame_cur;

    (void)vm;

    environment_stack_frame_cur = environment_stack->begin;
    running_stack_frame_cur = running_stack->bottom;
    while ((environment_stack_frame_cur != NULL) && (running_stack_frame_cur != NULL))
    {
        environment_stack_frame_cur->trap_pc = running_stack_frame_cur->trap_pc; 
        environment_stack_frame_cur = environment_stack_frame_cur->next; 
        running_stack_frame_cur = running_stack_frame_cur->next; 
    }

    return 0;
}

static int virtual_machine_thread_step_control_copy_overwrite_computing_stack( \
        struct virtual_machine *vm, \
        struct virtual_machine_computing_stack *computing_stack_dst, \
        struct virtual_machine_computing_stack *computing_stack_src)
{
    int ret = 0;
    struct virtual_machine_object *object_cur, *new_object = NULL;

    /* Clear the exist elements on the computing stack */
    if ((ret = virtual_machine_computing_stack_clear(vm, \
                    computing_stack_dst)) != 0)
    { goto fail; }

    /* Copy the computing stack of bottom of running stack frame */
    object_cur = computing_stack_src->bottom;
    while (object_cur != NULL)
    {
        if ((new_object = virtual_machine_object_clone(vm, object_cur)) == NULL)
        { goto fail; }
        ret = virtual_machine_computing_stack_push(computing_stack_dst, new_object);
        if (ret != 0) { goto fail; }
        new_object = NULL;
        object_cur = object_cur->next; 
    }

    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
done:
    return ret;
}

static int virtual_machine_thread_step_control_restore_from_environment_stack( \
        struct virtual_machine *vm, \
        struct virtual_machine_running_stack *running_stack, \
        struct virtual_machine_environment_stack *environment_stack, \
        size_t turning_point)
{
    int ret = 0;
    struct virtual_machine_environment_stack_frame *environment_stack_frame_cur;
    struct virtual_machine_object *new_object = NULL;
    size_t idx;
    struct virtual_machine_running_stack_frame *running_stack_frame_cur;
    struct environment_entrance_kernel_record *new_kernel_record = NULL;
    struct virtual_machine_object_environment_entrance *environment_entrance;
    struct virtual_machine_object_environment_entrance_internal *environment_entrance_internal;
    struct environment_entrance_kernel_record_item *environment_entrance_kernel_record_item_target;
    struct virtual_machine_object *new_object_environment_entrance = NULL;
    struct virtual_machine_computing_stack *computing_stack_dst;
    struct virtual_machine_computing_stack *computing_stack_src;

    /* Bottom stack frame */
    if ((environment_stack == NULL) || \
            (environment_stack->begin == NULL))
    {
        VM_ERR_INTERNAL(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    /* Record the kernels */
    new_kernel_record = record_kernels_from_environment_stack( \
            vm, \
            environment_stack);
    if (new_kernel_record == NULL)
    {
        VM_ERR_INTERNAL(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    /* Walkthough the entire running stack */ 
    running_stack_frame_cur = running_stack->bottom;
    environment_stack_frame_cur = environment_stack->begin;
    for (idx = 0; idx != environment_stack->size; idx++)
    {
        /* Clone the kernel of the entrance */
        environment_entrance = environment_stack_frame_cur->environment_entrance->ptr;
        environment_entrance_internal = environment_entrance->ptr_internal; 
        environment_entrance_kernel_record_item_target = environment_entrance_kernel_record_lookup( \
                new_kernel_record, \
                environment_entrance_internal->entrance);
        if (environment_entrance_kernel_record_item_target == NULL) 
        {
            VM_ERR_INTERNAL(vm->r);
            ret = -MULTIPLE_ERR_VM;
            goto fail; 
        }
        new_object_environment_entrance = virtual_machine_object_environment_entrance_new( \
                vm, \
                environment_entrance_kernel_record_item_target->environment_stack_frame_cloned);
        if (new_object_environment_entrance == NULL)
        {
            VM_ERR_INTERNAL(vm->r);
            ret = -MULTIPLE_ERR_VM;
            goto fail; 
        }
        environment_entrance_kernel_record_item_target->environment_stack_frame_cloned = NULL;

        if (idx < turning_point)
        {
            /* Part 1 */

            /* Restore Computing Stack */
            computing_stack_src =  environment_stack_frame_cur->computing_stack;
            computing_stack_dst = running_stack_frame_cur->computing_stack;
            ret = virtual_machine_thread_step_control_copy_overwrite_computing_stack( \
                    vm, \
                    computing_stack_dst, \
                    computing_stack_src);
            if (ret != 0) { goto fail; }

            /* Restore pc */
            running_stack_frame_cur->pc = environment_stack_frame_cur->pc;

            running_stack_frame_cur = running_stack_frame_cur->next;
        }
        else if (idx == turning_point)
        {
            /* Part 2 */

            /* Restore Computing Stack */
            computing_stack_src = environment_stack_frame_cur->computing_stack;
            computing_stack_dst = running_stack_frame_cur->computing_stack;
            ret = virtual_machine_thread_step_control_copy_overwrite_computing_stack( \
                    vm, \
                    computing_stack_dst, \
                    computing_stack_src);
            if (ret != 0) { goto fail; }

            /* Restore pc */
            if (idx != 0)
            {
                running_stack_frame_cur->pc = environment_stack_frame_cur->pc;
            }

            /* Restore Environment */
            /*virtual_machine_object_destroy(vm, running_stack_frame_cur->environment_entrance);*/
            /*running_stack_frame_cur->environment_entrance = NULL;*/
            /*running_stack_frame_cur->environment_entrance = new_object_environment_entrance;*/
            /*new_object_environment_entrance = NULL;*/

            running_stack_frame_cur = running_stack_frame_cur->next;
        }
        else
        {
            /* Part 3 */

            /* Append new frame */
            if ((ret = virtual_machine_running_stack_push_with_configure(vm, running_stack, \
                            environment_stack_frame_cur->module, \
                            environment_stack_frame_cur->pc, \
                            environment_stack_frame_cur->args_count, \
                            environment_stack_frame_cur->closure, \
                            environment_stack_frame_cur->trap_pc, \
                            environment_stack_frame_cur->trap_enabled, \
                            new_object_environment_entrance, \
                            environment_stack_frame_cur->variables \
                            )) != 0)
            { goto fail; }
        }
        if (new_object_environment_entrance != NULL)
        {
            virtual_machine_object_destroy(vm, new_object_environment_entrance);
            new_object_environment_entrance = NULL;
        }

        environment_stack_frame_cur = environment_stack_frame_cur->next; 
    }

fail:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
    if (new_object_environment_entrance != NULL) virtual_machine_object_destroy(vm, new_object_environment_entrance);
    if (new_kernel_record != NULL) environment_entrance_kernel_record_destroy(vm, new_kernel_record);
    return ret;
}

static int virtual_machine_thread_step_control_set_trap( \
        struct virtual_machine *vm, \
        struct virtual_machine_running_stack *running_stack, \
        struct virtual_machine_environment_stack *environment_stack)
{
    struct virtual_machine_environment_stack_frame *environment_stack_frame_cur;
    struct virtual_machine_running_stack_frame *running_stack_frame_cur;

    (void)vm;

    environment_stack_frame_cur = environment_stack->begin;
    running_stack_frame_cur = running_stack->bottom;
    while (environment_stack_frame_cur->next != NULL)
    {
        environment_stack_frame_cur = environment_stack_frame_cur->next; 
        running_stack_frame_cur = running_stack_frame_cur->next; 
    }
    running_stack_frame_cur->trap_enabled = 1;

    return 0;
}

static int virtual_machine_thread_step_control_transport_to_prepush_arguments( \
        struct virtual_machine *vm, \
        struct virtual_machine_computing_stack *computing_stack_dst, \
        struct virtual_machine_computing_stack *computing_stack_src, \
        size_t count, \
        struct virtual_machine_running_stack_frame *target_frame)
{
    int ret = 0;
    struct virtual_machine_object *object_src = computing_stack_src->top;
    struct virtual_machine_object *object_src_solved = NULL;

    while (count-- != 0)
    {

        if ((ret = virtual_machine_variable_solve(&object_src_solved, object_src, target_frame, 1, vm)) != 0)
        { goto fail; }

        ret = virtual_machine_computing_stack_push(computing_stack_dst, object_src_solved);
        if (ret != 0) { goto fail; }
        object_src_solved = NULL;

        /* Locate to the target */
        object_src = object_src->prev;
    }

    goto done;
fail:
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved); 
done:
    return ret;
}

int virtual_machine_thread_step_control_cont(struct virtual_machine *vm)
{
    int ret = 0;
    uint32_t opcode;
    struct virtual_machine_thread *current_thread = vm->tp;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_computing_stack *current_computing_stack;
    struct virtual_machine_object *object_arg = NULL;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_func *object_function = NULL;
    struct virtual_machine_object_func_internal *object_function_internal = NULL;
    struct virtual_machine_object_int *object_args_count = NULL;
    struct virtual_machine_environment_stack *environment = NULL;
    struct virtual_machine_object *new_object_environment = NULL;

    struct virtual_machine_computing_stack *temp_computing_stack = NULL;

    uint32_t function_instrument_number = 0; /* for invoking function */
    size_t args_count;
    struct virtual_machine_module *module_cur = NULL;
    size_t turning_point = 0;
    size_t preserve_running_stack_size = 0;

    current_running_stack = current_thread->running_stack;
    current_frame = current_running_stack->top;
    current_computing_stack = current_frame->computing_stack;

    opcode = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].opcode;
    switch (opcode)
    {
        case OP_CALL:
        case OP_CALLC:
        case OP_TAILCALL:
        case OP_TAILCALLC:

            /* At least the function and arguments count */
            if (current_computing_stack->size < 2)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if (virtual_machine_object_func_type(current_computing_stack->top) != \
                    VIRTUAL_MACHINE_OBJECT_FUNC_TYPE_CONT)
            {
                VM_ERR_INTERNAL(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Extract Object */
            object_function = current_computing_stack->top->ptr;
            object_function_internal = object_function->ptr_internal; 

            /* Internal Function */
            module_cur = object_function_internal->module;
            function_instrument_number = object_function_internal->pc;

            /* Link environment */
            environment = virtual_machine_object_func_extract_environment(current_computing_stack->top);
            if (environment == NULL)
            {
                /* No environment ? */
                VM_ERR_INTERNAL(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Arguments Count */
            object_args_count = current_computing_stack->top->prev->ptr;
            if (object_args_count->value < 0)
            {
                VM_ERR_INTERNAL(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            args_count = (size_t)(object_args_count->value);

            /* Pop the function name and arguments count */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack); /* Function */
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack); /* Argument Number */
            if (ret != 0) { goto fail; }

            /* Update PC */
            current_thread->running_stack->top->pc++;

            /* Create a temp computing for contain pre-pushed arguments */
            if ((temp_computing_stack = virtual_machine_computing_stack_new(vm)) == NULL)
            { goto fail; }

            /* Prepare pre-pushed arguments */
            if ((ret = virtual_machine_thread_step_control_transport_to_prepush_arguments(
                            vm, \
                            temp_computing_stack, \
                            current_running_stack->top->computing_stack, \
                            args_count, \
                            current_running_stack->top)) != 0)
            { goto fail; }

            /* Before Crushing the frames, extract traps to environment */
            if ((ret = virtual_machine_thread_step_control_patch_trap_to_environment(vm, \
                            environment, \
                            current_running_stack)) != 0)
            { goto fail; }

            /* What is the real meaning of the value of preserve_running_stack_size? */
            preserve_running_stack_size = object_function_internal->turning_point;

            /* Environment less than current stack size is possible */
            if (environment->size < preserve_running_stack_size) preserve_running_stack_size = environment->size;

            /* Convert 'size' to idx */
            turning_point = preserve_running_stack_size;
            turning_point -= 1;

            /* preserve running stack size turns to be the turning point,
             * and turning point is the lowest runnning stack size has ever
             * been since the cont been created. 
             * So, the running stack has been divided into 3 parts
             *
             * (Note: each running stack frame's been labeled with a non-negative 
             * integer number x, which has range from 0 (lowest) to size-1 (highest) )
             *
             * Part 1 (x < turning point): 
             * The environments should be preserved.
             * Restore computing stack on the running stack.
             * Restore pc
             *
             * Part 2 (x == turning point):
             * Do the same things as in part 1
             *
             * Part 3 (x > turning point)
             * The environments should be destructed.
             * */

            /*printf("<turning_point = %u, ", (unsigned int)turning_point);*/
            /*printf("preserve_running_stack_size = %u, ", (unsigned int)preserve_running_stack_size);*/
            /*printf("current_running_stack size = %u, ", (unsigned int)current_running_stack->size);*/
            /*printf("environment size = %u>\n", (unsigned int)environment->size);*/

            /* Remove the stack frame from in range of [0, preserve_running_stack_size) */
            if ((ret = virtual_machine_thread_step_control_crush(vm, \
                            current_running_stack, \
                            preserve_running_stack_size)) != 0)
            { goto fail; }

            /* Restore the the whole running stack */
            if ((virtual_machine_thread_step_control_restore_from_environment_stack(vm, \
                            current_running_stack, \
                            environment, \
                            turning_point)) != 0)
            { goto fail; }

            /* Set trap */
            if ((virtual_machine_thread_step_control_set_trap(vm, \
                            current_running_stack, \
                            environment)) != 0)
            { goto fail; }

            /* Create a new frame with pc */
            if ((ret = virtual_machine_running_stack_push_with_configure(vm, current_running_stack, \
                            module_cur, function_instrument_number, \
                            args_count, ((opcode == OP_CALLC) || (opcode == OP_TAILCALLC)) ? 1 : 0, \
                            0,    /* Trap PC */ \
                            0,    /* Trap Enabled */ \
                            NULL, /* Environment Entrance */ \
                            NULL  /* Variables */ \
                            )) != 0)
            { goto fail; }
            environment = NULL;

            if (temp_computing_stack != NULL)
            {
                /* Save arguments to pre-pushed arguments of the new created stack frame */
                if ((ret = virtual_machine_computing_stack_transport(vm, \
                                current_running_stack->top->arguments, \
                                temp_computing_stack, \
                                args_count)) != 0)
                { goto fail; }

                if (temp_computing_stack != NULL) virtual_machine_computing_stack_destroy(vm, temp_computing_stack);
                temp_computing_stack = NULL;
            }

            switch (opcode)
            {
                case OP_TAILCALL:
                case OP_TAILCALLC:
                    if ((ret = virtual_machine_running_stack_lift_merge(vm, current_running_stack)) != 0)
                    { goto fail; }
                    break;
            }
            break;

        default:
            VM_ERR_INTERNAL(vm->r);
            ret = -MULTIPLE_ERR_VM;
            goto fail;
    }
    ret = 0;
    goto done;
fail:
done:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
    if (object_arg != NULL) virtual_machine_object_destroy(vm, object_arg);
    if (temp_computing_stack != NULL) virtual_machine_computing_stack_destroy(vm, temp_computing_stack);
    if (new_object_environment != NULL) virtual_machine_object_destroy(vm, new_object_environment);
    return ret;
}

