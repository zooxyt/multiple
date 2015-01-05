/* Identifier Objects
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

#ifndef _VM_OBJECT_ID_H_
#define _VM_OBJECT_ID_H_

#include <stdint.h>

struct virtual_machine_object;

struct virtual_machine_object_identifier
{
    /* Identifier itself */
    char *id;
    size_t id_len;
    uint32_t id_module_id; /* In which module */
    uint32_t id_data_id; /* Data Section Item */

    /* Domain */
    char *domain_id;
    size_t domain_id_len;
    uint32_t domain_id_module_id; /* In which module */
    uint32_t domain_id_data_id; /* Data Section Item */
};

struct virtual_machine_object *virtual_machine_object_identifier_new_with_value( \
        struct virtual_machine *vm, \
        const char *id, const size_t id_len, const uint32_t id_module_id, const uint32_t id_data_id, \
        const char *domain_id, const size_t domain_id_len, const uint32_t domain_id_module_id, const uint32_t domain_id_data_id);
int virtual_machine_object_identifier_destroy(struct virtual_machine *vm, \
        struct virtual_machine_object *object);
struct virtual_machine_object *virtual_machine_object_identifier_clone(struct virtual_machine *vm, \
        const struct virtual_machine_object *object);
int virtual_machine_object_identifier_print( \
        const struct virtual_machine_object *object);

int virtual_machine_object_identifier_get_value(const struct virtual_machine_object *object, char **id, size_t *len);
int virtual_machine_object_identifier_get_id(const struct virtual_machine_object *object, uint32_t *module_id, uint32_t *data_id);

#endif

