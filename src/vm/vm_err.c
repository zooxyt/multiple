/* Virtual Machine Error Handling
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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "vm_err.h"
#include "vm_infrastructure.h"


#define CHAR_CR 13
#define CHAR_LF 10

#define IS_EOL(x) (((x)=='\r')||((x)=='\n'))

#define EOL_UNKNOWN 0 /* unknown */
#define EOL_UNIX 1 /* <LF> */
#define EOL_DOS 2 /* <CR><LF> */
#define EOL_MAC 3 /* <CR> */

/* Runtime Error */

int vm_err_init(struct vm_err *rail)
{
    rail->malloc = &malloc;
    rail->free = &free;

    rail->module = 0;
    rail->pc = 0;
    rail->opcode = 0;
    rail->operand = 0;

    rail->occurred = 0;
    rail->number_enabled = 0;
    rail->number = 0;
    rail->description_enabled = 0;
    rail->description[0] = '\0';

    rail->source_level_enabled = 0;
    rail->pathname[0] = '\0';
    rail->pathname_enabled = 0;
    rail->source_line_no = 0;
    rail->source_line_text[0] = '\0';

    return 0;
}

int vm_err_clear(struct vm_err *rail)
{
    rail->occurred = 0;
    rail->number_enabled = 0;
    rail->number = 0;
    rail->description_enabled = 0;
    rail->description[0] = '\0';
    rail->module_name[0] = '\0';

    rail->source_level_enabled = 0;
    rail->pathname[0] = '\0';
    rail->pathname_enabled = 0;
    rail->source_line_no = 0;
    rail->source_line_text[0] = '\0';

    return 0;
}

int vm_err_occurred(struct vm_err *rail)
{
    return rail->occurred;
}

int vm_err_update(struct vm_err *rail, int number, const char *fmt, ...)
{
    va_list args;


    if (rail->number_enabled == 0)
    {
        if (number != 0)
        {
            rail->number = number;
            rail->number_enabled = 1;
            rail->occurred = 1;
        }
    }

    if (rail->description_enabled == 0)
    {
        if (fmt != NULL)
        {
            va_start(args, fmt);
            vsnprintf(rail->description, VM_ERR_DESCRIPTION_LEN, fmt, args);
            va_end(args);
            rail->description_enabled = 1;
            rail->occurred = 1;
        }
    }

    return 0;
}

#define EOL_DETECT_STATUS_INIT 0
#define EOL_DETECT_STATUS_CR 1
#define EOL_DETECT_STATUS_FINISH 2

/* Detect End Of Line type */
static int eol_detect_simple(const char *text, const size_t len)
{
    int status = EOL_DETECT_STATUS_INIT;
    int result = EOL_UNIX;
    int ch;
    const char *text_p = text, *text_endp = text_p + len;
    if (text == NULL) { return -EOL_UNKNOWN; }
    while (text_p != text_endp)
    {
        ch = *text_p;
        switch (status)
        {
            case EOL_DETECT_STATUS_INIT:
                if (ch == CHAR_CR)
                { result = EOL_MAC; status = EOL_DETECT_STATUS_CR; }
                else if (ch == CHAR_LF)
                { result = EOL_UNIX; status = EOL_DETECT_STATUS_FINISH; } 
                break;
            case EOL_DETECT_STATUS_CR:
                if (ch == CHAR_LF)
                { result = EOL_DOS; status = EOL_DETECT_STATUS_FINISH; }
                else
                { status = EOL_DETECT_STATUS_FINISH; }
                break;
            case EOL_DETECT_STATUS_FINISH:
                goto done;
                break;
        }
        text_p++;
    }
done:
    return result;
}

enum {
    LEX_STATUS_INIT = 0,

    LEX_STATUS_EOL, /* EOL of Windows? Mac? */
};

static int copy_line_from_text(char *buf, size_t buf_len, char *text, size_t len, uint32_t line_no)
{
    int eol_type = eol_detect_simple(text, len);
    char *text_p = text, *text_endp = text + len;
    int ch;
    int status = LEX_STATUS_INIT;
    uint32_t pos_ln = 1;
    int in_copy = 0;
    size_t in_copy_len = 0;
    char *buf_p = buf;

    while (text_p != text_endp)
    {
        ch = *text_p;
        switch (status)
        {
            case LEX_STATUS_EOL:
                status = LEX_STATUS_INIT;
                break;
            case LEX_STATUS_INIT:
                if (IS_EOL(ch)) 
                {
                    if (pos_ln == line_no)
                    {
                        in_copy = 0;
                    }

                    /* Reset location */
                    pos_ln += 1;

                    if (pos_ln == line_no)
                    {
                        in_copy = 1;
                    }

                    switch (eol_type)
                    {
                        case EOL_UNIX:
                        case EOL_MAC:
                            break;
                        case EOL_DOS:
                            status = LEX_STATUS_EOL;
                            break;
                    }
                }
                else
                {
                    if (in_copy != 0)
                    {
                        *buf_p++ = (char)ch;
                        in_copy_len++;
                        if (in_copy_len >= buf_len) goto finish;
                    }
                }
                break;
        }
        text_p += 1;
    }
finish:
    *buf_p = '\0';

    return 0;
}

int vm_err_final(struct vm_err *rail)
{
    size_t i;

    if (rail->occurred)
    {
        /* Module Name */
        if (rail->module != NULL)
        {
            memcpy(rail->module_name, rail->module->name, rail->module->name_len);
            rail->module_name[rail->module->name_len] = '\0';
			/* Source Level */
			if ((rail->module->source_section != NULL) && (rail->module->source_section->code != NULL))
			{
				for (i = 0; i != rail->module->debug_section->size; i++)
				{
					if (rail->module->debug_section->debugs[i].asm_ln == rail->pc)
					{
						rail->source_level_enabled = 1;
						rail->source_line_no = rail->module->debug_section->debugs[i].source_ln_start;
						copy_line_from_text(rail->source_line_text, VM_ERR_DESCRIPTION_LEN, \
								rail->module->source_section->code, rail->module->source_section->len, \
								rail->source_line_no);
						break;
					}
				}
			}
        }
        else
        {
            rail->module_name[0] = '\0';
        }
    }

    return 0;
}

int vm_err_print(struct vm_err *rail)
{
    if (rail->occurred == 0) return 0;

    if (rail->description_enabled != 0)
    {
        fputs(rail->description, stderr);
        fprintf(stderr, "\n");

        if (rail->source_level_enabled != 0)
        {
            fprintf(stderr, "%s: ", rail->module_name);
            fprintf(stderr, "%u: ", rail->source_line_no);
            fprintf(stderr, "%s", rail->source_line_text);
            fprintf(stderr, "\n");
        }
        else
        {
            fprintf(stderr, "core: module: %s, pc: %u, opcode:operand: %u:%u", \
                    rail->module_name, rail->pc, rail->opcode, rail->operand);
            fprintf(stderr, "\n");
        }
    }
    else if (rail->number != 0)
    {
        printf("error: undefined error\n");
        exit(1);
    }

    return 0;
}

