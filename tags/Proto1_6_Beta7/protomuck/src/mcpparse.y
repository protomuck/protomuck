%{
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
 * This module contains the BNF in YACC for the MCP language, and builds the
 * parse tree which is then translated into MUF.
 */

#include	<stdio.h>
#include	<ctype.h>
#include	<string.h>
#include	<stdlib.h>
#include	"mcpstruct.h"
#include	"copyright.h"

#ifndef TEST
#include "parser.h"
#endif

void yyerror(char *);
void yywarn(char *);

extern int yylex(void);

extern void initMcpParser(void);
extern void writeMcpProgram(FILE *);
extern void clearMem(void);

extern void insertFunction(char *, mcp_piecelist *, mcp_slist *);
extern void insertPublicDecl(char *);
extern void insertDefine(char *, mcp_slist *);
extern void insertComment(char *);
extern void insertDirective(mcp_directive *);
extern void insertVar(int, mcp_piecelist *);

extern mcp_directive *makeDirective(int, char *);

extern mcp_slist *makeStatement(int, mcp_expr *);
extern mcp_slist *makeArrayDecl(char *id, mcp_expr *expr);
extern mcp_slist *makeNull(void);
extern mcp_slist *makeComment(char *);
extern mcp_slist *makeDirectiveStatement(mcp_directive *);
extern mcp_slist *makeIf(mcp_slist *, mcp_slist *, mcp_slist *);
extern mcp_slist *makeLoop(int,mcp_slist *,mcp_slist *,mcp_slist *,mcp_slist *);
extern mcp_slist *appendStatement(mcp_slist *, mcp_slist *);
extern mcp_slist *makeAssign(mcp_piecelist *, mcp_expr *);
extern mcp_slist *makeSwitch(int, mcp_expr *, mcp_caselist *);

extern mcp_expr *makeNumber(int);
extern mcp_expr *makeObject(char *);
extern mcp_expr *makeString(char *);
extern mcp_expr *makeTop(void);
extern mcp_expr *makeIdent(char *);
extern mcp_expr *makePush(mcp_exprlist *);
extern mcp_expr *makeCall(char *, mcp_exprlist *, char *);
extern mcp_expr *makeIncr(int, char *);
extern mcp_expr *makeOp(int, mcp_expr *, mcp_expr *);
extern mcp_expr *makeArrayRef(char *, mcp_expr *);
extern mcp_exprlist *makeExpr(mcp_expr *);
extern mcp_exprlist *appendExpr(mcp_expr *, mcp_exprlist *);

extern mcp_piecelist *makeId(char *);
extern mcp_piecelist *makeArray(char *, mcp_expr *);
extern mcp_piecelist *appendId(mcp_piecelist *, mcp_piecelist *);
extern mcp_piecelist *makeBlank(void);

extern mcp_caselist *makeCase(mcp_expr *, mcp_slist *);
extern mcp_caselist *appendCase(mcp_caselist *, mcp_caselist *);

%}

%union {
   int val;
   char *string;
   mcp_directive *dir;
   mcp_slist *stmt;
   mcp_expr *expr;
   mcp_exprlist *elist;
   mcp_piecelist *ilist;
   mcp_caselist *clist;
}

%expect 1

%token <val> NUM
%token <string> STR IDENT OBJ COMMENT IFDEF IFNDEF ECHODEF
%token INCLUDE DEFINE UNDEF ENDIF ELSEDEF
%token IF ELSE WHILE DO FOR BREAK CONTINUE RETURN
%token SWITCH CASE DEFAULT SWITCHALL MAKEARRAY
%token TOP PUSH FUNC PUBLIC VAR COMMA BLANK SEMICOLON COLON
%token OPAREN CPAREN OBRACE CBRACE OBRACK CBRACK

%right ASGN
%left  OR 
%left  AND
%left  BITOR
%left  BITXOR
%left  BITAND
%left  EQ NE
%left  GT GTE LT LTE
%left  SHIFTR SHIFTL
%left  ADD SUB
%left  MUL DIV MOD
%right INCR DECR UNARY NOT
%left  CALL

%type <val> program global funcdef opt_pub switch
%type <stmt> cond statement statements assign
%type <dir> directive
%type <expr> expr
%type <elist> arglist
%type <clist> case caselist default
%type <ilist> vardecl varlist

%%
program:
      /* empty */ { $$ = 0; }
    | program global { $$ = 0; }
    | program directive { insertDirective($2); $$ = 0; }
    | program funcdef { $$ = 0; }
    | error { yyerror("top level parse error"); yyerrok; yyclearin; $$ = 0; }
    ;

global:
      COMMENT { insertComment($1); $$ = 0; }
    | VAR vardecl SEMICOLON { insertVar(PIECE_VAR, $2); $$ = 0; }
    | VAR error { yyerror("invalid var declaration"); $$ = 0;  }
    | VAR vardecl error { yyerror("missing ;"); $$ = 0; }
    | DEFINE IDENT statement { insertDefine($2, $3); $$ = 0; }
    | DEFINE error { yyerror("expected identifier"); $$ = 0; }
    | DEFINE IDENT error { yyerror("bogus statement"); $$ = 0; }
    | INCLUDE OBJ { insertDirective(makeDirective(DIR_INCLUDE, $2)); $$ = 0; }
    | INCLUDE error { yyerror("expected an object"); $$ = 0; }
    | UNDEF IDENT { insertDirective(makeDirective(DIR_UNDEF, $2)); $$ = 0; }
    | UNDEF error { yyerror("expected an identifier"); $$ = 0; }
    ;

directive:
      ECHODEF { $$ = makeDirective(DIR_ECHO, $1); }
    | IFDEF { $$ = makeDirective(DIR_IFDEF, $1); }
    | IFNDEF { $$ = makeDirective(DIR_IFNDEF, $1); }
    | ELSEDEF { $$ = makeDirective(DIR_ELSEDEF, NULL); }
    | ENDIF { $$ = makeDirective(DIR_ENDIF, NULL); }
    ;

statement:
      error SEMICOLON { yyerror("invalid statement"); $$ = makeNull(); }
    | OBRACE statements CBRACE { $$ = $2; }
    | OBRACE error { yyerror("bogus statement"); $$ = makeNull(); }
    | OBRACE statements error { yyerror("missing }"); $$ = makeNull(); }
    | expr SEMICOLON { $$ = makeStatement(STMT_EXPR, $1); }
    | expr error { yyerror("missing ;"); $$ = makeNull(); }
    | directive { $$ = makeDirectiveStatement($1); }
    | COMMENT { $$ = makeComment($1); }
    | MAKEARRAY OPAREN IDENT COMMA expr CPAREN SEMICOLON {
          $$ = makeArrayDecl($3, $5);
      }
    | MAKEARRAY error { yyerror("missing ("); $$ = makeNull(); }
    | MAKEARRAY OPAREN error { yyerror("missing ident"); $$ = makeNull(); }
    | MAKEARRAY OPAREN IDENT error { yyerror("missing ,"); $$ = makeNull(); }
    | MAKEARRAY OPAREN IDENT COMMA error {
          yyerror("missing expr");
          $$ = makeNull();
      }
    | MAKEARRAY OPAREN IDENT COMMA expr error {
          yyerror("missing )");
          $$ = makeNull();
      }
    | MAKEARRAY OPAREN IDENT COMMA expr CPAREN error {
          yyerror("missing ;");
          $$ = makeNull();
      }
    | RETURN expr SEMICOLON { $$ = makeStatement(STMT_RETURN, $2); }
    | RETURN SEMICOLON { $$ = makeStatement(STMT_RETURN, NULL); }
    | RETURN error { yyerror("missing ;"); $$ = makeNull(); }
    | BREAK SEMICOLON { $$ = makeStatement(STMT_BREAK, NULL); }
    | BREAK error { yyerror("missing ;"); $$ = makeNull(); }
    | CONTINUE SEMICOLON { $$ = makeStatement(STMT_CONT, NULL); }
    | CONTINUE error { yyerror("missing ;"); $$ = makeNull(); }
    | VAR vardecl SEMICOLON { insertVar(PIECE_LVAR, $2); $$ = makeNull(); }
    | VAR vardecl error { yyerror("missing ;"); $$ = makeNull(); }
    | assign SEMICOLON { $$ = $1; }
    | IF OPAREN cond CPAREN statement { $$ = makeIf($3, $5, NULL); }
    | IF error { yyerror("missing ("); $$ = makeNull(); }
    | IF OPAREN cond error { yyerror("invalid expression"); $$ = makeNull(); }
    | IF OPAREN cond CPAREN error {
          yyerror("bogus statement");
          $$ = makeNull();
      }
    | IF OPAREN cond CPAREN statement ELSE statement {
          $$ = makeIf($3, $5, $7);
      }
    | IF OPAREN cond CPAREN statement ELSE error {
          yyerror("bogus statement");
          $$ = makeNull();
      }
    | DO statement WHILE OPAREN cond CPAREN SEMICOLON {
          $$ = makeLoop(LOOP_DO, NULL, $2, $5, NULL);
      }
    | DO error { yyerror("bogus statement"); $$ = makeNull(); }
    | DO statement error {
          yyerror("expected while");
          $$ = makeNull();
      }
    | DO statement WHILE error {
          yyerror("missing (");
          $$ = makeNull();
      }
    | DO statement WHILE OPAREN cond error {
          yyerror("missing )");
          $$ = makeNull();
      }
    | DO statement WHILE OPAREN cond CPAREN error {
          yyerror("missing ;");
          $$ = makeNull();
      }
    | WHILE OPAREN cond CPAREN statement {
          $$ = makeLoop(LOOP_WHILE, NULL, $5, $3, NULL);
      }
    | WHILE error { yyerror("missing ("); $$ = makeNull(); }
    | WHILE OPAREN cond error { yyerror("missing )"); $$ = makeNull(); }
    | WHILE OPAREN cond CPAREN error {
          yyerror("bogus statement");
          $$ = makeNull();
      }
    | FOR OPAREN cond SEMICOLON cond SEMICOLON cond CPAREN statement {
          $$ = makeLoop(LOOP_FOR, $3, $9, $5, $7);
      }
    | FOR error { yyerror("missing ("); $$ = makeNull(); }
    | FOR OPAREN cond error { yyerror("missing ;"); $$ = makeNull(); }
    | FOR OPAREN cond SEMICOLON cond error {
          yyerror("missing ;");
          $$ = makeNull();
      }
    | FOR OPAREN cond SEMICOLON cond SEMICOLON cond error {
          yyerror("missing )");
          $$ = makeNull();
      }
    | FOR OPAREN cond SEMICOLON cond SEMICOLON cond CPAREN error {
          yyerror("bogus statement");
          $$ = makeNull();
      }
    | switch OPAREN expr CPAREN OBRACE caselist CBRACE {
          mcp_caselist *tmp = $6;
          int had_error = 0;
          if(tmp->e != NULL) {
              tmp = tmp->next;
              while(tmp) {
                 if(tmp->e == NULL) {
                     yyerror("default case must come last");
                     had_error = 1;
                 }
                 tmp = tmp->next;
              }
          }
          if(had_error)
              $$ = makeNull();
          else
              $$ = makeSwitch($1, $3, $6);
      }
    | switch error { yyerror("missing ("); $$ = makeNull(); }
    | switch OPAREN error { yyerror("invalid expression"); $$ = makeNull(); }
    | switch OPAREN expr error { yyerror("missing )"); $$ = makeNull(); }
    | switch OPAREN expr CPAREN error { yyerror("missing {"); $$=makeNull(); }
    | switch OPAREN expr CPAREN OBRACE error {
          yyerror("invalid case");
          $$=makeNull();
      }
    | switch OPAREN expr CPAREN OBRACE caselist error {
          yyerror("missing }");
          $$=makeNull();
      }
    ;

switch:
      SWITCH { $$ = 0; }
    | SWITCHALL { $$ = 1; }
    ;

caselist:
      case { $$ = $1; }
    | caselist case { $$ = appendCase($2, $1); }
    | caselist default { $$ = appendCase($2, $1); }
    ;

case:
    CASE expr COLON statement { $$ = makeCase($2, $4); }
    ;

default:
    DEFAULT COLON statement { $$ = makeCase(NULL, $3); }
    ;

statements:
      statement { $$ = $1; }
    | statements statement { $$ = appendStatement($2, $1); }
    ;

cond:
      /* empty */ { $$ = makeNull(); }
    | expr { $$ = makeStatement(STMT_EXPR, $1); }
    | assign { $$ = $1; }
    ;

assign:
      vardecl ASGN expr {
          if($3)
             $$ = makeAssign($1, $3);
          else {
             yyerror("missing expression in assignment");
             $$ = makeNull();
          }
      }
    ;

expr:
      NUM { $$ = makeNumber($1); }
    | OBJ { $$ = makeObject($1); }
    | STR { $$ = makeString($1); }
    | TOP { $$ = makeTop(); }
    | IDENT { $$ = makeIdent($1); }
    | IDENT OBRACK expr CBRACK { $$ = makeArrayRef($1, $3); }
    | IDENT OBRACK error { yyerror("invalid expression"); $$ = NULL; }
    | IDENT OBRACK expr error { yyerror("missing ]"); $$ = NULL; }
    | IDENT INCR { $$ = makeIncr(EXPR_INC, $1); }
    | IDENT DECR { $$ = makeIncr(EXPR_DEC, $1); }
    | OPAREN expr CPAREN { $$ = $2; }
    | OPAREN expr error { yyerror("expected )"); $$ = NULL; }
    | expr OR expr { $$ = makeOp(OR, $1, $3); }
    | expr AND expr { $$ = makeOp(AND, $1, $3); }
    | expr ADD expr { $$ = makeOp(ADD, $1, $3); }
    | expr SUB expr { $$ = makeOp(SUB, $1, $3); }
    | expr MUL expr { $$ = makeOp(MUL, $1, $3); }
    | expr DIV expr { $$ = makeOp(DIV, $1, $3); }
    | expr MOD expr { $$ = makeOp(MOD, $1, $3); }
    | expr BITAND expr { $$ = makeOp(BITAND, $1, $3); }
    | expr BITOR expr { $$ = makeOp(BITOR, $1, $3); }
    | expr BITXOR expr { $$ = makeOp(BITXOR, $1, $3); }
    | expr SHIFTR expr { $$ = makeOp(SHIFTR, $1, $3); }
    | expr SHIFTL expr { $$ = makeOp(SHIFTL, $1, $3); }
    | expr GT expr { $$ = makeOp(GT, $1, $3); }
    | expr GTE expr { $$ = makeOp(GTE, $1, $3); }
    | expr LT expr { $$ = makeOp(LT, $1, $3); }
    | expr LTE expr { $$ = makeOp(LTE, $1, $3); }
    | expr EQ expr { $$ = makeOp(EQ, $1, $3); }
    | expr NE expr { $$ = makeOp(NE, $1, $3); }
    | SUB expr %prec UNARY { $$ = makeOp(SUB, $2, NULL); }
    | NOT expr { $$ = makeOp(NOT, $2, NULL); }
    | PUSH OPAREN arglist CPAREN  { $$ = makePush($3); }
    | PUSH error { yyerror("missing )"); $$ = NULL; }
    | PUSH OPAREN arglist error { yyerror("missing )"); $$ = NULL; }
    | IDENT OPAREN arglist CPAREN { $$ = makeCall($1, $3, NULL); }
    | IDENT OPAREN arglist error { yyerror("missing )"); $$ = NULL; }
    | OBJ CALL IDENT OPAREN arglist CPAREN { $$ = makeCall($3, $5, $1); }
    | OBJ CALL error { yyerror("expected identifier"); $$ = NULL; }
    | OBJ CALL IDENT error { yyerror("missing )"); $$ = NULL; }
    | OBJ CALL IDENT OPAREN arglist error { yyerror("missing )"); $$ = NULL; }
    ;

opt_pub:
      /* empty */ { $$ = 0; }
    | PUBLIC { $$ = 1; }
    ;

funcdef:
      opt_pub FUNC IDENT OPAREN varlist CPAREN statement {
          insertFunction($3, $5, $7);
          if($1)
              insertPublicDecl($3);
          $$ = 0;
      }
    | opt_pub FUNC error { yyerror("missing identifier"); $$ = 0; };
    | opt_pub FUNC IDENT error { yyerror("missing ("); $$ = 0; };
    | opt_pub FUNC IDENT OPAREN varlist error { yyerror("missing )"); $$ = 0; }
    | opt_pub FUNC IDENT OPAREN varlist CPAREN error {
          yyerror("bogus statement"); $$ = 0;
      }
    ;

arglist:
      /* empty */ { $$ = makeExpr(NULL) }
    | expr { $$ = makeExpr($1); }
    | arglist COMMA expr { $$ = appendExpr($3, $1); }
    ;

vardecl:
      BLANK { $$ = makeBlank() }
    | IDENT { $$ = makeId($1); }
    | IDENT OBRACK expr CBRACK { $$ = makeArray($1, $3); }
    | IDENT OBRACK error { yyerror("invalid expr"); $$ = makeBlank(); }
    | IDENT OBRACK expr error { yyerror("missing ]"); $$ = makeBlank(); }
    | vardecl COMMA IDENT { $$ = appendId(makeId($3), $1); }
    | vardecl COMMA IDENT OBRACK expr CBRACK {
          $$ = appendId(makeArray($3, $5), $1);
      }
    | vardecl COMMA IDENT OBRACK error {
          yyerror("invalid expression");
          $$ = makeBlank();
      }
    | vardecl COMMA IDENT OBRACK expr error {
          yyerror("missing ]");
          $$ = makeBlank();
      }
    | vardecl COMMA BLANK { $$ = appendId(makeBlank(), $1); }
    ;

varlist:
      /* nothing */ { $$ = makeId(NULL) }
    | vardecl { $$ = $1; }
    ;

%%

#ifndef TEST
dbref cur_player;

extern FILE *yyin;
#endif

int yylinenum;
static int parse_error;
static int expected_line;
static int expected_other_error;

#ifndef TEST
int convert (char *infile, char *outfile, dbref player)
#else
int convert(void)
#endif
{
#ifndef TEST
    FILE *outf;

    cur_player = player;
    yyin = fopen(infile, "r");
    if(!yyin) return -1;

    outf = fopen(outfile, "w");
    if(!outf) return -2;
#endif

    yylinenum = 1;
    parse_error = 0;
    expected_line = 0;
    expected_other_error = 0;

    initMcpParser();
    yyparse();
    if(expected_other_error) {
        yylinenum = expected_line;
        yyerror("unexpected parse error");
    }
    if(!parse_error)
#ifdef TEST
        writeMcpProgram(stdout);
#else
        writeMcpProgram(outf);
    fclose(yyin);
    fclose(outf);
#endif
    clearMem();

    return ( (parse_error) ? -3 : 0);
}

void yyerror(char *arg)
{
    char buf[1024];
    if(strcmp(arg, "parse error") == 0) {
        expected_other_error = 1;
        expected_line = yylinenum;
        return;
    }
    sprintf(buf, "(line %d) Error: %s\n", yylinenum, arg);
#ifdef TEST
    fprintf(stderr, "%s",  buf);
#else
    notify(cur_player,  buf);
#endif
    parse_error = 1;
    expected_other_error = 0;
}

void yywarn(char *arg)
{
    char buf[1024];
    sprintf(buf, "(line %d) Warning: %s\n", yylinenum, arg);
#ifdef TEST
    fprintf(stderr, "%s",  buf);
#else
    notify(cur_player, buf);
#endif
}

#ifdef TEST
void main(void)
{
    convert();
}
#endif
