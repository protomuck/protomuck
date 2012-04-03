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
char *
p_commandtext(dbref player, char *command, char *text)
{
    static char buf[BUFFER_LEN];
    strcpy(buf, "");
    if (FLAG2(player) & F2PUEBLO) {
        sprintf(buf, "<a xch_cmd=\"%s\">%s</a>", command, text);
    }
    return (buf);
}
char *
p_playmidi(dbref player, char *musicurl, char *volume)
{
    static char buf[BUFFER_LEN];
    strcpy(buf, "");
    if (FLAG2(player) & F2PUEBLO) {
        sprintf(buf, "<img xch_sound=play href=\"%s\" xch_volume=%s>",
                musicurl, volume);
    }
    return (buf);
}
char *
p_stopmidi(dbref player)
{
    static char buf[BUFFER_LEN];
    strcpy(buf, "");
    if (FLAG2(player) & F2PUEBLO) {
        sprintf(buf, "<img xch_sound=stop device=midi>");
    }
    return (buf);
}
void
prim_stopmidi(PRIM_PROTOTYPE)
{
	char buf[BUFFER_LEN];
    
    if (!valid_object(&oper[0]))
        abort_interp("Invalid object argument.");
    CHECKOFLOW(1);
    strcpy(buf, p_stopmidi(oper[0].data.objref));
    
    PushString(buf);
}
void
prim_playmidi(PRIM_PROTOTYPE)
{
	char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    char buf3[5];
    
    if (oper[0].type != PROG_INTEGER)
        abort_interp("Integer value expected (3)");
    if (oper[1].type != PROG_STRING)
        abort_interp("String value expected (2)");
    if (!valid_object(&oper[2]))
        abort_interp("Invalid object argument (1)");
    CHECKOFLOW(1);
    strcpy(buf2, oper[1].data.string->data);
    sprintf(buf3, "%d", oper[0].data.number);
    strcpy(buf, p_playmidi(oper[2].data.objref, buf2, buf3));
    
    PushString(buf);
}
void
prim_commandtext(PRIM_PROTOTYPE)
{
	char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    char buf3[BUFFER_LEN];
    
    if (oper[0].type != PROG_STRING)
        abort_interp("String argument expected (3)");
    if (oper[1].type != PROG_STRING)
        abort_interp("String argument expected (2)");
    if (!valid_object(&oper[2]))
        abort_interp("Invalid object reference (1)");
    strcpy(buf2, oper[1].data.string->data);
    strcpy(buf3, oper[0].data.string->data);
    strcpy(buf, p_commandtext(oper[2].data.objref, buf2, buf3));
    
    PushString(buf);
}
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
