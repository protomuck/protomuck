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
 * This module contains the utility functions necessary to build and to
 * output the parse tree.  It also contains a few simple memory management
 * functions to ensure that memory doesn't leak during the parse process.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "mcpstruct.h"
#include "mcpparse.h"
#include "copyright.h"

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#ifdef DEBUG
#define MALLOC(x) my_malloc((x))
#define FREE(x) my_free((x))
static int malloc_count = 0;
static int free_count = 0;
#else
#define MALLOC(x) malloc((x))
#define FREE(x) free((x))
#endif

#define MCP_VER "1.5"

#define INDENT_STRING "    "

extern void yywarn(char *);
extern void yyerror(char *);

static mcp_memblock *mem_top;
static mcp_program prog;
static int have_vars = 0;
static int indent_level = 0;

static char buffer[1024];
static char retbuf[1024];

void insertComment(char *);
void writeComment(char *, FILE *);
void writeDirective(mcp_directive *, FILE *);
void writeStatement(mcp_slist *, FILE *);
void writeExpression(mcp_expr*);

#ifdef DEBUG
void *my_malloc(int size) {
    void *tmp = malloc(size);
    malloc_count++;
    return tmp;
}

void my_free(void *mem) {
    free(mem);
    free_count++;
}
#endif

void registerMem(void *mem) {
    if(!mem_top || mem_top->free == -1) {
        mcp_memblock *newmem = MALLOC(sizeof(mcp_memblock));
        newmem->free = 1021;
        newmem->next = mem_top;
        mem_top = newmem;
    }
    mem_top->mem[1021 - mem_top->free] = mem;
    mem_top->free--;
}

void clearMemBlock(mcp_memblock *mem) {
    if(mem) {
        int i;
        clearMemBlock(mem->next);
        for(i = mem->free + 1; i < 1022; i++) {
            FREE(mem->mem[1021-i]);
        }
        FREE(mem);
    } else
        return;
}

void clearMem(void) {
    clearMemBlock(mem_top);
    mem_top = NULL;
#ifdef DEBUG
    if(malloc_count != free_count) {
       fprintf(stderr, "BOGUS memory counts!!! %d allocs - %d frees\n",
	       malloc_count, free_count);
    }
#endif
}

char *savestr(char *string) {
    char *tmp = MALLOC(strlen(string)+1);
    strcpy(tmp, string);
    registerMem(tmp);
    return tmp;
}

void initMcpParser(void) {
    char buf[1024];
    have_vars = 0;
    indent_level = 0;
#ifdef DEBUG
    malloc_count = 0;
    free_count = 0;
#endif
    mem_top = NULL;
    prog.top = NULL;
    prog.vars = NULL;
    prog.pubs = NULL;
    sprintf(buf, "Created with MCP version %s", MCP_VER);
    insertComment(savestr(buf));
}

void insertVarPiece(int type, char *varname) {
    mcp_piecelist *tmp = prog.vars;
    while(tmp) {
       if(strcmp(tmp->t.val, varname) == 0) {
           char buf[1024];
           sprintf(buf, "Redeclaration of variable '%s' ignored", varname);
           yywarn(buf);
           return;
       }
       tmp = tmp->next;
    }
    tmp = MALLOC(sizeof(mcp_piecelist));
    registerMem(tmp);
    tmp->type = type;
    tmp->t.val = varname;
    tmp->next = prog.vars;
    prog.vars = tmp;
}

void insertPiece(int type, void *val) {
    mcp_piecelist *tmp = MALLOC(sizeof(mcp_piecelist));
    registerMem(tmp);
    tmp->type = type;
    if(type == PIECE_DEF || type == PIECE_FUN)
        tmp->t.def = (mcp_def *)val;
    else if(type == PIECE_DIRECTIVE)
        tmp->t.direct = (mcp_directive *)val;
    else
        tmp->t.val = (char *)val;
    tmp->next = prog.top;
    prog.top = tmp;
}

void insertPublicDecl(char *pub) {
    mcp_piecelist *tmp = MALLOC(sizeof(mcp_piecelist));
    registerMem(tmp);
    tmp->type = PIECE_PUB;
    tmp->t.val = pub;
    tmp->next = prog.pubs;
    prog.pubs= tmp;
}

void insertDefine(char *id, mcp_slist *def) {
    mcp_def *tmp= MALLOC(sizeof(mcp_def));
    registerMem(tmp);
    tmp->name = id;
    tmp->vars = NULL;
    tmp->dfn = def;
    insertPiece(PIECE_DEF, tmp);
}

void insertFunction(char *name, mcp_piecelist *args, mcp_slist *body) {
    mcp_def *tmp = MALLOC(sizeof(mcp_def));
    registerMem(tmp);
    tmp->name = name;
    tmp->vars = args;
    tmp->dfn = body;
    if (!have_vars) {
        insertPiece(PIECE_VAR, NULL); 
        have_vars = 1;
    }
    insertPiece(PIECE_FUN, tmp);
}

void insertComment(char *comment) { insertPiece(PIECE_CMT, comment); }

void insertDirective(mcp_directive *d) { insertPiece(PIECE_DIRECTIVE, d); }

void insertVar(int type, mcp_piecelist *var) {
    if(var)
        insertVar(type, var->next);
    else
        return;
    if(var->type == PIECE_BLANK)
        yyerror("[] found in var declaration");
    else if(var->type == PIECE_ARRAYREF)
#ifndef OLD_MUCK
	yyerror("array reference in var declaration");
#else
	yyerror("array reference are only supported in NeonMuck");
#endif    
    else
        insertVarPiece(type, var->t.val);
}

mcp_directive *makeDirective(int type, char *val) {
    mcp_directive *tmp = MALLOC(sizeof(mcp_directive));
    registerMem(tmp);
    tmp->type = type;
    tmp->val = val;
    return tmp;
}

mcp_slist *makeNull(void) {
    mcp_slist *tmp = MALLOC(sizeof(mcp_slist));
    registerMem(tmp);
    tmp->type = STMT_NULL;
    tmp->v.comment = NULL;
    tmp->next = NULL;
    return tmp;
}

mcp_slist *makeArrayDecl(char *id, mcp_expr *expr) {
    mcp_slist *tmp = makeNull();
#ifndef OLD_MUCK
    mcp_array *tarray = MALLOC(sizeof(mcp_array));
    registerMem(tarray);
    tarray->id = id;
    tarray->e = expr;
    tmp->type = STMT_ARRAYDECL;
    tmp->v.arr = tarray;
#else
    yyerror("Arrays are only supported in NeonMuck");
#endif
    return tmp;
}

mcp_slist *makeStatement(int type, mcp_expr *expr) {
    mcp_slist *tmp = makeNull();
    tmp->type = type;
    tmp->v.expr = expr;
    return tmp;
}

mcp_slist *makeComment(char *comment) {
    mcp_slist *tmp = makeNull();
    tmp->type = STMT_COMMENT;
    tmp->v.comment = comment;
    return tmp;
}

mcp_slist *makeDirectiveStatement(mcp_directive *directive) {
    mcp_slist *tmp = makeNull();
    tmp->type = STMT_DIRECTIVE;
    tmp->v.direct = directive;
    return tmp;
}

mcp_slist *makeIf(mcp_slist *cond, mcp_slist *c1, mcp_slist *c2) {
    mcp_slist *tmp = makeNull();
    mcp_if *tmpif = MALLOC(sizeof(mcp_if));
    registerMem(tmpif);
    tmpif->cond = cond;
    tmpif->clause1 = c1;
    tmpif->clause2 = c2;
    tmp->type = STMT_IF;
    tmp->v.ifcond = tmpif;
    return tmp;
}

mcp_slist *makeSwitch(int flag, mcp_expr *e, mcp_caselist *cases) {
    mcp_slist *tmp = makeNull();
    mcp_switch *tmpsw = MALLOC(sizeof(mcp_switch));
    registerMem(tmpsw);
    tmpsw->e = e;
    tmpsw->c = cases;
    tmp->type = (flag ) ? STMT_SWITCHALL : STMT_SWITCH;
    tmp->v.s = tmpsw;
    return tmp;
}

mcp_slist *makeAssign(mcp_piecelist *vars, mcp_expr *e) {
    mcp_slist *tmp = makeNull();
    mcp_assign *tmpa = MALLOC(sizeof(mcp_assign));
    registerMem(tmpa);
    tmpa->idents = vars;
    tmpa->e = e;
    tmp->type = STMT_ASSIGN;
    tmp->v.lassign = tmpa;
    return tmp;
}

mcp_slist *makeLoop(int type, mcp_slist *pre, mcp_slist *body,
		    mcp_slist *cond, mcp_slist *post) {
    mcp_slist *tmp = makeNull();
    mcp_loop *tmploop = MALLOC(sizeof(mcp_loop));
    registerMem(tmploop);
    tmploop->type = type;
    tmploop->cond = cond;
    tmploop->pre = pre;
    tmploop->post = post;
    tmploop->body = body;
    tmp->type = STMT_LOOP;
    tmp->v.loop = tmploop;
    return tmp;
}

mcp_slist *appendStatement(mcp_slist *nval, mcp_slist *list) {
    nval->next = list;
    return nval;
}

mcp_caselist *makeCase(mcp_expr *expr, mcp_slist *state) {
    mcp_caselist *tmp = MALLOC(sizeof(mcp_caselist));
    registerMem(tmp);
    tmp->next = NULL;
    tmp->s = state;
    tmp->e = expr;
    return tmp;
}

mcp_caselist *appendCase(mcp_caselist *nval, mcp_caselist *list) {
    nval->next = list;
    return nval;
}

mcp_expr *makeNumber(int val) {
    mcp_expr *tmp = MALLOC(sizeof(mcp_expr));
    registerMem(tmp);
    tmp->type = EXPR_NUM;
    tmp->val.i = val;
    return tmp;
}

mcp_expr *makeObject(char *val) {
    mcp_expr *tmp = MALLOC(sizeof(mcp_expr));
    registerMem(tmp);
    tmp->type = EXPR_OBJ;
    tmp->val.str = val;
    return tmp;
}

mcp_expr *makeString(char *val) {
    mcp_expr *tmp = MALLOC(sizeof(mcp_expr));
    registerMem(tmp);
    tmp->type = EXPR_STR;
    tmp->val.str = val;
    return tmp;
}

mcp_expr *makeTop(void) {
    mcp_expr *tmp = MALLOC(sizeof(mcp_expr));
    registerMem(tmp);
    tmp->type = EXPR_TOP;
    tmp->val.str = NULL;
    return tmp;
}

mcp_expr *makeIdent(char *val) {
    mcp_expr *tmp = MALLOC(sizeof(mcp_expr));
    registerMem(tmp);
    tmp->type = EXPR_ID;
    tmp->val.str = val;
    return tmp;
}

mcp_expr *makePush(mcp_exprlist *val) {
    mcp_expr *tmp = MALLOC(sizeof(mcp_expr));
    registerMem(tmp);
    tmp->type = EXPR_PUSH;
    tmp->val.push = val;
    return tmp;
}

mcp_expr *makeCall(char *fun, mcp_exprlist *args, char *obj) {
    mcp_expr *tmp = MALLOC(sizeof(mcp_expr));
    mcp_call *tmpcall = MALLOC(sizeof(mcp_call));
    registerMem(tmp);
    registerMem(tmpcall);
    tmpcall->ident = fun;
    if(obj)
      tmpcall->obj = makeObject(obj);
    else
      tmpcall->obj = NULL;
    tmpcall->args = args;
    tmp->type = EXPR_CALL;
    tmp->val.call = tmpcall;
    return tmp;
}

mcp_expr *makeIncr(int type, char *id) {
    mcp_expr *tmp = MALLOC(sizeof(mcp_expr));
    registerMem(tmp);
    tmp->type = type;
    tmp->val.str = id;
    return tmp;
}

mcp_expr *makeOp(int op, mcp_expr *l, mcp_expr *r) {
    mcp_expr *tmp = MALLOC(sizeof(mcp_expr));
    mcp_op *tmpop = MALLOC(sizeof(mcp_op));
    registerMem(tmp);
    registerMem(tmpop);
    tmpop->op = op;
    tmpop->left = l;
    tmpop->right = r; 
    tmp->type = EXPR_OP;
    tmp->val.op = tmpop;
    return tmp;
}

mcp_expr *makeArrayRef(char *id, mcp_expr *e) {
#ifndef OLD_MUCK
    mcp_expr *tmp = MALLOC(sizeof(mcp_expr));
    mcp_array *tarray = MALLOC(sizeof(mcp_array));
    registerMem(tmp);
    registerMem(tarray);
    tarray->id = id;
    tarray->e = e;
    tmp->val.ar = tarray;
    tmp->type = EXPR_ARRAYREF;
    return tmp;
#else
    yyerror("Arrays are only supported in NeonMuck");
    return NULL;
#endif
}

mcp_exprlist *makeExpr(mcp_expr *e) {
    mcp_exprlist *tmp = MALLOC(sizeof(mcp_exprlist));
    registerMem(tmp);
    tmp->next = NULL;
    tmp->e = e;
    return tmp;
}

mcp_exprlist *appendExpr(mcp_expr *e, mcp_exprlist *l) {
    mcp_exprlist *tmp = makeExpr(e);
    tmp->next = l;
    return tmp;
}

mcp_piecelist *makeArray(char *id, mcp_expr *e) {
    mcp_piecelist *tmp = MALLOC(sizeof(mcp_piecelist));
#ifndef OLD_MUCK
    mcp_array *tarray = MALLOC(sizeof(mcp_array));
    registerMem(tmp);
    registerMem(tarray);
    tarray->id = id;
    tarray->e = e;
    tmp->type = PIECE_ARRAYREF;
    tmp->next = NULL;
    tmp->t.arr = tarray;
#else
    yyerror("Arrays are only supported in NeonMuck");
#endif
    return tmp;
}

mcp_piecelist *makeId(char *val) {
    mcp_piecelist *tmp = MALLOC(sizeof(mcp_piecelist));
    registerMem(tmp);
    tmp->next = NULL;
    tmp->type = PIECE_ID;
    tmp->t.val = val;
    return tmp;
}

mcp_piecelist *makeBlank(void) {
    mcp_piecelist *tmp = MALLOC(sizeof(mcp_piecelist));
    registerMem(tmp);
    tmp->next = NULL;
    tmp->type = PIECE_BLANK;
    return tmp;
}

mcp_piecelist *appendId(mcp_piecelist *el, mcp_piecelist *l) {
    el->next = l;
    return el;
}

void writeLine(FILE *f, char *line, ...) {
    int i;
    va_list vars;
    va_start(vars, line);
    for(i = 0; i < indent_level; i++) {
        fprintf(f, "%s", INDENT_STRING);
    }
    vfprintf(f, line, vars);
    va_end(vars);
}

void writeArgs(mcp_piecelist *args, FILE *f) {
    mcp_piecelist *tmp = args;
    while(tmp) {
	if(tmp->type == PIECE_BLANK)
	    writeLine(f, "pop\n");
	else if(tmp->type == PIECE_ID && tmp->t.val)
	    writeLine(f, "%s !\n", tmp->t.val);
#ifndef OLD_MUCK
	else if(tmp->type == PIECE_ARRAYREF) {
	    writeExpression(tmp->t.arr->e);
	    writeLine(f, "%s%s a!\n", retbuf, tmp->t.arr->id);
	}
#endif
	tmp = tmp->next;
    }
}

void writeExprList(mcp_exprlist *elist) {
    char buf[1024];
    strcpy(retbuf, "");
    strcpy(buf, "");
    if(elist) {
        writeExprList(elist->next);
        strcpy(buf, retbuf);
        writeExpression(elist->e);
        strcat(buf, retbuf);
        strcpy(retbuf, buf);
    } else
        return;
}

void writeOp(mcp_op *op) {
    char buf[1024];

    if(op->op == SUB && !op->right) {
      writeExpression(op->left);
      sprintf(buf, "0 %s", retbuf);
      strcpy(retbuf, buf);
    } else if (op->op == NOT) {
      writeExpression(op->left);
    } else if (op->op == SHIFTR) {
      writeExpression(op->left);
      sprintf(buf, "%s0 ", retbuf);
      writeExpression(op->right);
      strcat(buf, retbuf);
      strcat(buf, "- ");
      strcpy(retbuf, buf);
    } else {
      writeExpression(op->left);
      sprintf(buf, "%s", retbuf);
      writeExpression(op->right);
      strcat(buf, retbuf);
      strcpy(retbuf, buf);
    }
    switch(op->op) {
        case ADD:
            strcat(retbuf, "+ ");
            break;
        case SUB:
            strcat(retbuf, "- ");
            break;
        case MUL:
            strcat(retbuf, "* ");
            break;
        case DIV:
            strcat(retbuf, "/ ");
            break;
        case MOD:
            strcat(retbuf, "% ");
            break;
        case AND:
            strcat(retbuf, "and ");
            break;
        case OR:
            strcat(retbuf, "or ");
            break;
        case NOT:
            strcat(retbuf, "not ");
            break;
        case BITOR:
            strcat(retbuf, "bitor ");
            break;
        case BITXOR:
            strcat(retbuf, "bitxor ");
            break;
        case BITAND:
            strcat(retbuf, "bitand ");
            break;
        case SHIFTR:
        case SHIFTL:
            strcat(retbuf, "bitshift ");
            break;
        case EQ:
            strcat(retbuf, "= ");
            break;
        case NE:
            strcat(retbuf, "= not ");
            break;
        case GT:
            strcat(retbuf, "> ");
            break;
        case GTE:
            strcat(retbuf, ">= ");
            break;
        case LT:
            strcat(retbuf, "< ");
            break;
        case LTE:
            strcat(retbuf, "<= ");
            break;
    }
}

void writeExpression(mcp_expr *e) {
    if(!e || !e->type)
       return;

    switch(e->type) {
        case EXPR_NUM:
            sprintf(retbuf, "%d ", e->val.i);
            break;
#ifndef OLD_MUCK
        case EXPR_ARRAYREF:
            writeExpression(e->val.ar->e);
            strcpy(buffer, retbuf);
            sprintf(retbuf, "%s%s a@ ", buffer, e->val.ar->id);
	    break;
#endif
        case EXPR_OBJ:
            if(*e->val.str != '#') {
                sprintf(retbuf, "\"%s\" ", e->val.str);
                strcat(retbuf, "match ");
            } else
                sprintf(retbuf, "%s ", e->val.str);
            break;
        case EXPR_STR:
            sprintf(retbuf, "%s ", e->val.str);
            break;
        case EXPR_TOP:
            sprintf(retbuf, " ");
	    break;
        case EXPR_ID:
            sprintf(retbuf, "%s @ ", e->val.str);
            break;
        case EXPR_PUSH:
            writeExprList(e->val.push);
            break;
        case EXPR_CALL:
            writeExprList(e->val.call->args);
            strcpy(buffer, retbuf);
            if(e->val.call->obj) {
                writeExpression(e->val.call->obj);
                strcat(buffer, retbuf);
                sprintf(retbuf, "%s\"%s\" call ", buffer, e->val.call->ident);
            } else
                sprintf(retbuf, "%s%s ", buffer, e->val.call->ident);
            break;
#ifndef OLD_MUCK
        case EXPR_INC:
        case EXPR_DEC:
            sprintf(retbuf, "%s %s ", e->val.str,
                            ((e->type == EXPR_INC) ? "++" : "--"));
            break;
#else
	case EXPR_INC:
	    sprintf(retbuf, "%s @ 1 + %s ! ", e->val.str, e->val.str);
	    break;
	case EXPR_DEC:
	    sprintf(retbuf, "%s @ 1 - %s ! ", e->val.str, e->val.str);
	    break;
#endif
        case EXPR_OP:
            writeOp(e->val.op);
            break;
    }
}

void writeLoop(mcp_loop *loop, FILE *f) {
    if(loop->type == LOOP_FOR && loop->pre)
        writeStatement(loop->pre, f);
    writeLine(f, "begin\n");
    if(loop->type == LOOP_DO) {
        indent_level++;
        writeStatement(loop->body, f);
        indent_level--;
        if(loop->cond->type == STMT_EXPR) {
            writeExpression(loop->cond->v.expr);
            writeLine(f, "%suntil", retbuf);
        } else {
            writeStatement(loop->cond, f);
            writeLine(f, "until");
        }
    } else {
        if(loop->cond->type == STMT_EXPR) {
            writeExpression(loop->cond->v.expr);
            writeLine(f, "%swhile", retbuf);
        } else {
            writeStatement(loop->cond, f);
            writeLine(f, "while");
        }
        indent_level++;
        writeStatement(loop->body, f);
        if(loop->type == LOOP_FOR && loop->post)
            writeStatement(loop->post, f);
        indent_level--;
        writeLine(f, "repeat");
    }
}

void writeIf(mcp_if *ifcond, FILE *f) {
    if(ifcond->cond->type == STMT_EXPR) {
        writeExpression(ifcond->cond->v.expr);
        writeLine(f, "%sif\n", retbuf);
    } else {
        writeStatement(ifcond->cond, f);
        writeLine(f, "if\n");
    }
    indent_level++;
    writeStatement(ifcond->clause1, f);
    if(ifcond->clause2) {
        indent_level--;
        writeLine(f, "else\n");
        indent_level++;
        writeStatement(ifcond->clause2, f);
    }
    indent_level--;
    writeLine(f, "then"); 
}

int writeCases(mcp_expr *e, mcp_caselist *cases, FILE *f) {
    char buf[1024];
    int count = 0;
    if(cases)
        count = writeCases(e, cases->next, f);
    else
        return 0;

    if(cases->e) {
        indent_level += count;
        writeExpression(e);
        strcpy(buf, retbuf);
        writeExpression(cases->e);
        writeLine(f, "%s%s= if\n", buf, retbuf);
        indent_level++;
        writeStatement(cases->s, f);
        indent_level--;
        writeLine(f, "else\n");
        indent_level -= count;
    } else {
        indent_level += count;
        writeStatement(cases->s, f);
        indent_level -= count;
        for(;count>0; count--) {
           indent_level += count-1;
           writeLine(f, "then\n");
           indent_level -= count-1;
        }
    }
    return count + 1;
}

void writeAllCases(mcp_expr *e, mcp_caselist *cases, FILE *f) {
    char buf[1024];
    if(cases)
	writeAllCases(e, cases->next, f);
    else
        return;

    if(cases->e) {
        writeExpression(e);
        strcpy(buf, retbuf);
        writeExpression(cases->e);
        writeLine(f, "%s%s= if\n", buf, retbuf);
        indent_level++;
        writeStatement(cases->s, f);
        indent_level--;
        writeLine(f, "then\n");
    } else
        writeStatement(cases->s, f);
}

void writeStatement(mcp_slist *states, FILE *f) {
    if(states)
        writeStatement(states->next, f);
    else
        return;

    switch(states->type) {
        case STMT_NULL:
	    break;
#ifndef OLD_MUCK
	case STMT_ARRAYDECL:
            writeExpression(states->v.arr->e);
 	    writeLine(f, "%s %snewarray\n", states->v.arr->id, retbuf);
	    break;
#endif
        case STMT_SWITCH:
	    (void)writeCases(states->v.s->e, states->v.s->c, f);
            break;
        case STMT_SWITCHALL:
	    writeAllCases(states->v.s->e, states->v.s->c, f);
            break;
        case STMT_LOOP:
            writeLoop(states->v.loop, f);
            writeLine(f, "\n");
            break;
        case STMT_IF:
            writeIf(states->v.ifcond, f);
            writeLine(f, "\n");
            break;
        case STMT_CONT:
            writeLine(f, "continue\n");
            break;
        case STMT_BREAK:
            writeLine(f, "break\n");
            break;
        case STMT_DIRECTIVE:
	    writeDirective(states->v.direct, f);
	    writeLine(f, "\n");
	    break;
        case STMT_COMMENT:
            writeComment(states->v.comment, f);
            writeLine(f, "\n");
            break;
        case STMT_ASSIGN:
	    writeExpression(states->v.lassign->e);
            if(states->v.lassign->idents->next) {
                writeLine(f, "%s\n", retbuf);
                writeArgs(states->v.lassign->idents, f);
            } else
		if(states->v.lassign->idents->type == PIECE_ID) {
		    writeLine(f, "%s%s !\n", retbuf,
			      states->v.lassign->idents->t.val);
		} else if(states->v.lassign->idents->type == PIECE_BLANK) {
		    writeLine(f, "pop\n", retbuf,
			      states->v.lassign->idents->t.val);
#ifndef OLD_MUCK
		} else if(states->v.lassign->idents->type == PIECE_ARRAYREF) {
		    char buf[1024];
		    strcpy(buf, retbuf);
		    writeExpression(states->v.lassign->idents->t.arr->e);
		    writeLine(f, "%s%s%s a!\n", buf, retbuf,
			      states->v.lassign->idents->t.arr->id);
#endif
		}
            break;
        case STMT_RETURN:
            strcpy(retbuf, "");
            if(states->v.expr)
                writeExpression(states->v.expr);
            strcat(retbuf, "exit");
            writeLine(f, "%s\n", retbuf);
            break;
        case STMT_EXPR:
            writeExpression(states->v.expr);
            writeLine(f, "%s\n", retbuf);
            break;
    }
}

void writeDef(mcp_def *def, FILE *f) {
    writeLine(f, "$define %s\n", def->name);
    indent_level++;
    writeStatement(def->dfn, f);
    indent_level--;
    writeLine(f, "$enddef\n", def->name);
}

void writeFun(mcp_def *fun, FILE *f) {
    writeLine(f, ": %s\n", fun->name);
    indent_level++;
    if(fun->vars->type == PIECE_BLANK || fun->vars->t.val) {
        writeArgs(fun->vars, f);
        writeLine(f, "\n");
    }
    writeStatement(fun->dfn, f);
    indent_level--;
    writeLine(f, ";\n");
}

void writePublic(char *name, FILE *f) {
    writeLine(f, "public %s\n", name);
}

void translateStr(char *str, char input, char output) {
    char *tmp = str;
    while(*tmp) {
       if(*tmp == input) *tmp = output;
       tmp++;
    }
}

void writeComment(char *comment, FILE *f) {
    char buf[1024], *t = comment;
    while(isspace(*t) || *t == '/' || *t == '*')
        t++;
    strcpy(buf, t);
    t = buf + strlen(t) - 1;
    while(isspace(*t) || *t == '/' || *t == '*')
        t--;
    *++t = '\0';

    translateStr(buf, '(', '<');
    translateStr(buf, ')', '>');

    writeLine(f, "( %s )", buf);
}

void writeInclude(char *object, FILE *f) {
    writeLine(f, "$include %s\n", object);
}

void writeUndef(char *undef, FILE *f) {
    writeLine(f, "$undef %s\n", undef);
}

void writeDirective(mcp_directive *d, FILE *f) {
    int temp = indent_level;
    indent_level = 0;
    switch(d->type) {
	case DIR_NULL: break;
	case DIR_ECHO: writeLine(f, "$echo"); break;
	case DIR_IFDEF: writeLine(f, "$ifdef"); break;
	case DIR_IFNDEF: writeLine(f, "$ifndef"); break;
	case DIR_ELSEDEF: writeLine(f, "$else"); break;
	case DIR_ENDIF: writeLine(f, "$endif"); break;
	case DIR_UNDEF: writeLine(f, "$undef"); break;
	case DIR_INCLUDE: writeLine(f, "$include"); break;
    }
    if(d->type != DIR_NULL && d->val)
	writeLine(f, " %s", d->val);
    indent_level = temp;
}

void writeVars(mcp_piecelist *vars, FILE *f) {
    if(vars)
      writeVars(vars->next, f);
    else
      return;

    writeLine(f, "%s %s\n", ((vars->type == PIECE_VAR) ? "var" : "lvar"),
                 vars->t.val);
}

void writePiece(mcp_piecelist *piece, FILE *f)
{
    if(piece) {
        writePiece(piece->next, f);
        if(piece->next && piece->next->type != piece->type)
            writeLine(f, "\n");
        if(piece->next && piece->next->type == PIECE_FUN &&
           piece->type == PIECE_FUN)
            writeLine(f, "\n");
    } else
        return;
    switch(piece->type) {
        case PIECE_DEF:
	    writeDef(piece->t.def, f);
            break;
        case PIECE_FUN:
            writeFun(piece->t.def, f);
            break;
        case PIECE_PUB:
            writePublic(piece->t.val, f);
            break;
        case PIECE_VAR:
            writeVars(prog.vars, f);
            break;
        case PIECE_DIRECTIVE:
	    writeDirective(piece->t.direct, f);
	    writeLine(f, "\n");
	    break;
        case PIECE_CMT:
            writeComment(piece->t.val, f);
            writeLine(f, "\n");
            break;
    }
}

void writeMcpProgram(FILE *f) {
    writePiece(prog.top, f);
    writeLine(f, "\n");
    writePiece(prog.pubs, f);
    return;
}

