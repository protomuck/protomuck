/* Primitives package */
#include "copyright.h"
#include "config.h"
#ifdef NEWHTTPD
#define SECURE_FILE_PRIMS
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
#if defined(FILE_PRIMS) && defined(SECURE_FILE_PRIMS)
extern char *set_directory(char *filename);
extern int valid_name(char *filename);
extern char *parse_token(char *filename);
#endif
void
prim_descr_safeboot(PRIM_PROTOTYPE)
{
    struct descriptor_data *d;
    
    if (oper[0].type != PROG_INTEGER)
        abort_interp("Descriptor integer expected.");
    if (!(d = descrdata_by_descr(oper[0].data.number)))
        abort_interp("Invalid descriptor.");
    d->booted = 4;
    
}
void
prim_body_getchar(PRIM_PROTOTYPE)
{
    struct descriptor_data *d;
	int result;

    if (oper[0].type != PROG_INTEGER)
        abort_interp("Integer argument expected. (2)");
    if (oper[1].type != PROG_INTEGER)
        abort_interp("Descriptor integer expected. (1)");
    if (!(d = descrdata_by_descr(oper[1].data.number)))
        abort_interp("Invalid descriptor.");
    if (d->type != CT_HTTP)
        abort_interp("Non-HTTP connection.");
    if (!d->http->body.data || d->http->body.len < 1) {
        result = -2;
    } else if (oper[0].data.number < 0 || oper[0].data.number > d->http->body.len) {
        result = -1;
    } else {
        result = d->http->body.data[oper[0].data.number];
    }
    
    PushInt(result);
}
void
prim_body_nextchar(PRIM_PROTOTYPE)
{
    struct descriptor_data *d;
	int result, tmp;

    if (oper[0].type != PROG_INTEGER)
        abort_interp("Descriptor integer expected. (1)");
    if (!(d = descrdata_by_descr(oper[0].data.number)))
        abort_interp("Invalid descriptor.");
    if (d->type != CT_HTTP)
        abort_interp("Non-HTTP connection.");
    if (!d->http->body.data || d->http->body.len < 1) {
        result = -2;
    } else {
        tmp = d->http->body.curr;
        if (tmp < 0 || tmp > d->http->body.len) {
            result = -1;
        } else {
            result = d->http->body.data[tmp];
            d->http->body.curr = tmp + 1;
        }
    }
    
    PushInt(result);
}
void
prim_body_prevchar(PRIM_PROTOTYPE)
{
    struct descriptor_data *d;
	int result, tmp;

    if (oper[0].type != PROG_INTEGER)
        abort_interp("Descriptor integer expected. (1)");
    if (!(d = descrdata_by_descr(oper[0].data.number)))
        abort_interp("Invalid descriptor.");
    if (d->type != CT_HTTP)
        abort_interp("Non-HTTP connection.");
    if (!d->http->body.data || d->http->body.len < 1) {
        result = -2;
    } else {
        tmp = d->http->body.curr - 1;
        if (tmp < 0 || tmp > d->http->body.len) {
            result = -1;
        } else {
            result = d->http->body.data[tmp];
            d->http->body.curr = tmp;
        }
    }
    
    PushInt(result);
}
void
prim_httpdata(PRIM_PROTOTYPE)
{
    struct descriptor_data *d;
    stk_array *nw;
    
    if (oper[0].type != PROG_INTEGER)
        abort_interp("Descriptor integer expected.");
    if (!(d = descrdata_by_descr(oper[0].data.number)))
        abort_interp("Invalid descriptor.");
    if (d->type != CT_HTTP)
        abort_interp("Non-HTTP connection.");
    nw = http_makearray(d);
    
    PushArrayRaw(nw);
}
#endif /* NEWHTTPD */
