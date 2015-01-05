/* Multiply -- Multiple IR Generator
 * Number
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

#ifndef _MULTIPLY_NUM_H_
#define _MULTIPLY_NUM_H_

#include <stdio.h>
#include <stdint.h>

/* Signed number representations */

/* Sign and Magnitude to Complement */
uint32_t snr_sam_to_cmp(int32_t num);
/* Complement to Sign and Magnitude */
int32_t snr_cmp_to_sam(uint32_t num);

#endif

