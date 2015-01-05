/* Virtual Machine CPU : Class
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

#include "multiple_err.h"

#include "vm_opcode.h"
#include "vm_types.h"
#include "vm_infrastructure.h"
#include "vm_object.h"
#include "vm_object_aio.h"
#include "vm_err.h"
#include "vm_cpu_class.h"

int virtual_machine_thread_step_class(struct virtual_machine *vm)
{
    int ret = 0;
    uint32_t opcode, operand;
    struct virtual_machine_thread *current_thread = vm->tp;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_computing_stack *current_computing_stack;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object *new_object_def = NULL, *new_object_this = NULL;
    struct virtual_machine_object *new_object_hash = NULL, *new_object_key = NULL, *new_object_value = NULL;

    struct virtual_machine_data_type *data_type_target;
    struct virtual_machine_object_identifier *object_identifier;

    struct virtual_machine_object_identifier *object_identifier_method;
    struct virtual_machine_object_identifier *object_identifier_data_type;
    struct virtual_machine_object_identifier *object_identifier_def;
    int method_type;

    char *data_type_name = NULL;
    size_t data_type_len;

    int args_count;

    current_running_stack = current_thread->running_stack;
    current_frame = current_running_stack->top;
    current_computing_stack = current_frame->computing_stack;

	opcode = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].opcode;
	operand = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].operand;
    (void)operand;
    switch (opcode)
    {
        case OP_CLSTYPEREG:

            if (current_computing_stack->top->type != OBJECT_TYPE_IDENTIFIER)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'identifier\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            virtual_machine_object_identifier_get_value(current_computing_stack->top, &data_type_name, &data_type_len);

            /* TODO: detect if the data type has been registered? */
            if (virtual_machine_data_type_list_lookup(&data_type_target, vm->data_types, \
                        data_type_name, data_type_len) == LOOKUP_FOUND)
            {
                vm_err_update(vm->r, -VM_ERR_DATA_TYPE, \
                        "runtime error: data type \'%s\' already been registered", data_type_name);
                return -MULTIPLE_ERR_VM;
                goto fail;
            }

            if (virtual_machine_data_type_list_append_with_configure( \
                        vm, \
                        vm->data_types, \
                        data_type_name, data_type_len) != 0)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Pop the 1 element */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            current_thread->running_stack->top->pc++;

            break;

        case OP_CLSINSTMK:

            if (current_computing_stack->top->type != OBJECT_TYPE_IDENTIFIER)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'identifier\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            object_identifier = (struct virtual_machine_object_identifier *)(current_computing_stack->top->ptr);

            if (virtual_machine_data_type_list_lookup(&data_type_target, vm->data_types, \
                        object_identifier->id,
                        object_identifier->id_len) == LOOKUP_NOT_FOUND)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: data type \'%s\' not declared", object_identifier->id);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            if ((new_object = virtual_machine_object_class_new_with_value(vm, data_type_target)) == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            /* Pop the 1 elements */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            /* Push the result object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            current_thread->running_stack->top->pc++;
            break;

        case OP_CLSINSTRM:
            VM_ERR_NOT_IMPLEMENTED(vm->r);
            ret = -MULTIPLE_ERR_VM;
            goto fail;

			/*
            current_thread->running_stack->top->pc++;
            break;
			*/

        case OP_CLSPGET:

            /* At least 1 object and 1 property on the stack */
            if (current_computing_stack->size < 2)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Get Property */
            if ((ret = virtual_machine_object_property_get(&new_object, \
                            current_computing_stack->top->prev, \
                            current_computing_stack->top, \
                            vm)) != 0)
            { goto fail; }


            /* Pop the 2 elements */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Push the result object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            current_thread->running_stack->top->pc++;
            break;

        case OP_CLSPSET:

            /* At least 1 object, 1 property and 1 value on the stack */
            if (current_computing_stack->size < 3)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* src, property, value */

            /* Set Property */
            if ((ret = virtual_machine_object_property_set(&new_object, \
                            current_computing_stack->top, \
                            current_computing_stack->top->prev, \
                            current_computing_stack->top->prev->prev, \
                            vm)) != 0)
            { goto fail; }


            /* Pop the 3 elements */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Push the result object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            current_thread->running_stack->top->pc++;
            break;

        case OP_CLSDTOR:

            if (current_computing_stack->size < 1)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Solve 'this' */
            if ((ret = virtual_machine_variable_solve(&new_object_this, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }

            /* Perform method */
            if ((ret = virtual_machine_object_destructor_confirm(new_object_this, vm)) != 0)
            { goto fail; }

            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            current_thread->running_stack->top->pc++;

            break;

        case OP_CLSMADD:
        case OP_CLSCTORADD:
        case OP_CLSDTORADD:

            if (current_computing_stack->size < 3)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((current_computing_stack->top->type != OBJECT_TYPE_IDENTIFIER) && \
                    (current_computing_stack->top->prev->type != OBJECT_TYPE_IDENTIFIER) && \
                    (current_computing_stack->top->prev->prev->type != OBJECT_TYPE_IDENTIFIER))
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'identifier\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }


            object_identifier_def = (struct virtual_machine_object_identifier *)(current_computing_stack->top->ptr);
            object_identifier_data_type = (struct virtual_machine_object_identifier *)(current_computing_stack->top->prev->ptr);
            object_identifier_method = (struct virtual_machine_object_identifier *)(current_computing_stack->top->prev->prev->ptr);
            switch (opcode)
            {
                case OP_CLSMADD:
                    method_type = VIRTUAL_MACHINE_DATA_TYPE_METHOD_TYPE_NORMAL;
                    break;
                case OP_CLSCTORADD:
                    method_type = VIRTUAL_MACHINE_DATA_TYPE_METHOD_TYPE_CONSTRUCTOR;
                    break;
                case OP_CLSDTORADD:
                    method_type = VIRTUAL_MACHINE_DATA_TYPE_METHOD_TYPE_DESTRUCTOR;
					break;
				default:
					VM_ERR_INTERNAL(vm->r);
					ret = -MULTIPLE_ERR_VM;
					goto fail;
            }
            if ((ret = virtual_machine_data_type_list_method_add_with_configure( \
                            vm, vm->data_types, \
                            object_identifier_method->id, object_identifier_method->id_len,
                            object_identifier_data_type->id, object_identifier_data_type->id_len,
                            object_identifier_def->id, object_identifier_def->id_len, object_identifier_def->id_module_id, object_identifier_def->id_data_id,
                            object_identifier_def->domain_id, object_identifier_def->domain_id_len, object_identifier_def->domain_id_module_id, object_identifier_def->domain_id_data_id, \
                            method_type)) != 0)
            { goto fail; }

            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            current_thread->running_stack->top->pc++;
            break;

        case OP_CLSMINVOKE:

            /* At least 1 object and 1 method and 1 arguments count on the stack */
            if (current_computing_stack->size < 3)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            if (current_computing_stack->top->prev->type != OBJECT_TYPE_IDENTIFIER)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'identifier\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            if (current_computing_stack->top->prev->prev->type != OBJECT_TYPE_INT)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'int\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            args_count = ((struct virtual_machine_object_int *)(current_computing_stack->top->prev->prev->ptr))->value;
            if (args_count < 0)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: arguments number should be non-negative number");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Solve 'this' */
            if ((ret = virtual_machine_variable_solve(&new_object_this, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }

            /* Perform method */
            if ((ret = virtual_machine_object_method_invoke(&new_object_def, \
                            new_object_this, \
                            current_computing_stack->top->prev, \
                            current_computing_stack->top->prev->prev, \
                            (unsigned int)args_count, vm)) != 0)
            { goto fail; }

            /* Pop unsolved this, method name and args count */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Push this */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object_this);
            if (ret != 0) { goto fail; }
            new_object_this = NULL;

            /* Push an increased arguments count */
            new_object = virtual_machine_object_int_new_with_value(vm, args_count + 1);
            if (new_object == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            /* Push function of method */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object_def);
            if (ret != 0) { goto fail; }
            new_object_def = NULL;

            /* All stuff's ready, wait for the execution of the next 'call' instrument */

            current_thread->running_stack->top->pc++;

            break;
    }
    ret = 0;
    goto done;
fail:
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
done:
    if (new_object_hash != NULL) virtual_machine_object_destroy(vm, new_object_hash);
    if (new_object_key != NULL) virtual_machine_object_destroy(vm, new_object_key);
    if (new_object_value != NULL) virtual_machine_object_destroy(vm, new_object_value);
    if (new_object_def != NULL) virtual_machine_object_destroy(vm, new_object_def);
    if (new_object_this != NULL) virtual_machine_object_destroy(vm, new_object_this);

    return ret;
}

