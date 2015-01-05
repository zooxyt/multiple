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

/* An identifier in virtual machine is a symbol to represent an object
 * (Different types). In Virtual Machine's implementation, it has been put
 * into the data section and assigned with a unique number, this number will
 * be used as operand of Virtual Machine's instruments */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "multiple_err.h"

#include "vm_res.h"
#include "vm_types.h"
#include "vm_object_aio.h"

/* Create a new identifier object with specified value */
struct virtual_machine_object *virtual_machine_object_identifier_new_with_value( \
        struct virtual_machine *vm, \
        const char *id, const size_t id_len, const uint32_t id_module_id, const uint32_t id_data_id, \
        const char *domain_id, const size_t domain_id_len, const uint32_t domain_id_module_id, const uint32_t domain_id_data_id)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_identifier *new_object_id = NULL;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_IDENTIFIER)) == NULL)
    { goto fail; }

    /* Identifier's body */
    if ((new_object_id = (struct virtual_machine_object_identifier *)virtual_machine_resource_malloc( \
                    vm->resource, (sizeof(struct virtual_machine_object_identifier)))) == NULL)
    { goto fail; }
    new_object_id->id_data_id = id_data_id;
    new_object_id->id_module_id = id_module_id;
    new_object_id->domain_id_data_id = domain_id_data_id;
    new_object_id->domain_id_module_id = domain_id_module_id;
    new_object_id->id = NULL;
    new_object_id->domain_id = NULL;

    /* ID */
    if ((new_object_id->id = (char *)virtual_machine_resource_malloc_primitive( \
                    vm->resource, sizeof(char) * (id_len + 1))) == NULL)
    { goto fail; }
    memcpy(new_object_id->id, id, id_len);
    new_object_id->id[id_len] = '\0';
    new_object_id->id_len = id_len;

    /* Domain */
    if (domain_id != NULL)
    {
        if ((new_object_id->domain_id = (char *)virtual_machine_resource_malloc_primitive( \
                        vm->resource, sizeof(char) * (domain_id_len + 1))) == NULL)
        { goto fail; }
        memcpy(new_object_id->domain_id, domain_id, domain_id_len);
        new_object_id->domain_id[domain_id_len] = '\0';
        new_object_id->domain_id_len = domain_id_len;
    }

    if (_virtual_machine_object_ptr_set(new_object, new_object_id) != 0) { goto fail; }
    goto done;
fail:
    if (new_object != NULL) { _virtual_machine_object_destroy(vm, new_object); }
    if (new_object_id != NULL)
    {
        if (new_object_id->id != NULL) virtual_machine_resource_free_primitive(vm->resource, new_object_id->id);
        if (new_object_id->domain_id != NULL) virtual_machine_resource_free_primitive(vm->resource, new_object_id->domain_id);
        virtual_machine_resource_free(vm->resource, new_object_id);
    }
done:
    return new_object;
}

int virtual_machine_object_identifier_destroy(struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    struct virtual_machine_object_identifier *object_id_ptr = NULL;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_id_ptr = object->ptr;
    if (object_id_ptr != NULL)
    {
        if (object_id_ptr->id != NULL) virtual_machine_resource_free_primitive(vm->resource, object_id_ptr->id);
        if (object_id_ptr->domain_id != NULL) virtual_machine_resource_free_primitive(vm->resource, object_id_ptr->domain_id);
        virtual_machine_resource_free(vm->resource, object_id_ptr);
    }
    virtual_machine_resource_free(vm->resource, object);

    return 0;
}

struct virtual_machine_object *virtual_machine_object_identifier_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_identifier *object_identifier_ptr = NULL;

    if (object == NULL) return NULL;

    object_identifier_ptr = object->ptr;
    if ((new_object = virtual_machine_object_identifier_new_with_value(vm, \
                    object_identifier_ptr->id, object_identifier_ptr->id_len, 
                    object_identifier_ptr->id_module_id, object_identifier_ptr->id_data_id,
                    object_identifier_ptr->domain_id, object_identifier_ptr->domain_id_len, 
                    object_identifier_ptr->domain_id_module_id, object_identifier_ptr->domain_id_data_id)) == NULL)
    { return NULL; }

    return new_object;
}

int virtual_machine_object_identifier_print(const struct virtual_machine_object *object)
{
    struct virtual_machine_object_identifier *new_object_identifier_ptr = NULL;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    new_object_identifier_ptr = object->ptr;

    if (new_object_identifier_ptr->domain_id != NULL) 
    {
        fwrite(new_object_identifier_ptr->domain_id, new_object_identifier_ptr->domain_id_len, 1, stdout);
        printf("::");
    }
    fwrite(new_object_identifier_ptr->id, new_object_identifier_ptr->id_len, 1, stdout);

    return 0;
}

int virtual_machine_object_identifier_get_value(const struct virtual_machine_object *object, char **id, size_t *len)
{
    struct virtual_machine_object_identifier *new_object_identifier_ptr = NULL;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    new_object_identifier_ptr = object->ptr;

    *id = new_object_identifier_ptr->id;
    *len = new_object_identifier_ptr->id_len;

    return 0;
}

