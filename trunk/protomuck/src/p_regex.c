/* Primitives Package */

#include "copyright.h"
#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <regex.h>
#include "db.h"
#include "tune.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "strings.h"
#include "interp.h"

#define MUF_RE_CACHE_ITEMS 64

static struct inst *oper1, *oper2, *oper3, *oper4, *oper5, *oper6;
static char buf[BUFFER_LEN];

typedef struct {
    struct shared_string *pattern;
    int flags;
    regex_t re;
} muf_re;

static muf_re muf_re_cache[MUF_RE_CACHE_ITEMS];

muf_re *
muf_re_get(struct shared_string *pattern, int flags, int *err)
{
    int idx =
        (hash(DoNullInd(pattern), MUF_RE_CACHE_ITEMS) +
         flags) % MUF_RE_CACHE_ITEMS;
    muf_re *re = &muf_re_cache[idx];

    if (re->pattern) {
        if ((flags != re->flags)
            || strcmp(DoNullInd(pattern), DoNullInd(re->pattern))) {
            regfree(&re->re);

            if (re->pattern && (--re->pattern->links == 0))
                free((void *) re->pattern);
        } else
            return re;
    }

    if ((*err =
         regcomp(&re->re, DoNullInd(pattern), flags | REG_EXTENDED)) != 0) {
        re->pattern = NULL;
        return NULL;
    }

    re->pattern = pattern;
    re->pattern->links++;

    re->flags = flags;

    return re;
}

const char *
muf_re_error(int err)
{
    switch (err) {
    case REG_NOMATCH:
        return "No matches";
    case REG_BADPAT:
        return "Invalid regular expression";
    case REG_ECOLLATE:
        return "Invalid collating element referenced";
    case REG_ECTYPE:
        return "Invalid character class type referenced";
    case REG_EESCAPE:
        return "Trailing \\ in pattern";
    case REG_ESUBREG:
        return "Number in \\digit invalid or in error";
    case REG_EBRACK:
        return "[ ] imbalance";
    case REG_EPAREN:
        return "\\( \\) or ( ) imbalance";
    case REG_EBRACE:
        return "{ } imbalance";
    case REG_BADBR:
        return "Content of \\{ \\} invalid";
    case REG_ERANGE:
        return "Invalid endpoint in range expression";
    case REG_ESPACE:
        return "Out of memory";
    case REG_BADRPT:
        return "?, * or + not preceded by valid regular expression";
    default:
        return "Unknown error";
    }
}

void
prim_regexp(PRIM_PROTOTYPE)
{
    stk_array *nu_val = 0;
    stk_array *nu_idx = 0;
    regmatch_t *matches = 0;
    muf_re *re;
    char *text;
    int flags = 0;
    int nosubs, err, len, i;

    CHECKOP(3);

    oper3 = POP();              /* int:Flags */
    oper2 = POP();              /* str:Pattern */
    oper1 = POP();              /* str:Text */

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument (1)");
    if (oper2->type != PROG_STRING)
        abort_interp("Non-string argument (2)");
    if (oper3->type != PROG_INTEGER)
        abort_interp("Non-integer argument (3)");
    if (!oper2->data.string)
        abort_interp("Empty string argument (2)");

    if (oper3->data.number & MUF_RE_ICASE)
        flags |= REG_ICASE;

    if ((re = muf_re_get(oper2->data.string, flags, &err)) == NULL)
        abort_interp(muf_re_error(err));

    text = DoNullInd(oper1->data.string);
    len = strlen(text);
    nosubs = re->re.re_nsub + 1;

    if ((matches = (regmatch_t *) malloc(sizeof(regmatch_t) * nosubs)) == NULL)
        abort_interp("Out of memory");

    if ((err = regexec(&re->re, text, nosubs, matches, 0)) != 0) {
        if (err != REG_NOMATCH) {
            free(matches);
            abort_interp(muf_re_error(err));
        }

        if (((nu_val = new_array_packed(0)) == NULL) ||
            ((nu_idx = new_array_packed(0)) == NULL)) {
            free(matches);

            if (nu_val != NULL)
                array_free(nu_val);

            if (nu_idx != NULL)
                array_free(nu_idx);

            abort_interp("Out of memory");
        }
    } else {
        if (((nu_val = new_array_packed(nosubs)) == NULL) ||
            ((nu_idx = new_array_packed(nosubs)) == NULL)) {
            free(matches);

            if (nu_val != NULL)
                array_free(nu_val);

            if (nu_idx != NULL)
                array_free(nu_idx);

            abort_interp("Out of memory");
        }

        for (i = 0; i < nosubs; i++) {
            regmatch_t *cm = &matches[i];
            struct inst idx, val;
            stk_array *nu;

            if ((cm->rm_so >= 0) && (cm->rm_eo >= 0) && (cm->rm_so < len))
                snprintf(buf, BUFFER_LEN, "%.*s", cm->rm_eo - cm->rm_so,
                         &text[cm->rm_so]);
            else
                buf[0] = '\0';

            idx.type = PROG_INTEGER;
            idx.data.number = i;
            val.type = PROG_STRING;
            val.data.string = alloc_prog_string(buf);

            array_setitem(&nu_val, &idx, &val);

            CLEAR(&idx);
            CLEAR(&val);

            if ((nu = new_array_packed(2)) == NULL) {
                free(matches);

                array_free(nu_val);
                array_free(nu_idx);

                abort_interp("Out of memory");
            }

            idx.type = PROG_INTEGER;
            idx.data.number = 0;
            val.type = PROG_INTEGER;
            val.data.number = cm->rm_so + 1;

            array_setitem(&nu, &idx, &val);

            CLEAR(&idx);
            CLEAR(&val);

            idx.type = PROG_INTEGER;
            idx.data.number = 1;
            val.type = PROG_INTEGER;
            val.data.number = cm->rm_eo - cm->rm_so;

            array_setitem(&nu, &idx, &val);

            CLEAR(&idx);
            CLEAR(&val);

            idx.type = PROG_INTEGER;
            idx.data.number = i;
            val.type = PROG_ARRAY;
            val.data.array = nu;

            array_setitem(&nu_idx, &idx, &val);

            CLEAR(&idx);
            CLEAR(&val);
        }
    }

    free(matches);

    CLEAR(oper3);
    CLEAR(oper2);
    CLEAR(oper1);

    PushArrayRaw(nu_val);
    PushArrayRaw(nu_idx);
}

void
prim_regsub(PRIM_PROTOTYPE)
{
    regmatch_t *matches = 0;
    int flags = 0;
    char *write_ptr = buf;
    int write_left = BUFFER_LEN - 1;
    muf_re *re;
    char *text;
    int nosubs, err, len, i;

    CHECKOP(4);

    oper4 = POP();              /* int:Flags */
    oper3 = POP();              /* str:Replace */
    oper2 = POP();              /* str:Pattern */
    oper1 = POP();              /* str:Text */

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
        flags |= REG_ICASE;

    if ((re = muf_re_get(oper2->data.string, flags, &err)) == NULL)
        abort_interp(muf_re_error(err));

    text = DoNullInd(oper1->data.string);
    nosubs = re->re.re_nsub + 1;

    if ((matches = (regmatch_t *) malloc(sizeof(regmatch_t) * nosubs)) == NULL)
        abort_interp("Out of memory");

    while ((*text != '\0') && (write_left > 0)) {
        len = strlen(text);

        if ((err = regexec(&re->re, text, nosubs, matches, 0)) != 0) {
            if (err != REG_NOMATCH) {
                free(matches);
                abort_interp(muf_re_error(err));
            }

            while ((write_left > 0) && (*text != '\0')) {
                *write_ptr++ = *text++;
                write_left--;
            }

            break;
        } else {
            regmatch_t *cm = &matches[0];
            char *read_ptr = DoNullInd(oper3->data.string);
            int soff = cm->rm_so;
            int count;

            for (count = cm->rm_so;
                 (write_left > 0) && (*text != '\0') && (count > 0); count--) {
                *write_ptr++ = *text++;
                write_left--;
            }

            while ((write_left > 0) && (*read_ptr != '\0')) {
                if (*read_ptr == '\\') {
                    if (!isdigit(*(++read_ptr))) {
                        *write_ptr++ = *read_ptr++;
                        write_left--;
                    } else {
                        int idx = (*read_ptr++) - '0';

                        if ((idx < 0) || (idx >= nosubs)) {
                            free(matches);
                            abort_interp("Invalid \\subexp (3)");
                        }

                        cm = &matches[idx];

                        if ((cm->rm_so >= 0) && (cm->rm_eo >= 0)
                            && (cm->rm_so < len)) {
                            char *ptr = &text[cm->rm_so - soff];

                            count = cm->rm_eo - cm->rm_so;

                            if (count > write_left) {
                                free(matches);
                                abort_interp
                                    ("Operation would result in overflow");
                            }

                            for (;
                                 (write_left > 0) && (count > 0)
                                 && (*ptr != '\0'); count--) {
                                *write_ptr++ = *ptr++;
                                write_left--;
                            }
                        }
                    }
                } else {
                    *write_ptr++ = *read_ptr++;
                    write_left--;
                }
            }

            cm = &matches[0];

            for (count = cm->rm_eo - cm->rm_so; (*text != '\0') && (count > 0);
                 count--)
                text++;
        }

        if ((oper4->data.number & MUF_RE_ALL) == 0) {
            while ((write_left > 0) && (*text != '\0')) {
                *write_ptr++ = *text++;
                write_left--;
            }

            break;
        }
    }

    free(matches);

    if (*text != '\0')
        abort_interp("Operation would result in overflow");

    *write_ptr = '\0';

    CLEAR(oper4);
    CLEAR(oper3);
    CLEAR(oper2);
    CLEAR(oper1);

    PushString(buf);
}
