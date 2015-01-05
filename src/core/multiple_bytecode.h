/* Bytecode File Generator
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

#ifndef _MULTIPLE_BYTECODE_H_
#define _MULTIPLE_BYTECODE_H_

#include "multiple_err.h"

/* Bytecode is the target format for virtual machine,
 * it uses the opcode the recognized by virtual machine, 
 * it interpreted by virtual machine as running in a real machine */

int multiple_bytecode_gen(struct multiple_error *err, FILE *fp_out, struct multiple_ir *icode);

#endif

