extern void prim_kill_macro(PRIM_PROTOTYPE);
extern void prim_insert_macro(PRIM_PROTOTYPE);
extern void prim_get_macros_array(PRIM_PROTOTYPE);
extern void prim_program_linecount(PRIM_PROTOTYPE);
extern void prim_program_getlines(PRIM_PROTOTYPE);
extern void prim_program_deletelines(PRIM_PROTOTYPE);
extern void prim_program_insertlines(PRIM_PROTOTYPE);

#define PRIMS_MUFEDIT_FUNCS prim_kill_macro, prim_insert_macro, \
    prim_get_macros_array, prim_program_linecount, prim_program_getlines, \
    prim_program_deletelines, prim_program_insertlines

#define PRIMS_MUFEDIT_NAMES "KILL_MACRO", "INSERT_MACRO", \
    "GET_MACROS_ARRAY", "PROGRAM_LINECOUNT", "PROGRAM_GETLINES", \
    "PROGRAM_DELETELINES", "PROGRAM_INSERTLINES"

#ifdef MUF_EDIT_PRIMS
#define PRIMS_MUFEDIT_CNT 7
#else
#define PRIMS_MUFEDIT_CNT 0
#endif
