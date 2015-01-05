/* Complex Objects
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

/* new */
struct virtual_machine_object *virtual_machine_object_complex_new_with_ff( \
        struct virtual_machine *vm, \
        int real_sign, const double real_flt, \
        int image_sign, const double image_flt)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_complex *new_object_complex = NULL;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_COMPLEX)) == NULL)
    {
        return NULL;
    }

    if ((new_object_complex = (struct virtual_machine_object_complex *)virtual_machine_resource_malloc_primitive( \
                    vm->resource, sizeof(struct virtual_machine_object_complex))) == NULL)
    {
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }

    if (_virtual_machine_object_ptr_set(new_object, new_object_complex) != 0)
    {
        virtual_machine_resource_free_primitive(vm->resource, new_object_complex);
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }
    new_object_complex->type_real = VIRTUAL_MACHINE_OBJECT_COMPLEX_TYPE_FLT;
    new_object_complex->type_image = VIRTUAL_MACHINE_OBJECT_COMPLEX_TYPE_FLT;
    new_object_complex->real.sign = real_sign;
    new_object_complex->real.numeric.flt = real_flt;
    new_object_complex->image.sign = image_sign;
    new_object_complex->image.numeric.flt = image_flt;

    return new_object;
}

struct virtual_machine_object *virtual_machine_object_complex_new_with_rr( \
        struct virtual_machine *vm, \
        int real_sign, const unsigned int real_rat_numerator, const unsigned int real_rat_denumerator, \
        int image_sign, const unsigned int image_rat_numerator, const unsigned int image_rat_denumerator)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_complex *new_object_complex = NULL;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_COMPLEX)) == NULL)
    {
        return NULL;
    }

    if ((new_object_complex = (struct virtual_machine_object_complex *)virtual_machine_resource_malloc_primitive( \
                    vm->resource, sizeof(struct virtual_machine_object_complex))) == NULL)
    {
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }

    if (_virtual_machine_object_ptr_set(new_object, new_object_complex) != 0)
    {
        virtual_machine_resource_free_primitive(vm->resource, new_object_complex);
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }
    new_object_complex->type_real = VIRTUAL_MACHINE_OBJECT_COMPLEX_TYPE_RAT;
    new_object_complex->type_image = VIRTUAL_MACHINE_OBJECT_COMPLEX_TYPE_RAT;
    new_object_complex->real.sign = real_sign;
    new_object_complex->real.numeric.rat.numerator = real_rat_numerator;
    new_object_complex->real.numeric.rat.denominator = real_rat_denumerator;
    new_object_complex->image.sign = image_sign;
    new_object_complex->image.numeric.rat.numerator = image_rat_numerator;
	new_object_complex->image.numeric.rat.denominator = image_rat_denumerator;

    return new_object;
}

/* Destroy a complex object */
int virtual_machine_object_complex_destroy(struct virtual_machine *vm, \
        struct virtual_machine_object *object)
{
    if (object == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (object->ptr != NULL) virtual_machine_resource_free_primitive(vm->resource, object->ptr);
    _virtual_machine_object_destroy(vm, object);

    return 0;
}

/* Clone a complex object */
struct virtual_machine_object *virtual_machine_object_complex_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object)
{
    struct virtual_machine_object *new_object = NULL;
    struct virtual_machine_object_complex *new_object_complex = NULL;
    struct virtual_machine_object_complex *object_complex = NULL;

    if ((object == NULL) || (object->ptr == NULL)) return NULL;

    object_complex = object->ptr;

    /* Create the object's infrastructure */
    if ((new_object = _virtual_machine_object_new(vm, OBJECT_TYPE_COMPLEX)) == NULL)
    {
        return NULL;
    }

    if ((new_object_complex = (struct virtual_machine_object_complex *)virtual_machine_resource_malloc_primitive( \
                    vm->resource, sizeof(struct virtual_machine_object_complex))) == NULL)
    {
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }

    if (_virtual_machine_object_ptr_set(new_object, new_object_complex) != 0)
    {
        virtual_machine_resource_free_primitive(vm->resource, new_object_complex);
        _virtual_machine_object_destroy(vm, new_object);
        return NULL;
    }
    new_object_complex->type_real = object_complex->type_real; 
    new_object_complex->type_image = object_complex->type_image; 
    memcpy(&new_object_complex->real, &object_complex->real, sizeof(object_complex->real));
    memcpy(&new_object_complex->image, &object_complex->image, sizeof(object_complex->image));

    return new_object;
}

/* print */
int virtual_machine_object_complex_print(const struct virtual_machine_object *object)
{
    struct virtual_machine_object_complex *object_complex = NULL;

    if ((object == NULL) || (object->ptr == NULL)) return -MULTIPLE_ERR_NULL_PTR;

    object_complex = ((struct virtual_machine_object_complex *)(object->ptr));

    /* Real Part */
    if (object_complex->real.sign != 0) { fputc('-', stdout); }
    switch (object_complex->type_real)
    {
        case VIRTUAL_MACHINE_OBJECT_COMPLEX_TYPE_FLT:
			printf("%lf", object_complex->real.numeric.flt);
            break;

        case VIRTUAL_MACHINE_OBJECT_COMPLEX_TYPE_RAT: 
			if (object_complex->real.numeric.rat.numerator == 0)
            {
                printf("0");
            }
			else if (object_complex->real.numeric.rat.denominator == 1)
            {
				printf("%u", object_complex->real.numeric.rat.numerator);
            }
            else
            {
				printf("%u/%u", object_complex->real.numeric.rat.numerator, object_complex->real.numeric.rat.denominator);
            }
            break;
    }


    /* Image Part */
    if (object_complex->image.sign != 0) { fputc('-', stdout); }
    else { fputc('+', stdout); }

    switch (object_complex->type_image)
    {
        case VIRTUAL_MACHINE_OBJECT_COMPLEX_TYPE_FLT:
			printf("%lf", object_complex->image.numeric.flt);
            break;

        case VIRTUAL_MACHINE_OBJECT_COMPLEX_TYPE_RAT: 
			if (object_complex->image.numeric.rat.numerator == 0)
            {
                printf("0");
            }
            else if (object_complex->image.numeric.rat.denominator == 1)
            {
                printf("%u", object_complex->image.numeric.rat.numerator); 
            }
            else
            {
				printf("%u/%u", object_complex->image.numeric.rat.numerator, object_complex->image.numeric.rat.denominator);
            }
            break;
    }

    { fputc('i', stdout); }

    return 0;
}

