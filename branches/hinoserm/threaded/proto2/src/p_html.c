/* Primitives package */
#include "copyright.h"
#include "config.h"
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

void
prim_unescape_url(PRIM_PROTOTYPE)
{
	char buf[BUFFER_LEN];
    
    if (oper[0].type != PROG_STRING)
        abort_interp("Non-string argument. (1)");
    if (oper[0].data.string) {
        strcpy(buf, oper[0].data.string->data);
        unescape_url(buf);      /* Found in cgi.c */
        
        PushString(buf);
    } else {
        
        PushNullStr;
    }
}
void
prim_escape_url(PRIM_PROTOTYPE)
{
	char buf[BUFFER_LEN];
    
    if (oper[0].type != PROG_STRING)
        abort_interp("Non-string argument. (1)");
    if (oper[0].data.string) {
        escape_url(buf, oper[0].data.string->data); /* Found in cgi.c */
        
        PushString(buf);
    } else {
        
        PushNullStr;
    }
}
