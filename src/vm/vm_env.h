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

#ifndef _VM_ENV_H_
#define _VM_ENV_H_

/* Data Structure */

#include "vm_infrastructure.h"

/* Kernel Record (for cloning the 'real' part of environment) */

struct environment_entrance_kernel_record_item
{
    struct virtual_machine_environment_stack_frame *environment_stack_frame_src;
    struct virtual_machine_environment_stack_frame *environment_stack_frame_cloned;

    struct environment_entrance_kernel_record_item *next;
};

struct environment_entrance_kernel_record *record_kernels_from_environment_stack( \
        struct virtual_machine *vm, \
        struct virtual_machine_environment_stack *environment_stack);

struct virtual_machine_environment_stack *virtual_machine_environment_stack_new_from_running_stack( \
        struct virtual_machine *vm, \
        struct virtual_machine_running_stack *running_stack);

int environment_entrance_kernel_record_destroy( \
        struct virtual_machine *vm, \
        struct environment_entrance_kernel_record *environment_entrance_kernel_record);

struct environment_entrance_kernel_record_item *environment_entrance_kernel_record_lookup( \
        struct environment_entrance_kernel_record *environment_entrance_kernel_record, \
        struct virtual_machine_environment_stack_frame *environment_stack_frame_src);


/* Continuation List (For tracing continuations and updating the turning point) */

struct continuation_list_item
{
    struct virtual_machine_object_func_internal *object_func_internal;

    struct continuation_list_item *prev;
    struct continuation_list_item *next;
};
struct continuation_list_item *continuation_list_item_new( \
        struct virtual_machine *vm, \
        struct virtual_machine_object_func_internal *object_func_internal);
int continuation_list_item_destroy( \
        struct virtual_machine *vm, \
        struct continuation_list_item *item);

struct continuation_list
{
    struct continuation_list_item *begin;
    struct continuation_list_item *end;
};
struct continuation_list *continuation_list_new( \
        struct virtual_machine *vm);
int continuation_list_clear( \
        struct virtual_machine *vm, \
        struct continuation_list *list);
int continuation_list_destroy( \
        struct virtual_machine *vm, \
        struct continuation_list *list);
int continuation_list_append( \
        struct continuation_list *list, \
        struct continuation_list_item *new_item);
int continuation_list_remove( \
        struct virtual_machine *vm, \
        struct continuation_list *list, \
        struct continuation_list_item *item_target);
int continuation_list_update( \
        struct virtual_machine *vm, \
        struct continuation_list *list, \
        size_t running_stack_size);

#endif

