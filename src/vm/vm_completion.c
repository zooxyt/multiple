/* Virtual Machine : Completion
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

#include <string.h>
#include <stdarg.h>

#include "multiple.h"
#include "multiple_ir.h"
#include "multiple_err.h"
#include "multiple_misc.h"
#include "multiple_prelude.h"

#include "vm_predef.h"
#include "vm_res.h"
#include "vm_opcode.h"
#include "vm_types.h"

#include "vm_infrastructure.h"
#include "vm_gc.h"
#include "vm_cpu.h"
#include "vm.h"
#include "vm_dynlib.h"
#include "vm_err.h"

/* List Libraries */
static int vm_completion_lib_dir(struct multiple_error *err)
{
    int ret = 0;
    char lib_path[PATHNAME_LEN_MAX];
    size_t lib_path_len;

    /* Get library directory */
    if ((ret = get_library_dir_path(err, lib_path, PATHNAME_LEN_MAX)) != 0) 
    { goto fail; }
    lib_path_len = strlen(lib_path);

    fwrite(lib_path, lib_path_len, 1, stdout);
    printf("\n");

    goto done;
fail:
done:
    return ret;
}

/* Wrapped entrance routine for completion */
int virtual_machine_completion(struct multiple_error *err, \
        struct vm_err *r, \
        struct multiple_ir *icode, \
        struct virtual_machine_startup *startup, \
        int debug, \
        struct multiple_stub_function_list *external_functions, \
        const char *completion_cmd)
{
    size_t completion_cmd_len;

    (void)r;
    (void)icode;
    (void)startup;
    (void)debug;
    (void)external_functions;

    completion_cmd_len = strlen(completion_cmd);

    if ((completion_cmd_len == strlen("lib-dir")) && \
        (strncmp(completion_cmd, "lib-dir", completion_cmd_len) == 0))
    {
        vm_completion_lib_dir(err);
    }
    return 0;
}

