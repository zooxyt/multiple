/* Generic Lexical Scanner
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

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS 1
#endif

/* disable conditional expression is constant warning */
#ifdef _MSC_VER
#pragma warning( disable: 4127 ) 
#endif

#include <stdlib.h>
#include <string.h>

#include "multiply_lexer.h"

#include "multiple_err.h"

#define EOL_DETECT_STATUS_INIT 0
#define EOL_DETECT_STATUS_CR 1
#define EOL_DETECT_STATUS_FINISH 2

/* Detect End Of Line type */
int eol_detect(struct multiple_error *err, const char *text, const size_t len)
{
    int status = EOL_DETECT_STATUS_INIT;
    int result = EOL_UNIX;
    int ch;
    const char *text_p = text, *text_endp = text_p + len;
    if (text == NULL)
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }
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


struct token *token_new(void)
{
    struct token *new_token;
    if ((new_token = (struct token *)malloc(sizeof(struct token))) == NULL)
    {
        return NULL;
    }
    new_token->value = TOKEN_UNDEFINED;
    new_token->str = NULL;
    new_token->len = 0;
    new_token->pos_col = 0;
    new_token->pos_ln = 0;
    new_token->next = NULL;
    return new_token;
}

struct token *token_new_with_configure(\
        int value, char *str, size_t len, uint32_t num_col, uint32_t num_ln)
{
    struct token *new_token;
    if ((new_token = (struct token *)malloc(sizeof(struct token))) == NULL)
    {
        return NULL;
    }
    new_token->value = value;
    if (str == NULL)
    {
        new_token->str = NULL;
        new_token->len = 0;
    }
    else
    {
        if ((new_token->str = (char *)malloc(sizeof(char) * (len + 1))) == NULL)
        {
            free(new_token);
            return NULL;
        }
        new_token->len = len;
        if (len > 0) 
        {
            memcpy(new_token->str, str, len);
        }
        new_token->str[len] = '\0';
    }
    new_token->pos_col = num_col;
    new_token->pos_ln = num_ln;
    new_token->next = new_token->prev = NULL;

    return new_token;
}

int token_destroy(struct token *token)
{
    if (token == NULL) 
    {
        return -MULTIPLE_ERR_NULL_PTR;
    }

    if (token->str != NULL) free(token->str);
    free(token);

    return 0;
}

struct token *token_clone(struct token *token)
{
	struct token *new_token = NULL;
    if (token == NULL) return NULL;
    new_token = token_new_with_configure(\
            token->value,
            token->str, token->len,
            token->pos_col, token->pos_ln);
    if (new_token == NULL) return NULL;
    return new_token;
}

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

int token_print(struct token *token)
{
    int i;

    if (token == NULL) return -MULTIPLE_ERR_NULL_PTR;

    printf("(value=%4d, str=", token->value);
    if (token->len < 10)
    {
        for (i = 0; i != MIN((10 - (int)token->len), 10); i++)
        { putchar(' '); }
    }
    putchar('\"');
    if ((token->str != NULL) && (token->len > 0))
    { fwrite(token->str, token->len, 1, stdout); }
    putchar('\"');
    printf(", len=%4u, ln=%4u, col=%4u", (unsigned int)token->len, token->pos_ln, token->pos_col);
    printf(")\n");

    return 0;
}

struct token_list *token_list_new(void)
{
    struct token_list *new_list;

    new_list = (struct token_list *)malloc(sizeof(struct token_list));
    if (new_list == NULL) return NULL;
    new_list->begin = new_list->end = NULL;
    new_list->size = 0;

    return new_list;
}

int token_list_destroy(struct token_list *list)
{
    struct token *token_cur, *token_next;

	if (list == NULL) return -MULTIPLE_ERR_NULL_PTR;

    token_cur = list->begin;
    while (token_cur != NULL)
    {
        token_next = token_cur->next;
        token_destroy(token_cur);
        token_cur = token_next;
    }
    free(list);

    return 0;
}

int token_list_append(struct token_list *list, struct token *new_token)
{
    if ((list == NULL) || (new_token == NULL)) return -MULTIPLE_ERR_NULL_PTR;

    if (list->begin == NULL)
    {
        list->begin = list->end = new_token;
    }
    else
    {
        list->end->next = new_token;
        new_token->prev = list->end;
        list->end = new_token;
    }
    list->size += 1;
    return 0;
}

int token_list_append_token_with_configure(struct token_list *list,\
       int value, char *str, size_t len, uint32_t pos_col, uint32_t pos_ln)
{
    int ret = 0;
    struct token *new_token = token_new_with_configure(value, str, len, pos_col, pos_ln);

    if ((list == NULL) || (new_token == NULL)) return -MULTIPLE_ERR_NULL_PTR;

    if ((ret = token_list_append(list, new_token)) != 0)
    {
        token_destroy(new_token);
        return ret;
    }
    return 0;
}

int token_list_append_token_with_template(struct token_list *list, struct token *new_token)
{
    return token_list_append_token_with_configure(list,
            new_token->value, 
            new_token->str, 
            new_token->len, 
            new_token->pos_col, 
            new_token->pos_ln);
}

int token_list_walk(struct token_list *list)
{
    struct token *token_cur, *token_next;

    if (list == NULL) return -MULTIPLE_ERR_NULL_PTR;

    token_cur = list->begin;
    while (token_cur != NULL)
    {
        token_next = token_cur->next;
        token_print(token_cur);
        token_cur = token_next;
    }

    return 0;
}

/* Create a new generic lexical scanner */
struct generic_lexer *generic_lexer_new(void)
{
    struct generic_lexer *new_lexer = NULL;

    if ((new_lexer = (struct generic_lexer *)malloc(sizeof(struct generic_lexer))) == NULL)
    { return NULL; }
    memset(new_lexer, 0, sizeof(struct generic_lexer));

    return new_lexer;
}

/* Create a new generic lexical scanner with default settings */
struct generic_lexer *generic_lexer_new_with_default_setting(void)
{
    struct generic_lexer *new_lexer = NULL;

    if ((new_lexer = generic_lexer_new()) == NULL)
    {
        return NULL;
    }

    /* Whitespaces */
    new_lexer->enabled_whitespace = 1;

    /* Comments */
    new_lexer->enabled_single_line_comment = 1;
    strncpy(new_lexer->literal_single_line_comment, "#", GENERIC_LEXER_LITERAL_LEN_MAX);
    new_lexer->literal_single_line_comment_len = 1;

    /* Identifier */
    new_lexer->enabled_identifier = 1;

    /* Integer */
    new_lexer->enabled_integer = 1;

    /* String */
    new_lexer->enabled_str = 1;
    strncpy(new_lexer->literal_str_symbol_start, "\"", GENERIC_LEXER_LITERAL_LEN_MAX);
    new_lexer->literal_str_symbol_start_len = 1;
    strncpy(new_lexer->literal_str_symbol_end, "\"", GENERIC_LEXER_LITERAL_LEN_MAX);
    new_lexer->literal_str_symbol_end_len = 1;

    new_lexer->optrs = NULL;
    new_lexer->optrs_count = 0;
    new_lexer->keywords = NULL;
    new_lexer->keywords_count = 0;
    new_lexer->data_types = NULL;
    new_lexer->data_types_count = 0;
    new_lexer->constants = NULL;
    new_lexer->constants_count = 0;

    return new_lexer;
}

/* Destroy a generic lexical scanner */
int generic_lexer_destroy(struct multiple_error *err, struct generic_lexer *lexer)
{
    if (lexer == NULL)
    {
        MULTIPLE_ERROR_NULL_PTR();
        return -MULTIPLE_ERR_NULL_PTR;
    }

    free(lexer);
    
    return 0;
}

/* Status definitions for lexical analysis */
enum {
    LEX_STATUS_INIT = 0,

    LEX_STATUS_COMMENT, /* #.*?<EOL> */
    LEX_STATUS_EOL, /* EOL of Windows? Mac? */
    LEX_STATUS_IDENTIFIER_P_1, /* Identifier ? */

    LEX_STATUS_INTEGER_BOH, /* 0[b|[0-9]|h] */
    LEX_STATUS_INTEGER_BOH_B, /* 0b */
    LEX_STATUS_INTEGER_BOH_B1, /* 0b[01] */
    LEX_STATUS_INTEGER_BOH_O, /* 0[0-9] */
    LEX_STATUS_INTEGER_BOH_H, /* 0x */
    LEX_STATUS_INTEGER_BOH_H1, /* 0x[01] */
    LEX_STATUS_INTEGER_D, /* 0x */

    LEX_STATUS_STRING, /* " */

    LEX_STATUS_BACK_FINISH, /* Finished, and break */
    LEX_STATUS_FINISH, /* Finished */
    LEX_STATUS_ERROR, /* Error */
};

#define JMP(status,dst) do{(status)=(dst);}while(0);
#define FIN(x) do{(x)=LEX_STATUS_FINISH;}while(0);
#define BFIN(x) do{(x)=LEX_STATUS_BACK_FINISH;}while(0);
#define UND(x) do{(x)=LEX_STATUS_ERROR;}while(0);
#define KEEP() do{}while(0);

#define ENOUGH_SPACE(p, endp, delta) ((((size_t)((endp)-(p)))>=((size_t)delta))?(1):(0))
#define MATCH(symbol,symbol_len,p,endp) ((ENOUGH_SPACE(p,endp,symbol_len)!=0)&&(memcmp(symbol,p,symbol_len)==0))

/* Get one token from the char stream */
static int eat_token(struct multiple_error *err, struct token *new_token, const char *p, const char *endp, uint32_t *pos_col, uint32_t *pos_ln, const int eol_type, struct generic_lexer *lexer)
{
    const char *p_init = p;
    int status = LEX_STATUS_INIT;
    int ch = 0;
    size_t idx = 0;
    size_t bytes_number;

    int is_eol = 0; /* For update eol and ln */

    /* Clean template */
    new_token->value = TOKEN_UNDEFINED;
    new_token->str = (char *)p_init;
    new_token->len = 0;
    new_token->pos_col = *pos_col;
    new_token->pos_ln = *pos_ln;

    while (p != endp)
    {
        ch = *p;
        switch (status)
        {
            case LEX_STATUS_EOL:
                if (ch == CHAR_LF) { FIN(status); } else { BFIN(status); }
                break;
            case LEX_STATUS_COMMENT:
                if (IS_EOL(ch)) 
                {
                    /* Reset location */
                    *pos_col = 1;
                    *pos_ln += 1;
                    is_eol = 1;
                    /* "" (Null String) */
                    new_token->value = TOKEN_WHITESPACE;
                    FIN(status);
                }
                else
                {
                    KEEP();
                }
                break;
            case LEX_STATUS_INIT:
                if (IS_EOL(ch)) 
                {
                    /* Reset location */
                    *pos_col = 1;
                    *pos_ln += 1;
                    is_eol = 1;

                    new_token->value = TOKEN_WHITESPACE; 
                    switch (eol_type)
                    {
                        case EOL_UNIX:
                        case EOL_MAC:
                            FIN(status);
                            break;
                        case EOL_DOS:
                            JMP(status, LEX_STATUS_EOL);
                            break;
                    }
                }
                /* Comments */
                else if ((lexer->enabled_single_line_comment != 0) && \
                        (MATCH(lexer->literal_single_line_comment, lexer->literal_single_line_comment_len,p,endp)))
                {
                    JMP(status, LEX_STATUS_COMMENT);
                    goto finish_init;
                }
                /* Whitespace */
                if ((lexer->enabled_whitespace != 0) && IS_WHITESPACE(ch))
                {
                    new_token->value = TOKEN_WHITESPACE; FIN(status);
                    goto finish_init;
                }

                /* Operators */
                for (idx = 0; idx != lexer->optrs_count; idx++)
                {
                    if (MATCH(lexer->optrs[idx].literal,lexer->optrs[idx].len,p,endp) != 0)
                    {
                        new_token->value = lexer->optrs[idx].value; 
                        FIN(status);
                        p += (lexer->optrs[idx].len - 1);
                        goto finish_init;
                    }
                }

                /* Identifiers */
                if ((lexer->enabled_identifier != 0) && (IS_ID(ch)))
                {
                    /* Identifier ? */
                    new_token->value = TOKEN_IDENTIFIER;
                    JMP(status, LEX_STATUS_IDENTIFIER_P_1);
                    goto finish_init;
                }

                if (IS_ID_HYPER(ch)) 
                {
                    bytes_number = id_hyper_length((char)ch);
                    if ((bytes_number == 0) || ((size_t)(endp - p) < bytes_number))
                    {
                        MULTIPLE_ERROR_INTERNAL();
                        return -MULTIPLE_ERR_LEXICAL;
                    }
                    bytes_number--;
                    while (bytes_number-- != 0) { p += 1; }
                    /* Identifier ? */
                    new_token->value = TOKEN_IDENTIFIER;
                    JMP(status, LEX_STATUS_IDENTIFIER_P_1);
                    goto finish_init;
                }

                /* Integer */
                if (lexer->enabled_integer != 0)
                {
                    if (ch == '0')
                    {
                        /* 0x???? -> Hex */
                        /* 0b???? -> Bin */
                        /* 0???? -> Oct */
                        new_token->value = TOKEN_CONSTANT_INTEGER_DECIMAL;
                        JMP(status, LEX_STATUS_INTEGER_BOH);
                        goto finish_init;
                    }
                    else if (('1' <= ch) && (ch <= '9'))
                    {
                        new_token->value = TOKEN_CONSTANT_INTEGER_DECIMAL;
                        JMP(status, LEX_STATUS_INTEGER_D);
                        goto finish_init;
                    }
                }

                /* String */
                if ((lexer->enabled_str != 0) && 
                        (MATCH(lexer->literal_str_symbol_start,lexer->literal_str_symbol_start_len,p,endp)))
                {
                    JMP(status, LEX_STATUS_STRING);
                    goto finish_init;
                }

                /* Undefined */
                {new_token->value = TOKEN_UNDEFINED; UND(status);} /* Undefined! */
finish_init:
                break;
            case LEX_STATUS_IDENTIFIER_P_1:
                if (IS_ID(ch)||IS_INTEGER_DECIMAL(ch)) {KEEP();}
                else if (IS_ID_HYPER(ch)) 
                {
                    bytes_number = id_hyper_length((char)ch);
                    if ((bytes_number == 0) || ((size_t)(endp - p) < bytes_number))
                    {
                        MULTIPLE_ERROR_INTERNAL();
                        return -MULTIPLE_ERR_LEXICAL;
                    }
                    bytes_number--;
                    while (bytes_number-- != 0) { p += 1; }
                }
                else {new_token->value = TOKEN_IDENTIFIER; BFIN(status);} /* Identifier! */
                break;
            case LEX_STATUS_INTEGER_BOH: 
                /* 0<- */
                if ((ch == 'b')||(ch == 'B')) {JMP(status, LEX_STATUS_INTEGER_BOH_B);}
                else if (IS_INTEGER_DECIMAL(ch)) 
                {
                    new_token->value = TOKEN_CONSTANT_INTEGER_OCTAL;
                    JMP(status, LEX_STATUS_INTEGER_BOH_O);
                }
                else if ((ch == 'x')||(ch == 'X')) {JMP(status, LEX_STATUS_INTEGER_BOH_H);}
                else {BFIN(status);} /* Decimal 0 */
                break;
            case LEX_STATUS_INTEGER_BOH_B:
                if (IS_INTEGER_BINARY(ch)) 
                {
                    new_token->value = TOKEN_CONSTANT_INTEGER_BINARY;
                    JMP(status, LEX_STATUS_INTEGER_BOH_B1);
                }
                else 
                {
                    /* 0b2 */
                    p -= 1;
                    BFIN(status);
                }
                break;
            case LEX_STATUS_INTEGER_BOH_B1:
                if (IS_INTEGER_BINARY(ch)) {KEEP();}
                else {BFIN(status);} /* Binary Integer! */
                break;
            case LEX_STATUS_INTEGER_BOH_O:
                if (IS_INTEGER_OCTAL(ch)) {KEEP();}
                else {BFIN(status);} /* Octal Integer! */
                break;
            case LEX_STATUS_INTEGER_BOH_H:
                if (IS_INTEGER_HEXADECIMAL(ch)) 
                {
                    new_token->value = TOKEN_CONSTANT_INTEGER_HEXADECIMAL;
                    JMP(status, LEX_STATUS_INTEGER_BOH_H1);
                }
                else 
                {
                    /* 0xq */
                    p -= 1;
                    BFIN(status);
                }
                break;
            case LEX_STATUS_INTEGER_BOH_H1:
                if (IS_INTEGER_HEXADECIMAL(ch)){KEEP();}
                else {BFIN(status);} /* Identifier! */
                break;
            case LEX_STATUS_INTEGER_D:
                if (IS_INTEGER_DECIMAL(ch)){KEEP();}
                else {BFIN(status);} /* Identifier! */
                break;
            case LEX_STATUS_STRING:
                /* "<- */
                if (MATCH(lexer->literal_str_symbol_end,lexer->literal_str_symbol_end_len,p,endp) != 0)
                {
                    /* "" (Null String) */
                    new_token->value = TOKEN_CONSTANT_STRING;
                    FIN(status);
                }
                else
                {
                    KEEP();
                }
                break;
            case LEX_STATUS_ERROR:
                new_token->str = NULL;
                new_token->len = 0;
                multiple_error_update(err, -MULTIPLE_ERR_LEXICAL, "%d:%d: error: undefined token", *pos_ln, *pos_col);
                return -MULTIPLE_ERR_LEXICAL;
                break;
            case LEX_STATUS_BACK_FINISH:
                p--;
            case LEX_STATUS_FINISH:
                goto done;
                break;
            default:
                new_token->str = NULL;
                new_token->len = 0;
                multiple_error_update(err, -MULTIPLE_ERR_LEXICAL, "%d:%d: error: undefined lexical analysis state, something impossible happened", *pos_ln, *pos_col);
                return -MULTIPLE_ERR_LEXICAL;
                break;
        }
        if (status == LEX_STATUS_BACK_FINISH) break;
        p += 1;
    }
    if (status == LEX_STATUS_INTEGER_BOH_B || status == LEX_STATUS_INTEGER_BOH_H)
    {
        /* 0b$ and 0x$ */
        p -= 1;
    }
done:
    if (!is_eol)
    {
        *pos_col += (uint32_t)(p - p_init);
    }
    if (new_token->value == TOKEN_UNDEFINED)
    {
        new_token->len = 0;
    }
    else
    {
        new_token->len = (size_t)(p - p_init);
        if (new_token->value == TOKEN_IDENTIFIER)
        {
            /* Keywords */
            for (idx = 0; idx != lexer->keywords_count; idx++)
            {
                if ((lexer->keywords[idx].len == new_token->len) && \
                        (strncmp(lexer->keywords[idx].literal, new_token->str, lexer->keywords[idx].len) == 0))
                {
                    new_token->value = lexer->keywords[idx].value; 
                    FIN(status);
                    goto finish_patch;
                }
            }

            /* Data Types */
            for (idx = 0; idx != lexer->data_types_count; idx++)
            {
                if ((lexer->keywords[idx].len == new_token->len) && \
                        (strncmp(lexer->keywords[idx].literal, new_token->str, lexer->keywords[idx].len) == 0))
                {
                    new_token->value = lexer->data_types[idx].value; 
                    FIN(status);
                    goto finish_patch;
                }
            }

            /* Constants */
            for (idx = 0; idx != lexer->constants_count; idx++)
            {
                if ((lexer->keywords[idx].len == new_token->len) && \
                        (strncmp(lexer->keywords[idx].literal, new_token->str, lexer->keywords[idx].len) == 0))
                {
                    new_token->value = lexer->constants[idx].value; 
                    FIN(status);
                    goto finish_patch;
                }
            }
        }
    }
finish_patch:
    return 0;
}

/* Tokenize source code with a lexeme scanner */
int generic_tokenize(struct multiple_error *err, struct token_list **list_out, const char *data, const size_t data_len, struct generic_lexer *lexer)
{
    int ret = 0;
    uint32_t pos_col = 1, pos_ln = 1;
    struct token_list *new_list = NULL;
    struct token *token_template = NULL;
    const char *data_p = data, *data_endp = data_p + data_len;
    int eol_type;

    eol_type = eol_detect(err, data, data_len);
    if (eol_type < 0)
    {
        goto fail;
    }

    *list_out = NULL;

    if ((new_list = token_list_new()) == NULL)
    {
        MULTIPLE_ERROR_MALLOC();
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }

    if ((token_template = token_new()) == NULL)
    {
        MULTIPLE_ERROR_MALLOC();
        ret = -MULTIPLE_ERR_MALLOC;
        goto fail;
    }

    while (data_p != data_endp)
    {
        if ((ret = eat_token(err, token_template, data_p, data_endp, &pos_col, &pos_ln, eol_type, lexer)) != 0)
        {
            goto fail;
        }
        if (token_template->value != TOKEN_WHITESPACE)
        {
            if ((ret = token_list_append_token_with_template(new_list, token_template)) != 0)
            {
                goto fail;
            }
        }
        /* Move on */
        data_p += token_template->len;
    }
    ret = token_list_append_token_with_configure(new_list, TOKEN_FINISH, NULL, 0, pos_col, pos_ln);
    if (ret != 0) goto fail;

    *list_out = new_list;
    ret = 0;
fail:
    if (token_template != NULL)
	{
		token_template->str = NULL;
		free(token_template);
	}
    if (ret != 0)
    {
        if (new_list != NULL) token_list_destroy(new_list);
    }
    return ret;
}

struct token_value_name_tbl_item
{
    const int value;
    const char *name;
};

static struct token_value_name_tbl_item token_value_name_tbl_items[] = 
{
    { TOKEN_FINISH, "generic.finish" },
    { TOKEN_UNDEFINED, "generic.undefined" },
    { TOKEN_WHITESPACE, "generic.whitespace" },
    { TOKEN_IDENTIFIER, "generic.identifier" },
    { TOKEN_CONSTANT_INTEGER_BINARY, "generic.constant.integer.binary" },
    { TOKEN_CONSTANT_INTEGER_OCTAL, "generic.constant.integer.octal" },
    { TOKEN_CONSTANT_INTEGER_DECIMAL, "generic.constant.integer.decimal" },
    { TOKEN_CONSTANT_INTEGER_HEXADECIMAL, "generic.constant.integer.hexadecimal" },
    { TOKEN_CONSTANT_FLOAT_BINARY, "generic.constant.float.binary" },
    { TOKEN_CONSTANT_FLOAT_OCTAL, "generic.constant.float.octal" },
    { TOKEN_CONSTANT_FLOAT_DECIMAL, "generic.constant.float.decimal" },
    { TOKEN_CONSTANT_FLOAT_HEXADECIMAL, "generic.constant.float.hexadecimal" },
    { TOKEN_CONSTANT_STRING, "generic.constant.string" },
    { TOKEN_CONSTANT_NONE, "generic.constant.none" },
    { TOKEN_CONSTANT_FALSE, "generic.constant.false" },
    { TOKEN_CONSTANT_TRUE, "generic.constant.true"}
};
#define TOKEN_VALUE_NAME_TBL_ITEMS_COUNT (sizeof(token_value_name_tbl_items)/sizeof(struct token_value_name_tbl_item))

/* Get token name */
int generic_token_name(char **token_name, size_t *token_name_len, const int value)
{
    int i;
    for (i = 0; i != TOKEN_VALUE_NAME_TBL_ITEMS_COUNT; i++)
    {
        if (value == token_value_name_tbl_items[i].value)
        {
            *token_name = (char *)token_value_name_tbl_items[i].name;
            *token_name_len = (size_t)strlen(token_value_name_tbl_items[i].name);
            return 0;
        }
    }
    return -1;
}

size_t id_hyper_length(char ch)
{
    size_t bytes_number;
    if ((ch & 0x80) == 0) bytes_number = 1; /* 0xxxxxxx */
    else if ((ch & 0xe0) == 0xc0) bytes_number = 2; /* 110xxxxx, 10xxxxxx */
    else if ((ch & 0xf0) == 0xe0) bytes_number = 3; /* 1110xxxx, 10xxxxxx, 10xxxxxx */
    else if ((ch & 0xf8) == 0xf0) bytes_number = 4; /* 11110xxx, 10xxxxxx, 10xxxxxx, 10xxxxxx */
    else if ((ch & 0xfc) == 0xf8) bytes_number = 5; /* 111110xx, 10xxxxxx, 10xxxxxx, 10xxxxxx, 10xxxxxx */
    else if ((ch & 0xfe) == 0xfc) bytes_number = 6; /* 1111110x, 10xxxxxx, 10xxxxxx, 10xxxxxx, 10xxxxxx, 10xxxxxx */
    else bytes_number = 0;
    return bytes_number;
}

