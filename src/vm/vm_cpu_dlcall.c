/* Virtual Machine CPU : Dynamic Library 
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "multiple_err.h"

#include "vm_opcode.h"
#include "vm_types.h"
#include "vm_infrastructure.h"
#include "vm_object.h"
#include "vm_object_aio.h"
#include "vm_err.h"
#include "vm_dynlib.h"
#include "vm_cpu_dlcall.h"

int virtual_machine_thread_step_dlcall(struct virtual_machine *vm)
{
    int ret = 0;
    uint32_t opcode;
    struct virtual_machine_thread *current_thread = vm->tp;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_computing_stack *current_computing_stack;
    struct virtual_machine_object *new_object = NULL;
    char *name_func = NULL, *name_lib = NULL;
    char *str_extract = NULL;
    size_t str_extract_len;

    struct multiple_stub_function_args *function_args = NULL;

    current_running_stack = current_thread->running_stack;
    current_frame = current_running_stack->top;
    current_computing_stack = current_frame->computing_stack;

	opcode = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].opcode;
    switch (opcode)
    {
        case OP_DLCALL:
            /* At least the name of function and library on the top of stack */
            if (current_computing_stack->size < 2)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Library name */

            /* Solve the variable of this variable */
            if ((ret = virtual_machine_variable_solve(&new_object, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }
            if (new_object->type != OBJECT_TYPE_STR)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND_TYPE, \
                        "runtime error: invalid operand type, expected \'str\'");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            virtual_machine_object_str_extract(&str_extract, &str_extract_len, new_object);
            name_lib = (char *)virtual_machine_resource_malloc(vm->resource, sizeof(char) * (str_extract_len + 1));
            if (name_lib == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            memcpy(name_lib, str_extract, str_extract_len);
            name_lib[str_extract_len] = '\0';
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            virtual_machine_object_destroy(vm, new_object);
            new_object = NULL;

            /* Function name */
            if ((ret = virtual_machine_variable_solve(&new_object, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }
            if (new_object->type != OBJECT_TYPE_STR)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND_TYPE, \
                        "runtime error: invalid operand type, expected \'str\'");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            virtual_machine_object_str_extract(&str_extract, &str_extract_len, new_object);
            name_func = (char *)virtual_machine_resource_malloc(vm->resource, sizeof(char) * (str_extract_len + 1));
            if (name_func == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            memcpy(name_func, str_extract, str_extract_len);
            name_func[str_extract_len] = '\0';
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            virtual_machine_object_destroy(vm, new_object);
            new_object = NULL;

            /* Create function arguments for data communication */
            if ((function_args = multiple_stub_function_args_new()) == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            function_args->runtime_return_value = 0;
            function_args->frame = current_frame;
            function_args->vm = vm;

            /* Invoke external function */
            ret = virtual_machine_dynlib_invoke(vm, function_args, name_lib, name_func);
            if (ret != 0) { goto fail; }

            /* Return Value */
            if (function_args->return_value != NULL)
            {
                if ((ret = virtual_machine_computing_stack_push(current_computing_stack, function_args->return_value)) != 0)
                { goto fail; }
                function_args->return_value = NULL;
            }
            else
            {
                if ((new_object = virtual_machine_object_int_new_with_value(vm, ret)) == NULL)
                {
                    VM_ERR_MALLOC(vm->r);
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }
                if ((ret = virtual_machine_computing_stack_push(current_computing_stack, new_object)) != 0)
                { goto fail; }
                new_object = NULL;
            }

            current_thread->running_stack->top->pc++;

            break;
    }
    ret = 0;
    goto done;
fail:
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
done:
    if (name_lib != NULL) virtual_machine_resource_free(vm->resource, name_lib);
    if (name_func != NULL) virtual_machine_resource_free(vm->resource, name_func);
    if (function_args != NULL) multiple_stub_function_args_destroy(function_args);
    return ret;
}


