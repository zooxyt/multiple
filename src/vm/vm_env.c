/* Virtual Machine : Environment
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "vm_infrastructure.h"
#include "vm_object.h"
#include "vm_object_aio.h"
#include "vm_env.h"
#include "vm_err.h"

static struct environment_entrance_kernel_record_item *environment_entrance_kernel_record_item_new( \
        struct virtual_machine *vm, \
        struct virtual_machine_environment_stack_frame *environment_stack_frame_src)
{
    struct environment_entrance_kernel_record_item *new_environment_entrance_kernel_record_item = NULL;

    new_environment_entrance_kernel_record_item = (struct environment_entrance_kernel_record_item *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct environment_entrance_kernel_record_item));
    if (new_environment_entrance_kernel_record_item == NULL) { goto fail; }
    new_environment_entrance_kernel_record_item->environment_stack_frame_src = environment_stack_frame_src;
    new_environment_entrance_kernel_record_item->environment_stack_frame_cloned = NULL;
    new_environment_entrance_kernel_record_item->next = NULL;

    goto done;
fail:
    if (new_environment_entrance_kernel_record_item != NULL)
    {
        virtual_machine_resource_free(vm->resource, new_environment_entrance_kernel_record_item);
        new_environment_entrance_kernel_record_item = NULL;
    }
done:
    return new_environment_entrance_kernel_record_item;
}

static int environment_entrance_kernel_record_item_destroy( \
        struct virtual_machine *vm, \
        struct environment_entrance_kernel_record_item *environment_entrance_kernel_record_item)
{
    /* src part can't be freed */

    /* cloned part can be freed */
    if (environment_entrance_kernel_record_item->environment_stack_frame_cloned != NULL)
    {
        virtual_machine_environment_stack_frame_destroy( \
                vm, \
                environment_entrance_kernel_record_item->environment_stack_frame_cloned);
    }
    virtual_machine_resource_free(vm->resource, environment_entrance_kernel_record_item);

    return 0;
}

struct environment_entrance_kernel_record
{
    struct environment_entrance_kernel_record_item *begin;
    struct environment_entrance_kernel_record_item *end;
    size_t size;
};

static struct environment_entrance_kernel_record *environment_entrance_kernel_record_new( \
        struct virtual_machine *vm)
{
    struct environment_entrance_kernel_record *new_environment_entrance_kernel_record = NULL;

    new_environment_entrance_kernel_record = (struct environment_entrance_kernel_record *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct environment_entrance_kernel_record));
    if (new_environment_entrance_kernel_record == NULL) { goto fail; }
    new_environment_entrance_kernel_record->begin = NULL;
    new_environment_entrance_kernel_record->end = NULL;
    new_environment_entrance_kernel_record->size = 0;

    goto done;
fail:
    if (new_environment_entrance_kernel_record != NULL)
    {
        virtual_machine_resource_free(vm->resource, new_environment_entrance_kernel_record);
        new_environment_entrance_kernel_record = NULL;
    }
done:
    return new_environment_entrance_kernel_record;
}

int environment_entrance_kernel_record_destroy( \
        struct virtual_machine *vm, \
        struct environment_entrance_kernel_record *environment_entrance_kernel_record)
{
    struct environment_entrance_kernel_record_item *environment_entrance_kernel_record_item_cur;
    struct environment_entrance_kernel_record_item *environment_entrance_kernel_record_item_next;

    environment_entrance_kernel_record_item_cur = environment_entrance_kernel_record->begin;
    while (environment_entrance_kernel_record_item_cur != NULL)
    {
        environment_entrance_kernel_record_item_next = environment_entrance_kernel_record_item_cur->next;
        environment_entrance_kernel_record_item_destroy(vm, environment_entrance_kernel_record_item_cur);
        environment_entrance_kernel_record_item_cur = environment_entrance_kernel_record_item_next;
    }
    virtual_machine_resource_free(vm->resource, environment_entrance_kernel_record);

    return 0;
}

static int environment_entrance_kernel_record_append( \
        struct environment_entrance_kernel_record *environment_entrance_kernel_record, \
        struct environment_entrance_kernel_record_item *new_environment_entrance_kernel_record_item)
{
    if (environment_entrance_kernel_record->begin == NULL)
    {
        environment_entrance_kernel_record->begin = new_environment_entrance_kernel_record_item;
        environment_entrance_kernel_record->end = new_environment_entrance_kernel_record_item;
    }
    else
    {
        environment_entrance_kernel_record->end->next = new_environment_entrance_kernel_record_item;
        environment_entrance_kernel_record->end = new_environment_entrance_kernel_record_item;
    }
    environment_entrance_kernel_record->size += 1;

    return 0;
}

static int environment_entrance_kernel_record_exist( \
        struct environment_entrance_kernel_record *environment_entrance_kernel_record, \
        struct virtual_machine_environment_stack_frame *environment_stack_frame_src)
{
    struct environment_entrance_kernel_record_item *environment_entrance_kernel_record_item_cur;

    environment_entrance_kernel_record_item_cur = environment_entrance_kernel_record->begin;
    while (environment_entrance_kernel_record_item_cur != NULL)
    {
        if (environment_entrance_kernel_record_item_cur->environment_stack_frame_src == \
                environment_stack_frame_src)
        { return 1; }
        environment_entrance_kernel_record_item_cur = environment_entrance_kernel_record_item_cur->next;
    }
    return 0;
}

struct environment_entrance_kernel_record_item *environment_entrance_kernel_record_lookup( \
        struct environment_entrance_kernel_record *environment_entrance_kernel_record, \
        struct virtual_machine_environment_stack_frame *environment_stack_frame_src)
{
    struct environment_entrance_kernel_record_item *environment_entrance_kernel_record_item_cur;

    environment_entrance_kernel_record_item_cur = environment_entrance_kernel_record->begin;
    while (environment_entrance_kernel_record_item_cur != NULL)
    {
        if (environment_entrance_kernel_record_item_cur->environment_stack_frame_src == \
                environment_stack_frame_src)
        { return environment_entrance_kernel_record_item_cur; }
        environment_entrance_kernel_record_item_cur = environment_entrance_kernel_record_item_cur->next;
    }
    return NULL;
}

static int environment_entrance_kernel_record_append_configure_unique( \
        struct virtual_machine *vm, \
        struct environment_entrance_kernel_record *environment_entrance_kernel_record, \
        struct virtual_machine_environment_stack_frame *environment_stack_frame_src)
{
    int ret = 0;
    struct environment_entrance_kernel_record_item *new_environment_entrance_kernel_record_item = NULL;

    if (environment_entrance_kernel_record_exist( \
                environment_entrance_kernel_record, \
                environment_stack_frame_src) == 0)
    {
        new_environment_entrance_kernel_record_item = environment_entrance_kernel_record_item_new( \
                vm, \
                environment_stack_frame_src);
        if (new_environment_entrance_kernel_record_item == NULL)
        {
            VM_ERR_MALLOC(vm->r);
            ret = -MULTIPLE_ERR_VM;
            goto fail;
        }
        environment_entrance_kernel_record_append( \
                environment_entrance_kernel_record, \
                new_environment_entrance_kernel_record_item);
        new_environment_entrance_kernel_record_item = NULL;
    }

    goto done;
fail:
    if (new_environment_entrance_kernel_record_item != NULL)
    {
        environment_entrance_kernel_record_item_destroy( \
                vm, \
                new_environment_entrance_kernel_record_item);
    }
done:
    return ret;
}


/* Procedures of dealing with snapshot */

static struct environment_entrance_kernel_record *record_kernels_from_running_stack( \
        struct virtual_machine *vm, \
        struct virtual_machine_running_stack *running_stack)
{
    struct environment_entrance_kernel_record *new_environment_entrance_kernel_record = NULL;
    struct virtual_machine_running_stack_frame *running_stack_frame_cur;
    struct virtual_machine_object_environment_entrance *environment_entrance;
    struct virtual_machine_object_environment_entrance_internal *environment_entrance_internal;
    struct virtual_machine_environment_stack_frame *environment_stack_frame;
    struct environment_entrance_kernel_record_item *new_environment_entrance_kernel_record_item = NULL;
    struct environment_entrance_kernel_record_item *environment_entrance_kernel_record_item_cur;
    struct virtual_machine_environment_stack_frame *new_environment_stack_frame = NULL;
    struct environment_entrance_kernel_record_item *environment_entrance_kernel_record_item_target = NULL;

    /* Create Record List */
    new_environment_entrance_kernel_record = environment_entrance_kernel_record_new(vm);
    if (new_environment_entrance_kernel_record == NULL) { goto fail; }

    /* Record Source Items */
    running_stack_frame_cur = running_stack->top;
    while (running_stack_frame_cur != NULL)
    {
        environment_entrance = running_stack_frame_cur->environment_entrance->ptr;
        environment_entrance_internal = environment_entrance->ptr_internal; 

        if ((environment_entrance_kernel_record_append_configure_unique( \
                        vm, \
                        new_environment_entrance_kernel_record, \
                        environment_entrance_internal->entrance)) != 0)
        { goto fail; }

        running_stack_frame_cur = running_stack_frame_cur->prev; 
    }

    /* Create Clones of Items */
    environment_entrance_kernel_record_item_cur = new_environment_entrance_kernel_record->begin;
    while (environment_entrance_kernel_record_item_cur != NULL)
    {
        if (environment_entrance_kernel_record_item_cur->environment_stack_frame_cloned == NULL)
        {
            /* Not been cloned */

            /* Extract current frame */
            environment_stack_frame = environment_entrance_kernel_record_item_cur->environment_stack_frame_src;

            /* Clone the environment stack frame */
            new_environment_stack_frame = virtual_machine_environment_stack_frame_new_with_configure( \
                    vm, \
                    environment_stack_frame->variables, \
                    environment_stack_frame->computing_stack, \
                    environment_stack_frame->environment_entrance, \
                    environment_stack_frame->pc, \
                    environment_stack_frame->module, \
                    environment_stack_frame->args_count, \
                    environment_stack_frame->closure, \
                    environment_stack_frame->trap_pc, \
                    environment_stack_frame->trap_enabled);
            if (new_environment_stack_frame == NULL) { goto fail; }

            environment_entrance_kernel_record_item_cur->environment_stack_frame_cloned = new_environment_stack_frame;
            new_environment_stack_frame = NULL;
        }
        environment_entrance_kernel_record_item_cur = environment_entrance_kernel_record_item_cur->next;
    }

    /* Connect Items */
    environment_entrance_kernel_record_item_cur = new_environment_entrance_kernel_record->begin;
    while (environment_entrance_kernel_record_item_cur != NULL)
    {
        if (environment_entrance_kernel_record_item_cur->environment_stack_frame_cloned == NULL)
        {
            environment_entrance_kernel_record_item_target = environment_entrance_kernel_record_lookup( \
                    new_environment_entrance_kernel_record, \
                    environment_entrance_kernel_record_item_cur->environment_stack_frame_src);
            if (environment_entrance_kernel_record_item_target == NULL)
            {
                environment_entrance_kernel_record_item_cur->environment_stack_frame_cloned->prev = NULL;
            }
            else
            {
                environment_entrance_kernel_record_item_cur-> \
                    environment_stack_frame_cloned->prev = environment_entrance_kernel_record_item_target->environment_stack_frame_cloned;
            }

        }
        environment_entrance_kernel_record_item_cur = environment_entrance_kernel_record_item_cur->next;
    }


    goto done;
fail:
    if (new_environment_stack_frame != NULL)
    {
        virtual_machine_environment_stack_frame_destroy(vm, new_environment_stack_frame);
    }
    if (new_environment_entrance_kernel_record_item != NULL)
    {
        environment_entrance_kernel_record_item_destroy(vm, new_environment_entrance_kernel_record_item);
    }
    if (new_environment_entrance_kernel_record != NULL)
    {
        environment_entrance_kernel_record_destroy(vm, new_environment_entrance_kernel_record);
        new_environment_entrance_kernel_record = NULL;
    }
done:
    return new_environment_entrance_kernel_record;
}

struct environment_entrance_kernel_record *record_kernels_from_environment_stack( \
        struct virtual_machine *vm, \
        struct virtual_machine_environment_stack *environment_stack)
{
    struct environment_entrance_kernel_record *new_environment_entrance_kernel_record = NULL;
    struct virtual_machine_environment_stack_frame *environment_stack_frame_cur;
    struct virtual_machine_object_environment_entrance *environment_entrance;
    struct virtual_machine_object_environment_entrance_internal *environment_entrance_internal;
    struct virtual_machine_environment_stack_frame *environment_stack_frame;
    struct environment_entrance_kernel_record_item *new_environment_entrance_kernel_record_item = NULL;
    struct environment_entrance_kernel_record_item *environment_entrance_kernel_record_item_cur;
    struct virtual_machine_environment_stack_frame *new_environment_stack_frame = NULL;
    struct environment_entrance_kernel_record_item *environment_entrance_kernel_record_item_target = NULL;

    /* Create Record List */
    new_environment_entrance_kernel_record = environment_entrance_kernel_record_new(vm);
    if (new_environment_entrance_kernel_record == NULL) { goto fail; }

    /* Record Source Items */
    environment_stack_frame_cur = environment_stack->begin;
    while (environment_stack_frame_cur != NULL)
    {
        environment_entrance = environment_stack_frame_cur->environment_entrance->ptr;
        environment_entrance_internal = environment_entrance->ptr_internal; 

        if ((environment_entrance_kernel_record_append_configure_unique( \
                        vm, \
                        new_environment_entrance_kernel_record, \
                        environment_entrance_internal->entrance)) != 0)
        { goto fail; }

        environment_stack_frame_cur = environment_stack_frame_cur->next;
    }

    /* Create Clones of Items */
    environment_entrance_kernel_record_item_cur = new_environment_entrance_kernel_record->begin;
    while (environment_entrance_kernel_record_item_cur != NULL)
    {
        if (environment_entrance_kernel_record_item_cur->environment_stack_frame_cloned == NULL)
        {
            /* Not been cloned */

            /* Extract current frame */
            environment_stack_frame = environment_entrance_kernel_record_item_cur->environment_stack_frame_src;

            /* Clone the environment stack frame */
            new_environment_stack_frame = virtual_machine_environment_stack_frame_new_with_configure( \
                    vm, \
                    environment_stack_frame->variables, \
                    environment_stack_frame->computing_stack, \
                    environment_stack_frame->environment_entrance, \
                    environment_stack_frame->pc, \
                    environment_stack_frame->module, \
                    environment_stack_frame->args_count, \
                    environment_stack_frame->closure, \
                    environment_stack_frame->trap_pc, \
                    environment_stack_frame->trap_enabled);
            if (new_environment_stack_frame == NULL) { goto fail; }

            environment_entrance_kernel_record_item_cur->environment_stack_frame_cloned = new_environment_stack_frame;
            new_environment_stack_frame = NULL;
        }
        environment_entrance_kernel_record_item_cur = environment_entrance_kernel_record_item_cur->next;
    }

    /* Connect Items */
    environment_entrance_kernel_record_item_cur = new_environment_entrance_kernel_record->begin;
    while (environment_entrance_kernel_record_item_cur != NULL)
    {
        if (environment_entrance_kernel_record_item_cur->environment_stack_frame_cloned == NULL)
        {
            environment_entrance_kernel_record_item_target = environment_entrance_kernel_record_lookup( \
                    new_environment_entrance_kernel_record, \
                    environment_entrance_kernel_record_item_cur->environment_stack_frame_src);
            if (environment_entrance_kernel_record_item_target == NULL)
            {
                environment_entrance_kernel_record_item_cur->environment_stack_frame_cloned->prev = NULL;
            }
            else
            {
                environment_entrance_kernel_record_item_cur-> \
                    environment_stack_frame_cloned->prev = environment_entrance_kernel_record_item_target->environment_stack_frame_cloned;
            }

        }
        environment_entrance_kernel_record_item_cur = environment_entrance_kernel_record_item_cur->next;
    }


    goto done;
fail:
    if (new_environment_stack_frame != NULL)
    {
        virtual_machine_environment_stack_frame_destroy(vm, new_environment_stack_frame);
    }
    if (new_environment_entrance_kernel_record_item != NULL)
    {
        environment_entrance_kernel_record_item_destroy(vm, new_environment_entrance_kernel_record_item);
    }
    if (new_environment_entrance_kernel_record != NULL)
    {
        environment_entrance_kernel_record_destroy(vm, new_environment_entrance_kernel_record);
        new_environment_entrance_kernel_record = NULL;
    }
done:
    return new_environment_entrance_kernel_record;
}

struct virtual_machine_environment_stack *virtual_machine_environment_stack_new_from_running_stack( \
        struct virtual_machine *vm, \
        struct virtual_machine_running_stack *running_stack)
{
    struct environment_entrance_kernel_record *environment_entrance_kernel_record = NULL;
    struct virtual_machine_environment_stack *new_environment_stack = NULL;
    struct virtual_machine_environment_stack_frame *new_environment_stack_frame = NULL;
    struct virtual_machine_running_stack_frame *running_stack_frame_cur;
    struct virtual_machine_object *new_object_environment_entrance = NULL;
    struct environment_entrance_kernel_record_item *environment_entrance_kernel_record_item_target = NULL;
    struct virtual_machine_object_environment_entrance *environment_entrance;
    struct virtual_machine_object_environment_entrance_internal *environment_entrance_internal;

    new_environment_stack = virtual_machine_environment_stack_new(vm);
    if (new_environment_stack == NULL) { goto fail; }

    /* Record kernels */
    environment_entrance_kernel_record = record_kernels_from_running_stack( \
            vm, \
            running_stack);
    if (environment_entrance_kernel_record == NULL) { goto fail; }

    /* Walkthough the entire running stack */ 
    running_stack_frame_cur = running_stack->bottom;
    while (running_stack_frame_cur != NULL)
    {
        environment_entrance = running_stack_frame_cur->environment_entrance->ptr;
        environment_entrance_internal = environment_entrance->ptr_internal; 

        environment_entrance_kernel_record_item_target = environment_entrance_kernel_record_lookup( \
                environment_entrance_kernel_record, \
                environment_entrance_internal->entrance);
        if (environment_entrance_kernel_record_item_target == NULL) { goto fail; }

        /* Clone the kernel of the entrance */
        new_object_environment_entrance = virtual_machine_object_environment_entrance_new( \
                vm, \
                environment_entrance_kernel_record_item_target->environment_stack_frame_cloned);
        if (new_object_environment_entrance == NULL)
        { goto fail; }
        environment_entrance_kernel_record_item_target->environment_stack_frame_cloned = NULL;

        new_environment_stack_frame = virtual_machine_environment_stack_frame_new_with_configure( \
                vm, \
                running_stack_frame_cur->variables, \
                running_stack_frame_cur->computing_stack, \
                new_object_environment_entrance, \
                running_stack_frame_cur->pc, \
                running_stack_frame_cur->module, \
                running_stack_frame_cur->args_count, \
                running_stack_frame_cur->closure, \
                running_stack_frame_cur->trap_pc, \
                running_stack_frame_cur->trap_enabled);
        if (new_environment_stack_frame == NULL) { goto fail; }

        virtual_machine_object_destroy(vm, new_object_environment_entrance);
        new_object_environment_entrance = NULL;

        virtual_machine_environment_stack_append(new_environment_stack, new_environment_stack_frame);
        new_environment_stack_frame = NULL;
        running_stack_frame_cur = running_stack_frame_cur->next; 
    }
    goto done;
fail:
    if (new_environment_stack != NULL)
    {
        virtual_machine_environment_stack_destroy(vm, new_environment_stack);
        new_environment_stack = NULL;
    }
done:
    if (environment_entrance_kernel_record != NULL)
    {
        environment_entrance_kernel_record_destroy(vm, environment_entrance_kernel_record);
    }
    return new_environment_stack;
}


struct continuation_list_item *continuation_list_item_new( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_func_internal *object_func_internal)
{
    struct continuation_list_item *new_item = NULL;

    new_item = (struct continuation_list_item *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct continuation_list_item));
    if (new_item == NULL) { goto fail; }
    new_item->object_func_internal = object_func_internal;
    new_item->prev = NULL;
    new_item->next = NULL;

    goto done;
fail:
    if (new_item != NULL)
    {
        continuation_list_item_destroy(vm, new_item);
        new_item = NULL;
    }
done:
    return new_item;
}

int continuation_list_item_destroy( \
        struct virtual_machine *vm, \
        struct continuation_list_item *item)
{
    /* Inform the object kernel that not been traced any more */
    item->object_func_internal->continuation_tracing_item = NULL;
    /* Destroy the item */
    virtual_machine_resource_free(vm->resource, item);

    return 0;
}

struct continuation_list *continuation_list_new( \
        struct virtual_machine *vm)
{
    struct continuation_list *new_list = NULL;

    new_list = (struct continuation_list *)virtual_machine_resource_malloc( \
            vm->resource, sizeof(struct continuation_list));
    if (new_list == NULL) { goto fail; }
    new_list->begin = NULL;
    new_list->end = NULL;

    goto done;
fail:
    if (new_list != NULL)
    {
        continuation_list_destroy(vm, new_list);
        new_list = NULL;
    }
done:
    return new_list;
}

int continuation_list_clear( \
        struct virtual_machine *vm, \
        struct continuation_list *list)
{
    struct continuation_list_item *item_cur, *item_next;

    item_cur = list->begin;
    while (item_cur != NULL)
    {
        item_next = item_cur->next; 
        continuation_list_item_destroy(vm, item_cur);
        item_cur = item_next; 
    }
    list->begin = NULL;
    list->end = NULL;

    return 0;
}

int continuation_list_destroy( \
        struct virtual_machine *vm, \
        struct continuation_list *list)
{
    continuation_list_clear(vm, list);
    virtual_machine_resource_free(vm->resource, list);

    return 0;
}

int continuation_list_append( \
        struct continuation_list *list, \
        struct continuation_list_item *new_item)
{
    if (list->begin == NULL)
    {
        list->begin = list->end = new_item;
    }
    else
    {
        new_item->prev = list->end;
        list->end->next = new_item;
        list->end = new_item;
    }

    return 0;
}

int continuation_list_remove( \
        struct virtual_machine *vm, \
        struct continuation_list *list, \
        struct continuation_list_item *item_target)
{
    struct continuation_list_item *item_cur, *item_prev, *item_next;

    item_cur = list->begin;
    while (item_cur != NULL)
    {
        item_prev = item_cur->prev;
        item_next = item_cur->next;

        if (item_cur == item_target)
        {
            if (item_prev != NULL) { item_prev->next = item_next; }
            else { list->begin = item_next; }
            if (item_next != NULL) { item_next->prev = item_next; }
            else { list->end = item_prev; }

            continuation_list_item_destroy(vm, item_cur);

            return 0;
        }

        item_cur = item_next;
    }

    return -1;
}

int continuation_list_update( \
        struct virtual_machine *vm, \
        struct continuation_list *list, \
        size_t running_stack_size)
{
    struct continuation_list_item *item_cur;

    /* Update turning point */
    item_cur = list->begin;
    while (item_cur != NULL)
    {
        if (item_cur->object_func_internal->turning_point > running_stack_size)
        {
            item_cur->object_func_internal->turning_point = running_stack_size;
        }
        item_cur = item_cur->next; 
    }

    if (running_stack_size == 1)
    {
        /* Reached the bottom of the stack, 
         * clear all tracing record*/
        continuation_list_clear(vm, list);
    }

    return 0;
}

