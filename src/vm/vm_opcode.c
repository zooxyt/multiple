/* Opcode
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

#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "multiple_err.h"

#include "vm_opcode.h"

struct virtual_machine_opcode_item
{
    const uint32_t opcode;
    const char *name;
    const int oprand_type;
    const int stack_require;
};

static struct virtual_machine_opcode_item virtual_machine_opcode_item[] = 
{
    {OP_NOP        , "nop"        , OPERAND_TYPE_NIL, 0} ,
    {OP_HALT       , "halt"       , OPERAND_TYPE_NIL, 0} ,
    {OP_FASTLIB    , "fastlib"    , OPERAND_TYPE_NUM, 0} ,
    {OP_DEF        , "def"        , OPERAND_TYPE_RES, 0} ,
    {OP_ARGP       , "argp"       , OPERAND_TYPE_NIL, 0} ,
    {OP_ARGCS      , "argcs"      , OPERAND_TYPE_NIL, 0} ,
    {OP_LSTARGCS   , "lstargcs"   , OPERAND_TYPE_NIL, 0} ,
    {OP_LSTRARGCS  , "lstrargcs"  , OPERAND_TYPE_NIL, 0} ,
    {OP_ARG        , "arg"        , OPERAND_TYPE_RES, 0} ,
    {OP_LSTARG     , "lstarg"     , OPERAND_TYPE_RES, 0} ,
    {OP_LSTRARG    , "lstrarg"    , OPERAND_TYPE_RES, 0} ,
    {OP_HASHARG    , "hasharg"    , OPERAND_TYPE_RES, 0} ,
    {OP_ARGC       , "argc"       , OPERAND_TYPE_RES, 0} ,
    {OP_ARGCL      , "argcl"      , OPERAND_TYPE_RES, 0} ,
    {OP_LSTARGC    , "lstargc"    , OPERAND_TYPE_RES, 0} ,
    {OP_LSTRARGC   , "lstrargc"   , OPERAND_TYPE_RES, 0} ,
    {OP_HASHARGC   , "hashargc"   , OPERAND_TYPE_RES, 0} ,
    {OP_PUSH       , "push"       , OPERAND_TYPE_RES, 0} ,
    {OP_PUSHC      , "pushc"      , OPERAND_TYPE_RES, 0} ,
    {OP_VARP       , "varp"       , OPERAND_TYPE_RES, 1} ,
    {OP_VARCP      , "varcp"      , OPERAND_TYPE_RES, 1} ,
    {OP_VARCLP     , "varclp"     , OPERAND_TYPE_RES, 1} ,
    {OP_POP        , "pop"        , OPERAND_TYPE_RES, 1} ,
    {OP_POPC       , "popc"       , OPERAND_TYPE_RES, 1} ,
    {OP_POPCL      , "popcl"      , OPERAND_TYPE_RES, 1} ,
    {OP_POPCX      , "popcx"      , OPERAND_TYPE_RES, 1} ,
    {OP_PUSHG      , "pushg"      , OPERAND_TYPE_RES, 0} ,
    {OP_POPG       , "popg"       , OPERAND_TYPE_RES, 1} ,
    {OP_PUSHM      , "pushm"      , OPERAND_TYPE_RES, 0} ,
    {OP_POPM       , "popm"       , OPERAND_TYPE_RES, 1} ,
    {OP_MSCALL     , "mscall"     , OPERAND_TYPE_NIL, 0} ,
    {OP_MSRETURN   , "msreturn"   , OPERAND_TYPE_NIL, 0} ,
    {OP_MSPUSH     , "mspush"     , OPERAND_TYPE_RES, 1} ,
    {OP_MSPOP      , "mspop"      , OPERAND_TYPE_RES, 1} ,
    {OP_MSREM      , "msrem"      , OPERAND_TYPE_NUM, 0} ,
    {OP_INT        , "int"        , OPERAND_TYPE_RES, 0} ,
    {OP_IEGC       , "iegc"       , OPERAND_TYPE_NIL, 0} ,
    {OP_IDGC       , "idgc"       , OPERAND_TYPE_NIL, 0} ,
    {OP_TRAPSET    , "trapset"    , OPERAND_TYPE_NUM, 0} ,
    {OP_TRAP       , "trap"       , OPERAND_TYPE_NIL, 0} ,
    {OP_PRINT      , "print"      , OPERAND_TYPE_NIL, 1} ,
    {OP_RETURNTO   , "returnto"   , OPERAND_TYPE_NUM, 0} ,
    {OP_RETURN     , "return"     , OPERAND_TYPE_NIL, 1} ,
    {OP_RETNONE    , "retnone"    , OPERAND_TYPE_NIL, 0} ,
    {OP_YIELD      , "yield"      , OPERAND_TYPE_NIL, 1} ,
    {OP_DOMAIN     , "domain"     , OPERAND_TYPE_NIL, 2} ,
    {OP_CALL       , "call"       , OPERAND_TYPE_NIL, 1} ,
    {OP_TAILCALL   , "tailcall"   , OPERAND_TYPE_NIL, 1} ,
    {OP_CALLC      , "callc"      , OPERAND_TYPE_NIL, 1} ,
    {OP_TAILCALLC  , "tailcallc"  , OPERAND_TYPE_NIL, 1} ,
    {OP_DLCALL     , "dlcall"     , OPERAND_TYPE_NIL, 2} ,
    {OP_DUP        , "dup"        , OPERAND_TYPE_NIL, 1} ,
    {OP_DROP       , "drop"       , OPERAND_TYPE_NIL, 1} ,
    {OP_PICK       , "pick"       , OPERAND_TYPE_NIL, 0} ,
    {OP_PICKCP     , "pickcp"     , OPERAND_TYPE_NIL, 0} ,
    {OP_INSERT     , "insert"     , OPERAND_TYPE_NIL, 0} ,
    {OP_INSERTCP   , "insertcp"   , OPERAND_TYPE_NIL, 0} ,
    {OP_REVERSE    , "reverse"    , OPERAND_TYPE_NIL, 0} ,
    {OP_REVERSEP   , "reversep"   , OPERAND_TYPE_NIL, 0} ,
    {OP_LIFT       , "lift"       , OPERAND_TYPE_NUM, 0} ,
    {OP_JMP        , "jmp"        , OPERAND_TYPE_NUM, 0} ,
    {OP_JMPC       , "jmpc"       , OPERAND_TYPE_NUM, 1} ,
    {OP_JMPR       , "jmpr"       , OPERAND_TYPE_NUM, 0} ,
    {OP_JMPCR      , "jmpcr"      , OPERAND_TYPE_NUM, 1} ,
    {OP_TCUR       , "tcur"       , OPERAND_TYPE_NIL, 0} ,
    {OP_TFK        , "tfk"        , OPERAND_TYPE_NIL, 0} ,
    {OP_TWAIT      , "twait"      , OPERAND_TYPE_NIL, 1} ,
    {OP_TEXIT      , "texit"      , OPERAND_TYPE_NIL, 0} ,
    {OP_TALIVE     , "talive"     , OPERAND_TYPE_NIL, 1} ,
    {OP_TYIELD     , "tyield"     , OPERAND_TYPE_NIL, 0} ,
    {OP_TISEMPTY   , "tisempty"   , OPERAND_TYPE_NIL, 0} ,
    {OP_TSENDMSG   , "tsendmsg"   , OPERAND_TYPE_NIL, 0} ,
    {OP_TRECVMSG   , "trecvmsg"   , OPERAND_TYPE_NIL, 0} ,
    {OP_TSUSPEND   , "tsuspend"   , OPERAND_TYPE_NIL, 0} ,
    {OP_TRESUME    , "tresume"    , OPERAND_TYPE_NIL, 0} ,
    {OP_MTXMK      , "mtxmk"      , OPERAND_TYPE_NIL, 0} ,
    {OP_MTXLCK     , "mtxlck"     , OPERAND_TYPE_NIL, 0} ,
    {OP_MTXUNLCK   , "mtxunlck"   , OPERAND_TYPE_NIL, 0} ,
    {OP_SEMMK      , "semmk"      , OPERAND_TYPE_NIL, 0} ,
    {OP_SEMP       , "semp"       , OPERAND_TYPE_NIL, 0} ,
    {OP_SEMV       , "semv"       , OPERAND_TYPE_NIL, 0} ,
    {OP_ADD        , "add"        , OPERAND_TYPE_NIL, 2} ,
    {OP_SUB        , "sub"        , OPERAND_TYPE_NIL, 2} ,
    {OP_MUL        , "mul"        , OPERAND_TYPE_NIL, 2} ,
    {OP_DIV        , "div"        , OPERAND_TYPE_NIL, 2} ,
    {OP_MOD        , "mod"        , OPERAND_TYPE_NIL, 2} ,
    {OP_LSHIFT     , "lshift"     , OPERAND_TYPE_NIL, 2} ,
    {OP_RSHIFT     , "rshift"     , OPERAND_TYPE_NIL, 2} ,
    {OP_ANDA       , "anda"       , OPERAND_TYPE_NIL, 2} ,
    {OP_ORA        , "ora"        , OPERAND_TYPE_NIL, 2} ,
    {OP_XORA       , "xora"       , OPERAND_TYPE_NIL, 2} ,
    {OP_ANDL       , "andl"       , OPERAND_TYPE_NIL, 2} ,
    {OP_ORL        , "orl"        , OPERAND_TYPE_NIL, 2} ,
    {OP_XORL       , "xorl"       , OPERAND_TYPE_NIL, 2} ,
    {OP_EQ         , "eq"         , OPERAND_TYPE_NIL, 2} ,
    {OP_NE         , "ne"         , OPERAND_TYPE_NIL, 2} ,
    {OP_L          , "l"          , OPERAND_TYPE_NIL, 2} ,
    {OP_G          , "g"          , OPERAND_TYPE_NIL, 2} ,
    {OP_LE         , "le"         , OPERAND_TYPE_NIL, 2} ,
    {OP_GE         , "ge"         , OPERAND_TYPE_NIL, 2} ,
    {OP_NEG        , "neg"        , OPERAND_TYPE_NIL, 1} ,
    {OP_NOTA       , "nota"       , OPERAND_TYPE_NIL, 1} ,
    {OP_NOTL       , "notl"       , OPERAND_TYPE_NIL, 1} ,
    {OP_CONVERT    , "convert"    , OPERAND_TYPE_NUM, 1} ,
    {OP_SIZE       , "size"       , OPERAND_TYPE_NIL, 1} ,
    {OP_TYPE       , "type"       , OPERAND_TYPE_NIL, 1} ,
    {OP_TYPEP      , "typep"      , OPERAND_TYPE_NUM, 1} ,
    {OP_TYPEUP     , "typeup"     , OPERAND_TYPE_NIL, 2} ,
    {OP_SLV        , "slv"        , OPERAND_TYPE_NIL, 1} ,
    {OP_TRYSLV     , "tryslv"     , OPERAND_TYPE_NIL, 1} ,
    {OP_STI        , "sti"        , OPERAND_TYPE_NIL, 1} ,
    {OP_ITS        , "its"        , OPERAND_TYPE_NIL, 1} ,
    {OP_CLSTYPEREG , "clstypereg" , OPERAND_TYPE_NIL, 0} ,
    {OP_CLSINSTMK  , "clsinstmk"  , OPERAND_TYPE_NIL, 0} ,
    {OP_CLSINSTRM  , "clsinstrm"  , OPERAND_TYPE_NIL, 0} ,
    {OP_CLSPGET    , "clspget"    , OPERAND_TYPE_NIL, 0} ,
    {OP_CLSPSET    , "clspset"    , OPERAND_TYPE_NIL, 0} ,
    {OP_CLSMADD    , "clsmadd"    , OPERAND_TYPE_NIL, 0} ,
    {OP_CLSCTORADD , "clsctoradd" , OPERAND_TYPE_NIL, 0} ,
    {OP_CLSDTORADD , "clsdtoradd" , OPERAND_TYPE_NIL, 0} ,
    {OP_CLSMINVOKE , "clsminvoke" , OPERAND_TYPE_NIL, 0} ,
    {OP_CLSDTOR    , "clsdtor"    , OPERAND_TYPE_NIL, 0} ,
    {OP_REFGET     , "refget"     , OPERAND_TYPE_NIL, 2} ,
    {OP_REFSET     , "refset"     , OPERAND_TYPE_NIL, 3} ,
    {OP_LSTMK      , "lstmk"      , OPERAND_TYPE_NUM, 0} ,
    {OP_LSTUNPACK  , "lstunpack"  , OPERAND_TYPE_NIL, 0} ,
    {OP_LSTUNPACKR , "lstunpackr" , OPERAND_TYPE_NIL, 0} ,
    {OP_LSTCAR     , "lstcar"     , OPERAND_TYPE_NIL, 0} ,
    {OP_LSTCDR     , "lstcdr"     , OPERAND_TYPE_NIL, 0} ,
    {OP_LSTCARSET  , "lstcarset"  , OPERAND_TYPE_NIL, 0} ,
    {OP_LSTCDRSET  , "lstcdrset"  , OPERAND_TYPE_NIL, 0} ,
    {OP_LSTADD     , "lstadd"     , OPERAND_TYPE_NIL, 0} ,
    {OP_LSTADDH    , "lstaddh"    , OPERAND_TYPE_NIL, 0} ,
    {OP_ARRMK      , "arrmk"      , OPERAND_TYPE_NUM, 0} ,
    {OP_ARRCAR     , "arrcar"     , OPERAND_TYPE_NIL, 0} ,
    {OP_ARRCDR     , "arrcdr"     , OPERAND_TYPE_NIL, 0} ,
    {OP_ARRADD     , "arradd"     , OPERAND_TYPE_NIL, 0} ,
    {OP_TUPMK      , "tupmk"      , OPERAND_TYPE_NUM, 0} ,
    {OP_TUPCAR     , "tupcar"     , OPERAND_TYPE_NIL, 0} ,
    {OP_TUPCDR     , "tupcdr"     , OPERAND_TYPE_NIL, 0} ,
    {OP_HASHMK     , "hashmk"     , OPERAND_TYPE_NUM, 0} ,
    {OP_HASHADD    , "hashadd"    , OPERAND_TYPE_NIL, 0} ,
    {OP_HASHDEL    , "hashdel"    , OPERAND_TYPE_NIL, 0} ,
    {OP_HASHHASKEY , "hashhaskey" , OPERAND_TYPE_NIL, 0} ,
    {OP_HASHCAR    , "hashcar"    , OPERAND_TYPE_NIL, 0} ,
    {OP_HASHCDR    , "hashcdr"    , OPERAND_TYPE_NIL, 0} ,
    {OP_PAIRMK     , "pairmk"     , OPERAND_TYPE_NIL, 0} ,
    {OP_PAIRCAR    , "paircar"    , OPERAND_TYPE_NIL, 0} ,
    {OP_PAIRCDR    , "paircdr"    , OPERAND_TYPE_NIL, 0} ,
    {OP_PAIRCARSET , "paircarset" , OPERAND_TYPE_NIL, 0} ,
    {OP_PAIRCDRSET , "paircdrset" , OPERAND_TYPE_NIL, 0} ,
    {OP_FUNCMK     , "funcmk"     , OPERAND_TYPE_NIL, 1} ,
    {OP_NFUNCMK    , "nfuncmk"    , OPERAND_TYPE_NUM, 0} ,
    {OP_LAMBDAMK   , "lambdamk"   , OPERAND_TYPE_NUM, 0} ,
    {OP_CONTMK     , "contmk"     , OPERAND_TYPE_NUM, 0} ,
    {OP_PROMMK     , "prommk"     , OPERAND_TYPE_NUM, 0} ,
    {OP_PROMC      , "promc"      , OPERAND_TYPE_NUM, 0} ,
    {OP_SYMMK      , "symmk"      , OPERAND_TYPE_RES, 0} ,
};
#define VIRTUAL_MACHINE_OPCODE_ITEM_COUNT (sizeof(virtual_machine_opcode_item) / sizeof(struct virtual_machine_opcode_item))

int virtual_machine_opcode_to_instrument(char **str, size_t *len, uint32_t opcode)
{
    unsigned int i;

    for (i = 0; i != VIRTUAL_MACHINE_OPCODE_ITEM_COUNT; i++)
    {
        if (virtual_machine_opcode_item[i].opcode == opcode)
        {
            *str = (char *)virtual_machine_opcode_item[i].name;
            if (len != NULL)
            {
                *len = strlen(virtual_machine_opcode_item[i].name);
            }
            return 0;
        }
    }

    return -MULTIPLE_ERR_ATOMIC;
}

int virtual_machine_instrument_to_opcode(uint32_t *opcode_out, const char *str, const size_t len)
{
    unsigned int i;

    for (i = 0; i != VIRTUAL_MACHINE_OPCODE_ITEM_COUNT; i++)
    {
        if ((strlen(virtual_machine_opcode_item[i].name) == len) && \
                (strncmp(virtual_machine_opcode_item[i].name, str, len) == 0))
        {
            *opcode_out = virtual_machine_opcode_item[i].opcode;
            return 0;
        }
    }

    return -MULTIPLE_ERR_ATOMIC;
}

int virtual_machine_instrument_to_operand_type(int *operand_type_out, uint32_t opcode)
{
    unsigned int i;

    for (i = 0; i != VIRTUAL_MACHINE_OPCODE_ITEM_COUNT; i++)
    {
        if (virtual_machine_opcode_item[i].opcode == opcode)
        {
            *operand_type_out = virtual_machine_opcode_item[i].oprand_type;
            return 0;
        }
    }

    return 0;
}

int virtual_machine_computing_stack_check(uint32_t opcode, size_t computing_stack_size)
{
    struct virtual_machine_opcode_item *opcode_item = virtual_machine_opcode_item + opcode;
    if ((int)computing_stack_size >= opcode_item->stack_require)
    { return 0; }
    else
    {
        return -1; 
    }
}

