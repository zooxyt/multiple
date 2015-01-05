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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "multiple_tunnel.h"
#include "vm_object.h"

#include "multiple_err.h"

struct multiple_stub_function_args *multiple_stub_function_args_new(void)
{
    struct multiple_stub_function_args *new_function_args = NULL;
    if ((new_function_args = (struct multiple_stub_function_args *)malloc(sizeof(struct multiple_stub_function_args))) == NULL)
    {
        goto fail;
    }
    new_function_args->frame = NULL;
    new_function_args->return_value = NULL;
    new_function_args->vm = NULL;
    new_function_args->args_count = 0;
    new_function_args->rail = NULL;

    return new_function_args;
fail:
    if (new_function_args != NULL)
    {
        free(new_function_args);
    }
    return NULL;
}

int multiple_stub_function_args_destroy(struct multiple_stub_function_args *function_args)
{
    if (function_args == NULL) 
    {
        return -MULTIPLE_ERR_NULL_PTR;
    }
    if (function_args->return_value != NULL)
    {
        virtual_machine_object_destroy(function_args->vm, function_args->return_value);
    }
    free(function_args);
    return 0;
}

struct multiple_stub_function *multiple_stub_function_new_with_value( \
        struct virtual_machine *vm, \
        char *func_name, \
        int (*func)(struct multiple_stub_function_args *args))
{
    struct multiple_stub_function *new_function = NULL;

    if ((func_name == NULL) || (func == NULL))
    {
        return NULL;
    }

    if ((new_function = (struct multiple_stub_function *)malloc(sizeof(struct multiple_stub_function))) == NULL)
    {
        return NULL;
    }
    new_function->name = NULL;
    new_function->name_len = 0;
    new_function->func = func;

    new_function->args = multiple_stub_function_args_new();
    new_function->args->vm = vm;
    new_function->next = NULL;

    new_function->name_len = strlen(func_name);
    if ((new_function->name = (char *)malloc(sizeof(char) * (new_function->name_len + 1))) == NULL)
    {
        goto fail;
    }
    memcpy(new_function->name, func_name, new_function->name_len);
    new_function->name[new_function->name_len] = '\0';

    return new_function;
fail:
    if (new_function != NULL)
    {
        if (new_function->args != NULL) multiple_stub_function_args_destroy(new_function->args);
        if (new_function->name != NULL) free(new_function->name);
        free(new_function);
    }
    return NULL;
}

int multiple_stub_function_destroy(struct multiple_stub_function *function)
{
    if (function == NULL)
    {
        return -MULTIPLE_ERR_NULL_PTR;
    }

    if (function->args != NULL) multiple_stub_function_args_destroy(function->args);
    if (function->name != NULL) free(function->name);
    free(function);

    return 0;
}

struct multiple_stub_function_list *multiple_stub_function_list_new(void)
{
    struct multiple_stub_function_list *new_list = NULL;

    if ((new_list = (struct multiple_stub_function_list *)malloc(sizeof(struct multiple_stub_function_list))) == NULL)
    {
        return new_list;
    }
    new_list->begin = new_list->end = NULL;
    new_list->size = 0;

    return new_list;
}

int multiple_stub_function_list_destroy(struct multiple_stub_function_list *list)
{
    struct multiple_stub_function *function_cur, *function_next;

    if (list == NULL) 
    {
        return -MULTIPLE_ERR_NULL_PTR; 
    }

    function_cur = list->begin;
    while (function_cur != NULL)
    {
        function_next = function_cur->next;
        multiple_stub_function_destroy(function_cur);
        function_cur = function_next;
    }

    free(list);
    return 0;
}

int multiple_stub_function_list_append(struct multiple_stub_function_list *list, struct multiple_stub_function *new_function)
{
    if ((list == NULL) || (new_function == NULL))
    {
        return -MULTIPLE_ERR_NULL_PTR;
    }

    if (list->begin == NULL)
    {
        list->begin = list->end = new_function;
    }
    else
    {
        list->end->next = new_function;
        list->end = new_function;
    }
    list->size += 1;

    return 0;
}

