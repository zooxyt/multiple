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

#ifndef _VM_OBJECT_COMPLEX_H_
#define _VM_OBJECT_COMPLEX_H_

#include <stdint.h>

struct virtual_machine_object;

#define VIRTUAL_MACHINE_OBJECT_COMPLEX_TYPE_FLT 0
#define VIRTUAL_MACHINE_OBJECT_COMPLEX_TYPE_RAT 1

struct virtual_machine_object_complex
{
    int sign;
	int padding;
	struct virtual_machine_object_complex_part
    {
		union virtual_machine_object_complex_part_numeric
        {
			struct virtual_machine_object_complex_part_numeric_rat
            {
                unsigned int numerator;
                unsigned int denominator;
            } rat;
            double flt;
		} numeric;
        int sign;
		int padding;
	} real, image;
	int type_real;
    int type_image;
};

/* new */
struct virtual_machine_object *virtual_machine_object_complex_new_with_ff( \
        struct virtual_machine *vm, \
        int real_sign, const double real_flt, \
        int image_sign, const double image_flt);
struct virtual_machine_object *virtual_machine_object_complex_new_with_rr( \
        struct virtual_machine *vm, \
        int real_sign, const unsigned int real_rat_numerator, const unsigned int real_rat_denumerator, \
        int image_sign, const unsigned int image_rat_numerator, const unsigned int image_rat_denumerator);
/* destroy */
int virtual_machine_object_complex_destroy( \
        struct virtual_machine *vm, \
        struct virtual_machine_object *object);
/* clone */
struct virtual_machine_object *virtual_machine_object_complex_clone( \
        struct virtual_machine *vm, \
        const struct virtual_machine_object *object);
/* print */
int virtual_machine_object_complex_print(const struct virtual_machine_object *object);

#endif

