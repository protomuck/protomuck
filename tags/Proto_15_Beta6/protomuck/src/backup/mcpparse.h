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
#define	NUM	258
#define	STR	259
#define	IDENT	260
#define	OBJ	261
#define	COMMENT	262
#define	IFDEF	263
#define	IFNDEF	264
#define	ECHODEF	265
#define	INCLUDE	266
#define	DEFINE	267
#define	UNDEF	268
#define	ENDIF	269
#define	ELSEDEF	270
#define	IF	271
#define	ELSE	272
#define	WHILE	273
#define	DO	274
#define	FOR	275
#define	BREAK	276
#define	CONTINUE	277
#define	RETURN	278
#define	SWITCH	279
#define	CASE	280
#define	DEFAULT	281
#define	SWITCHALL	282
#define	MAKEARRAY	283
#define	TOP	284
#define	PUSH	285
#define	FUNC	286
#define	PUBLIC	287
#define	VAR	288
#define	COMMA	289
#define	BLANK	290
#define	SEMICOLON	291
#define	COLON	292
#define	OPAREN	293
#define	CPAREN	294
#define	OBRACE	295
#define	CBRACE	296
#define	OBRACK	297
#define	CBRACK	298
#define	ASGN	299
#define	OR	300
#define	AND	301
#define	BITOR	302
#define	BITXOR	303
#define	BITAND	304
#define	EQ	305
#define	NE	306
#define	GT	307
#define	GTE	308
#define	LT	309
#define	LTE	310
#define	SHIFTR	311
#define	SHIFTL	312
#define	ADD	313
#define	SUB	314
#define	MUL	315
#define	DIV	316
#define	MOD	317
#define	INCR	318
#define	DECR	319
#define	UNARY	320
#define	NOT	321
#define	CALL	322


extern YYSTYPE yylval;
