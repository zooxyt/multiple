/* Virtual Machine : Garbage Collection
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

#include "selfcheck.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "multiple_ir.h"
#include "multiple_err.h"
#include "multiple_tunnel.h"
#include "vm_predef.h"
#include "vm.h"
#include "vm_startup.h"
#include "vm_gc.h"
#include "vm_res.h"
#include "vm_object.h"
#include "vm_object_aio.h"
#include "vm_err.h"

#include "gc.h"


/* Register Reference Type Object */

int virtual_machine_resource_reference_register( \
        gc_stub_t *gc_stub, \
        struct virtual_machine_object *object_src, \
        void *object_internal_ptr, \
        int (*internal_marker)(void *internal_object_ptr), \
        int (*internal_collector)(void *internal_object_ptr, int *confirm))
{
    int ret = 0;
    struct gc_object_table_item *target_object_table_item = NULL;

    if ((ret = gc_reference_register( \
            gc_stub, \
            object_internal_ptr, \
            internal_marker, \
            internal_collector, \
            &target_object_table_item)) != 0)
    { return ret; }

    object_src->object_table_item_ptr = target_object_table_item;

    return 0;
}

int virtual_machine_resource_reference_unregister( \
        gc_stub_t *gc_stub, \
        void *object_internal_ptr)
{
    return gc_resource_reference_unregister( \
            gc_stub, \
            object_internal_ptr);
}


/* Complete GC (Major GC) */

static int virtual_machine_marks_clear(struct virtual_machine *vm)
{
    return gc_begin(vm->gc_stub);
}

int virtual_machine_marks_object(struct virtual_machine_object *object, int type)
{
    gc_object_table_item_t *object_table_item = object->object_table_item_ptr;

    if (object_table_item == NULL) return 0;

    /*printf("Mark Object:");*/
    /*virtual_machine_object_print(object);*/
    /*printf(" at address %llX", (unsigned long long)object->object_table_item_ptr->internal_object_ptr);*/
    /*printf("\n");*/

    switch (type)
    {
        case VIRTUAL_MACHINE_GARBAGE_COLLECT_MAJOR:
            gc_mark_major(object_table_item);
            break;
        case VIRTUAL_MACHINE_GARBAGE_COLLECT_MINOR:
            gc_mark_minor(object_table_item);
            break;
    }

    return 0;
}

int virtual_machine_marks_variable_list(struct virtual_machine_variable_list *list, int type)
{
    struct virtual_machine_variable *variable_cur = list->begin;

    while (variable_cur != NULL)
    {
        virtual_machine_marks_object(variable_cur->ptr, type);

        variable_cur = variable_cur->next;
    }

    return 0;
}

int virtual_machine_marks_computing_stack(struct virtual_machine_computing_stack *computing_stack, int type)
{
    struct virtual_machine_object *object_cur;

    object_cur = computing_stack->bottom;
    while (object_cur != NULL)
    {
        virtual_machine_marks_object(object_cur, type);

        object_cur = object_cur->next; 
    }

    return 0;
}

int virtual_machine_marks_environment_stack_frame(struct virtual_machine_environment_stack_frame *environment_stack_frame, int type)
{
    if (environment_stack_frame->environment_entrance != NULL)
    { virtual_machine_marks_object(environment_stack_frame->environment_entrance, type); }
    if (environment_stack_frame->variables != NULL)
    { virtual_machine_marks_variable_list(environment_stack_frame->variables, type); }
    if (environment_stack_frame->computing_stack != NULL)
    { virtual_machine_marks_computing_stack(environment_stack_frame->computing_stack, type); }

    return 0;
}

int virtual_machine_marks_environment_stack(struct virtual_machine_environment_stack *environment, int type)
{
    struct virtual_machine_environment_stack_frame *environment_stack_frame_cur; 

    if (environment != NULL)
    {
        environment_stack_frame_cur = environment->begin; 
        while (environment_stack_frame_cur != NULL)
        {
            virtual_machine_marks_environment_stack_frame(environment_stack_frame_cur, type);

            environment_stack_frame_cur = environment_stack_frame_cur->next; 
        }
    }

    return 0;
}

int virtual_machine_marks_thread(struct virtual_machine_thread *thread, int type)
{
    struct virtual_machine_message *message_cur;
    struct virtual_machine_running_stack_frame *running_stack_frame_cur, *generator_running_stack_frame_cur;

    /* Messages */
    message_cur = thread->messages->begin;
    while (message_cur != NULL)
    {
        virtual_machine_marks_object(message_cur->object, type);
        message_cur = message_cur->next;
    }

    /* Stack Frames */
    running_stack_frame_cur = thread->running_stack->bottom;
    while (running_stack_frame_cur != NULL)
    {

        /* Variables and computing stack */
        virtual_machine_marks_variable_list(running_stack_frame_cur->variables, type);
        virtual_machine_marks_computing_stack(running_stack_frame_cur->computing_stack, type);
        virtual_machine_marks_computing_stack(running_stack_frame_cur->arguments, type);

        /* Environment */
        virtual_machine_marks_object(running_stack_frame_cur->environment_entrance, type);

        /* Generators */
        generator_running_stack_frame_cur = running_stack_frame_cur->generators->begin;
        while (generator_running_stack_frame_cur != NULL)
        {
            /* Variables and computing stack */
            virtual_machine_marks_variable_list(generator_running_stack_frame_cur->variables, \
                    type);
            virtual_machine_marks_computing_stack(generator_running_stack_frame_cur->computing_stack, \
                    type);
            virtual_machine_marks_computing_stack(generator_running_stack_frame_cur->arguments, \
                    type);
            if (generator_running_stack_frame_cur->environment_entrance != NULL)
            {
                virtual_machine_marks_object(generator_running_stack_frame_cur->environment_entrance, type);
            }

            generator_running_stack_frame_cur = generator_running_stack_frame_cur->next;
        }

        running_stack_frame_cur = running_stack_frame_cur->next;
    }

    return 0;
}

static int virtual_machine_garbage_collect_raw(struct virtual_machine *vm, int type)
{
    int ret = 0;

    struct virtual_machine_module *module_cur;
    struct virtual_machine_thread *thread_cur;

    struct virtual_machine_external_event *external_event_cur; 

    /* Clear marks */
    if ((ret = virtual_machine_marks_clear(vm)) != 0)
    { goto fail; }

    /* External Events */
    thread_mutex_lock(&vm->external_events->lock);
    external_event_cur = vm->external_events->begin;
    while (external_event_cur != NULL)
    {
        virtual_machine_marks_object(external_event_cur->object_callback, type);
        virtual_machine_marks_computing_stack(external_event_cur->args, type);
        external_event_cur = external_event_cur->next; 
    }
    thread_mutex_unlock(&vm->external_events->lock);

    /* Mark built-in and global variables */
    virtual_machine_marks_variable_list(vm->variables_builtin, type);
    virtual_machine_marks_variable_list(vm->variables_global, type);

    /* Modules */
    module_cur = vm->modules->begin;
    while (module_cur != NULL)
    {
        virtual_machine_marks_variable_list(module_cur->variables, type);

        module_cur = module_cur->next; 
    }

    /* Threads */
    thread_cur = vm->threads->begin;
    while (thread_cur != NULL)
    {
        virtual_machine_marks_thread(thread_cur, type);

        thread_cur = thread_cur->next;
    }

    goto done;
fail:
done:
    return ret;
}

static int virtual_machine_garbage_collect_minor_survivor(struct virtual_machine *vm)
{
    int ret = 0;
    struct gc_object_table_item *table_item_cur, *table_item_target;

    (void)vm;

    table_item_cur = vm->gc_stub->obj_tbl->eden->begin;
    while (table_item_cur != NULL)
    {
        if (table_item_cur->mark == 1)
        {
            if (table_item_cur->mark_count >= VIRTUAL_MACHINE_SURVIVOR_THRESHOLD)
            {
                table_item_target = table_item_cur;
                table_item_cur = table_item_cur->next;

                ret = gc_transport_to_survivor( \
                        vm->gc_stub, \
                        table_item_target);
                if (ret != 0) { goto fail; }
            }
        }
        else
        {
            /* Skip to next item only when current item not been collected */
            table_item_cur = table_item_cur->next;
        }
    }

fail:
    return ret;
}

int virtual_machine_garbage_collect(struct virtual_machine *vm)
{
    int ret = 0;

    /*
    static unsigned int gc_time = 0;
    printf("GC %u\n", gc_time++);
    */

    ret = virtual_machine_garbage_collect_raw(vm, VIRTUAL_MACHINE_GARBAGE_COLLECT_MAJOR);
    if (ret != 0) goto fail;

    /* Collect unmarked objects */
    gc_collect(vm->gc_stub);
fail:
    return ret;
}

int virtual_machine_garbage_collect_and_feedback(struct virtual_machine *vm)
{
    if (virtual_machine_resource_lack(vm->resource))
    {
        virtual_machine_garbage_collect(vm);
        virtual_machine_resource_feedback(vm->resource);
    }

    return 0;
}

int virtual_machine_garbage_collect_minor(struct virtual_machine *vm)
{
    int ret = 0;

    ret = virtual_machine_garbage_collect_raw(vm, VIRTUAL_MACHINE_GARBAGE_COLLECT_MINOR);
    if (ret != 0) goto fail;

    /* Lock */
    thread_mutex_lock(&vm->external_events->lock);

    /* Move aged objects from youth to survivors */
    virtual_machine_garbage_collect_minor_survivor(vm);

    /* Collect unmarked objects */
    gc_collect(vm->gc_stub);

    /* Unlock */
    thread_mutex_unlock(&vm->external_events->lock);

fail:
    return ret;
}

