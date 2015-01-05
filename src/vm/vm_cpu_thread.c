/* Virtual Machine CPU : Thread
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
#include "vm_cpu_thread.h"

static int virtual_machine_thread_step_thread_message(struct virtual_machine *vm)
{
    int ret = 0;
    uint32_t opcode;
    struct virtual_machine_thread *current_thread = vm->tp;
    struct virtual_machine_thread *target_thread = NULL; 
    uint32_t target_tid;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_computing_stack *current_computing_stack;

    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object *new_object_thread = NULL;
    struct virtual_machine_object *new_object_message = NULL;
    struct virtual_machine_object *new_object_message_bak = NULL;
    struct virtual_machine_message *new_message = NULL;

    current_running_stack = current_thread->running_stack;
    current_frame = current_running_stack->top;
    current_computing_stack = current_frame->computing_stack;

    opcode = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].opcode;
    switch (opcode)
    {
        case OP_TSENDMSG:

            /* At least the thread and the message object on the top of stack */
            if (current_computing_stack->size < 2)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Thread */
            if ((ret = virtual_machine_variable_solve(&new_object_thread, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }
            if (new_object_thread->type != OBJECT_TYPE_THREAD)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'thread\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            target_tid = ((struct virtual_machine_object_thread *)(new_object_thread->ptr))->tid;
            virtual_machine_object_destroy(vm, new_object_thread); new_object_thread = NULL;

            /* Message */
            if ((ret = virtual_machine_variable_solve(&new_object_message, current_computing_stack->top->prev, current_frame, 1, vm)) != 0)
            {
                goto fail; 
            }

            /* Search the target thread */
            target_thread = vm->threads->begin;
            while (target_thread != NULL)
            {
                if (target_thread->tid == target_tid)
                {
                    break;
                }
                target_thread = target_thread->next;
            }

            if (target_thread == NULL)
            {
                vm_err_update(vm->r, -VM_ERR_OBJECT_NOT_FOUND, \
                        "runtime error: thread not found");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((new_object_message_bak = virtual_machine_object_clone(vm, new_object_message)) == NULL)
            { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
            
            if ((ret = virtual_machine_message_queue_push_with_configure(vm, target_thread->messages, new_object_message, current_thread->tid)) != 0)
            { goto fail; }
            new_object_message = NULL;

            /* Pop the top 2 element */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Push message itself back */
            if ((ret = virtual_machine_computing_stack_push(current_computing_stack, new_object_message_bak)) != 0)
            {
                VM_ERR_INTERNAL(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            new_object_message_bak = NULL;

            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;

        case OP_TRECVMSG:

            if ((ret = virtual_machine_message_queue_pop(current_thread->messages, &new_message)) != 0)
            {
                goto fail;
            }
            if (new_message == NULL)
            {
                if ((new_object_message = virtual_machine_object_none_new(vm)) == NULL)
                { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
            }
            else
            {
                if ((new_object_message = virtual_machine_object_clone(vm, new_message->object)) == NULL)
                { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
                virtual_machine_message_destroy(vm, new_message);
                new_message = NULL;
            }

            /* Push message itself back */
            if ((ret = virtual_machine_computing_stack_push(current_computing_stack, new_object_message)) != 0)
            { VM_ERR_INTERNAL(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
            new_object_message = NULL;

            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;

        case OP_TISEMPTY:

            if ((new_object = virtual_machine_object_bool_new_with_value(vm, current_thread->messages->size == 0)) == NULL)
            { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
            if ((ret = virtual_machine_computing_stack_push(current_computing_stack, new_object)) != 0)
            { VM_ERR_INTERNAL(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
            new_object = NULL;

            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;
    }
    ret = 0;
    goto done;
fail:
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
    if (new_object_thread != NULL) _virtual_machine_object_destroy(vm, new_object_thread);
    if (new_object_message != NULL) _virtual_machine_object_destroy(vm, new_object_message);
    if (new_object_message_bak != NULL) _virtual_machine_object_destroy(vm, new_object_message_bak);
done:
    return ret;
}

static int virtual_machine_thread_step_thread_mutex(struct virtual_machine *vm)
{
    int ret = 0;
    int ret_local;
    uint32_t opcode;
    struct virtual_machine_thread *current_thread = vm->tp;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_computing_stack *current_computing_stack;
    struct virtual_machine_mutex *new_mutex = NULL;

    struct virtual_machine_object *new_object = NULL;
    uint32_t mutex_id;

    current_running_stack = current_thread->running_stack;
    current_frame = current_running_stack->top;
    current_computing_stack = current_frame->computing_stack;

    opcode = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].opcode;
    switch (opcode)
    {
        case OP_MTXMK:

            /* Create a new mutex */
            new_mutex = virtual_machine_mutex_new(vm);
            if (new_mutex == NULL)
            { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
            /* Append into the mutex list, and get the mutex id assigned */
            virtual_machine_mutex_list_append(vm->mutexes, new_mutex);
            mutex_id = new_mutex->id;
            new_mutex = NULL;

            new_object = virtual_machine_object_mutex_new_with_mutex_id(vm, mutex_id);
            if (new_object == NULL)
            { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }

            if ((ret = virtual_machine_computing_stack_push(current_thread->running_stack->top->computing_stack, new_object)) != 0)
            { VM_ERR_INTERNAL(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
            new_object = NULL;


            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;

        case OP_MTXLCK:

            if (current_computing_stack->size == 0)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((ret = virtual_machine_variable_solve(&new_object, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }

            if (new_object->type != OBJECT_TYPE_MUTEX)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'mutex\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            ret_local = virtual_machine_object_mutex_state( \
                    vm, \
                    new_object);
            switch (ret_local)
            {
                case VIRTUAL_MACHINE_MUTEX_LOCKED:
                    /* Mark locking */
                    vm->locked = 1;
                    break;

                case VIRTUAL_MACHINE_MUTEX_UNLOCKED:
                    virtual_machine_object_mutex_lock( \
                            vm, \
                            new_object);

                    /* Pop the top 1 element */
                    ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
                    if (ret != 0) { goto fail; }

                    /* Update PC */
                    current_thread->running_stack->top->pc++;

                    break;

                default:
                    vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                            "runtime error: invalid \'mutex\'");
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
            }

            virtual_machine_object_destroy(vm, new_object);

            break;

        case OP_MTXUNLCK:

            if (current_computing_stack->size == 0)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((ret = virtual_machine_variable_solve(&new_object, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }

            if (new_object->type != OBJECT_TYPE_MUTEX)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'mutex\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            virtual_machine_object_mutex_unlock( \
                    vm, \
                    new_object);

            virtual_machine_object_destroy(vm, new_object);

            /* Pop the top 1 element */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;
    }
    ret = 0;
    goto done;
fail:
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
    if (new_mutex != NULL) virtual_machine_mutex_destroy(vm, new_mutex);
done:
    return ret;
}

static int virtual_machine_thread_step_thread_semaphore(struct virtual_machine *vm)
{
    int ret = 0;
    int value;
    int retval;
    uint32_t opcode;
    struct virtual_machine_thread *current_thread = vm->tp;
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_computing_stack *current_computing_stack;
    struct virtual_machine_semaphore *new_semaphore = NULL;

    struct virtual_machine_object *new_object = NULL;
    uint32_t semaphore_id;

    current_running_stack = current_thread->running_stack;
    current_frame = current_running_stack->top;
    current_computing_stack = current_frame->computing_stack;

    opcode = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].opcode;
    switch (opcode)
    {
        case OP_SEMMK:

            /* Initial Value of semaphore */
            if (current_computing_stack->size == 0)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            if ((ret = virtual_machine_variable_solve(&new_object, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }
            if (new_object->type != OBJECT_TYPE_INT)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'int\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            value = virtual_machine_object_int_get_primitive_value(new_object);
            virtual_machine_object_destroy(vm, new_object);

            /* Pop the top 1 element */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Create a new semaphore */
            new_semaphore = virtual_machine_semaphore_new(vm, value);
            if (new_semaphore == NULL)
            { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
            /* Append into the semaphore list, and get the semaphore id assigned */
            virtual_machine_semaphore_list_append(vm->semaphores, new_semaphore);
            semaphore_id = new_semaphore->id;
            new_semaphore = NULL;

            new_object = virtual_machine_object_semaphore_new_with_semaphore_id(vm, semaphore_id);
            if (new_object == NULL)
            { VM_ERR_MALLOC(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }

            if ((ret = virtual_machine_computing_stack_push(current_thread->running_stack->top->computing_stack, new_object)) != 0)
            { VM_ERR_INTERNAL(vm->r); ret = -MULTIPLE_ERR_VM; goto fail; }
            new_object = NULL;

            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;

        case OP_SEMP:

            if (current_computing_stack->size == 0)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((ret = virtual_machine_variable_solve(&new_object, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }

            if (new_object->type != OBJECT_TYPE_SEMAPHORE)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'semaphore\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            retval = virtual_machine_object_semaphore_try_p( \
                    vm, \
                    new_object);
            if (retval == VIRTUAL_MACHINE_SEMAPHORE_TRY_BUSY)
            {
                /* Mark locking */
                /*fprintf(stderr, "vm lock\n");fflush(stderr);*/
                vm->locked = 1;
            }
            else if (retval == VIRTUAL_MACHINE_SEMAPHORE_NO_FOUND)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: semaphore not found");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            else
            {
                /* Pop the top 1 element */
                ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
                if (ret != 0) { goto fail; }
                /* Update PC */
                current_thread->running_stack->top->pc++;
            }

            virtual_machine_object_destroy(vm, new_object);

            break;

        case OP_SEMV:

            if (current_computing_stack->size == 0)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((ret = virtual_machine_variable_solve(&new_object, current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }

            if (new_object->type != OBJECT_TYPE_SEMAPHORE)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'semaphore\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((virtual_machine_object_semaphore_v( \
                    vm, \
                    new_object)) != 0)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: semaphore not found");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            virtual_machine_object_destroy(vm, new_object);

            /* Pop the top 1 element */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }

            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;
    }
    ret = 0;
    goto done;
fail:
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
    if (new_semaphore != NULL) virtual_machine_semaphore_destroy(vm, new_semaphore);
done:
    return ret;
}

int virtual_machine_thread_step_thread(struct virtual_machine *vm)
{
    int ret = 0;
    uint32_t opcode;
    struct virtual_machine_thread *current_thread = vm->tp;
    struct virtual_machine_thread *new_thread = NULL; /* For creating new thread */
    struct virtual_machine_running_stack *current_running_stack;
    struct virtual_machine_running_stack_frame *current_frame;
    struct virtual_machine_computing_stack *current_computing_stack;
    struct virtual_machine_object *new_object = NULL;
    uint32_t tid;

    current_running_stack = current_thread->running_stack;
    current_frame = current_running_stack->top;
    current_computing_stack = current_frame->computing_stack;

    opcode = vm->tp->running_stack->top->module->text_section->instruments[(size_t)current_frame->pc].opcode;
    switch (opcode)
    {
        case OP_TFK:
            /* Forking */
            if ((new_thread = virtual_machine_thread_clone(vm, current_thread)) == NULL)
            {
                vm_err_update(vm->r, -MULTIPLE_ERR_VM, \
                        "runtime error: failed to clone current thread");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            /* Increase PC of new thread */
            new_thread->running_stack->top->pc++;

            /* Append the thread into thread list */
            if ((ret = virtual_machine_thread_list_append(vm->threads, new_thread)) != 0)
            {
                VM_ERR_INTERNAL(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            /* Return child thread id to parent thread */
            if ((new_object = virtual_machine_object_thread_new_with_tid(vm, new_thread->tid)) == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            if ((ret = virtual_machine_computing_stack_push(current_thread->running_stack->top->computing_stack, new_object)) != 0)
            {
                VM_ERR_INTERNAL(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            new_object = NULL;

            /* Return none to child thread */
            if ((new_object = virtual_machine_object_none_new(vm)) == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            if ((ret = virtual_machine_computing_stack_push(new_thread->running_stack->top->computing_stack, new_object)) != 0)
            {
                VM_ERR_INTERNAL(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            new_object = NULL;

            /* Switch to the new thread */
            vm->tp = new_thread;

            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;

        case OP_TWAIT:

            if ((ret = virtual_machine_variable_solve(&new_object,current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }
            if (new_object->type != OBJECT_TYPE_THREAD)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'thread\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if (virtual_machine_thread_list_is_tid_exists( \
                        vm->threads, \
                        ((struct virtual_machine_object_thread *)(new_object->ptr))->tid) != 0)
            {
                /* Exists, should wait for it */
                /* Skip to next thread */
                if (vm->tp->next != NULL) { vm->tp = vm->tp->next; }
            }
            else
            {
                /* Update PC */
                current_thread->running_stack->top->pc++;
            }
            virtual_machine_object_destroy(vm, new_object); new_object = NULL;
            break;

        case OP_TCUR:
            /* Return current thread */
            if ((new_object = virtual_machine_object_thread_new_with_tid(vm, current_thread->tid)) == NULL)
            {
                VM_ERR_MALLOC(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((ret = virtual_machine_computing_stack_push(current_computing_stack, new_object)) != 0)
            {
                VM_ERR_INTERNAL(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            new_object = NULL;

            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;

        case OP_TALIVE:

            /* At least the thread on the top of stack */
            if (current_computing_stack->size == 0)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((ret = virtual_machine_variable_solve(&new_object,current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }
            if (new_object->type != OBJECT_TYPE_THREAD)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'thread\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if (virtual_machine_thread_list_is_tid_exists(vm->threads, ((struct virtual_machine_object_thread *)(new_object->ptr))->tid) != 0)
            {
                virtual_machine_object_destroy(vm, new_object); new_object = NULL;
                if ((new_object = virtual_machine_object_bool_new_with_value(vm, 1)) == NULL)
                {
                    VM_ERR_MALLOC(vm->r);
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }
            }
            else
            {
                virtual_machine_object_destroy(vm, new_object); new_object = NULL;
                if ((new_object = virtual_machine_object_bool_new_with_value(vm, 0)) == NULL)
                {
                    VM_ERR_MALLOC(vm->r);
                    ret = -MULTIPLE_ERR_VM;
                    goto fail;
                }
            }

            /* Pop the top 1 element */
            ret = virtual_machine_computing_stack_pop(vm, current_computing_stack);
            if (ret != 0) { goto fail; }
            
            if ((ret = virtual_machine_computing_stack_push(current_computing_stack, new_object)) != 0)
            {
                VM_ERR_INTERNAL(vm->r);
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }
            new_object = NULL;
            
            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;

        case OP_TEXIT:

            /* Initial Stack Frame remains, destroy the thread */
            virtual_machine_thread_list_remove(vm, vm->threads, vm->tp);
            /* Skip to next thread */
            if (vm->threads->size == 0) { vm->tp = NULL; }
            else { vm->tp = vm->threads->begin; }
            break;

        case OP_TSUSPEND:
        case OP_TRESUME:

            /* At least the thread on the top of stack */
            if (current_computing_stack->size == 0)
            {
                vm_err_update(vm->r, -VM_ERR_COMPUTING_STACK_EMPTY, \
                        "runtime error: computing stack empty");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            if ((ret = virtual_machine_variable_solve(&new_object,current_computing_stack->top, current_frame, 1, vm)) != 0)
            { goto fail; }
            if (new_object->type != OBJECT_TYPE_THREAD)
            {
                vm_err_update(vm->r, -VM_ERR_INVALID_OPERAND, \
                        "runtime error: invalid operand, type \'thread\' expected");
                ret = -MULTIPLE_ERR_VM;
                goto fail;
            }

            tid = ((struct virtual_machine_object_thread *)(new_object->ptr))->tid;

            switch (opcode)
            {
                case OP_TSUSPEND:
                    virtual_machine_thread_list_set_state_by_tid(vm, \
                            vm->threads, tid, VIRTUAL_MACHINE_THREAD_STATE_SUSPENDED);
                    break;
                case OP_TRESUME:
                    virtual_machine_thread_list_set_state_by_tid(vm, \
                            vm->threads, tid, VIRTUAL_MACHINE_THREAD_STATE_NORMAL);
                    break;
            }

            /* Update PC */
            current_thread->running_stack->top->pc++;

            break;

        case OP_TYIELD:

            /* Update PC */
            current_thread->running_stack->top->pc++;
            break;

        case OP_TSENDMSG:
        case OP_TRECVMSG:
        case OP_TISEMPTY:
            if ((ret = virtual_machine_thread_step_thread_message(vm)) != 0) goto fail;
            break;

        case OP_MTXMK:
        case OP_MTXLCK:
        case OP_MTXUNLCK:
            if ((ret = virtual_machine_thread_step_thread_mutex(vm)) != 0) goto fail;
            break;

        case OP_SEMMK:
        case OP_SEMP:
        case OP_SEMV:
            if ((ret = virtual_machine_thread_step_thread_semaphore(vm)) != 0) goto fail;
            break;
    }
    ret = 0;
    goto done;
fail:
    if (new_object != NULL) _virtual_machine_object_destroy(vm, new_object);
done:
    return ret;
}

