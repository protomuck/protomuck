/* Primitives Package */

#include "copyright.h"
#include "config.h"

#ifdef PCRE_SUPPORT

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>

#ifdef HAVE_PCREINCDIR
# include <pcre/pcre.h>
#else
#  include <pcre.h>
#endif

#include "db.h"
#include "tune.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "strings.h"
#include "interp.h"
#include "props.h"

#define MUF_RE_CACHE_ITEMS 64

extern dbref ref;
extern int result;
extern int prop_read_perms(dbref player, dbref obj, const char *name, int mlev);
extern int prop_write_perms(dbref player, dbref obj, const char *name,
                            int mlev);

static struct inst *oper1, *oper2, *oper3, *oper4, *oper5, *oper6;
static struct inst temp1, temp2;
static char buf[BUFFER_LEN];

/*
   Non-PCRE regex was deprecated in FBMUCK and we have followed suit. While
   it would be nice to support both PCRE and non-PCRE flavored regexps, this
   would create some messy incompatibilities in softcode to keep track of.
   Other languages such as PHP have been dropping support for non-PCRE regex as
   well, so this seems the right way to go.
   
   -brevantes
*/

typedef struct {
    struct shared_string *pattern;
    int flags;
    /* regex_t re; */
    pcre* re;
} muf_re;

static muf_re muf_re_cache[MUF_RE_CACHE_ITEMS];

muf_re* muf_re_get(struct shared_string* pattern, int flags, const char** errmsg)
{
    int     idx = (hash(DoNullInd(pattern), MUF_RE_CACHE_ITEMS) + flags) % MUF_RE_CACHE_ITEMS;
    muf_re* re  = &muf_re_cache[idx];
    int     erroff;

    if (re->pattern)
    {
        if ((flags != re->flags) || strcmp(DoNullInd(pattern), DoNullInd(re->pattern)))
        {
            pcre_free(re->re);

            if (re->pattern && (--re->pattern->links == 0))
                free((void *)re->pattern);
        }
        else
            return re;
    }

    re->re = pcre_compile(DoNullInd(pattern), flags, errmsg, &erroff, NULL);
    if (re->re == NULL)
    {
        re->pattern = NULL;
        return NULL;
    }

    re->pattern = pattern;

    if (pattern)
        re->pattern->links++;

    re->flags   = flags;

    return re;
}

const char* muf_re_error(int err)
{
    switch(err)
    {
        case PCRE_ERROR_NOMATCH:        return "No matches";
        case PCRE_ERROR_NULL:           return "Internal error: NULL arg to pcre_exec()";
        case PCRE_ERROR_BADOPTION:      return "Invalid regexp option.";
        case PCRE_ERROR_BADMAGIC:       return "Internal error: bad magic number.";
        case PCRE_ERROR_UNKNOWN_NODE:   return "Internal error: bad regexp node.";
        case PCRE_ERROR_NOMEMORY:       return "Out of memory.";
        case PCRE_ERROR_NOSUBSTRING:    return "No substring.";
        case PCRE_ERROR_MATCHLIMIT:     return "Match recursion limit exceeded.";
        case PCRE_ERROR_CALLOUT:        return "Internal error: callout error.";
        default:            return "Unknown error";
    }
}

#define MATCH_ARR_SIZE 30

void
prim_regexp(PRIM_PROTOTYPE)
{
    stk_array*  nu_val  = 0;
    stk_array*  nu_idx  = 0;
    int         matches[MATCH_ARR_SIZE];
    muf_re*     re;
    char*       text;
    int         flags   = 0;
    int         len, i;
    int         matchcnt = 0;
    const char* errstr;

    CHECKOP(3);

    oper3 = POP(); /* int:Flags */
    oper2 = POP(); /* str:Pattern */
    oper1 = POP(); /* str:Text */

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument (1)");
    if (oper2->type != PROG_STRING)
        abort_interp("Non-string argument (2)");
    if (oper3->type != PROG_INTEGER)
        abort_interp("Non-integer argument (3)");
    if (!oper2->data.string)
        abort_interp("Empty string argument (2)");

    if (oper3->data.number & MUF_RE_ICASE)
        flags |= PCRE_CASELESS;
    if (oper3->data.number & MUF_RE_EXTENDED)
        flags |= PCRE_EXTENDED;

    if ((re = muf_re_get(oper2->data.string, flags, &errstr)) == NULL)
        abort_interp(errstr);

    text    = (char *)DoNullInd(oper1->data.string);
    len     = strlen(text);

    if ((matchcnt = pcre_exec(re->re, NULL, text, len, 0, 0, matches, MATCH_ARR_SIZE)) < 0)
    {
        if (matchcnt != PCRE_ERROR_NOMATCH)
        {
            abort_interp(muf_re_error(matchcnt));
        }

        if (((nu_val = new_array_packed(0)) == NULL) ||
            ((nu_idx = new_array_packed(0)) == NULL))
        {
            if (nu_val != NULL)
                array_free(nu_val);

            if (nu_idx != NULL)
                array_free(nu_idx);

            abort_interp("Out of memory");
        }
    }
    else
    {
        if (((nu_val = new_array_packed(matchcnt)) == NULL) ||
            ((nu_idx = new_array_packed(matchcnt)) == NULL))
        {
            if (nu_val != NULL)
                array_free(nu_val);

            if (nu_idx != NULL)
                array_free(nu_idx);

            abort_interp("Out of memory");
        }

        for(i = 0; i < matchcnt; i++)
        {
            int substart = matches[i*2];
            int subend = matches[i*2+1];
            struct inst idx, val;
            stk_array*  nu;

            if ((substart >= 0) && (subend >= 0) && (substart < len))
                snprintf(buf, BUFFER_LEN, "%.*s", (int)(subend - substart), &text[substart]);
            else
                buf[0] = '\0';

            idx.type        = PROG_INTEGER;
            idx.data.number = i;
            val.type        = PROG_STRING;
            val.data.string = alloc_prog_string(buf);

            array_setitem(&nu_val, &idx, &val);

            CLEAR(&idx);
            CLEAR(&val);

            if ((nu = new_array_packed(2)) == NULL)
            {
                array_free(nu_val);
                array_free(nu_idx);

                abort_interp("Out of memory");
            }

            idx.type        = PROG_INTEGER;
            idx.data.number = 0;
            val.type        = PROG_INTEGER;
            val.data.number = substart + 1;

            array_setitem(&nu, &idx, &val);

            CLEAR(&idx);
            CLEAR(&val);

            idx.type        = PROG_INTEGER;
            idx.data.number = 1;
            val.type        = PROG_INTEGER;
            val.data.number = subend - substart;

            array_setitem(&nu, &idx, &val);

            CLEAR(&idx);
            CLEAR(&val);

            idx.type        = PROG_INTEGER;
            idx.data.number = i;
            val.type        = PROG_ARRAY;
            val.data.array  = nu;

            array_setitem(&nu_idx, &idx, &val);

            CLEAR(&idx);
            CLEAR(&val);
        }
    }

    CLEAR(oper3);
    CLEAR(oper2);
    CLEAR(oper1);

    PushArrayRaw(nu_val);
    PushArrayRaw(nu_idx);
}

void
prim_regsub(PRIM_PROTOTYPE)
{
    int         matches[MATCH_ARR_SIZE];
    int         flags       = 0;
    char*       write_ptr   = buf;
    int         write_left  = BUFFER_LEN - 1;
    muf_re*     re;
    pcre_extra* extra = NULL;
    char*       text;
    char*       textstart;
    const char* errstr;
    int         matchcnt, len;

    CHECKOP(4);

    oper4 = POP(); /* int:Flags */
    oper3 = POP(); /* str:Replace */
    oper2 = POP(); /* str:Pattern */
    oper1 = POP(); /* str:Text */

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument (1)");
    if (oper2->type != PROG_STRING)
        abort_interp("Non-string argument (2)");
    if (oper3->type != PROG_STRING)
        abort_interp("Non-string argument (3)");
    if (oper4->type != PROG_INTEGER)
        abort_interp("Non-integer argument (4)");
    if (!oper2->data.string)
        abort_interp("Empty string argument (2)");

    if (oper4->data.number & MUF_RE_ICASE)
        flags |= PCRE_CASELESS;
    if (oper4->data.number & MUF_RE_EXTENDED)
        flags |= PCRE_EXTENDED;

    if ((re = muf_re_get(oper2->data.string, flags, &errstr)) == NULL)
        abort_interp(errstr);

    if (oper4->data.number & MUF_RE_ALL) {
        /* User requested a recursive pattern search. This generally means
         * pcre_exec will be called at least twice unless the pattern doesn't
         * exist in the string at all. Presence of this option suggests that
         * the user anticipates the pattern occurring at least once, so it's
         * safest to go ahead and study the pattern. -brevantes */
        extra = pcre_study(re->re, 0, &errstr);
        if (errstr)
            abort_interp(errstr);
    }


    textstart = text = (char *)DoNullInd(oper1->data.string);

    len = strlen(textstart);
    while((*text != '\0') && (write_left > 0))
    {
        if ((matchcnt = pcre_exec(re->re, extra, textstart, len, text-textstart, 0, matches, MATCH_ARR_SIZE)) < 0)
        {
            if (matchcnt != PCRE_ERROR_NOMATCH)
            {
                abort_interp(muf_re_error(matchcnt));
            }

            while((write_left > 0) && (*text != '\0'))
            {
                *write_ptr++ = *text++;
                write_left--;
            }

            break;
        }
        else
        {
            int         allstart    = matches[0];
            int         allend      = matches[1];
            int         substart    = -1;
            int         subend      = -1;
            char*       read_ptr    = (char *)DoNullInd(oper3->data.string);
            int         count;

            for(count = allstart-(text-textstart); (write_left > 0) && (*text != '\0') && (count > 0); count--)
            {
                *write_ptr++ = *text++;
                write_left--;
            }

            while((write_left > 0) && (*read_ptr != '\0'))
            {
                if (*read_ptr == '\\')
                {
                    if (!isdigit(*(++read_ptr)))
                    {
                        *write_ptr++ = *read_ptr++;
                        write_left--;
                    }
                    else
                    {
                        int idx = (*read_ptr++) - '0';

                        if ((idx < 0) || (idx >= matchcnt))
                        {
                            abort_interp("Invalid \\subexp in substitution string. (3)");
                        }

                        substart = matches[idx*2];
                        subend = matches[idx*2+1];

                        if ((substart >= 0) && (subend >= 0) && (substart < len))
                        {
                            char* ptr = &textstart[substart];

                            count = subend - substart;

                            if (count > write_left)
                            {
                                abort_interp("Operation would result in overflow");
                            }

                            for(; (write_left > 0) && (count > 0) && (*ptr != '\0'); count--)
                            {
                                *write_ptr++ = *ptr++;
                                write_left--;
                            }
                        }
                    }
                }
                else
                {
                    *write_ptr++ = *read_ptr++;
                    write_left--;
                }
            }

            for(count = allend - allstart; (*text != '\0') && (count > 0); count--)
                text++;

            if (allstart == allend && *text) {
                *write_ptr++ = *text++;
                write_left--;
            }
        }

        if ((oper4->data.number & MUF_RE_ALL) == 0)
        {
            while((write_left > 0) && (*text != '\0'))
            {
                *write_ptr++ = *text++;
                write_left--;
            }

            break;
        }
    }

    if (*text != '\0')
        abort_interp("Operation would result in overflow");

    *write_ptr = '\0';

    free(extra);

    CLEAR(oper4);
    CLEAR(oper3);
    CLEAR(oper2);
    CLEAR(oper1);

    PushString(buf);
}


void
prim_array_regsub(PRIM_PROTOTYPE)
{
    struct inst *in;
    stk_array *arr;
    stk_array *nw;
    int         matches[MATCH_ARR_SIZE];
    int         flags       = 0;
    char*       write_ptr   = buf;
    int         write_left  = BUFFER_LEN - 1;
    muf_re*     re;
    pcre_extra* extra = NULL;
    char*       text;
    char*       textstart;
    const char* errstr;
    int         matchcnt, len;

    CHECKOP(4);

    oper4 = POP(); /* int:Flags */
    oper3 = POP(); /* str:Replace */
    oper2 = POP(); /* str:Pattern */
    oper1 = POP(); /* str:Text */

    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array of strings. (1)");
    if (!array_is_homogenous(oper1->data.array, PROG_STRING))
        abort_interp("Argument not an array of strings. (1)");
    if (oper2->type != PROG_STRING)
        abort_interp("Non-string argument (2)");
    if (oper3->type != PROG_STRING)
        abort_interp("Non-string argument (3)");
    if (oper4->type != PROG_INTEGER)
        abort_interp("Non-integer argument (4)");
    if (!oper2->data.string)
        abort_interp("Empty string argument (2)");

    if (oper4->data.number & MUF_RE_ICASE)
        flags |= PCRE_CASELESS;
    if (oper4->data.number & MUF_RE_EXTENDED)
        flags |= PCRE_EXTENDED;

    if ((re = muf_re_get(oper2->data.string, flags, &errstr)) == NULL)
        abort_interp(errstr);



    nw = new_array_dictionary();
    arr = oper1->data.array;

    if ((oper4->data.number & MUF_RE_ALL ) || array_count(arr) > 2) {
        /* Study the pattern if the user requested recursive substitution, or
         * if the input array contains at least three items. */
        extra = pcre_study(re->re, 0, &errstr);
        if (errstr)
            abort_interp(errstr);
    }

    if (array_first(arr, &temp1)) {
        do {
            write_ptr = buf;
            write_left = BUFFER_LEN - 1;

            in = array_getitem(arr, &temp1);
            textstart = text = (char *)DoNullInd(in->data.string);
            len = strlen(textstart);

            while((*text != '\0') && (write_left > 0))
            {
                if ((matchcnt = pcre_exec(re->re, extra, textstart, len,
                                          text-textstart, 0, matches,
                                          MATCH_ARR_SIZE)) < 0)
                {
                    if (matchcnt != PCRE_ERROR_NOMATCH)
                    {
                        abort_interp(muf_re_error(matchcnt));
                    }

                    while((write_left > 0) && (*text != '\0'))
                    {
                        *write_ptr++ = *text++;
                        write_left--;
                    }

                    break;
                }
                else
                {
                    int         allstart    = matches[0];
                    int         allend      = matches[1];
                    int         substart    = -1;
                    int         subend      = -1;
                    char*       read_ptr    = (char *)DoNullInd(oper3->data.string);
                    int         count;

                    for(count = allstart-(text-textstart);
                                (write_left > 0) && (*text != '\0') && (count > 0);
                                count--)
                    {
                        *write_ptr++ = *text++;
                        write_left--;
                    }

                    while((write_left > 0) && (*read_ptr != '\0'))
                    {
                        if (*read_ptr == '\\')
                        {
                            if (!isdigit(*(++read_ptr)))
                            {
                                *write_ptr++ = *read_ptr++;
                                write_left--;
                            }
                            else
                            {
                                int idx = (*read_ptr++) - '0';

                                if ((idx < 0) || (idx >= matchcnt))
                                {
                                    abort_interp("Invalid \\subexp in substitution string. (3)");
                                }

                                substart = matches[idx*2];
                                subend = matches[idx*2+1];

                                if ((substart >= 0) && (subend >= 0) && (substart < len))
                                {
                                    char* ptr = &textstart[substart];

                                    count = subend - substart;

                                    if (count > write_left)
                                    {
                                        abort_interp("Operation would result in overflow");
                                    }

                                    for(; (write_left > 0) && (count > 0) && (*ptr != '\0'); count--)
                                    {
                                        *write_ptr++ = *ptr++;
                                        write_left--;
                                    }
                                }
                            }
                        }
                        else
                        {
                            *write_ptr++ = *read_ptr++;
                            write_left--;
                        }
                    }

                    for(count = allend - allstart; (*text != '\0') && (count > 0); count--)
                        text++;

                    if (allstart == allend && *text) {
                        *write_ptr++ = *text++;
                        write_left--;
                    }
                }

                if ((oper4->data.number & MUF_RE_ALL) == 0)
                {
                    while((write_left > 0) && (*text != '\0'))
                    {
                        *write_ptr++ = *text++;
                        write_left--;
                    }

                    break;
                }
            }

            if (*text != '\0')
                abort_interp("Operation would result in overflow");

            *write_ptr = '\0';

            temp2.type = PROG_STRING;
            temp2.data.string = alloc_prog_string(buf);

            array_setitem(&nw, &temp1, &temp2);
            CLEAR(&temp2);
        } while (array_next(arr, &temp1));
    }

    free(extra);

    CLEAR(oper4);
    CLEAR(oper3);
    CLEAR(oper2);
    CLEAR(oper1);

    PushArrayRaw(nw);
}

void
prim_regmatch(PRIM_PROTOTYPE)
{
    muf_re*     re;
    char*       text;
    int         flags;
    int         matchcnt = 0;
    const char* errstr = NULL;
    int         result = 0;

    CHECKOP(3);
    oper3 = POP(); /* int:Flags */
    oper2 = POP(); /* str:Pattern */
    oper1 = POP(); /* str:Text */

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument (1)");
    if (oper2->type != PROG_STRING)
        abort_interp("Non-string argument (2)");
    if (oper3->type != PROG_INTEGER)
        abort_interp("Non-integer argument (3)");

    /* This primitive is for matching, not capturing. Using user-supplied
     * parenthesis for anything other than grouping purposes is therefore a
     * waste of resources, but most casual regex users won't know about the
     * non-capturing parenthesis alternatives available when PCRE_EXTENDED is
     * set. Since this is the case, we'll enable PCRE_NO_AUTO_CAPTURE as a
     * default option to optimize the majority of lazy user input.
     * -brevantes */

    flags = PCRE_NO_AUTO_CAPTURE;

    if (oper3->data.number & MUF_RE_ICASE)
        flags |= PCRE_CASELESS;
    if (oper3->data.number & MUF_RE_EXTENDED)
        flags |= PCRE_EXTENDED;

    re = regmatch_re_get(oper2->data.string, flags, &errstr);
    if (errstr)
        abort_interp(errstr)

    text    = (char *)DoNullInd(oper1->data.string);

    if ((matchcnt = regmatch_exec(re, NULL, text)) < 0) {
        if (matchcnt != PCRE_ERROR_NOMATCH)
        {
            abort_interp(muf_re_error(matchcnt));
        }
    }
    else
        /* Returning matchcnt isn't useful in match-only mode. */
        result = 1;
    
    CLEAR(oper3);
    CLEAR(oper2);
    CLEAR(oper1);

    PushInt(result);
}

void
prim_array_regmatchkey(PRIM_PROTOTYPE)
{
    struct inst *in;
    stk_array *arr;
    stk_array *nw;
    muf_re* re;
    pcre_extra* extra = NULL;
    char* text;
    int flags;
    int matchcnt = 0;
    const char* errstr = NULL;

    CHECKOP(3);
    oper3 = POP();              /* int  pcreflags */
    oper2 = POP();              /* str  pattern */
    oper1 = POP();              /* arr  Array */
    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");
    if (oper2->type != PROG_STRING)
        abort_interp("Argument not a string pattern. (2)");
    if (oper3->type != PROG_INTEGER)
        abort_interp("Non-integer argument (3)");

    flags = PCRE_NO_AUTO_CAPTURE;

    if (oper3->data.number & MUF_RE_ICASE)
        flags |= PCRE_CASELESS;
    if (oper3->data.number & MUF_RE_EXTENDED)
        flags |= PCRE_EXTENDED;

    re = regmatch_re_get(oper2->data.string, flags, &errstr);
    if (errstr)
        abort_interp(errstr)

    nw = new_array_dictionary();
    arr = oper1->data.array;

    if (re && array_count(arr) > 2) {
        /* This pattern is getting used 3 or more times, let's study it. A null
         * return is okay, that just means there's nothing to optimize. */
        extra = pcre_study(re->re, 0, &errstr);
        if (errstr)
            abort_interp(errstr);
    }

    if (array_first(arr, &temp1)) {
        do {
            if (temp1.type == PROG_STRING) {
                text    = (char *)DoNullInd(temp1.data.string);

                if ((matchcnt = regmatch_exec(re, NULL, text)) < 0) {
                    if (matchcnt != PCRE_ERROR_NOMATCH)
                        abort_interp(muf_re_error(matchcnt));
                } else {
                    in = array_getitem(arr, &temp1);
                    array_setitem(&nw, &temp1, in);
                }
            }
        } while (array_next(arr, &temp1));
    }

    free(extra);

    CLEAR(oper3);
    CLEAR(oper2);
    CLEAR(oper1);

    PushArrayRaw(nw);
}

void
prim_array_regmatchval(PRIM_PROTOTYPE)
{
    struct inst *in;
    stk_array *arr;
    stk_array *nw;
    muf_re* re;
    pcre_extra* extra = NULL;
    char* text;
    int flags;
    int matchcnt = 0;
    const char* errstr = NULL;

    CHECKOP(3);
    oper3 = POP();              /* int  pcreflags */
    oper2 = POP();              /* str  pattern */
    oper1 = POP();              /* arr  Array */
    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");
    if (oper2->type != PROG_STRING)
        abort_interp("Argument not a string pattern. (2)");
    if (oper3->type != PROG_INTEGER)
        abort_interp("Non-integer argument (3)");

    flags = PCRE_NO_AUTO_CAPTURE;

    if (oper3->data.number & MUF_RE_ICASE)
        flags |= PCRE_CASELESS;
    if (oper3->data.number & MUF_RE_EXTENDED)
        flags |= PCRE_EXTENDED;

    re = regmatch_re_get(oper2->data.string, flags, &errstr);
    if (errstr)
        abort_interp(errstr)

    nw = new_array_dictionary();
    arr = oper1->data.array;

    if (re && array_count(arr) > 2) {
        /* This pattern is getting used 3 or more times, let's study it. A null
         * return is okay, that just means there's nothing to optimize. */
        extra = pcre_study(re->re, 0, &errstr);
        if (errstr)
            abort_interp(errstr);
    }

    if (array_first(arr, &temp1)) {
        do {
            in = array_getitem(arr, &temp1);
            if (in->type == PROG_STRING) {
                text    = (char *)DoNullInd(in->data.string);
                if ((matchcnt = regmatch_exec(re, NULL, text)) < 0) {
                    if (matchcnt != PCRE_ERROR_NOMATCH)
                        abort_interp(muf_re_error(matchcnt));
                } else {
                    array_setitem(&nw, &temp1, in);
                }
            } else if (in->type == PROG_OBJECT) {
                text    = (char *) NAME(in->data.objref);
                if ((matchcnt = regmatch_exec(re, NULL, text)) < 0) {
                    if (matchcnt != PCRE_ERROR_NOMATCH)
                        abort_interp(muf_re_error(matchcnt));
                } else {
                    array_setitem(&nw, &temp1, in);
                }
            }
        } while (array_next(arr, &temp1));
    }

    free(extra);

    CLEAR(oper3);
    CLEAR(oper2);
    CLEAR(oper1);

    PushArrayRaw(nw);
}


void
prim_array_regfilter_prop(PRIM_PROTOTYPE)
{
    char buf[BUFFER_LEN];
    struct inst *in;
    stk_array *arr;
    stk_array *nu;
    char* prop;
    const char* ptr;
    muf_re* re;
    pcre_extra* extra = NULL;
    int flags;
    int matchcnt = 0;
    const char* errstr = NULL;

    CHECKOP(4);
    oper4 = POP();    /* int     pcreflags */
    oper3 = POP();    /* str     pattern */
    oper2 = POP();    /* str     propname */
    oper1 = POP();    /* refarr  Array */

    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");
    if (!array_is_homogenous(oper1->data.array, PROG_OBJECT))
        abort_interp("Argument not an array of dbrefs. (1)");
    if (oper2->type != PROG_STRING || !oper2->data.string)
        abort_interp("Argument not a non-null string. (2)");
    if (oper3->type != PROG_STRING)
        abort_interp("Argument not a string pattern. (3)");
    if (oper4->type != PROG_INTEGER)
        abort_interp("Non-integer argument (4)");


    ptr = oper2->data.string->data;
    while ((ptr = index(ptr, PROPDIR_DELIMITER)))
        if (!(*(++ptr)))
            abort_interp("Cannot access a propdir directly.");
    nu = new_array_packed(0);
    arr = oper1->data.array;

    flags = PCRE_NO_AUTO_CAPTURE;

    if (oper4->data.number & MUF_RE_ICASE)
        flags |= PCRE_CASELESS;
    if (oper4->data.number & MUF_RE_EXTENDED)
        flags |= PCRE_EXTENDED;

    re = regmatch_re_get(oper3->data.string, flags, &errstr);
    if (errstr)
        abort_interp(errstr)

    if (re && array_count(arr) > 2) {
        /* This pattern is getting used 3 or more times, let's study it. A null
         * return is okay, that just means there's nothing to optimize. */
        extra = pcre_study(re->re, 0, &errstr);
        if (errstr)
            abort_interp(errstr);
    }

    prop = (char *) DoNullInd(oper2->data.string);
    if (array_first(arr, &temp1)) {
        do {
            in = array_getitem(arr, &temp1);
            if (valid_object(in)) {
                ref = in->data.objref;
                CHECKREMOTE(ref);
                if (prop_read_perms(ProgUID, ref, prop, mlev)) {
                    ptr = get_property_class(ref, prop);
                    if (ptr)
                        strcpy(buf, ptr);
                    else
                        strcpy(buf, "");
                    if ((matchcnt = regmatch_exec(re, NULL, buf)) < 0) {
                        if (matchcnt != PCRE_ERROR_NOMATCH)
                            abort_interp(muf_re_error(matchcnt));
                    } else {
                        array_appenditem(&nu, in);
                    }
                }
            }
        } while (array_next(arr, &temp1));
    }

    free(extra);

    CLEAR(oper4);
    CLEAR(oper3);
    CLEAR(oper2);
    CLEAR(oper1);
    PushArrayRaw(nu);
}

void
prim_regfind_array(PRIM_PROTOTYPE)
{
    struct flgchkdat check;
    dbref ref, who;
    const char *name;
    stk_array *nw;
    muf_re* re;
    pcre_extra* extra = NULL;
    char* text;
    int flags;
    int matchcnt = 0;
    const char* errstr = NULL;

    CHECKOP(4);
    oper4 = POP();              /* int:pcreflags */
    oper3 = POP();              /* str:objflags */
    oper2 = POP();              /* str:namepattern */
    oper1 = POP();              /* ref:owner */

    if (mlev < LMAGE)
        abort_interp("MAGE prim.");
    if (oper4->type != PROG_INTEGER)
        abort_interp("Non-integer argument (4)");
    if (oper3->type != PROG_STRING)
        abort_interp("Expected string argument. (3)");
    if (oper2->type != PROG_STRING)
        abort_interp("Expected string argument. (2)");
    if (oper1->type != PROG_OBJECT)
        abort_interp("Expected dbref argument. (1)");
    if (oper1->data.objref < NOTHING || oper1->data.objref >= db_top)
        abort_interp("Bad object. (1)");


    flags = PCRE_NO_AUTO_CAPTURE;

    if (oper4->data.number & MUF_RE_ICASE)
        flags |= PCRE_CASELESS;
    if (oper4->data.number & MUF_RE_EXTENDED)
        flags |= PCRE_EXTENDED;

    re = regmatch_re_get(oper2->data.string, flags, &errstr);
    if (errstr)
        abort_interp(errstr)

    /* We're scanning a chunk of the DB, so studying should pay off.
     * A null return is fine, it just means we can't optimize further. */
    if (re)
        extra = pcre_study(re->re, 0, &errstr);
    if (errstr)
        abort_interp(errstr);

    who = oper1->data.objref;
    name = DoNullInd(oper2->data.string);

    init_checkflags(PSafe, DoNullInd(oper3->data.string), &check);
    nw = new_array_packed(0);

    /* The "result = array_appendref" stuff was copied from find_array. I'm
     * making sure these alterations work as-is before attempting to remove it.
     * -brevantes */
    for (ref = (dbref) 0; ref < db_top; ref++) {
        if (((who == NOTHING) ? 1 : (OWNER(ref) == who)) &&
            checkflags(ref, check) && NAME(ref)) {
            if (!*name)
                result = array_appendref(&nw, ref);
            else
                text = (char *)NAME(ref);
                if ((matchcnt = regmatch_exec(re, NULL, text)) < 0)
                {
                    if (matchcnt != PCRE_ERROR_NOMATCH)
                        abort_interp(muf_re_error(matchcnt));
                } else {
                    result = array_appendref(&nw, ref);
                }
        }
    }

    free(extra);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    CLEAR(oper4);
    PushArrayRaw(nw);
}

void
prim_regfindnext(PRIM_PROTOTYPE)
{
    struct flgchkdat check;
    dbref who, item, ref, i;
    const char *name;
    muf_re* re;
    pcre_extra* extra = NULL;
    char* text;
    int flags;
    int matchcnt = 0;
    const char* errstr = NULL;

    CHECKOP(5);
    oper5 = POP();              /* int:pcreflags */
    oper4 = POP();              /* str:objflags */
    oper3 = POP();              /* str:namepattern */
    oper2 = POP();              /* ref:owner */
    oper1 = POP();              /* ref:currobj */

    if (oper5->type != PROG_INTEGER)
        abort_interp("Non-integer argument (5)");
    if (oper4->type != PROG_STRING)
        abort_interp("Expected string argument. (4)");
    if (oper3->type != PROG_STRING)
        abort_interp("Expected string argument. (3)");
    if (oper2->type != PROG_OBJECT)
        abort_interp("Expected dbref argument. (2)");
    if (oper2->data.objref < NOTHING || oper2->data.objref >= db_top)
        abort_interp("Bad object. (2)");
    if (oper1->type != PROG_OBJECT)
        abort_interp("Expected dbref argument. (1)");
    if (oper1->data.objref < NOTHING || oper1->data.objref >= db_top)
        abort_interp("Bad object. (1)");
    if (oper2->data.objref != NOTHING &&
        Typeof(oper2->data.objref) == TYPE_GARBAGE)
        abort_interp("Owner dbref is garbage. (2)");

    item = oper1->data.objref;
    who = oper2->data.objref;
    name = DoNullInd(oper3->data.string);

    if (mlev < 2)
        abort_interp("Permission denied.  Requires at least Mucker Level 2.");

    if (mlev < 3) {
        if (who == NOTHING) {
            abort_interp
                ("Permission denied.  Owner inspecific searches require Mucker Level 3.");
        } else if (who != ProgUID) {
            abort_interp
                ("Permission denied.  Searching for other people's stuff requires Mucker Level 3.");
        }
    }

    flags = PCRE_NO_AUTO_CAPTURE;

    if (oper5->data.number & MUF_RE_ICASE)
        flags |= PCRE_CASELESS;
    if (oper5->data.number & MUF_RE_EXTENDED)
        flags |= PCRE_EXTENDED;

    re = regmatch_re_get(oper3->data.string, flags, &errstr);
    if (errstr)
        abort_interp(errstr)

    /* We're scanning a chunk of the DB, so studying should pay off.
     * A null return is fine, it just means we can't optimize further. */
    if (re) {
        extra = pcre_study(re->re, 0, &errstr);
        if (errstr)
            abort_interp(errstr);
    }

    if (item == NOTHING) {
        item = 0;
    } else {
        item++;
    }

    ref = NOTHING;
    init_checkflags(PSafe, DoNullInd(oper4->data.string), &check);
    for (i = item; i < db_top; i++) {
        if ((who == NOTHING || OWNER(i) == who) &&
            checkflags(i, check) && NAME(i)) {
            if (!*name) {
                ref = i;
                break;
            } else {
                text = (char *) NAME(i);
                if ((matchcnt = regmatch_exec(re, NULL, text)) < 0) {
                    if (matchcnt != PCRE_ERROR_NOMATCH)
                        abort_interp(muf_re_error(matchcnt));
                } else {
                    ref = i;
                    break;
                }
            }
        }
    }

    free(extra);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    CLEAR(oper4);
    CLEAR(oper5);

    PushObject(ref);
}


#endif /* PCRE_SUPPORT */
