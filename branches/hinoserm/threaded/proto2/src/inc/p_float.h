extern void prim_ceil(PRIM_PROTOTYPE);
extern void prim_floor(PRIM_PROTOTYPE);
extern void prim_float(PRIM_PROTOTYPE);
extern void prim_sqrt(PRIM_PROTOTYPE);
extern void prim_pow(PRIM_PROTOTYPE);
extern void prim_frand(PRIM_PROTOTYPE);
extern void prim_sin(PRIM_PROTOTYPE);
extern void prim_cos(PRIM_PROTOTYPE);
extern void prim_tan(PRIM_PROTOTYPE);
extern void prim_asin(PRIM_PROTOTYPE);
extern void prim_acos(PRIM_PROTOTYPE);
extern void prim_atan(PRIM_PROTOTYPE);
extern void prim_atan2(PRIM_PROTOTYPE);
extern void prim_exp(PRIM_PROTOTYPE);
extern void prim_log(PRIM_PROTOTYPE);
extern void prim_log10(PRIM_PROTOTYPE);
extern void prim_fabs(PRIM_PROTOTYPE);
extern void prim_strtof(PRIM_PROTOTYPE);
extern void prim_ftostr(PRIM_PROTOTYPE);
extern void prim_fmod(PRIM_PROTOTYPE);
extern void prim_modf(PRIM_PROTOTYPE);
extern void prim_pi(PRIM_PROTOTYPE);
/* extern void prim_inf(PRIM_PROTOTYPE); */
extern void prim_round(PRIM_PROTOTYPE);
extern void prim_xyz_to_polar(PRIM_PROTOTYPE);
extern void prim_polar_to_xyz(PRIM_PROTOTYPE);
extern void prim_dist3d(PRIM_PROTOTYPE);
extern void prim_diff3(PRIM_PROTOTYPE);
extern void prim_epsilon(PRIM_PROTOTYPE);
extern void prim_ftostrc(PRIM_PROTOTYPE);

#define PRIMS_FLOAT_FUNCS prim_ceil, prim_floor, prim_float, prim_sqrt,  \
        prim_pow, prim_frand, prim_sin, prim_cos, prim_tan, prim_asin,   \
        prim_acos, prim_atan, prim_atan2, prim_exp, prim_log, prim_log10,\
	prim_fabs, prim_strtof, prim_ftostr, prim_fmod, prim_modf,       \
	prim_pi, prim_round, prim_dist3d, prim_xyz_to_polar,             \
	prim_polar_to_xyz, prim_pow, prim_diff3, prim_epsilon,           \
        prim_ftostrc

#define PRIMS_FLOAT_NAMES "CEIL", "FLOOR", "FLOAT", "SQRT", \
        "POW", "FRAND", "SIN", "COS", "TAN", "ASIN",        \
	"ACOS", "ATAN", "ATAN2", "EXP", "LOG", "LOG10",     \
	"FABS", "STRTOF", "FTOSTR", "FMOD", "MODF",         \
	"PI", "ROUND", "DIST3D", "XYZ_TO_POLAR",            \
	"POLAR_TO_XYZ", "^", "DIFF3", "EPSILON", "FTOSTRC"  

#define PRIMLIST_FLOAT  { "CEIL",          LM1, 1, prim_ceil },         \
                        { "FLOOR",         LM1, 1, prim_floor },        \
                        { "FLOAT",         LM1, 1, prim_float },        \
                        { "SQRT",          LM1, 1, prim_sqrt },         \
                        { "POW",           LM1, 2, prim_pow },          \
                        { "FRAND",         LM1, 0, prim_frand },        \
                        { "SIN",           LM1, 1, prim_sin },          \
                        { "COS",           LM1, 1, prim_cos },          \
                        { "TAN",           LM1, 1, prim_tan },          \
                        { "ASIN",          LM1, 1, prim_asin },         \
                        { "ACOS",          LM1, 1, prim_acos },         \
                        { "ATAN",          LM1, 1, prim_atan },         \
                        { "ATAN2",         LM1, 2, prim_atan2 },        \
                        { "EXP",           LM1, 1, prim_exp },          \
                        { "LOG",           LM1, 1, prim_log },          \
                        { "LOG10",         LM1, 1, prim_log10 },        \
                        { "FABS",          LM1, 1, prim_fabs },         \
                        { "STRTOF",        LM1, 1, prim_strtof },       \
                        { "FTOSTR",        LM1, 1, prim_ftostr },       \
                        { "FMOD",          LM1, 2, prim_fmod },         \
                        { "MODF",          LM1, 1, prim_modf },         \
                        { "PI",            LM1, 0, prim_pi },           \
                        { "ROUND",         LM1, 2, prim_round },        \
                        { "DIST3D",        LM1, 3, prim_dist3d },       \
                        { "XYZ_TO_POLAR",  LM1, 3, prim_xyz_to_polar }, \
                        { "POLAR_TO_XYZ",  LM1, 3, prim_polar_to_xyz }, \
                        { "^",             LM1, 2, prim_pow },          \
                        { "DIFF3",         LM1, 6, prim_diff3 },        \
                        { "EPSILON",       LM1, 0, prim_epsilon },      \
                        { "FTOSTRC",       LM1, 1, prim_ftostrc }
   
   
#define PRIMS_FLOAT_CNT 30

