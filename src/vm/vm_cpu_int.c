/* Virtual Machine CPU : Interrupt
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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "multiple_err.h"

#include "vm_opcode.h"
#include "vm_types.h"
#include "vm_infrastructure.h"
#include "vm_object.h"
#include "vm_object_aio.h"
#include "vm_err.h"

#include "vm_cpu_int.h"

/* Global Variable for passing command line arguments */
extern const char **g_argv;
extern int g_argc;

int virtual_machine_interrupt(struct virtual_machine *vm)
{
    int ret = 0;
    uint32_t opcode, operand;
    struct virtual_machine_thread *thread = vm->tp;
    struct virtual_machine_thread *current_thread = vm->tp;
    struct virtual_machine_module *current_module;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_computing_stack *current_computing_stack;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object *new_object_solved = NULL;
    struct virtual_machine_object *try_object = NULL;
    int i;
    uint32_t ee_num;
    uint32_t event_id;
    struct virtual_machine_external_event *new_external_event = NULL;

    struct virtual_machine_data_section_item *data_section_item_operand = NULL;

    int interrupt_number;

    current_running_stack = current_thread->running_stack;
    current_frame = current_running_stack->top;
    current_module = current_frame->module;
    current_computing_stack = current_frame->computing_stack;

    opcode = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].opcode;
	operand = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].operand;
    switch (opcode)
    {
        case OP_INT:
            /* Get interrupt number */
            if (virtual_machine_module_lookup_data_section_items(current_module, &data_section_item_operand, operand) == LOOKUP_NOT_FOUND)
            {
                VM_ERR_INTERNAL(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail; 
            }
            if ((new_object = virtual_machine_object_new_from_data_section_item(vm, data_section_item_operand)) == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail; 
            }
            if ((ret = virtual_machine_variable_solve(&new_object_solved, new_object, current_frame, 1, vm)) != 0)
            { goto fail; }
            virtual_machine_object_destroy(vm, new_object);
            new_object = NULL;

            if (new_object_solved->type != OBJECT_TYPE_INT)
            {
                vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                        "runtime error: unsupported operand type");
                ret = -MULTIPLE_ERR_VM;
                goto fail; 
            }

            interrupt_number = ((struct virtual_machine_object_int *)(new_object_solved->ptr))->value;
            virtual_machine_object_destroy(vm, new_object_solved);
            new_object_solved = NULL;

            switch (interrupt_number)
            {
                case VM_INT_ARGS:
                    for (i = g_argc - 1; ;)
                    {
                        /* Create new string object with an argument */
                        new_object = virtual_machine_object_str_new_with_value(vm, g_argv[(size_t)i], strlen(g_argv[(size_t)i]));
                        if (new_object == NULL)
                        {
                            VM_ERR_MALLOC(vm->r);
                            ret = -MULTIPLE_ERR_VM;
                            goto fail; 
                        }
                        /* Push value into computing stack */
                        if ((ret = virtual_machine_computing_stack_push(current_computing_stack, new_object)) != 0)
                        {
                            VM_ERR_INTERNAL(vm->r);
                            ret = -MULTIPLE_ERR_VM;
                            goto fail;
                        }
                        new_object = NULL;

                        i--;
                        if (i == -1) break;
                    }
                    if ((ret = virtual_machine_object_list_make(vm, \
                                    &new_object, \
                                    current_computing_stack->top, (size_t)g_argc, VIRTUAL_MACHINE_OBJECT_LIST_MAKE_ORDER_DEFAULT, \
                                    NULL)) != 0)
                    { goto fail; }
                    /* Push the list into computing stack */
                    if ((ret = virtual_machine_computing_stack_push(current_computing_stack, new_object)) != 0)
                    {
                        VM_ERR_INTERNAL(vm->r);
                        ret = -MULTIPLE_ERR_VM;
                        goto fail;
                    }
                    new_object = NULL;
                    break;

                case VM_INT_GC:

                    /* Garbage Collect */
                    virtual_machine_garbage_collect(vm);
                    break;

                case VM_INT_EE_ALLOC:

                    /* Allocate Event Number */

                    thread_mutex_lock(&vm->external_events->lock);
                    event_id = virtual_machine_external_event_list_get_id(vm->external_events);
                    thread_mutex_unlock(&vm->external_events->lock);

                    new_object = virtual_machine_object_int_new_with_value( \
                            vm, \
                            (int)event_id);
                    if (new_object == NULL)
                    {
                        VM_ERR_MALLOC(vm->r);
                        ret = -MULTIPLE_ERR_VM;
                        goto fail; 
                    }
                    /* Push the list into computing stack */
                    if ((ret = virtual_machine_computing_stack_push(current_computing_stack, new_object)) != 0)
                    {
                        VM_ERR_INTERNAL(vm->r);
                        ret = -MULTIPLE_ERR_VM;
                        goto fail;
                    }
                    new_object = NULL;

                    break;

                case VM_INT_EE_INSTALL:

                    /* External Event Install */

                    /* Stack Top element */
                    if (current_running_stack->size < 2)
                    {
                        vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                                "runtime error: computing stack empty");
                        ret = -MULTIPLE_ERR_VM;
                        goto fail;
                    }

                    /* External Event Number */
                    if ((ret = virtual_machine_variable_solve(&new_object_solved, current_computing_stack->top, current_frame, 1, vm)) != 0)
                    { goto fail; }
                    if (new_object_solved->type != OBJECT_TYPE_INT)
                    {
                        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                                "runtime error: unsupported operand type");
                        ret = -MULTIPLE_ERR_VM;
                        goto fail; 
                    }
                    ee_num = (uint32_t)virtual_machine_object_int_get_primitive_value(new_object_solved);
                    virtual_machine_object_destroy(vm, new_object_solved);
                    new_object_solved = NULL;

                    /* Callback : function */
                    if ((ret = virtual_machine_variable_solve(&new_object_solved, current_computing_stack->top->prev, current_frame, 1, vm)) != 0)
                    { goto fail; }
                    if (new_object_solved->type != OBJECT_TYPE_FUNCTION)
                    {
                        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                                "runtime error: unsupported operand type");
                        ret = -MULTIPLE_ERR_VM;
                        goto fail; 
                    }

                    ret = virtual_machine_computing_stack_pop(vm, current_frame->computing_stack);
                    if (ret != 0) { goto fail; }
                    ret = virtual_machine_computing_stack_pop(vm, current_frame->computing_stack);
                    if (ret != 0) { goto fail; }

                    /* Create new event */
                    new_external_event = virtual_machine_external_event_new(vm, new_object_solved, ee_num);
                    if (new_external_event == NULL)
                    {
                        VM_ERR_MALLOC(vm->r);
                        ret = -MULTIPLE_ERR_VM;
                        goto fail; 
                    }
                    new_object_solved = NULL;
                    thread_mutex_lock(&vm->external_events->lock);
                    virtual_machine_external_event_list_append(vm->external_events, new_external_event);
                    thread_mutex_unlock(&vm->external_events->lock);
                    new_external_event = NULL;

                    break;

                case VM_INT_EE_UNINSTALL:

                    /* External Event Uninstall */
                    break;

                case VM_INT_EE_WAIT:

                    /* Wait for External Event */

                    /* Stack Top element */
                    if (current_running_stack->size < 1)
                    {
                        vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                                "runtime error: computing stack empty");
                        ret = -MULTIPLE_ERR_VM;
                        goto fail;
                    }

                    /* External Event Number */
                    if ((ret = virtual_machine_variable_solve(&new_object_solved, current_computing_stack->top, current_frame, 1, vm)) != 0)
                    { goto fail; }
                    if (new_object_solved->type != OBJECT_TYPE_INT)
                    {
                        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                                "runtime error: unsupported operand type");
                        ret = -MULTIPLE_ERR_VM;
                        goto fail; 
                    }
                    ee_num = (uint32_t)virtual_machine_object_int_get_primitive_value(new_object_solved);
                    (void)ee_num;

                    virtual_machine_object_destroy(vm, new_object_solved);
                    new_object_solved = NULL;

                    ret = virtual_machine_computing_stack_pop(vm, current_frame->computing_stack);
                    if (ret != 0) { goto fail; }

                    break;

                case VM_INT_DBG_START:

                    /* Start debugging */
                    current_thread->type = VIRTUAL_MACHINE_THREAD_TYPE_DEBUGGER;
                    vm->debug_mode = 1;

                    break;

                default:
                    /* Invalid interrupt number */
                    break;
            }

            /* Update PC */
            thread->running_stack->top->pc++;

            break;
        default:
            break;
    }
    ret = 0;
    goto done;
fail:
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
    if (new_object_solved != NULL) _virtual_machine_object_destroy(vm, new_object_solved);
    if (new_external_event != NULL) virtual_machine_external_event_destroy(vm, new_external_event);
done:
    if (try_object != NULL) virtual_machine_object_destroy(vm, try_object);
    return ret;
}

