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
extern void prim_inf(PRIM_PROTOTYPE);
extern void prim_round(PRIM_PROTOTYPE);
extern void prim_xyz_to_polar(PRIM_PROTOTYPE);
extern void prim_polar_to_xyz(PRIM_PROTOTYPE);
extern void prim_dist3d(PRIM_PROTOTYPE);
extern void prim_diff3(PRIM_PROTOTYPE);

#define PRIMS_FLOAT_FUNCS prim_ceil, prim_floor, prim_float, prim_sqrt,  \
        prim_pow, prim_frand, prim_sin, prim_cos, prim_tan, prim_asin,   \
        prim_acos, prim_atan, prim_atan2, prim_exp, prim_log, prim_log10,\
	prim_fabs, prim_strtof, prim_ftostr, prim_fmod, prim_modf,       \
	prim_pi, prim_inf, prim_round, prim_dist3d, prim_xyz_to_polar,  \
	prim_polar_to_xyz, prim_pow, prim_diff3

#define PRIMS_FLOAT_NAMES "CEIL", "FLOOR", "FLOAT", "SQRT", \
        "POW", "FRAND", "SIN", "COS", "TAN", "ASIN",        \
	"ACOS", "ATAN", "ATAN2", "EXP", "LOG", "LOG10",     \
	"FABS", "STRTOF", "FTOSTR", "FMOD", "MODF",         \
	"PI", "INF", "ROUND", "DIST3D", "XYZ_TO_POLAR",     \
	"POLAR_TO_XYZ", "^", "DIFF3"

#define PRIMS_FLOAT_CNT 29
