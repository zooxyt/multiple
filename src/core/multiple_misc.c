/* Miscellaneous
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

#if defined(UNIX)
/* 'lstat' */
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#elif defined(WINDOWS)

#ifdef _MSC_VER
#pragma warning(push, 1)
#endif

#include <windows.h>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "multiple_err.h"
#include "multiple_misc.h"
#include "multiple_general.h"

/* Read code text from file */
int read_file(struct multiple_error *err, char **str, size_t *len, const char *pathname)
{
    int ret = 0;
    FILE *fp = NULL;
    char *file_data = NULL, *file_data_p;
    long file_size;
    size_t remain_size, task_size, read_size;

    *str = NULL;
    *len = 0;

    /* Open file */
    if ((fp = fopen(pathname, "rb")) == NULL)
    {
        multiple_error_update(err, -MULTIPLE_ERR_ATOMIC, "error: opening %s for reading failed", pathname);
        ret = -MULTIPLE_ERR_ATOMIC;
        goto fail;
    }

    /* Get file size */
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if ((file_data = (char *)malloc(sizeof(char) * (size_t)file_size)) == NULL)
    {
        MULTIPLE_ERROR_MALLOC();
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }

    /* Read data */
    remain_size = (size_t)file_size;
    file_data_p = file_data;
    while (remain_size > 0)
    {
        task_size = MIN(remain_size, BUFFER_SIZE);
        if ((read_size = fread(file_data_p, task_size, 1, fp)) < 1)
        {
            multiple_error_update(err, -MULTIPLE_ERR_ATOMIC, "error: reading data from %s failed", pathname);
            ret = -MULTIPLE_ERR_ATOMIC;
            goto fail;
        }
        file_data_p += task_size;
        remain_size -= task_size;
    }

    /* Finished reading, now can close the file */
    fclose(fp); fp = NULL;

    *str = file_data;
    *len = (size_t)file_size;

    /* Finished processing */
    ret = 0;

    goto done;
fail:
    if (file_data != NULL) free(file_data);
done:
    if (fp != NULL) fclose(fp);
    return ret;
}

/* Print string and replace escape chars with readable format */
int print_escaped_string(FILE *fp_out, const char *str, const size_t len)
{
    const char *str_p = str, *str_endp = str_p + len;
    while (str_p != str_endp)
    {
        switch (*str_p)
        {
            case '\a': fputc('\\', fp_out); fputc('a', fp_out); break;
            case '\b': fputc('\\', fp_out); fputc('b', fp_out); break;
            case '\f': fputc('\\', fp_out); fputc('f', fp_out); break;
            case '\n': fputc('\\', fp_out); fputc('n', fp_out); break;
            case '\r': fputc('\\', fp_out); fputc('r', fp_out); break;
            case '\t': fputc('\\', fp_out); fputc('t', fp_out); break;
            case '\v': fputc('\\', fp_out); fputc('v', fp_out); break;
            case '\\': fputc('\\', fp_out); fputc('\\', fp_out); break;
            case '\?': fputc('\\', fp_out); fputc('?', fp_out); break;
            case '\'': fputc('\\', fp_out); fputc('\'', fp_out); break;
            case '\"': fputc('\\', fp_out); fputc('\"', fp_out); break;
            case '\0': fputc('\\', fp_out); fputc('0', fp_out); break;
            default: fputc(*str_p, fp_out); break;
        }
        str_p++;
    }
    return 0;
}


/* Extract directory part of a path 
 * "/lib/libc.so" -> "/lib/" */
int dirname_get(char **dirname_p, size_t *dirname_len, const char *pathname, const size_t pathname_len)
{
    const char *pathname_p;

    *dirname_p = NULL;
    *dirname_len = 0;

    if (pathname_len == 0) return 0;

    pathname_p = pathname + pathname_len;
    do
    {
        pathname_p--;
        if ((*pathname_p == '/') || (*pathname_p == '\\'))
        {
            *dirname_p = (char *)pathname;
            *dirname_len = (size_t)(pathname_p - pathname) + 1;
            return 0;
        }
    } while (pathname_p != pathname);

    return 0;
}

/* Extract filename part of a path 
 * "/lib/libc.so" -> "libc.so" */
int basename_get(char **basename_p, size_t *basename_len, const char *pathname, const size_t pathname_len)
{
    const char *pathname_p;

    if (pathname_len == 0) return 0;

    if (strchr(pathname, '/') == NULL)
    {
        *basename_p = (char *)pathname;
        *basename_len = (size_t)pathname_len;
        return 0;
    }

    *basename_p = NULL;
    *basename_len = 0;
    pathname_p = pathname + pathname_len;
    do
    {
        pathname_p--;
        if ((*pathname_p == '/') || (*pathname_p == '\\'))
        {
            *basename_p = (char *)pathname_p + 1;
            *basename_len = (size_t)((pathname + pathname_len) - pathname_p - 1); /* Is it safe? */
            return 0;
        }
    } while (pathname_p != pathname);
    return 0;
}

/* Extract extension of a path
 * "libc.so" -> "so" */
int extension_get(char **extension_p, size_t *extension_len, const char *pathname, const size_t pathname_len)
{
    const char *pathname_p, *pathname_endp;

    if (pathname_len == 0) return 0;

    *extension_p = NULL;
    *extension_len = 0;
    pathname_p = pathname;
    pathname_endp = pathname_p + pathname_len;
    while (pathname_p != pathname_endp)
    {
        if (*pathname_p == '.')
        {
            *extension_p = (char *)pathname_p + 1;
            *extension_len = 0;
        }
        else if ((*pathname_p == '/') || (*pathname_p == '\\'))
        {
            *extension_p = NULL;
            *extension_len = 0;
        }
        pathname_p++;

        if (*extension_p != NULL) *extension_len += 1;
    }
    *extension_len -= 1;
    return 0;
}


/* Get current working directory */
int current_working_dir_get(char *buffer)
{
#if defined(UNIX)
	if (getcwd(buffer, PATHNAME_LEN_MAX) != NULL) return 0;
	else return -1;
#elif defined(WINDOWS)
	if (GetCurrentDirectoryA(PATHNAME_LEN_MAX, buffer) > 0) return 0;
	else return -1;
#else
	(void)buffer;
	return -1;
#endif
}

/* Check if the file exists
 * RETURN: 1 for exists, 0 for not exists, -1 for error */
#if defined(UNIX)
int is_file_exists(const char *pathname)
{
    struct stat buf;

    if (lstat(pathname, &buf) != 0) return 0;
    return ((buf.st_mode & S_IFMT) == S_IFREG) ? 1 : 0;
}
#elif defined(WINDOWS)
int is_file_exists(const char *pathname)
{
    WIN32_FIND_DATAA wfd;
    int ret = 0;
    HANDLE hFind; 

    if (pathname == NULL) return -MULTIPLE_ERR_NULL_PTR;

    hFind = FindFirstFileA(pathname, &wfd);
    if ((hFind != INVALID_HANDLE_VALUE) && ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0))
    {
        ret = 1;
    }
    FindClose(hFind);
    return ret;
}
#else
int is_file_exists(const char *pathname)
{
	(void)pathname;
	return -1;
}
#endif

/* Check if the volume exists
 * RETURN: 1 for exists, 0 for not exists, -1 for error */
#if defined(UNIX)
int is_vol_exists(const char vol)
{
    (void)vol;
    return 0;
}
#elif defined(WINDOWS)
int is_vol_exists(const char vol)
{
    DWORD drvs;
    unsigned int vol_bit;

    if (((char)('A') <= vol) && (vol <= (char)('Z'))) vol_bit = (unsigned int)vol - (unsigned int)('A');
    else if (((char)('a') <= vol) && (vol <= (char)('z'))) vol_bit = (unsigned int)vol - (unsigned int)('a');
	else return -1;
    drvs = GetLogicalDrives();
    if (drvs == 0) return -1;
    if (drvs & (DWORD)(1 << vol_bit))
    {
        return 1;
    }
    else
    {
        return 0;
    }
}
#endif

/* Check if the directory exists
 * RETURN: 1 for exists, 0 for not exists, -1 for error */
#if defined(UNIX)
int is_dir_exists(const char *pathname)
{
    struct stat buf;
    if (lstat(pathname, &buf) != 0) return 0;
    return ((buf.st_mode & S_IFMT) == S_IFDIR) ? 1 : 0;
}
#elif defined(WINDOWS)
int is_dir_exists(const char *pathname)
{
    WIN32_FIND_DATAA wfd;
    int ret = 0;
    HANDLE hFind;
    size_t pathname_len;
    char pathname_without_tail[PATHNAME_LEN_MAX];

    if (pathname == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if ((strlen(pathname) == 3) && \
            (pathname[1] == ':') && (pathname[2] == '\\') && \
            ((('A' <= pathname[0]) && (pathname[0] <= 'Z')) || \
             (('a' <= pathname[0]) && (pathname[0] <= 'z'))))
    {
        /* Volume */
        return is_vol_exists((char)pathname[0]);
    }
    else
    {
        /* Construct a full path without that tail '\' */
        pathname_len = strlen(pathname);
        if (pathname_len == 0) return 0;

        memcpy(pathname_without_tail, pathname, pathname_len);
        if ((pathname[pathname_len - 1] == '\\') || (pathname[pathname_len - 1] == '/'))
        {
            pathname_without_tail[pathname_len - 1] = '\0';
        }
        else
        {
            pathname_without_tail[pathname_len] = '\0';
        }
        hFind = FindFirstFileA(pathname_without_tail, &wfd);
        if ((hFind != INVALID_HANDLE_VALUE) && (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
        {
            ret = 1;
        }
        FindClose(hFind);
        return ret;
    }

}
#else
int is_dir_exists(const char *pathname)
{
    (void)pathname;
    return 0;
}
#endif

/* Concat Path */
int concat_path(char *dst_path, size_t *dst_path_len,
        const char *src_path_1, size_t src_path_1_len,
        const char *src_path_2, size_t src_path_2_len)
{
    *dst_path = '\0';
    *dst_path_len = 0;
    if ((src_path_1_len >= 1) && (src_path_1[src_path_1_len - 1] == '/')
            && (src_path_2_len >= 1) && (src_path_2[0] == '/'))
    {
        /* Skip '/' */
        src_path_1_len--;
    }
    memcpy(dst_path, src_path_1, src_path_1_len);
    memcpy(dst_path + src_path_1_len, src_path_2, src_path_2_len);
    *dst_path_len = src_path_1_len + src_path_2_len;
    dst_path[*dst_path_len] = '\0';
    return 0;
}

/* Get exe path */
int get_exe_path(struct multiple_error *err, char *buffer, size_t buffer_len)
{
#if defined(UNIX)
    char link[PATHNAME_LEN_MAX];
    ssize_t link_len;
    if (is_dir_exists("/proc") == 1)
    {
        snprintf(link, PATHNAME_LEN_MAX, "/proc/%d/exe", getpid());  
        link_len = readlink(link, buffer, buffer_len);  
        buffer[link_len] = '\0';
    }
    else
    {
        multiple_error_update(err, -MULTIPLE_ERR_ATOMIC, "error: \'/proc\' directory doesn't exists");
        return -MULTIPLE_ERR_ATOMIC;
    }
    return 0;
#elif defined(WINDOWS)
    DWORD ret;
	(void)err;
    ret = GetModuleFileNameA(NULL, (LPSTR)buffer, (DWORD)buffer_len); 
    buffer[(size_t)ret] = '\0';
    return 0;
#else
	(void)buffer_len;
	(void)buffer;
    multiple_error_update(err, -MULTIPLE_ERR_ATOMIC, "error: operating System not defined");
    return -MULTIPLE_ERR_ATOMIC;
#endif
}

/* Get Library directory */
int get_library_dir_path(struct multiple_error *err, char *buffer, size_t buffer_len)
{
    int ret = 0;

    char exe_path[PATHNAME_LEN_MAX];
    size_t exe_path_len;
    char *exe_dir_path = NULL;
    size_t exe_dir_path_len;
    char conf_dir_path[PATHNAME_LEN_MAX];
    size_t conf_dir_path_len;

    if ((ret = get_exe_path(err, exe_path, PATHNAME_LEN_MAX)) != 0)
    {
        goto fail;
    }
    exe_path_len = strlen(exe_path);

    if ((ret = dirname_get(&exe_dir_path, &exe_dir_path_len, exe_path, exe_path_len)) != 0)
    {
        multiple_error_update(err, -MULTIPLE_ERR_ATOMIC, "error: extracting directory name failed");
        ret = -MULTIPLE_ERR_ATOMIC;
        goto fail;
    }
    if (exe_dir_path == NULL)
    {
        MULTIPLE_ERROR_INTERNAL();
        ret = -MULTIPLE_ERR_ATOMIC;
        goto fail;
    }

    if ((ret = concat_path(conf_dir_path, &conf_dir_path_len, \
            exe_dir_path, exe_dir_path_len, \
            LIB_DIR, strlen(LIB_DIR))) != 0)
    {
        multiple_error_update(err, -MULTIPLE_ERR_ATOMIC, "error: concating path failed");
        ret = -MULTIPLE_ERR_ATOMIC;
        goto fail;
    }
    if (conf_dir_path_len < buffer_len)
    {
        /* Enough space to contain the path */
        memcpy(buffer, conf_dir_path, conf_dir_path_len);
        buffer[conf_dir_path_len] = '\0';
    }
    else
    {
        multiple_error_update(err, -MULTIPLE_ERR_ATOMIC, "error: no enough space for containing lib directory path");
        ret = -MULTIPLE_ERR_ATOMIC;
        goto fail;
    }
    ret = 0;
    goto done;
fail:
done:
    return ret;
}

/* A size conversion function supports "KB, MB, GB" */
/* [0-9]+[kKmMgGtT]?([bB](yte(s)?)?)? */

/* Status Definations */
#define SIZE_ATOI_STATUS_INIT 0
#define SIZE_ATOI_STATUS_NUMBER 1
#define SIZE_ATOI_STATUS_K 2
#define SIZE_ATOI_STATUS_M 3
#define SIZE_ATOI_STATUS_G 4
#define SIZE_ATOI_STATUS_B 5
#define SIZE_ATOI_STATUS_BY 6
#define SIZE_ATOI_STATUS_BYT 7
#define SIZE_ATOI_STATUS_BYTE 8
#define SIZE_ATOI_STATUS_BYTES 9
#define SIZE_ATOI_STATUS_ERROR -1

/* Conversation Standards */
#define KB_SIZE (1 << 10)
#define MB_SIZE (1 << 20)
#define GB_SIZE (1 << 30)

int size_atoin(long *size_ptr, const char *str, const size_t len)
{
    long result = 0;
    int status = SIZE_ATOI_STATUS_INIT;
    const char *str_p, *str_endp;

    if (size_ptr == NULL) return -1;
    if (str == NULL) return -1;

    str_p = str;
	str_endp = str_p + len;
    *size_ptr = 0;
    while (str_p != str_endp)
    {
        if (*str_p == ' ')
        {
            /* skip space */
        }
        else
        {
            switch (status)
            {
                case SIZE_ATOI_STATUS_INIT:
                    if (('0' <= *str_p) && (*str_p <= '9'))
                    {
                        result = result * 10 + ((int)(*str_p) - (int)('0'));
                        status = SIZE_ATOI_STATUS_NUMBER;
                    }
                    else 
                    {
                        status = SIZE_ATOI_STATUS_ERROR;
                    }
                    break;
                case SIZE_ATOI_STATUS_NUMBER:
                    if (('0' <= *str_p) && (*str_p <= '9'))
                    {
                        result = result * 10 + ((int)(*str_p) - (int)('0'));
                        /* Status doesn't required to change */
                    }
                    else if ((*str_p == 'k') || (*str_p == 'K'))
                    {
                        result *= KB_SIZE;
                        status = SIZE_ATOI_STATUS_K;
                    }
                    else if ((*str_p == 'm') || (*str_p == 'M'))
                    {
                        result *= MB_SIZE;
                        status = SIZE_ATOI_STATUS_M;
                    }
                    else if ((*str_p == 'g') || (*str_p == 'G'))
                    {
                        result *= GB_SIZE;
                        status = SIZE_ATOI_STATUS_G;
                    }
                    else if ((*str_p == 't') || (*str_p == 'T'))
                    {
                        /* TB doesn't supported currently */
                        status = SIZE_ATOI_STATUS_ERROR;
                    }
                    else 
                    {
                        status = SIZE_ATOI_STATUS_ERROR;
                    }
                    break;
                case SIZE_ATOI_STATUS_K:
                case SIZE_ATOI_STATUS_M:
                case SIZE_ATOI_STATUS_G:
                    if ((*str_p == 'b') || (*str_p == 'B'))
                    { status = SIZE_ATOI_STATUS_B; }
                    else
                    { status = SIZE_ATOI_STATUS_ERROR; }
                    break;
                case SIZE_ATOI_STATUS_B:
                    if ((*str_p == 'y') || (*str_p == 'Y'))
                    { status = SIZE_ATOI_STATUS_BY; }
                    else
                    { status = SIZE_ATOI_STATUS_ERROR; }
                    break;
                case SIZE_ATOI_STATUS_BY:
                    if ((*str_p == 't') || (*str_p == 'T'))
                    { status = SIZE_ATOI_STATUS_BYT; }
                    else
                    { status = SIZE_ATOI_STATUS_ERROR; }
                    break;
                case SIZE_ATOI_STATUS_BYT:
                    if ((*str_p == 'e') || (*str_p == 'E'))
                    { status = SIZE_ATOI_STATUS_BYTE; }
                    else
                    { status = SIZE_ATOI_STATUS_ERROR; }
                    break;
                case SIZE_ATOI_STATUS_BYTE:
                    if ((*str_p == 's') || (*str_p == 'S'))
                    { status = SIZE_ATOI_STATUS_BYTES; }
                    else
                    { status = SIZE_ATOI_STATUS_ERROR; }
                    break;
                case SIZE_ATOI_STATUS_BYTES:
                    status = SIZE_ATOI_STATUS_ERROR;
                    break;
                case SIZE_ATOI_STATUS_ERROR:
                default:
                    return -1;
                    break;
            }
        }
        str_p++;
    }
    switch (status)
    {
        case SIZE_ATOI_STATUS_NUMBER:
        case SIZE_ATOI_STATUS_K:
        case SIZE_ATOI_STATUS_M:
        case SIZE_ATOI_STATUS_G:
        case SIZE_ATOI_STATUS_B:
        case SIZE_ATOI_STATUS_BYTE:
        case SIZE_ATOI_STATUS_BYTES:
            *size_ptr = result;
            break;
        default:
            *size_ptr = 0;
            return -1;
            break;
    }
    return 0;
}

/* Take a break */
void do_rest(void)
{
#if defined(WINDOWS)
    Sleep(1);
#elif defined(UNIX)
    usleep((unsigned int)1);
#else
	/* Do Nothing */
#endif
}

