/* Virtual Machine
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

#ifndef _VM_H_
#define _VM_H_

#include "vm_infrastructure.h"
#include "vm_err.h"
#include "vm_startup.h"

#include "multiple_ir.h"
#include "multiple_tunnel.h"
#include "multiple_err.h"

#define DEBUGGER_COMMAND_LEN_MAX 255

/* Wrapped entrance routine */
int vm_run(struct multiple_error *err, struct vm_err *r, struct multiple_ir *ir, struct virtual_machine_startup *startup, int debug, struct multiple_stub_function_list *external_functions);

/* Wrapped entrance routine for completion */
int virtual_machine_completion(struct multiple_error *err, \
        struct vm_err *r, \
        struct multiple_ir *ir, \
        struct virtual_machine_startup *startup, \
        int debug_info, \
        struct multiple_stub_function_list *external_functions, \
        const char *completion_cmd);

int virtual_machine_destroy(struct virtual_machine *vm, int error_occurred);

#endif 

