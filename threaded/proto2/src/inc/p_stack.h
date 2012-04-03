extern void prim_pop (PRIM_PROTOTYPE);
extern void prim_dup (PRIM_PROTOTYPE);
extern void prim_pdup (PRIM_PROTOTYPE);
extern void prim_nip (PRIM_PROTOTYPE);
extern void prim_tuck (PRIM_PROTOTYPE);
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
extern void prim_sqlp(PRIM_PROTOTYPE);
extern void prim_markp(PRIM_PROTOTYPE);
extern void prim_ndup(PRIM_PROTOTYPE);

/* Internal functions */
extern void prim_for(PRIM_PROTOTYPE);
extern void prim_foreach(PRIM_PROTOTYPE);
extern void prim_foriter(PRIM_PROTOTYPE);
extern void prim_forpop(PRIM_PROTOTYPE);
extern void prim_trypop(PRIM_PROTOTYPE);

#define PRIMLIST_STACK  { "POP",         LM1, 1, prim_pop        },  \
                        { "DUP",         LM1, 0, prim_dup        },  \
                        { "@",           LM1, 1, prim_at         },  \
                        { "!",           LM1, 2, prim_bang       },  \
                        { "VARIABLE",    LM1, 1, prim_var        },  \
                        { "LOCALVAR",    LM1, 1, prim_localvar   },  \
                        { "SWAP",        LM1, 2, prim_swap       },  \
                        { "OVER",        LM1, 0, prim_over       },  \
                        { "PICK",        LM1, 1, prim_pick       },  \
                        { "PUT",         LM1, 2, prim_put        },  \
                        { "ROT",         LM1, 3, prim_rot        },  \
                        { "ROTATE",      LM1, 1, prim_rotate     },  \
                        { "DBTOP",       LM1, 0, prim_dbtop      },  \
                        { "DEPTH",       LM1, 0, prim_depth      },  \
                        { "PROG",        LM1, 0, prim_prog       },  \
                        { "TRIG",        LM1, 0, prim_trig       },  \
                        { "CALLER",      LM1, 0, prim_caller     },  \
                        { "INT?",        LM1, 1, prim_intp       },  \
                        { "STRING?",     LM1, 1, prim_stringp    },  \
                        { "DBREF?",      LM1, 1, prim_dbrefp     },  \
                        { "ADDRESS?",    LM1, 1, prim_addressp   },  \
                        { "LOCK?",       LM1, 1, prim_lockp      },  \
                        { "CHECKARGS",   LM1, 1, prim_checkargs  },  \
                        { "MODE",        LM1, 0, prim_mode       },  \
                        { "SETMODE",     LM1, 1, prim_setmode    },  \
                        { "INTERP",      LM1, 3, prim_interp     },  \
                        { "SORT",        LM1, 2, prim_sort       },  \
                        { "POPN",        LM1, 1, prim_popn       },  \
                        { "VARIABLE?",   LM1, 1, prim_variablep  },  \
                        { "{",           LM1, 0, prim_mark       },  \
                        { "}",           LM1, 0, prim_findmark   },  \
                        { "FLOAT?",      LM1, 1, prim_floatp     },  \
                        { "ARRAY?",      LM1, 1, prim_arrayp     },  \
                        { "DICTIONARY?", LM1, 1, prim_dictionaryp},  \
                        { "REVERSE",     LM1, 1, prim_reverse    },  \
                        { "LREVERSE",    LM1, 1, prim_lreverse   },  \
                        { "DUPN",        LM1, 1, prim_dupn       },  \
                        { "LDUP",        LM1, 1, prim_ldup       },  \
                        { " FOR",        LM1, 3, prim_for        },  \
                        { " FOREACH",    LM1, 1, prim_foreach    },  \
                        { "SOCKET?",     LM1, 1, prim_socketp    },  \
                        { "MARK?",       LM1, 1, prim_markp      },  \
                        { "SQL?",        LM1, 1, prim_sqlp       },  \
                        { "?DUP",        LM1, 0, prim_pdup       },  \
                        { "NIP",         LM1, 2, prim_nip        },  \
                        { "TUCK",        LM1, 2, prim_tuck       },  \
                        { "NDUP",        LM1, 1, prim_ndup       }                        
                        
#define PRIMS_STACK_CNT 47

#define PRIMLIST_INTERNAL { " FORITER", LM1, 0, prim_foriter}, \
                          { " FORPOP",  LM1, 0, prim_forpop},  \
                          { " TRYPOP",  LM1, 0, prim_trypop}

#define PRIMS_INTERNAL_CNT 3




