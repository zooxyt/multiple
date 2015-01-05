/* Virtual Machine CPU : Control
   Copyright(C) 2013 Cheryl Natsu

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

int virtual_machine_thread_step_control(struct virtual_machine *vm)
{
    int ret = 0;
    uint32_t opcode, operand;
    struct virtual_machine_thread *current_thread = vm->tp;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_running_stack_frame *previous_frame;
    struct virtual_machine_running_stack_frame *generator_frame;
    struct virtual_machine_computing_stack *current_computing_stack;
    struct virtual_machine_object *object_arg = NULL;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_func *object_function = NULL;
    struct virtual_machine_object_func_internal *object_function_internal = NULL;

    struct virtual_machine_object *object_environment_entrance = NULL;
    struct virtual_machine_object *new_object_environment_entrance = NULL;

    struct virtual_machine_computing_stack *temp_computing_stack = NULL;

    uint32_t function_instrument_number = 0; /* for invoking function */
    size_t args_count;
    struct virtual_machine_module *module_cur = NULL;

    int (*extern_func)(struct multiple_stub_function_args *args) = NULL;
    struct multiple_stub_function_args *extern_func_args = NULL;
    int func_ret_code;

    current_running_stack = current_thread->running_stack;
    current_frame = current_running_stack->top;
    current_computing_stack = current_frame->computing_stack;
    if (current_thread->running_stack->top->prev != NULL) previous_frame = current_frame->prev;
	else previous_frame = NULL;

	opcode = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].opcode;
	operand = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].operand;
    switch (opcode)
    {
        case OP_RETURNTO:

            if (current_running_stack->top->prev == NULL)
            {
                vm_err_update(vm->r, -MULTIPLE_ERR_VM, \
                        "runtime error: previous running frame stack is unavailiable");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            current_running_stack->top->prev->pc = operand;

            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;

        case OP_RETURN:
        case OP_YIELD:

            /* Push return value to the upper stack frame */
            if (current_frame->computing_stack->size == 0)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            if (current_frame->prev != NULL)
            {
                /* Solve the variable of this variable */
                if ((ret = virtual_machine_variable_solve(&new_object, current_computing_stack->top, current_frame, 1, vm)) != 0)
                { goto fail; }

                /* Pop the current object */
                ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
                if (ret != 0) { goto fail; }
                /* Push the solved object */
                ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
                if (ret != 0) { goto fail; }
                new_object = NULL;

                /* Transport value from current computing stack to previous one */
                if ((ret = virtual_machine_computing_stack_transport( \
                                vm, \
                                current_frame->prev->computing_stack, 
                                current_frame->computing_stack, 1)) != 0)
                {
                    goto fail;
                }
            }
			/* Directly return on the only rest frame */
			if ((opcode == OP_YIELD) && previous_frame == NULL) opcode = OP_RETURN;
			/* Return or yield */
            switch (opcode)
            {
                case OP_RETURN:
                    /* Delete the top running stack frame */
                    if ((ret = virtual_machine_running_stack_pop(vm, current_running_stack)) != 0)
                    { goto fail; }
                    break;
                case OP_YIELD:
                    /* Move the top running stack frame to generator list */
                    current_frame->pc += 1;
                    current_running_stack->top = current_frame->prev;
                    current_frame->next = current_frame->prev = NULL;
                    if ((ret = virtual_machine_running_stack_frame_list_append(previous_frame->generators, current_frame)) != 0)
                    {
                        VM_ERR_INTERNAL(vm->r);
                        ret = -MULTIPLE_ERR_VM;
                        goto fail; 
                    }
                    current_running_stack->size--;
                    previous_frame->next = NULL;
                    break;
                default:
                    break;
            }

            /* Update Continuation List */
            continuation_list_update( \
                    vm, \
                    current_thread->continuations, \
                    current_running_stack->size);

            break;

        case OP_RETNONE:

            if (current_frame->prev != NULL)
            {
                /* Create a none object */
                new_object = virtual_machine_object_none_new(vm);
                if (new_object == NULL)
                {
                    VM_ERR_MALLOC(vm->r);
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }

                /* Push the solved object */
                ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
                if (ret != 0) { goto fail; }
                new_object = NULL;

                /* Transport value from current computing stack to previous one */
                if ((ret = virtual_machine_computing_stack_transport( \
                                vm, \
                                current_frame->prev->computing_stack, 
                                current_frame->computing_stack, 1)) != 0)
                {
                    goto fail;
                }
            }

            /* Delete the top running stack frame */
            if ((ret = virtual_machine_running_stack_pop(vm, current_running_stack)) != 0)
            { goto fail; }

            /* Update Continuation List */
            continuation_list_update( \
                    vm, \
                    current_thread->continuations, \
                    current_running_stack->size);

            break;

        case OP_LIFT:

            /* Enough element on the stack */
            if (current_computing_stack->size < (size_t)operand)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            args_count = (size_t)operand;
            
            while (args_count-- != 0)
            {
                if (current_frame->computing_stack->top->type == OBJECT_TYPE_IDENTIFIER)
                {
                    /* Variable */
                    if ((ret = virtual_machine_variable_solve(&new_object, current_frame->computing_stack->top, current_frame, 1, vm)) != 0)
                    { goto fail; }
					if (previous_frame == NULL) 
                    {
                        VM_ERR_INTERNAL(vm->r);
                        ret = -MULTIPLE_ERR_VM;
                        goto fail;
                    }
                    /* Append the new variable */
					if ((ret = virtual_machine_computing_stack_push(previous_frame->computing_stack, new_object)) != 0)
                    {
                        VM_ERR_INTERNAL(vm->r);
                        ret = -MULTIPLE_ERR_VM;
                        goto fail;
                    }
                    new_object = NULL;
                }
                else
                {
					if (previous_frame == NULL) 
					{
                        VM_ERR_INTERNAL(vm->r);
                        ret = -MULTIPLE_ERR_VM;
                        goto fail;
					}
                    /* Transport value from current computing stack to previous one */
                    if ((ret = virtual_machine_computing_stack_transport(
                                    vm, 
                                    previous_frame->computing_stack, 
                                    current_frame->computing_stack, 1)) != 0)
                    { goto fail; }
                }
                ret = virtual_machine_computing_stack_pop(vm, current_frame->computing_stack);
                if (ret != 0) { goto fail; }
            }

            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;

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

            if (current_computing_stack->top->type != OBJECT_TYPE_FUNCTION)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'function\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Extract Object */
            object_function = current_computing_stack->top->ptr;
            object_function_internal = object_function->ptr_internal; 

            if ((object_function_internal->promise != 0) && \
                    (object_function_internal->cached_promise != NULL))
            {
                /* Pop arguments count and function */
                ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
                if (ret != 0) { goto fail; }
                ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
                if (ret != 0) { goto fail; }

                /* Push the cached answer */
                new_object = virtual_machine_object_clone(vm, object_function_internal->cached_promise);
                if (new_object == NULL)
                {
                    VM_ERR_MALLOC(vm->r);
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }
                if ((ret = virtual_machine_computing_stack_push(current_computing_stack, new_object)) != 0)
                { goto fail; }
                new_object = NULL;

                /* Update PC */
                current_thread->running_stack->top->pc++;

                break;
            }

            if (object_function_internal->cont != 0)
            {
                /* Continuation */
                ret = virtual_machine_thread_step_control_cont(vm);
                if (ret != 0) { goto fail; }
                break;
            }

            if (object_function_internal->extern_func != NULL)
            {
                /* Extern Function */
                extern_func = object_function_internal->extern_func;
                extern_func_args = object_function_internal->extern_func_args;
            }
            else
            {
                /* Internal Function */
                module_cur = object_function_internal->module;
                function_instrument_number = object_function_internal->pc;

                /* Link environment entrance */
                object_environment_entrance = ((struct virtual_machine_object_func *) \
                        (current_computing_stack->top->ptr))->ptr_internal->environment_entrance;
            }

            /* Arguments Count */
            if (current_computing_stack->top->prev->type != OBJECT_TYPE_INT)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'int\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            args_count = (size_t)virtual_machine_object_int_get_primitive_value(current_computing_stack->top->prev);

            /* Pop the function name and arguments count */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack); /* Function */
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack); /* Argument Number */
            if (ret != 0) { goto fail; }

            /* Update PC */
            current_thread->running_stack->top->pc++;

            /* Is the target external? */
            if (extern_func != NULL)
            {
                /* Set Current computing stack for passing argumets */
                extern_func_args->runtime_return_value = 0;
                extern_func_args->frame = current_frame;
                extern_func_args->args_count = args_count;
                extern_func_args->rail = vm->r;
                extern_func_args->vm = vm;
                /* Execute function */
                func_ret_code = (*extern_func)(extern_func_args);
                if (vm_err_occurred(vm->r)) { goto fail; }

                /* Return Value */
                if (extern_func_args->return_value != NULL)
                {
                    if ((ret = virtual_machine_computing_stack_push(current_computing_stack, extern_func_args->return_value)) != 0)
                    { goto fail; }
                    extern_func_args->return_value = NULL;
                }
                else
                {
                    if ((new_object = virtual_machine_object_int_new_with_value(vm, func_ret_code)) == NULL)
                    {
                        VM_ERR_MALLOC(vm->r);
                        ret = -MULTIPLE_ERR_VM;
                        goto fail;
                    }
                    if ((ret = virtual_machine_computing_stack_push(current_computing_stack, new_object)) != 0)
                    { goto fail; }
                    new_object = NULL;
                }
            }
            else
            {
                /* Is the target a generator? */
                generator_frame = virtual_machine_running_stack_frame_get_generator(current_frame, module_cur, function_instrument_number);
                if (generator_frame == NULL)
                {
                    /* Normal Call */

                    new_object_environment_entrance = virtual_machine_object_environment_entrance_make_linked( \
                            vm, \
                            object_environment_entrance);
                    if (new_object_environment_entrance == NULL)
                    {
                        VM_ERR_MALLOC(vm->r);
                        ret = -MULTIPLE_ERR_VM;
                        goto fail;
                    }

                    /* Create a new frame with pc */
                    if ((ret = virtual_machine_running_stack_push_with_configure(vm, current_running_stack, \
                                    module_cur, function_instrument_number, \
                                    args_count, ((opcode == OP_CALLC) || (opcode == OP_TAILCALLC)) ? 1 : 0, \
                                    0, /* Trap PC */ \
                                    0, /* Trap Enabled */ \
                                    new_object_environment_entrance, /* Environment Entrance */ \
                                    NULL /* Variables */ \
                                    )) != 0)
                    { goto fail; }

                    virtual_machine_object_destroy(vm, new_object_environment_entrance);
                    new_object_environment_entrance = NULL;

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
                }
                else
                {
                    /* Pick generator out and push onto stack */
                    if (generator_frame->prev != NULL)
                    {
                        generator_frame->prev->next = generator_frame->next;
                    }
                    else
                    {
                        current_frame->generators->begin = generator_frame->next;
                    }
                    if (generator_frame->next != NULL)
                    {
                        generator_frame->next->prev = generator_frame->prev;
                    }
                    else
                    {
                        current_frame->generators->end = generator_frame->prev;
                    }
                    current_frame->generators->size -= 1;
                    generator_frame->next = generator_frame->prev = NULL;

                    /* Reset argument count */
                    generator_frame->args_count = args_count;

                    if ((ret = virtual_machine_running_stack_push(current_running_stack, generator_frame)) != 0)
                    { goto fail; }
                }

                switch (opcode)
                {
                    case OP_TAILCALL:
                    case OP_TAILCALLC:
                        if ((ret = virtual_machine_running_stack_lift_merge(vm, current_running_stack)) != 0)
                        { goto fail; }
                        break;
                }
            }
            break;

        case OP_TRAPSET:

            current_running_stack->top->trap_pc = operand;

            /* Update PC */
            current_thread->running_stack->top->pc++;

            break;

        case OP_TRAP:

            /*{*/
            /*struct virtual_machine_running_stack_frame *running_stack_frame_cur;*/

            /*printf("\ntrap\n");*/
            /*running_stack_frame_cur = vm->tp->running_stack->top;*/
            /*while (running_stack_frame_cur != NULL)*/
            /*{*/
            /*printf("%d, %u\n", running_stack_frame_cur->trap_enabled, (unsigned int)running_stack_frame_cur->trap_pc);*/
            /*running_stack_frame_cur = running_stack_frame_cur->prev; */
            /*}*/
            /*printf("end\n\n");*/
            /*}*/

            if (current_running_stack->top->trap_enabled != 0)
            {
                current_running_stack->top->pc = current_running_stack->top->trap_pc;
                current_running_stack->top->trap_enabled = 0;
            }
            else
            {
                /* Update PC */
                current_thread->running_stack->top->pc++;
            }

            break;

        case OP_PROMC:

            /* At least the function and result */
            if (current_computing_stack->size < 2)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if (current_computing_stack->top->prev->type != OBJECT_TYPE_FUNCTION)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'function\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Extract Object */
            object_function = current_computing_stack->top->prev->ptr;
            object_function_internal = object_function->ptr_internal; 

            if (object_function_internal->promise == 0)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'promise\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if (object_function_internal->cached_promise != NULL)
            {
                /* Answer already been cached, do nothing */
            }
            else
            {
                /* Cache the answer */
                object_function_internal->cached_promise = virtual_machine_object_clone(vm, current_computing_stack->top);
                if (object_function_internal->cached_promise == NULL)
                {
                    VM_ERR_MALLOC(vm->r);
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }

                /* Clear useless environment entrance */
                if (object_function_internal->environment_entrance != NULL)
                {
                    virtual_machine_object_destroy(vm, object_function_internal->environment_entrance);
                    object_function_internal->environment_entrance = NULL;
                }
            }

            /* Clone result */
            new_object = virtual_machine_object_clone(vm, current_computing_stack->top);
            if (new_object == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            /* Pop function and result */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack); /* Function */
            if (ret != 0) { goto fail; }
            /* Push result back */
            if ((ret = virtual_machine_computing_stack_push(current_computing_stack, new_object)) != 0)
            { goto fail; }
            new_object = NULL;

            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;
    }
    ret = 0;
    goto done;
fail:
done:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
    if (object_arg != NULL) virtual_machine_object_destroy(vm, object_arg);
    if (temp_computing_stack != NULL) virtual_machine_computing_stack_destroy(vm, temp_computing_stack);
    if (new_object_environment_entrance != NULL) virtual_machine_object_destroy(vm, new_object_environment_entrance);
    return ret;
}

