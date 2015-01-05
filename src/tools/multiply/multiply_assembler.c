/* Multiply -- Multiple IR Generator
 * Assembler
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

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

/* Multiple */
#include "multiple_ir.h"
#include "multiple_err.h"
#include "vm_types.h"

/* Multiply */
#include "multiply.h"
#include "multiply_num.h"
#include "multiply_assembler.h"

struct multiply_assembler_line
{
    int type;

	int value;
    double value_dbl;
	uint32_t opcode;
    uint32_t operand;
    uint32_t lbl_id;
    uint32_t raw_int;
	char *str;
	size_t str_len;

    struct multiply_assembler_line *next;
};

struct multiply_assembler
{
    struct multiply_assembler_line *begin;
    struct multiply_assembler_line *end;
    size_t size;

    struct multiple_ir *icode;
};

static struct multiply_assembler *multiply_assembler_new(struct multiple_ir *icode);
static int multiply_assembler_destroy(struct multiply_assembler *assembler);
static struct multiply_assembler_line *multiply_assembler_op_id(const uint32_t opcode, const char *str, const size_t str_len);
static struct multiply_assembler_line *multiply_assembler_op_str(const uint32_t opcode, const char *str, const size_t str_len);
static struct multiply_assembler_line *multiply_assembler_op_int(const uint32_t opcode, const int value);
static struct multiply_assembler_line *multiply_assembler_op_none(const uint32_t opcode);
static struct multiply_assembler_line *multiply_assembler_op_false(const uint32_t opcode);
static struct multiply_assembler_line *multiply_assembler_op_true(const uint32_t opcode);
static struct multiply_assembler_line *multiply_assembler_op_nan(const uint32_t opcode, int sign);
static struct multiply_assembler_line *multiply_assembler_op_inf(const uint32_t opcode, int sign);
static struct multiply_assembler_line *multiply_assembler_op_lbl(const uint32_t opcode, const uint32_t lbl_id);
static struct multiply_assembler_line *multiply_assembler_op_raw(const uint32_t opcode, const uint32_t raw_int);
static struct multiply_assembler_line *multiply_assembler_op_type(const uint32_t opcode, const char *str, const size_t str_len);
static struct multiply_assembler_line *multiply_assembler_op(const uint32_t opcode);
static struct multiply_assembler_line *multiply_assembler_lbl(const uint32_t lbl_id);


/* Definition */

static struct multiply_assembler_line *multiply_assembler_line_new(int type)
{
    struct multiply_assembler_line *new_line;

    new_line = (struct multiply_assembler_line *)malloc(sizeof(struct multiply_assembler_line));
    if (new_line == NULL) { goto fail; }
    switch (type)
    {
        case MULTIPLY_ASM_LINE_TYPE_OP_ID:
        case MULTIPLY_ASM_LINE_TYPE_OP_STR:
            new_line->str = NULL;
            new_line->str_len = 0;
            break;
    }
    new_line->type = type;
    new_line->next = NULL;
fail:
    return new_line;
}

static int multiply_assembler_line_destroy(struct multiply_assembler_line *line)
{
    if (line == NULL) return -MULTIPLE_ERR_NULL_PTR;
    switch (line->type)
    {
        case MULTIPLY_ASM_LINE_TYPE_OP_ID:
        case MULTIPLY_ASM_LINE_TYPE_OP_STR:
        case MULTIPLY_ASM_LINE_TYPE_OP_TYPE:
            if (line->str != NULL) free(line->str);
            break;
    }
    free(line);
    return 0;
}

struct multiply_assembler *multiply_assembler_new(struct multiple_ir *icode)
{
    struct multiply_assembler *new_assembler = NULL;

    new_assembler = (struct multiply_assembler *)malloc(sizeof(struct multiply_assembler));
    if (new_assembler == NULL) goto fail;
    new_assembler->begin = new_assembler->end = NULL;
    new_assembler->size = 0;
    new_assembler->icode = icode;
    goto done;
fail:
    if (new_assembler != NULL)
    {
        free(new_assembler);
        new_assembler = NULL;
    }
done:
    return new_assembler;
}

int multiply_assembler_destroy(struct multiply_assembler *assembler)
{
    struct multiply_assembler_line *assembler_line_cur = NULL, *assembler_line_next;

    if (assembler == NULL) return -MULTIPLE_ERR_NULL_PTR;

    assembler_line_cur = assembler->begin;
    while (assembler_line_cur != NULL)
    {
        assembler_line_next = assembler_line_cur->next;
        multiply_assembler_line_destroy(assembler_line_cur);
        assembler_line_cur = assembler_line_next;
    }
    free(assembler);
    return 0;
}

static int multiply_assembler_append(struct multiply_assembler *assembler, struct multiply_assembler_line *line)
{
    if (assembler == NULL) return -MULTIPLE_ERR_NULL_PTR;
    if (line == NULL) return -MULTIPLE_ERR_NULL_PTR;

    if (assembler->begin == NULL)
    {
        assembler->begin = assembler->end = line;
    }
    else
    {
        assembler->end->next = line;
        assembler->end = line;
    }
    assembler->size += 1;
    return 0;
}

static struct multiply_assembler_line *multiply_assembler_op_id(const uint32_t opcode, const char *str, const size_t str_len)
{
    struct multiply_assembler_line *new_line;

    new_line = multiply_assembler_line_new(MULTIPLY_ASM_LINE_TYPE_OP_ID);
    if (new_line == NULL) return NULL;
    new_line->opcode = opcode;
	new_line->str = NULL;
    new_line->str = (char *)malloc(sizeof(char) * (str_len + 1));
	if (new_line->str == NULL) { goto fail; }
    memcpy(new_line->str, str, str_len);
    new_line->str[str_len] = '\0';
    new_line->str_len = str_len;
	goto done;
fail:
	if (new_line != NULL)
	{
		if (new_line->str != NULL) { free(new_line->str); }
		free(new_line);
		new_line = NULL;
	}
done:
    return new_line;
}

static struct multiply_assembler_line *multiply_assembler_op_str(const uint32_t opcode, const char *str, const size_t str_len)
{
    struct multiply_assembler_line *new_line;

    new_line = multiply_assembler_line_new(MULTIPLY_ASM_LINE_TYPE_OP_STR);
    if (new_line == NULL) return NULL;
	new_line->str = NULL;
    new_line->opcode = opcode;
    new_line->str = (char *)malloc(sizeof(char) * (str_len + 1));
	if (new_line->str == NULL)
	{
		goto fail;
	}
    memcpy(new_line->str, str, str_len);
    new_line->str[str_len] = '\0';
    new_line->str_len = str_len;
	goto done;
fail:
	if (new_line != NULL)
	{
		if (new_line->str != NULL) { free(new_line->str); }
        free(new_line);
		new_line = NULL;
	}
done:
    return new_line;
}

static struct multiply_assembler_line *multiply_assembler_op_int(const uint32_t opcode, const int value)
{
    struct multiply_assembler_line *new_line;

    new_line = multiply_assembler_line_new(MULTIPLY_ASM_LINE_TYPE_OP_INT);
    if (new_line == NULL) return NULL;
    new_line->opcode = opcode;
    new_line->value = value;

    return new_line;
}

static struct multiply_assembler_line *multiply_assembler_op_float(const uint32_t opcode, const double value)
{
    struct multiply_assembler_line *new_line;

    new_line = multiply_assembler_line_new(MULTIPLY_ASM_LINE_TYPE_OP_FLOAT);
    if (new_line == NULL) return NULL;
    new_line->opcode = opcode;
    new_line->value_dbl = value;

    return new_line;
}

static struct multiply_assembler_line *multiply_assembler_op_none(const uint32_t opcode)
{
    struct multiply_assembler_line *new_line;

    new_line = multiply_assembler_line_new(MULTIPLY_ASM_LINE_TYPE_OP_NONE);
    if (new_line == NULL) return NULL;
    new_line->opcode = opcode;

    return new_line;
}

static struct multiply_assembler_line *multiply_assembler_op_false(const uint32_t opcode)
{
    struct multiply_assembler_line *new_line;

    new_line = multiply_assembler_line_new(MULTIPLY_ASM_LINE_TYPE_OP_FALSE);
    if (new_line == NULL) return NULL;
    new_line->opcode = opcode;

    return new_line;
}

static struct multiply_assembler_line *multiply_assembler_op_true(const uint32_t opcode)
{
    struct multiply_assembler_line *new_line;

    new_line = multiply_assembler_line_new(MULTIPLY_ASM_LINE_TYPE_OP_TRUE);
    if (new_line == NULL) return NULL;
    new_line->opcode = opcode;

    return new_line;
}

static struct multiply_assembler_line *multiply_assembler_op_nan(const uint32_t opcode, int sign)
{
    struct multiply_assembler_line *new_line;

    new_line = multiply_assembler_line_new(MULTIPLY_ASM_LINE_TYPE_OP_NAN);
    if (new_line == NULL) return NULL;
    new_line->opcode = opcode;
    new_line->value = sign;

    return new_line;
}

static struct multiply_assembler_line *multiply_assembler_op_inf(const uint32_t opcode, int sign)
{
    struct multiply_assembler_line *new_line;

    new_line = multiply_assembler_line_new(MULTIPLY_ASM_LINE_TYPE_OP_INF);
    if (new_line == NULL) return NULL;
    new_line->opcode = opcode;
    new_line->value = sign;

    return new_line;
}

static struct multiply_assembler_line *multiply_assembler_op_lbl(const uint32_t opcode, const uint32_t lbl_id)
{
    struct multiply_assembler_line *new_line;

    new_line = multiply_assembler_line_new(MULTIPLY_ASM_LINE_TYPE_OP_LBL);
    if (new_line == NULL) return NULL;
    new_line->opcode = opcode;
    new_line->lbl_id = lbl_id;

    return new_line;
}

static struct multiply_assembler_line *multiply_assembler_op_lblr(const uint32_t opcode, const uint32_t lbl_id)
{
    struct multiply_assembler_line *new_line;

    new_line = multiply_assembler_line_new(MULTIPLY_ASM_LINE_TYPE_OP_LBLR);
    if (new_line == NULL) return NULL;
    new_line->opcode = opcode;
    new_line->lbl_id = lbl_id;

    return new_line;
}

static struct multiply_assembler_line *multiply_assembler_op_raw(const uint32_t opcode, const uint32_t raw_int)
{
    struct multiply_assembler_line *new_line;

    new_line = multiply_assembler_line_new(MULTIPLY_ASM_LINE_TYPE_OP_RAW);
    if (new_line == NULL) return NULL;
    new_line->opcode = opcode;
    new_line->raw_int = raw_int;

    return new_line;
}

static struct multiply_assembler_line *multiply_assembler_op_type(const uint32_t opcode, const char *str, const size_t str_len)
{
    struct multiply_assembler_line *new_line;

    new_line = multiply_assembler_line_new(MULTIPLY_ASM_LINE_TYPE_OP_TYPE);
    if (new_line == NULL) return NULL;
    new_line->opcode = opcode;
	new_line->str = NULL;
    new_line->str = (char *)malloc(sizeof(char) * (str_len + 1));
	if (new_line->str == NULL)
	{
		goto fail;
	}
    memcpy(new_line->str, str, str_len);
    new_line->str[str_len] = '\0';
    new_line->str_len = str_len;
	goto done;
fail:
	if (new_line != NULL)
	{
		if (new_line->str != NULL) free(new_line->str);
		free(new_line);
		new_line = NULL;
	}
done:
    return new_line;
}

static struct multiply_assembler_line *multiply_assembler_op(const uint32_t opcode)
{
    struct multiply_assembler_line *new_line;

    new_line = multiply_assembler_line_new(MULTIPLY_ASM_LINE_TYPE_OP);
    if (new_line == NULL) return NULL;
    new_line->opcode = opcode;
    new_line->operand = 0;

    return new_line;
}

static struct multiply_assembler_line *multiply_assembler_lbl(const uint32_t lbl_id)
{
    struct multiply_assembler_line *new_line;

    new_line = multiply_assembler_line_new(MULTIPLY_ASM_LINE_TYPE_LABEL);
    if (new_line == NULL) return NULL;
    new_line->lbl_id = lbl_id;

    return new_line;
}

static int multiple_icg_asm_generate_icode_search(uint32_t *instrument_number_out, \
        uint32_t instrument_number_init, \
        uint32_t lbl_target, \
        struct multiply_assembler *assembler)
{
    struct multiply_assembler_line *assembler_line_cur = NULL;

    assembler_line_cur = assembler->begin;
    while (assembler_line_cur != NULL)
    {
        if (assembler_line_cur->type == MULTIPLY_ASM_LABEL)
        {
            if (assembler_line_cur->lbl_id == lbl_target)
            {
                *instrument_number_out = instrument_number_init;
                return 0;
            }
        }
        else
        {
            instrument_number_init += 1;
        }
        assembler_line_cur = assembler_line_cur->next; 
    }
    return -1;
}

static int multiple_icg_asm_generate_icode( \
        struct multiple_error *err, \
        struct multiple_ir *icode, \
        struct multiply_resource_id_pool *res_id, \
        struct multiply_assembler *assembler)
{
    int ret = 0;
    struct multiply_assembler_line *assembler_line_cur = NULL;
    uint32_t id;
    uint32_t instrument_number, instrument_number_init;
    uint32_t instrument_number_cur = 0;
    uint32_t type_id;

    instrument_number_init = (uint32_t)(icode->text_section->size);
    assembler_line_cur = assembler->begin;
    while (assembler_line_cur != NULL)
    {
        switch (assembler_line_cur->type)
        {
            case MULTIPLY_ASM_OP_ID:
                if ((ret = multiply_resource_get_id( \
                                err, icode, res_id, \
                                &id, \
                                assembler_line_cur->str, \
                                assembler_line_cur->str_len)) != 0)
                { goto fail; }
                if ((ret = multiply_icodegen_text_section_append(err, icode, assembler_line_cur->opcode, id)) != 0) { goto fail; }
                break;
            case MULTIPLY_ASM_OP_STR:
                if ((ret = multiply_resource_get_str(
                                err, icode, res_id, \
                                &id, \
                                assembler_line_cur->str, \
                                assembler_line_cur->str_len)) != 0)
                { goto fail; }
                if ((ret = multiply_icodegen_text_section_append(err, icode, assembler_line_cur->opcode, id)) != 0) { goto fail; }
                break;
            case MULTIPLY_ASM_OP_INT:
                if ((ret = multiply_resource_get_int( \
                                err, icode, res_id, \
                                &id, \
                                assembler_line_cur->value)) != 0)
                { goto fail; }
                if ((ret = multiply_icodegen_text_section_append(err, icode, assembler_line_cur->opcode, id)) != 0) { goto fail; }
                break;
            case MULTIPLY_ASM_OP_FLOAT:
                if ((ret = multiply_resource_get_float( \
                                err, icode, res_id, \
                                &id, \
                                assembler_line_cur->value_dbl)) != 0)
                { goto fail; }
                if ((ret = multiply_icodegen_text_section_append(err, icode, assembler_line_cur->opcode, id)) != 0) { goto fail; }
                break;
            case MULTIPLY_ASM_OP_NONE:
                if ((ret = multiply_resource_get_none( \
                                err, icode, res_id, \
                                &id)) != 0)
                { goto fail; }
                if ((ret = multiply_icodegen_text_section_append(err, icode, assembler_line_cur->opcode, id)) != 0) { goto fail; }
                break;
            case MULTIPLY_ASM_OP_FALSE:
                if ((ret = multiply_resource_get_false( \
                                err, icode, res_id, \
                                &id)) != 0)
                { goto fail; }
                if ((ret = multiply_icodegen_text_section_append(err, icode, assembler_line_cur->opcode, id)) != 0) { goto fail; }
                break;
            case MULTIPLY_ASM_OP_TRUE:
                if ((ret = multiply_resource_get_true( \
                                err, icode, res_id, \
                                &id)) != 0)
                { goto fail; }
                if ((ret = multiply_icodegen_text_section_append(err, icode, assembler_line_cur->opcode, id)) != 0) { goto fail; }
                break;
            case MULTIPLY_ASM_OP_NAN:
                if ((ret = multiply_resource_get_nan( \
                                err, icode, res_id, \
                                &id, \
                                assembler_line_cur->value)) != 0) 
                { goto fail; }
                if ((ret = multiply_icodegen_text_section_append(err, icode, assembler_line_cur->opcode, id)) != 0) { goto fail; }
                break;
            case MULTIPLY_ASM_OP_INF:
                if ((ret = multiply_resource_get_inf( \
                                err, icode, res_id, \
                                &id, \
                                assembler_line_cur->value)) != 0) 
                { goto fail; }
                if ((ret = multiply_icodegen_text_section_append(err, icode, assembler_line_cur->opcode, id)) != 0) { goto fail; }
                break;
            case MULTIPLY_ASM_OP_RAW:
                if ((ret = multiply_icodegen_text_section_append(err, icode, \
                                assembler_line_cur->opcode, assembler_line_cur->raw_int)) != 0) { goto fail; }
                break;
            case MULTIPLY_ASM_OP_TYPE:
                ret = virtual_machine_object_type_name_to_id(&type_id, assembler_line_cur->str, assembler_line_cur->str_len);
                if (ret != 0) 
                {
                    multiple_error_update(err, -MULTIPLE_ERR_ICODEGEN, "\'%s\' isn't a valid type name", assembler_line_cur->str);
                    ret = -MULTIPLE_ERR_ICODEGEN; 
                    goto fail; 
                }
                if ((ret = multiply_icodegen_text_section_append(err, icode, \
                                assembler_line_cur->opcode, type_id)) != 0) { goto fail; }
                break;
            case MULTIPLY_ASM_OP_LBL:
                /* Search label */
                if (multiple_icg_asm_generate_icode_search(&instrument_number, \
                            instrument_number_init, \
                            assembler_line_cur->lbl_id, \
                            assembler) != 0)
                {
                    multiple_error_update(err, -MULTIPLE_ERR_INTERNAL, \
                            "error: label id %u not found", \
                            assembler_line_cur->lbl_id);
                    ret = -MULTIPLE_ERR_INTERNAL;
                    goto fail;
                }
                if ((ret = multiply_icodegen_text_section_append(err, icode, \
                                assembler_line_cur->opcode, instrument_number)) != 0) { goto fail; }
                break;
            case MULTIPLY_ASM_OP_LBLR:
                /* Search label */
                if (multiple_icg_asm_generate_icode_search(&instrument_number, \
                            instrument_number_init, \
                            assembler_line_cur->lbl_id, \
                            assembler) != 0)
                {
                    multiple_error_update(err, -MULTIPLE_ERR_INTERNAL, \
                            "error: label id %u not found", \
                            assembler_line_cur->lbl_id);
                    ret = -MULTIPLE_ERR_INTERNAL;
                    goto fail;
                }
                if ((ret = multiply_icodegen_text_section_append(err, icode, \
                                assembler_line_cur->opcode, \
                                snr_sam_to_cmp((int32_t)instrument_number - (int32_t)instrument_number_cur) \
                                )) != 0) { goto fail; }
                break;
            case MULTIPLY_ASM_OP:
                if ((ret = multiply_icodegen_text_section_append(err, icode, \
                                assembler_line_cur->opcode, 0)) != 0) { goto fail; }
                break;
        }
        instrument_number_cur += 1;
        assembler_line_cur = assembler_line_cur->next; 
    }

fail:
    return ret;
}

static int multiple_icg_asm_precompiled( \
        struct multiple_error *err, \
        struct multiple_ir *icode, \
        struct multiply_resource_id_pool *res_id, \
        struct multiply_text_precompiled **icg_text_precompiled_out, \
        struct multiply_assembler *assembler)
{
    int ret = 0;
    struct multiply_assembler_line *assembler_line_cur = NULL;
    uint32_t id;
    uint32_t instrument_number;
    uint32_t instrument_number_cur = 0;
    uint32_t type_id;
    struct multiply_text_precompiled *new_text_precompiled = NULL;
    size_t idx = 0;

    new_text_precompiled = multiply_text_precompiled_new(assembler->size);
    if (new_text_precompiled == NULL)
    {
        MULTIPLE_ERROR_MALLOC();
        ret = -MULTIPLE_ERR_MALLOC; 
        goto fail; 
    }

    assembler_line_cur = assembler->begin;
    while (assembler_line_cur != NULL)
    {
        /* Opcode */
        new_text_precompiled->lines[idx].opcode = assembler_line_cur->opcode;
        /* Operand */
        switch (assembler_line_cur->type)
        {
            case MULTIPLY_ASM_OP_ID:
                if ((ret = multiply_resource_get_id( \
                                err, icode, res_id, \
                                &id, \
                                assembler_line_cur->str, \
                                assembler_line_cur->str_len)) != 0)
                { goto fail; }
                new_text_precompiled->lines[idx].operand = id;
                break;
            case MULTIPLY_ASM_OP_STR:
                if ((ret = multiply_resource_get_str(
                                err, icode, res_id, \
                                &id, \
                                assembler_line_cur->str, \
                                assembler_line_cur->str_len)) != 0)
                { goto fail; }
                new_text_precompiled->lines[idx].operand = id;
                break;
            case MULTIPLY_ASM_OP_INT:
                if ((ret = multiply_resource_get_int( \
                                err, icode, res_id, \
                                &id, \
                                assembler_line_cur->value)) != 0)
                { goto fail; }
                new_text_precompiled->lines[idx].operand = id;
                break;
            case MULTIPLY_ASM_OP_FLOAT:
                if ((ret = multiply_resource_get_float( \
                                err, icode, res_id, \
                                &id, \
                                assembler_line_cur->value_dbl)) != 0)
                { goto fail; }
                new_text_precompiled->lines[idx].operand = id;
                break;
            case MULTIPLY_ASM_OP_NONE:
                if ((ret = multiply_resource_get_none( \
                                err, icode, res_id, \
                                &id)) != 0)
                { goto fail; }
                new_text_precompiled->lines[idx].operand = id;
                break;
            case MULTIPLY_ASM_OP_FALSE:
                if ((ret = multiply_resource_get_false( \
                                err, icode, res_id, \
                                &id)) != 0)
                { goto fail; }
                new_text_precompiled->lines[idx].operand = id;
                break;
            case MULTIPLY_ASM_OP_TRUE:
                if ((ret = multiply_resource_get_true( \
                                err, icode, res_id, \
                                &id)) != 0)
                { goto fail; }
                new_text_precompiled->lines[idx].operand = id;
                break;
            case MULTIPLY_ASM_OP_NAN:
                if ((ret = multiply_resource_get_nan( \
                                err, icode, res_id, \
                                &id, \
                                assembler_line_cur->value)) != 0) { goto fail; }
                new_text_precompiled->lines[idx].operand = id;
                break;
            case MULTIPLY_ASM_OP_INF:
                if ((ret = multiply_resource_get_inf( \
                                err, icode, res_id, \
                                &id, \
                                assembler_line_cur->value)) != 0) { goto fail; }
                new_text_precompiled->lines[idx].operand = id;
                break;
            case MULTIPLY_ASM_OP_RAW:
                new_text_precompiled->lines[idx].operand = assembler_line_cur->raw_int;
                break;
            case MULTIPLY_ASM_OP_TYPE:
                ret = virtual_machine_object_type_name_to_id(&type_id, assembler_line_cur->str, assembler_line_cur->str_len);
                if (ret != 0) 
                {
                    multiple_error_update(err, -MULTIPLE_ERR_ICODEGEN, \
                            "\'%s\' isn't a valid type name", \
                            assembler_line_cur->str);
                    ret = -MULTIPLE_ERR_ICODEGEN; 
                    goto fail; 
                }
                new_text_precompiled->lines[idx].operand = type_id;
                break;
            case MULTIPLY_ASM_OP_LBL:
                /* Search label */
                if (multiple_icg_asm_generate_icode_search(&instrument_number, \
                            0, \
                            assembler_line_cur->lbl_id, \
                            assembler) != 0)
                {
                    multiple_error_update(err, -MULTIPLE_ERR_INTERNAL, \
                            "error: label id %u not found", \
                            assembler_line_cur->lbl_id);
                    ret = -MULTIPLE_ERR_INTERNAL;
                    goto fail;
                }
                new_text_precompiled->lines[idx].operand = instrument_number;
                break;
            case MULTIPLY_ASM_OP_LBLR:
                /* Search label */
                if (multiple_icg_asm_generate_icode_search(&instrument_number, \
                            0, \
                            assembler_line_cur->lbl_id, \
                            assembler) != 0)
                {
                    multiple_error_update(err, -MULTIPLE_ERR_INTERNAL, \
                            "error: label id %u not found", \
                            assembler_line_cur->lbl_id);
                    ret = -MULTIPLE_ERR_INTERNAL;
                    goto fail;
                }
                /* Convert to absolute relative */
                new_text_precompiled->lines[idx].operand = snr_sam_to_cmp( \
                        (int32_t)instrument_number - (int32_t)instrument_number_cur);

                break;
            case MULTIPLY_ASM_OP:
                new_text_precompiled->lines[idx].operand = 0;
                break;
        }

        if (assembler_line_cur->type != MULTIPLY_ASM_LABEL)
        {
            idx++;
            instrument_number_cur += 1;
        }
        assembler_line_cur = assembler_line_cur->next; 
    }
    /* Update the new size value (label lines excluded) */
    new_text_precompiled->size = idx;

    *icg_text_precompiled_out = new_text_precompiled;

    goto done;
fail:
    if (new_text_precompiled != NULL)
    {
        multiply_text_precompiled_destroy(new_text_precompiled);
    }
done:
    return ret;
}

/* Precompiled text  */

struct multiply_text_precompiled *multiply_text_precompiled_new(size_t size)
{
    struct multiply_text_precompiled *new_text_precompiled = NULL;

    new_text_precompiled = (struct multiply_text_precompiled *)malloc(sizeof(struct multiply_text_precompiled));
    if (new_text_precompiled == NULL) { goto fail; }
    new_text_precompiled->lines = NULL;
    new_text_precompiled->size = size;
    new_text_precompiled->lines = (struct multiply_text_precompiled_line *)malloc( \
            sizeof(struct multiply_text_precompiled_line) * size);
    if (new_text_precompiled->lines == NULL)
    { goto fail; }

    goto done;
fail:
    if (new_text_precompiled != NULL)
    {
        if (new_text_precompiled->lines != NULL) { free(new_text_precompiled->lines); }
        free(new_text_precompiled);
        new_text_precompiled = NULL;
    }
done:
    return new_text_precompiled;
}

int multiply_text_precompiled_destroy(struct multiply_text_precompiled *text_precompiled)
{
    if (text_precompiled->lines != NULL) { free(text_precompiled->lines); }
    free(text_precompiled);
    return 0;
}


/* Interface */

/* Assembler */
static int multiple_icg_asm_raw( \
        struct multiple_error *err, \
        struct multiple_ir *icode, \
        struct multiply_resource_id_pool *res_id, \
        struct multiply_assembler **assembler_out, \
        va_list *argp_in)
{
    int ret = 0;
    struct multiply_assembler *new_assembler = NULL;
    struct multiply_assembler_line *new_assembler_line = NULL;
    struct multiple_ir_export_section_item *new_export_section_item = NULL;
    va_list argp;

    int op;
    uint32_t opcode;
    char *str_p;
    size_t str_len;
    int value;
    double value_dbl;
    uint32_t lbl_id;
    int args_count;
    size_t idx;
    uint32_t id;

    new_assembler = multiply_assembler_new(icode);
    if (new_assembler == NULL) 
    {
        MULTIPLE_ERROR_MALLOC();
        ret = -MULTIPLE_ERR_MALLOC; goto fail; 
    }

    va_copy(argp, *argp_in);

    /* Fill Assembler */
    for (;;)
    {
        /* Operation */
        op = va_arg(argp, int);
        if (op == MULTIPLY_ASM_FINISH) break;
        else if (op == MULTIPLY_ASM_EXPORT)
        {
            new_export_section_item = multiple_ir_export_section_item_new();

            /* Instrument */
            new_export_section_item->instrument_number = (uint32_t)(icode->text_section->size);

            /* Export name */
            str_p = va_arg(argp, char *); 
            str_len = strlen(str_p);
            if ((ret = multiply_resource_get_id( \
                            err, icode, res_id, \
                            &id, \
                            str_p, \
                            str_len)) != 0)
            { goto fail; }
            new_export_section_item->name = id;

            /* Arguments count */
            args_count = va_arg(argp, int); 
            new_export_section_item->args_count = (size_t)args_count;

            /* Arguments */
            new_export_section_item->args = (uint32_t *)malloc(sizeof(uint32_t) * new_export_section_item->args_count);
            if (new_export_section_item->args == NULL)
            {
                MULTIPLE_ERROR_MALLOC();
                ret = -MULTIPLE_ERR_MALLOC; goto fail; 
            }
            new_export_section_item->args_types = (uint32_t *)malloc(sizeof(uint32_t) * new_export_section_item->args_count);
            if (new_export_section_item->args_types == NULL)
            {
                MULTIPLE_ERROR_MALLOC();
                ret = -MULTIPLE_ERR_MALLOC; goto fail; 
            }
			for (idx = 0; idx != new_export_section_item->args_count; idx++)
            {
                str_p = va_arg(argp, char *); 
                str_len = strlen(str_p);
                if ((ret = multiply_resource_get_id( \
                                err, icode, res_id, \
                                &id, \
                                str_p, \
                                str_len)) != 0)
                { goto fail; }
                new_export_section_item->args[idx] = id;
                new_export_section_item->args_types[idx] = MULTIPLE_IR_EXPORT_SECTION_ITEM_ARGS_NORMAL;
            }
            if ((ret = multiple_ir_export_section_append(icode->export_section, new_export_section_item)) != 0)
            { goto fail; }
            new_export_section_item = NULL;
        }
        else
        {
            switch (op)
            {
                case MULTIPLY_ASM_OP_ID:
                    opcode = va_arg(argp, uint32_t); 
                    str_p = va_arg(argp, char *); 
                    str_len = strlen(str_p);
                    if ((new_assembler_line = multiply_assembler_op_id(opcode, str_p, str_len)) == NULL)
                    {
                        MULTIPLE_ERROR_MALLOC();
                        ret = -MULTIPLE_ERR_MALLOC; goto fail; 
                    }
                    break;
                case MULTIPLY_ASM_OP_STR:
                    opcode = va_arg(argp, uint32_t); 
                    str_p = va_arg(argp, char *); 
                    str_len = strlen(str_p);
                    if ((new_assembler_line = multiply_assembler_op_str(opcode, str_p, str_len)) == NULL)
                    {
                        MULTIPLE_ERROR_MALLOC();
                        ret = -MULTIPLE_ERR_MALLOC; goto fail; 
                    }
                    break;
                case MULTIPLY_ASM_OP_INT:
                    opcode = va_arg(argp, uint32_t); 
                    value = va_arg(argp, int); 
                    if ((new_assembler_line = multiply_assembler_op_int(opcode, value)) == NULL)
                    {
                        MULTIPLE_ERROR_MALLOC();
                        ret = -MULTIPLE_ERR_MALLOC; goto fail; 
                    }
                    break;
                case MULTIPLY_ASM_OP_FLOAT:
                    opcode = va_arg(argp, uint32_t); 
                    value_dbl = va_arg(argp, double); 
                    if ((new_assembler_line = multiply_assembler_op_float(opcode, value_dbl)) == NULL)
                    {
                        MULTIPLE_ERROR_MALLOC();
                        ret = -MULTIPLE_ERR_MALLOC; goto fail; 
                    }
                    break;
                case MULTIPLY_ASM_OP_NONE:
                    opcode = va_arg(argp, uint32_t); 
                    if ((new_assembler_line = multiply_assembler_op_none(opcode)) == NULL)
                    {
                        MULTIPLE_ERROR_MALLOC();
                        ret = -MULTIPLE_ERR_MALLOC; goto fail; 
                    }
                    break;
                case MULTIPLY_ASM_OP_FALSE:
                    opcode = va_arg(argp, uint32_t); 
                    if ((new_assembler_line = multiply_assembler_op_false(opcode)) == NULL)
                    {
                        MULTIPLE_ERROR_MALLOC();
                        ret = -MULTIPLE_ERR_MALLOC; goto fail; 
                    }
                    break;
                case MULTIPLY_ASM_OP_TRUE:
                    opcode = va_arg(argp, uint32_t); 
                    if ((new_assembler_line = multiply_assembler_op_true(opcode)) == NULL)
                    {
                        MULTIPLE_ERROR_MALLOC();
                        ret = -MULTIPLE_ERR_MALLOC; goto fail; 
                    }
                    break;
                case MULTIPLY_ASM_OP_NAN:
                    opcode = va_arg(argp, uint32_t); 
                    value = va_arg(argp, int); 
                    if ((new_assembler_line = multiply_assembler_op_nan(opcode, value)) == NULL)
                    {
                        MULTIPLE_ERROR_MALLOC();
                        ret = -MULTIPLE_ERR_MALLOC; goto fail; 
                    }
                    break;
                case MULTIPLY_ASM_OP_INF:
                    opcode = va_arg(argp, uint32_t); 
                    value = va_arg(argp, int); 
                    if ((new_assembler_line = multiply_assembler_op_inf(opcode, value)) == NULL)
                    {
                        MULTIPLE_ERROR_MALLOC();
                        ret = -MULTIPLE_ERR_MALLOC; goto fail; 
                    }
                    break;
                case MULTIPLY_ASM_OP_LBL:
                    opcode = va_arg(argp, uint32_t); 
                    lbl_id = va_arg(argp, uint32_t); 
                    if ((new_assembler_line = multiply_assembler_op_lbl(opcode, lbl_id)) == NULL)
                    {
                        MULTIPLE_ERROR_MALLOC();
                        ret = -MULTIPLE_ERR_MALLOC; goto fail; 
                    }
                    break;
                case MULTIPLY_ASM_OP_LBLR:
                    opcode = va_arg(argp, uint32_t); 
                    lbl_id = va_arg(argp, uint32_t); 
                    if ((new_assembler_line = multiply_assembler_op_lblr(opcode, lbl_id)) == NULL)
                    {
                        MULTIPLE_ERROR_MALLOC();
                        ret = -MULTIPLE_ERR_MALLOC; goto fail; 
                    }
                    break;
                case MULTIPLY_ASM_OP_RAW:
                    opcode = va_arg(argp, uint32_t); 
                    lbl_id = va_arg(argp, uint32_t); 
                    if ((new_assembler_line = multiply_assembler_op_raw(opcode, lbl_id)) == NULL)
                    {
                        MULTIPLE_ERROR_MALLOC();
                        ret = -MULTIPLE_ERR_MALLOC; goto fail; 
                    }
                    break;
                case MULTIPLY_ASM_OP_TYPE:
                    opcode = va_arg(argp, uint32_t); 
                    str_p = va_arg(argp, char *); 
                    str_len = strlen(str_p);
                    if ((new_assembler_line = multiply_assembler_op_type(opcode, str_p, str_len)) == NULL)
                    {
                        MULTIPLE_ERROR_MALLOC();
                        ret = -MULTIPLE_ERR_MALLOC; goto fail; 
                    }
                    break;
                case MULTIPLY_ASM_OP:
                    opcode = va_arg(argp, uint32_t); 
                    if ((new_assembler_line = multiply_assembler_op(opcode)) == NULL)
                    {
                        MULTIPLE_ERROR_MALLOC();
                        ret = -MULTIPLE_ERR_MALLOC; goto fail; 
                    }
                    break;
                case MULTIPLY_ASM_LABEL:
                    lbl_id = va_arg(argp, uint32_t); 
                    if ((new_assembler_line = multiply_assembler_lbl(lbl_id)) == NULL)
                    {
                        MULTIPLE_ERROR_MALLOC();
                        ret = -MULTIPLE_ERR_MALLOC; goto fail; 
                    }
                    break;
            }
            if ((ret = multiply_assembler_append(new_assembler, new_assembler_line)) != 0)
            {
                MULTIPLE_ERROR_INTERNAL();
                ret = -MULTIPLE_ERR_MALLOC; goto fail; 
            }
            new_assembler_line = NULL;
        }
    }

    *assembler_out = new_assembler;
    new_assembler = NULL;

    goto done;
fail:
    if (new_assembler != NULL) multiply_assembler_destroy(new_assembler);
done:
    if (new_assembler_line != NULL) multiply_assembler_line_destroy(new_assembler_line);
    if (new_export_section_item != NULL) multiple_ir_export_section_item_destroy(new_export_section_item);
    return ret;
}

/* Assembler to precompiled text */
int multiply_asm_precompile( \
        struct multiple_error *err, \
        struct multiple_ir *icode, \
        struct multiply_resource_id_pool *res_id, \
        struct multiply_text_precompiled **text_precompiled_out, \
        ...)
{
    int ret = 0;
    struct multiply_assembler *new_assembler = NULL;
    va_list argp;

    va_start(argp, text_precompiled_out);

    if ((ret = multiple_icg_asm_raw(err, \
                    icode, \
                    res_id, \
                    &new_assembler, \
                    &argp)) != 0)
    { goto fail; }

    /* Generate icode */
    if ((ret = multiple_icg_asm_precompiled(err, \
                    icode, \
                    res_id, \
                    text_precompiled_out, \
                    new_assembler)) != 0)
    { goto fail; }

    va_end(argp);

    goto done;
fail:
done:
    if (new_assembler != NULL) multiply_assembler_destroy(new_assembler);
    return ret;
}

/* Assemble to icode directly */
int multiply_asm( \
        struct multiple_error *err, \
        struct multiple_ir *icode, \
        struct multiply_resource_id_pool *res_id, \
        ...)
{
    int ret = 0;
    struct multiply_assembler *new_assembler = NULL;
    va_list argp;

    va_start(argp, res_id);

    if ((ret = multiple_icg_asm_raw(err, \
                    icode, \
                    res_id, \
                    &new_assembler, \
                    &argp)) != 0)
    { goto fail; }

    /* Generate icode */
    if ((ret = multiple_icg_asm_generate_icode(err, icode, res_id, new_assembler)) != 0)
    { goto fail; }

    va_end(argp);

    goto done;
fail:
done:
    if (new_assembler != NULL) multiply_assembler_destroy(new_assembler);
    return ret;
}

