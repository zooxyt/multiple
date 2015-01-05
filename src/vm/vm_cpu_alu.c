/* Virtual Machine CPU : Arithmetic Logic Unit
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
#include "multiple_ir.h"

#include "vm_opcode.h"
#include "vm_infrastructure.h"
#include "vm_object.h"
#include "vm_object_aio.h"
#include "vm_err.h"
#include "vm_cpu_alu.h"

#define IS_JMP_CONDITIONAL(opcode) (((opcode)==OP_JMPC)||((opcode)==OP_JMPCR))
#define IS_JMP_RELATIVE(opcode) (((opcode)==OP_JMPR)||((opcode)==OP_JMPCR))

/* Signed number representations */

/* Sign and Magnitude to Complement */
/*
static uint32_t snr_sam_to_cmp(int32_t num)
{
    if (num >= 0) { return (uint32_t)num; }
    else 
    {
        return (((uint32_t)(~(-num)))|(1u<<31)) + 1;
    }
}
*/

/* Complement to Sign and Magnitude */
static int32_t snr_cmp_to_sam(uint32_t num)
{
    int sign = (int)(num >> 31);
    if (sign == 0) return (int32_t)num;
    else 
    {
        return -(~((int32_t)(num - 1)));
    }
}


static int virtual_machine_thread_step_alu_reverse(struct virtual_machine *vm, size_t args_count)
{
    int ret = 0;
    struct virtual_machine_thread *current_thread = vm->tp;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_computing_stack *current_computing_stack;
    struct virtual_machine_computing_stack *new_computing_stack_1 = NULL;
    struct virtual_machine_computing_stack *new_computing_stack_2 = NULL;
    struct virtual_machine_object *object_target, *object_prev;
    size_t idx;

    current_running_stack = current_thread->running_stack;
    current_frame = current_running_stack->top;
    current_computing_stack = current_frame->computing_stack;

    if ((new_computing_stack_1 = virtual_machine_computing_stack_new(vm)) == NULL)
    { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
    if ((new_computing_stack_2 = virtual_machine_computing_stack_new(vm)) == NULL)
    { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }

    /* Current Stack -> Stack 2 */
    for (idx = 0; idx != args_count; idx++)
    {
        /* Pick the top one */
        object_target = current_computing_stack->top;
        current_computing_stack->top = current_computing_stack->top->prev;
        current_computing_stack->size -= 1;
        object_target->prev = object_target->next = NULL;

        /* Push into the Stack 1 */
        virtual_machine_computing_stack_push(new_computing_stack_1, object_target);
    }
    if (current_computing_stack->top == NULL)
    {
        current_computing_stack->bottom = NULL;
    }

    /* Stack 1 -> Stack 2 */
    object_target = new_computing_stack_1->top;
    while (object_target != NULL)
    {
        object_prev = object_target->prev;
        object_target->prev = object_target->next = NULL;

        /* Push into the Stack 2 */
        virtual_machine_computing_stack_push(new_computing_stack_2, object_target);

        object_target = object_prev;
    }
    new_computing_stack_1->top = new_computing_stack_1->bottom = NULL;

    /* Stack 2 -> Current Stack */
    object_target = new_computing_stack_2->top;
    while (object_target != NULL)
    {
        object_prev = object_target->prev;
        object_target->prev = object_target->next = NULL;

        /* Push into the Current Stack */
        virtual_machine_computing_stack_push(current_computing_stack, object_target);

        object_target = object_prev;
    }
    new_computing_stack_2->top = new_computing_stack_2->bottom = NULL;

fail:
    if (new_computing_stack_1 != NULL) virtual_machine_computing_stack_destroy(vm, new_computing_stack_1);
    if (new_computing_stack_2 != NULL) virtual_machine_computing_stack_destroy(vm, new_computing_stack_2);
    return ret;
}

static int virtual_machine_thread_step_alu_pick(struct virtual_machine *vm, size_t args_count)
{
    struct virtual_machine_thread *current_thread = vm->tp;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_computing_stack *current_computing_stack;
    struct virtual_machine_object *object_cur, *object_prev, *object_next;

    current_running_stack = current_thread->running_stack;
    current_frame = current_running_stack->top;
    current_computing_stack = current_frame->computing_stack;

    /* Locate to the nth object */
    args_count -= 1;
    object_cur = current_computing_stack->top;
    while (args_count != 0)
    {
        object_cur = object_cur->prev; 
        args_count -= 1;
    }
    object_prev = object_cur->prev;
    object_next = object_cur->next;
    object_cur->next = NULL;
    object_cur->prev = current_computing_stack->top;
    current_computing_stack->top = object_cur;
    if (object_prev == NULL)
    {
        /* The bottom */
        current_computing_stack->bottom = object_next;
        object_next->prev = NULL;
    }
    else
    {
        object_prev->next = object_next;
        object_next->prev = object_prev;
    }

    return 0;
}

static int virtual_machine_thread_step_alu_pickcp(struct virtual_machine *vm, size_t args_count)
{
    int ret = 0;
    struct virtual_machine_thread *current_thread = vm->tp;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_computing_stack *current_computing_stack;
    struct virtual_machine_object *object_cur;
    struct virtual_machine_object *new_object = NULL;

    current_running_stack = current_thread->running_stack;
    current_frame = current_running_stack->top;
    current_computing_stack = current_frame->computing_stack;

    /* Locate to the nth object */
    args_count -= 1;
    object_cur = current_computing_stack->top;
    while (args_count != 0)
    {
        object_cur = object_cur->prev; 
        args_count -= 1;
    }

    /* Clone the element on the top of Computing Stack */
    if ((new_object = virtual_machine_object_clone(vm, object_cur)) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    /* Push the result object into computing stack */
    ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
    if (ret != 0) { goto fail; }
    new_object = NULL;

    goto done;
fail:
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
done:
    return ret;
}

static int virtual_machine_thread_step_alu_insert(struct virtual_machine *vm, size_t args_count)
{
    int ret = 0;
    struct virtual_machine_thread *current_thread = vm->tp;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_computing_stack *current_computing_stack;
    struct virtual_machine_object *object_cur, *object_next;

    struct virtual_machine_object *object_target;

    current_running_stack = current_thread->running_stack;
    current_frame = current_running_stack->top;
    current_computing_stack = current_frame->computing_stack;

    /* 'Pick' the top element out */
    if (current_computing_stack->top->prev == NULL)
    {
        /* Contains only one element? */
        return 0;
    }

    object_target = current_computing_stack->top;
    current_computing_stack->top->prev->next = NULL;
    current_computing_stack->size -= 1;
    object_target->prev = NULL;

    /* Locate to the position */
    args_count -= 1;
    object_cur = current_computing_stack->top;
    while (args_count != 0)
    {
        if (object_cur == NULL)
        { VM_ERR_INTERNAL(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
        object_cur = object_cur->prev; 
        args_count -= 1;
    }
    if (object_cur == NULL)
    { VM_ERR_INTERNAL(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
    object_next = object_cur->next;
    object_cur->next = object_target;
    object_target->prev = object_cur;
    object_target->next = object_next;
    object_next->prev = object_target;
    current_computing_stack->size += 1;

    goto done;
fail:
done:
    return ret;
}

static int virtual_machine_thread_step_alu_insertcp(struct virtual_machine *vm, size_t args_count)
{
    int ret = 0;
    struct virtual_machine_thread *current_thread = vm->tp;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_computing_stack *current_computing_stack;
    struct virtual_machine_object *object_cur, *object_next;

    struct virtual_machine_object *new_object;

    current_running_stack = current_thread->running_stack;
    current_frame = current_running_stack->top;
    current_computing_stack = current_frame->computing_stack;

    /* Clone the top element out */
    if ((new_object = virtual_machine_object_clone(vm, current_computing_stack->top)) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail; 
    }

    /* Locate to the position */
    args_count -= 1;
    object_cur = current_computing_stack->top;
    while (args_count != 0)
    {
        object_cur = object_cur->prev; 
        args_count -= 1;
    }
    object_next = object_cur->next;
    object_cur->next = new_object;
    new_object->prev = object_cur;
    new_object->next = object_next;
    object_next->prev = new_object;
    current_computing_stack->size += 1;

    goto done;
fail:
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
done:
    return ret;
}

int virtual_machine_thread_step_alu(struct virtual_machine *vm)
{
    int ret = 0;
    uint32_t opcode, operand;
    struct virtual_machine_thread *current_thread = vm->tp;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_computing_stack *current_computing_stack;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object *object_solved = NULL;
    int value;
    size_t args_count;

    current_running_stack = current_thread->running_stack;
    current_frame = current_running_stack->top;
    current_computing_stack = current_frame->computing_stack;

	opcode = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].opcode;
	operand = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].operand;
    switch (opcode)
    {
        case OP_DUP:
            /* Stack Top element */
            if (current_computing_stack->size == 0)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Clone the element on the top of Computing Stack */
            if ((new_object = virtual_machine_object_clone(vm, current_computing_stack->top)) == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail; 
            }

            /* Push the clone into Computing Stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;

        case OP_DROP:
            /* Pop the argument count object */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;

        case OP_PICK:
        case OP_PICKCP:
        case OP_INSERT:
        case OP_INSERTCP:

            /* Get the value of the condition */
            if ((ret = virtual_machine_variable_solve(&object_solved, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }
            if (object_solved->type != OBJECT_TYPE_INT)
            {
                vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                        "runtime error: unsupported operand type");
                ret = -MULTIPLE_ERR_VM;
                goto fail; 
            }
            /* Extract argument count */
            args_count = (size_t)virtual_machine_object_int_get_primitive_value(object_solved);
            /* Release solved argument count object */
            virtual_machine_object_destroy(vm, object_solved);
            object_solved = NULL;
            /* Pop the argument count object */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }


            switch (opcode)
            {
                case OP_PICK:
                case OP_PICKCP:
                    /* 0 and 1 for do nothing, 2 for the previous one than top one */
                    if (args_count <= 1)
                    {
                        /* do nothing */
                    }
                    else
                    {
                        if (args_count > current_computing_stack->size)
                        {
                            vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                                    "runtime error: computing stack empty");
                            ret = -MULTIPLE_ERR_VM;
                            goto fail;
                        }
                        switch (opcode)
                        {
                            case OP_PICK:
                                if ((ret = virtual_machine_thread_step_alu_pick(vm, args_count)) != 0)
                                { goto fail; }
                                break;
                            case OP_PICKCP:
                                if ((ret = virtual_machine_thread_step_alu_pickcp(vm, args_count)) != 0)
                                { goto fail; }
                                break;
                        }
                    }
                    break;

                case OP_INSERT:
                case OP_INSERTCP:
                    /* 0 and 1 for do nothing, 2 for the previous one than top one */
                    if (args_count <= 1)
                    {
                        /* do nothing */
                    }
                    else
                    {
                        if (args_count > current_computing_stack->size)
                        {
                            vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                                    "runtime error: computing stack empty");
                            ret = -MULTIPLE_ERR_VM;
                            goto fail;
                        }
                        switch (opcode)
                        {
                            case OP_INSERT:
                                if ((ret = virtual_machine_thread_step_alu_insert(vm, args_count)) != 0)
                                { goto fail; }
                                break;
                            case OP_INSERTCP:
                                if ((ret = virtual_machine_thread_step_alu_insertcp(vm, args_count)) != 0)
                                { goto fail; }
                                break;
                        }
                    }
                    break;
            }

            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;

        case OP_REVERSE:
        case OP_REVERSEP:

            /* Clone the element on the top of Computing Stack */
            if ((object_solved = virtual_machine_object_clone(vm, current_computing_stack->top)) == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail; 
            }
            /* Get the value of the condition */
            if (object_solved->type != OBJECT_TYPE_INT)
            {
                vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                        "runtime error: unsupported operand type");
                ret = -MULTIPLE_ERR_VM;
                goto fail; 
            }
            /* Extract argument count */
            args_count = (size_t)virtual_machine_object_int_get_primitive_value(object_solved);
            /* Release solved argument count object */
            virtual_machine_object_destroy(vm, object_solved);
            object_solved = NULL;
            /* Pop the argument count object */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            if (current_computing_stack->size < args_count)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Reverse */
            if (args_count > 1)
            {
                virtual_machine_thread_step_alu_reverse(vm, args_count);
            }

            if (opcode == OP_REVERSEP)
            {
                /* Restore the preserved count */
                if ((new_object = virtual_machine_object_int_new_with_value( \
                                vm, \
                                (int)args_count)) == NULL)
                {
                    VM_ERR_MALLOC(vm->r);
                    ret = -MULTIPLE_ERR_VM;
                    goto fail; 
                }
                new_object->next = NULL;
                new_object->prev = NULL;
                ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
                if (ret != 0) { goto fail; }
                new_object = NULL;
            }

            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;

        case OP_ADD: case OP_SUB: case OP_MUL: case OP_DIV: case OP_MOD:
        case OP_LSHIFT: case OP_RSHIFT:
        case OP_ANDA: case OP_ORA: case OP_XORA:
        case OP_ANDL: case OP_ORL: case OP_XORL:
        case OP_EQ: case OP_NE: 
        case OP_L: case OP_G: case OP_LE: case OP_GE:

            /* Binary operator requires at least 2 objects */
            if (current_computing_stack->size < 2)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            ret = virtual_machine_object_binary_operate(&new_object, current_computing_stack->top->prev, current_computing_stack->top, \
                    opcode, vm);
            if (ret != 0) goto fail;

            /* Pop the top 2 elements */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Push the result object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;

        case OP_NEG:
        case OP_NOTA:
        case OP_NOTL:

            /* Unary operator requires at least 1 objects */
            if (current_computing_stack->size < 1)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            ret = virtual_machine_object_unary_operate(&new_object, current_computing_stack->top, \
                    opcode, vm);
            if (ret != 0) goto fail;

            /* Pop the top 1 element */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Push the result object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;


        case OP_JMP:
        case OP_JMPC:
        case OP_JMPR:
        case OP_JMPCR:

            if (IS_JMP_CONDITIONAL(opcode))
            {
                /* At least 1 boolean value on the stack */
                if (current_computing_stack->size == 0)
                {
                    vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                            "runtime error: computing stack empty");
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }

                if ((ret = virtual_machine_variable_solve(&object_solved, current_computing_stack->top, current_frame, 1, vm)) != 0)
                { goto fail; }

                /* Get the value of the condition */
                if ((ret = virtual_machine_object_bool_valid(object_solved)) != 0)
                {
                    vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                            "runtime error: unsupported operand type");
                    ret = -MULTIPLE_ERR_VM;
                    goto fail; 
                }
                if ((ret = virtual_machine_object_bool_get_value(object_solved, &value)) != 0)
                { goto fail; }

                /* Pop the top 1 element */
                ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
                if (ret != 0) { goto fail; }

                virtual_machine_object_destroy(vm, object_solved); object_solved = NULL;
            }
            else
            {
                value = VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE;
            }

            if (value == VIRTUAL_MACHINE_OBJECT_BOOL_VALUE_TRUE)
            {
                if (IS_JMP_RELATIVE(opcode))
                {
                    /* Relative */
                    int rel = snr_cmp_to_sam(operand);
                    if (rel >= 0)
                    {
                        current_thread->running_stack->top->pc += (uint32_t)(rel);
                    }
                    else
                    {
                        current_thread->running_stack->top->pc -= (uint32_t)(-rel);
                    }
                }
                else
                {
                    /* Absolute */
                    current_thread->running_stack->top->pc = operand;
                }
            }
            else
            {
                /* Update PC */
                current_thread->running_stack->top->pc++;
            }
            break;
    }
    ret = 0;
    goto done;
fail:
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
done:
    return ret;
}

