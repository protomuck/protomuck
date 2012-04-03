#ifndef P_REGEX_H
#define P_REGEX_H

#ifdef PCRE_SUPPORT

#define MUF_RE_ICASE      1
#define MUF_RE_ALL        2
#define MUF_RE_EXTENDED   4

#define MUF_RE_ICASE_STR     "1"
#define MUF_RE_ALL_STR       "2"
#define MUF_RE_EXTENDED_STR  "4"

/* The regmatch_* macros are convenience wrappers for simple text matching
 * scenarios that don't require capturing. regmatch_re_get returns a null
 * pointer if fed a null pattern, and regmatch_exec performs a strcmp against
 * an empty string if fed a null muf_re pointer.
 *
 * If you want to use pcre_study on a value returned by regmatch_re_get, you'll
 * need to make sure it didn't return a null pointer.
 * 
 * *muf_re regmatch_re_get(const char* pattern, int flags, const char* errstr)
 *     int   regmatch_exec(muf_re* re, const char *inputstr)
 */
#define regmatch_re_get(x,y,z) x ? (muf_re_get(x,y,z)) : NULL

#define regmatch_exec(x,y) x ? (pcre_exec(x->re,x->extra,y,strlen(y),0,0,NULL,0)) \
                               : (strcmp("",y) == 0 ? 1 : PCRE_ERROR_NOMATCH )

extern void prim_regexp(PRIM_PROTOTYPE);
extern void prim_regsub(PRIM_PROTOTYPE);
extern void prim_regmatch(PRIM_PROTOTYPE);
extern void prim_array_regmatchkey(PRIM_PROTOTYPE);
extern void prim_array_regmatchval(PRIM_PROTOTYPE);
extern void prim_array_regfilter_prop(PRIM_PROTOTYPE);
extern void prim_regfind_array(PRIM_PROTOTYPE);
extern void prim_regfindnext(PRIM_PROTOTYPE);
extern void prim_array_regsub(PRIM_PROTOTYPE);

#define PRIMLIST_REGEX { "REGEXP",                 LM1,   3,     prim_regexp               }, \
                       { "REGSUB",                 LM1,   4,     prim_regsub               }, \
                       { "REGMATCH",               LM1,   3,     prim_regmatch             }, \
                       { "ARRAY_REGMATCHKEY",      LM1,   3,     prim_array_regmatchkey    }, \
                       { "ARRAY_REGMATCHVAL",      LM1,   3,     prim_array_regmatchval    }, \
                       { "ARRAY_REGFILTER_PROP",   LM1,   4,     prim_array_regfilter_prop }, \
                       { "REGFIND_ARRAY",          LM2,   4,     prim_regfind_array        }, \
                       { "REGFINDNEXT",            LM2,   5,     prim_regfindnext          }, \
                       { "ARRAY_REGSUB",           LM1,   4,     prim_array_regsub         }

#define PRIMS_REGEX_CNT 9

#else /* PCRE_SUPPORT */

#define PRIMS_REGEX_CNT 0

#endif /* PCRE_SUPPORT */

#endif /* P_REGEX_H */
