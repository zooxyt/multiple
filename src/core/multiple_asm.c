/* Assembly Language Source File Generator
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

#include "selfcheck.h"

#include <string.h>

#include "multiple_ir.h"
#include "multiple_misc.h"
#include "multiple_err.h"

#include "vm_opcode.h"
#include "multiple_asm.h"

#define BUFFER_SIZE 4096
#define DATA_SECTION_ITEM_TYPE_INT_LEN 3
#define DATA_SECTION_ITEM_TYPE_FLOAT_LEN 5
#define DATA_SECTION_ITEM_TYPE_STR_LEN 3
#define DATA_SECTION_ITEM_TYPE_CHAR_LEN 4
#define DATA_SECTION_ITEM_TYPE_NONE_LEN 4
#define DATA_SECTION_ITEM_TYPE_BOOL_LEN 4
#define DATA_SECTION_ITEM_TYPE_NAN_LEN 3
#define DATA_SECTION_ITEM_TYPE_INF_LEN 3
#define DATA_SECTION_ITEM_TYPE_IDENTIFIER_LEN 2


static int print_white_space(FILE *fp_out, int size)
{
    if (size < 0) return 0;
    while (size-- > 0) { fputc(' ', fp_out); }
    return 0;
}

static struct multiple_ir_data_section_item *asm_code_gen_data_section_lookup(struct multiple_ir_data_section* data_section, uint32_t id)
{
    struct multiple_ir_data_section_item *data_section_item_cur;

    data_section_item_cur = data_section->begin;
    while (data_section_item_cur != NULL)
    {
        if (data_section_item_cur->id == id) { return data_section_item_cur; }
        data_section_item_cur = data_section_item_cur->next; 
    }
    return NULL;
}

static int asm_code_gen_data_section_item(FILE *fp_out, struct multiple_ir_data_section_item *data_section_item)
{
    int ret = 0;

    switch (data_section_item->type)
    {
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INT: 
            fprintf(fp_out, "%d", data_section_item->u.value_int);
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_CHAR: 
            fprintf(fp_out, "%u", (unsigned int)(data_section_item->u.value_char));
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_STR: 
            print_escaped_string(fp_out, \
                    data_section_item->u.value_str.str, \
                    data_section_item->u.value_str.len);
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER:
            fwrite(data_section_item->u.value_str.str, \
                    data_section_item->u.value_str.len, \
                    1, fp_out);
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL:
            switch (data_section_item->u.value_bool)
            {
                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL_FALSE:
                    fprintf(fp_out, "false");
                    break;
                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL_TRUE:
                    fprintf(fp_out, "true");
                    break;
            }
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NONE:
            fprintf(fp_out, "none");
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_FLOAT: 
            fprintf(fp_out, "%016lX", (unsigned long int)(data_section_item->u.value_float));
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NAN:
            switch (data_section_item->u.signed_nan)
            {
                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NAN_NEG:
                    fputc('-', stdout);
                    break;
            }
            fprintf(fp_out, "nan");
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INF:
            switch (data_section_item->u.signed_inf)
            {
                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INF_NEG:
                    fputc('-', stdout);
                    break;
            }
            fprintf(fp_out, "inf");
            break;
        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_UNKNOWN: 
            break;
    }

    return ret;
}

static int asm_code_gen_data_section(struct multiple_error *err, FILE *fp_out, struct multiple_ir *icode)
{
    int ret = 0;
    struct multiple_ir_data_section_item *data_section_item_cur;
    char buffer[BUFFER_SIZE];
    int written_len = 0;
    int id_len = 0, type_len = 0, size_len = 0;

    if (icode->data_section->size > 0)
    {
        /* Calculate length of each field */
        data_section_item_cur = icode->data_section->begin;
        while (data_section_item_cur != NULL)
        {
            written_len = sprintf(buffer, "%u", data_section_item_cur->id);
            if (written_len > id_len) id_len = written_len;

            switch (data_section_item_cur->type)
            {
				case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INT: 
                    written_len = DATA_SECTION_ITEM_TYPE_INT_LEN;
                    break;
				case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_FLOAT:
                    written_len = DATA_SECTION_ITEM_TYPE_FLOAT_LEN;
                    break;
				case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_STR: 
                    written_len = DATA_SECTION_ITEM_TYPE_STR_LEN;
                    break;
				case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_CHAR:
                    written_len = DATA_SECTION_ITEM_TYPE_CHAR_LEN;
                    break;
				case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NONE:
                    written_len = DATA_SECTION_ITEM_TYPE_NONE_LEN;
                    break;
				case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL:
                    written_len = DATA_SECTION_ITEM_TYPE_BOOL_LEN;
                    break;
                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER:
                    written_len = DATA_SECTION_ITEM_TYPE_IDENTIFIER_LEN;
                    break;
                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NAN:
                    written_len = DATA_SECTION_ITEM_TYPE_NAN_LEN;
                    break;
                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INF:
                    written_len = DATA_SECTION_ITEM_TYPE_INF_LEN;
                    break;
                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_UNKNOWN:
                    written_len = 0; 
                    break;
            }
            if (written_len > type_len) type_len = written_len;

            written_len = sprintf(buffer, "%u", data_section_item_cur->size);
            if (written_len > size_len) size_len = written_len;

            data_section_item_cur = data_section_item_cur->next; 
        }

        /* Write .data segment */
        fprintf(fp_out, ".data\n");
        data_section_item_cur = icode->data_section->begin;
        while (data_section_item_cur != NULL)
        {
            fprintf(fp_out, "  ");
            written_len = fprintf(fp_out, "%u", data_section_item_cur->id);
            if (id_len > written_len) print_white_space(fp_out, id_len - written_len);

            fprintf(fp_out, " ");
            switch (data_section_item_cur->type)
            {
                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INT: 
                    written_len = fprintf(fp_out, "int"); break;
                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_STR: 
                    written_len = fprintf(fp_out, "str"); break;
                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NONE: 
                    written_len = fprintf(fp_out, "none"); break;
                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_CHAR: 
                    written_len = fprintf(fp_out, "char"); break;
                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL: 
                    written_len = fprintf(fp_out, "bool"); break;
                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER: 
                    written_len = fprintf(fp_out, "id"); break;
                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_FLOAT: 
                    written_len = fprintf(fp_out, "float"); break;
                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NAN: 
                    written_len = fprintf(fp_out, "nan"); break;
                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INF: 
                    written_len = fprintf(fp_out, "inf"); break;
                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_UNKNOWN: 
                    multiple_error_update(err, -MULTIPLE_ERR_ASM, "error: unsupported data type : %d", data_section_item_cur->type);
                    return -MULTIPLE_ERR_ASM;
            }
            if (type_len > written_len) print_white_space(fp_out, type_len - written_len);

            fprintf(fp_out, " ");
            written_len = fprintf(fp_out, "%u", data_section_item_cur->size);
            if (size_len > written_len) print_white_space(fp_out, size_len - written_len);

            fprintf(fp_out, " ");
            asm_code_gen_data_section_item(fp_out, data_section_item_cur);
            fprintf(fp_out, "\n");

            data_section_item_cur = data_section_item_cur->next; 
        }
    }
    return ret;
}

static int asm_code_gen_text_section_operand(struct multiple_error *err, FILE *fp_out, struct multiple_ir_data_section *data_section, uint32_t opcode, uint32_t operand)
{
    struct multiple_ir_data_section_item *data_section_item_cur;

    /* Operand */
    switch (opcode)
    {
        case OP_JMP:
        case OP_JMPC:
        case OP_JMPR:
        case OP_JMPCR:
        case OP_RETURNTO:
        case OP_NFUNCMK:
        case OP_LAMBDAMK:
        case OP_CONTMK:
        case OP_PROMMK:
        case OP_TRAPSET:
        case OP_LSTMK:
        case OP_ARRMK:
        case OP_TUPMK:
        case OP_HASHMK:
        case OP_LIFT:
        case OP_CONVERT:
        case OP_FASTLIB:

            /* Operands are absolute offsets */
            fprintf(fp_out, "  ");
            fprintf(fp_out, "%u", operand);
            break;

        case OP_DEF: 
        case OP_ARG: 
        case OP_LSTARG: 
        case OP_ARGC: 
        case OP_ARGCL: 
        case OP_LSTARGC: 
        case OP_PUSH:
        case OP_PUSHC:

        case OP_VARP:
        case OP_VARCP:
        case OP_VARCLP:

        case OP_POP:
        case OP_POPC:
        case OP_POPCL:
        case OP_POPCX:
        case OP_PUSHG:
        case OP_POPG:
        case OP_POPM:
        case OP_INT:
        case OP_SYMMK:

            /* Operands are data section items */
            fprintf(fp_out, "  ");
            fprintf(fp_out, "%u", operand);

            /* Comment */
            fprintf(fp_out, "  ; ");
            data_section_item_cur = data_section->begin;
            while (data_section_item_cur != NULL)
            {
                if (data_section_item_cur->id == operand)
                {
                    switch (data_section_item_cur->type)
                    {
                        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER:
                            print_escaped_string(fp_out, \
                                    data_section_item_cur->u.value_str.str, \
                                    data_section_item_cur->u.value_str.len);
                            break;
                        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_STR:
                            print_escaped_string(fp_out, \
                                    data_section_item_cur->u.value_str.str, \
                                    data_section_item_cur->u.value_str.len);
                            break;
                        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INT:
                            printf("%d", data_section_item_cur->u.value_int);
                            break;
                        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_CHAR: 
                            printf("%u", (unsigned int)(data_section_item_cur->u.value_char));
                            break;
                        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_FLOAT:
                            printf("%016lX", (unsigned long int)(data_section_item_cur->u.value_float));
                            break;
                        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL:
                            switch (data_section_item_cur->u.value_bool)
                            {
                                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL_FALSE:
                                    printf("false");
                                    break;
                                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL_TRUE:
                                    printf("true");
                                    break;
                            }
                            break;
                        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NONE:
                            printf("none");
                            break;
                        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NAN:
                            switch (data_section_item_cur->u.signed_nan)
                            {
                                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NAN_NEG:
                                    fputc('-', stdout);
                                    break;
                            }
                            printf("nan");
                            break;
                        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INF:
                            switch (data_section_item_cur->u.signed_inf)
                            {
                                case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INF_NEG:
                                    fputc('-', stdout);
                                    break;
                            }
                            printf("inf");
                            break;
                        case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_UNKNOWN:
                            multiple_error_update(err, -MULTIPLE_ERR_ASM, "error: unsupported data type : %d", data_section_item_cur->type);
                            return -MULTIPLE_ERR_ASM;
                    }
                    break;
                }
                data_section_item_cur = data_section_item_cur->next; 
            }
            break;
        default:
            break;
    }
    fprintf(fp_out, "\n");

    return 0;
}

static int asm_code_gen_text_section_operand_from_text_section_item(struct multiple_error *err, FILE *fp_out, struct multiple_ir *icode, struct multiple_ir_text_section_item *text_section_item_cur)
{
    asm_code_gen_text_section_operand(err, fp_out, icode->data_section, text_section_item_cur->opcode, text_section_item_cur->operand);
    return 0;
}

static int asm_code_gen_text_section(struct multiple_error *err, FILE *fp_out, struct multiple_ir *icode)
{
    int ret = 0;
    struct multiple_ir_text_section_item *text_section_item_cur;
    int opcode_len = 0;

    char *asm_str;
    size_t asm_len;
    unsigned int ln = 0;

    if (icode == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }


    if (icode->text_section->size > 0)
    {
        fprintf(fp_out, ".text\n");
        text_section_item_cur = icode->text_section->begin;

        /* Calculate width of the opcode field */
        while (text_section_item_cur != NULL)
        {
            if (virtual_machine_opcode_to_instrument(&asm_str, &asm_len, text_section_item_cur->opcode) == 0)
            {
                if ((int)asm_len > opcode_len) opcode_len = (int)asm_len;
            }
            else
            {
                multiple_error_update(err, -MULTIPLE_ERR_ASM, "error: unsupported opcode %d\n", text_section_item_cur->opcode);
                ret = -MULTIPLE_ERR_ASM; 
                goto fail; 
            }
            text_section_item_cur = text_section_item_cur->next; 
        }

        text_section_item_cur = icode->text_section->begin;
        while (text_section_item_cur != NULL)
        {
            /* offset */
            fprintf(fp_out, "%4u ", ln++);

            /* instrument */
            fprintf(fp_out, "  ");
            if (virtual_machine_opcode_to_instrument(&asm_str, &asm_len, text_section_item_cur->opcode) == 0)
            {
                fwrite(asm_str, asm_len, 1, fp_out);
                if (opcode_len > (int)asm_len) print_white_space(fp_out, opcode_len - (int)asm_len);
            }
            /* operand */
            asm_code_gen_text_section_operand_from_text_section_item(err, fp_out, icode, text_section_item_cur);



            text_section_item_cur = text_section_item_cur->next; 
        }

    }

    ret = 0;
fail:
    return ret;
}

/* <instrument number> <function name> <argument count> <arguments...> */
static int asm_code_gen_export_section(struct multiple_error *err, FILE *fp_out, struct multiple_ir *icode)
{
    int ret = 0;
    size_t idx;
    struct multiple_ir_export_section_item *item_cur;
    char buffer[BUFFER_SIZE];
    int written_len, instrument_number_len = 5, function_name_len = 0, args_count_len = 0;
    struct multiple_ir_data_section_item *data_section_item_target;

    if (icode == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }
    if (icode->export_section->size > 0)
    {
        fprintf(fp_out, ".export\n");
    }


    instrument_number_len = sprintf(buffer, "%u", (unsigned int)(icode->text_section->size)) + 2;

    /* Length */
    item_cur = icode->export_section->begin;
    while (item_cur != NULL)
    {
        if (item_cur->blank != 0)
        {
            written_len = sprintf(buffer, "%u", item_cur->instrument_number);
            if (written_len > instrument_number_len) instrument_number_len = written_len;
            written_len = sprintf(buffer, "%u", item_cur->name);
            if (written_len > function_name_len) function_name_len = written_len;
            written_len = sprintf(buffer, "%u", (unsigned int)item_cur->args_count);
            if (written_len > args_count_len) args_count_len = written_len;
        }
        item_cur = item_cur->next;
    }

    item_cur = icode->export_section->begin;
    while (item_cur != NULL)
    {
        if (item_cur->blank == 0)
        {
            fprintf(fp_out, "  ");

            /* instrument number */
            written_len = fprintf(fp_out, "%u", item_cur->instrument_number);
            print_white_space(fp_out, instrument_number_len - written_len);

            /* function name */
            data_section_item_target = asm_code_gen_data_section_lookup(icode->data_section, item_cur->name);
            asm_code_gen_data_section_item(fp_out, data_section_item_target);

            /* parameters */
            fputc('(', stdout);

            for (idx = 0; idx != item_cur->args_count; idx++)
            {
                if (idx != 0) { fputc(',', stdout); fputc(' ', stdout); }
                switch (item_cur->args_types[idx])
                {
                    case MULTIPLE_IR_EXPORT_SECTION_ITEM_ARGS_NORMAL:
                        break;
                    case MULTIPLE_IR_EXPORT_SECTION_ITEM_ARGS_LIST:
                    case MULTIPLE_IR_EXPORT_SECTION_ITEM_ARGS_LIST_REVERSE:
                        fprintf(fp_out, "*");
                        break;
                    case MULTIPLE_IR_EXPORT_SECTION_ITEM_ARGS_MAP:
                        break;
                }
                data_section_item_target = asm_code_gen_data_section_lookup(icode->data_section, item_cur->args[idx]);
                asm_code_gen_data_section_item(fp_out, data_section_item_target);
            }
            fputc(')', stdout);

            fprintf(fp_out, "\n");
        }

        item_cur = item_cur->next;
    }
    ret = 0;
    return ret;
}

/* asm_instrument source_line_start source_line_end */
static int asm_code_gen_debug_section(struct multiple_error *err, FILE *fp_out, struct multiple_ir *icode)
{
    int ret = 0;
    struct multiple_ir_debug_section_item *item_cur;
    int asm_ln_len = 0, source_ln_start_len = 0, source_ln_end_len = 0, written_len;
    char buffer[BUFFER_SIZE];

    if (icode == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }
    if (icode->debug_section->size > 0)
    {
        fprintf(fp_out, ".debug\n");
    }

    item_cur = icode->debug_section->begin;
    while (item_cur != NULL)
    {
        written_len = sprintf(buffer, "%u", item_cur->line_number_asm);
        if (written_len > asm_ln_len) asm_ln_len = written_len;
        written_len = sprintf(buffer, "%u", item_cur->line_number_source_start);
        if (written_len > source_ln_start_len) source_ln_start_len = written_len;
        written_len = sprintf(buffer, "%u", item_cur->line_number_source_end);
        if (written_len > source_ln_end_len) source_ln_end_len = written_len;

        item_cur = item_cur->next;
    }

    item_cur = icode->debug_section->begin;
    while (item_cur != NULL)
    {
        fprintf(fp_out, "  ");

        written_len = fprintf(fp_out, "%u", item_cur->line_number_asm);
        print_white_space(fp_out, asm_ln_len - written_len);
        fprintf(fp_out, " ");

        written_len = fprintf(fp_out, "%u", item_cur->line_number_source_start);
        print_white_space(fp_out, source_ln_start_len - written_len);
        fprintf(fp_out, " ");

        written_len = fprintf(fp_out, "%u", item_cur->line_number_source_end);
        print_white_space(fp_out, source_ln_end_len - written_len);
        fprintf(fp_out, " ");

        fprintf(fp_out, "\n");

        item_cur = item_cur->next;
    }

    return ret;
}

/* <module name> */
static int asm_code_gen_import_section(struct multiple_error *err, FILE *fp_out, struct multiple_ir *icode)
{
    int ret = 0;
    struct multiple_ir_import_section_item *item_cur;

    if (icode == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }
    if (icode->import_section->size > 0)
    {
        fprintf(fp_out, ".import\n");
    }

    item_cur = icode->import_section->begin;
    while (item_cur != NULL)
    {
        fprintf(fp_out, "  ");

        /* module name */
        fprintf(fp_out, "%u", item_cur->name);
        fprintf(fp_out, "\n");

        item_cur = item_cur->next;
    }
    ret = 0;
    return ret;
}

/* <module name> */
static int asm_code_gen_module_section(struct multiple_error *err, FILE *fp_out, struct multiple_ir *icode)
{
    if (icode == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }
    if (icode->module_section != NULL)
    {
        if (icode->module_section->enabled != 0)
        {
            fprintf(fp_out, ".module\n");
            /* module name */
            fprintf(fp_out, "  ");
            fprintf(fp_out, "%u", icode->module_section->name);
            fprintf(fp_out, "\n");
        }
    }

    return 0;
}

/* <source code length> */
static int asm_code_gen_source_section(struct multiple_error *err, FILE *fp_out, struct multiple_ir *icode)
{
    if (icode == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }
    if (icode->source_section != NULL)
    {
        if (icode->source_section->data != NULL)
        {
            fprintf(fp_out, ".source\n");
            /* module name */
            fprintf(fp_out, "  ");
            fprintf(fp_out, "%u", (unsigned int)icode->source_section->size);
            fprintf(fp_out, "\n");
        }
    }

    return 0;
}

int multiple_asm_code_gen(struct multiple_error *err, FILE *fp_out, struct multiple_ir *icode)
{
    int ret = 0;

    if (icode == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }

    if ((ret = asm_code_gen_data_section(err, fp_out, icode)) != 0)
    { goto fail; }
    if ((ret = asm_code_gen_text_section(err, fp_out, icode)) != 0)
    { goto fail; }
    if ((ret = asm_code_gen_export_section(err, fp_out, icode)) != 0)
    { goto fail; }
    if ((ret = asm_code_gen_debug_section(err, fp_out, icode)) != 0)
    { goto fail; }
    if ((ret = asm_code_gen_import_section(err, fp_out, icode)) != 0)
    { goto fail; }
    if ((ret = asm_code_gen_module_section(err, fp_out, icode)) != 0)
    { goto fail; }
    if ((ret = asm_code_gen_source_section(err, fp_out, icode)) != 0)
    { goto fail; }
    ret = 0;
fail:
    return ret;
}

