/*
 *	NMH's Simple C Compiler, 2011,2012
 *	Definitions
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#define PREFIX		'C'
#define LPREFIX		'L'

#ifndef SCCDIR
 #define SCCDIR		"."
#endif

#define ASCMD		"as -o %s %s"
#define LDCMD		"ld -o %s %s/lib/crt0.o"
#define SCCLIBC		"%s/lib/libscc.a"
#define SYSLIBC		"/usr/lib/libc.a"

#define INTSIZE		4
#define PTRSIZE		INTSIZE
#define CHARSIZE	1
#define CHAROFF		0

#define TEXTLEN		512
#define NAMELEN		16

#define MAXFILES	32

#define MAXIFDEF	16
#define MAXNMAC		32
#define MAXCASE		256
#define MAXBREAK	16
#define MAXLOCINIT	32
#define MAXFNARGS	32

#define NSYMBOLS	1024
#define POOLSIZE	8192

/* types */
#define TVARIABLE	1
#define TARRAY		2
#define TFUNCTION	3
#define TCONSTANT	4
#define TMACRO		5

/* primitive types */
#define PCHAR	1
#define PINT	2
#define CHARPTR	3
#define INTPTR	4
#define CHARPP	5
#define INTPP	6
#define PVOID	7
#define VOIDPTR	8
#define VOIDPP	9
#define FUNPTR	10

/* storage classes */
#define CPUBLIC	1
#define CEXTERN	2
#define CSTATIC	3
#define CLSTATC	4
#define CAUTO	5

/* lvalue structure */
#define LVSYM	0
#define LVPRIM	1
#define LV	2

/* debug options */
#define D_LSYM	1
#define D_GSYM	2
#define D_STAT	4

enum {
	SLASH, STAR, MOD, PLUS, MINUS, LSHIFT, RSHIFT,
	GREATER, GTEQ, LESS, LTEQ, EQUAL, NOTEQ, AMPER,
	CARET, PIPE, LOGAND, LOGOR,

	__ARGC, ASAND, ASXOR, ASLSHIFT, ASMINUS, ASMOD, ASOR,
	ASPLUS, ASRSHIFT, ASDIV, ASMUL, ASSIGN, BREAK, CASE,
	CHAR, COLON, COMMA, CONTINUE, DECR, DEFAULT, DO, ELLIPSIS,
	ELSE, ENUM, EXTERN, FOR, IDENT, IF, INCR, INT, INTLIT,
	LBRACE, LBRACK, LPAREN, NOT, QMARK, RBRACE, RBRACK,
	RETURN, RPAREN, SEMI, SIZEOF, STATIC, STRLIT, SWITCH,
	TILDE, VOID, WHILE, XEOF, XMARK,

	P_INCLUDE, P_DEFINE, P_ENDIF, P_ELSE, P_ELSENOT, P_IFDEF,
	P_IFNDEF, P_UNDEF
};
