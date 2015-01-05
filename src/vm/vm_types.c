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

#include <string.h>
#include <stdint.h>

#include "vm_types.h"
#include "vm_object_aio.h"

#include "vm_object.h"


struct virtual_machine_object_type_item
{
    const enum virtual_machine_object_type_id id;
    const char *name;
};

static struct virtual_machine_object_type_item virtual_machine_object_type_item[] = 
{
    {OBJECT_TYPE_UNKNOWN, NULL},
    {OBJECT_TYPE_IDENTIFIER, "id"},
    {OBJECT_TYPE_INT, "int"},
    {OBJECT_TYPE_STR, "str"},
    {OBJECT_TYPE_BOOL, "bool"},
    {OBJECT_TYPE_NONE, "none"},
    {OBJECT_TYPE_UNDEF, "undef"},
    {OBJECT_TYPE_NAN, "nan"},
    {OBJECT_TYPE_INF, "inf"},
    {OBJECT_TYPE_FLOAT, "float"},
    {OBJECT_TYPE_RATIONAL, "rational"},
    {OBJECT_TYPE_COMPLEX, "complex"},
    {OBJECT_TYPE_TYPE, "type"},
    {OBJECT_TYPE_LIST, "list"},
    {OBJECT_TYPE_ARRAY, "array"},
    {OBJECT_TYPE_TUPLE, "tuple"},
    {OBJECT_TYPE_HASH, "hash"},
    {OBJECT_TYPE_THREAD, "thread"},
    {OBJECT_TYPE_MUTEX, "mutex"},
    {OBJECT_TYPE_SEMAPHORE, "semaphore"},
    {OBJECT_TYPE_FUNCTION, "function"},
    {OBJECT_TYPE_RAW, "raw"},
    {OBJECT_TYPE_CLASS, "class"},
    {OBJECT_TYPE_PAIR, "pair"},
    {OBJECT_TYPE_CHAR, "char"},
    {OBJECT_TYPE_SYMBOL, "symbol"},
    {OBJECT_TYPE_ENV, "env"},
    {OBJECT_TYPE_ENV_ENT, "envent"},
    {OBJECT_TYPE_FINAL, NULL},
};


/* Convert type name to type id, 0 for success, -1 for not found */
int virtual_machine_object_type_name_to_id(enum virtual_machine_object_type_id *id_out, \
        const char *str, const size_t len)
{
    size_t i;

	for (i = OBJECT_TYPE_FIRST; i != OBJECT_TYPE_FINAL; i++)
    {
        if ((strlen(virtual_machine_object_type_item[i].name) == len) && \
                (strncmp(virtual_machine_object_type_item[i].name, str, len) == 0))
        {
            *id_out = virtual_machine_object_type_item[i].id;
            return 0;
        }
    }

    return -1;
}

/* Convert type type id to type name, 0 for success, -1 for not found */
int virtual_machine_object_id_to_type_name(char **str_out, size_t *len_out, \
        const enum virtual_machine_object_type_id id)
{
    size_t i;

    if (str_out != NULL) *str_out = NULL;
	for (i = OBJECT_TYPE_FIRST; i != OBJECT_TYPE_FINAL; i++)
    {
        if (virtual_machine_object_type_item[i].id == id)
        {
            if (str_out != NULL) *str_out = (char *)virtual_machine_object_type_item[i].name;
            if (len_out != NULL) 
            {
                if (virtual_machine_object_type_item[i].name != NULL)
                {
                    *len_out = strlen(virtual_machine_object_type_item[i].name);
                }
                else
                {
                    *len_out = 0;
                }
            }
            return 0;
        }
    }

    return -1;
}

