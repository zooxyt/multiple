/* Virtual Machine : Types
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

#ifndef _VM_TYPES_H_
#define _VM_TYPES_H_

#include <stdio.h>

/* Object types */
enum virtual_machine_object_type_id
{
    OBJECT_TYPE_UNKNOWN = 0,
    OBJECT_TYPE_IDENTIFIER = 1,
    OBJECT_TYPE_INT = 2,
    OBJECT_TYPE_STR = 3,
    OBJECT_TYPE_BOOL = 4,
    OBJECT_TYPE_NONE = 5,
    OBJECT_TYPE_UNDEF = 6,
    OBJECT_TYPE_NAN = 7,
    OBJECT_TYPE_INF = 8,
    OBJECT_TYPE_FLOAT = 9,
    OBJECT_TYPE_RATIONAL = 10,
    OBJECT_TYPE_COMPLEX = 11,
    OBJECT_TYPE_TYPE = 12,
    OBJECT_TYPE_LIST = 13,
    OBJECT_TYPE_ARRAY = 14,
    OBJECT_TYPE_TUPLE = 15,
    OBJECT_TYPE_HASH = 16,
    OBJECT_TYPE_THREAD = 17,
    OBJECT_TYPE_MUTEX = 18,
    OBJECT_TYPE_SEMAPHORE = 19,
    OBJECT_TYPE_FUNCTION = 20,
    OBJECT_TYPE_RAW = 21,
    OBJECT_TYPE_CLASS = 22,
    OBJECT_TYPE_PAIR = 23,
    OBJECT_TYPE_CHAR = 24,
    OBJECT_TYPE_SYMBOL = 25,
    OBJECT_TYPE_ENV = 26,
    OBJECT_TYPE_ENV_ENT = 27,
    OBJECT_TYPE_FINAL,
};

#define OBJECT_TYPE_FIRST (OBJECT_TYPE_IDENTIFIER)
#define OBJECT_TYPE_COUNT (OBJECT_TYPE_FINAL - OBJECT_TYPE_FIRST) 

#define OBJECT_IS_IDENTIFIER(x) (((x)==(OBJECT_TYPE_IDENTIFIER))?1:0)
#define OBJECT_IS_NONE(x) (((x)==(OBJECT_TYPE_NONE))?1:0)

#define VIRTUAL_MACHINE_OBJECT_TYPE_ITEM_COUNT OBJECT_TYPE_COUNT

/* Convert type name to type id, 0 for success, -1 for not found */
int virtual_machine_object_type_name_to_id(enum virtual_machine_object_type_id *id_out, \
        const char *str, const size_t len);

/* Convert type type id to type name, 0 for success, -1 for not found */
int virtual_machine_object_id_to_type_name(char **str_out, size_t *len_out, \
        const enum virtual_machine_object_type_id id);


#endif

