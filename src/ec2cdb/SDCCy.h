/* A Bison parser, made by GNU Bison 1.875c.  */

/* Skeleton parser for Yacc-like parsing with Bison,
   Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

/* As a special exception, when this file is copied by Bison into a
   Bison output file, you may use that output file without restriction.
   This special exception was added by the Free Software Foundation
   in version 1.24 of Bison.  */

/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     IDENTIFIER = 258,
     TYPE_NAME = 259,
     CONSTANT = 260,
     STRING_LITERAL = 261,
     SIZEOF = 262,
     TYPEOF = 263,
     PTR_OP = 264,
     INC_OP = 265,
     DEC_OP = 266,
     LEFT_OP = 267,
     RIGHT_OP = 268,
     LE_OP = 269,
     GE_OP = 270,
     EQ_OP = 271,
     NE_OP = 272,
     AND_OP = 273,
     OR_OP = 274,
     MUL_ASSIGN = 275,
     DIV_ASSIGN = 276,
     MOD_ASSIGN = 277,
     ADD_ASSIGN = 278,
     SUB_ASSIGN = 279,
     LEFT_ASSIGN = 280,
     RIGHT_ASSIGN = 281,
     AND_ASSIGN = 282,
     XOR_ASSIGN = 283,
     OR_ASSIGN = 284,
     TYPEDEF = 285,
     EXTERN = 286,
     STATIC = 287,
     AUTO = 288,
     REGISTER = 289,
     CODE = 290,
     EEPROM = 291,
     INTERRUPT = 292,
     SFR = 293,
     SFR16 = 294,
     SFR32 = 295,
     AT = 296,
     SBIT = 297,
     REENTRANT = 298,
     USING = 299,
     XDATA = 300,
     DATA = 301,
     IDATA = 302,
     PDATA = 303,
     VAR_ARGS = 304,
     CRITICAL = 305,
     NONBANKED = 306,
     BANKED = 307,
     SHADOWREGS = 308,
     WPARAM = 309,
     CHAR = 310,
     SHORT = 311,
     INT = 312,
     LONG = 313,
     SIGNED = 314,
     UNSIGNED = 315,
     FLOAT = 316,
     DOUBLE = 317,
     FIXED16X16 = 318,
     CONST = 319,
     VOLATILE = 320,
     VOID = 321,
     BIT = 322,
     STRUCT = 323,
     UNION = 324,
     ENUM = 325,
     ELIPSIS = 326,
     RANGE = 327,
     FAR = 328,
     CASE = 329,
     DEFAULT = 330,
     IF = 331,
     ELSE = 332,
     SWITCH = 333,
     WHILE = 334,
     DO = 335,
     FOR = 336,
     GOTO = 337,
     CONTINUE = 338,
     BREAK = 339,
     RETURN = 340,
     NAKED = 341,
     JAVANATIVE = 342,
     OVERLAY = 343,
     INLINEASM = 344,
     IFX = 345,
     ADDRESS_OF = 346,
     GET_VALUE_AT_ADDRESS = 347,
     SPIL = 348,
     UNSPIL = 349,
     GETHBIT = 350,
     GETABIT = 351,
     GETBYTE = 352,
     GETWORD = 353,
     BITWISEAND = 354,
     UNARYMINUS = 355,
     IPUSH = 356,
     IPOP = 357,
     PCALL = 358,
     ENDFUNCTION = 359,
     JUMPTABLE = 360,
     RRC = 361,
     RLC = 362,
     CAST = 363,
     CALL = 364,
     PARAM = 365,
     NULLOP = 366,
     BLOCK = 367,
     LABEL = 368,
     RECEIVE = 369,
     SEND = 370,
     ARRAYINIT = 371,
     DUMMY_READ_VOLATILE = 372,
     ENDCRITICAL = 373,
     SWAP = 374,
     INLINE = 375,
     RESTRICT = 376
   };
#endif
#define IDENTIFIER 258
#define TYPE_NAME 259
#define CONSTANT 260
#define STRING_LITERAL 261
#define SIZEOF 262
#define TYPEOF 263
#define PTR_OP 264
#define INC_OP 265
#define DEC_OP 266
#define LEFT_OP 267
#define RIGHT_OP 268
#define LE_OP 269
#define GE_OP 270
#define EQ_OP 271
#define NE_OP 272
#define AND_OP 273
#define OR_OP 274
#define MUL_ASSIGN 275
#define DIV_ASSIGN 276
#define MOD_ASSIGN 277
#define ADD_ASSIGN 278
#define SUB_ASSIGN 279
#define LEFT_ASSIGN 280
#define RIGHT_ASSIGN 281
#define AND_ASSIGN 282
#define XOR_ASSIGN 283
#define OR_ASSIGN 284
#define TYPEDEF 285
#define EXTERN 286
#define STATIC 287
#define AUTO 288
#define REGISTER 289
#define CODE 290
#define EEPROM 291
#define INTERRUPT 292
#define SFR 293
#define SFR16 294
#define SFR32 295
#define AT 296
#define SBIT 297
#define REENTRANT 298
#define USING 299
#define XDATA 300
#define DATA 301
#define IDATA 302
#define PDATA 303
#define VAR_ARGS 304
#define CRITICAL 305
#define NONBANKED 306
#define BANKED 307
#define SHADOWREGS 308
#define WPARAM 309
#define CHAR 310
#define SHORT 311
#define INT 312
#define LONG 313
#define SIGNED 314
#define UNSIGNED 315
#define FLOAT 316
#define DOUBLE 317
#define FIXED16X16 318
#define CONST 319
#define VOLATILE 320
#define VOID 321
#define BIT 322
#define STRUCT 323
#define UNION 324
#define ENUM 325
#define ELIPSIS 326
#define RANGE 327
#define FAR 328
#define CASE 329
#define DEFAULT 330
#define IF 331
#define ELSE 332
#define SWITCH 333
#define WHILE 334
#define DO 335
#define FOR 336
#define GOTO 337
#define CONTINUE 338
#define BREAK 339
#define RETURN 340
#define NAKED 341
#define JAVANATIVE 342
#define OVERLAY 343
#define INLINEASM 344
#define IFX 345
#define ADDRESS_OF 346
#define GET_VALUE_AT_ADDRESS 347
#define SPIL 348
#define UNSPIL 349
#define GETHBIT 350
#define GETABIT 351
#define GETBYTE 352
#define GETWORD 353
#define BITWISEAND 354
#define UNARYMINUS 355
#define IPUSH 356
#define IPOP 357
#define PCALL 358
#define ENDFUNCTION 359
#define JUMPTABLE 360
#define RRC 361
#define RLC 362
#define CAST 363
#define CALL 364
#define PARAM 365
#define NULLOP 366
#define BLOCK 367
#define LABEL 368
#define RECEIVE 369
#define SEND 370
#define ARRAYINIT 371
#define DUMMY_READ_VOLATILE 372
#define ENDCRITICAL 373
#define SWAP 374
#define INLINE 375
#define RESTRICT 376




#if ! defined (YYSTYPE) && ! defined (YYSTYPE_IS_DECLARED)
#line 70 "SDCC.y"
typedef union YYSTYPE {
    symbol     *sym ;      /* symbol table pointer       */
    structdef  *sdef;      /* structure definition       */
    char       yychar[SDCC_NAME_MAX+1];
    sym_link   *lnk ;      /* declarator  or specifier   */
    int        yyint;      /* integer value returned     */
    value      *val ;      /* for integer constant       */
    initList   *ilist;     /* initial list               */
    const char *yyinline;  /* inlined assembler code     */
    ast        *asts;      /* expression tree            */
} YYSTYPE;
/* Line 1275 of yacc.c.  */
#line 291 "SDCCy.h"
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
# define YYSTYPE_IS_TRIVIAL 1
#endif

extern YYSTYPE yylval;



