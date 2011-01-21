#ifndef P_REGEX_H
#define P_REGEX_H

#ifndef WIN_VC

#define MUF_RE_ICASE      1
#define MUF_RE_ALL        2
#define MUF_RE_EXTENDED     4

#define MUF_RE_ICASE_STR "1"
#define MUF_RE_ALL_STR   "2"

extern void prim_regexp(PRIM_PROTOTYPE);
extern void prim_regsub(PRIM_PROTOTYPE);
extern void prim_regmatch(PRIM_PROTOTYPE);

#define PRIMS_REGEX_FUNCS prim_regexp, prim_regsub, prim_regmatch

#define PRIMS_REGEX_NAMES "REGEXP", "REGSUB", "REGMATCH"

#define PRIMS_REGEX_CNT 3

#else

#define PRIMS_REGEX_CNT 0

#endif /* WIN_VC */


#endif /* P_REGEX_H */
