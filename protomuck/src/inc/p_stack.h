extern void prim_pop (PRIM_PROTOTYPE);
extern void prim_dup (PRIM_PROTOTYPE);
extern void prim_at (PRIM_PROTOTYPE);
extern void prim_bang (PRIM_PROTOTYPE);
extern void prim_var (PRIM_PROTOTYPE);
extern void prim_localvar (PRIM_PROTOTYPE);
extern void prim_swap (PRIM_PROTOTYPE);
extern void prim_over (PRIM_PROTOTYPE);
extern void prim_pick (PRIM_PROTOTYPE);
extern void prim_put (PRIM_PROTOTYPE);
extern void prim_rot (PRIM_PROTOTYPE);
extern void prim_popn (PRIM_PROTOTYPE);
extern void prim_sort (PRIM_PROTOTYPE);
extern void prim_rotate (PRIM_PROTOTYPE);
extern void prim_dbtop (PRIM_PROTOTYPE);
extern void prim_depth (PRIM_PROTOTYPE);
extern void prim_prog (PRIM_PROTOTYPE);
extern void prim_trig (PRIM_PROTOTYPE);
extern void prim_caller (PRIM_PROTOTYPE);
extern void prim_preempt (PRIM_PROTOTYPE);
extern void prim_foreground (PRIM_PROTOTYPE);
extern void prim_background (PRIM_PROTOTYPE);
extern void prim_intp(PRIM_PROTOTYPE);
extern void prim_stringp(PRIM_PROTOTYPE);
extern void prim_dbrefp(PRIM_PROTOTYPE);
extern void prim_addressp(PRIM_PROTOTYPE);
extern void prim_lockp(PRIM_PROTOTYPE);
extern void prim_checkargs(PRIM_PROTOTYPE);
extern void prim_mode(PRIM_PROTOTYPE);
extern void prim_setmode(PRIM_PROTOTYPE);
extern void prim_interp(PRIM_PROTOTYPE);
extern void prim_variablep(PRIM_PROTOTYPE);
extern void prim_mark(PRIM_PROTOTYPE);
extern void prim_findmark(PRIM_PROTOTYPE);
extern void prim_floatp(PRIM_PROTOTYPE);
extern void prim_arrayp(PRIM_PROTOTYPE);
extern void prim_dictionaryp(PRIM_PROTOTYPE);
extern void prim_reverse(PRIM_PROTOTYPE);
extern void prim_lreverse(PRIM_PROTOTYPE);
extern void prim_dupn(PRIM_PROTOTYPE);
extern void prim_ldup(PRIM_PROTOTYPE);
extern void prim_socketp(PRIM_PROTOTYPE);
extern void prim_mysqlp(PRIM_PROTOTYPE);
extern void prim_markp(PRIM_PROTOTYPE);

/* Internal functions */
extern void prim_for(PRIM_PROTOTYPE);
extern void prim_foreach(PRIM_PROTOTYPE);
extern void prim_foriter(PRIM_PROTOTYPE);
extern void prim_forpop(PRIM_PROTOTYPE);
extern void prim_trypop(PRIM_PROTOTYPE);

#define PRIMS_STACK_FUNCS prim_pop, prim_dup, prim_at, prim_bang, prim_var,  \
    prim_localvar, prim_swap, prim_over, prim_pick, prim_put, prim_rot,      \
    prim_rotate, prim_dbtop, prim_depth, prim_prog, prim_trig, \
    prim_caller, prim_intp, prim_stringp, prim_dbrefp, prim_addressp,        \
    prim_lockp, prim_checkargs, prim_mode, prim_setmode, prim_interp,	     \
    prim_sort, prim_popn, prim_variablep, prim_mark, prim_findmark,          \
    prim_floatp, prim_arrayp, prim_dictionaryp, prim_reverse, prim_lreverse, \
    prim_dupn, prim_ldup, prim_for, prim_foreach, prim_socketp, prim_markp, \
    prim_mysqlp

#define PRIMS_STACK_NAMES "POP", "DUP", "@", "!", "VARIABLE",  \
    "LOCALVAR", "SWAP", "OVER", "PICK", "PUT", "ROT",          \
    "ROTATE", "DBTOP", "DEPTH",  "PROG", "TRIG",     \
    "CALLER", "INT?", "STRING?", "DBREF?", "ADDRESS?",         \
    "LOCK?", "CHECKARGS", "MODE", "SETMODE", "INTERP",	   \
    "SORT", "POPN", "VARIABLE?", "{", "}", "FLOAT?", "ARRAY?", \
    "DICTIONARY?", "REVERSE", "LREVERSE", "DUPN", "LDUP",      \
    " FOR", " FOREACH", "SOCKET?", "MARK?", "MYSQL?"

#define PRIMS_STACK_CNT 43

#define PRIMS_INTERNAL_FUNCS prim_foriter, prim_forpop, prim_trypop

#define PRIMS_INTERNAL_NAMES " FORITER", " FORPOP", " TRYPOP"

#define PRIMS_INTERNAL_CNT 3




