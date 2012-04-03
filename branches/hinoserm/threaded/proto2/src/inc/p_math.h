extern void prim_add (PRIM_PROTOTYPE);
extern void prim_subtract (PRIM_PROTOTYPE);
extern void prim_multiply (PRIM_PROTOTYPE);
extern void prim_divide (PRIM_PROTOTYPE);
extern void prim_mod (PRIM_PROTOTYPE);
extern void prim_bitor (PRIM_PROTOTYPE);
extern void prim_bitxor (PRIM_PROTOTYPE);
extern void prim_bitand (PRIM_PROTOTYPE);
extern void prim_bitshift (PRIM_PROTOTYPE);
extern void prim_and (PRIM_PROTOTYPE);
extern void prim_or (PRIM_PROTOTYPE);
extern void prim_xor (PRIM_PROTOTYPE);
extern void prim_not (PRIM_PROTOTYPE);
extern void prim_lessthan (PRIM_PROTOTYPE);
extern void prim_greathan (PRIM_PROTOTYPE);
extern void prim_equal (PRIM_PROTOTYPE);
extern void prim_noteq (PRIM_PROTOTYPE);
extern void prim_lesseq (PRIM_PROTOTYPE);
extern void prim_greateq (PRIM_PROTOTYPE);
extern void prim_random (PRIM_PROTOTYPE);
extern void prim_int(PRIM_PROTOTYPE);
extern void prim_srand(PRIM_PROTOTYPE);
extern void prim_getseed(PRIM_PROTOTYPE);
extern void prim_setseed(PRIM_PROTOTYPE);
extern void prim_plusplus(PRIM_PROTOTYPE);
extern void prim_minusminus(PRIM_PROTOTYPE);
extern void prim_abs(PRIM_PROTOTYPE);
extern void prim_sign(PRIM_PROTOTYPE);

#define PRIMS_MATH_FUNCS prim_add, prim_subtract, prim_multiply, prim_divide, \
    prim_mod, prim_bitor, prim_bitxor, prim_bitand, prim_bitshift, prim_and,  \
    prim_or, prim_xor, prim_not, prim_lessthan, prim_greathan, prim_equal,    \
    prim_noteq, prim_lesseq, prim_greateq, prim_random, prim_int, prim_srand, \
    prim_setseed, prim_getseed, prim_plusplus, prim_minusminus, prim_abs,     \
    prim_sign

#define PRIMS_MATH_NAMES  "+",  "-",  "*",  "/",          \
    "%", "BITOR", "BITXOR", "BITAND", "BITSHIFT", "AND",  \
    "OR",  "XOR",  "NOT",  "<",  ">",  "=",  "!=",  "<=", \
    ">=", "RANDOM", "INT", "SRAND", "SETSEED", "GETSEED", \
    "++", "--", "ABS", "SIGN"
    
    
#define PRIMLIST_MATH   { "+",         LM1, 2, prim_add },          \
                        { "-",         LM1, 2, prim_subtract },     \
                        { "*",         LM1, 2, prim_multiply },     \
                        { "/",         LM1, 2, prim_divide },       \
                        { "%",         LM1, 2, prim_mod },          \
                        { "BITOR",     LM1, 2, prim_bitor },        \
                        { "BITXOR",    LM1, 2, prim_bitxor },       \
                        { "BITAND",    LM1, 2, prim_bitand },       \
                        { "BITSHIFT",  LM1, 2, prim_bitshift },     \
                        { "AND",       LM1, 2, prim_and },          \
                        { "OR",        LM1, 2, prim_or },           \
                        { "XOR",       LM1, 2, prim_xor },          \
                        { "NOT",       LM1, 1, prim_not },          \
                        { "<",         LM1, 2, prim_lessthan },     \
                        { ">",         LM1, 2, prim_greathan },     \
                        { "=",         LM1, 2, prim_equal },        \
                        { "!=",        LM1, 2, prim_noteq },        \
                        { "<=",        LM1, 2, prim_lesseq },       \
                        { ">=",        LM1, 2, prim_greateq },      \
                        { "RANDOM",    LM1, 0, prim_random },       \
                        { "INT",       LM1, 1, prim_int },          \
                        { "SRAND",     LM1, 0, prim_srand },        \
                        { "SETSEED",   LM1, 1, prim_setseed },      \
                        { "GETSEED",   LM1, 0, prim_getseed },      \
                        { "++",        LM1, 1, prim_plusplus },     \
                        { "--",        LM1, 1, prim_minusminus },   \
                        { "ABS",       LM1, 1, prim_abs },          \
                        { "SIGN",      LM1, 1, prim_sign }
      
#define PRIMS_MATH_CNT 28



