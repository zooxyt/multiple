/* String Objects
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

#ifndef _VM_OBJECT_STR_H_
#define _VM_OBJECT_STR_H_

#include <stdio.h>
#include <stdint.h>

#include "crc32.h"

struct virtual_machine_object;

/* Internal Part */

struct virtual_machine_object_str_internal
{
    char *str;
    size_t len;

    crc32_t checksum_crc32;

    struct virtual_machine *vm;
};

/* Shell Part */

struct virtual_machine_object_str
{
    /* Pointer to internal (the kernel part) */
    struct virtual_machine_object_str_internal *ptr_internal;
};

struct virtual_machine_object *virtual_machine_object_str_new_with_value(struct virtual_machine *vm, \
        const char *str, const size_t len);
int virtual_machine_object_str_destroy(struct virtual_machine *vm, \
        struct virtual_machine_object *object);
struct virtual_machine_object *virtual_machine_object_str_clone(struct virtual_machine *vm, \
        const struct virtual_machine_object *object);
int virtual_machine_object_str_print(const struct virtual_machine_object *object);


int virtual_machine_object_str_property_get(struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const struct virtual_machine_object *object_property, \
        struct virtual_machine *vm);

/* size */
int virtual_machine_object_str_size( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src);
/* convert */
int virtual_machine_object_str_convert( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, const uint32_t type);

/* add */
int virtual_machine_object_str_add( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode);
/* mul */
int virtual_machine_object_str_mul( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, 
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, 
        const uint32_t opcode);
/* eq, ne */
int virtual_machine_object_str_binary_equality( \
        struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_left, const struct virtual_machine_object *object_right, \
        const uint32_t opcode);

/* Extract string */
int virtual_machine_object_str_extract(char **str_out, size_t *str_len, struct virtual_machine_object *object);

#endif

