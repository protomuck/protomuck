/*
 * MCP parser by Joseph Traub and Jeremy Blackman
 * Copyright(C) 1996, Joseph L. Traub and Jeremy Blackman, All rights reserved.
 *
 * This software is part of the NeonMuck distribution and is usable under
 * the terms and conditions of the license attendant with that software.
 *
 * This code is based loosely on ideas and concepts of the MUV grammer written
 * by Russ Smith and Marcus Ranum.
 *
 * This module contains the structures and definitions used in the parse
 * tree as well as some miscellaneous structures for memory management.
 */

#include "copyright.h"

typedef struct _mcp_memblock {
    int free;
    struct _mcp_memblock *next;
    void *mem[1022];
} mcp_memblock;

typedef struct _mcp_array {
    char *id;
    struct _mcp_expr *e;
} mcp_array;

typedef struct _mcp_exprlist {
    struct _mcp_expr *e;
    struct _mcp_exprlist *next;
} mcp_exprlist;

typedef struct _mcp_call {
    char *ident;
    struct _mcp_expr *obj;
    mcp_exprlist *args;
} mcp_call;

typedef struct _mcp_op {
    int op;
    struct _mcp_expr *left;
    struct _mcp_expr *right;
} mcp_op;

typedef struct _mcp_expr {
    #define EXPR_NULL 0
    #define EXPR_OBJ 1
    #define EXPR_STR 2
    #define EXPR_TOP 3
    #define EXPR_ID 4
    #define EXPR_PUSH 6
    #define EXPR_CALL 7
    #define EXPR_INC 8
    #define EXPR_DEC 9
    #define EXPR_OP 10
    #define EXPR_NUM 11
    #define EXPR_ARRAYREF 12

    char type;
    union {
	char *str;
	int i;
	mcp_call *call;
	mcp_exprlist *push;
	mcp_op *op;
        mcp_array *ar;
    } val;
} mcp_expr;

typedef struct _mcp_if {
    struct _mcp_slist *cond;
    struct _mcp_slist *clause1;
    struct _mcp_slist *clause2;
} mcp_if;

typedef struct _mcp_loop {
    #define LOOP_DO 0
    #define LOOP_WHILE 1
    #define LOOP_FOR 2

    char type;
    struct _mcp_slist *pre;
    struct _mcp_slist *cond;
    struct _mcp_slist *body;
    struct _mcp_slist *post;
} mcp_loop;

typedef struct _mcp_directive {
    #define DIR_NULL 0
    #define DIR_ECHO 1
    #define DIR_IFDEF 2
    #define DIR_IFNDEF 3
    #define DIR_ELSEDEF 4
    #define DIR_ENDIF 5
    #define DIR_UNDEF 6
    #define DIR_INCLUDE 7

    char type;
    char *val;
} mcp_directive;

typedef struct _mcp_assign {
    struct _mcp_piecelist *idents;
    mcp_expr *e;
} mcp_assign;

typedef struct _mcp_caselist {
    mcp_expr *e;
    struct _mcp_slist *s;
    struct _mcp_caselist *next;
} mcp_caselist;

typedef struct _mcp_switch {
    mcp_expr *e;
    mcp_caselist *c;
} mcp_switch;

typedef struct _mcp_slist {
    #define STMT_EXPR 0
    #define STMT_COMMENT 1
    #define STMT_RETURN 2
    #define STMT_BREAK 3
    #define STMT_CONT 4
    #define STMT_IF 5
    #define STMT_LOOP 6
    #define STMT_NULL 7
    #define STMT_ASSIGN 8
    #define STMT_DIRECTIVE 9
    #define STMT_SWITCH 10
    #define STMT_SWITCHALL 11
    #define STMT_ARRAYDECL 12

    char type;
    union {
	mcp_expr *expr;
	mcp_loop *loop;
	mcp_if *ifcond;
	mcp_assign *lassign;
	char *comment;
	mcp_directive *direct;
        mcp_switch *s;
        mcp_array *arr;
    } v;
    struct _mcp_slist *next;
} mcp_slist;

typedef struct _mcp_def {
    char *name;
    struct _mcp_piecelist *vars;
    mcp_slist *dfn;
} mcp_def;

typedef struct _mcp_piecelist {
    #define PIECE_BLANK 0
    #define PIECE_ID 1
    #define PIECE_DEF 2
    #define PIECE_FUN 3
    #define PIECE_PUB 4
    #define PIECE_CMT 5
    #define PIECE_VAR 6
    #define PIECE_LVAR 7
    #define PIECE_DIRECTIVE 8
    #define PIECE_ARRAYREF 9

    char type;
    union {
	mcp_def *def;
	char *val;
	mcp_directive *direct;
        mcp_array *arr;
    } t;
    struct _mcp_piecelist *next;
} mcp_piecelist;

typedef struct _mcp_program {
    mcp_piecelist *top;
    mcp_piecelist *vars;
    mcp_piecelist *pubs;
} mcp_program;
