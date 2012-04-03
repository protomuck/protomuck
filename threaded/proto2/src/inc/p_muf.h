extern void prim_kill_macro(PRIM_PROTOTYPE);
extern void prim_insert_macro(PRIM_PROTOTYPE);
extern void prim_get_macros_array(PRIM_PROTOTYPE);
extern void prim_program_linecount(PRIM_PROTOTYPE);
extern void prim_program_getlines(PRIM_PROTOTYPE);
extern void prim_program_deletelines(PRIM_PROTOTYPE);
extern void prim_program_insertlines(PRIM_PROTOTYPE);
 
#define PRIMLIST_MUFEDIT { "KILL_MACRO",            LWIZ,  1, prim_kill_macro         }, \
                         { "INSERT_MACRO",          LWIZ,  2, prim_insert_macro       }, \
                         { "GET_MACROS_ARRAY",      LMAGE, 0, prim_get_macros_array   }, \
                         { "PROGRAM_LINECOUNT",     LWIZ,  1, prim_program_linecount  }, \
                         { "PROGRAM_GETLINES",      LM1,   3, prim_program_getlines   }, \
                         { "PROGRAM_DELETELINES",   LBOY,  3, prim_program_deletelines}, \
                         { "PROGRAM_INSERTLINES",   LBOY,  3, prim_program_insertlines}
 
#ifdef MUF_EDIT_PRIMS
#define PRIMS_MUFEDIT_CNT 7
#else
#define PRIMS_MUFEDIT_CNT 0
#endif
