/* Primitives package */

#include "copyright.h"
#include "config.h"

#ifdef NEWHTTPD

#define SECURE_FILE_PRIMS

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "db.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "strings.h"
#include "interp.h"
#include "cgi.h"
#include "newhttp.h"

extern struct inst *oper1, *oper2, *oper3, *oper4, *oper5, *oper6;
extern struct inst temp1, temp2, temp3;
extern int tmp, result;
extern dbref ref;
extern char buf[BUFFER_LEN];

#if defined(FILE_PRIMS) && defined(SECURE_FILE_PRIMS)
extern char *set_directory(char *filename);
extern int valid_name(char *filename);
extern char *parse_token(char *filename);
#endif

void
prim_descr_safeboot(PRIM_PROTOTYPE)
{
    struct descriptor_data *d;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Descriptor integer expected.");
    if (mlev < LARCH)
        abort_interp("W3 prim.");
    if (!(d = descrdata_by_descr(oper1->data.number)))
        abort_interp("Invalid descriptor.");

    d->booted = 4;

    CLEAR(oper1);
}

void
prim_descr_sendfile(PRIM_PROTOTYPE)
{
    struct descriptor_data *d;
    char *filename;

    CHECKOP(4);
    oper1 = POP();              /* (s) file  */
    oper2 = POP();              /* (i) end   */
    oper3 = POP();              /* (i) start */
    oper4 = POP();              /* (i) descr */

#ifndef FILE_PRIMS
    abort_interp("File prims are currently disabled.");
#else /* FILE_PRIMS */
    if (oper1->type != PROG_STRING)
        abort_interp("String expected. (4)");
    if (!oper1->data.string)
        abort_interp("Empty string argument (4)");
    if (oper2->type != PROG_INTEGER)
        abort_interp("Integer argument expected. (3)");
    if (oper3->type != PROG_INTEGER)
        abort_interp("Integer argument expected. (2)");
    if (oper4->type != PROG_INTEGER)
        abort_interp("Descriptor integer expected. (1)");
    if (mlev < LBOY)
        abort_interp("W4 prim.");
    if (!(d = descrdata_by_descr(oper4->data.number)))
        abort_interp("Invalid descriptor.");

    filename = oper1->data.string->data;

#ifdef SECURE_FILE_PRIMS
    /* These functions are in p_file.c.  */
    /* Maybe I'll eventually make my own more graceful versions. */
    if (!(valid_name(filename)))
        abort_interp("Invalid file name.");
    if (strchr(filename, '$') == NULL)
        filename = set_directory(filename);
    else
        filename = parse_token(filename);
    if (filename == NULL)
        abort_interp("Invalid shortcut used.");
#endif

    d->http.file.pid = fr->pid;
    d->http.file.ishttp = 0;
    result =
        descr_sendfile(d, oper3->data.number, oper2->data.number, filename);

#endif /* FILE_PRIMS */

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    CLEAR(oper4);
    PushInt(result);
}

void
prim_body_getchar(PRIM_PROTOTYPE)
{
    struct descriptor_data *d;

    CHECKOP(2);
    oper1 = POP();              /* (i) char  */
    oper2 = POP();              /* (i) descr */

    if (oper1->type != PROG_INTEGER)
        abort_interp("Integer argument expected. (2)");
    if (oper2->type != PROG_INTEGER)
        abort_interp("Descriptor integer expected. (1)");
    if (mlev < LM2)
        abort_interp("M2 prim.");
    if (!(d = descrdata_by_descr(oper2->data.number)))
        abort_interp("Invalid descriptor.");
    if (d->type != CT_HTTP)
        abort_interp("Non-HTTP connection.");

    if (!d->http.body.data || d->http.body.len < 1) {
        result = -2;
    } else if (oper1->data.number < 0 || oper1->data.number > d->http.body.len) {
        result = -1;
    } else {
        result = d->http.body.data[oper1->data.number];
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void
prim_body_nextchar(PRIM_PROTOTYPE)
{
    struct descriptor_data *d;

    CHECKOP(1);
    oper1 = POP();              /* (i) descr */

    if (oper1->type != PROG_INTEGER)
        abort_interp("Descriptor integer expected. (1)");
    if (mlev < LM2)
        abort_interp("M2 prim.");
    if (!(d = descrdata_by_descr(oper1->data.number)))
        abort_interp("Invalid descriptor.");
    if (d->type != CT_HTTP)
        abort_interp("Non-HTTP connection.");

    if (!d->http.body.data || d->http.body.len < 1) {
        result = -2;
    } else {
        tmp = d->http.body.curr;
        if (tmp < 0 || tmp > d->http.body.len) {
            result = -1;
        } else {
            result = d->http.body.data[tmp];
            d->http.body.curr = tmp + 1;
        }
    }

    CLEAR(oper1);
    PushInt(result);
}

void
prim_body_prevchar(PRIM_PROTOTYPE)
{
    struct descriptor_data *d;

    CHECKOP(1);
    oper1 = POP();              /* (i) descr */

    if (oper1->type != PROG_INTEGER)
        abort_interp("Descriptor integer expected. (1)");
    if (mlev < LM2)
        abort_interp("M2 prim.");
    if (!(d = descrdata_by_descr(oper1->data.number)))
        abort_interp("Invalid descriptor.");
    if (d->type != CT_HTTP)
        abort_interp("Non-HTTP connection.");

    if (!d->http.body.data || d->http.body.len < 1) {
        result = -2;
    } else {
        tmp = d->http.body.curr - 1;
        if (tmp < 0 || tmp > d->http.body.len) {
            result = -1;
        } else {
            result = d->http.body.data[tmp];
            d->http.body.curr = tmp;
        }
    }

    CLEAR(oper1);
    PushInt(result);
}

void
prim_base64encode(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument. (1)");

    if (!oper1->data.string) {
        CLEAR(oper1);
        PushNullStr;
    } else {
        if (((oper1->data.string->length + 2) / 3 * 4) > BUFFER_LEN)
            abort_interp("Operation would result in overflow.");
        http_encode64(oper1->data.string->data, oper1->data.string->length,
                      buf);
        CLEAR(oper1);
        PushString(buf);
    }
}

void
prim_base64decode(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument. (1)");

    if (!oper1->data.string) {
        CLEAR(oper1);
        PushNullStr;
    } else {
        result =
            http_decode64(oper1->data.string->data, oper1->data.string->length,
                          buf);
        CLEAR(oper1);
        if (result < 0)
            PushNullStr;
        else
            PushString(buf);
    }
}

void
prim_httpdata(PRIM_PROTOTYPE)
{
    struct descriptor_data *d;
    stk_array *nw;

    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_INTEGER)
        abort_interp("Descriptor integer expected.");
    if (mlev < LMAGE)
        abort_interp("MAGE prim.");
    if (!(d = descrdata_by_descr(oper1->data.number)))
        abort_interp("Invalid descriptor.");
    if (d->type != CT_HTTP)
        abort_interp("Non-HTTP connection.");

    nw = http_makearray(d);

    CLEAR(oper1);
    PushArrayRaw(nw);
}

#endif /* NEWHTTPD */
