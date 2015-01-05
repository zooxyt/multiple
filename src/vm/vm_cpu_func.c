/* Virtual Machine CPU : Function
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
#include "vm_object.h"
#include "vm_object_aio.h"
#include "vm_err.h"
#include "vm_cpu_func.h"

int virtual_machine_thread_step_func(struct virtual_machine *vm)
{
    int ret = 0;
    uint32_t opcode, operand;
    struct virtual_machine_thread *current_thread = vm->tp;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_computing_stack *current_computing_stack;
    struct virtual_machine_object *new_object = NULL;

    current_running_stack = current_thread->running_stack;
    current_frame = current_running_stack->top;
    current_computing_stack = current_frame->computing_stack;

	opcode = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].opcode;
	operand = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].operand;
    switch (opcode)
    {
        case OP_FUNCMK:

            /* At least the domain name && function name of sub routine on the top of stack */
            if (current_computing_stack->size < 1)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            if (current_computing_stack->top->type != OBJECT_TYPE_FUNCTION)
            {
                if ((ret = virtual_machine_object_func_make(&new_object, current_computing_stack->top, vm)) != 0)
                {
                    vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                            "runtime error: making function failed");
                    ret = -MULTIPLE_ERR_VM;
                    goto fail; 
                }

                ret = virtual_machine_computing_stack_pop(vm, current_computing_stack); /* Function */
                if (ret != 0) { goto fail; }

                if ((ret = virtual_machine_computing_stack_push(current_computing_stack, new_object)) != 0)
                { goto fail; }
            }

            current_thread->running_stack->top->pc++;
            break;

        case OP_NFUNCMK:
        case OP_LAMBDAMK:
        case OP_PROMMK:
        case OP_CONTMK:

            switch (opcode)
            {
                case OP_NFUNCMK:
                    if ((ret = virtual_machine_object_func_make_normal(&new_object, current_frame->module, operand, vm)) != 0)
                    {
                        vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                                "runtime error: making function failed");
                        ret = -MULTIPLE_ERR_VM;
                        goto fail; 
                    }
                    break;
                case OP_LAMBDAMK:
                    if ((ret = virtual_machine_object_func_make_lambda(&new_object, current_frame->module, operand, vm)) != 0)
                    {
                        vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                                "runtime error: making function failed");
                        ret = -MULTIPLE_ERR_VM;
                        goto fail; 
                    }
                    break;
                case OP_PROMMK:
                    if ((ret = virtual_machine_object_func_make_promise(&new_object, current_frame->module, operand, vm)) != 0)
                    {
                        vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                                "runtime error: making promise failed");
                        ret = -MULTIPLE_ERR_VM;
                        goto fail; 
                    }
                    break;
                case OP_CONTMK:
                    if ((ret = virtual_machine_object_func_make_cont(&new_object, current_frame->module, operand, vm)) != 0)
                    {
                        vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                                "runtime error: making function failed");
                        ret = -MULTIPLE_ERR_VM;
                        goto fail; 
                    }
                    break;
            }

            if ((ret = virtual_machine_computing_stack_push(current_computing_stack, new_object)) != 0)
            { goto fail; }

            current_thread->running_stack->top->pc++;
            break;
    }
    ret = 0;
    goto done;
fail:
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
done:
    return ret;
}

