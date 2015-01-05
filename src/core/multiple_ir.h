/* Multiple : Intermediate Representation
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

#ifndef _MULTIPLE_IR_H_
#define _MULTIPLE_IR_H_

#include "selfcheck.h"

#include <stdint.h>
#include <stdio.h>


/* .data Section */

enum multiple_ir_data_section_item_type
{
    MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_UNKNOWN = 0,
    MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NONE = 1,
    MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INT,
    MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_FLOAT,
    MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_CHAR,
    MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL,
    MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NAN,
    MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INF,
    MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER,
    MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_STR,
};

#define MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL_FALSE 0
#define MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL_TRUE 1
#define MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NONE_NONE 0
#define MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NAN_POS 0
#define MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NAN_NEG 1
#define MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INF_POS 0
#define MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INF_NEG 1

struct multiple_ir_data_section_item
{
    /* Resource ID */
    uint32_t id;

    /* Body of data section item */
    enum multiple_ir_data_section_item_type type;
    union
    {
        int value_int;
        double value_float;
        uint32_t value_char;
        int value_bool;
        int signed_nan;
        int signed_inf;
        struct 
        {
            char *str;
            size_t len;
        } value_str;
    } u;

    /* Memory usage of this item */
    uint32_t size;

    struct multiple_ir_data_section_item *next;
};
struct multiple_ir_data_section_item *multiple_ir_data_section_item_new(enum multiple_ir_data_section_item_type type);
int multiple_ir_data_section_item_destroy(struct multiple_ir_data_section_item *item);

struct multiple_ir_data_section
{
    struct multiple_ir_data_section_item *begin;
    struct multiple_ir_data_section_item *end;
    size_t size;
};

struct multiple_ir_data_section *multiple_ir_data_section_new(void);
int multiple_ir_data_section_destroy(struct multiple_ir_data_section *section);
int multiple_ir_data_section_append(struct multiple_ir_data_section *section, \
        struct multiple_ir_data_section_item *new_item);


/* .text Section */

struct multiple_ir_text_section_item
{
    uint32_t opcode;
    uint32_t operand;
    uint32_t source_line_start;
    uint32_t source_line_end;

    struct multiple_ir_text_section_item *next;
};

struct multiple_ir_text_section_item *multiple_ir_text_section_item_new(void);
int multiple_ir_text_section_item_destroy(struct multiple_ir_text_section_item *item);

struct multiple_ir_text_section
{
    struct multiple_ir_text_section_item *begin;
    struct multiple_ir_text_section_item *end;
    size_t size;
};

struct multiple_ir_text_section *text_section_new(void);
int multiple_ir_text_section_destroy(struct multiple_ir_text_section *section);
int multiple_ir_text_section_append(struct multiple_ir_text_section *section, \
        struct multiple_ir_text_section_item *new_item);


/* .debug Section */

struct multiple_ir_debug_section_item
{
    uint32_t line_number_asm;
    uint32_t line_number_source_start;
    uint32_t line_number_source_end;

    struct multiple_ir_debug_section_item *next;
};

struct multiple_ir_debug_section_item *multiple_ir_debug_section_item_new(void);
int multiple_ir_debug_section_item_destroy(struct multiple_ir_debug_section_item *item);

struct multiple_ir_debug_section
{
    struct multiple_ir_debug_section_item *begin;
    struct multiple_ir_debug_section_item *end;
    size_t size;
};

struct multiple_ir_debug_section *multiple_ir_debug_section_new(void);
int multiple_ir_debug_section_destroy(struct multiple_ir_debug_section *section);
int multiple_ir_debug_section_append(struct multiple_ir_debug_section *section, \
        struct multiple_ir_debug_section_item *new_item);


/* .export Section */

enum multiple_export_section_item_type
{
    MULTIPLE_IR_EXPORT_SECTION_ITEM_ARGS_UNKNOWN = 0, 
    MULTIPLE_IR_EXPORT_SECTION_ITEM_ARGS_NORMAL,
    MULTIPLE_IR_EXPORT_SECTION_ITEM_ARGS_LIST,
    MULTIPLE_IR_EXPORT_SECTION_ITEM_ARGS_LIST_REVERSE,
    MULTIPLE_IR_EXPORT_SECTION_ITEM_ARGS_MAP,
};

struct multiple_ir_export_section_item
{
    uint32_t name;

    uint32_t *args_types;
    uint32_t *args;
    
    size_t args_count;

    uint32_t instrument_number; /* for locating the entrance of the function */

    int blank; /* only occupies space */

    struct multiple_ir_export_section_item *next;
};
struct multiple_ir_export_section_item *multiple_ir_export_section_item_new(void);
int multiple_ir_export_section_item_destroy(struct multiple_ir_export_section_item *item);

struct multiple_ir_export_section
{
    struct multiple_ir_export_section_item *begin;
    struct multiple_ir_export_section_item *end;
    size_t size;
};
struct multiple_ir_export_section *multiple_ir_export_section_new(void);
int multiple_ir_export_section_destroy(struct multiple_ir_export_section *section);
int multiple_ir_export_section_append(struct multiple_ir_export_section *section, \
        struct multiple_ir_export_section_item *new_item);


/* .import Section */

struct multiple_ir_import_section_item
{
    uint32_t name;

    struct multiple_ir_import_section_item *next;
};

struct multiple_ir_import_section_item *multiple_ir_import_section_item_new(void);
int multiple_ir_import_section_item_destroy(struct multiple_ir_import_section_item *item);

struct multiple_ir_import_section
{
    struct multiple_ir_import_section_item *begin;
    struct multiple_ir_import_section_item *end;
    size_t size;
};

struct multiple_ir_import_section *multiple_ir_import_section_new(void);
int multiple_ir_import_section_destroy(struct multiple_ir_import_section *section);
int multiple_ir_import_section_append(struct multiple_ir_import_section *section, \
        struct multiple_ir_import_section_item *new_item);
int multiple_ir_import_section_append_deduplicate(struct multiple_ir_import_section *section, \
        struct multiple_ir_import_section_item **new_item);


/* .module Section */

struct multiple_ir_module_section
{
    uint32_t name;
    int enabled;
};

struct multiple_ir_module_section *multiple_ir_module_section_new(void);
int multiple_ir_module_section_destroy(struct multiple_ir_module_section *section);


/* .source Section */

struct multiple_ir_source_section
{
    char *data;
    size_t size;
};

struct multiple_ir_source_section *multiple_ir_source_section_new(void);
int multiple_ir_source_section_destroy(struct multiple_ir_source_section *section);


/* Multiple IR */

struct multiple_ir
{
    struct multiple_ir_data_section *data_section;
    struct multiple_ir_text_section *text_section;
    struct multiple_ir_debug_section *debug_section;
    struct multiple_ir_export_section *export_section;
    struct multiple_ir_import_section *import_section;
    struct multiple_ir_module_section *module_section;
    struct multiple_ir_source_section *source_section;

    char *filename;
    size_t filename_len;
};

struct multiple_ir *multiple_ir_new(void);
int multiple_ir_destroy(struct multiple_ir *icode);


#endif

