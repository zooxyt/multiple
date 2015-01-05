/* Virtual Machine Error Handling
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

#ifndef _VM_ERR_H_
#define _VM_ERR_H_

#include <stdint.h>
#include <stdio.h>

/* Runtime Error */

enum
{
    VM_ERR_UNKNOWN = 1,
    VM_ERR_UNDEFINED = 2,
    VM_ERR_INTERNAL = 3,
    VM_ERR_NOT_IMPLEMENTED = 4,
    VM_ERR_NULL_PTR = 5,
    VM_ERR_MALLOC = 6,

    VM_ERR_DEFAULT,

    VM_ERR_DIVIDE_BY_ZERO,
    VM_ERR_STACK_OVERFLOW,

    VM_ERR_UNSUPPORTED_OBJECT,
    VM_ERR_COMPUTING_STACK_EMPTY,
    VM_ERR_RUNNING_STACK_EMPTY,
    VM_ERR_OBJECT_NOT_FOUND,
    VM_ERR_ACCESS_VIOLATION,
    VM_ERR_UNSUPPORTED_OPCODE,
    VM_ERR_INVALID_OPERAND,
    VM_ERR_INVALID_OPERAND_TYPE,
    VM_ERR_UNSUPPORTED_OPERAND_TYPE,
    VM_ERR_NO_PROPERTY,
    VM_ERR_NO_SUCH_PROPERTY,
    VM_ERR_NO_METHOD,
    VM_ERR_NO_SUCH_METHOD,
    VM_ERR_EMPTY_LIST,
    VM_ERR_EMPTY_HASH,
    VM_ERR_FILE_NOT_OPEN_FOR_READING,
    VM_ERR_FILE_NOT_OPEN_FOR_WRITING,
    VM_ERR_UNDECLARED_VARIABLE,
    VM_ERR_CONVERT_FAILED,
    VM_ERR_UNDEFINED_SYMBOL,
    VM_ERR_OUT_OF_BOUNDS,
    VM_ERR_JUMP_TARGET_NOT_FOUND,
    VM_ERR_NO_AVALIABLE_MODULE,
    VM_ERR_DATA_TYPE,
    VM_ERR_DLCALL,
};

#define VM_ERR_PATHNAME_LEN 256
#define VM_ERR_DESCRIPTION_LEN 512

struct virtual_machine_module;

struct vm_err
{
    /* Memory */
    void *(*malloc)(size_t size);
    void (*free)(void *ptr);

    /* Current State */
    struct virtual_machine_module *module;
    char module_name[VM_ERR_PATHNAME_LEN];
    uint32_t pc;
    uint32_t opcode;
    uint32_t operand;

    /* Error */
    int occurred;
    int number;
    int number_enabled;
    char description[VM_ERR_DESCRIPTION_LEN];
    int description_enabled;

    /* Source Level Info */
    int source_level_enabled;
    char pathname[VM_ERR_PATHNAME_LEN];
    int pathname_enabled;
    uint32_t source_line_no;
    char source_line_text[VM_ERR_DESCRIPTION_LEN];
};

int vm_err_init(struct vm_err *rail);
int vm_err_clear(struct vm_err *rail);
int vm_err_occurred(struct vm_err *rail);
int vm_err_update(struct vm_err *rail, int number, const char *fmt, ...);
int vm_err_final(struct vm_err *rail);
int vm_err_print(struct vm_err *rail);

#ifndef __FUNC_NAME__
#if defined(__GNUC__)
# define __FUNC_NAME__ __FUNCTION__
#elif defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901)
# define __FUNC_NAME__ __func__
#else
# define __FUNC_NAME__ ("??")
#endif
#endif


#define VM_ERR_UNKNOWN(r) \
    vm_err_update(r, -VM_ERR_UNKNOWN, "runtime error: unknown error in %s() %s:%d", __FUNC_NAME_, __FILE__, __LINE__)

#define VM_ERR_UNDEFINED(r) \
    vm_err_update(r, -VM_ERR_UNDEFINED, "runtime error: undefined error in %s() %s:%d", __FUNC_NAME_, __FILE__, __LINE__)

#define VM_ERR_INTERNAL(r) \
    vm_err_update(r, -VM_ERR_INTERNAL, "runtime error: internal error in %s() %s:%d", __FUNC_NAME__, __FILE__, __LINE__)

#define VM_ERR_NOT_IMPLEMENTED(r) \
    vm_err_update(r, -VM_ERR_NOT_IMPLEMENTED, "runtime error: feature not implemented yet in %s() %s:%d",  __FUNC_NAME__, __FILE__, __LINE__)

#define VM_ERR_NULL_PTR(r) \
    vm_err_update(r, -VM_ERR_NULL_PTR, "runtime error: null pointer in %s() %s:%d",  __FUNC_NAME__, __FILE__, __LINE__)

#define VM_ERR_MALLOC(r) \
    vm_err_update(r, -VM_ERR_MALLOC, "runtime error: out of memory in %s() %s:%d",  __FUNC_NAME__, __FILE__, __LINE__)

#endif

