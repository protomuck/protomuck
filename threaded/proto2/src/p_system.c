/* Primitives package */
/* Created this new p_system.c file to contain many of the system
 * related prims that have been stuffed in p_misc.c and other files before.
 */
#include "copyright.h"
#include "config.h"

#include "db.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "mufevent.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "strings.h"
#include "interp.h"
#include "netresolve.h"

/* mutex? -hinoserm */
extern struct frame* aForceFrameStack[9];

extern struct line *read_program(dbref i);
extern int tune_setparm(const dbref player, const char *parmname,
                        const char *val);

void
prim_sysparm(PRIM_PROTOTYPE)
{
    const char *ptr;
    const char *tune_get_parmstring(const char *name, int mlev);

    if (oper[0].type != PROG_STRING)
        abort_interp("Invalid argument");
    if (oper[0].data.string) {
        ptr = tune_get_parmstring(oper[0].data.string->data, mlev);
    } else {
        ptr = "";
    }
    CHECKOFLOW(1);
    PushString(ptr);
}

void
prim_setsysparm(PRIM_PROTOTYPE)
{
	int result;

    if (oper[1].type != PROG_STRING)
        abort_interp("Invalid argument. (1)");
    if (!oper[1].data.string)
        abort_interp("Null string argument. (1)");
    if (oper[0].type != PROG_STRING)
        abort_interp("Invalid argument. (2)");
    if (!oper[0].data.string)
        abort_interp("Null string argument. (2)");

    result = tune_setparm(program, oper[1].data.string->data,
                          oper[0].data.string->data);

    switch (result) {
        case 0:                /* TUNESET_SUCCESS */
            log_status("TUNED (MUF): %s(%d) tuned %s to %s\n",
                       OkObj(player) ? NAME(player) : "(Login)", player,
                       oper[1].data.string->data, oper[0].data.string->data);
            break;
        case 1:                /* TUNESET_UNKNOWN */
            abort_interp("Unknown parameter. (1)");
            break;
        case 2:                /* TUNESET_SYNTAX */
            abort_interp("Bad parameter syntax. (2)");
            break;
        case 3:                /* TUNESET_BADVAL */
            abort_interp("Bad parameter value. (2)");
            break;
    }
}

void
prim_version(PRIM_PROTOTYPE)
{
    char temp[256];

    CHECKOFLOW(1);
    sprintf(temp, "%s(ProtoMUCK%s)", VERSION, PROTOBASE);
    PushString(temp);
}

void
prim_force(PRIM_PROTOTYPE)
{
	char buf[BUFFER_LEN];
    int nFrameIndex = -1; /* -1 means it hasn't been set */
    int nCurFr = 0; /* Loop iterator */
    int wclen = -2;
    int len;
	dbref ref;

    /* d s -- */
    if (fr->level > 8)
        abort_interp("Interp call loops not allowed");
    if (oper[0].type != PROG_STRING)
        abort_interp("Non-string argument (2)");
    if (oper[1].type != PROG_OBJECT)
        abort_interp("Non-object argument (1)");
    ref = oper[1].data.objref;
    if (ref < 0 || ref >= db_top)
        abort_interp("Invalid object to force (1)");
    if (Typeof(ref) != TYPE_PLAYER && Typeof(ref) != TYPE_THING)
        abort_interp("Object to force not a thing or player (1)");
    if (!oper[0].data.string)
        abort_interp("Null string argument (2)");
    if (index(oper[0].data.string->data, '\r'))
        abort_interp("Carriage returns not allowed in command string (2)");
    if (Man(oper[1].data.objref) && !(Man(OWNER(program)) && Boy(program)))
        abort_interp("Cannot force the man (1)");

    strcpy(buf, oper[0].data.string->data);
    len = oper[0].data.string->length;
#ifdef UTF8_SUPPORT
    wclen = oper[0].data.string->wclength;
#endif

    force_level++;
    /* Okay, we need to store a pointer to the fr in the global stack of
     * frame pointers we need to enable ispid? and getpidinfo to be able
     * to search. */
    for ( ; nCurFr < 9; ++nCurFr )
    {
        if ( !aForceFrameStack[nCurFr] )
        {
            aForceFrameStack[nCurFr] = fr;
            nFrameIndex = nCurFr;
            break;
        }
    }
    
    if ( nFrameIndex == -1 )
    {
        abort_interp( "Internal error trying to cache frame pointer." );
    }

    fr->level++;
    interp_set_depth(fr);
    process_command(dbref_first_descr(ref), ref, buf, len, wclen);
    fr->level--;
    interp_set_depth(fr);
    force_level--;

    /* Now remove our pointer from the end of the array */
    aForceFrameStack[nFrameIndex] = NULL; 
}

void
prim_force_level(PRIM_PROTOTYPE)
{
    CHECKOFLOW(1);
    PushInt(force_level);
}

void
prim_logstatus(PRIM_PROTOTYPE)
{
	char buf[BUFFER_LEN];

    if (oper[0].type != PROG_STRING)
        abort_interp("Non-string argument (1).");
    if (oper[0].data.string) {
        strcpy(buf, oper[0].data.string->data);
        log_status("%s\r\n", buf);
    }
}

void
prim_dump(PRIM_PROTOTYPE)
{
    /* ( -- ) */
    dump_db_now();
}


void
prim_delta(PRIM_PROTOTYPE)
{
    /* ( -- ) */
#ifdef DELTADUMPS
    delta_dump_now();
#else
    abort_interp("Support for Delta dumps is not compiled.");
#endif
}

void
prim_shutdown(PRIM_PROTOTYPE)
{
    /* (s<message> -- ) */
    if (oper[0].type != PROG_STRING)
        abort_interp("String expected.");

    log_status("SHUT(MUF: %d): by %s(%d)\n", program,
               OkObj(player) ? NAME(player) : "(login)", player);
    shutdown_flag = 1;
    restart_flag = 0;
    if (oper[0].data.string) {
        strcat(shutdown_message, SYSWHITE MARK SYSNORMAL);
        strcat(shutdown_message, oper[0].data.string->data);
        strcat(shutdown_message, "\r\n");
    }
}

void
prim_restart(PRIM_PROTOTYPE)
{
    /* (s<message> -- ) */
    if (oper[0].type != PROG_STRING)
        abort_interp("String expected.");

    log_status("RESTART(MUF: %d): by %s(%d)\n", program,
               OkObj(player) ? NAME(player) : "(login)", player);
    shutdown_flag = 1;
    restart_flag = 1;

    if (oper[0].data.string) {
        strcat(restart_message, SYSWHITE MARK SYSNORMAL);
        strcat(restart_message, oper[0].data.string->data);
        strcat(restart_message, "\r\n");
    }
}

void
prim_armageddon(PRIM_PROTOTYPE)
{
    char buf[BUFFER_LEN];

    /* ( s<message> -- ) */
    if (oper[0].type != PROG_STRING)
        abort_interp("String expected.");

    sprintf(buf, "\r\nImmediate shutdown by %s.\r\n", NAME(PSafe));
    if (oper[0].data.string) {
        strcat(buf, SYSWHITE MARK SYSNORMAL);
        strcat(buf, oper[0].data.string->data);
        strcat(buf, "\r\n");
    }

    log_status("DDAY(MUF: %d): by %s(%d)\n", program,
               OkObj(player) ? NAME(player) : "(login)", player);
    fprintf(stderr, "DDAY(MUF: %d): by %s(%d)\n", program,
            OkObj(player) ? NAME(player) : "(login)", player);
    close_sockets(buf);

#ifdef SPAWN_HOST_RESOLVER
    kill_resolver();
#endif
#ifdef USE_RESLVD
    reslvd_close();
#endif

    exit(1);
}

void
prim_sysparm_array(PRIM_PROTOTYPE)
{
    stk_array *nu;

    if (oper[0].type != PROG_STRING)
        abort_interp("Expected a string smatch pattern. (1)");

    nu = tune_parms_array(DoNullInd(oper[0].data.string), mlev);

    PushArrayRaw(nu);
}
