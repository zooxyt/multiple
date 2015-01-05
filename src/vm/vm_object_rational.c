/* Rational Objects
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

#include "selfcheck.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "multiple_err.h"

#include "vm_opcode.h"
#include "vm_types.h"

#include "vm_infrastructure.h"
#include "vm_object_aio.h"
#include "vm_err.h"

/* Compatible */
#if defined(_MSC_VER)
#define snprintf _snprintf
#elif defined(WIN32)
#define snprintf _snprintf
#endif

/* Create a new native object with specified value */
struct virtual_machine_object *virtual_machine_object_rational_new_with_value( \
        struct virtual_machine *vm, \
        int sign, const unsigned int numerator, const unsigned int denominator)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_rational *new_object_rational = NULL;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_RATIONAL)) == NULL)
    {
        return NULL;
    }

    if ((new_object_rational = (struct virtual_machine_object_rational *)virtual_machine_resource_malloc_primitive( \
                    vm->resource, sizeof(struct virtual_machine_object_rational))) == NULL)
    {
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }

    if (_virtual_machine_object_ptr_set(new_object, new_object_rational) != 0)
    {
        virtual_machine_resource_free_primitive(vm->resource, new_object_rational);
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }
    new_object_rational->sign = sign;
    new_object_rational->numerator = numerator;
    new_object_rational->denominator = denominator;
    return new_object;
}

/* Destroy a rational object */
int virtual_machine_object_rational_destroy(struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (object->ptr != NULL) virtual_machine_resource_free_primitive(vm->resource, object->ptr);
    _virtual_machine_object_destroy(vm, object);

    return 0;
}

/* Clone a rational object */
struct virtual_machine_object *virtual_machine_object_rational_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_rational *object_rational = NULL;

    if ((object == NULL) || (object->ptr == NULL)) return NULL;

    object_rational = ((struct virtual_machine_object_rational *)(object->ptr));
    if ((new_object = virtual_machine_object_rational_new_with_value(vm, \
                    object_rational->sign, \
                    object_rational->numerator, \
                    object_rational->denominator)) == NULL)
    {
        return NULL;
    }

    return new_object;
}

/* print */
int virtual_machine_object_rational_print(const struct virtual_machine_object *object)
{
    struct virtual_machine_object_rational *object_rational = NULL;

    if ((object == NULL) || (object->ptr == NULL)) return -MULTIPLE_ERR_NULL_PTR;

    object_rational = ((struct virtual_machine_object_rational *)(object->ptr));
    if (object_rational->sign != 0) { fputc('-', stdout); }
    printf("%u/%u", object_rational->numerator, object_rational->denominator); 

    return 0;
}

