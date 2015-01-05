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

#ifndef _VM_STARTUP_H_
#define _VM_STARTUP_H_

enum 
{
    VIRTUAL_MACHINE_STARTUP_MEM_ITEM_INFRASTRUCTURE = 0,
    VIRTUAL_MACHINE_STARTUP_MEM_ITEM_PRIMITIVE = 1,
    VIRTUAL_MACHINE_STARTUP_MEM_ITEM_REFERENCE = 2,
};
#define VIRTUAL_MACHINE_STARTUP_MEM_TYPE_COUNT 3


enum 
{
    VIRTUAL_MACHINE_STARTUP_MEM_TYPE_DEFAULT = 0,
    VIRTUAL_MACHINE_STARTUP_MEM_TYPE_LIB_C = VIRTUAL_MACHINE_STARTUP_MEM_TYPE_DEFAULT,
    VIRTUAL_MACHINE_STARTUP_MEM_TYPE_PAGED_FALLBACK = 1,
    VIRTUAL_MACHINE_STARTUP_MEM_TYPE_PAGED_64B_FALLBACK = 2,
    VIRTUAL_MACHINE_STARTUP_MEM_TYPE_PAGED_128B_FALLBACK = 3,
    VIRTUAL_MACHINE_STARTUP_MEM_TYPE_LINKED_FALLBACK = 4,
    VIRTUAL_MACHINE_STARTUP_MEM_TYPE_LINKED_64B_FALLBACK = 5,
    VIRTUAL_MACHINE_STARTUP_MEM_TYPE_LINKED_128B_FALLBACK = 6,
};
#define VIRTUAL_MACHINE_STARTUP_MEM_INFRASTRUCTURE_SIZE_DEFAULT (5 * 1024 * 1024) /* 5 MB */
/*#define VIRTUAL_MACHINE_STARTUP_MEM_INFRASTRUCTURE_TYPE_DEFAULT (VIRTUAL_MACHINE_STARTUP_MEM_TYPE_LIB_C)*/
#define VIRTUAL_MACHINE_STARTUP_MEM_INFRASTRUCTURE_TYPE_DEFAULT (VIRTUAL_MACHINE_STARTUP_MEM_TYPE_LINKED_128B_FALLBACK)
/*#define VIRTUAL_MACHINE_STARTUP_MEM_INFRASTRUCTURE_TYPE_DEFAULT (VIRTUAL_MACHINE_STARTUP_MEM_TYPE_PAGED_128B_FALLBACK)*/

#define VIRTUAL_MACHINE_STARTUP_MEM_PRIMITIVE_SIZE_DEFAULT (1 * 1024 * 1024) /* 1 MB */
/*#define VIRTUAL_MACHINE_STARTUP_MEM_PRIMITIVE_TYPE_DEFAULT (VIRTUAL_MACHINE_STARTUP_MEM_TYPE_LIB_C)*/
#define VIRTUAL_MACHINE_STARTUP_MEM_PRIMITIVE_TYPE_DEFAULT (VIRTUAL_MACHINE_STARTUP_MEM_TYPE_LINKED_64B_FALLBACK)
/*#define VIRTUAL_MACHINE_STARTUP_MEM_PRIMITIVE_TYPE_DEFAULT (VIRTUAL_MACHINE_STARTUP_MEM_TYPE_PAGED_64B_FALLBACK)*/

#define VIRTUAL_MACHINE_STARTUP_MEM_OBJECTS_SIZE_DEFAULT (2 * 1024 * 1024) /* 2 MB */
#define VIRTUAL_MACHINE_STARTUP_MEM_OBJECTS_TYPE_DEFAULT (VIRTUAL_MACHINE_STARTUP_MEM_TYPE_LIB_C)
/*#define VIRTUAL_MACHINE_STARTUP_MEM_OBJECTS_TYPE_DEFAULT (VIRTUAL_MACHINE_STARTUP_MEM_TYPE_LINKED_128B_FALLBACK)*/

struct virtual_machine_startup_item
{
    int type;
    size_t size;
};

struct virtual_machine_startup
{
    struct virtual_machine_startup_item items[VIRTUAL_MACHINE_STARTUP_MEM_TYPE_COUNT];
    int keep_dll;
};

int virtual_machine_startup_init(struct virtual_machine_startup *startup);

int virtual_machine_startup_memory_usage(struct virtual_machine_startup *startup, \
        const char *mem_item, const char *mem_type, const char *mem_size);

#endif

