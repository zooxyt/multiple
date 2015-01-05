/* Main Routine
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
#include <stdio.h>
#include <string.h>

#include "argsparse.h"
#include "multiple.h"
#include "multiple_err.h"
#include "multiple_misc.h"
#include "multiple_version.h"
#include "vm_err.h"

/* Global Variable for passing command line arguments */
extern const char **g_argv;
extern int g_argc;
const char **g_argv;
int g_argc;

static int show_version(void)
{
    const char *info = "" 
        "multiple - MULTIple Paradigm Language Emulator " _MULTIPLE_VERSION_ " (" _MULTIPLE_MARK_ ")" "\n"
        "Copyright(C) 2013-2014 Cheryl Natsu\n";
    puts(info);
    return 0;
}

static int show_build_info(void)
{
    const char *info = "" 
		"compiler : " COMPILER "\n"
		"date     : " __DATE__ "\n"
        "time     : " __TIME__ "\n";
    puts(info);
    return 0;
}

static int show_experimental(void)
{
    const char *info = ""
    "Experimental Functions:\n"
    "  -it, --internal-tokens        Print tokens\n"
    "  -ir, --internal-reconstract   Reconstruct source\n"
    "  --keep-dll                    Do not unload dynamic libraries\n"
    "  --stack-size <size>           Set stack size (default:100)\n"
    "  -tco, --tail-call-optimize    Enable Tail Call Optimization\n";
    puts(info);
    return 0;
}

static int show_help(void)
{
    const char *info = ""
    "Usage : multiple [options] <file> [arguments]\n"
    "\n"
    "General Options:\n"
    "  -c <file>                     Source code file\n"
    "  -o <file>                     Output file (default:stdout)\n"
    "  -g                            Include Debug Information\n"
    "Frontend Options:\n"
    "  -f, --frontend <frontend>     Specify frontend (default:auto)\n"
    "  -lf, --list-frontends         List frontends\n"
    "Backend Options:\n"
    "  -r, --run                     Virtual Machine (default)\n"
    "  -b                            Bytecode\n"
    "  -S                            Assembly language\n"
    "  -d, --debug                   Debugger\n"
    "Optimization Options:\n"
    "  -O<num>                       Enable Optimization\n"
    "Virtual Machine Options:\n"
    "  Memory Usage:\n"
    "      --vm-mem <item> <type> <size>\n"
    "        item: [infrastructure|primitive|reference]\n"
    "        type: [default|libc|4k|64b|128b]\n"
    "        size: (0 for unlimited)\n"
    "Additions:\n"
    "  --completion <cmd>            Completion\n"
    "\n"
    "  --help                        Show help information\n"
    "  --build-info                  Show build information\n"
    "  --experimental                Show experimental functions\n"
    "  --version                     Show version information\n";
    show_version();
    puts(info);
    return 0;
}

#define WORKING_MODE_TOKENS 1
#define WORKING_MODE_RECONSTRUCT 2
#define WORKING_MODE_BYTECODE 3
#define WORKING_MODE_ASM 4
#define WORKING_MODE_RUN 5
#define WORKING_MODE_DEBUG 6
#define WORKING_MODE_COMPLETION 7
#define WORKING_MODE_LIST_FRONTENDS 8

static int multiple_io_type_from_str(const char *pathname, const size_t pathname_len)
{
    if (strncmp(pathname, "stdin", pathname_len) == 0)
    { return MULTIPLE_IO_STDIN; }
    else if (strncmp(pathname, "stdout", pathname_len) == 0)
    { return MULTIPLE_IO_STDOUT; }
    else if (strncmp(pathname, "stderr", pathname_len) == 0)
    { return MULTIPLE_IO_STDERR; }

    return MULTIPLE_IO_PATHNAME; 
}

int main(int argc, const char *argv[])
{
    int ret = 0;

    struct multiple_stub *stub = NULL;
    struct multiple_error *err = NULL;
    struct vm_err r;

    char *pathname_src = NULL; int type_src = MULTIPLE_IO_NULL;
    char *pathname_dst = NULL; int type_dst = MULTIPLE_IO_NULL;
    char *frontend = NULL;

    int opt_working_mode = WORKING_MODE_BYTECODE;
    int opt_debug_info = 0;
    int opt_optimize = 0;
    int opt_keep_dll = 0;

    char *mem_item;
    char *mem_type;
    char *mem_size;

    char *completion_cmd = NULL;

    /* Arguments */
    g_argv = argv;
    g_argc = argc;

    /* Clear Error */
    if ((err = multiple_error_new()) == NULL)
    {
        fprintf(stderr, "error: failed to create error handler");
        exit(1);
    }
    multiple_error_clear(err);
    vm_err_init(&r);

    opt_working_mode = WORKING_MODE_RUN;

    /* Quick launch a script */
    if (argc == 2 && argv[1][0] != '-')
    {
        pathname_src = (char *)argv[1];
        type_src = multiple_io_type_from_str(pathname_src, strlen(pathname_src));
    }
    else
    {
        /* Argument Parser */
        int arg_idx;
        char *arg_p;
        argsparse_init(&arg_idx);

        if (argc == 1)
        {
            show_help();
            goto done;
        }
        while (argsparse_request(argc, argv, &arg_idx, &arg_p) == 0)
        {
            if (!strcmp(arg_p, "--version"))
            { show_version(); goto done; }
            else if (!strcmp(arg_p, "--build-info"))
            { show_build_info(); goto done; }
            else if (!strcmp(arg_p, "--experimental"))
            { show_experimental(); goto done; }
            else if (!strcmp(arg_p, "--help"))
            { show_help(); goto done; }
            else if (!strcmp(arg_p, "-c"))
            {
                if (argsparse_request(argc, argv, &arg_idx, &arg_p) != 0)
                {
                    multiple_error_update(err, -MULTIPLE_ERR_INVALID_ARG, "error: invalid argument");
                    goto fail;
                }
                pathname_src = arg_p;
                type_src = multiple_io_type_from_str(arg_p, strlen(arg_p));
                break;
            }
            else if (!strcmp(arg_p, "-o"))
            {
                if (argsparse_request(argc, argv, &arg_idx, &arg_p) != 0)
                {
                    multiple_error_update(err, -MULTIPLE_ERR_INVALID_ARG, "error: invalid argument");
                    goto fail;
                }
                pathname_dst = arg_p;
                type_dst = multiple_io_type_from_str(arg_p, strlen(arg_p));
            }
            else if (!strcmp(arg_p, "-g"))
            {
                opt_debug_info = 1;
            }
            else if ((!strcmp(arg_p, "-f")) || (!strcmp(arg_p, "--frontend")))
            {
                if (argsparse_request(argc, argv, &arg_idx, &arg_p) != 0)
                {
                    multiple_error_update(err, -MULTIPLE_ERR_INVALID_ARG, "error: invalid argument");
                    goto fail;
                }
                frontend = arg_p;
            }
            else if (!strcmp(arg_p, "-S"))
            { opt_working_mode = WORKING_MODE_ASM; }
            else if ((!strcmp(arg_p, "-it")) || (!strcmp(arg_p, "--internal-tokens")))
            { opt_working_mode = WORKING_MODE_TOKENS; }
            else if ((!strcmp(arg_p, "-ir")) || (!strcmp(arg_p, "--internal-reconstruct")))
            { opt_working_mode = WORKING_MODE_RECONSTRUCT; }
            else if ((!strcmp(arg_p, "-r")) || (!strcmp(arg_p, "--run")))
            { opt_working_mode = WORKING_MODE_RUN; }
            else if ((!strcmp(arg_p, "-b")) || (!strcmp(arg_p, "--bytecode")))
            { opt_working_mode = WORKING_MODE_BYTECODE; }
            else if ((!strcmp(arg_p, "-d")) || (!strcmp(arg_p, "--debug")))
            { opt_working_mode = WORKING_MODE_DEBUG; }
            else if (!strcmp(arg_p, "--keep-dll"))
            { opt_keep_dll = 1; }
            else if (!strcmp(arg_p, "--completion"))
            { 
                if (argsparse_request(argc, argv, &arg_idx, &completion_cmd) != 0)
                { multiple_error_update(err, -MULTIPLE_ERR_INVALID_ARG, "error: invalid argument"); goto fail; }
                opt_working_mode = WORKING_MODE_COMPLETION;
            }
            else if ((!strcmp(arg_p, "-lf")) || (!strcmp(arg_p, "--list-frontends")))
            { 
                opt_working_mode = WORKING_MODE_LIST_FRONTENDS; 
            }
            else if ((!strcmp(arg_p, "-O")) || (!strcmp(arg_p, "--optimize")))
            { opt_optimize = 1; }
            else if ((!strcmp(arg_p, "--vm-mem")) || (!strcmp(arg_p, "-m")))
            {
                if (argsparse_request(argc, argv, &arg_idx, &mem_item) != 0)
                { multiple_error_update(err, -MULTIPLE_ERR_INVALID_ARG, "error: invalid argument"); goto fail; }
                if (argsparse_request(argc, argv, &arg_idx, &mem_type) != 0)
                { multiple_error_update(err, -MULTIPLE_ERR_INVALID_ARG, "error: invalid argument"); goto fail; }
                if (argsparse_request(argc, argv, &arg_idx, &mem_size) != 0)
                { multiple_error_update(err, -MULTIPLE_ERR_INVALID_ARG, "error: invalid argument"); goto fail; }

                if ((ret = multiple_stub_virtual_machine_memory_usage(err, stub, mem_item, mem_type, mem_size)) != 0)
                { goto fail; }
            }
            else if (is_file_exists(arg_p))
            {
                /* Source code file ? */
                pathname_src = arg_p;
                type_src = multiple_io_type_from_str(arg_p, strlen(arg_p));
                break;
            }
            else
            {
                multiple_error_update(err, -MULTIPLE_ERR_INVALID_ARG, "error: invalid argument");
                goto fail;
            }
        }
        if (pathname_src == NULL)
        {
            switch (opt_working_mode)
            {
                case WORKING_MODE_COMPLETION:
                    break;
                case WORKING_MODE_LIST_FRONTENDS:
                    break;
                default:
                    multiple_error_update(err, -MULTIPLE_ERR_INVALID_ARG, "error: no specified source code file");
                    goto fail;
            }
        }

        if (pathname_dst != NULL)
        {
            if (pathname_src != NULL)
            {
                if (strcmp(pathname_dst, pathname_src) == 0)
                {
                    multiple_error_update(err, -MULTIPLE_ERR_INVALID_ARG, "error: output file name is the same as input file name");
                    goto fail;
                }
            }
        }

        /* Update Arguments */
        g_argv++;
        g_argc--;
    }

    if (opt_working_mode == WORKING_MODE_LIST_FRONTENDS)
    {
        if ((ret = multiple_stub_list_frontends(err)) != 0)
        { goto fail; }
        goto done;
    }

    if ((ret = multiple_stub_create(err, &stub, \
                    pathname_dst, type_dst, \
                    pathname_src, type_src, \
                    frontend)) != 0) { goto fail; }

    /* Debug Info */
    if (opt_debug_info)
    {
        if ((ret = multiple_stub_debug_info_set(err, stub, opt_debug_info)) != 0) { goto fail; }
    }
    /* Optimize */
    if (opt_optimize)
    {
        if ((ret = multiple_stub_optimize_set(err, stub, opt_optimize)) != 0) { goto fail; }
    }
    /* Keep DLL */
    if (opt_keep_dll)
    {
        stub->startup.keep_dll = 1;
    }

    switch (opt_working_mode)
    {
        case WORKING_MODE_TOKENS:
            if ((ret = multiple_stub_tokens_print(err, stub)) != 0) goto fail;
            break;
        case WORKING_MODE_RECONSTRUCT:
            if ((ret = multiple_stub_reconstruct(err, stub)) != 0) goto fail;
            break;
        case WORKING_MODE_ASM:
            if ((ret = multiple_stub_asm_code_gen(err, stub)) != 0) goto fail;
            break;
        case WORKING_MODE_BYTECODE:
            if (pathname_dst == NULL)
            {
                multiple_error_update(err, -MULTIPLE_ERR_INVALID_ARG, "error: no specified destination file");
                goto fail;
            }
            if ((ret = multiple_stub_bytecode(err, stub)) != 0) goto fail;
            break;
        case WORKING_MODE_RUN:
            if ((ret = multiple_stub_run(err, &r, stub)) != 0) goto fail;
            if (multiple_error_occurred(err)) goto fail;
            else if (vm_err_occurred(&r)) goto fail;
            break;
        case WORKING_MODE_DEBUG:
            if ((ret = multiple_stub_debug(err, &r, stub)) != 0) goto fail;
            if (multiple_error_occurred(err)) goto fail;
            else if (vm_err_occurred(&r)) goto fail;
            break;
        case WORKING_MODE_COMPLETION:
            if ((ret = multiple_stub_completion(err, stub, completion_cmd)) != 0) goto fail;
            break;
        default:
            break;
    }

    goto done;
fail:
    if (ret != 0) multiple_error_final(err, ret);

    if (vm_err_occurred(&r))
    {
        vm_err_print(&r);
        ret = -1;
    }
    else if (multiple_error_occurred(err))
    {
        multiple_error_print(err);
        ret = -1;
    }
done:
    if (err != NULL) multiple_error_destroy(err);
    if (stub != NULL) multiple_stub_destroy(stub);
    return ret;
}

