/* Dynamic Library 
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

#ifndef _VM_DYNLIB_H_
#define _VM_DYNLIB_H_

#include "multiple_err.h"
#include "multiple_tunnel.h"

#include "vm_infrastructure.h"

int virtual_machine_dynlib_invoke(struct virtual_machine *vm, struct multiple_stub_function_args *args, char *name_lib, char *name_func);
int virtual_machine_dynlib_close_handle(struct virtual_machine *vm, void *handle);

#endif

