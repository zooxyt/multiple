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

#include <stdio.h>
#include <stdint.h>

#include "multiply_num.h"

/* Signed number representations */

/* Sign and Magnitude to Complement */
uint32_t snr_sam_to_cmp(int32_t num)
{
    if (num >= 0) { return (uint32_t)num; }
    else 
    {
        return (((uint32_t)(~(-num)))|(1u<<31)) + 1;
    }
}

/* Complement to Sign and Magnitude */
int32_t snr_cmp_to_sam(uint32_t num)
{
    int sign = (int)(num >> 31);
    if (sign == 0) return (int32_t)num;
    else 
    {
        return -(~((int32_t)(num - 1)));
    }
}

