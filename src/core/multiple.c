/* Multiple All In One
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
#include <stdio.h>
#include <string.h>

#include "multiple.h"
#include "multiple_tunnel.h"
#include "multiple_ir.h"
#include "multiple_asm.h"
#include "multiple_bytecode.h"
#include "multiple_err.h"
#include "multiple_misc.h"

#include "multiple_frontend.h"

#include "vm.h"
#include "vm_infrastructure.h"
#include "vm_types.h"
#include "vm_object.h"
#include "vm_object_aio.h"
#include "vm_err.h"

static int update_icode_pathname(struct multiple_ir *icode, char *pathname, size_t pathname_len)
{
    if (icode->filename != NULL)
    {
        free(icode->filename);
        icode->filename = NULL;
    }

    icode->filename = (char *)malloc(sizeof(char) * (pathname_len + 1));
    if (icode->filename == NULL) return -MULTIPLE_ERR_MALLOC;

    memcpy(icode->filename, pathname, pathname_len);
    icode->filename[pathname_len] = '\0';
    icode->filename_len = pathname_len;

    return 0;
}

int multiple_ir_update_icode_source_code(struct multiple_ir *icode, char *code, size_t code_len)
{
    if (icode == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (icode->source_section == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (icode->source_section->data != NULL)
    {
        free(icode->source_section->data);
        icode->source_section->data = NULL;
    }

    icode->source_section->data = (char *)malloc(sizeof(char) * (code_len + 1));
    if (icode->source_section->data == NULL) return -MULTIPLE_ERR_MALLOC;

    memcpy(icode->source_section->data, code, code_len);
    icode->source_section->data[code_len] = '\0';
    icode->source_section->size = code_len;

    return 0;
}

int multiple_frontend_list_register(struct multiple_error *err, struct multiple_frontend_list *frontend_list, \
        const char *frontend_name, \
        const char *frontend_full_name, \
        const char *frontend_ext_name, \
        int (*create)(struct multiple_error *err, void **stub_out, \
            char *pathname_dst, int type_dst, \
            char *pathname_src, int type_src), \
        int (*destroy)(void *stub), \
        int (*debug_info_set)(void *stub, int debug_info), \
        int (*optimize_set)(void *stub, int debug_info), \
        int (*tokens_print)(struct multiple_error *err, void *stub), \
        int (*reconstruct)(struct multiple_error *err, struct multiple_ir **multiple_ir, void *stub), \
        int (*icodegen)(struct multiple_error *err, struct multiple_ir **multiple_ir, void *stub))
{
    int ret = 0;
    struct multiple_frontend *new_frontend = NULL;

    if (frontend_list == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }
    if ((new_frontend = (struct multiple_frontend *)malloc(sizeof(struct multiple_frontend))) == NULL) 
    {
        MULTIPLE_ERROR_MALLOC();
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }
    if (strlen(frontend_name) >= FRONTEND_NAME_MAX)
    {
        multiple_error_update(err, -MULTIPLE_ERR_STUB, "error: frontend name exceed the length limit");
        ret = -MULTIPLE_ERR_STUB;
        goto fail;
    }
    if (strlen(frontend_full_name) >= FRONTEND_FULLNAME_MAX)
    {
        multiple_error_update(err, -MULTIPLE_ERR_STUB, "error: frontend full name exceed the length limit");
        ret = -MULTIPLE_ERR_STUB;
        goto fail;
    }
    if (strlen(frontend_ext_name) >= FRONTEND_EXT_MAX)
    {
        multiple_error_update(err, -MULTIPLE_ERR_STUB, "error: frontend ext name exceed the length limit");
        ret = -MULTIPLE_ERR_STUB;
        goto fail;
    }
    strncpy(new_frontend->frontend_name, frontend_name, FRONTEND_NAME_MAX);
    strncpy(new_frontend->frontend_full_name, frontend_full_name, FRONTEND_FULLNAME_MAX);
    strncpy(new_frontend->frontend_ext_name, frontend_ext_name, FRONTEND_EXT_MAX);
    new_frontend->create = create;
    new_frontend->destroy = destroy;
    new_frontend->debug_info_set = debug_info_set;
    new_frontend->optimize_set = optimize_set;
    new_frontend->tokens_print = tokens_print;
    new_frontend->reconstruct = reconstruct;
    new_frontend->irgen = icodegen;
    new_frontend->next = NULL;

    if (frontend_list->begin == NULL)
    {
        frontend_list->begin = frontend_list->end = new_frontend;
    }
    else
    {
        frontend_list->end->next = new_frontend;
        frontend_list->end = new_frontend;
    }
    frontend_list->size += 1;

    goto done;
fail:
    if (new_frontend != NULL)
    {
        free(new_frontend);
    }
done:
    return ret;
}

int multiple_frontend_list_initialize(struct multiple_error *err, struct multiple_frontend_list *frontend_list)
{
    int ret = 0;

    if (frontend_list == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        ret = -MULTIPLE_ERR_NULL_PTR;
        goto fail;
    }

#include "multiple_register.h"

    ret = 0;
fail:
    return ret;
}

int multiple_stub_frontend_unregister(struct multiple_frontend *frontend)
{
    free(frontend);
    return 0;
}

struct multiple_frontend_list *multiple_frontend_list_new(void) 
{
    struct multiple_frontend_list *new_multiple_frontend_list;

    if ((new_multiple_frontend_list = (struct multiple_frontend_list *)malloc(sizeof(struct multiple_frontend_list))) == NULL)
    {
        return NULL;
    }
    new_multiple_frontend_list->begin = new_multiple_frontend_list->end = NULL;
    new_multiple_frontend_list->size = 0; 
    return new_multiple_frontend_list;
}

int multiple_frontend_list_destroy(struct multiple_frontend_list *frontend_list)
{
    struct multiple_frontend *cur_frontend = frontend_list->begin, *next_frontend = NULL;
    while (cur_frontend != NULL)
    {
        next_frontend = cur_frontend->next;
        multiple_stub_frontend_unregister(cur_frontend);
        cur_frontend = next_frontend;
    }
    free(frontend_list);
    return 0;
}

static int locate_ext(char **ext, size_t *ext_len, char *str, size_t size)
{
    char *str_p = str, *str_endp = str + size;

    *ext = NULL;
    *ext_len = 0;
    while (str_p != str_endp)
    {
        if (*str_p == '.')
        {
            *ext = str_p;
            *ext_len = 0;
        }
        str_p++;
        (*ext_len)++;
    }
    return 0;
}

static char *match_ext_find_semicolon(char *pattern, char *pattern_endp)
{
    char *pattern_p = pattern;

    for (;;)
    {
        if (pattern_p == pattern_endp)
        {
            return pattern_p;
        }
        else if (*pattern_p == ';')
        {
            return pattern_p;
        }
        pattern_p++;
    }
    return NULL;
}

static int match_ext(char *ext, size_t ext_len, char *pattern, size_t pattern_len)
{
    char *pattern_p = pattern, *pattern_endp = pattern_p + pattern_len;
    char *pattern_nextp;

    while (pattern_p != pattern_endp)
    {
        pattern_nextp = match_ext_find_semicolon(pattern_p, pattern_endp);
        if (pattern_nextp == NULL) return 0;
        if (pattern_nextp - pattern_p > 0)
        {
            if (((size_t)(pattern_nextp - pattern_p) == ext_len) && \
                    (strncmp(pattern_p, ext, ext_len) == 0))
            { return 1; }
        }
        if (pattern_nextp == pattern_endp) break;
        pattern_p = pattern_nextp + 1;
    }
    return 0;
}

/* Implements of stub functions */
int multiple_stub_create(struct multiple_error *err, \
        struct multiple_stub **stub_out, \
        char *pathname_dst, int type_dst, \
        char *pathname_src, int type_src, \
        char *frontend_name)
{
    int ret = 0;
    struct multiple_stub *new_stub = NULL;
    struct multiple_frontend *cur_frontend;

    size_t frontend_name_len;

    char *ext;
    size_t ext_len;

    size_t pathname_src_len;

    static const char *str_stdin = "stdin";
    static const char *str_str = "str";

    *stub_out = NULL;

    /* Detect pathname type */
    switch (type_src)
    {
        case MULTIPLE_IO_STDIN: 
            multiple_error_update_pathname(err, str_stdin);
            break;
        case MULTIPLE_IO_PATHNAME: 
            if (is_file_exists(pathname_src) == 0)
            {
                if (is_dir_exists(pathname_src) != 0)
                {
                    multiple_error_update(err, -MULTIPLE_ERR_STUB, "error: \'%s\' is a directory", pathname_src);
                    ret = -MULTIPLE_ERR_STUB;
                    goto fail;
                }
                else
                {
                    multiple_error_update(err, -MULTIPLE_ERR_STUB, "error: can not open \'%s\'", pathname_src);
                    ret = -MULTIPLE_ERR_STUB;
                    goto fail;
                }
            }
            multiple_error_update_pathname(err, pathname_src);
            break;
        case MULTIPLE_IO_STR: 
            multiple_error_update_pathname(err, str_str);
            break;
        default:
            multiple_error_update(err, -MULTIPLE_ERR_STUB, "error: unsupported source type");
            ret = -MULTIPLE_ERR_STUB;
            goto fail;
    }

    if (frontend_name == NULL)
    {
        locate_ext(&ext, &ext_len, pathname_src, strlen(pathname_src));
        if (ext == NULL)
        {
            multiple_error_update(err, -MULTIPLE_ERR_STUB, "error: no matched frontend for \'%s\'", pathname_src);
            ret = -MULTIPLE_ERR_STUB;
            goto fail;
        }
        frontend_name = ext + 1;
        frontend_name_len = ext_len - 1;
    }
    else
    {
        frontend_name_len = strlen(frontend_name);
    }

    if ((new_stub = (struct multiple_stub *)malloc(sizeof(struct multiple_stub))) == NULL)
    {
        MULTIPLE_ERROR_MALLOC();
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }
    new_stub->frontend = NULL;
    new_stub->frontend_list = NULL;
    new_stub->ir = NULL;
    new_stub->sub_stub = NULL;
    new_stub->vm = NULL;
    new_stub->optimize = 0;
    new_stub->external_functions = NULL;
    new_stub->pathname = NULL;

    /* Startup Info */
    virtual_machine_startup_init(&new_stub->startup);

    if ((new_stub->frontend_list = multiple_frontend_list_new()) == NULL)
    {
        MULTIPLE_ERROR_MALLOC();
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }
    if ((new_stub->external_functions = multiple_stub_function_list_new()) == NULL)
    {
        MULTIPLE_ERROR_MALLOC();
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }

    if ((ret = multiple_frontend_list_initialize(err, new_stub->frontend_list)) != 0)
    {
        goto fail;
    }

    if (new_stub->frontend_list->size != 0)
    {
        cur_frontend = new_stub->frontend_list->begin;
        while (cur_frontend != NULL)
        {
            if (match_ext(frontend_name, frontend_name_len, \
                        cur_frontend->frontend_ext_name, strlen(cur_frontend->frontend_ext_name)) != 0)
            {
                /* Matched */
                new_stub->frontend = cur_frontend;
                goto finish;
            }
            cur_frontend = cur_frontend->next;
        }
        multiple_error_update(err, -MULTIPLE_ERR_STUB, "error: no matched frontend for %s", pathname_src);
        ret = -MULTIPLE_ERR_STUB;
        goto fail;
    }
    else
    {
        multiple_error_update(err, -MULTIPLE_ERR_STUB, "error: no frontend available");
        ret = -MULTIPLE_ERR_STUB;
        goto fail;
    }

finish:

    pathname_src_len = strlen(pathname_src);
    new_stub->pathname = (char *)malloc(sizeof(char) * (pathname_src_len + 1));
    if (new_stub->pathname == NULL)
    {
        MULTIPLE_ERROR_MALLOC();
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }
    memcpy(new_stub->pathname, pathname_src, pathname_src_len);
    new_stub->pathname[pathname_src_len] = '\0';
    new_stub->pathname_len = pathname_src_len;

    if ((ret = new_stub->frontend->create(err, (void *)&(new_stub->sub_stub), \
                    pathname_dst, type_dst, \
                    pathname_src, type_src)) != 0)
    { goto fail; }

    if (pathname_dst == NULL)
    {
        new_stub->fp_out = stdout;
    }
    else
    {
        new_stub->fp_out = fopen(pathname_dst, "wb+");
        if (new_stub->fp_out == NULL)
        {
            multiple_error_update(err, -MULTIPLE_ERR_STUB, "error: can not open file %s for writing");
            ret = -MULTIPLE_ERR_STUB;
            goto fail;
        }
    }

    *stub_out = new_stub;
    ret = 0;
    goto done;
fail:
    /* TODO : UNREGISTER All frontends */
    if (new_stub != NULL)
    {
        if (new_stub->pathname != NULL) free(new_stub->pathname);
        if (new_stub->external_functions != NULL) multiple_stub_function_list_destroy(new_stub->external_functions);
        if (new_stub->frontend_list != NULL) multiple_frontend_list_destroy(new_stub->frontend_list);
        free(new_stub);
    }
done:

    return ret;
}

int multiple_stub_destroy(struct multiple_stub *stub)
{
    if (stub == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (stub->sub_stub != NULL) stub->frontend->destroy(stub->sub_stub);
    if (stub->vm != NULL) virtual_machine_destroy(stub->vm, 0);
    if (stub->ir != NULL) multiple_ir_destroy(stub->ir);
    if (stub->frontend_list != NULL) multiple_frontend_list_destroy(stub->frontend_list);
    if (stub->external_functions != NULL) multiple_stub_function_list_destroy(stub->external_functions);
    if (stub->pathname != NULL) free(stub->pathname);
    free(stub);

    return 0;
}

int multiple_stub_debug_info_set(struct multiple_error *err, struct multiple_stub *stub, int debug_info)
{
    int ret = 0;

    if (stub == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }

    stub->debug_info = debug_info;
    ret = stub->frontend->debug_info_set(stub->sub_stub, debug_info);

    return ret;
}

int multiple_stub_optimize_set(struct multiple_error *err, struct multiple_stub *stub, int optimize)
{
    int ret = 0;

    if (stub == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }

    stub->optimize = optimize;
    ret = stub->frontend->optimize_set(stub->sub_stub, optimize);

    return ret;
}

int multiple_stub_tokens_print(struct multiple_error *err, struct multiple_stub *stub)
{
    int ret = 0;

    if (stub == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }

    if (stub->sub_stub != NULL) 
    {
        ret = stub->frontend->tokens_print(err, stub->sub_stub);
    }

    return ret;
}

int multiple_stub_icg(struct multiple_error *err, struct multiple_stub *stub)
{
    int ret = 0;

    if (stub == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }

    if (stub->sub_stub != NULL) 
    {
        ret = stub->frontend->irgen(err, &stub->ir, stub->sub_stub);
        if (ret != 0) return ret;
    }
    update_icode_pathname(stub->ir, stub->pathname, stub->pathname_len);

    return ret;
}

int multiple_stub_reconstruct(struct multiple_error *err, struct multiple_stub *stub)
{
    int ret = 0;

    if (stub == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }

    if (stub->sub_stub != NULL) 
    {
        ret = stub->frontend->reconstruct(err, &stub->ir, stub->sub_stub);
    }

    return ret;
}

int multiple_stub_asm_code_gen(struct multiple_error *err, struct multiple_stub *stub)
{
    int ret;

    if (stub == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }

    /* dependence */
    if (stub->ir == NULL)
    { if ((ret = stub->frontend->irgen(err, &stub->ir, stub->sub_stub)) != 0) return ret; }

    if (stub->ir == NULL)
    {
        multiple_error_update(err, -MULTIPLE_ERR_STUB, "error: icode generation not been fully implemented");
        return -MULTIPLE_ERR_STUB;
    }

    update_icode_pathname(stub->ir, stub->pathname, stub->pathname_len);
    /* work */
    if ((ret = multiple_asm_code_gen(err, stub->fp_out, stub->ir)) != 0) return ret;

    return ret;
}

int multiple_stub_bytecode(struct multiple_error *err, struct multiple_stub *stub)
{
    int ret;

    if (stub == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }

    /* dependence */
    if (stub->ir == NULL)
    { if ((ret = stub->frontend->irgen(err, &stub->ir, stub->sub_stub)) != 0) return ret; }

    if (stub->ir == NULL)
    {
        multiple_error_update(err, -MULTIPLE_ERR_STUB, "error: icode generation not been fully implemented");
        return -MULTIPLE_ERR_STUB;
    }

    update_icode_pathname(stub->ir, stub->pathname, stub->pathname_len);
    /* work */
    if ((ret = multiple_bytecode_gen(err, stub->fp_out, stub->ir)) != 0) return ret;

    return ret;
}

int multiple_stub_run(struct multiple_error *err, struct vm_err *r, struct multiple_stub *stub)
{
    int ret;

    if (stub == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }

    /* dependence */
    if (stub->ir == NULL)
    { if ((ret = stub->frontend->irgen(err, &stub->ir, stub->sub_stub)) != 0) return ret; }
    if (stub->ir == NULL)
    {
        multiple_error_update(err, -MULTIPLE_ERR_STUB, "error: icode generation not been fully implemented");
        return -MULTIPLE_ERR_STUB;
    }

    update_icode_pathname(stub->ir, stub->pathname, stub->pathname_len);

    /* run in vm */
    if ((ret = vm_run(err, r, stub->ir, &stub->startup, 0, stub->external_functions)) != 0) return ret;

    return ret;
}

int multiple_stub_debug(struct multiple_error *err, struct vm_err *r, struct multiple_stub *stub)
{
    int ret;

    if (stub == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }

    /* dependence */
    if (stub->ir == NULL)
    { if ((ret = stub->frontend->irgen(err, &stub->ir, stub->sub_stub)) != 0) return ret; }
    if (stub->ir == NULL)
    {
        multiple_error_update(err, -MULTIPLE_ERR_STUB, "error: icode generation not been fully implemented");
        return -MULTIPLE_ERR_STUB;
    }

    update_icode_pathname(stub->ir, stub->pathname, stub->pathname_len);
    /* debug in vm */
    if ((ret = vm_run(err, r, stub->ir, &stub->startup, 1, stub->external_functions)) != 0) return ret;

    return ret;
}

int multiple_stub_completion(struct multiple_error *err, struct multiple_stub *stub, const char *completion_cmd)
{
    int ret;

    struct vm_err r_local;

    vm_err_init(&r_local);

    if (stub == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }

    /* dependence */
    if (stub->ir == NULL)
    { if ((ret = stub->frontend->irgen(err, &stub->ir, stub->sub_stub)) != 0) return ret; }
    if (stub->ir == NULL)
    {
        multiple_error_update(err, -MULTIPLE_ERR_STUB, "error: icode generation not been fully implemented");
        return -MULTIPLE_ERR_STUB;
    }

    update_icode_pathname(stub->ir, stub->pathname, stub->pathname_len);
    if ((ret = virtual_machine_completion(err, &r_local, stub->ir, &stub->startup, 1, stub->external_functions, completion_cmd)) != 0) return ret;

    return ret;
}

int multiple_stub_list_frontends(struct multiple_error *err)
{
    int ret = 0;
    struct multiple_frontend_list *new_frontend_list = NULL;
    struct multiple_frontend *frontend_cur;

    if ((new_frontend_list = multiple_frontend_list_new()) == NULL)
    {
        MULTIPLE_ERROR_MALLOC();
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }

    if ((ret = multiple_frontend_list_initialize(err, new_frontend_list)) != 0)
    { goto fail; }

    frontend_cur = new_frontend_list->begin;
    while (frontend_cur != NULL)
    {
        printf("%s (%s)\n", frontend_cur->frontend_name, frontend_cur->frontend_full_name);
        frontend_cur = frontend_cur->next; 
    }

    goto done;
fail:
done:
    if (new_frontend_list != NULL)
    {
        multiple_frontend_list_destroy(new_frontend_list);
    }
    return ret;
}

int multiple_stub_virtual_machine_memory_usage(struct multiple_error *err, struct multiple_stub *stub, \
        const char *mem_item, const char *mem_type, const char *mem_size)
{
    if (virtual_machine_startup_memory_usage(&stub->startup, mem_item, mem_type, mem_size) != 0)
    {
        multiple_error_update(err, -MULTIPLE_ERR_STUB, "error: invalid startup parameter");
        return -MULTIPLE_ERR_STUB;
    }

    return 0;
}

int multiple_stub_register(struct multiple_error *err, \
        struct multiple_stub *stub, \
        char *func_name, \
        int (*func)(struct multiple_stub_function_args *args))
{
    int ret = 0;
    struct multiple_stub_function *new_function;

    if ((new_function = multiple_stub_function_new_with_value(stub->vm, func_name, func)) == NULL)
    {
        MULTIPLE_ERROR_MALLOC();
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }
    if ((ret = multiple_stub_function_list_append(stub->external_functions, new_function)) != 0)
    {
        goto fail;
    }
    goto done;
fail:
    if (new_function != NULL)
    {
        multiple_stub_function_destroy(new_function);
    }
done:
    return ret;
}

int multiple_stub_args_get_int(struct multiple_stub_function_args *args, int *ptr)
{
    int ret = 0;
    char *bad_type_name;
    struct virtual_machine_object *object_solved_arg = NULL;  

    if (args->frame->computing_stack->size == 0)
    {
        vm_err_update(args->rail, -VM_ERR_COMPUTING_STACK_EMPTY, \
                "runtime error: computing stack empty");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    object_solved_arg = (struct virtual_machine_object *)args->frame->computing_stack->top;
    if ((ret = virtual_machine_variable_solve(&object_solved_arg, \
                    (struct virtual_machine_object *)args->frame->computing_stack->top, \
                    NULL, \
                    1, \
                    args->vm)) != 0)
    { goto fail; }
    if (vm_err_occurred(args->rail) != 0)
    { goto fail; }

    if (object_solved_arg->type != OBJECT_TYPE_INT)
    {
        ret = virtual_machine_object_id_to_type_name(&bad_type_name, NULL, object_solved_arg->type);
        vm_err_update(args->rail, -VM_ERR_INVALID_OPERAND_TYPE, \
                "runtime error: invalid operand type, 'int' is expected, but '%s' is given", \
                ret == 0 ? bad_type_name : "undefined type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    /* Pick out value */
    *ptr = ((struct virtual_machine_object_int *)(object_solved_arg->ptr))->value;
    /* Pop argument */
    virtual_machine_computing_stack_pop(args->vm, args->frame->computing_stack);

    ret = 0;
    goto done;
fail:
done:
    if (object_solved_arg != NULL) virtual_machine_object_destroy(args->vm, object_solved_arg);
    return ret;
}

int multiple_stub_args_get_float(struct multiple_stub_function_args *args, double *ptr)
{
    int ret = 0;
    char *bad_type_name;
    struct virtual_machine_object *object_solved_arg = NULL;  

    if (args->frame->computing_stack->size == 0)
    {
        vm_err_update(args->rail, -VM_ERR_COMPUTING_STACK_EMPTY, \
                "runtime error: computing stack empty");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    object_solved_arg = (struct virtual_machine_object *)args->frame->computing_stack->top;
    if ((ret = virtual_machine_variable_solve(&object_solved_arg, (struct virtual_machine_object *)args->frame->computing_stack->top, NULL, 1, args->vm)) != 0)
    { goto fail; }
    if (vm_err_occurred(args->rail) != 0)
    { goto fail; }

    if (object_solved_arg->type != OBJECT_TYPE_FLOAT)
    {
        ret = virtual_machine_object_id_to_type_name(&bad_type_name, NULL, object_solved_arg->type);
        vm_err_update(args->rail, -VM_ERR_INVALID_OPERAND_TYPE, \
                "runtime error: invalid operand type, 'float' is expected, but '%s' is given", \
                ret == 0 ? bad_type_name : "undefined type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    /* Pick out value */
    *ptr = ((struct virtual_machine_object_float *)(object_solved_arg->ptr))->value;
    /* Pop argument */
    virtual_machine_computing_stack_pop(args->vm, args->frame->computing_stack);

    ret = 0;
    goto done;
fail:
done:
    if (object_solved_arg != NULL) virtual_machine_object_destroy(args->vm, object_solved_arg);
    return ret;
}

int multiple_stub_args_get_str(struct multiple_stub_function_args *args, char **str_p, size_t *str_len)
{
    int ret = 0;
    char *bad_type_name;
    struct virtual_machine_object *object_solved_arg = NULL; 
    char *object_solved_arg_str;
    size_t object_solved_arg_str_len;

    if (args->frame->computing_stack->size == 0)
    {
        vm_err_update(args->rail, -VM_ERR_COMPUTING_STACK_EMPTY, \
                "runtime error: computing stack empty");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    object_solved_arg = (struct virtual_machine_object *)args->frame->computing_stack->top;
    if ((ret = virtual_machine_variable_solve(&object_solved_arg, (struct virtual_machine_object *)args->frame->computing_stack->top, NULL, 1, args->vm)) != 0)
    { goto fail; }
    if (vm_err_occurred(args->rail) != 0)
    { goto fail; }

    if (object_solved_arg->type != OBJECT_TYPE_STR)
    {
        ret = virtual_machine_object_id_to_type_name(&bad_type_name, NULL, object_solved_arg->type);
        vm_err_update(args->rail, -VM_ERR_INVALID_OPERAND_TYPE, \
                "runtime error: invalid operand type, 'str' is expected, but '%s' is given", \
                ret == 0 ? bad_type_name : "undefined type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    /* Pick out value */
    virtual_machine_object_str_extract(&object_solved_arg_str, &object_solved_arg_str_len, object_solved_arg);

    *str_p = (char *)malloc(sizeof(char) * (object_solved_arg_str_len + 1));
    if (*str_p == NULL)
    {
        vm_err_update(args->rail, -VM_ERR_MALLOC, \
                "runtime error: out of memory while creating string");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    memcpy(*str_p, object_solved_arg_str, object_solved_arg_str_len);
    *str_len = object_solved_arg_str_len;
    (*str_p)[*str_len] = '\0';
    /* Pop argument */
    virtual_machine_computing_stack_pop(args->vm, args->frame->computing_stack);

    ret = 0;
    goto done;
fail:
done:
    if (object_solved_arg != NULL) virtual_machine_object_destroy(args->vm, object_solved_arg);
    return ret;
}

int multiple_stub_args_get_bool(struct multiple_stub_function_args *args, int *ptr)
{
    int ret = 0;
    char *bad_type_name;
    struct virtual_machine_object *object_solved_arg = NULL;  

    if (args->frame->computing_stack->size == 0)
    {
        vm_err_update(args->rail, -VM_ERR_COMPUTING_STACK_EMPTY, \
                "runtime error: computing stack empty");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    object_solved_arg = (struct virtual_machine_object *)args->frame->computing_stack->top;
    if ((ret = virtual_machine_variable_solve(&object_solved_arg, (struct virtual_machine_object *)args->frame->computing_stack->top, NULL, 1, args->vm)) != 0)
    { goto fail; }
    if (vm_err_occurred(args->rail) != 0)
    { goto fail; }

    if (object_solved_arg->type != OBJECT_TYPE_BOOL)
    {
        ret = virtual_machine_object_id_to_type_name(&bad_type_name, NULL, object_solved_arg->type);
        vm_err_update(args->rail, -VM_ERR_INVALID_OPERAND_TYPE, \
                "runtime error: invalid operand type, 'bool' is expected, but '%s' is given", \
                ret == 0 ? bad_type_name : "undefined type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    /* Pick out value */
    *ptr = ((struct virtual_machine_object_bool *)(object_solved_arg->ptr))->value;
    /* Pop argument */
    virtual_machine_computing_stack_pop(args->vm, args->frame->computing_stack);

    ret = 0;
    goto done;
fail:
done:
    if (object_solved_arg != NULL) virtual_machine_object_destroy(args->vm, object_solved_arg);
    return ret;
}

int multiple_stub_args_get_none(struct multiple_stub_function_args *args)
{
    int ret = 0;
    char *bad_type_name;
    struct virtual_machine_object *object_solved_arg = NULL;  

    if (args->frame->computing_stack->size == 0)
    {
        vm_err_update(args->rail, -VM_ERR_COMPUTING_STACK_EMPTY, \
                "runtime error: computing stack empty");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    object_solved_arg = (struct virtual_machine_object *)args->frame->computing_stack->top;
    if ((ret = virtual_machine_variable_solve(&object_solved_arg, (struct virtual_machine_object *)args->frame->computing_stack->top, NULL, 1, args->vm)) != 0)
    { goto fail; }
    if (vm_err_occurred(args->rail) != 0) 
    { goto fail; }

    if (object_solved_arg->type != OBJECT_TYPE_NONE)
    {
        ret = virtual_machine_object_id_to_type_name(&bad_type_name, NULL, object_solved_arg->type);
        vm_err_update(args->rail, -VM_ERR_INVALID_OPERAND_TYPE, \
                "runtime error: invalid operand type, 'none' is expected, but '%s' is given", \
                ret == 0 ? bad_type_name : "undefined type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    /* Pop argument */
    virtual_machine_computing_stack_pop(args->vm, args->frame->computing_stack);

    ret = 0;
    goto done;
fail:
done:
    if (object_solved_arg != NULL) virtual_machine_object_destroy(args->vm, object_solved_arg);
    return ret;
}

int multiple_stub_args_get_raw(struct multiple_stub_function_args *args, const char *name, const size_t len, void **ptr)
{
    int ret = 0;
    char *bad_type_name;
    struct virtual_machine_object *object_solved_arg = NULL;  
    struct virtual_machine_object_raw *object_raw = NULL;  
    struct virtual_machine_object_raw *new_ptr = NULL;

    if (args->frame->computing_stack->size == 0)
    {
        vm_err_update(args->rail, -VM_ERR_COMPUTING_STACK_EMPTY, \
                "runtime error: computing stack empty");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    object_solved_arg = (struct virtual_machine_object *)args->frame->computing_stack->top;
    if ((ret = virtual_machine_variable_solve(&object_solved_arg, (struct virtual_machine_object *)args->frame->computing_stack->top, NULL, 1, args->vm)) != 0)
    { goto fail; }
    if (vm_err_occurred(args->rail) != 0) 
    { goto fail; }

    if (object_solved_arg->type != OBJECT_TYPE_RAW)
    {
        ret = virtual_machine_object_id_to_type_name(&bad_type_name, NULL, object_solved_arg->type);
        vm_err_update(args->rail, -VM_ERR_INVALID_OPERAND_TYPE, \
                "runtime error: invalid operand type, 'raw' is expected, but '%s' is given", \
                ret == 0 ? bad_type_name : "undefined type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    object_raw = object_solved_arg->ptr;
    if ((object_raw->len != len) || strncmp(object_raw->name, name, len) != 0)
    {
        multiple_stub_error(args, -VM_ERR_INVALID_OPERAND_TYPE, "runtime error: invalid operand type, '%s' is expected, but '%s' is given", \
                name, object_raw->name);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    /* Clone a raw object (if will be freed after pop) */
    new_ptr = object_raw->func_clone(object_raw->ptr);
    if (new_ptr == NULL)
    {
        multiple_stub_error_malloc(args);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    *ptr = new_ptr; new_ptr = NULL;

    /* Pop argument */
    virtual_machine_computing_stack_pop(args->vm, args->frame->computing_stack);

    ret = 0;
    goto done;
fail:
    if (new_ptr != NULL) 
    {
        if (object_raw != NULL) object_raw->func_clone(new_ptr);
    }
done:
    if (object_solved_arg != NULL) virtual_machine_object_destroy(args->vm, object_solved_arg);
    return ret;
}

int multiple_stub_args_get_any(struct multiple_stub_function_args *args, char **name, size_t *len, void **ptr)
{
    int ret = 0;
    struct virtual_machine_object *object_solved_arg = NULL;  
    struct virtual_machine_object *new_ptr = NULL;
    char *data_type_name = NULL;
    size_t data_type_name_len = 0;

    if (args->frame->computing_stack->size == 0)
    {
        vm_err_update(args->rail, -VM_ERR_COMPUTING_STACK_EMPTY, \
                "runtime error: computing stack empty");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    if ((ret = virtual_machine_variable_solve(&object_solved_arg, (struct virtual_machine_object *)args->frame->computing_stack->top, NULL, 1, args->vm)) != 0)
    { goto fail; }
    if (vm_err_occurred(args->rail) != 0) 
    { goto fail; }

    if ((virtual_machine_object_id_to_type_name(&data_type_name, &data_type_name_len, object_solved_arg->type)) != 0)
    {
        vm_err_update(args->rail, -VM_ERR_COMPUTING_STACK_EMPTY, \
                "runtime error: invalid type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    /* Clone a raw object (if will be freed after pop) */
    new_ptr = object_solved_arg; object_solved_arg = NULL;
    *ptr = new_ptr; new_ptr = NULL;
    *name = data_type_name;
    *len = data_type_name_len;

    /* Pop argument */
    virtual_machine_computing_stack_pop(args->vm, args->frame->computing_stack);

    ret = 0;
    goto done;
fail:
    if (new_ptr != NULL) virtual_machine_object_destroy(args->vm, new_ptr);
    if (data_type_name != NULL) free(data_type_name);
done:
    if (object_solved_arg != NULL) virtual_machine_object_destroy(args->vm, object_solved_arg);
    return ret;
}

int multiple_stub_args_get_semaphore(struct multiple_stub_function_args *args, uint32_t *semaphore_id)
{
    int ret = 0;
    char *bad_type_name;
    struct virtual_machine_object *object_solved_arg = NULL; 
    uint32_t id;

    if (args->frame->computing_stack->size == 0)
    {
        vm_err_update(args->rail, -VM_ERR_COMPUTING_STACK_EMPTY, \
                "runtime error: computing stack empty");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    object_solved_arg = (struct virtual_machine_object *)args->frame->computing_stack->top;
    if ((ret = virtual_machine_variable_solve(&object_solved_arg, (struct virtual_machine_object *)args->frame->computing_stack->top, NULL, 1, args->vm)) != 0)
    { goto fail; }
    if (vm_err_occurred(args->rail) != 0)
    { goto fail; }

    if (object_solved_arg->type != OBJECT_TYPE_SEMAPHORE)
    {
        ret = virtual_machine_object_id_to_type_name(&bad_type_name, NULL, object_solved_arg->type);
        vm_err_update(args->rail, -VM_ERR_INVALID_OPERAND_TYPE, \
                \
                "runtime error: invalid operand type, 'semaphore' is expected, but '%s' is given", \
                ret == 0 ? bad_type_name : "undefined type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    id = virtual_machine_object_semaphore_get_id(object_solved_arg);

    *semaphore_id = id;

    /* Pop argument */
    virtual_machine_computing_stack_pop(args->vm, args->frame->computing_stack);

    ret = 0;
    goto done;
fail:
done:
    if (object_solved_arg != NULL) virtual_machine_object_destroy(args->vm, object_solved_arg);
    return ret;
}


int multiple_stub_return_int(struct multiple_stub_function_args *args, int value)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    if (args->return_value != NULL) 
    {
        virtual_machine_object_destroy(args->vm, args->return_value);
        args->return_value = NULL;
    }
    if ((new_object = virtual_machine_object_int_new_with_value(args->vm, value)) == NULL)
    {
        multiple_stub_error_malloc(args);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    args->return_value = new_object;

    ret = 0;
    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(args->vm, new_object);
done:
    return ret;
}

int multiple_stub_return_float(struct multiple_stub_function_args *args, double value)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    if (args->return_value != NULL) 
    {
        virtual_machine_object_destroy(args->vm, args->return_value);
        args->return_value = NULL;
    }
    if ((new_object = virtual_machine_object_float_new_with_value(args->vm, value)) == NULL)
    {
        multiple_stub_error_malloc(args);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    args->return_value = new_object;

    ret = 0;
    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(args->vm, new_object);
done:
    return ret;
}

int multiple_stub_return_str(struct multiple_stub_function_args *args, char *str_p, size_t str_len)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    if (args->return_value != NULL) 
    {
        virtual_machine_object_destroy(args->vm, args->return_value);
        args->return_value = NULL;
    }
    if ((new_object = virtual_machine_object_str_new_with_value(args->vm, str_p, str_len)) == NULL)
    {
        multiple_stub_error_malloc(args);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    args->return_value = new_object;

    ret = 0;
    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(args->vm, new_object);
done:
    return ret;
}

int multiple_stub_return_bool(struct multiple_stub_function_args *args, int value)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    if (args->return_value != NULL) 
    {
        virtual_machine_object_destroy(args->vm, args->return_value);
        args->return_value = NULL;
    }
    if ((new_object = virtual_machine_object_bool_new_with_value( \
                    args->vm, \
                    value)) == NULL)
    {
        multiple_stub_error_malloc(args);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    args->return_value = new_object;

    ret = 0;
    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(args->vm, new_object);
done:
    return ret;
}

int multiple_stub_return_none(struct multiple_stub_function_args *args)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;

    if (args->return_value != NULL) 
    {
        virtual_machine_object_destroy(args->vm, args->return_value);
        args->return_value = NULL;
    }
    if ((new_object = virtual_machine_object_none_new(args->vm)) == NULL)
    {
        multiple_stub_error_malloc(args);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    args->return_value = new_object;

    ret = 0;
    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(args->vm, new_object);
done:
    return ret;
}


static int __virtual_machine_data_type_list_lookup(struct virtual_machine_data_type **data_type_out, struct virtual_machine_data_type_list *list, const char *name, const size_t len);
int multiple_stub_return_raw(struct multiple_stub_function_args *args, const char *name, const size_t len, void *ptr)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_raw *new_object_raw = NULL;
    struct virtual_machine_data_type *data_type_target;

    if (args->return_value != NULL) 
    {
        virtual_machine_object_destroy(args->vm, args->return_value);
        args->return_value = NULL;
    }
    if ((__virtual_machine_data_type_list_lookup(&data_type_target, args->vm->data_types, name, len)) != 0)
    {
        vm_err_update(args->rail, -VM_ERR_DEFAULT, \
                "runtime error: invalid data type \'%s\'", name);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    if ((new_object = virtual_machine_object_raw_new_with_value(args->vm, name, len, ptr)) == NULL)
    {
        multiple_stub_error_malloc(args);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
    new_object_raw = (struct virtual_machine_object_raw *)new_object->ptr;
    new_object_raw->func_clone = data_type_target->func_clone;
    new_object_raw->func_destroy = data_type_target->func_destroy;
    new_object_raw->func_print = data_type_target->func_print;

    args->return_value = new_object;

    ret = 0;
    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(args->vm, new_object);
done:
    return ret;
}

/* External Event */
int multiple_stub_external_event_arg_push_int(struct virtual_machine *vm, const uint32_t id, int value)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_external_event *external_event_target = NULL;

    /* Lock */
    thread_mutex_lock(&vm->external_events->lock);

    if ((new_object = virtual_machine_object_int_new_with_value(vm, value)) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        thread_mutex_unlock(&vm->external_events->lock);
        goto fail;
    }

    external_event_target = virtual_machine_external_event_list_lookup(vm, vm->external_events, id);
    if (external_event_target == NULL)
    {
        vm_err_update(vm->r, -VM_ERR_DEFAULT, \
                "runtime error: external event id \'%u\' not installed", (unsigned int)id);
        ret = -MULTIPLE_ERR_VM;
        thread_mutex_unlock(&vm->external_events->lock);
        goto fail;
    }

    virtual_machine_computing_stack_push(external_event_target->args, new_object);
    new_object = NULL;

    /* Unlock */
    thread_mutex_unlock(&vm->external_events->lock);

    ret = 0;
    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
done:
    return ret;
}

int multiple_stub_external_event_arg_push_raw(struct virtual_machine *vm, \
        const uint32_t id, const char *name, const size_t len, void *ptr)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_raw *new_object_raw = NULL;
    struct virtual_machine_data_type *data_type_target;
    struct virtual_machine_external_event *external_event_target = NULL;

    /* Lock */
    thread_mutex_lock(&vm->external_events->lock);

    if ((__virtual_machine_data_type_list_lookup(&data_type_target, vm->data_types, name, len)) != 0)
    {
        vm_err_update(vm->r, -VM_ERR_DEFAULT, \
                "runtime error: invalid data type \'%s\'", name);
        ret = -MULTIPLE_ERR_VM;
        thread_mutex_unlock(&vm->external_events->lock);
        goto fail;
    }

    if ((new_object = virtual_machine_object_raw_new_with_value(vm, name, len, ptr)) == NULL)
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        thread_mutex_unlock(&vm->external_events->lock);
        goto fail;
    }
    new_object_raw = (struct virtual_machine_object_raw *)new_object->ptr;
    new_object_raw->func_clone = data_type_target->func_clone;
    new_object_raw->func_destroy = data_type_target->func_destroy;
    new_object_raw->func_print = data_type_target->func_print;

    external_event_target = virtual_machine_external_event_list_lookup(vm, vm->external_events, id);
    if (external_event_target == NULL)
    {
        vm_err_update(vm->r, -VM_ERR_DEFAULT, \
                "runtime error: external event id \'%u\' not installed", (unsigned int)id);
        ret = -MULTIPLE_ERR_VM;
        thread_mutex_unlock(&vm->external_events->lock);
        goto fail;
    }

    virtual_machine_computing_stack_push(external_event_target->args, new_object);
    new_object = NULL;

    /* Unlock */
    thread_mutex_unlock(&vm->external_events->lock);

    ret = 0;
    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
done:
    return ret;
}

int multiple_stub_external_event_raise(struct virtual_machine *vm, const uint32_t id)
{
    int ret = 0;

    /* Lock */
    thread_mutex_lock(&vm->external_events->lock);

    if (virtual_machine_external_event_list_raise( \
                vm, \
                vm->external_events, id) != 0)
    {
        vm_err_update(vm->r, -VM_ERR_DEFAULT, \
                "runtime error: external event id \'%u\' not installed", (unsigned int)id);
        ret = -MULTIPLE_ERR_VM;
        thread_mutex_unlock(&vm->external_events->lock);
        goto fail;
    }

    /* Unlock */
    thread_mutex_unlock(&vm->external_events->lock);

fail:
    return ret;
}

int multiple_stub_external_event_args_count(struct virtual_machine *vm, const uint32_t id, size_t *count)
{
    int ret = 0;

    /* Lock */
    thread_mutex_lock(&vm->external_events->lock);

    if (virtual_machine_external_event_list_count( \
                vm, \
                vm->external_events, id, 
                count) != 0)
    {
        vm_err_update(vm->r, -VM_ERR_DEFAULT, \
                "runtime error: external event id \'%u\' not installed", (unsigned int)id);
        ret = -MULTIPLE_ERR_VM;
        thread_mutex_unlock(&vm->external_events->lock);
        goto fail;
    }

    /* Unlock */
    thread_mutex_unlock(&vm->external_events->lock);

fail:
    return ret;
}


/* Synchronization */
int virtual_machine_semaphore_p(struct virtual_machine *vm, uint32_t semaphore_id)
{
    virtual_machine_semaphore_list_p( \
            vm, \
            vm->semaphores, \
            semaphore_id);

    return 0;
}

int virtual_machine_semaphore_v(struct virtual_machine *vm, uint32_t semaphore_id)
{
    virtual_machine_semaphore_list_v( \
            vm, \
            vm->semaphores, \
            semaphore_id);

    return 0;
}

int virtual_machine_semaphore_value(struct virtual_machine *vm, uint32_t semaphore_id)
{
    return virtual_machine_semaphore_list_value( \
            vm->semaphores, \
            semaphore_id);
}


/* Error Reporting */
int multiple_stub_error(struct multiple_stub_function_args *args, int number, const char *fmt, ...)
{
    va_list vargs;


    if (args->rail->number_enabled == 0)
    {
        if (number != 0)
        {
            args->rail->number = number;
            args->rail->number_enabled = 1;
            args->rail->occurred = 1;
        }
    }

    if (args->rail->description_enabled == 0)
    {
        if (fmt != NULL)
        {
            va_start(vargs, fmt);
            vsnprintf(args->rail->description, VM_ERR_DESCRIPTION_LEN, fmt, vargs);
            va_end(vargs);
            args->rail->description_enabled = 1;
            args->rail->occurred = 1;
        }
    }

    return 0;
}

/* Data native type */
int multiple_stub_data_register(struct multiple_stub_function_args *args, const char *name, const size_t len)
{
    int ret = 0;
    struct virtual_machine_data_type *new_data_type = NULL;

    if ((args->rail == NULL) || (args == NULL) || (name == NULL))
    {
        multiple_stub_error_null_ptr(args);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    if ((new_data_type = virtual_machine_data_type_new(args->vm, name, len)) == NULL)
    { 
        multiple_stub_error_malloc(args);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    virtual_machine_data_type_set_native(new_data_type, 1);

    if (virtual_machine_data_type_list_append(args->vm->data_types, new_data_type) != 0)
    {
        vm_err_update(args->rail, -VM_ERR_INTERNAL, \
                "runtime error: failed to append new data type");
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    new_data_type = NULL;
fail:
    if (new_data_type != NULL) virtual_machine_data_type_destroy(args->vm, new_data_type);

    return ret;
}

static int __virtual_machine_data_type_list_lookup(struct virtual_machine_data_type **data_type_out, struct virtual_machine_data_type_list *list, const char *name, const size_t len)
{
    struct virtual_machine_data_type *data_type_cur;

    data_type_cur = list->begin;
    while (data_type_cur != NULL)
    {
        if (data_type_cur->name_len == len)
        {
            if (strncmp(data_type_cur->name, name, len) == 0)
            {
                if (data_type_out != NULL) *data_type_out = data_type_cur;
                return 0;
            }
        }
        data_type_cur = data_type_cur->next;
    }
    if (data_type_out != NULL) *data_type_out = NULL;
    return -1;
}


int multiple_stub_data_is_registered(struct multiple_stub_function_args *args, const char *name, const size_t len)
{
    if ((__virtual_machine_data_type_list_lookup(NULL, args->vm->data_types, name, len)) != 0)
    { return 0; }

    return 1;
}

#define MULTIPLE_STUB_DATA_CALLBACK_FUNC_SET_DESTROY 0
#define MULTIPLE_STUB_DATA_CALLBACK_FUNC_SET_CLONE 1
#define MULTIPLE_STUB_DATA_CALLBACK_FUNC_SET_PRINT 2
#define MULTIPLE_STUB_DATA_CALLBACK_FUNC_SET_EQ 3

static int multiple_stub_data_callback_func_set(struct multiple_stub_function_args *args, const char *name, const size_t len, \
        int func_type, void *ptr)
{
    int ret = 0;
    struct virtual_machine_data_type *data_type_target;

    if ((args->rail == NULL) || (args == NULL) || (name == NULL))
    {
        multiple_stub_error_null_ptr(args);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    if ((__virtual_machine_data_type_list_lookup(&data_type_target, args->vm->data_types, name, len)) != 0)
    {
        vm_err_update(args->rail, -VM_ERR_DEFAULT, \
                "runtime error: invalid data type \'%s\'", name);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    switch (func_type)
    {
        case MULTIPLE_STUB_DATA_CALLBACK_FUNC_SET_DESTROY:
            data_type_target->func_destroy = (int (*)(const void *))ptr;
            break;
        case MULTIPLE_STUB_DATA_CALLBACK_FUNC_SET_CLONE:
            data_type_target->func_clone = (void *(*)(const void *))ptr;
            break;
        case MULTIPLE_STUB_DATA_CALLBACK_FUNC_SET_PRINT:
            data_type_target->func_print = (int (*)(const void *))ptr;
            break;
        case MULTIPLE_STUB_DATA_CALLBACK_FUNC_SET_EQ:
            data_type_target->func_eq = (int (*)(const void *, const void *))ptr;
            break;
        default:
            multiple_stub_error_internal(args);
            ret = -MULTIPLE_ERR_VM;
            break;
    }

fail:
    return ret;
}

int multiple_stub_data_callback_destroy_set(struct multiple_stub_function_args *args, const char *name, const size_t len, \
        int (*func_destroy)(const void *ptr))
{
    return multiple_stub_data_callback_func_set(args, name, len, MULTIPLE_STUB_DATA_CALLBACK_FUNC_SET_DESTROY, func_destroy);
}

int multiple_stub_data_callback_clone_set(struct multiple_stub_function_args *args, const char *name, const size_t len, \
        void *(*func_clone)(const void *ptr))
{
    return multiple_stub_data_callback_func_set(args, name, len, MULTIPLE_STUB_DATA_CALLBACK_FUNC_SET_CLONE, func_clone);
}

int multiple_stub_data_callback_print_set(struct multiple_stub_function_args *args, const char *name, const size_t len, \
        int (*func_print)(const void *ptr))
{
    return multiple_stub_data_callback_func_set(args, name, len, MULTIPLE_STUB_DATA_CALLBACK_FUNC_SET_PRINT, func_print);
}

int multiple_stub_data_callback_eq_set(struct multiple_stub_function_args *args, const char *name, const size_t len, \
        int (*func_eq)(const void *ptr_left, const void *ptr_right))
{
    return multiple_stub_data_callback_func_set(args, name, len, MULTIPLE_STUB_DATA_CALLBACK_FUNC_SET_EQ, func_eq);
}

