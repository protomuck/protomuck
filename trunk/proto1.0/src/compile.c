/*
 *  Compile.c   (This is really a tokenizer, not a compiler)
 */

#include "copyright.h"
#include "config.h"

#include "db.h"
#include "props.h"
#include "interface.h"
#include "inst.h"
#include "externs.h"
#include "params.h"
#include "tune.h"
#include "match.h"
#include "interp.h"
#include <ctype.h>
#include <time.h>

/* This file contains code for doing "byte-compilation" of
   mud-forth programs.  As such, it contains many internal
   data structures and other such which are not found in other
   parts of TinyMUCK.                                       */

/* The IF_STACK is a stack for holding previous IF statements.
   Everytime a THEN is encountered, the next address is inserted
   into the code before the most recent IF.  */

#define CTYPE_IF    1
#define CTYPE_ELSE  2
#define CTYPE_BEGIN 3
#define CTYPE_FOR   4
#define CTYPE_WHILE 5

/* These would be constants, but their value isn't known until runtime. */
static int IN_FORITER;
static int IN_FOREACH;
static int IN_FORPOP;
static int IN_FOR;

static hash_tab primitive_list[COMP_HASH_SIZE];

struct IF_STACK {
    short   type;
    struct INTERMEDIATE *place;
    struct IF_STACK *next;
}      *if_stack, *loop_stack;

/* This structure is an association list that contains both a procedure
   name and the place in the code that it belongs.  A lookup to the procedure
   will see both it's name and it's number and so we can generate a
   reference to it.  Since I want to disallow co-recursion,  I will not allow
   forward referencing.
   */

struct PROC_LIST {
    const char *name;
    struct INTERMEDIATE *code;
    struct PROC_LIST *next;
}      *procs;

/* The intermediate code is code generated as a linked list
   when there is no hint or notion of how much code there
   will be, and to help resolve all references.
   There is always a pointer to the current word that is
   being compiled kept.                                   */

struct INTERMEDIATE {
    int     no;			/* which number instruction this is */
    struct inst in;		/* instruction itself */
    short   line;		/* line number of instruction */
    struct INTERMEDIATE *next;	/* next instruction */
};

static int nowords;		/* number of words compiled */
static struct INTERMEDIATE *curr_word;	/* word currently being compiled */
static struct INTERMEDIATE *first_word;	/* first word of the list */
static struct INTERMEDIATE *curr_proc;	/* first word of current proc. */
static struct INTERMEDIATE *curr_decl;	/* DECLVAR for current proc. */
static struct publics *currpubs;
static int nested_fors;

/*
 *static struct macrotable *defstop;
 */

/* variable names.  The index into variables give you what position
 * the variable holds.
 */
static const char *variables[MAX_VAR];
static const char *localvars[MAX_VAR] = {"ME", "LOC", "TRIGGER", "COMMAND"};
static const char *scopedvars[MAX_VAR];

static struct line *curr_line;	/* current line */
static int lineno;		/* current line number */
static int cstatdescr;             /* descriptor that initialized compiler */
static const char *next_char;	/* next char * */
static dbref player, program;	/* globalized player and program */

/* 1 if error occured */
static int compile_err;

int     primitive(const char *s);	/* returns primitive_number if
					 * primitive */
void    free_prog(struct inst *, int);
const char *next_token(void);
const char *next_token_raw(void);
struct INTERMEDIATE *next_word(const char *);
struct INTERMEDIATE *process_special(const char *);
struct INTERMEDIATE *primitive_word(const char *);
struct INTERMEDIATE *string_word(const char *);
struct INTERMEDIATE *float_word(const char *);
struct INTERMEDIATE *number_word(const char *);
struct INTERMEDIATE *object_word(const char *);
struct INTERMEDIATE *quoted_word(const char *);
struct INTERMEDIATE *call_word(const char *);
struct INTERMEDIATE *var_word(const char *);
struct INTERMEDIATE *lvar_word(const char *);
struct INTERMEDIATE *svar_word(const char *);
const char *do_string(void);
void    do_comment(void);
void    do_directive(char *direct);
struct prog_addr *alloc_addr(int, struct inst *);
struct INTERMEDIATE *new_inst(void);
struct INTERMEDIATE *locate_if(void);
struct INTERMEDIATE *find_if(void);
struct INTERMEDIATE *find_else(void);
struct INTERMEDIATE *locate_begin(void);
struct INTERMEDIATE *locate_for(void);
struct INTERMEDIATE *find_begin(void);
struct INTERMEDIATE *find_while(void);
void    cleanpubs(struct publics * mypub);
void    cleanup(void);
void    add_proc(const char *, struct INTERMEDIATE *);
void    addif(struct INTERMEDIATE *);
void    addelse(struct INTERMEDIATE *);
void    addbegin(struct INTERMEDIATE *);
void    addfor(struct INTERMEDIATE *);
void    addwhile(struct INTERMEDIATE *);
void    resolve_loop_addrs(int where);
int     add_variable(const char *);
int     add_localvar(const char *);
int     add_scopedvar(const char *);
int     special(const char *);
int     call(const char *);
int     quoted(const char *);
int     object(const char *);
int     string(const char *);
int     variable(const char *);
int     localvar(const char *);
int     scopedvar(const char *);
int     get_primitive(const char *);
void    copy_program(void);
void    set_start(void);
char   *line_copy = NULL;
int     macrosubs;		/* Safeguard for macro-subst. infinite loops */

/* Character defines */
#define BEGINCOMMENT '('
#define ENDCOMMENT ')'
#define BEGINSTRING '"'
#define ENDSTRING '"'
#define BEGINMACRO '.'
#define BEGINDIRECTIVE '$'
#define BEGINESCAPE '\\'

#define SUBSTITUTIONS 50	/* How many nested macros will we allow? */

void
do_abort_compile(const char *c)
{
    static char _buf[BUFFER_LEN];

    sprintf(_buf, "Error in line %d: %s", lineno, c);
    if (line_copy) {
	free((void *) line_copy);
	line_copy = NULL;
    }
    if ((FLAGS(player) & INTERACTIVE) && !(FLAGS(player) & READMODE)) {
	notify_nolisten(player, _buf, 1);
    } else {
	log_muf("MUF compiler warning in program %d:\n%s\n",
		(int) program, _buf);
    }
    compile_err++;
    if (compile_err > 1) {
       return;
    }
    cleanup();
    cleanpubs(currpubs);
    currpubs = NULL;
    free_prog(DBFETCH(program)->sp.program.code,
	      DBFETCH(program)->sp.program.siz);
    cleanpubs(DBFETCH(program)->sp.program.pubs);
    DBFETCH(program)->sp.program.pubs = NULL;
}

/* abort compile macro */
#define abort_compile(C) { do_abort_compile(C); return 0; }

/* abort compile for void functions */
#define v_abort_compile(C) { do_abort_compile(C); return; }


void
fixpubs(struct publics * mypubs, struct inst * offset)
{
    while (mypubs) {
	mypubs->addr.ptr = offset + mypubs->addr.no;
	mypubs = mypubs->next;
    }
}


int
size_pubs(struct publics *mypubs)
{
    int bytes = 0;
    while (mypubs) {
	bytes += sizeof(*mypubs);
	mypubs = mypubs->next;
    }
    return bytes;
}



static hash_tab defhash[DEFHASHSIZE];

char   *
expand_def(const char *defname)
{
    hash_data *exp = find_hash(defname, defhash, DEFHASHSIZE);

    if (!exp) {
	if (*defname == BEGINMACRO) {
	    return ( macro_expansion(macrotop, &defname[1]));
	} else {
	    return (NULL);
	}
    }
    return (string_dup((char *) exp->pval));
}


void
kill_def(const char *defname)
{
    hash_data *exp = find_hash(defname, defhash, DEFHASHSIZE);

    if (exp) {
	free(exp->pval);
	(void) free_hash(defname, defhash, DEFHASHSIZE);
    }
}


void
insert_def(const char *defname, const char *deff)
{
    hash_data hd;

    (void) kill_def(defname);
    hd.pval = (void *) string_dup(deff);
    (void) add_hash(defname, hd, defhash, DEFHASHSIZE);
}


void
insert_intdef(const char *defname, int deff)
{
    char buf[BUFFER_LEN];
    sprintf(buf, "%d", deff);
    insert_def(defname, buf);
}


void
purge_defs(void)
{
    kill_hash(defhash, DEFHASHSIZE, 1);
}


void
include_defs(dbref i)
{
    char    dirname[BUFFER_LEN];
    char    temp[BUFFER_LEN];
    const char *tmpptr;
    PropPtr j, pptr;

    strcpy(dirname, "/_defs/");
    j = first_prop(i, dirname, &pptr, temp);
    if ((int)j == -1)
	return;
    while (j) {
	strcpy(dirname, "/_defs/");
	strcat(dirname, temp);
	tmpptr = uncompress(get_property_class(i, dirname));
	if (tmpptr && *tmpptr)
	    insert_def(temp, (char *) tmpptr);
	j = next_prop(pptr, j, temp);
    }
}


void
init_defs(void)
{
    char buf [1024];
    /* Create standard server defines */
    sprintf(buf, "%s", VERSION);
    insert_def("__version", buf);
    sprintf(buf, "%s", tp_muckname);
    insert_def("__muckname", buf);
    sprintf(buf, "%s", NEONVER);
    insert_def("__neon", NEONVER);
    sprintf(buf, "%s", PROTOVER);
    insert_def("__proto", buf);
    insert_def("strip", "striplead striptail");
    insert_def("instring", "tolower swap tolower swap instr");
    insert_def("rinstring", "tolower swap tolower swap rinstr");
    insert_def("stripspaces", "strip begin dup \"  \" instr while \" \" \"  \" subst repeat");
#ifndef OLDPARSE
    insert_def("parseprop", "4 pick 4 rotate getpropstr rot rot parsempi");
#endif
    insert_def("++", "dup variable? if dup @ 1 + swap ! else 1 + then");
    insert_def("--", "dup variable? if dup @ 1 - swap ! else 1 - then");
    insert_intdef("bg_mode", BACKGROUND);
    insert_intdef("fg_mode", FOREGROUND);
    insert_intdef("pr_mode", PREEMPT);
    insert_intdef("max_variable_count", MAX_VAR);

    /* make defines for compatability to removed primitives */
    insert_def("desc", "\"_/de\" getpropstr");
    insert_def("idesc", "\"_/ide\" getpropstr");
    insert_def("ansidesc", "\"_/anside\" getpropstr");
    insert_def("iansidesc", "\"_/ianside\" getpropstr");
    insert_def("htmldesc", "\"_/htmlde\" getpropstr");
    insert_def("ihtmldesc", "\"_/ihtmlde\" getpropstr");
    insert_def("succ", "\"_/sc\" getpropstr");
    insert_def("fail", "\"_/fl\" getpropstr");
    insert_def("drop", "\"_/dr\" getpropstr");
    insert_def("osucc", "\"_/osc\" getpropstr");
    insert_def("ofail", "\"_/ofl\" getpropstr");
    insert_def("odrop", "\"_/odr\" getpropstr");
    insert_def("setdesc", "\"_/de\" swap 0 addprop");
    insert_def("setidesc", "\"_/ide\" swap 0 addprop");
    insert_def("setansidesc", "\"_/anside\" swap 0 addprop");
    insert_def("setiansidesc", "\"_/ianside\" swap 0 addprop");
    insert_def("sethtmldesc", "\"_/htmlde\" swap 0 addprop");
    insert_def("setihtmldesc", "\"_/ihtmlde\" swap 0 addprop");
    insert_def("setsucc", "\"_/sc\" swap 0 addprop");
    insert_def("setfail", "\"_/fl\" swap 0 addprop");
    insert_def("setdrop", "\"_/dr\" swap 0 addprop");
    insert_def("setosucc", "\"_/osc\" swap 0 addprop");
    insert_def("setofail", "\"_/ofl\" swap 0 addprop");
    insert_def("setodrop", "\"_/odr\" swap 0 addprop");
    insert_def("preempt", "pr_mode setmode");
    insert_def("background", "bg_mode setmode");
    insert_def("foreground", "fg_mode setmode");
    insert_def("notify_except", "1 swap notify_exclude");
    insert_def("ansi_notify_except", "1 swap ansi_notify_exclude");
    insert_def("notify_html_except", "1 swap notify_html_exclude");
    insert_def("html_nocr_except", "1 swap notify_html_exclude_nocr");
    insert_def("case", "begin dup");
    insert_def("when", "if pop");
    insert_def("end", "break then dup");
    insert_def("default", "pop 1 if");
    insert_def("endcase", "pop pop 1 until");
    insert_def("}list", "} array_make");
    insert_def("}dict", "} 2 / array_make_dict");
    insert_def("err_divzero?", "0 is_set?");
    insert_def("err_nan?", "1 is_set?");
    insert_def("err_imaginary?", "2 is_set?");
    insert_def("err_fbounds?", "3 is_set?");
    insert_def("err_ibounds?", "4 is_set?");
    insert_def("[]", "array_getitem");
    insert_def("[..]", "array_getrange");
    insert_def("array_diff", "2 array_ndiff");
    insert_def("array_union", "2 array_nunion");
    insert_def("array_intersect", "2 array_nintersect");
    insert_def("sockopen", "nbsockopen \"Invalid host.\" over strcmp if pop 1 10 1 for 10 = if \"timed out\" break then dup sockcheck if \"noerr\" break then 1 sleep repeat then");
    /* include any defines set in #0's _defs/ propdir. */
    include_defs((dbref) 0);

    /* include any defines set in program owner's _defs/ propdir. */
    include_defs(OWNER(program));
}

void
uncompile_program(dbref i)
{
    /* free program */
    (void) dequeue_prog(i, 1);
    program = i;
    free_prog(DBFETCH(i)->sp.program.code, DBFETCH(i)->sp.program.siz);
    cleanpubs(DBFETCH(i)->sp.program.pubs);
    DBFETCH(i)->sp.program.pubs = NULL;
    DBFETCH(i)->sp.program.code = NULL;
    DBFETCH(i)->sp.program.siz = 0;
    DBFETCH(i)->sp.program.start = NULL;
}


void
do_uncompile(dbref player)
{
    dbref   i;

    if (!Arch(OWNER(player))) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }
    for (i = 0; i < db_top; i++) {
	if (Typeof(i) == TYPE_PROGRAM) {
	    uncompile_program(i);
	}
    }
    anotify_nolisten(player, CSUCC "All programs decompiled.", 1);
}

void
do_proginfo(dbref player, const char *arg)
{
    char buf[BUFFER_LEN];
    dbref thing, i;
    time_t now = time(NULL);
    int tcnt=0, tsize=0, tinst=0, timem=0, ccnt, csize, cinst, cimem;

    if( *arg != '#' ) {
	anotify_nolisten2(player, YELLOW "Usage: @proginfo #prognum");
	if (Mage(OWNER(player))) {
	    anotify_nolisten2(player, AQUA "Inst Object ProgSz Insts " BROWN "Name");
	    for(i=0;i<db_top;i++) {
		if(Typeof(i) == TYPE_PROGRAM && DBFETCH(i)->sp.program.siz) {
		    tcnt  += ccnt  = DBFETCH(i)->sp.program.instances;
		    tsize += csize = size_object(i, 0);
		    timem += cimem = size_prog(i);
		    tinst += cinst = DBFETCH(i)->sp.program.siz;
		    sprintf(buf, CYAN "%4d %6d %6d %5d %s " RED "by " GREEN "%s",
			ccnt, csize, cimem, cinst,
			ansi_unparse_object(player, i), NAME(OWNER(i))
		    );
		    anotify_nolisten2(player, buf);
		}
	    }
	    sprintf(buf, AQUA "%4d %6d %6d %5d " BROWN "Total",
			tcnt, tsize, timem, tinst);
	    anotify_nolisten2(player, buf );
	}
	return;
    }

    thing = (dbref) atoi(arg+1);
    if( Typeof(thing) != TYPE_PROGRAM || !controls(player,thing)) {
	anotify_nolisten2(player, RED NOPERM_MESG);
	return;
    }

    anotify_nolisten2(player, YELLOW "1 = cleanable, 0 = used recently");
    sprintf( buf, "AI: %d Age: %d Instances: %d==0?",
	!(FLAGS(thing) & (ABODE|INTERNAL)),
	(now - DBFETCH(thing)->ts.lastused) > tp_clean_interval,
	DBFETCH(thing)->sp.program.instances
    );
    notify(player, buf);
}

void
free_unused_programs()
{
    dbref i;
    time_t now = time(NULL);

    for (i = 0; i < db_top; i++) {
	if ( Typeof(i) == TYPE_PROGRAM )
	if ( !(FLAGS(i) & (ABODE|INTERNAL)) )
	if ( (now - DBFETCH(i)->ts.lastused) > tp_clean_interval ) 
	if ( DBFETCH(i)->sp.program.instances == 0 )
	{
	    uncompile_program(i);
	}
    }
}


/* overall control code.  Does piece-meal tokenization parsing and
   backward checking.                                            */
void
do_compile(int descr, dbref player_in, dbref program_in)
{
    const char *token;
    struct INTERMEDIATE *new_word;
    int i;

/*    for (i = 0; i < MAX_VAR; i++) {
	variables[i] = NULL;
	localvars[i] = NULL;
	scopedvars[i] = NULL;
    }

    variables[0] = "ME";
    variables[1] = "LOC";
    variables[2] = "TRIGGER";
    variables[3] = "COMMAND"; */
    cstatdescr = descr;
    /* set all global variables */
    nowords = 0;
    curr_word = first_word = curr_proc = 0;
    curr_proc = NULL;
    currpubs = NULL;
    nested_fors = 0;
    player = player_in;
    program = program_in;
    init_defs();
    if_stack = 0;
    loop_stack = 0;
    lineno = 1;
    curr_line = DBFETCH(program)->sp.program.first;
    if (curr_line)
	next_char = curr_line->this_line;
    first_word = curr_word = NULL;
    procs = 0;
    compile_err = 0;
    /* free old stuff */
    (void) dequeue_prog(program, 1);
    free_prog(DBFETCH(program)->sp.program.code, DBFETCH(program)->sp.program.siz);
    cleanpubs(DBFETCH(program)->sp.program.pubs);
    DBFETCH(program)->sp.program.pubs = NULL;

    if (!curr_line)
	v_abort_compile("Missing program text.");

    /* do compilation */
    while ((token = next_token())) {
	new_word = next_word(token);

	/* test for errors */
	if (compile_err)
	    return;

	if (new_word) {
	    if (!first_word)
		first_word = curr_word = new_word;
	    else {
		curr_word->next = new_word;
		curr_word = curr_word->next;
	    }
	}
	while (curr_word && curr_word->next)
	    curr_word = curr_word->next;

	free((void *) token);
    }

    if (curr_proc)
	v_abort_compile("Unexpected end of file.");

    if (!procs)
	v_abort_compile("Missing procedure definition.");

    /* do copying over */
    copy_program();
    fixpubs(currpubs, DBFETCH(program)->sp.program.code);
    DBFETCH(program)->sp.program.pubs = currpubs;

    if (compile_err)
	return;

    set_start();
    cleanup();

    /* restart AUTOSTART program. */
    if ((FLAGS(program) & ABODE) && TMage(OWNER(program)))
	add_muf_queue_event(descr, OWNER(program), NOTHING, NOTHING, program,
			    "Startup", "Queued Event.", 0);
}

struct INTERMEDIATE *
next_word(const char *token)
{
    struct INTERMEDIATE *new_word;
    static char buf[BUFFER_LEN];

    if (!token)
	return 0;

    if (call(token))
	new_word = call_word(token);
    else if (localvar(token))
	new_word = lvar_word(token);
    else if (scopedvar(token))
	new_word = svar_word(token);
    else if (variable(token))
	new_word = var_word(token);
    else if (special(token))
	new_word = process_special(token);
    else if (primitive(token))
	new_word = primitive_word(token);
    else if (string(token))
	new_word = string_word(token + 1);
    else if (number(token))
	new_word = number_word(token);
    else if (ifloat(token))
	new_word = float_word(token);
    else if (object(token))
	new_word = object_word(token);
    else if (quoted(token))
	new_word = quoted_word(token + 1);
    else {
	sprintf(buf, "Unrecognized word %s.", token);
	abort_compile(buf);
    }
    return new_word;
}



/* Little routine to do the line_copy handling right */
void
advance_line()
{
    curr_line = curr_line->next;
    lineno++;
    macrosubs = 0;
    if (line_copy) {
	free((void *) line_copy);
	line_copy = NULL;
    }
    if (curr_line)
	next_char = (line_copy = alloc_string(curr_line->this_line));
    else
	next_char = (line_copy = NULL);
}

/* Skips comments, grabs strings, returns NULL when no more tokens to grab. */
const char *
next_token_raw()
{
    static char buf[BUFFER_LEN];
    int     i;

    if (!curr_line)
	return (char *) 0;

    if (!next_char)
	return (char *) 0;

    /* skip white space */
    while (*next_char && isspace(*next_char))
	next_char++;

    if (!(*next_char)) {
	advance_line();
	return next_token_raw();
    }
    /* take care of comments */
    if (*next_char == BEGINCOMMENT) {
	do_comment();
	return next_token_raw();
    }
    if (*next_char == BEGINSTRING)
	return do_string();

    for (i = 0; *next_char && !isspace(*next_char); i++) {
	buf[i] = *next_char;
	next_char++;
    }
    buf[i] = '\0';
    return alloc_string(buf);
}


const char *
next_token()
{
    char *expansion, *temp;

    temp = (char *) next_token_raw();
    if (!temp)
	return NULL;

    if (temp[0] == BEGINDIRECTIVE) {
	do_directive(temp);
	free(temp);
	return next_token();
    }
    if (temp[0] == BEGINESCAPE) {
	if (temp[1]) {
            /* return (++temp) */
            expansion = temp;
            temp = (char *)malloc(strlen(expansion));
            strcpy(temp, (expansion+1));
            free(expansion);
	}
	return (temp);
    }
    if ((expansion = expand_def(temp))) {
	free(temp);
	if (++macrosubs > SUBSTITUTIONS) {
	    abort_compile("Too many macro substitutions.");
	} else {
	    temp = (char *) malloc(strlen(next_char) + strlen(expansion) + 21);
	    strcpy(temp, expansion);
	    strcat(temp, next_char);
	    free((void *) expansion);
	    if (line_copy) {
		free((void *) line_copy);
	    }
	    next_char = line_copy = temp;
	    return next_token();
	}
    } else {
	return (temp);
    }
}



/* skip comments */
void
do_comment()
{
    while (*next_char && *next_char != ENDCOMMENT)
	next_char++;
    if (!(*next_char)) {
	advance_line();
	if (!curr_line) {
	    v_abort_compile("Unterminated comment.");
	}
	do_comment();
    } else {
	next_char++;
	if (!(*next_char))
	    advance_line();
    }
}

/* handle compiler directives */
void
do_directive(char *direct)
{
    char    temp[BUFFER_LEN];
    char   *tmpname;
    char   *tmpptr;
    int     i = 0;
    int     j;

    strcpy(temp, ++direct);

    if (!(temp[0])) {
	v_abort_compile("I don't understand that compiler directive!");
    }
    if (!string_compare(temp, "define")) {
	tmpname = (char *) next_token_raw();
	if (!tmpname)
	    v_abort_compile("Unexpected end of file looking for $define name.");
	i = 0;
	while ((tmpptr = (char *) next_token_raw()) &&
		(string_compare(tmpptr, "$enddef"))) {
	    char *cp;
	    for (cp = tmpptr; i < (BUFFER_LEN / 2) && *cp;) {
		if (*tmpptr == BEGINSTRING && cp != tmpptr &&
			(*cp == ENDSTRING || *cp == BEGINESCAPE)) {
		    temp[i++] = BEGINESCAPE;
		}
		temp[i++] = *cp++;
	    }
	    if (*tmpptr == BEGINSTRING)
		temp[i++] = ENDSTRING;
	    temp[i++] = ' ';
	    free(tmpptr);
	    if (i > (BUFFER_LEN / 2))
		v_abort_compile("$define definition too long.");
	}
	if (i) i--;
	temp[i] = '\0';
	if (!tmpptr)
	    v_abort_compile("Unexpected end of file in $define definition.");
	free(tmpptr);
	(void) insert_def(tmpname, temp);
	free(tmpname);

    } else if (!string_compare(temp, "enddef")) {
	v_abort_compile("$enddef without a previous matching $define.");

    } else if (!string_compare(temp, "def")) {
	tmpname = (char *) next_token_raw();
	if (!tmpname)
	    v_abort_compile("Unexpected end of file looking for $def name.");
	(void) insert_def(tmpname, next_char);
	while (*next_char)
	    next_char++;
	advance_line();
	free(tmpname);

    } else if (!string_compare(temp, "include")) {
	struct match_data md;

	tmpname = (char *) next_token_raw();
	if (!tmpname)
	    v_abort_compile("Unexpected end of file while doing $include.");
	{
	    char    tempa[BUFFER_LEN], tempb[BUFFER_LEN];

	    strcpy(tempa, match_args);
	    strcpy(tempb, match_cmdname);
	    init_match(cstatdescr, player, tmpname, NOTYPE, &md);
	    match_registered(&md);
	    match_absolute(&md);
	    match_me(&md);
	    i = (int) match_result(&md);
	    strcpy(match_args, tempa);
	    strcpy(match_cmdname, tempb);
	}
	free(tmpname);
	if (((dbref) i == NOTHING) || (i < 0) || (i > db_top)
		|| (Typeof(i) == TYPE_GARBAGE))
	    v_abort_compile("I don't understand what object you want to $include.");
	include_defs((dbref) i);

    } else if (!string_compare(temp, "undef")) {
	tmpname = (char *) next_token_raw();
	if (!tmpname)
	    v_abort_compile("Unexpected end of file looking for name to $undef.");
	kill_def(tmpname);
	free(tmpname);

    } else if (!string_compare(temp, "echo")) {
	notify_nolisten(player, next_char, 1);
	while (*next_char) next_char++;
	advance_line();

    } else if (!string_compare(temp, "ifdef")) {
	tmpname = (char *) next_token_raw();
	if (!tmpname)
	    v_abort_compile("Unexpected end of file looking for $ifdef condition.");
	strcpy(temp, tmpname);
	free(tmpname);
	for (i = 1; temp[i] && (temp[i] != '=') && (temp[i] != '>') && (temp[i] != '<'); i++);
	tmpname = &(temp[i]);
	i = (temp[i] == '>') ? 1 : ((temp[i] == '=') ? 0 : ((temp[i] == '<') ? -1 : -2));
	*tmpname = '\0';
	tmpname++;
	tmpptr = (char *) expand_def(temp);
	if (i == -2) {
	    j = (!tmpptr);
	    if (tmpptr)
		free(tmpptr);
	} else {
	    if (!tmpptr) {
		j = 1;
	    } else {
		j = string_compare(tmpptr, tmpname);
		j = !((!i && !j) || ((i * j) > 0));
		free(tmpptr);
	    }
	}
	if (j) {
	    i = 0;
	    while ((tmpptr = (char *) next_token_raw()) &&
		    (i || ((string_compare(tmpptr, "$else"))
			   && (string_compare(tmpptr, "$endif"))))) {
		if (!string_compare(tmpptr, "$ifdef"))
		    i++;
		else if (!string_compare(tmpptr, "$ifndef"))
		    i++;
		else if (!string_compare(tmpptr, "$endif"))
		    i--;
	    }
	    if (!tmpptr) {
		v_abort_compile("Unexpected end of file in $ifdef clause.");
	    }
	    free(tmpptr);
	}
    } else if (!string_compare(temp, "ifndef")) {
	tmpname = (char *) next_token_raw();
	if (!tmpname) {
	    v_abort_compile("Unexpected end of file looking for $ifndef condition.");
	}
	strcpy(temp, tmpname);
	free(tmpname);
	for (i = 1; temp[i] && (temp[i] != '=') && (temp[i] != '>') && (temp[i] != '<'); i++);
	tmpname = &(temp[i]);
	i = (temp[i] == '>') ? 1 : ((temp[i] == '=') ? 0 : ((temp[i] == '<') ? -1 : -2));
	*tmpname = '\0';
	tmpname++;
	tmpptr = (char *) expand_def(temp);
	if (i == -2) {
	    j = (!tmpptr);
	    if (tmpptr)
		free(tmpptr);
	} else {
	    if (!tmpptr) {
		j = 1;
	    } else {
		j = string_compare(tmpptr, tmpname);
		j = !((!i && !j) || ((i * j) > 0));
		free(tmpptr);
	    }
	}
	if (!j) {
	    i = 0;
	    while ((tmpptr = (char *) next_token_raw()) &&
		    (i || ((string_compare(tmpptr, "$else"))
			   && (string_compare(tmpptr, "$endif"))))) {
		if (!string_compare(tmpptr, "$ifdef"))
		    i++;
		else if (!string_compare(tmpptr, "$ifndef"))
		    i++;
		else if (!string_compare(tmpptr, "$endif"))
		    i--;
	    }
	    if (!tmpptr) {
		v_abort_compile("Unexpected end of file in $ifndef clause.");
	    }
	    free(tmpptr);
	}
    } else if (!string_compare(temp, "else")) {
	i = 0;
	while ((tmpptr = (char *) next_token_raw()) &&
		(i || (string_compare(tmpptr, "$endif")))) {
	    if (!string_compare(tmpptr, "$ifdef"))
		i++;
	    else if (!string_compare(tmpptr, "$ifndef"))
		i++;
	    else if (!string_compare(tmpptr, "$endif"))
		i--;
	    free(tmpptr);
	}
	if (!tmpptr) {
	    v_abort_compile("Unexpected end of file in $else clause.");
	}
	free(tmpptr);

    } else if (!string_compare(temp, "endif")) {

    } else {
	v_abort_compile("Unrecognized compiler directive.");
    }
}


/* return string */
const char *
do_string()
{
    static char buf[BUFFER_LEN];
    int     i = 0, quoted = 0;

    buf[i] = *next_char;
    next_char++;
    i++;
    while ((quoted || *next_char != ENDSTRING) && *next_char) {
	if (*next_char == '\\' && !quoted) {
	    quoted++;
	    next_char++;
	} else if (*next_char == 'r' && quoted) {
	    buf[i++] = '\r';
	    next_char++;
	    quoted = 0;
	} else if (*next_char == '[' && quoted) {
	    buf[i++] = ESCAPE_CHAR;
	    next_char++;
  	    quoted = 0;
	} else {
	    buf[i] = *next_char;
	    i++;
	    next_char++;
	    quoted = 0;
	}
    }
    if (!*next_char) {
	abort_compile("Unterminated string found at end of line.");
    }
    next_char++;
    buf[i] = '\0';
    return alloc_string(buf);
}



/* process special.  Performs special processing.
   It sets up FOR and IF structures.  Remember --- for those,
   we've got to set aside an extra argument space.         */
struct INTERMEDIATE *
process_special(const char *token)
{
    static char buf[BUFFER_LEN];
    const char *tok;
    struct INTERMEDIATE *new;

    if (!string_compare(token, ":")) {
	const char *proc_name;
	struct INTERMEDIATE *tmpinst, *new2;

	if (curr_proc)
	    abort_compile("Definition within definition.");
	proc_name = next_token();
	if (!proc_name)
	    abort_compile("Unexpected end of file within procedure.");
	new = new_inst();
	new->no = nowords++;
	new->in.type = PROG_FUNCTION;
	new->in.line = lineno;
	new->in.data.string = alloc_prog_string(proc_name);

/*	tok = next_token();
	tmpinst = next_word(tok);
	if (tok)
	    free((void *) tok);
	if (!tmpinst) {
	    sprintf(buf, "Error in definition of %s.", proc_name);
	    abort_compile(buf);
	} */

	new2 = (new->next = new_inst());
	new2->no = nowords++;
	new2->in.type = PROG_DECLVAR;
	new2->in.line = lineno;
	new2->in.data.number = 0;

/*	new->next = tmpinst; */
	curr_decl = new2;
	curr_proc = new;

	add_proc(proc_name, new);
	if (proc_name)
	    free((void *) proc_name);
	return new;
    } else if (!string_compare(token, ";")) {
	int i;

	if (if_stack || loop_stack)
	    abort_compile("Unexpected end of procedure definition.");
	if (!curr_proc)
	    abort_compile("Procedure end without body.");
	curr_proc = 0;
	new = new_inst();
	new->no = nowords++;
	new->in.type = PROG_PRIMITIVE;
	new->in.line = lineno;
	new->in.data.number = IN_RET;
	for (i = 0; i < MAX_VAR && scopedvars[i]; i++) {
		free((void *) scopedvars[i]);
		scopedvars[i] = 0;
	}
	return new;
    } else if (!string_compare(token, "IF")) {

	new = new_inst();
	new->no = nowords++;
	new->in.type = PROG_IF;
	new->in.line = lineno;
	new->in.data.call = 0;
	addif(new);
	return new;
    } else if (!string_compare(token, "ELSE")) {
	struct INTERMEDIATE *eef;
	struct INTERMEDIATE *after;

	eef = find_if();
	if (!eef)
	    abort_compile("ELSE without IF.");
	new = new_inst();
	new->no = nowords++;
	new->in.type = PROG_JMP;
	new->in.line = lineno;
	new->in.data.call = 0;
	addelse(new);
	tok = next_token();
	new->next = after = next_word(tok);
	if (tok)
	    free((void *) tok);
	if (!after)
	    abort_compile("Unexpected end of program.");
	eef->in.data.number = after->no;
	return new;
    } else if (!string_compare(token, "THEN")) {
	/* can't use 'if' because it's a reserved word */
	struct INTERMEDIATE *eef;

	eef = find_else();
	if (!eef)
	    abort_compile("THEN without IF.");
	tok = next_token();
	new = next_word(tok);
	if (tok)
	    free((void *) tok);
	if (!new)
	    abort_compile("Unexpected end of program.");
	eef->in.data.number = new->no;
	return new;
    } else if (!string_compare(token, "BEGIN")) {
	tok = next_token();
	new = next_word(tok);
	if (tok)
	    free((void *) tok);
	if (!new)
	    abort_compile("Unexpected end of program.");
	addbegin(new);
	return new;
	} else if (!string_compare(token, "FOR")) {
		struct INTERMEDIATE *new2, *new3;

		new = new_inst();
		new->no = nowords++;
		new->in.type = PROG_PRIMITIVE;
		new->in.line = lineno;
		new->in.data.number = IN_FOR;
		new2 = (new->next = new_inst());
		new2->no = nowords++;
		new2->in.type = PROG_PRIMITIVE;
		new2->in.line = lineno;
		new2->in.data.number = IN_FORITER;
		new3 = (new2->next = new_inst());
		new3->no = nowords++;
		new3->in.line = lineno;
		new3->in.type = PROG_IF;
		new3->in.data.number = 0;

		addfor(new2);
		return new;
	} else if (!string_compare(token, "FOREACH")) {
		struct INTERMEDIATE *new2, *new3;

		new = new_inst();
		new->no = nowords++;
		new->in.type = PROG_PRIMITIVE;
		new->in.line = lineno;
		new->in.data.number = IN_FOREACH;
		new2 = (new->next = new_inst());
		new2->no = nowords++;
		new2->in.type = PROG_PRIMITIVE;
		new2->in.line = lineno;
		new2->in.data.number = IN_FORITER;
		new3 = (new2->next = new_inst());
		new3->no = nowords++;
		new3->in.line = lineno;
		new3->in.type = PROG_IF;
		new3->in.data.number = 0;

		addfor(new2);
		return new;
    } else if (!string_compare(token, "UNTIL")) {
	/* can't use 'if' because it's a reserved word */
	struct INTERMEDIATE *eef;
	struct INTERMEDIATE *beef;
	struct INTERMEDIATE *curr;

	resolve_loop_addrs(nowords + 1);
	curr = locate_for();
	eef = find_begin();
	if (!eef)
	    abort_compile("UNTIL without BEGIN.");
	beef = locate_if();
	if (beef && (beef->no > eef->no))
	    abort_compile("Unexpected end of loop.");
	new = new_inst();
	new->no = nowords++;
	new->in.type = PROG_IF;
	new->in.line = lineno;
	new->in.data.number = eef->no;
	if (curr) {
		curr = (new->next = new_inst());
		curr->no = nowords++;
		curr->in.type = PROG_PRIMITIVE;
		curr->in.line = lineno;
		curr->in.data.number = IN_FORPOP;
	}
	return new;
    } else if (!string_compare(token, "WHILE")) {
	/* can't use 'if' because it's a reserved word */
	struct INTERMEDIATE *eef;

	eef = locate_begin();
	if (!eef)
	    abort_compile("Can't have a WHILE outside of a loop.");
	new = new_inst();
	new->no = nowords++;
	new->in.type = PROG_IF;
	new->in.line = lineno;
	new->in.data.number = 0;

	addwhile(new);
	return new;
    } else if (!string_compare(token, "BREAK")) {
	/* can't use 'if' because it's a reserved word */
	struct INTERMEDIATE *eef;

	eef = locate_begin();
	if (!eef)
	    abort_compile("Can't have a BREAK outside of a loop.");

	new = new_inst();
	new->no = nowords++;
	new->in.type = PROG_JMP;
	new->in.line = lineno;
	new->in.data.number = 0;

	addwhile(new);
	return new;
    } else if (!string_compare(token, "CONTINUE")) {
	/* can't use 'if' because it's a reserved word */
	struct INTERMEDIATE *beef;

	beef = locate_begin();
	if (!beef)
	    abort_compile("Can't CONTINUE outside of a loop.");
	new = new_inst();
	new->no = nowords++;
	new->in.type = PROG_JMP;
	new->in.line = lineno;
	new->in.data.number = beef->no;

	return new;
    } else if (!string_compare(token, "REPEAT")) {
	/* can't use 'if' because it's a reserved word */
	struct INTERMEDIATE *eef;
	struct INTERMEDIATE *beef;
	struct INTERMEDIATE *curr;

	resolve_loop_addrs(nowords + 1);
	curr = locate_for();
	eef = find_begin();
	if (!eef)
	    abort_compile("REPEAT without BEGIN.");
	beef = locate_if();
	if (beef && (beef->no > eef->no))
	    abort_compile("Unexpected end of loop.");
	new = new_inst();
	new->no = nowords++;
	new->in.type = PROG_JMP;
	new->in.line = lineno;
	new->in.data.number = eef->no;

	if (curr) {
		curr = (new->next = new_inst());
		curr->no = nowords++;
		curr->in.type = PROG_PRIMITIVE;
		curr->in.line = lineno;
		curr->in.data.number = IN_FORPOP;
	}
	return new;
    } else if (!string_compare(token, "CALL")) {

	new = new_inst();
	new->no = nowords++;
	new->in.type = PROG_PRIMITIVE;
	new->in.line = lineno;
	new->in.data.number = IN_CALL;
	return new;
    } else if (!string_compare(token, "PUBLIC") || !string_compare(token, "WIZCALL")) {
	struct PROC_LIST *p;
	struct publics *pub;
	int wizflag = 0;

	if (!string_compare(token, "WIZCALL"))
 	    wizflag = 1;
	if (curr_proc)
	    abort_compile("PUBLIC or WIZCALL declaration within procedure.");
	tok = next_token();
	if ((!tok) || !call(tok))
	    abort_compile("Subroutine unknown in PUBLIC or WIZCALL declaration.");
	for (p = procs; p; p = p->next)
	    if (!string_compare(p->name, tok))
		break;
	if (!p)
	    abort_compile("Subroutine unknown in PUBLIC or WIZCALL declaration.");
	if (!currpubs) {
	    currpubs = (struct publics *) malloc(sizeof(struct publics));
	    currpubs->next = NULL;
	    currpubs->subname = (char *) string_dup(tok);
	    if (tok)
		free((void *) tok);
	    currpubs->addr.no = p->code->no;
          currpubs->mlev = wizflag ? 5 : 1;
	    return 0;
	}
	for (pub = currpubs; pub;) {
	    if (!string_compare(tok, pub->subname)) {
		abort_compile("Function already declared as PUBLIC or WIZCALL.");
	    } else {
		if (pub->next) {
		    pub = pub->next;
		} else {
		    pub->next = (struct publics *) malloc(sizeof(struct publics));
		    pub = pub->next;
		    pub->next = NULL;
		    pub->subname = (char *) string_dup(tok);
		    if (tok)
			free((void *) tok);
		    pub->addr.no = p->code->no;
                pub->mlev = wizflag ? 5 : 1;
		    pub = NULL;
		}
	    }
	}
	return 0;
	} else if (!string_compare(token, "VAR")) {
		if (curr_proc) {
/*			abort_compile("Variable declared within procedure."); */
			tok = next_token();
			if (!tok)
				abort_compile("Unexpected end of program.");
			if (add_scopedvar(tok) < 0)
				abort_compile("Variable limit exceeded.");
			if (tok)
				free((void *) tok);
			curr_decl->in.data.number++;
		} else {
			tok = next_token();
			if (!tok)
				abort_compile("Unexpected end of program.");
			if (add_variable(tok) == -1)
				abort_compile("Variable limit exceeded.");
			if (tok)
				free((void *) tok);
		}
		return 0;
	} else if (!string_compare(token, "LVAR")) {
		if (curr_proc)
			abort_compile("Local variable declared within procedure.");
		tok = next_token();
		if (!tok || (add_localvar(tok) == -1))
			abort_compile("Local variable limit exceeded.");
		if (tok)
			free((void *) tok);
		return 0;
    } else {
	sprintf(buf, "Unrecognized special form %s found. (%d)", token, lineno);
	abort_compile(buf);
    }
}

/* return primitive word. */
struct INTERMEDIATE *
primitive_word(const char *token)
{
    struct INTERMEDIATE *new;

    new = new_inst();
    new->no = nowords++;
    new->in.type = PROG_PRIMITIVE;
    new->in.line = lineno;
    new->in.data.number = get_primitive(token);
    return new;
}

/* return self pushing word (string) */
struct INTERMEDIATE *
string_word(const char *token)
{
    struct INTERMEDIATE *new;

    new = new_inst();
    new->no = nowords++;
    new->in.type = PROG_STRING;
    new->in.line = lineno;
    new->in.data.string = alloc_prog_string(token);
    return new;
}

/* return self pushing word (float) */
struct INTERMEDIATE *
float_word(const char *token)
{
	struct INTERMEDIATE *new;

	new = new_inst();
	new->no = nowords++;
	new->in.type = PROG_FLOAT;
	new->in.line = lineno;
	sscanf(token, "%g", &(new->in.data.fnumber));
	return new;
}
 
/* return self pushing word (number) */
struct INTERMEDIATE *
number_word(const char *token)
{
    struct INTERMEDIATE *new;

    new = new_inst();
    new->no = nowords++;
    new->in.type = PROG_INTEGER;
    new->in.line = lineno;
    new->in.data.number = atoi(token);
    return new;
}

/* do a subroutine call --- push address onto stack, then make a primitive
   CALL.
   */
struct INTERMEDIATE *
call_word(const char *token)
{
    struct INTERMEDIATE *new;
    struct PROC_LIST *p;

    new = new_inst();
    new->no = nowords++;
    new->in.type = PROG_EXEC;
    new->in.line = lineno;
    for (p = procs; p; p = p->next)
	if (!string_compare(p->name, token))
	    break;

    new->in.data.number = p->code->no;
    return new;
}

struct INTERMEDIATE *
quoted_word(const char *token)
{
    struct INTERMEDIATE *new;
    struct PROC_LIST *p;

    new = new_inst();
    new->no = nowords++;
    new->in.type = PROG_ADD;
    new->in.line = lineno;
    for (p = procs; p; p = p->next)
	if (!string_compare(p->name, token))
	    break;

    new->in.data.number = p->code->no;
    return new;
}

/* returns number corresponding to variable number.
   We assume that it DOES exist */
struct INTERMEDIATE *
var_word(const char *token)
{
    struct INTERMEDIATE *new;
    int     i, var_no;

    new = new_inst();
    new->no = nowords++;
    new->in.type = PROG_VAR;
    new->in.line = lineno;
    for (var_no = i = 0; i < MAX_VAR; i++) {
	if (!variables[i]) break;
	if (!string_compare(token, variables[i])) var_no = i;
    }
    new->in.data.number = var_no;

    return new;
}

struct INTERMEDIATE *
svar_word(const char *token)
{
	struct INTERMEDIATE *new;
	int i, var_no;

	new = new_inst();
	new->no = nowords++;
	new->in.type = PROG_SVAR;
	new->in.line = lineno;
	for (i = var_no = 0; i < MAX_VAR; i++) {
		if (!scopedvars[i])
			break;
		if (!string_compare(token, scopedvars[i]))
			var_no = i;
	}
	new->in.data.number = var_no;

	return new;
}

struct INTERMEDIATE *
lvar_word(const char *token)
{
    struct INTERMEDIATE *new;
    int     i, var_no;

    new = new_inst();
    new->no = nowords++;
    new->in.type = PROG_LVAR;
    new->in.line = lineno;
    for (i = var_no = 0; i < MAX_VAR; i++) {
	if (!localvars[i]) break;
	if (!string_compare(token, localvars[i])) var_no = i;
    }
    new->in.data.number = var_no;

    return new;
}

/* check if object is in database before putting it in */
struct INTERMEDIATE *
object_word(const char *token)
{
    struct INTERMEDIATE *new;
    int     objno;

    objno = atol(token + 1);
    new = new_inst();
    new->no = nowords++;
    new->in.type = PROG_OBJECT;
    new->in.line = lineno;
    new->in.data.objref = objno;
    return new;
}



/* support routines for internal data structures. */

/* add procedure to procedures list */
void
add_proc(const char *proc_name, struct INTERMEDIATE * place)
{
    struct PROC_LIST *new;

    new = (struct PROC_LIST *) malloc(sizeof(struct PROC_LIST));
    new->name = alloc_string(proc_name);
    new->code = place;
    new->next = procs;
    procs = new;
}

/* add if to if stack */
void
addif(struct INTERMEDIATE * place)
{
    struct IF_STACK *new;

    new = (struct IF_STACK *) malloc(sizeof(struct IF_STACK));
    new->place = place;
    new->type = CTYPE_IF;
    new->next = if_stack;
    if_stack = new;
}

/* add else to if stack */
void
addelse(struct INTERMEDIATE * place)
{
    struct IF_STACK *new;

    new = (struct IF_STACK *) malloc(sizeof(struct IF_STACK));
    new->place = place;
    new->type = CTYPE_ELSE;
    new->next = if_stack;
    if_stack = new;
}

/* add begin to if stack */
void
addbegin(struct INTERMEDIATE * place)
{
    struct IF_STACK *new;

    new = (struct IF_STACK *) malloc(sizeof(struct IF_STACK));
    new->place = place;
    new->type = CTYPE_BEGIN;
    new->next = loop_stack;
    loop_stack = new;
}

/* add for to LOOP stack */
void
addfor(struct INTERMEDIATE *place)
{
	struct IF_STACK *new;

	new = (struct IF_STACK *) malloc(sizeof(struct IF_STACK));
	new->place = place;
	new->type = CTYPE_FOR;
	new->next = loop_stack;
	loop_stack = new;
	nested_fors++;
}

/* add while to if stack */
void
addwhile(struct INTERMEDIATE * place)
{
    struct IF_STACK *new;

    new = (struct IF_STACK *) malloc(sizeof(struct IF_STACK));
    new->place = place;
    new->type = CTYPE_WHILE;
    new->next = loop_stack;
    loop_stack = new;
}

/* finds topmost else or if off the stack */
struct INTERMEDIATE *
locate_if(void)
{
    if (!if_stack)
	return 0;
    if ((if_stack->type != CTYPE_IF) &&
	    (if_stack->type != CTYPE_ELSE))
	return 0;

    return (if_stack->place);
}

/* pops topmost if off the stack */
struct INTERMEDIATE *
find_if(void)
{
    struct INTERMEDIATE *temp;
    struct IF_STACK *tofree;

    if (!if_stack)
	return 0;
    if (if_stack->type != CTYPE_IF)
	return 0;

    temp = if_stack->place;
    tofree = if_stack;
    if_stack = if_stack->next;
    free((void *) tofree);
    return temp;
}

/* pops topmost else or if off the stack */
struct INTERMEDIATE *
find_else(void)
{
    struct INTERMEDIATE *temp;
    struct IF_STACK *tofree;

    if (!if_stack)
	return 0;
    if ((if_stack->type != CTYPE_IF) &&
	    (if_stack->type != CTYPE_ELSE))
	return 0;

    temp = if_stack->place;
    tofree = if_stack;
    if_stack = if_stack->next;
    free((void *) tofree);
    return temp;
}

/* returns topmost begin off the stack */
struct INTERMEDIATE *
locate_begin(void)
{
    struct IF_STACK *tempeef;

    tempeef = loop_stack;
    while (tempeef && (tempeef->type == CTYPE_WHILE))
	tempeef = tempeef->next;
    if (!tempeef)
	return 0;
    if (tempeef->type != CTYPE_BEGIN && tempeef->type != CTYPE_FOR)
	return 0;

    return tempeef->place;
}

/* checks if topmost loop stack item is a for */
struct INTERMEDIATE *
locate_for(void)
{
	struct IF_STACK *tempeef;

	tempeef = loop_stack;
	while (tempeef && (tempeef->type == CTYPE_WHILE))
		tempeef = tempeef->next;
	if (!tempeef)
		return 0;
	if (tempeef->type != CTYPE_FOR)
		return 0;

	return tempeef->place;
}

/* pops topmost begin off the stack */
struct INTERMEDIATE *
find_begin(void)
{
    struct INTERMEDIATE *temp;
    struct IF_STACK *tofree;

    if (!loop_stack)
	return 0;
    if (loop_stack->type != CTYPE_BEGIN && loop_stack->type != CTYPE_FOR)
	return 0;

    if (loop_stack->type == CTYPE_FOR)
	nested_fors--;

    temp = loop_stack->place;
    tofree = loop_stack;
    loop_stack = loop_stack->next;
    free((void *) tofree);
    return temp;
}

/* pops topmost while off the stack */
struct INTERMEDIATE *
find_while(void)
{
    struct INTERMEDIATE *temp;
    struct IF_STACK *tofree;

    if (!loop_stack)
	return 0;
    if (loop_stack->type != CTYPE_WHILE)
	return 0;

    temp = loop_stack->place;
    tofree = loop_stack;
    loop_stack = loop_stack->next;
    free((void *) tofree);
    return temp;
}

void
resolve_loop_addrs(int where)
{
    struct INTERMEDIATE *eef;

    while ((eef = find_while()))
	eef->in.data.number = where;
    if ((eef = locate_for())) {
	eef->next->in.data.number = where;
    }
}

/* adds variable.  Return 0 if no space left */
int
add_variable(const char *varname)
{
    int     i;

    for (i = 0; i < MAX_VAR; i++)
	if (!variables[i])
	    break;

    if (i == MAX_VAR)
	return -1;

    variables[i] = alloc_string(varname);
    return i;
}

/* adds local variable.  Return 0 if no space left */
int
add_scopedvar(const char *varname)
{
	int i;

	for (i = 0; i < MAX_VAR; i++)
		if (!scopedvars[i])
			break;

	if (i == MAX_VAR)
		return -1;

	scopedvars[i] = alloc_string(varname);
	return i;
}

/* adds local variable.  Return 0 if no space left */
int
add_localvar(const char *varname)
{
    int     i;

    for (i = RES_VAR; i < MAX_VAR; i++)
	if (!localvars[i])
	    break;

    if (i == MAX_VAR)
	return -1;

    localvars[i] = alloc_string(varname);
    return i;
}


/* predicates for procedure calls */
int
special(const char *token)
{
    return (token && !(string_compare(token, ":")
		       && string_compare(token, ";")
		       && string_compare(token, "IF")
		       && string_compare(token, "ELSE")
		       && string_compare(token, "THEN")
		       && string_compare(token, "BEGIN")
		       && string_compare(token, "FOR")
		       && string_compare(token, "FOREACH")
		       && string_compare(token, "UNTIL")
		       && string_compare(token, "WHILE")
		       && string_compare(token, "BREAK")
		       && string_compare(token, "CONTINUE")
		       && string_compare(token, "REPEAT")
		       && string_compare(token, "CALL")
		       && string_compare(token, "PUBLIC")
                   && string_compare(token, "WIZCALL")
		       && string_compare(token, "LVAR")
		       && string_compare(token, "VAR")));
}

/* see if procedure call */
int
call(const char *token)
{
    struct PROC_LIST *i;

    for (i = procs; i; i = i->next)
	if (!string_compare(i->name, token))
	    return 1;

    return 0;
}

/* see if it's a quoted procedure name */
int
quoted(const char *token)
{
    return (*token == '\'' && call(token + 1));
}

/* see if it's an object # */
int
object(const char *token)
{
    if (*token == '#' && number(token + 1))
	return 1;
    else
	return 0;
}

/* see if string */
int
string(const char *token)
{
    return (token[0] == '"');
}

int
variable(const char *token)
{
    int     i;

    for (i = 0; i < MAX_VAR && variables[i]; i++)
	if (!string_compare(token, variables[i]))
	    return 1;

    return 0;
}

int
scopedvar(const char *token)
{
	int i;

	for (i = 0; i < MAX_VAR && scopedvars[i]; i++)
		if (!string_compare(token, scopedvars[i]))
			return 1;

	return 0;
}

int
localvar(const char *token)
{
    int     i;

    for (i = 0; i < MAX_VAR && localvars[i]; i++)
	if (!string_compare(token, localvars[i]))
	    return 1;

    return 0;
}

/* see if token is primitive */
int
primitive(const char *token)
{
/*    return get_primitive(token); */
    int primnum;

    primnum = get_primitive(token);
    return (primnum && primnum <= BASE_MAX - PRIMS_INTERNAL_CNT);
}

/* return primitive instruction */
int
get_primitive(const char *token)
{
    hash_data *hd;

    if ((hd = find_hash(token, primitive_list, COMP_HASH_SIZE)) == NULL)
	return 0;
    else {
	return (hd->ival);
    }
}



/* clean up as nicely as we can. */

void
cleanpubs(struct publics * mypub)
{
    struct publics *tmppub;

    while (mypub) {
	tmppub = mypub->next;
	free(mypub->subname);
	free(mypub);
	mypub = tmppub;
    }
}

void
cleanup(void)
{
    struct INTERMEDIATE *wd, *tempword;
    struct IF_STACK *eef, *tempif;
    struct PROC_LIST *p, *tempp;
    int     i;

    for (wd = first_word; wd; wd = tempword) {
	tempword = wd->next;
	if (wd->in.type == PROG_STRING || wd->in.type == PROG_FUNCTION)
	    if (wd->in.data.string)
		free((void *) wd->in.data.string);
	free((void *) wd);
    }
    first_word = 0;

    for (eef = if_stack; eef; eef = tempif) {
	tempif = eef->next;
	free((void *) eef);
    }
    if_stack = 0;

    for (eef = loop_stack; eef; eef = tempif) {
	tempif = eef->next;
	free((void *) eef);
    }
    loop_stack = 0;

    for (p = procs; p; p = tempp) {
	tempp = p->next;
	free((void *) p->name);
	free((void *) p);
    }
    procs = 0;

    purge_defs();

    for (i = 0; i < MAX_VAR && variables[i]; i++) {
	free((void *) variables[i]);
	variables[i] = 0;
    }

    for (i = 0; i < MAX_VAR && scopedvars[i]; i++) {
       free((void *) scopedvars[i]);
       scopedvars[i] = 0;
    }

    for (i = RES_VAR; i < MAX_VAR && localvars[i]; i++) {
	free((void *) localvars[i]);
	localvars[i] = 0;
    }
}


/* copy program to an array */
void
copy_program(void)
{
    /*
     * Everything should be peachy keen now, so we don't do any error checking
     */
    struct INTERMEDIATE *curr;
    struct inst *code;
    int     i;

    if (!first_word)
	v_abort_compile("Nothing to compile.");

    code = (struct inst *) malloc(sizeof(struct inst) * (nowords + 1));

    i = 0;
    for (curr = first_word; curr; curr = curr->next) {
	code[i].type = curr->in.type;
	code[i].line = curr->in.line;
	switch (code[i].type) {
	    case PROG_PRIMITIVE:
	    case PROG_INTEGER:
          case PROG_DECLVAR:
          case PROG_SVAR:
	    case PROG_LVAR:
	    case PROG_VAR:
		code[i].data.number = curr->in.data.number;
		break;
 	    case PROG_FLOAT:
		code[i].data.fnumber = curr->in.data.fnumber;
		break;
	    case PROG_STRING:
	    case PROG_FUNCTION:
		code[i].data.string = curr->in.data.string ?
		    alloc_prog_string(curr->in.data.string->data) : 0;
		break;
	    case PROG_OBJECT:
		code[i].data.objref = curr->in.data.objref;
		break;
	    case PROG_ADD:
		code[i].data.addr = alloc_addr(curr->in.data.number, code);
		break;
	    case PROG_IF:
	    case PROG_JMP:
	    case PROG_EXEC:
		code[i].data.call = code + curr->in.data.number;
		break;
	    default:
		v_abort_compile("Unknown type compile!  Internal error.");
		break;
	}
	i++;
    }
    DBFETCH(program)->sp.program.code = code;
}

void
set_start(void)
{
    DBFETCH(program)->sp.program.siz = nowords;
    DBFETCH(program)->sp.program.start = (DBFETCH(program)->sp.program.code + procs->code->no);
}


/* allocate and initialize data linked structure. */
struct INTERMEDIATE *
new_inst()
{
    struct INTERMEDIATE *new;

    new = (struct INTERMEDIATE *) malloc(sizeof(struct INTERMEDIATE));
    new->next = 0;
    new->no = 0;
    new->in.type = 0;
    new->in.line = 0;
    new->in.data.number = 0;
    return new;
}

/* allocate an address */
struct prog_addr *
alloc_addr(int offset, struct inst *codestart)
{
    struct prog_addr *new;

    new = (struct prog_addr *) malloc(sizeof(struct prog_addr));
    new->links = 1;
    new->progref = program;
    new->data = codestart + offset;
    return new;
}

void
free_prog(struct inst * c, int siz)
{
    int     i;

    if (c) {
	if (DBFETCH(program)->sp.program.instances) {
	    fprintf(stderr, "Freeing program %s with %d instances reported\n",
		   unparse_object(MAN, program), DBFETCH(program)->sp.program.instances);
	}
	i = scan_instances(program);
	if (i) {
	    fprintf(stderr, "Freeing program %s with %d instances found\n",
		   unparse_object(MAN, program), i);
	}
	for (i = 0; i < siz; i++) {
	    if (c[i].type == PROG_STRING || c[i].type == PROG_FUNCTION)
		CLEAR(c+i);
	    if (c[i].type == PROG_ADD) {
		if (c[i].data.addr->links != 1) {
		    fprintf(stderr, "Freeing address in %s with link count %d\n",
			   unparse_object(MAN, program), c[i].data.addr->links);
		}
		free(c[i].data.addr);
	    }
	}
	free((void *) c);
    }
    DBFETCH(program)->sp.program.code = 0;
    DBFETCH(program)->sp.program.siz = 0;
    DBFETCH(program)->sp.program.start = 0;
}

long
size_prog(dbref prog)
{
    struct inst *c;
    int i, siz, byts = 0;

    c = DBFETCH(prog)->sp.program.code;
    if (!c) return 0;
    siz = DBFETCH(prog)->sp.program.siz;
    for (i = 0L; i < siz; i++) {
	byts += sizeof(*c);
	if ((c[i].type == PROG_STRING || c[i].type == PROG_FUNCTION) &&
		c[i].data.string)
	    byts += strlen(c[i].data.string->data) + 1;
	if ((c[i].type == PROG_ADD))
	    byts += sizeof(struct prog_addr);
    }
    byts += size_pubs(DBFETCH(prog)->sp.program.pubs);
    return byts;
}

static void
add_primitive(int val)
{
    hash_data hd;

    hd.ival = val;
    if (add_hash(base_inst[val - BASE_MIN], hd, primitive_list,
		 COMP_HASH_SIZE) == NULL)
	panic("Out of memory");
    else
	return;
}

void
clear_primitives(void)
{
    kill_hash(primitive_list, COMP_HASH_SIZE, 0);
    return;
}

void
init_primitives(void)
{
    int     i;

    clear_primitives();
    for (i = BASE_MIN; i <= BASE_MAX; i++) {
	add_primitive(i);
    }
    IN_FORPOP = get_primitive(" FORPOP");
    IN_FORITER = get_primitive(" FORITER");
    IN_FOR = get_primitive(" FOR");
    IN_FOREACH = get_primitive(" FOREACH");
}




