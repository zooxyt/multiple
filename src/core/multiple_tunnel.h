/* Multiple Tunnel : Exchange Data with C
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

#ifndef _MULTIPLE_TUNNEL_H_
#define _MULTIPLE_TUNNEL_H_

#include <stdint.h>
#include <stdio.h>

#include "multiple_err.h"

struct vm_err;

struct multiple_stub_function_args
{
    struct virtual_machine_running_stack_frame *frame;

    struct virtual_machine_object *return_value;
    struct virtual_machine *vm; /* for solving id and register new data types */
    int runtime_return_value;
    size_t args_count;
    struct vm_err *rail;
};
struct multiple_stub_function_args *multiple_stub_function_args_new(void);
int multiple_stub_function_args_destroy(struct multiple_stub_function_args *function_args);

struct multiple_stub_function
{
    char *name;
    size_t name_len;

    int (*func)(struct multiple_stub_function_args *args);
    struct multiple_stub_function_args *args;

    struct multiple_stub_function *next;
};
struct multiple_stub_function *multiple_stub_function_new_with_value( \
        struct virtual_machine *vm, \
        char *func_name, \
        int (*func)(struct multiple_stub_function_args *args));
int multiple_stub_function_destroy(struct multiple_stub_function *function);

struct multiple_stub_function_list
{
    struct multiple_stub_function *begin;
    struct multiple_stub_function *end;
    size_t size;
};
struct multiple_stub_function_list *multiple_stub_function_list_new(void);
int multiple_stub_function_list_destroy(struct multiple_stub_function_list *list);
int multiple_stub_function_list_append(struct multiple_stub_function_list *list, struct multiple_stub_function *new_function);

#endif

