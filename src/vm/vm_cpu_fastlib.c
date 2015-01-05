/* Virtual Machine CPU : Fast Library
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
#include "vm_cpu_fastlib.h"

#include "umath.h"
#include "vm_cpu_fastlib_case.h"

/* Constants */

#ifndef DBL_EPSILON
#define DBL_EPSILON 0.00001
#endif
#ifndef ABS
#define ABS(x) ((x)>0?(x):(-(x)))
#endif

static int fastlib_putchar(struct virtual_machine *vm, \
        struct virtual_machine_running_stack_frame *target_frame, \
        struct virtual_machine_object *object_src)
{
    int ret = 0;
    struct virtual_machine_object *object_src_solved = NULL;
    int value_int;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, object_src, target_frame, 1, vm)) != 0)
    { goto fail; }

    if (object_src_solved->type == OBJECT_TYPE_INT)
    {
        value_int = virtual_machine_object_int_get_primitive_value(object_src_solved);
        putchar(value_int);
    }

fail:
    return ret;
}

static int fastlib_getchar(struct virtual_machine *vm, \
        struct virtual_machine_object **object_dst)
{
    int ret = 0;
    int value_int;
    struct virtual_machine_object *new_object = NULL;

    value_int = getchar();

    new_object = virtual_machine_object_int_new_with_value( \
            vm, \
            value_int);
    if (new_object == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    *object_dst = new_object;

    goto done;
fail:
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
done:
    return ret;
}

static int fastlib_abs(struct virtual_machine *vm, \
        struct virtual_machine_running_stack_frame *target_frame, \
        struct virtual_machine_object **object_dst, \
        struct virtual_machine_object *object_src)
{
    int ret = 0;
    struct virtual_machine_object *object_src_solved = NULL;
    int value_int;
    double value_float;

    *object_dst = NULL;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, object_src, target_frame, 1, vm)) != 0)
    { goto fail; }

    if (object_src_solved->type == OBJECT_TYPE_INT)
    {
        value_int = virtual_machine_object_int_get_primitive_value(object_src_solved);
        value_int = umath_int_abs(value_int);
        virtual_machine_object_int_set_primitive_value(object_src_solved, value_int);
    }
    else if (object_src_solved->type == OBJECT_TYPE_FLOAT)
    {
        value_float = virtual_machine_object_float_get_primitive_value(object_src_solved);
        value_float = umath_double_abs(value_float);
        virtual_machine_object_float_set_primitive_value(object_src_solved, value_float);
    }
    else
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    *object_dst = object_src_solved;
fail:
    return ret;
}

static int fastlib_sqrt(struct virtual_machine *vm, \
        struct virtual_machine_running_stack_frame *target_frame, \
        struct virtual_machine_object **object_dst, \
        struct virtual_machine_object *object_src)
{
    int ret = 0;
    struct virtual_machine_object *object_src_solved = NULL;
    struct virtual_machine_object *new_object = NULL;
    int value_int, value_int_result;
    double value_float, value_float_result;

    *object_dst = NULL;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, object_src, target_frame, 1, vm)) != 0)
    { goto fail; }

    if (object_src_solved->type == OBJECT_TYPE_INT)
    {
        value_int = virtual_machine_object_int_get_primitive_value(object_src_solved);
        if (value_int >= 0)
        {
            value_float_result = umath_double_sqrt((double)value_int);
            value_int_result = umath_int_sqrt(value_int);
            if (ABS((double)value_int_result - value_float_result) < DBL_EPSILON)
            {
                /* Integer */
                virtual_machine_object_int_set_primitive_value(object_src_solved, value_int_result);
                *object_dst = object_src_solved; object_src_solved = NULL;
            }
            else
            {
                /* Float */
                if ((new_object = virtual_machine_object_float_new_with_value(vm, \
                                value_float_result)) == NULL)
                { goto fail; }
                *object_dst = new_object; new_object = NULL;
            }
        }
        else
        {
            value_float_result = umath_double_sqrt(-(double)value_int);
            value_int_result = umath_int_sqrt(-value_int);
            if (ABS((double)value_int_result - value_float_result) < DBL_EPSILON)
            {
                /* Rational in Image Part */
                if ((new_object = virtual_machine_object_complex_new_with_rr(vm, \
                                0, 0, 1, \
                                0, (unsigned int)value_int_result, 1)) == NULL)
                { goto fail; }
                *object_dst = new_object; new_object = NULL;
            }
            else
            {
                /* Float in Image Part */
                if ((new_object = virtual_machine_object_complex_new_with_ff(vm, \
                                0, 0, \
                                0, value_float_result)) == NULL)
                { goto fail; }
                *object_dst = new_object; new_object = NULL;
            }
        }
    }
    else if (object_src_solved->type == OBJECT_TYPE_FLOAT)
    {
        value_float = virtual_machine_object_float_get_primitive_value(object_src_solved);
        if (value_float >= 0)
        {
            value_float_result = umath_double_sqrt(value_float);
            virtual_machine_object_float_set_primitive_value(object_src_solved, value_float_result);
            *object_dst = object_src_solved; object_src_solved = NULL;
        }
        else
        {
            value_float_result = umath_double_sqrt(-value_float);
            if ((new_object = virtual_machine_object_complex_new_with_ff(vm, \
                    0, 0, \
                    0, value_float_result)) == NULL)
            { goto fail; }
            *object_dst = new_object; new_object = NULL;
        }
    }
    else
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    goto done;
fail:
done:
    if (object_src_solved != NULL) virtual_machine_object_destroy(vm, object_src_solved);
    return ret;
}

static int fastlib_exp(struct virtual_machine *vm, \
        struct virtual_machine_running_stack_frame *target_frame, \
        struct virtual_machine_object **object_dst, \
        struct virtual_machine_object *object_src)
{
    int ret = 0;
    struct virtual_machine_object *object_src_solved = NULL;
    struct virtual_machine_object *new_object = NULL;
    int value_int;
    double value_float, value_float_result;

    *object_dst = NULL;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, object_src, target_frame, 1, vm)) != 0)
    { goto fail; }

    if (object_src_solved->type == OBJECT_TYPE_INT)
    {
        value_int = virtual_machine_object_int_get_primitive_value(object_src_solved);
        value_float = (double)value_int;
        value_float_result = umath_double_exp(value_float);
        if ((new_object = virtual_machine_object_float_new_with_value(vm, \
                        value_float_result)) == NULL)
        { goto fail; }
        *object_dst = new_object; new_object = NULL;
    }
    else if (object_src_solved->type == OBJECT_TYPE_FLOAT)
    {
        value_float = virtual_machine_object_float_get_primitive_value(object_src_solved);
        value_float = umath_double_exp(value_float);
        virtual_machine_object_float_set_primitive_value(object_src_solved, value_float);
    }
    else
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    *object_dst = object_src_solved;
fail:
    return ret;
}

static int fastlib_log(struct virtual_machine *vm, \
        struct virtual_machine_running_stack_frame *target_frame, \
        struct virtual_machine_object **object_dst, \
        struct virtual_machine_object *object_src)
{
    int ret = 0;
    struct virtual_machine_object *object_src_solved = NULL;
    struct virtual_machine_object *new_object = NULL;
    int value_int;
    double value_float, value_float_result;

    *object_dst = NULL;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, object_src, target_frame, 1, vm)) != 0)
    { goto fail; }

    if (object_src_solved->type == OBJECT_TYPE_INT)
    {
        value_int = virtual_machine_object_int_get_primitive_value(object_src_solved);
        value_float = (double)(value_int);
        value_float_result = umath_double_log(value_float);
        if ((new_object = virtual_machine_object_float_new_with_value(vm, \
                        value_float_result)) == NULL)
        { goto fail; }
        *object_dst = new_object; new_object = NULL;
    }
    else if (object_src_solved->type == OBJECT_TYPE_FLOAT)
    {
        value_float = virtual_machine_object_float_get_primitive_value(object_src_solved);
        value_float = umath_double_log(value_float);
        virtual_machine_object_float_set_primitive_value(object_src_solved, value_float);
        *object_dst = object_src_solved;
    }
    else
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

fail:
    return ret;
}

static int fastlib_triangle(struct virtual_machine *vm, \
        struct virtual_machine_running_stack_frame *target_frame, \
        struct virtual_machine_object **object_dst, \
        struct virtual_machine_object *object_src, \
        uint32_t operand)
{
    int ret = 0;
    struct virtual_machine_object *object_src_solved = NULL;
    struct virtual_machine_object *new_object = NULL;
    int value_int;
    double value_float, value_float_result = 0.0;

    *object_dst = NULL;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, object_src, target_frame, 1, vm)) != 0)
    { goto fail; }

    if (object_src_solved->type == OBJECT_TYPE_INT)
    {
        value_int = virtual_machine_object_int_get_primitive_value(object_src_solved);
        value_float = (double)value_int;

        switch (operand)
        {
            case OP_FASTLIB_SIN:
                value_float_result = umath_double_sin(value_float);
                break;
            case OP_FASTLIB_COS:
                value_float_result = umath_double_cos(value_float);
                break;
            case OP_FASTLIB_TAN:
                value_float_result = umath_double_tan(value_float);
                break;
            case OP_FASTLIB_ASIN:
                value_float_result = umath_double_asin(value_float);
                break;
            case OP_FASTLIB_ACOS:
                value_float_result = umath_double_acos(value_float);
                break;
            case OP_FASTLIB_ATAN:
                value_float_result = umath_double_atan(value_float);
                break;
        }

        /* Float */
        if ((new_object = virtual_machine_object_float_new_with_value(vm, \
                        value_float_result)) == NULL)
        { goto fail; }
        *object_dst = new_object; new_object = NULL;
    }
    else if (object_src_solved->type == OBJECT_TYPE_FLOAT)
    {
        value_float = virtual_machine_object_float_get_primitive_value(object_src_solved);
        switch (operand)
        {
            case OP_FASTLIB_SIN:
                value_float = umath_double_sin(value_float);
                break;
            case OP_FASTLIB_COS:
                value_float = umath_double_cos(value_float);
                break;
            case OP_FASTLIB_TAN:
                value_float = umath_double_tan(value_float);
                break;
            case OP_FASTLIB_ASIN:
                value_float = umath_double_asin(value_float);
                break;
            case OP_FASTLIB_ACOS:
                value_float = umath_double_acos(value_float);
                break;
            case OP_FASTLIB_ATAN:
                value_float = umath_double_atan(value_float);
                break;
        }
        virtual_machine_object_float_set_primitive_value(object_src_solved, value_float);
        *object_dst = object_src_solved;
    }
    else
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
fail:
    return ret;
}

static int fastlib_case_cast(struct virtual_machine *vm, \
        struct virtual_machine_running_stack_frame *target_frame, \
        struct virtual_machine_object **object_dst, \
        struct virtual_machine_object *object_src, \
        uint32_t opcode)
{
    int ret = 0;
    struct virtual_machine_object *object_src_solved = NULL;
    uint32_t value, value_out = 0;

    *object_dst = NULL;

    if ((ret = virtual_machine_variable_solve(&object_src_solved, object_src, target_frame, 1, vm)) != 0)
    { goto fail; }

    if (object_src_solved->type == OBJECT_TYPE_CHAR)
    {
        value = virtual_machine_object_char_get_primitive_value(object_src_solved);
        switch (opcode)
        {
            case OP_FASTLIB_UPCASE:
                if ((vm_cpu_fastlib_case_upcase_char(&value_out, value)) != 0)
                {
                    vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                            "runtime error: convert failed");
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }
                break;
            case OP_FASTLIB_DOWNCASE:
                if ((vm_cpu_fastlib_case_downcase_char(&value_out, value)) != 0)
                {
                    vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                            "runtime error: convert failed");
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }
                break;
        }
        virtual_machine_object_char_set_primitive_value(object_src_solved, value_out);
    }
    else
    {
        vm_err_update(vm->r, -VM_ERR_UNSUPPORTED_OPERAND_TYPE, \
                "runtime error: unsupported operand type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    *object_dst = object_src_solved;
fail:
    return ret;
}

int virtual_machine_thread_step_fastlib(struct virtual_machine *vm)
{
    int ret = 0;
    uint32_t opcode, operand;
    struct virtual_machine_thread *thread = vm->tp;
    struct virtual_machine_thread *current_thread = vm->tp;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_computing_stack *current_computing_stack;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object *object_solved = NULL;

    current_running_stack = current_thread->running_stack;
    current_frame = current_running_stack->top;
    current_computing_stack = current_frame->computing_stack;

	opcode = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].opcode;
	operand = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].operand;

    if (opcode != OP_FASTLIB)
    {
        VM_ERR_INTERNAL(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    switch (operand)
    {
        case OP_FASTLIB_NOP:

            /* Update PC */
            thread->running_stack->top->pc++;
            break;

        case OP_FASTLIB_PUTCHAR:

            /* Stack Top element */
            if (current_computing_stack->size < 1)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            ret = fastlib_putchar(vm, current_running_stack->top, current_computing_stack->top);
            if (ret != 0) { goto fail; }

            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Update PC */
            thread->running_stack->top->pc++;
            break;

        case OP_FASTLIB_GETCHAR:

            if ((ret = fastlib_getchar(vm, &new_object)) != 0)
            { goto fail; }

            /* Push the result object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            /* Update PC */
            thread->running_stack->top->pc++;
            break;


        case OP_FASTLIB_ABS:
        case OP_FASTLIB_SQRT:
        case OP_FASTLIB_EXP:
        case OP_FASTLIB_LOG:

            /* Stack Top element */
            if (current_computing_stack->size < 1)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            if ((ret = virtual_machine_variable_solve(&object_solved, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }

            switch (operand)
            {
                case OP_FASTLIB_ABS:
                    if ((ret = fastlib_abs(vm, current_frame, &new_object, object_solved)) != 0)
                    { goto fail; }
                    break;
                case OP_FASTLIB_SQRT:
                    if ((ret = fastlib_sqrt(vm, current_frame, &new_object, object_solved)) != 0)
                    { goto fail; }
                    break;
                case OP_FASTLIB_EXP:
                    if ((ret = fastlib_exp(vm, current_frame, &new_object, object_solved)) != 0)
                    { goto fail; }
                    break;
                case OP_FASTLIB_LOG:
                    if ((ret = fastlib_log(vm, current_frame, &new_object, object_solved)) != 0)
                    { goto fail; }
                    break;
            }

            /* Push the result object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            /* Update PC */
            thread->running_stack->top->pc++;
            break;

        case OP_FASTLIB_SIN: case OP_FASTLIB_COS: case OP_FASTLIB_TAN:
        case OP_FASTLIB_ASIN: case OP_FASTLIB_ACOS: case OP_FASTLIB_ATAN:

            /* Stack Top element */
            if (current_computing_stack->size < 1)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            if ((ret = virtual_machine_variable_solve(&object_solved, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }

            if ((ret = fastlib_triangle(vm, current_frame, &new_object, object_solved, operand)) != 0)
            { goto fail; }

            /* Push the result object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            /* Update PC */
            thread->running_stack->top->pc++;
            break;

        case OP_FASTLIB_UPCASE:
        case OP_FASTLIB_DOWNCASE:

            /* Stack Top element */
            if (current_computing_stack->size < 1)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            if ((ret = virtual_machine_variable_solve(&object_solved, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }

            if ((ret = fastlib_case_cast(vm, current_frame, &new_object, object_solved, operand)) != 0)
            { goto fail; }

            /* Push the result object into computing stack */
            ret = virtual_machine_computing_stack_push(current_computing_stack, new_object);
            if (ret != 0) { goto fail; }
            new_object = NULL;

            /* Update PC */
            thread->running_stack->top->pc++;
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
    if (object_solved != NULL) virtual_machine_object_destroy(vm, object_solved);
    return ret;
}

