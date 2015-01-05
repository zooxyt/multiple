/* Miscellaneous
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

#ifndef _MULTIPLE_MISC_H_
#define _MULTIPLE_MISC_H_

#include "selfcheck.h"

#include <stdio.h>
#include <stdlib.h>

#include "multiple_err.h"

/* Read code text from file */
int read_file(struct multiple_error *err, char **str, size_t *len, const char *pathname);

/* Print string and replace escape chars with readable format */
int print_escaped_string(FILE *fp_out, const char *str, const size_t len);

#define LIB_DIR_NAME "lib"

#ifdef PATH_MAX
#define PATHNAME_LEN_MAX PATH_MAX
#else
#define PATHNAME_LEN_MAX 255
#endif

/* Operating System's Default directory separator */
#if defined(UNIX)
#define OS_SEP_STR "/"
#define OS_SEP_CHAR '/'
#define OS_DYNLIB_EXT "so"
#elif defined(WINDOWS)
#define OS_SEP_STR "\\"
#define OS_SEP_CHAR '\\'
#define OS_DYNLIB_EXT "dll"
#else
#define OS_SEP_STR "/"
#define OS_SEP_CHAR '/'
#define OS_DYNLIB_EXT "so"
#endif

/* Library directory path */
#define LIB_DIR LIB_DIR_NAME OS_SEP_STR

/* Extract directory part of a path 
 * "/lib/libc.so" -> "/lib/" */
int dirname_get(char **dirname_p, size_t *dirname_len, const char *pathname, const size_t pathname_len);

/* Extract filename part of a path 
 * "/lib/libc.so" -> "libc.so" */
int basename_get(char **basename_p, size_t *basename_len, const char *pathname, const size_t pathname_len);

/* Extract extension of a path
 * "libc.so" -> "so" */
int extension_get(char **extension_p, size_t *extension_len, const char *pathname, const size_t pathname_len);

/* Get current working directory */
int current_working_dir_get(char *buffer);
 
/* Check if the volume exists
 * RETURN: 1 for exists, 0 for not exists, -1 for error */
int is_vol_exists(const char vol);

/* Check if the file exists
 * RETURN: 1 for exists, 0 for not exists, -1 for error */
int is_file_exists(const char *pathname);

/* Check if the directory exists
 * RETURN: 1 for exists, 0 for not exists, -1 for error */
int is_dir_exists(const char *pathname);

/* Check if the file or directory exists
 * RETURN: 1 for exists, 0 for not exists, -1 for error */
int is_file_dir_exists(const char *pathname);

/* Concatenate Path */
int concat_path(char *dst_path, size_t *dst_path_len,
        const char *src_path_1, size_t src_path_1_len,
        const char *src_path_2, size_t src_path_2_len);

/* Get exe path */
int get_exe_path(struct multiple_error *err, char *buffer, size_t buffer_len);

/* Get Library directory */
int get_library_dir_path(struct multiple_error *err, char *buffer, size_t buffer_len);

/* Convert string into size (in bytes) */
int size_atoin(long *size_ptr, const char *str, const size_t len);

/* Take a break */
void do_rest(void);

#endif 

