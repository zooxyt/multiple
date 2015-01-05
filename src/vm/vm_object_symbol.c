/* Symbol Objects
   Copyright(C) 2013-2014 Cheryl Natsu

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "multiple_err.h"

#include "vm_types.h"
#include "vm_res.h"
#include "vm_object_aio.h"

struct virtual_machine_object *virtual_machine_object_symbol_new_with_value( \
        struct virtual_machine *vm, \
        const char *id, const size_t id_len)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_symbol *new_object_id = NULL;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_SYMBOL)) == NULL)
    { goto fail; }

    /* Symbol's body */
    if ((new_object_id = (struct virtual_machine_object_symbol *)virtual_machine_resource_malloc( \
                    vm->resource, (sizeof(struct virtual_machine_object_symbol)))) == NULL)
    { goto fail; }
    new_object_id->id = NULL;

    /* ID */
    if ((new_object_id->id = (char *)virtual_machine_resource_malloc_primitive( \
                    vm->resource, sizeof(char) * (id_len + 1))) == NULL)
    { goto fail; }
    memcpy(new_object_id->id, id, id_len);
    new_object_id->id[id_len] = '\0';
    new_object_id->id_len = id_len;

    if (_virtual_machine_object_ptr_set(new_object, new_object_id) != 0) { goto fail; }
    goto done;
fail:
    if (new_object != NULL) { _virtual_machine_object_destroy(vm, new_object); }
    if (new_object_id != NULL)
    {
        if (new_object_id->id != NULL) virtual_machine_resource_free_primitive(vm->resource, new_object_id->id);
        virtual_machine_resource_free(vm->resource, new_object_id);
    }
done:
    return new_object;
}

int virtual_machine_object_symbol_destroy(struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    struct virtual_machine_object_symbol *object_id_ptr = NULL;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    object_id_ptr = object->ptr;
    if (object_id_ptr != NULL)
    {
        if (object_id_ptr->id != NULL) virtual_machine_resource_free_primitive(vm->resource, object_id_ptr->id);
        virtual_machine_resource_free(vm->resource, object_id_ptr);
    }
    virtual_machine_resource_free(vm->resource, object);

    return 0;
}

struct virtual_machine_object *virtual_machine_object_symbol_clone(struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_symbol *object_symbol_ptr = NULL;

    if (object == NULL) return NULL;

    object_symbol_ptr = object->ptr;
    if ((new_object = virtual_machine_object_symbol_new_with_value(vm, \
                    object_symbol_ptr->id, object_symbol_ptr->id_len)) == NULL)
    { return NULL; }

    return new_object;
}

int virtual_machine_object_symbol_print( \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object_symbol *new_object_symbol_ptr = NULL;

    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    new_object_symbol_ptr = object->ptr;

    fwrite(new_object_symbol_ptr->id, new_object_symbol_ptr->id_len, 1, stdout);

    return 0;
}

/* convert */
int virtual_machine_object_symbol_convert(struct virtual_machine *vm, \
        struct virtual_machine_object **object_out, \
        const struct virtual_machine_object *object_src, \
        const uint32_t type)
{
    int ret = 0;
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_symbol *object_symbol = NULL;

    (void)vm;

    if (object_src == NULL)
    {
        VM_ERR_NULL_PTR(vm->r);
        return -MULTIPLE_ERR_VM;
    }

    *object_out = NULL;

    switch (type)
    {
        case OBJECT_TYPE_STR:
            object_symbol = ((struct virtual_machine_object_symbol *)(object_src->ptr));
            new_object = virtual_machine_object_str_new_with_value(vm, object_symbol->id, object_symbol->id_len);
            break;
    }
    if (new_object == NULL) 
    {
        VM_ERR_MALLOC(vm->r);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }

    *object_out = new_object;
    goto done;
fail:
    if (new_object != NULL) virtual_machine_object_destroy(vm, new_object);
done:
    return ret;
}
