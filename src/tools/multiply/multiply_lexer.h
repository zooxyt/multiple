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

#ifndef _MULTIPLY_LEXER_H_
#define _MULTIPLY_LEXER_H_

#include <stdio.h>
#include <stdint.h>

#include "multiple_err.h"

/* ASSISTANT FUNCTIONS */

#define IS_EOL(x) (((x)=='\r')||((x)=='\n'))
#define IS_WHITESPACE(x) (((x)=='\t')||((x)=='\r')||((x)=='\n')||((x)==' '))
#define IS_ALPHA(x) ((('a'<=(x))&&((x)<='z'))|(('A'<=(x))&&((x)<='Z')))
#define IS_ID(x) (((x)=='_')||(IS_ALPHA(x)))
#define IS_ID_HYPER(x) (((x)&128)!=0?1:0)
#define IS_INTEGER_BINARY(x) (((x)=='0')||((x)=='1'))
#define IS_INTEGER_OCTAL(x) (('0'<=(x))&&((x)<='7'))
#define IS_INTEGER_DECIMAL(x) (('0'<=(x))&&((x)<='9'))
#define IS_INTEGER_HEXADECIMAL(x) (('0'<=(x))&&((x)<='9'))||(('a'<=(x))&&((x)<='f'))||(('A'<=(x))&&((x)<='F'))

#define CHAR_CR 13
#define CHAR_LF 10

#define EOL_UNKNOWN 0 /* unknown */
#define EOL_UNIX 1 /* <LF> */
#define EOL_DOS 2 /* <CR><LF> */
#define EOL_MAC 3 /* <CR> */

/* Detect End Of Line Type */
int eol_detect(struct multiple_error *err, const char *text, const size_t len);


/* Token */

struct token
{
    /* type of token */
    int value;

    /* literal value of token */
    char *str;
    size_t len;

    /* position in original file */
    uint32_t pos_col;
    uint32_t pos_ln;

    /* next */
    struct token *next;
    struct token *prev;
};

/* Create a token */
struct token *token_new(void);
/* Create a token and initialize with values */
struct token *token_new_with_configure(\
        int value, char *str, size_t len, uint32_t num_col, uint32_t num_ln);
/* Destroy a token */
int token_destroy(struct token *token);
/* Clone a token */
struct token *token_clone(struct token *token);
/* Print the detail of token */
int token_print(struct token *token);


/* Token List */

struct token_list
{
    struct token *begin;
    struct token *end;
    size_t size;
};

/* Append a token to a token list */
int token_list_append(struct token_list *list, struct token *new_token);
/* Append a token to a token list with the configure of an exist token */
int token_list_append_token_with_template(struct token_list *list, struct token *new_token);
/* Append a token to a token list with the specified configure */
int token_list_append_token_with_configure(struct token_list *list,\
       int value, char *str, size_t len, uint32_t num_col, uint32_t num_ln);
/* Show content of token list */
int token_list_walk(struct token_list *list);


/* Create a token list */
struct token_list *token_list_new(void);
/* Destroy a token list */
int token_list_destroy(struct token_list *list);

#define CUSTOM_TOKEN_STARTPOINT (256)

/* Preset tokens */
enum
{
    /* Special */
    TOKEN_FINISH = 1001, /* Append at the end of every stream
                            to mark the parsing finish */

    TOKEN_UNDEFINED = 1002,
    TOKEN_WHITESPACE = 1003, /* [\ \t\r\n] */

    /* Constants */
    TOKEN_IDENTIFIER = 2000, /* [a-zA-Z][a-zA-Z0-9]+ */
    TOKEN_CONSTANT_INTEGER_BINARY, /* 0b[01]+ */
    TOKEN_CONSTANT_INTEGER_DECIMAL, /* [1-9][0-9]+ */
    TOKEN_CONSTANT_INTEGER_OCTAL, /* 0[0-7]+ */
    TOKEN_CONSTANT_INTEGER_HEXADECIMAL, /* 0x[0-9a-fA-Z]+ */
    TOKEN_CONSTANT_FLOAT_BINARY, /* 0b[01]+.([01])?+ */
    TOKEN_CONSTANT_FLOAT_DECIMAL, /* [1-9][0-9]+.([0-9])?+ */
    TOKEN_CONSTANT_FLOAT_OCTAL, /* 0[0-7]+.([0-7])?+ */
    TOKEN_CONSTANT_FLOAT_HEXADECIMAL, /* 0x[0-9a-fA-F]+.([0-9a-fA-F])?+ */
    TOKEN_CONSTANT_STRING, /* ".*?" */
    TOKEN_CONSTANT_CHARACTER, /* '.*?" */
    TOKEN_CONSTANT_NONE,
    TOKEN_CONSTANT_FALSE,
    TOKEN_CONSTANT_TRUE,
};

#define IS_TOKEN_IDENTIFIER(x)\
    ((x)==(TOKEN_IDENTIFIER))

#define IS_TOKEN_CONSTANT(x)\
    (((x)==(TOKEN_CONSTANT_INTEGER_BINARY))||\
     ((x)==(TOKEN_CONSTANT_INTEGER_OCTAL))||\
     ((x)==(TOKEN_CONSTANT_INTEGER_DECIMAL))||\
     ((x)==(TOKEN_CONSTANT_INTEGER_HEXADECIMAL))||\
     ((x)==(TOKEN_CONSTANT_FLOAT_BINARY))||\
     ((x)==(TOKEN_CONSTANT_FLOAT_OCTAL))||\
     ((x)==(TOKEN_CONSTANT_FLOAT_DECIMAL))||\
     ((x)==(TOKEN_CONSTANT_FLOAT_HEXADECIMAL))||\
     ((x)==(TOKEN_CONSTANT_FALSE))||\
     ((x)==(TOKEN_CONSTANT_TRUE))||\
     ((x)==(TOKEN_CONSTANT_NONE))||\
     ((x)==(TOKEN_CONSTANT_STRING)))

#define IS_TOKEN_CONSTANT_INTEGER(x) \
    (((x)==(TOKEN_CONSTANT_INTEGER_BINARY))||\
     ((x)==(TOKEN_CONSTANT_INTEGER_OCTAL))||\
     ((x)==(TOKEN_CONSTANT_INTEGER_DECIMAL))||\
     ((x)==(TOKEN_CONSTANT_INTEGER_HEXADECIMAL)))

#define GENERIC_LEXER_LITERAL_LEN_MAX (16)

struct lexical_item
{
    int value;
    char literal[GENERIC_LEXER_LITERAL_LEN_MAX];
    size_t len;
};

struct generic_lexer
{
    /* Whitespace */
    int enabled_whitespace;

    /* Comment */
    int enabled_single_line_comment;
    char literal_single_line_comment[GENERIC_LEXER_LITERAL_LEN_MAX];
    size_t literal_single_line_comment_len;

    /* Identifier 
     * INCLUDING Keywords, Data Types, Constants */
    int enabled_identifier;

    /* C & C++ Style Integer */
    int enabled_integer;

    /* String */
    int enabled_str;
    char literal_str_symbol_start[GENERIC_LEXER_LITERAL_LEN_MAX]; /* " */
    size_t literal_str_symbol_start_len;
    char literal_str_symbol_end[GENERIC_LEXER_LITERAL_LEN_MAX]; /* " */
    size_t literal_str_symbol_end_len;

    /* Operators */
    struct lexical_item *optrs;
    size_t optrs_count;

    /* Keywords */
    struct lexical_item *keywords;
    size_t keywords_count;

    /* Data Types */
    struct lexical_item *data_types;
    size_t data_types_count;

    /* Constants */
    struct lexical_item *constants;
    size_t constants_count;
};

/* Create a new generic Lexical scanner */
struct generic_lexer *generic_lexer_new(void);

/* Create a new generic Lexical scanner with default settings */
struct generic_lexer *generic_lexer_new_with_default_setting(void);

/* Destroy a generic Lexical scanner */
int generic_lexer_destroy(struct multiple_error *err, struct generic_lexer *lexer);

/* Get token name */
int generic_token_name(char **token_name, size_t *token_name_len, const int value);

/* Do Lexical scanning on source code  */
int generic_tokenize(struct multiple_error *err, struct token_list **list_out, const char *data, const size_t data_len, struct generic_lexer *lexer);

size_t id_hyper_length(char ch);

#endif

