/* Bytecode File Generator
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "multiple_ir.h"
#include "multiple_bytecode.h"
#include "multiple_err.h"

#include "vm_opcode.h"
#include "vm_types.h"

/************************************************
 * File Header
 * Segment Header
 * Segment Info
 *
 * Segments
 ************************************************/


/************************************************
 * File Header (Offset:0) (Size:20)
 ************************************************
 *
 * Magic Key               : uint8_t[4] = "CLNU"
 * File Format Version     : uint32_t   = 0x1
 * Platform                : uint8_t[4] = "CLVM"
 * Platform Version        : uint32_t   = 0x1
 * Segment Header Offset   : uint32_t
 *
 ************************************************/


/************************************************
 * Segment Header (Offset:20) (4 + 4 * segment_count)
 ************************************************
 *
 * Segment Count           : uint32_t
 * Segment Info #1 Offset  : uint32_t
 * Segment Info #2 Offset  : uint32_t
 * ...
 * Segment Info #n Offset  : uint32_t
 *
 ************************************************/


/************************************************
 * Segment Info (Offset:20 + 4 + 4 * segment_count) (Size:20)
 ************************************************
 * 
 * Segment Name Length     : uint32_t (Should Always be 4)
 * Segment Name            : uint8_t[4]
 * Segment Offset          : uint32_t
 * Segment Size            : uint32_t
 *
 ************************************************/

#define SEGMENT_NAME_LEN_MAX 4

#define SEGMENT_TYPE_TEXT   ".txt"
#define SEGMENT_TYPE_DATA   ".dat"
#define SEGMENT_TYPE_EXPORT ".exp"
/*#define SEGMENT_TYPE_DEBUG  ".dbg"*/
/*#define SEGMENT_TYPE_INFO   ".ifo"*/
/*#define SEGMENT_TYPE_SOURCE ".src"*/

struct segment_info
{
    uint32_t name_len;
	uint8_t name[SEGMENT_NAME_LEN_MAX];
    uint32_t offset;
    uint32_t size;
};

/************************************************
 * .data Segment (4 + n * 4 Bytes)
 ************************************************
 * 
 * .data Segment Item Count : uint32_t
 * Segment Item #1 Offset
 * Segment Item #2 Offset
 * ...
 * Segment Item #n Offset
 *
 ************************************************/

/************************************************
 * .data Segment Item (12 + round_up(size) Bytes)
 ************************************************
 *
 * id                       : uint32_t
 * type                     : uint32_t
 * size                     : uint32_t
 * data                     : uint8_t[size]
 *
 * Note: data field round up to 4 bytes
 *
 ************************************************/


/************************************************
 * .text Segment
 ************************************************
 * 
 * 4 bytes for every instrument
 * opcode                  : uint16_t
 * operand                 : uint16_t
 *
 * Note: an operand is an identifier that represents 
 *       an item in .data section.
 *
 ************************************************/

static uint32_t round_up_to_4(uint32_t value)
{
    return ((value + 3) & (~3U));
}

static int write_u32(struct multiple_error *err, const uint32_t value, FILE *fp)
{
    if (fwrite(&value, sizeof(uint32_t), 1, fp) < 1) 
    {
        multiple_error_update(err, -MULTIPLE_ERR_BYTECODE, "error: failed to write data");
        return -MULTIPLE_ERR_BYTECODE;
    }

    return 0;
}

static int write_str(struct multiple_error *err, const char *str, const size_t len, FILE *fp)
{
    if (fwrite(str, len, 1, fp) < 1)
    {
        multiple_error_update(err, -MULTIPLE_ERR_BYTECODE, "error: failed to write data");
        return -MULTIPLE_ERR_BYTECODE;
    }

    return 0;
}

static int write_str_round_up_to_4(struct multiple_error *err, char *str, size_t len, FILE *fp)
{
    size_t null_len;

    if (len == 0) return 0;
    null_len = (size_t)round_up_to_4((uint32_t)len) - len;
    /* real content */
    if (fwrite(str, len, 1, fp) < 1)
    {
        multiple_error_update(err, -MULTIPLE_ERR_BYTECODE, "error: failed to write data");
        return -MULTIPLE_ERR_BYTECODE;
    }
    /* dummy */
    if (null_len > 0)
    {
        while (null_len-- > 0)
        {
            fputc('\0', fp);
        }
    }

    return 0;
}

static int bytecode_gen_header(struct multiple_error *err, struct multiple_ir *icode, FILE *fp, long *offset, long *offset_section_header)
{
    int ret = 0;
    unsigned int section_count = 0;
    unsigned int i;

    *offset = 0;
    *offset_section_header = 0;

    if (icode == NULL)
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }

    /* File Header */
    write_str(err, "CLNU", 4, fp); /* Magic Key */
    write_u32(err, 0x1, fp); /* File Format Version */
    write_str(err, "CLVM", 4, fp); /* Platform */
    write_u32(err, 0x1, fp); /* Platform Version */
    write_u32(err, 20, fp); /* Section Header Offset */

    /* Section Header */
    if ((icode->data_section != NULL) && (icode->data_section->size > 0)) section_count++;
    if ((icode->text_section != NULL) && (icode->text_section->size > 0)) section_count++;
    if ((icode->export_section!= NULL) && (icode->export_section->size > 0)) section_count++;
    write_u32(err, section_count, fp); /* Section Count */
    for (i = 0; i != section_count; i++)
    {
        write_u32(err, 20 + (4 + section_count * 4) + i * 16, fp); /* Point to Section Info */
    }
    *offset_section_header = 20 + (4 + (long int)section_count * 4);

    /* Section Info */
    if ((icode->data_section != NULL) && (icode->data_section->size > 0))
    {
        write_u32(err, 4, fp); /* Name Length */
        write_str(err, SEGMENT_TYPE_DATA, 4, fp); /* Name */
        write_u32(err, 0, fp); /* Offset */
        write_u32(err, 0, fp); /* Size */
    }
    if ((icode->text_section != NULL) && (icode->text_section->size > 0))
    {
        write_u32(err, 4, fp); /* Name Length */
        write_str(err, SEGMENT_TYPE_TEXT, 4, fp); /* Name */
        write_u32(err, 0, fp); /* Offset */
        write_u32(err, 0, fp); /* Size */
    }
    if ((icode->export_section != NULL) && (icode->export_section->size > 0))
    {
        write_u32(err, 4, fp); /* Name Length */
        write_str(err, SEGMENT_TYPE_EXPORT, 4, fp); /* Name */
        write_u32(err, 0, fp); /* Offset */
        write_u32(err, 0, fp); /* Size */
    }
    *offset = 20 + (4 + (long int)section_count * 4) + ((long int)section_count * 16);

    ret = 0;
    return ret;
}

static int bytecode_gen_data_section(struct multiple_error *err, struct multiple_ir *icode, FILE *fp, long int *offset, long offset_section_header, unsigned int *section_index)
{
    struct multiple_ir_data_section_item *data_section_item_cur;
    long int offset_local;
    unsigned long section_size = 0;

    if (icode == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }
    if ((icode->data_section == NULL) || (icode->data_section->size == 0)) return 0;

    /* Write Section Offset */
    fseek(fp, offset_section_header + (long int)(*section_index * 16) + 8, SEEK_SET);
    write_u32(err, (uint32_t)(*offset), fp);
    fseek(fp, *offset, SEEK_SET);

    /* Item Count */ 
    write_u32(err, (uint32_t)(icode->data_section->size), fp); 
    *offset += 4;

    /* Keep space for item offsets */
    offset_local = *offset;
    offset_local += (long int)icode->data_section->size * 4; 

	section_size = 4 + (long unsigned int)icode->data_section->size * 4;

    /* Write Item Offsets */
    data_section_item_cur = icode->data_section->begin;
    while (data_section_item_cur != NULL)
    {
        write_u32(err, (uint32_t)offset_local, fp); /* Write "current" offset */
        offset_local += 3 * 4 + (long int)round_up_to_4(data_section_item_cur->size);  /* Move on depends on size */
        section_size += 3 * 4 + round_up_to_4(data_section_item_cur->size); 
        data_section_item_cur = data_section_item_cur->next; 
    }

    /* Write data section contents */
    data_section_item_cur = icode->data_section->begin;
    while (data_section_item_cur != NULL)
    {
        write_u32(err, data_section_item_cur->id, fp);
        write_u32(err, data_section_item_cur->type, fp);
        write_u32(err, data_section_item_cur->size, fp);
        switch (data_section_item_cur->type)
        {
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_UNKNOWN:
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NONE:
                break;

            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INT:
                write_str_round_up_to_4(err, (char *)&data_section_item_cur->u.value_int, (size_t)data_section_item_cur->size, fp);
                break;
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_FLOAT:
                write_str_round_up_to_4(err, (char *)&data_section_item_cur->u.value_float, (size_t)data_section_item_cur->size, fp);
                break;
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_CHAR:
                write_str_round_up_to_4(err, (char *)&data_section_item_cur->u.value_char, (size_t)data_section_item_cur->size, fp);
                break;
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_BOOL:
                write_str_round_up_to_4(err, (char *)&data_section_item_cur->u.value_bool, (size_t)data_section_item_cur->size, fp);
                break;
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_NAN:
                write_str_round_up_to_4(err, (char *)&data_section_item_cur->u.signed_nan, (size_t)data_section_item_cur->size, fp);
                break;
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_INF:
                write_str_round_up_to_4(err, (char *)&data_section_item_cur->u.signed_inf, (size_t)data_section_item_cur->size, fp);
                break;
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_IDENTIFIER:
            case MULTIPLE_IR_DATA_SECTION_ITEM_TYPE_STR:
                write_str_round_up_to_4(err, (char *)&data_section_item_cur->u.value_str.str, (size_t)data_section_item_cur->size, fp);
                break;
        }
        data_section_item_cur = data_section_item_cur->next; 
    }

    *offset = offset_local;

    /* Write Section Size */
    fseek(fp, offset_section_header + (long int)(*section_index * 16) + 12, SEEK_SET);
    write_u32(err, (uint32_t)section_size, fp);
    fseek(fp, *offset, SEEK_SET);

    /* Update Section Index */
    *section_index += 1;

    return 0;
}

static int bytecode_gen_text_section(struct multiple_error *err, struct multiple_ir *icode, FILE *fp, long *offset, long offset_section_header, unsigned int *section_index)
{
    struct multiple_ir_text_section_item *text_section_item_cur;
    unsigned long section_size = 0;

    if (icode == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }
    if ((icode->text_section == NULL) || (icode->text_section->size == 0)) return 0;

    /* Write Section Offset */
    fseek(fp, offset_section_header + (long int)(*section_index * 16) + 8, SEEK_SET);
    write_u32(err, (uint32_t)(*offset), fp);
    fseek(fp, *offset, SEEK_SET);

    /* Item Count (Instrument Count) */ 
    write_u32(err, (uint32_t)(icode->text_section->size), fp); 
    *offset += 4;

    /* Instruments */
    text_section_item_cur = icode->text_section->begin;
    while (text_section_item_cur != NULL)
    {
        write_u32(err, text_section_item_cur->opcode, fp); /* opcode */
        write_u32(err, text_section_item_cur->operand, fp); /* operand */
        *offset += 8;
        text_section_item_cur = text_section_item_cur->next; 
    }
    section_size = 4 + (4 + 4) * (unsigned long)icode->text_section->size;

    /* Write Section Size */
    fseek(fp, offset_section_header + (long int)(*section_index * 16) + 12, SEEK_SET);
    write_u32(err, (uint32_t)section_size, fp);
    fseek(fp, *offset, SEEK_SET);

    /* Update Section Index */
    *section_index += 1;

    return 0;
}

static int bytecode_gen_export_section(struct multiple_error *err, struct multiple_ir *icode, FILE *fp, long *offset, long offset_section_header, unsigned int *section_index)
{
    struct multiple_ir_export_section_item *export_section_item_cur;
    unsigned long section_size = 0;
    size_t index;

    if (icode == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }
    if ((icode->export_section == NULL) || (icode->export_section->size == 0)) return 0;

    /* Write Section Offset */
    fseek(fp, offset_section_header + (long int)(*section_index * 16) + 8, SEEK_SET);
    write_u32(err, (uint32_t)(*offset), fp);
    fseek(fp, *offset, SEEK_SET);

    /* Item Count (Export Item Count) */ 
    write_u32(err, (uint32_t)(icode->export_section->size), fp); 
    *offset += 4;
    section_size = 4;

    export_section_item_cur = icode->export_section->begin;
    while (export_section_item_cur != NULL)
    {
        /* Function Name */
        write_u32(err, export_section_item_cur->name, fp);

        /* Arguments Count */
        write_u32(err, (uint32_t)export_section_item_cur->args_count, fp);

        /* Arguments */
        for (index = 0; index != export_section_item_cur->args_count; index++)
        {
            write_u32(err, export_section_item_cur->args[index], fp);
        }

        *offset += 8 + 4 * (long int)export_section_item_cur->args_count;
        section_size += 8 + 4 * (unsigned long)export_section_item_cur->args_count;

        export_section_item_cur = export_section_item_cur->next; 
    }


    /* Write Section Size */
    fseek(fp, offset_section_header + (long int)(*section_index * 16) + 12, SEEK_SET);
    write_u32(err, (uint32_t)section_size, fp);
    fseek(fp, *offset, SEEK_SET);

    /* Update Section Index */
    *section_index += 1;

    return 0;
}

static int bytecode_gen_import_section(struct multiple_error *err, struct multiple_ir *icode, FILE *fp, long *offset, long offset_section_header, unsigned int *section_index)
{
    struct multiple_ir_import_section_item *import_section_item_cur;
    unsigned long section_size = 0;

    if (icode == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }
    if ((icode->import_section == NULL) || (icode->import_section->size == 0)) return 0;

    /* Write Section Offset */
    fseek(fp, offset_section_header + (long int)(*section_index * 16) + 8, SEEK_SET);
    write_u32(err, (uint32_t)(*offset), fp);
    fseek(fp, *offset, SEEK_SET);

    /* Item Count (Import Item Count) */ 
    write_u32(err, (uint32_t)(icode->export_section->size), fp); 
    *offset += 4;
    section_size = 4;

    import_section_item_cur = icode->import_section->begin;
    while (import_section_item_cur != NULL)
    {
        /* Module Name */
        write_u32(err, import_section_item_cur->name, fp);

        import_section_item_cur = import_section_item_cur->next;
    }

    /* Write Section Size */
    fseek(fp, offset_section_header + (long int)(*section_index * 16) + 12, SEEK_SET);
    write_u32(err, (uint32_t)section_size, fp);
    fseek(fp, *offset, SEEK_SET);

    /* Update Section Index */
    *section_index += 1;

    return 0;
}

static int bytecode_gen_module_section(struct multiple_error *err, struct multiple_ir *icode, FILE *fp, long *offset, long offset_section_header, unsigned int *section_index)
{
    unsigned long section_size = 0;

    if (icode == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }
    if ((icode->module_section == NULL) || (icode->module_section->enabled == 0)) return 0;

    /* Write Section Offset */
    fseek(fp, offset_section_header + (long int)(*section_index * 16) + 8, SEEK_SET);
    write_u32(err, (uint32_t)(*offset), fp);
    fseek(fp, *offset, SEEK_SET);

    /* Item Count (Module Item Count) */ 
    write_u32(err, 1, fp); 
    *offset += 4;
    section_size = 4;

    /* Module Name */
    write_u32(err, icode->module_section->name, fp);

    /* Write Section Size */
    fseek(fp, offset_section_header + (long int)(*section_index * 16) + 12, SEEK_SET);
    write_u32(err, (uint32_t)(section_size), fp);
    fseek(fp, *offset, SEEK_SET);

    /* Update Section Index */
    *section_index += 1;

    return 0;
}


int multiple_bytecode_gen(struct multiple_error *err, FILE *fp_out, struct multiple_ir *icode)
{
    int ret = 0;
    long offset, offset_section_header;
    unsigned int section_index;

    if (icode == NULL) 
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }

    /* First, generate header and section header */
    if ((ret = bytecode_gen_header(err, icode, fp_out, &offset, &offset_section_header)) != 0)
    {
        goto fail;
    }
    /* offset section header now pointes to the first element in section header (normally 24) */

    section_index = 0;

    if ((ret = bytecode_gen_data_section(err, icode, fp_out, &offset, offset_section_header, &section_index)) != 0)
    {
        goto fail;
    }

    if ((ret = bytecode_gen_text_section(err, icode, fp_out, &offset, offset_section_header, &section_index)) != 0)
    {
        goto fail;
    }

    if ((ret = bytecode_gen_export_section(err, icode, fp_out, &offset, offset_section_header, &section_index)) != 0)
    {
        goto fail;
    }

    if ((ret = bytecode_gen_import_section(err, icode, fp_out, &offset, offset_section_header, &section_index)) != 0)
    {
        goto fail;
    }

    if ((ret = bytecode_gen_module_section(err, icode, fp_out, &offset, offset_section_header, &section_index)) != 0)
    {
        goto fail;
    }

    ret = 0;
    goto done;
fail:
done:
    return ret;
}

