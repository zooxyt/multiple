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

#include "selfcheck.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "multiple_ir.h"


/* .data Section */

struct multiple_ir_data_section_item *multiple_ir_data_section_item_new(enum multiple_ir_data_section_item_type type)
{
    struct multiple_ir_data_section_item *new_item = NULL;

    if ((new_item = (struct multiple_ir_data_section_item *)malloc(sizeof(struct multiple_ir_data_section_item))) == NULL)
    { return NULL; }
    new_item->next = NULL;
    new_item->id = 0;
    new_item->type = MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_UNKNOWN;
    switch (type)
    {
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NONE:
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INT:
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_FLOAT:
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_CHAR:
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL:
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NAN:
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INF:
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER:
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_STR:
            new_item->u.value_str.str = NULL;
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_UNKNOWN:
            goto fail;
    }
    new_item->type = type;
    new_item->size = 0;

    goto done;
fail:
    if (new_item != NULL)
    {
        multiple_ir_data_section_item_destroy(new_item);
        new_item = NULL;
    }
done:
    return new_item;
}

int multiple_ir_data_section_item_destroy(struct multiple_ir_data_section_item *item)
{
    switch (item->type)
    {
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER:
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_STR:
            if (item->u.value_str.str != NULL)
            { free(item->u.value_str.str); }
            break;
        default:
            break;
    }
    free(item);
    return 0;
}


struct multiple_ir_data_section *multiple_ir_data_section_new(void)
{
    struct multiple_ir_data_section *new_section = NULL;

    if ((new_section = (struct multiple_ir_data_section *)malloc(sizeof(struct multiple_ir_data_section))) == NULL)
    { return NULL; }
    new_section->begin = new_section->end = NULL;
    new_section->size = 0;

    return new_section;
}

int multiple_ir_data_section_destroy(struct multiple_ir_data_section *section)
{
    struct multiple_ir_data_section_item *item_cur, *item_next;

    item_cur = section->begin;
    while (item_cur != NULL)
    {
        item_next = item_cur->next;
        multiple_ir_data_section_item_destroy(item_cur);
        item_cur = item_next;
    }
    free(section);

    return 0;
}

int multiple_ir_data_section_append(struct multiple_ir_data_section *section, struct multiple_ir_data_section_item *new_item)
{
    if (section->begin == NULL)
    {
        section->begin = section->end = new_item;
    }
    else
    {
        section->end->next = new_item;
        section->end = new_item;
    }
    section->size += 1;

    return 0;
}


/* .text Section */

struct multiple_ir_text_section_item *multiple_ir_text_section_item_new(void)
{
    struct multiple_ir_text_section_item *new_item = NULL;

    if ((new_item = (struct multiple_ir_text_section_item *)malloc(sizeof(struct multiple_ir_text_section_item))) == NULL)
    {
        return NULL;
    }
    new_item->opcode = 0;
    new_item->operand = 0;
    new_item->source_line_start = 0;
    new_item->source_line_end = 0;
    new_item->next = NULL;

    return new_item;
}

int multiple_ir_text_section_item_destroy(struct multiple_ir_text_section_item *item)
{
    free(item);
    return 0;
}

struct multiple_ir_text_section *text_section_new(void)
{
    struct multiple_ir_text_section *new_section = NULL;

    if ((new_section = (struct multiple_ir_text_section *)malloc(sizeof(struct multiple_ir_text_section))) == NULL)
    {
        return NULL;
    }
    new_section->begin = new_section->end = NULL;
    new_section->size = 0;

    return new_section;
}

int multiple_ir_text_section_destroy(struct multiple_ir_text_section *section)
{
    struct multiple_ir_text_section_item *item_cur, *item_next;

    item_cur = section->begin;
    while (item_cur != NULL)
    {
        item_next = item_cur->next;
        multiple_ir_text_section_item_destroy(item_cur);
        item_cur = item_next;
    }
    free(section);

    return 0;
}

int multiple_ir_text_section_append(struct multiple_ir_text_section *section, struct multiple_ir_text_section_item *new_item)
{
    if (section->begin == NULL)
    {
        section->begin = section->end = new_item;
    }
    else
    {
        section->end->next = new_item;
        section->end = new_item;
    }
    section->size += 1;

    return 0;
}


/* Debug Section */

/* not been used */
struct multiple_ir_debug_section_item *multiple_ir_debug_section_item_new(void)
{
    struct multiple_ir_debug_section_item *new_item = NULL;
    if ((new_item = (struct multiple_ir_debug_section_item *)malloc(sizeof(struct multiple_ir_debug_section_item))) == NULL)
    {
        return NULL;
    }
    new_item->line_number_asm = 0;
    new_item->line_number_source_start = 0;
    new_item->line_number_source_end = 0;
    new_item->next = NULL;
    return new_item;
}

int multiple_ir_debug_section_item_destroy(struct multiple_ir_debug_section_item *item)
{
    free(item);

    return 0;
}


struct multiple_ir_debug_section *multiple_ir_debug_section_new(void)
{
    struct multiple_ir_debug_section *new_section = NULL;

    if ((new_section = (struct multiple_ir_debug_section *)malloc(sizeof(struct multiple_ir_debug_section))) == NULL)
    { return NULL; }
    new_section->begin = new_section->end = NULL;
    new_section->size = 0;

    return new_section;
}

int multiple_ir_debug_section_destroy(struct multiple_ir_debug_section *section)
{
    struct multiple_ir_debug_section_item *item_cur, *item_next;

    item_cur = section->begin;
    while (item_cur != NULL)
    {
        item_next = item_cur->next;
        multiple_ir_debug_section_item_destroy(item_cur);
        item_cur = item_next;
    }
    free(section);

    return 0;
}

int multiple_ir_debug_section_append(struct multiple_ir_debug_section *section, struct multiple_ir_debug_section_item *new_item)
{
    if (section->begin == NULL)
    {
        section->begin = section->end = new_item;
    }
    else
    {
        section->end->next = new_item;
        section->end = new_item;
    }
    section->size += 1;
    return 0;
}


/* Export Section */

struct multiple_ir_export_section_item *multiple_ir_export_section_item_new(void)
{
    struct multiple_ir_export_section_item *new_item = NULL;

    if ((new_item = (struct multiple_ir_export_section_item *)malloc(sizeof(struct multiple_ir_export_section_item))) == NULL)
    { return NULL; }

    new_item->name = 0;
    new_item->args = NULL;
    new_item->args_types = NULL;
    new_item->args_count = 0;
    new_item->next = NULL;
    new_item->instrument_number = 0;
    new_item->blank = 0;

    return new_item;
}

int multiple_ir_export_section_item_destroy(struct multiple_ir_export_section_item *item)
{
    if (item->args != NULL) free(item->args);
    if (item->args_types != NULL) free(item->args_types);
    free(item);

    return 0;
}


struct multiple_ir_export_section *multiple_ir_export_section_new(void)
{
    struct multiple_ir_export_section *new_export_section = NULL;

    if ((new_export_section = (struct multiple_ir_export_section *)malloc(sizeof(struct multiple_ir_export_section))) == NULL)
    { return NULL; }
    new_export_section->begin = new_export_section->end = NULL;
    new_export_section->size = 0;

    return new_export_section;
}

int multiple_ir_export_section_destroy(struct multiple_ir_export_section *section)
{
    struct multiple_ir_export_section_item *item_cur = NULL, *item_next;

    item_cur = section->begin;
    while (item_cur != NULL)
    {
        item_next = item_cur->next; 
        multiple_ir_export_section_item_destroy(item_cur);
        item_cur = item_next;
    }
    free(section);

    return 0;
}

int multiple_ir_export_section_append(struct multiple_ir_export_section *section, struct multiple_ir_export_section_item *new_item)
{
    if (section->begin == NULL)
    {
        section->begin = section->end = new_item;
    }
    else
    {
        section->end->next = new_item;
        section->end = new_item;
    }
    section->size++;

    return 0;
}

/* Import Section */

struct multiple_ir_import_section_item *multiple_ir_import_section_item_new(void)
{
    struct multiple_ir_import_section_item *new_item = NULL;

    if ((new_item = (struct multiple_ir_import_section_item *)malloc(sizeof(struct multiple_ir_import_section_item))) == NULL)
    { return NULL; }

    new_item->name = 0;
    new_item->next = NULL;

    return new_item;
}

int multiple_ir_import_section_item_destroy(struct multiple_ir_import_section_item *item)
{
    free(item);

    return 0;
}

struct multiple_ir_import_section *multiple_ir_import_section_new(void)
{
    struct multiple_ir_import_section *new_import_section = NULL;

    if ((new_import_section = (struct multiple_ir_import_section *)malloc(sizeof(struct multiple_ir_import_section))) == NULL)
    { return NULL; }
    new_import_section->begin = new_import_section->end = NULL;
    new_import_section->size = 0;

    return new_import_section;
}

int multiple_ir_import_section_destroy(struct multiple_ir_import_section *section)
{
    struct multiple_ir_import_section_item *item_cur = NULL, *item_next;

    item_cur = section->begin;
    while (item_cur != NULL)
    {
        item_next = item_cur->next; 
        multiple_ir_import_section_item_destroy(item_cur);
        item_cur = item_next;
    }
    free(section);

    return 0;
}

static struct multiple_ir_import_section_item *import_section_lookup_by_id(struct multiple_ir_import_section *section, uint32_t id)
{
    struct multiple_ir_import_section_item *section_item_cur;

    section_item_cur = section->begin;
    while (section_item_cur != NULL)
    {
        if (section_item_cur->name == id)
        {
            return section_item_cur;
        }
        section_item_cur = section_item_cur->next;
    }
    return NULL;
}

int multiple_ir_import_section_append(struct multiple_ir_import_section *section, struct multiple_ir_import_section_item *new_item)
{
    if (import_section_lookup_by_id(section, new_item->name) != NULL)
    {
        /* Exist */
        return 0;
    }

    if (section->begin == NULL)
    {
        section->begin = section->end = new_item;
    }
    else
    {
        section->end->next = new_item;
        section->end = new_item;
    }
    section->size++;

    return 0;
}

int multiple_ir_import_section_append_deduplicate(struct multiple_ir_import_section *section, \
        struct multiple_ir_import_section_item **new_item)
{
    if (import_section_lookup_by_id(section, (*new_item)->name) != NULL)
    {
        /* Exist */
        multiple_ir_import_section_item_destroy(*new_item);
        return 0;
    }

    if (section->begin == NULL)
    {
        section->begin = section->end = *new_item;
    }
    else
    {
        section->end->next = *new_item;
        section->end = *new_item;
    }
    section->size++;

    return 0;
}


/* Module Section */

struct multiple_ir_module_section *multiple_ir_module_section_new(void)
{
    struct multiple_ir_module_section *new_section;

    new_section = (struct multiple_ir_module_section *)malloc(sizeof(struct multiple_ir_module_section));
    if (new_section == NULL) { return NULL; }
    new_section->name = 0;
    new_section->enabled = 0;

    return new_section;
}

int multiple_ir_module_section_destroy(struct multiple_ir_module_section *section)
{
    free(section);

    return 0;
}


/* Source Section */

struct multiple_ir_source_section *multiple_ir_source_section_new(void)
{
    struct multiple_ir_source_section *new_section = NULL;

    new_section = (struct multiple_ir_source_section *)malloc(sizeof(struct multiple_ir_source_section));
    if (new_section == NULL) return NULL;

    new_section->data = NULL;
    new_section->size = 0;

    return new_section;
}

int multiple_ir_source_section_destroy(struct multiple_ir_source_section *section)
{
    if (section->data != NULL) free(section->data);
    free(section);

    return 0;
}


/* icode */

struct multiple_ir *multiple_ir_new(void)
{
    struct multiple_ir *new_icode;
    if ((new_icode = (struct multiple_ir *)malloc(sizeof(struct multiple_ir))) == NULL)
    {
        return NULL;
    }
    new_icode->data_section = NULL;
    new_icode->text_section = NULL;
    new_icode->debug_section = NULL;
    new_icode->export_section = NULL;
    new_icode->import_section = NULL;
    new_icode->module_section = NULL;
    new_icode->source_section = NULL;
    new_icode->filename = NULL;
    new_icode->filename_len = 0;
    new_icode->data_section = multiple_ir_data_section_new();
    new_icode->text_section = text_section_new();
    new_icode->debug_section = multiple_ir_debug_section_new();
    new_icode->export_section = multiple_ir_export_section_new();
    new_icode->import_section = multiple_ir_import_section_new();
    new_icode->module_section = multiple_ir_module_section_new();
    new_icode->source_section = multiple_ir_source_section_new();
    if ((new_icode->data_section == NULL) || \
            (new_icode->text_section == NULL) || \
            (new_icode->debug_section == NULL) || \
            (new_icode->export_section == NULL) || \
            (new_icode->import_section == NULL) || \
            (new_icode->module_section == NULL) || \
            (new_icode->source_section == NULL))
    { goto fail; }
    goto done;
fail:
    if (new_icode != NULL)
    {
        multiple_ir_destroy(new_icode);
        new_icode = NULL;
    }
done:
    return new_icode;
}

int multiple_ir_destroy(struct multiple_ir *icode)
{
    if (icode->data_section != NULL) multiple_ir_data_section_destroy(icode->data_section);
    if (icode->text_section != NULL) multiple_ir_text_section_destroy(icode->text_section);
    if (icode->debug_section != NULL) multiple_ir_debug_section_destroy(icode->debug_section);
    if (icode->export_section != NULL) multiple_ir_export_section_destroy(icode->export_section);
    if (icode->import_section != NULL) multiple_ir_import_section_destroy(icode->import_section);
    if (icode->module_section != NULL) multiple_ir_module_section_destroy(icode->module_section);
    if (icode->source_section != NULL) multiple_ir_source_section_destroy(icode->source_section);
    if (icode->filename != NULL) free(icode->filename);
    free(icode);

    return 0;
}

