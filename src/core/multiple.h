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

#ifndef _MULTIPLE_H_
#define _MULTIPLE_H_

#include <stdio.h>

#include "multiple_ir.h"
#include "multiple_tunnel.h"
#include "multiple_err.h"

#include "vm_startup.h"

#define FRONTEND_NAME_MAX 16
#define FRONTEND_FULLNAME_MAX 256
#define FRONTEND_EXT_MAX 32

struct multiple_frontend
{
    char frontend_name[FRONTEND_NAME_MAX];
    char frontend_full_name[FRONTEND_FULLNAME_MAX];
    char frontend_ext_name[FRONTEND_EXT_MAX];
    int (*create)(struct multiple_error *err, void **stub_out, \
            char *pathname_dst, int type_dst, \
            char *pathname_src, int type_src);
    int (*destroy)(void *stub);
    int (*debug_info_set)(void *stub, int debug_info);
    int (*optimize_set)(void *stub, int optimize);
    int (*tokens_print)(struct multiple_error *err, void *stub);
    int (*reconstruct)(struct multiple_error *err, struct multiple_ir **multiple_ir, void *stub);
    int (*irgen)(struct multiple_error *err, struct multiple_ir **multiple_ir, void *stub);

    int (*completion)(struct multiple_error *err, const char *cmd);

    struct multiple_frontend *next;
};

struct multiple_frontend_list
{
    struct multiple_frontend *begin;
    struct multiple_frontend *end;
    size_t size;
};
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
        int (*irgen)(struct multiple_error *err, struct multiple_ir **multiple_ir, void *stub));

int multiple_stub_frontend_unregister(struct multiple_frontend *frontend);
struct multiple_frontend_list *multiple_frontend_list_new(void);
int multiple_frontend_list_destroy(struct multiple_frontend_list *frontend_list);
int multiple_frontend_list_initialize(struct multiple_error *err, struct multiple_frontend_list *frontend_list);

struct multiple_stub
{
    /* Internal */
    struct multiple_frontend_list *frontend_list;

    struct multiple_frontend *frontend;

    struct multiple_ir *ir;
    struct virtual_machine *vm;
    void *sub_stub;
    FILE *fp_out;

    /* Startup Info */
    struct virtual_machine_startup startup;

    /* External */
    struct multiple_stub_function_list *external_functions;

    /* Source name */
    char *pathname;
    size_t pathname_len;

    /* Options */
    int debug_info;
    int optimize;
};

struct vm_err;

#define MULTIPLE_IO_STDIN 0
#define MULTIPLE_IO_STDOUT 1
#define MULTIPLE_IO_STDERR 2
#define MULTIPLE_IO_PATHNAME 3
#define MULTIPLE_IO_STR 4
#define MULTIPLE_IO_NULL 5

int multiple_stub_create(struct multiple_error *err, struct multiple_stub **stub_out, \
        char *pathname_dst, int type_dst, \
        char *pathname_src, int type_src, \
        char *frontend_name);
int multiple_stub_destroy(struct multiple_stub *stub);
int multiple_stub_frontend_register(struct multiple_error *err, struct multiple_stub *stub, \
        char *frontend_name,
        int (*create)(void **stub_out, char *pathname_dst, char *pathname_src), \
        int (*destroy)(void *stub), \
        int (*debug_info_set)(void *stub, int debug_info), \
        int (*optimize_set)(void *stub, int debug_info), \
        int (*tokens_print)(void *stub), \
        int (*reconstruct)(struct multiple_ir **multiple_ir, void *stub), \
        int (*irgen)(struct multiple_ir **multiple_ir, void *stub));
int multiple_stub_debug_info_set(struct multiple_error *err, struct multiple_stub *stub, int debug_info);
int multiple_stub_optimize_set(struct multiple_error *err, struct multiple_stub *stub, int optimize);

int multiple_stub_tokens_print(struct multiple_error *err, struct multiple_stub *stub);
int multiple_stub_icg(struct multiple_error *err, struct multiple_stub *stub);
int multiple_stub_reconstruct(struct multiple_error *err, struct multiple_stub *stub);
int multiple_stub_asm_code_gen(struct multiple_error *err, struct multiple_stub *stub);
int multiple_stub_bytecode(struct multiple_error *err, struct multiple_stub *stub);
int multiple_stub_run(struct multiple_error *err, struct vm_err *r, struct multiple_stub *stub);
int multiple_stub_debug(struct multiple_error *err, struct vm_err *r, struct multiple_stub *stub);
int multiple_stub_completion(struct multiple_error *err, struct multiple_stub *stub, const char *completion_cmd);
int multiple_stub_list_frontends(struct multiple_error *err);

int multiple_stub_virtual_machine_memory_usage(struct multiple_error *err, struct multiple_stub *stub, \
        const char *mem_item, const char *mem_type, const char *mem_size);

int multiple_stub_register(struct multiple_error *err, \
        struct multiple_stub *stub, \
        char *func_name, \
        int (*func)(struct multiple_stub_function_args *args));

/* Data type */

/* Register native data type */
int multiple_stub_data_register(struct multiple_stub_function_args *args, const char *name, const size_t len);

/* Judge if native data type has been registered: 1 for yes, 0 for not yet */
int multiple_stub_data_is_registered(struct multiple_stub_function_args *args, const char *name, const size_t len);

/* Set Call-back functions */
int multiple_stub_data_callback_destroy_set(struct multiple_stub_function_args *args, const char *name, const size_t len, \
        int (*func_destroy)(const void *ptr));
int multiple_stub_data_callback_clone_set(struct multiple_stub_function_args *args, const char *name, const size_t len, \
        void *(*func_clone)(const void *ptr));
int multiple_stub_data_callback_print_set(struct multiple_stub_function_args *args, const char *name, const size_t len, \
        int (*func_print)(const void *ptr));
int multiple_stub_data_callback_eq_set(struct multiple_stub_function_args *args, const char *name, const size_t len, \
        int (*func_eq)(const void *ptr_left, const void *ptr_right));

/* Object Operations */
int virtual_machine_object_print(const struct virtual_machine_object *object);
struct virtual_machine_object *virtual_machine_object_clone(struct virtual_machine *vm, const struct virtual_machine_object *obj);
int virtual_machine_object_destroy(struct virtual_machine *vm, struct virtual_machine_object *object);

int virtual_machine_object_to_type_name(char **str_out, size_t *len_out, const struct virtual_machine_object *object_src);


/* Argument passing */
int multiple_stub_args_get_int(struct multiple_stub_function_args *args, int *ptr);
int multiple_stub_args_get_float(struct multiple_stub_function_args *args, double *ptr);
int multiple_stub_args_get_str(struct multiple_stub_function_args *args, char **str_p, size_t *str_len);
int multiple_stub_args_get_bool(struct multiple_stub_function_args *args, int *ptr);
int multiple_stub_args_get_none(struct multiple_stub_function_args *args);
int multiple_stub_args_get_raw(struct multiple_stub_function_args *args, const char *name, const size_t len, void **ptr);
int multiple_stub_args_get_any(struct multiple_stub_function_args *args, char **name, size_t *len, void **ptr);
int multiple_stub_args_get_semaphore(struct multiple_stub_function_args *args, uint32_t *semaphore_id);

/* Value returning */
int multiple_stub_return_int(struct multiple_stub_function_args *args, int value);
int multiple_stub_return_float(struct multiple_stub_function_args *args, double value);
int multiple_stub_return_str(struct multiple_stub_function_args *args, char *str_p, size_t str_len);
int multiple_stub_return_bool(struct multiple_stub_function_args *args, int value);
int multiple_stub_return_none(struct multiple_stub_function_args *args);
int multiple_stub_return_raw(struct multiple_stub_function_args *args, const char *name, const size_t len, void *ptr);

/* External Event */
int multiple_stub_external_event_arg_push_int(struct virtual_machine *vm, const uint32_t id, \
        int value);
int multiple_stub_external_event_arg_push_raw(struct virtual_machine *vm, const uint32_t id, \
        const char *name, const size_t len, void *ptr);
int multiple_stub_external_event_raise(struct virtual_machine *vm, const uint32_t id);
int multiple_stub_external_event_args_count(struct virtual_machine *vm, const uint32_t id, size_t *count);

/* Synchronization */
int virtual_machine_semaphore_p(struct virtual_machine *vm, uint32_t semaphore_id);
int virtual_machine_semaphore_v(struct virtual_machine *vm, uint32_t semaphore_id);
int virtual_machine_semaphore_value(struct virtual_machine *vm, uint32_t semaphore_id);

/* GIL */
int virtual_machine_gil_lock(struct virtual_machine *vm);
int virtual_machine_gil_unlock(struct virtual_machine *vm);

/* Error Reporting */
int multiple_stub_error(struct multiple_stub_function_args *args, int number, const char *fmt, ...);

#define multiple_stub_error_unknown(args) \
    multiple_stub_error(args, -MULTIPLE_ERR_UNKNOWN, "runtime error: unknown error in %s() %s:%d", __FUNC_NAME_, __FILE__, __LINE__)

#define multiple_stub_error_undefined(args) \
    multiple_stub_error(args, -MULTIPLE_ERR_UNDEFINED, "runtime error: undefined error in %s() %s:%d", __FUNC_NAME_, __FILE__, __LINE__)

#define multiple_stub_error_internal(args) \
    multiple_stub_error(args, -MULTIPLE_ERR_INTERNAL, "runtime error: internal error in %s() %s:%d", __FUNC_NAME__, __FILE__, __LINE__)

#define multiple_stub_error_not_implemented(args) \
    multiple_stub_error(args, -MULTIPLE_ERR_NOT_IMPLEMENTED, "runtime error: feature not implemented yet in %s() %s:%d",  __FUNC_NAME__, __FILE__, __LINE__)

#define multiple_stub_error_null_ptr(args) \
    multiple_stub_error(args, -MULTIPLE_ERR_NULL_PTR, "runtime error: null pointer in %s() %s:%d",  __FUNC_NAME__, __FILE__, __LINE__)

#define multiple_stub_error_malloc(args) \
    multiple_stub_error(args, -MULTIPLE_ERR_MALLOC, "runtime error: out of memory in %s() %s:%d",  __FUNC_NAME__, __FILE__, __LINE__)

/* Assistance */
int multiple_ir_update_icode_source_code(struct multiple_ir *icode, char *code, size_t code_len);

#endif

