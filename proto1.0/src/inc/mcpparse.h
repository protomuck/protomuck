typedef union {
   int val;
   char *string;
   mcp_directive *dir;
   mcp_slist *stmt;
   mcp_expr *expr;
   mcp_exprlist *elist;
   mcp_piecelist *ilist;
   mcp_caselist *clist;
} YYSTYPE;
#define	NUM	257
#define	STR	258
#define	IDENT	259
#define	OBJ	260
#define	COMMENT	261
#define	IFDEF	262
#define	IFNDEF	263
#define	ECHODEF	264
#define	INCLUDE	265
#define	DEFINE	266
#define	UNDEF	267
#define	ENDIF	268
#define	ELSEDEF	269
#define	IF	270
#define	ELSE	271
#define	WHILE	272
#define	DO	273
#define	FOR	274
#define	BREAK	275
#define	CONTINUE	276
#define	RETURN	277
#define	SWITCH	278
#define	CASE	279
#define	DEFAULT	280
#define	SWITCHALL	281
#define	MAKEARRAY	282
#define	TOP	283
#define	PUSH	284
#define	FUNC	285
#define	PUBLIC	286
#define	VAR	287
#define	COMMA	288
#define	BLANK	289
#define	SEMICOLON	290
#define	COLON	291
#define	OPAREN	292
#define	CPAREN	293
#define	OBRACE	294
#define	CBRACE	295
#define	OBRACK	296
#define	CBRACK	297
#define	ASGN	298
#define	OR	299
#define	AND	300
#define	BITOR	301
#define	BITXOR	302
#define	BITAND	303
#define	EQ	304
#define	NE	305
#define	GT	306
#define	GTE	307
#define	LT	308
#define	LTE	309
#define	SHIFTR	310
#define	SHIFTL	311
#define	ADD	312
#define	SUB	313
#define	MUL	314
#define	DIV	315
#define	MOD	316
#define	INCR	317
#define	DECR	318
#define	UNARY	319
#define	NOT	320
#define	CALL	321


extern YYSTYPE yylval;
