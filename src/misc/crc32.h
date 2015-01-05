/* CRC-32
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

#ifndef _CRC32_H_
#define _CRC32_H_

#include <stdio.h>

typedef unsigned long crc32_t;

int crc32_init(crc32_t *crc);
int crc32_update(crc32_t *crc, unsigned char *buf, size_t len);
int crc32_final(crc32_t *crc);

int crc32_str(crc32_t *crc, unsigned char *buf, size_t len);

#endif

