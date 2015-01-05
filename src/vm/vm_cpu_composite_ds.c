/* Virtual Machine CPU : Composite Data Structure
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

#include "multiple_err.h"

#include "vm_opcode.h"
#include "vm_types.h"
#include "vm_infrastructure.h"
#include "vm_object.h"
#include "vm_object_aio.h"
#include "vm_err.h"
#include "vm_cpu_composite_ds.h"

int virtual_machine_thread_step_composite_ds(struct virtual_machine *vm)
{
    int ret = 0;
    uint32_t opcode, operand;
    struct virtual_machine_thread *current_thread = vm->tp;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_computing_stack *current_computing_stack;
    struct virtual_machine_object *new_object = NULL, *object_solved = NULL;
    struct virtual_machine_object *new_object_hash = NULL, *new_object_key = NULL, *new_object_value = NULL;
    struct virtual_machine_object *new_object_list = NULL, *new_object_array = NULL, *new_object_element = NULL;

    current_running_stack = current_thread->running_stack;
    current_frame = current_running_stack->top;
    current_computing_stack = current_frame->computing_stack;

	opcode = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].opcode;
	operand = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].operand;
    switch (opcode)
    {
        case OP_LSTMK:
        case OP_ARRMK:
        case OP_TUPMK:
        case OP_HASHMK:

            /* At least [operand] elements on the stack */
            switch (opcode)
            {
                case OP_LSTMK:
                case OP_ARRMK:
                case OP_TUPMK:
                    if (current_computing_stack->size < (size_t)operand)
                    {
                        vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                                "runtime error: computing stack empty");
                        ret = -MULTIPLE_ERR_VM;
                        goto fail;
                    }
                    break;
                case OP_HASHMK:
                    if (current_computing_stack->size < (size_t)operand * 2)
                    {
                        vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                                "runtime error: computing stack empty");
                        ret = -MULTIPLE_ERR_VM;
                        goto fail;
                    }
                    break;
            }

            /* Make list */
            switch (opcode)
            {
                case OP_LSTMK:
                    if ((ret = virtual_machine_object_list_make(vm, \
                                    &new_object, \
                                    current_computing_stack->top, \
									(size_t)operand, VIRTUAL_MACHINE_OBJECT_LIST_MAKE_ORDER_DEFAULT, \
                                    NULL)) != 0)
                    { goto fail; }
                    break;
                case OP_ARRMK:
                    if ((ret = virtual_machine_object_array_make(vm, \
                                    &new_object, \
                                    current_computing_stack->top, \
									(size_t)operand, VIRTUAL_MACHINE_OBJECT_ARRAY_MAKE_ORDER_DEFAULT, \
                                    NULL)) != 0)
                    { goto fail; }
                    break;
				case OP_TUPMK:
					if ((ret = virtual_machine_object_tuple_make(vm, \
						&new_object, \
						current_computing_stack->top, \
						(size_t)operand)) != 0)
					{ goto fail; }
					break;
                case OP_HASHMK:
                    if ((ret = virtual_machine_object_hash_make(vm, &new_object, current_computing_stack->top, (size_t)operand, NULL)) != 0)
                    { goto fail; }
                    break;
            }


            switch (opcode)
            {
                case OP_HASHMK:
                    /* Hash requires to pop two times of elements */
                    operand *= 2;
                    break;
            }

            /* Pop the elements of list */
            while (operand-- != 0)
            {
                ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
                if (ret != 0) { goto fail; }
            }

            /* Push the result object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            current_thread->running_stack->top->pc++;
            break;

        case OP_PAIRMK:

            if (current_computing_stack->size < 2)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((new_object = virtual_machine_object_pair_make(vm, \
                    current_computing_stack->top, \
                    current_computing_stack->top->prev)) == NULL)
            { goto fail; }

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

        case OP_LSTCAR:
        case OP_LSTCDR:
        case OP_ARRCAR:
        case OP_ARRCDR:
        case OP_TUPCAR:
        case OP_TUPCDR:
        case OP_PAIRCAR:
        case OP_PAIRCDR:

            /* At least 1 object on the stack */
            if (current_computing_stack->size < 1)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((ret = virtual_machine_variable_solve(&object_solved, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }

            /* Perform action */
            switch (opcode)
            {
                case OP_LSTCAR:
                    if (object_solved->type == OBJECT_TYPE_LIST)
                    {
                        ret = virtual_machine_object_list_car(vm, &new_object, object_solved);
                    }
                    else
                    {
                        ret = virtual_machine_object_pair_car(vm, &new_object, object_solved);
                    }
                    break;
                case OP_ARRCAR:
                    ret = virtual_machine_object_array_car(vm, &new_object, object_solved);
                    break;
                case OP_LSTCDR:
                    if (object_solved->type == OBJECT_TYPE_LIST)
                    {
                        ret = virtual_machine_object_list_cdr(vm, &new_object, object_solved);
                    }
                    else
                    {
                        ret = virtual_machine_object_pair_cdr(vm, &new_object, object_solved);
                    }
                    break;
                case OP_ARRCDR:
                    ret = virtual_machine_object_array_cdr(vm, &new_object, object_solved);
                    break;
            }
            if (ret != 0) { goto fail; }

            /* Pop the list object */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Push the result object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            current_thread->running_stack->top->pc++;
            break;

        case OP_REFGET:

            /* At least 1 object and 1 index element on the stack */
            if (current_computing_stack->size < 2)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Variable */
            if ((ret = virtual_machine_variable_solve(&object_solved, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }

            /* Reference by index */

            switch (object_solved->type)
            {
                case OBJECT_TYPE_LIST:
                    if ((ret = virtual_machine_object_list_ref_get(vm, \
                                    &new_object, \
                                    object_solved, \
                                    current_computing_stack->top->prev)) != 0)
                    { goto fail; }
                    break;
                case OBJECT_TYPE_ARRAY:
                    if ((ret = virtual_machine_object_array_ref_get(vm, \
                                    &new_object, \
                                    object_solved, \
                                    current_computing_stack->top->prev)) != 0)
                    { goto fail; }
                    break;
                case OBJECT_TYPE_TUPLE:
                    if ((ret = virtual_machine_object_tuple_ref_get(vm, \
                                    &new_object, \
                                    object_solved, \
                                    current_computing_stack->top->prev)) != 0)
                    { goto fail; }
                    break;
                case OBJECT_TYPE_HASH:
                    if ((ret = virtual_machine_object_hash_ref_get(vm, \
                                    &new_object, \
                                    object_solved, \
                                    current_computing_stack->top->prev)) != 0)
                    { goto fail; }
                    break;
                default:
                    vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                            "runtime error: invalid operand, composite type expected");
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                    break;
            }

            /* Pop 2 elements of list */
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

        case OP_REFSET:

            /* At least 1 object, 1 index element and 1 value on the stack */
            if (current_computing_stack->size < 3)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Variable */
            if ((ret = virtual_machine_variable_solve(&object_solved, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }

            /* Reference by index */

            switch (object_solved->type)
            {
                case OBJECT_TYPE_LIST:
                    if ((ret = virtual_machine_object_list_ref_set(vm, &new_object, \
                                    object_solved, \
                                    current_computing_stack->top->prev, \
                                    current_computing_stack->top->prev->prev)) != 0)
                    { goto fail; }
                    break;
                case OBJECT_TYPE_ARRAY:
                    if ((ret = virtual_machine_object_array_ref_set(vm, &new_object, \
                                    object_solved, \
                                    current_computing_stack->top->prev, \
                                    current_computing_stack->top->prev->prev)) != 0)
                    { goto fail; }
                    break;
                case OBJECT_TYPE_TUPLE:
                    if ((ret = virtual_machine_object_tuple_ref_set(vm, &new_object, \
                                    object_solved, \
                                    current_computing_stack->top->prev, \
                                    current_computing_stack->top->prev->prev)) != 0)
                    { goto fail; }
                    break;
                case OBJECT_TYPE_HASH:
                    if ((ret = virtual_machine_object_hash_ref_set(vm, \
                                    object_solved, \
                                    current_computing_stack->top->prev, \
                                    current_computing_stack->top->prev->prev)) != 0)
                    { goto fail; }
                    break;
                default:
                    vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                            "runtime error: invalid operand, composite type expected");
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                    break;
            }

            /* Pop 3 elements of list */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Push the result object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, object_solved);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            current_thread->running_stack->top->pc++;
            break;

        case OP_LSTADD: 
        case OP_LSTADDH: 
        case OP_ARRADD: 

            /* object */
            /* element */

            /* At least hash and 1 element on the stack */
            if (current_computing_stack->size < 2)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Element */
            if ((ret = virtual_machine_variable_solve(&new_object_element, current_computing_stack->top->prev, current_frame, 1, vm)) != 0)
            { goto fail; }

            switch (opcode)
            {
                case OP_LSTADD: 
                    if ((ret = virtual_machine_variable_solve(&new_object_list, current_computing_stack->top, current_frame, 1, vm)) != 0)
                    { goto fail; }
                    if (new_object_list->type != OBJECT_TYPE_LIST)
                    {
                        vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                                "runtime error: invalid operand, type \'list\' expected");
                        ret = -MULTIPLE_ERR_VM;
                        goto fail;
                    }
                    if ((ret = virtual_machine_object_list_append( \
                                    vm, \
                                    new_object_list, /* list */
                                    new_object_element) /* element */
                        ) != 0) 
                    { goto fail; }
                    break;
                case OP_LSTADDH: 
                    if ((ret = virtual_machine_variable_solve(&new_object_list, current_computing_stack->top, current_frame, 1, vm)) != 0)
                    { goto fail; }
                    if (new_object_list->type != OBJECT_TYPE_LIST)
                    {
                        vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                                "runtime error: invalid operand, type \'list\' expected");
                        ret = -MULTIPLE_ERR_VM;
                        goto fail;
                    }
                    if ((ret = virtual_machine_object_list_append_to_head( \
                                    vm, \
                                    new_object_list, /* list */
                                    new_object_element) /* element */
                        ) != 0) 
                    { goto fail; }
                    break;
                case OP_ARRADD: 
                    if ((ret = virtual_machine_variable_solve(&new_object_array, current_computing_stack->top, current_frame, 1, vm)) != 0)
                    { goto fail; }
                    if (new_object_array->type != OBJECT_TYPE_ARRAY)
                    {
                        vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                                "runtime error: invalid operand, type \'array\' expected");
                        ret = -MULTIPLE_ERR_VM;
                        goto fail;
                    }
                    if ((ret = virtual_machine_object_array_append( \
                                    vm, \
                                    new_object_array, /* list */
                                    new_object_element) /* element */
                        ) != 0) 
                    { goto fail; }
                    break;
            }

            new_object_element = NULL;

            /* Pop list and element */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            switch (opcode)
            {
                case OP_LSTADD: 
                    /* Push the result object into computing stack */
                    ret = virtual_machine_computing_stack_push(current_computing_stack, new_object_list);
                    if (ret != 0) { goto fail; }
                    new_object_list = NULL;
                    break;
                case OP_ARRADD: 
                    /* Push the result object into computing stack */
                    ret = virtual_machine_computing_stack_push(current_computing_stack, new_object_array);
                    if (ret != 0) { goto fail; }
                    new_object_array = NULL;
                    break;
            }
            current_thread->running_stack->top->pc++;
            break;

        case OP_HASHADD: 

            /* object */
            /* key */
            /* value */

            /* At least hash and 1 key and 1 value on the stack */
            if (current_computing_stack->size < 3)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Hash */
            if ((ret = virtual_machine_variable_solve(&new_object_hash, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }
            if (new_object_hash->type != OBJECT_TYPE_HASH)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'hash\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Key */
            if ((new_object_key = virtual_machine_object_clone(vm, current_computing_stack->top->prev)) == NULL)
            { goto fail; }
            /* Value */
            if ((new_object_value = virtual_machine_object_clone(vm, current_computing_stack->top->prev->prev)) == NULL)
            { goto fail; }

            if ((ret = virtual_machine_object_hash_append( \
                            vm, \
                            new_object_hash, /* hash */
                            new_object_key, /* key */
                            new_object_value)) != 0) /* value */
            {
                goto fail;
            }
            new_object_key = NULL;
            new_object_value = NULL;

            /* Pop hash and key and value */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Push the result object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object_hash);
            if (ret != 0) { goto fail; }
            new_object_hash = NULL;

            current_thread->running_stack->top->pc++;
            break;

        case OP_HASHDEL: 

            /* At least hash and 1 key on the stack */
            if (current_computing_stack->size < 2)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Hash */
            if ((ret = virtual_machine_variable_solve(&new_object_hash, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }

            /* Key */
            if ((new_object_key = virtual_machine_object_clone(vm, current_computing_stack->top->prev)) == NULL)
            { goto fail; }

            if (new_object_hash->type != OBJECT_TYPE_HASH)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'hash\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((ret = virtual_machine_object_hash_remove(new_object_hash, /* hash */
                            new_object_key, /* key */
                            vm)) != 0) 
            {
                goto fail;
            }

            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Push the result object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object_hash);
            if (ret != 0) { goto fail; }
            new_object_hash = NULL;

            current_thread->running_stack->top->pc++;
            break;

        case OP_HASHHASKEY: 

            /* At least hash and 1 key on the stack */
            if (current_computing_stack->size < 2)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Hash */
            if ((ret = virtual_machine_variable_solve(&new_object_hash, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }

            /* Key */
            if ((new_object_key = virtual_machine_object_clone(vm, current_computing_stack->top->prev)) == NULL)
            { goto fail; }

            if ((ret = virtual_machine_variable_solve(&new_object_key, current_computing_stack->top->prev, current_frame, 1, vm)) != 0)
            { goto fail; }

            if (new_object_hash->type != OBJECT_TYPE_HASH)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'hash\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((ret = virtual_machine_object_hash_haskey( \
                            vm, \
                            &new_object,
                            new_object_hash, /* hash */
                            new_object_key /* key */
                            )) != 0) 
            {
                goto fail;
            }

            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Push the result object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }

            current_thread->running_stack->top->pc++;
            break;

        case OP_HASHCAR: 
        case OP_HASHCDR: 

            /* At least hash and 1 key on the stack */
            if (current_computing_stack->size < 1)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            switch (opcode)
            {
                case OP_HASHCAR: 
                    if ((ret = virtual_machine_object_hash_car(vm, \
                                    &new_object, current_computing_stack->top)) != 0)
                    { goto fail; }
                    break;
                case OP_HASHCDR: 
                    if ((ret = virtual_machine_object_hash_cdr(vm, \
                                    &new_object, current_computing_stack->top)) != 0)
                    { goto fail; }
                    break;
            }

            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Push the result object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            current_thread->running_stack->top->pc++;
            break;

        case OP_PAIRCARSET: case OP_PAIRCDRSET:

            /* At least 2 objects on the stack */
            if (current_computing_stack->size < 2)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            switch (opcode)
            {
                case OP_PAIRCARSET:
                    if ((ret = virtual_machine_object_pair_car_set(vm, &new_object, \
                                    current_computing_stack->top, \
                                    current_computing_stack->top->prev)) != 0)
                    { goto fail; }
                    break;
                case OP_PAIRCDRSET:
                    if ((ret = virtual_machine_object_pair_cdr_set(vm, &new_object, \
                                    current_computing_stack->top, \
                                    current_computing_stack->top->prev)) != 0)
                    { goto fail; }
                    break;
            }

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

        case OP_LSTCDRSET: 

            /* At least 2 objects on the stack */
            if (current_computing_stack->size < 2)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((ret = virtual_machine_object_list_cdr_set( \
                            vm, \
                            &new_object, \
                            current_computing_stack->top, \
                            current_computing_stack->top->prev)) != 0)
            { goto fail; }

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


        case OP_LSTUNPACK:
        case OP_LSTUNPACKR:

            /* At least hash and 1 key on the stack */
            if (current_computing_stack->size < 1)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((ret = virtual_machine_variable_solve(&object_solved, \
                            current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }

            if (object_solved->type != OBJECT_TYPE_LIST)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, composite type expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Pop the original list */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            switch (opcode)
            {
                case OP_LSTUNPACK:
                    if ((ret = virtual_machine_object_list_unpack( \
                                    vm, \
                                    object_solved, \
                                    VIRTUAL_MACHINE_OBJECT_LIST_UNPACK_ORDER_DEFAULT)) != 0)
                    { goto fail; }
                    break;

                case OP_LSTUNPACKR:
                    if ((ret = virtual_machine_object_list_unpack( \
                                    vm, \
                                    object_solved, \
                                    VIRTUAL_MACHINE_OBJECT_LIST_UNPACK_ORDER_REVERSE)) != 0)
                    { goto fail; }
                    break;
            }

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
    if (new_object_list != NULL) virtual_machine_object_destroy(vm, new_object_list);
    if (new_object_element != NULL) virtual_machine_object_destroy(vm, new_object_element);
    if (object_solved != NULL) virtual_machine_object_destroy(vm, object_solved);
    return ret;
}

