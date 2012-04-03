extern void prim_clear(PRIM_PROTOTYPE);
extern void prim_clear_error(PRIM_PROTOTYPE);
extern void prim_set_error(PRIM_PROTOTYPE);
extern void prim_is_error(PRIM_PROTOTYPE);
extern void prim_is_set(PRIM_PROTOTYPE);
extern void prim_error_str(PRIM_PROTOTYPE);
extern void prim_error_name(PRIM_PROTOTYPE);
extern void prim_error_bit(PRIM_PROTOTYPE);
extern void prim_error_num(PRIM_PROTOTYPE);

#define PRIMLIST_ERROR { "CLEAR",         LM1,    0, prim_clear         }, \
                       { "CLEAR_ERROR",   LM1,    1, prim_clear_error   }, \
                       { "ERROR?",        LM1,    0, prim_is_error      }, \
                       { "SET_ERROR",     LM1,    1, prim_set_error     }, \
                       { "IS_SET?",       LM1,    1, prim_is_set        }, \
                       { "ERROR_STR",     LM1,    1, prim_error_str     }, \
                       { "ERROR_NAME",    LM1,    1, prim_error_name    }, \
                       { "ERROR_BIT",     LM1,    1, prim_error_bit     }, \
                       { "ERROR_NUM",     LM1,    0, prim_error_num     }

#define PRIMS_ERROR_CNT 9
