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

#ifndef _VM_OPCODE_H_
#define _VM_OPCODE_H_

#include <stdio.h>
#include <stdint.h>

/* Fast Library */
enum
{
    OP_FASTLIB_NOP = 0, 
    OP_FASTLIB_PUTCHAR, 
    OP_FASTLIB_GETCHAR, 
    OP_FASTLIB_ABS,
    OP_FASTLIB_GCD,
    OP_FASTLIB_LCM,
    OP_FASTLIB_EXPT,
    OP_FASTLIB_SQRT,

    OP_FASTLIB_MAX,
    OP_FASTLIB_MIN,

    OP_FASTLIB_SIN,
    OP_FASTLIB_COS,
    OP_FASTLIB_TAN,
    OP_FASTLIB_ASIN,
    OP_FASTLIB_ACOS,
    OP_FASTLIB_ATAN,

    OP_FASTLIB_EXP,
    OP_FASTLIB_LOG,

    OP_FASTLIB_UPCASE, 
    OP_FASTLIB_DOWNCASE, 
};

/* Opcode */
enum
{
    OP_NOP = 0, /* do nothing */
    OP_HALT, /* halt vm's running */
    OP_FASTLIB, /* fast lib for extended functions */
    OP_DEF, /* define a function */

    OP_ARGP, /* is at least one argument avaliable */
    OP_ARGCS, /* pass in an argument to computing stack */
    OP_LSTARGCS, /* pass in an argument to computing stack */
    OP_LSTRARGCS, /* pass in an argument to computing stack */

    OP_ARG, /* pass in an argument */
    OP_LSTARG, /* pass in remain arguments as list */
    OP_LSTRARG, /* pass in remain arguments as list in reverse order */
    OP_HASHARG, /* pass in remain arguments as hash */ 

    OP_ARGC,
    OP_ARGCL,
    OP_LSTARGC,
    OP_LSTRARGC,
    OP_HASHARGC,

    OP_PUSH, /* push data to computing stack */
    OP_PUSHC, /* push data to computing stack (looking up from environment) */

    OP_VARP, /* variable exists */
    OP_VARCP, /* variable exists (looking up from environment) */
    OP_VARCLP, /* variable exists (looking up from environment for one frame) */

    OP_POP, /* pop data from stack, and assign to a variable on current variable list */
    OP_POPC, /* pop data from stack, and assign to a variable in the environment */
    OP_POPCL, /* pop data from stack, and assign to a variable in current frame of the environment */
    OP_POPCX, /* pop data from stack, and assign to a variable in the environment, exists required */

    OP_PUSHG, /* push data to stack from global variable */
    OP_POPG, /* pop data from stack, and assign to a variable on global variable */
    OP_PUSHM, /* push data to stack from module variable */
    OP_POPM, /* pop data from stack, and assign to a variable on module variable */

    OP_MSCALL, /* Call into a Mask Stack Frame */
    OP_MSRETURN, /* Return from a Mask Stack Frame */
    OP_MSPUSH, /* Push variable from the top Mask Stack Frame */
    OP_MSPOP, 
    OP_MSREM, /* Remain n stack frame and pop the rest */

    OP_INT, /* invoke syscall */
    OP_IEGC, /* Enable GC */
    OP_IDGC, /* Disable GC */
    OP_TRAPSET, 
    OP_TRAP, 
    /* push args
     * push syscall number
     * int */
    OP_PRINT, /* print variable */
    /* push number of elements to be print
     * print */
    OP_RETURNTO, /* set previous running stack frame pc */
    OP_RETURN, /* return from a function */
    OP_RETNONE, /* return from a function with a none */
    OP_YIELD, /* yield */
    /* push return value 
     * return */
    OP_DOMAIN, /* domain prefix */
    OP_CALL, /* invoke specified function */
    OP_TAILCALL, /* invoke specified function */
    OP_CALLC, /* invoke specified function (closure version) */
    OP_TAILCALLC, /* invoke specified function (closure version) */
    /* reversely push args */
    /* push args number */
    /* push function id */
    /* push domain id */
    /* call */
    OP_DLCALL,   /* Call Dynamic library */
    OP_DUP,      /* duplicate the element on the top */
    OP_DROP,     /* drop the element on the top */
    OP_PICK,     /* move nth element up */
    OP_PICKCP,   /* copy nth element up */
    OP_INSERT,   /* move the top element into specific position */
    OP_INSERTCP, /* copy the top element into specific position */
    OP_REVERSE,  /* reverse arguments on the stack */
    OP_REVERSEP,  /* reverse arguments on the stack (preserve the count ) */
    OP_LIFT,     /* pass argument to upper stack frame */
    OP_JMP,      /* jump */
    OP_JMPC,     /* conditional jump */
    OP_JMPR,     /* relative jump */
    OP_JMPCR,    /* conditional relative jump */

    /* Thread */
    OP_TCUR,     /* get current thread */
    OP_TFK,      /* fork */
    OP_TWAIT,    /* wait */
    OP_TEXIT,    /* exit */
    OP_TALIVE,   /* alive */
    OP_TYIELD,   /* yield */
    OP_TSENDMSG, /* send message */
    OP_TRECVMSG, /* receive message (non-blocking) */
    OP_TISEMPTY, /* is message box empty */
    OP_TSUSPEND,
    OP_TRESUME,
    /* Mutex */
    OP_MTXMK,    /* Create a new mutex */
    OP_MTXLCK,   /* Lock */
    OP_MTXUNLCK, /* Unlock */
    /* Semaphore */
    OP_SEMMK,    /* Create a new semaphore */
    OP_SEMP,     /* P (wait) */
    OP_SEMV,     /* V (signal) */

    /* Arithmetic */
    OP_ADD, 
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,

    OP_LSHIFT,
    OP_RSHIFT,

    OP_ANDA,
    OP_ORA,
    OP_XORA,

    /* Logic */
    OP_ANDL,
    OP_ORL,
    OP_XORL,

    OP_EQ, /* == */
    OP_NE, /* != */
    OP_L, /* < */
    OP_G, /* > */
    OP_LE, /* <= */
    OP_GE, /* >= */

    /* Unary operations */
    OP_NEG,
    OP_NOTA,
    OP_NOTL,

    /* Type */
    OP_CONVERT,
    OP_SIZE,
    OP_TYPE,
    OP_TYPEP,
    OP_TYPEUP, /* Type Upgrade for top 2 elements */
    OP_SLV, /* Solving */
    OP_TRYSLV, /* Try Solving */
    OP_STI, /* String -> Identifier */
    OP_ITS, /* Identifier -> String */

    /* Class & Object */
    OP_CLSTYPEREG, /* Register Type */
    OP_CLSINSTMK, /* Make an instance */
    OP_CLSINSTRM, /* Remove an instance */
    OP_CLSPGET, /* Get property */
    OP_CLSPSET, /* Set property */
    OP_CLSMADD, /* Add new method */
    OP_CLSCTORADD, /* Add new constructor */
    OP_CLSDTORADD, /* Add new destructor */
    OP_CLSMINVOKE, /* Invoke method */
    OP_CLSDTOR, /* Confirm destruction */

    /* Composite Data Structures */
    OP_REFGET, /* get element by index */
    OP_REFSET, /* set element by index */

    /* List */
    OP_LSTMK, /* list make <elements> */
    OP_LSTUNPACK, /* unpack list (first on bottom) + list size */
    OP_LSTUNPACKR, /* unpack list (first on top) + list size */
    OP_LSTCAR, /* get first element */
    OP_LSTCDR, /* get list of rest elements */
    OP_LSTCARSET, /* set first element */
    OP_LSTCDRSET, /* set list of rest elements */
    OP_LSTADD, /* append new element */
    OP_LSTADDH, /* append new element to the head */

    /* Array */
    OP_ARRMK, /* array make <elements> */
    OP_ARRCAR, /* get first element */
    OP_ARRCDR, /* get list of rest elements */
    OP_ARRADD, /* append new element */

    /* Tuple */
    OP_TUPMK, /* tuple_make <elements> */
    OP_TUPCAR, /* get first element */
    OP_TUPCDR, /* get list of rest elements */

    /* Hash */
    OP_HASHMK, /* tuple make <key, value> */
    OP_HASHADD, /* append <key, value> */
    OP_HASHDEL, /* delete <key> */
    OP_HASHHASKEY, /* test if <key> exists */
    OP_HASHCAR, /* get first element */
    OP_HASHCDR, /* get list of rest element */

    /* Pair */
    OP_PAIRMK, /* pair make */
    OP_PAIRCAR, /* get first of pair */
    OP_PAIRCDR, /* get rest of pair */
    OP_PAIRCARSET, /* set first of pair */
    OP_PAIRCDRSET, /* set rest of pair */

    /* Function */
    OP_FUNCMK, /* Make function <domain:name> */
    OP_NFUNCMK,
    OP_LAMBDAMK,
    OP_CONTMK,
    OP_PROMMK,
    OP_PROMC,

    /* Symbol */
    OP_SYMMK, 

    OP_FINAL,
};
#define OPCODE_COUNT (OP_FINAL)

#define OPERAND_TYPE_NIL 0
#define OPERAND_TYPE_RES 1
#define OPERAND_TYPE_NUM 2

int virtual_machine_opcode_to_instrument(char **str, size_t *len, uint32_t opcode);
int virtual_machine_instrument_to_opcode(uint32_t *opcode_out, const char *str, const size_t len);
int virtual_machine_instrument_to_operand_type(int *operand_type_out, uint32_t opcode);

int virtual_machine_computing_stack_check(uint32_t opcode, size_t computing_stack_size);

#endif

