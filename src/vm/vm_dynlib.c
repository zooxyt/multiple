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

#include "selfcheck.h"

#ifndef UNIX
#ifndef WINDOWS

#if defined(_MSC_VER)
#define WINDOWS
#elif defined(WIN32)
#define WINDOWS
#elif defined(linux)
#define UNIX
#endif

#endif
#endif

#include <stdio.h>
#include <string.h>

#if defined(UNIX)
#include <dlfcn.h>
#elif defined(WINDOWS)

#ifdef _MSC_VER
#pragma warning(push, 1)
#endif

#include <windows.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif

#include "multiple_tunnel.h"
#include "multiple_misc.h"
#include "multiple_err.h"
#include "vm_infrastructure.h"
#include "vm_res.h"
#include "vm_err.h"
#include "vm_dynlib.h"

static int virtual_machine_shared_library_list_append_with_configure(struct virtual_machine *vm, struct virtual_machine_shared_library_list *list, char *name, size_t len, void *handle)
{
    int ret = 0;
    struct virtual_machine_shared_library *new_lib;

    new_lib = virtual_machine_shared_library_new_with_configure(vm, name, len, handle);
    if (new_lib == NULL) { ret = -MULTIPLE_ERR_MALLOC; goto fail; }

    virtual_machine_shared_library_list_append(list, new_lib);
    new_lib = NULL;

    goto done;
fail:
    if (new_lib != NULL) virtual_machine_shared_library_destroy(vm, new_lib);
done:

    return ret;
}


static int virtual_machine_dynlib_invoke_pathname(struct virtual_machine *vm, \
        struct multiple_stub_function_args *args, char *pathname_lib, char *name_func)
{

    /* Fill rail part of args */
    args->rail = vm->r;

#if defined(UNIX)

    int ret = 0;

    void *dl_handle;
    char *error;
    int (*func)(struct multiple_stub_function_args *args) = NULL;

    /* Check if shared library has been loaded */
    if (virtual_machine_shared_library_list_lookup(args->vm->shared_libraries, &dl_handle, pathname_lib, strlen(pathname_lib)) == 0)
    {
        dl_handle = dlopen(pathname_lib, RTLD_NOW | RTLD_GLOBAL);
        if (dl_handle == NULL)
        {
            vm_err_update(vm->r, -VM_ERR_DLCALL, \
                    "runtime error: error occurred while loading \"%s\", %s", pathname_lib, dlerror());
            return -MULTIPLE_ERR_VM;
        }
        ret = virtual_machine_shared_library_list_append_with_configure(vm, args->vm->shared_libraries, pathname_lib, strlen(pathname_lib), dl_handle);
        if (ret != 0)
        {
            vm_err_update(vm->r, -VM_ERR_DLCALL, \
                    "runtime error: error occurred while appending shared library \"%s\" into virtual machine", pathname_lib);
            return -MULTIPLE_ERR_VM;
        }
    }

    /* Resolve function */
    func = dlsym(dl_handle, name_func);
    error = dlerror();
    if (error != NULL) 
    {
        vm_err_update(vm->r, -VM_ERR_DLCALL, \
                "runtime error: error occurred while resolving function \"%s\"", name_func);
        return -MULTIPLE_ERR_VM;
    }

    /* Execute the function */
    ret = func(args);

    return ret;

#elif defined(WINDOWS)

	int ret = 0;
	HINSTANCE hDllInst = NULL;
    HINSTANCE *hDllInst_p = NULL;
	int (*func)(struct multiple_stub_function_args *args) = NULL;

    /* Check if shared library has been loaded */
    if (virtual_machine_shared_library_list_lookup(args->vm->shared_libraries, (void *)&hDllInst_p, pathname_lib, strlen(pathname_lib)) == 0)
    {
        hDllInst = LoadLibraryA(pathname_lib);
        if (hDllInst == NULL)
        {
            vm_err_update(vm->r, -VM_ERR_DLCALL, \
                    "runtime error: error occurred while loading \"%s\"", pathname_lib);
            return -MULTIPLE_ERR_VM;
        }
        hDllInst_p = (void *)virtual_machine_resource_malloc(vm->resource, sizeof(HINSTANCE));
        if (hDllInst_p == NULL)
        {
            vm_err_update(vm->r, -VM_ERR_DLCALL, \
                    "runtime error: error occurred while loading \"%s\"", pathname_lib);
            return -MULTIPLE_ERR_VM;
        }
        *hDllInst_p = hDllInst;
        ret = virtual_machine_shared_library_list_append_with_configure(vm, args->vm->shared_libraries, pathname_lib, strlen(pathname_lib), (void *)hDllInst_p);
        if (ret != 0)
        {
            vm_err_update(vm->r, -VM_ERR_DLCALL, \
                    "runtime error: error occurred while appending shared library \"%s\" into virtual machine", pathname_lib);
            return -MULTIPLE_ERR_VM;
        }
    }
    else
    {
        hDllInst = *hDllInst_p;
    }

    /* Resolve function */
	func = (int (*)(struct multiple_stub_function_args *args))GetProcAddress(hDllInst, name_func);
	if (func == NULL)
	{
        vm_err_update(vm->r, -VM_ERR_DLCALL, \
                "runtime error: error occurred while resolving function \"%s\"", name_func);
        /* Windows works the same as 'dlopen' in Linux?
         * FreeLibrary(hDllInst);
         */
        return -MULTIPLE_ERR_VM;
	}

    /* Execute the function */
	ret = func(args);

	return ret;
#else
    (void)pathname_lib;
    (void)name_func;
	vm_err_update(vm->r, -MULTIPLE_ERR_VM, \
            "runtime error: operating system not defined");
	return -MULTIPLE_ERR_VM;
#endif
}

int virtual_machine_dynlib_close_handle(struct virtual_machine *vm, void *handle)
{
    (void)vm;

    /* Keep loaded dll for debug symbols */
    if (vm->keep_dll != 0)
    { return 0; }

#if defined(UNIX)
    void *dl_handle;
    dl_handle = handle;

    /* When shared objects been unloaded, memory leak detect tools can't
     * Show the debug info in the share objects */ 
    dlclose(dl_handle);
    
    return 0;
#elif defined(WINDOWS)
	HINSTANCE hDllInst = NULL;

    hDllInst = *(HINSTANCE*)handle;
    FreeLibrary(hDllInst);
    virtual_machine_resource_free(vm->resource, handle);

    return 0;
#else
    (void)handle;
    vm_err_update(vm->r, -MULTIPLE_ERR_VM, \
            "runtime error: operating system not defined");
    return -MULTIPLE_ERR_VM;
#endif
}

int virtual_machine_dynlib_invoke(struct virtual_machine *vm, struct multiple_stub_function_args *args, char *name_lib, char *name_func)
{
    int ret = 0;
    struct multiple_error *err = NULL;
    char lib_path[PATHNAME_LEN_MAX];
    char dynlib_path[PATHNAME_LEN_MAX];

    err = multiple_error_new();
    if (err == NULL) 
    {
        MULTIPLE_ERROR_MALLOC();
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }
    multiple_error_clear(err);

    if ((ret = get_library_dir_path(err, lib_path, PATHNAME_LEN_MAX)) != 0) 
    { goto fail; }

    /* try libpath/<dynlib>.[so|dll] */
#if defined(snprintf)
    snprintf(dynlib_path, PATHNAME_LEN_MAX, "%s%s.%s", lib_path, name_lib, OS_DYNLIB_EXT);
#else 
    sprintf(dynlib_path, "%s%s.%s", lib_path, name_lib, OS_DYNLIB_EXT);
#endif
    /* Chech file existent */
    if (is_file_exists(dynlib_path))
    {
        ret = virtual_machine_dynlib_invoke_pathname(vm, args, dynlib_path, name_func);
        if (ret != 0) goto fail;
    }
    else
    {
        vm_err_update(vm->r, -VM_ERR_DLCALL, \
                "error: error occurred while loading \"%s\"", name_lib);
        ret = -MULTIPLE_ERR_VM;
        goto fail;
    }
fail:
    multiple_error_destroy(err);
    return ret;
}

