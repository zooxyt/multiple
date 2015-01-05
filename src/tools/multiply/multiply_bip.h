/* Multiply -- Multiple IR Generator
 * Built-in Procedure
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

#ifndef _MULTIPLY_BIP_H_
#define _MULTIPLY_BIP_H_


#include <stdio.h>
#include <stdint.h>


struct multiply_customizable_built_in_procedure_write_back
{
    uint32_t instrument_number;
    struct multiply_customizable_built_in_procedure_write_back *next;
};
struct multiply_customizable_built_in_procedure_write_back *multiply_customizable_built_in_procedure_write_back_new(uint32_t instrument_number);
int multiply_customizable_built_in_procedure_write_back_destroy(struct multiply_customizable_built_in_procedure_write_back *write_back);

struct multiply_customizable_built_in_procedure_write_back_list
{
    struct multiply_customizable_built_in_procedure_write_back *begin;
    struct multiply_customizable_built_in_procedure_write_back *end;
    size_t size;
};
struct multiply_customizable_built_in_procedure_write_back_list *multiply_customizable_built_in_procedure_write_back_list_new(void);
int multiply_customizable_built_in_procedure_write_back_list_destroy(struct multiply_customizable_built_in_procedure_write_back_list *write_back_list);
int multiply_customizable_built_in_procedure_write_back_list_append_with_configure(struct multiply_customizable_built_in_procedure_write_back_list *write_back_list, uint32_t instrument_number);

struct multiply_customizable_built_in_procedure
{
    char *name;
    size_t name_len;
    int called;

    /* instrument number in icode */
    uint32_t instrument_number_icode;
    struct multiply_customizable_built_in_procedure_write_back_list *write_backs;

    struct multiply_customizable_built_in_procedure *next;
};
struct multiply_customizable_built_in_procedure *multiply_customizable_built_in_procedure_new(const char *name, const size_t name_len);
int multiply_customizable_built_in_procedure_destroy(struct multiply_customizable_built_in_procedure *customizable_built_in_procedure);

struct multiply_customizable_built_in_procedure_list
{
    struct multiply_customizable_built_in_procedure *begin;
    struct multiply_customizable_built_in_procedure *end;

    char **customizable_built_in_procedures;
};
struct multiply_customizable_built_in_procedure_list *multiply_customizable_built_in_procedure_list_new(void);
int multiply_customizable_built_in_procedure_list_destroy(struct multiply_customizable_built_in_procedure_list *list);
int multiply_customizable_built_in_procedure_list_register(struct multiply_customizable_built_in_procedure_list *list, \
        char **customizable_built_in_procedures);

struct multiply_customizable_built_in_procedure *multiply_customizable_built_in_procedure_list_lookup( \
        struct multiply_customizable_built_in_procedure_list *customizable_built_in_procedure_list, \
        const char *name, const size_t name_len);
int multiply_customizable_built_in_procedure_list_called( \
        struct multiply_customizable_built_in_procedure_list *customizable_built_in_procedure_list, \
        const char *name, const size_t name_len);
int multiply_customizable_built_in_procedure_list_add_writeback( \
        struct multiply_customizable_built_in_procedure_list *customizable_built_in_procedure_list, \
        const char *name, const size_t name_len, uint32_t instrument_number);


#endif

