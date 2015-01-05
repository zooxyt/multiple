/* Virtual Machine : Startup
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

#include <stdio.h>
#include <string.h>

#include "multiple_misc.h"
#include "vm_startup.h"

int virtual_machine_startup_init(struct virtual_machine_startup *startup)
{

    startup->items[VIRTUAL_MACHINE_STARTUP_MEM_ITEM_INFRASTRUCTURE].type = VIRTUAL_MACHINE_STARTUP_MEM_INFRASTRUCTURE_TYPE_DEFAULT;
    startup->items[VIRTUAL_MACHINE_STARTUP_MEM_ITEM_INFRASTRUCTURE].size = VIRTUAL_MACHINE_STARTUP_MEM_INFRASTRUCTURE_SIZE_DEFAULT;
    startup->items[VIRTUAL_MACHINE_STARTUP_MEM_ITEM_PRIMITIVE].type = VIRTUAL_MACHINE_STARTUP_MEM_PRIMITIVE_TYPE_DEFAULT;
    startup->items[VIRTUAL_MACHINE_STARTUP_MEM_ITEM_PRIMITIVE].size = VIRTUAL_MACHINE_STARTUP_MEM_PRIMITIVE_SIZE_DEFAULT;
    startup->items[VIRTUAL_MACHINE_STARTUP_MEM_ITEM_REFERENCE].type = VIRTUAL_MACHINE_STARTUP_MEM_OBJECTS_TYPE_DEFAULT;
    startup->items[VIRTUAL_MACHINE_STARTUP_MEM_ITEM_REFERENCE].size = VIRTUAL_MACHINE_STARTUP_MEM_OBJECTS_SIZE_DEFAULT;
    startup->keep_dll = 0;

    return 0;
}

int virtual_machine_startup_memory_usage(struct virtual_machine_startup *startup, \
        const char *mem_item, const char *mem_type, const char *mem_size)
{
    size_t idx;
    size_t mem_item_len, mem_type_len, mem_size_len;

    int mem_item_idx = -1;
    int mem_type_idx = -1;
    long mem_size_number = 0;

    /* Item */
    static const char *memory_usage_item_str[] = 
    { "infrastructure", "primitive", "reference", };
    static const int memory_usage_item_idx[] = 
    { 
        VIRTUAL_MACHINE_STARTUP_MEM_ITEM_INFRASTRUCTURE,
        VIRTUAL_MACHINE_STARTUP_MEM_ITEM_PRIMITIVE,
        VIRTUAL_MACHINE_STARTUP_MEM_ITEM_REFERENCE,
    };
    const size_t memory_usage_item_count = (sizeof(memory_usage_item_str) / sizeof(const char *));

    /* Type */
    static const char *memory_usage_type_str[] = 
    { "default", "libc", "4k", "64b", "128b", };
    static const int memory_usage_type_idx[] = 
    { 
        VIRTUAL_MACHINE_STARTUP_MEM_TYPE_DEFAULT,
        VIRTUAL_MACHINE_STARTUP_MEM_TYPE_LIB_C,
        VIRTUAL_MACHINE_STARTUP_MEM_TYPE_LINKED_FALLBACK,
        VIRTUAL_MACHINE_STARTUP_MEM_TYPE_LINKED_64B_FALLBACK, 
        VIRTUAL_MACHINE_STARTUP_MEM_TYPE_LINKED_128B_FALLBACK, 
    };
    const size_t memory_usage_type_count = (sizeof(memory_usage_type_str) / sizeof(const char *));

    if (mem_item == NULL || mem_type == NULL || mem_size == NULL) return -1;

    mem_item_len = strlen(mem_item);
    mem_type_len = strlen(mem_type);
    mem_size_len = strlen(mem_size);

    /* Item */
    for (idx = 0; idx != memory_usage_item_count; idx++)
    {
        if ((strlen(memory_usage_item_str[idx]) == mem_item_len) && \
                (strncmp(memory_usage_item_str[idx], mem_item, mem_item_len) == 0))
        {
             mem_item_idx = memory_usage_item_idx[idx];
             break;
        }
    }
    if (mem_item_idx == -1) return -1;

    /* Type */
    for (idx = 0; idx != memory_usage_type_count; idx++)
    {
        if ((strlen(memory_usage_type_str[idx]) == mem_type_len) && \
                (strncmp(memory_usage_type_str[idx], mem_type, mem_type_len) == 0))
        {
             mem_type_idx = memory_usage_type_idx[idx];
             break;
        }
    }
    if (mem_type_idx == -1) return -1;

    /* Size */
    if (size_atoin(&mem_size_number, mem_size, mem_size_len) != 0) return -1;

    startup->items[mem_item_idx].type = mem_type_idx;
    startup->items[mem_item_idx].size = (size_t)mem_size_number;

    return -1;
}

