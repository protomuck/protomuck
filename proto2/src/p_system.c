/* Primitives package */
/* Created this new p_system.c file to contain many of the system
 * related prims that have been stuffed in p_misc.c and other files before.
 */
#include "copyright.h"
#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
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

extern struct inst *oper1, *oper2, *oper3, *oper4, *oper5, *oper6;
extern struct inst temp1, temp2, temp3;
extern int tmp, result;
extern dbref ref;
extern char buf[BUFFER_LEN];
struct tm *time_tm;

extern struct line *read_program(dbref i);
extern int tune_setparm(const dbref player, const char *parmname,
                        const char *val);

void
prim_sysparm(PRIM_PROTOTYPE)
{
    const char *ptr;
    const char *tune_get_parmstring(const char *name, int mlev);

    CHECKOP(1);
    oper1 = POP();              /* string: system parm name */
    if (oper1->type != PROG_STRING)
        abort_interp("Invalid argument");
    if (oper1->data.string) {
        ptr = tune_get_parmstring(oper1->data.string->data, mlev);
    } else {
        ptr = "";
    }
    CHECKOFLOW(1);
    CLEAR(oper1);
    PushString(ptr);
}

void
prim_setsysparm(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();              /* string: new parameter value */
    oper2 = POP();              /* string: parameter to tune */

    if (mlev < 6)
        abort_interp("Archwizards or higher only.");
    if (oper2->type != PROG_STRING)
        abort_interp("Invalid argument. (1)");
    if (!oper2->data.string)
        abort_interp("Null string argument. (1)");
    if (oper1->type != PROG_STRING)
        abort_interp("Invalid argument. (2)");
    if (!oper1->data.string)
        abort_interp("Null string argument. (2)");

    result = tune_setparm(program, oper2->data.string->data,
                          oper1->data.string->data);

    switch (result) {
        case 0:                /* TUNESET_SUCCESS */
            log_status("TUNED (MUF): %s(%d) tuned %s to %s\n",
                       player != -1 ? NAME(player) : "(Login)", player,
                       oper2->data.string->data, oper1->data.string->data);
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
    CLEAR(oper1);
    CLEAR(oper2);
}

void
prim_version(PRIM_PROTOTYPE)
{
    char temp[256];

    CHECKOP(0);
    CHECKOFLOW(1);
    sprintf(temp, "%s(ProtoMUCK%s)", VERSION, PROTOBASE);
    PushString(temp);
}

void
prim_force(PRIM_PROTOTYPE)
{
    /* d s -- */
    CHECKOP(2);
    oper1 = POP();              /* string to @force */
    oper2 = POP();              /* player dbref */
    if (mlev < LARCH)
        abort_interp("Arch prim");
    if (fr->level > 8)
        abort_interp("Interp call loops not allowed");
    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument (2)");
    if (oper2->type != PROG_OBJECT)
        abort_interp("Non-object argument (1)");
    ref = oper2->data.objref;
    if (ref < 0 || ref >= db_top)
        abort_interp("Invalid object to force (1)");
    if (Typeof(ref) != TYPE_PLAYER && Typeof(ref) != TYPE_THING)
        abort_interp("Object to force not a thing or player (1)");
    if (!oper1->data.string)
        abort_interp("Null string argument (2)");
    if (index(oper1->data.string->data, '\r'))
        abort_interp("Carriage returns not allowed in command string (2)");
    if (Man(oper2->data.objref) && !(Man(OWNER(program)) && Boy(program)))
        abort_interp("Cannot force the man (1)");

    force_level++;
    interp_set_depth(fr);
    process_command(dbref_first_descr(oper2->data.objref), oper2->data.objref,
                    oper1->data.string->data);
    fr->level--;
    interp_set_depth(fr);
    force_level--;
    CLEAR(oper1);
    CLEAR(oper2);
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
    CHECKOP(1);
    oper1 = POP();
    if (mlev < LARCH)
        abort_interp("Archwizard primitive.");
    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument (1).");
    if (oper1->data.string) {
        strcpy(buf, oper1->data.string->data);
        log_status("%s\r\n", buf);
    }
    CLEAR(oper1);
}

void
prim_dump(PRIM_PROTOTYPE)
{
    /* ( -- ) */
    if (mlev < LARCH)
        abort_interp("Archwizard primitive.");
    dump_db_now();
}


void
prim_delta(PRIM_PROTOTYPE)
{
    /* ( -- ) */
    if (mlev < LARCH)
        abort_interp("Archwizard primitive.");
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
    CHECKOP(1);
    oper1 = POP();

    if (mlev < LBOY)
        abort_interp("W4 primitive.");
    if (oper1->type != PROG_STRING)
        abort_interp("String expected.");

    log_status("SHUT(MUF): by %s\n", unparse_object(player, player));
    shutdown_flag = 1;
    restart_flag = 0;
    if (oper1->data.string) {
        strcat(shutdown_message, SYSWHITE MARK SYSNORMAL);
        strcat(shutdown_message, oper1->data.string->data);
        strcat(shutdown_message, "\r\n");
    }

    CLEAR(oper1);
}

void
prim_restart(PRIM_PROTOTYPE)
{
    /* (s<message> -- ) */
    CHECKOP(1);
    oper1 = POP();
    if (mlev < LBOY)
        abort_interp("W4 primitive.");
    if (oper1->type != PROG_STRING)
        abort_interp("String expected.");

    log_status("REST(MUF): by %s\n", unparse_object(player, player));
    shutdown_flag = 1;
    restart_flag = 1;

    if (oper1->data.string) {
        strcat(restart_message, SYSWHITE MARK SYSNORMAL);
        strcat(restart_message, oper1->data.string->data);
        strcat(restart_message, "\r\n");
    }

    CLEAR(oper1);
}

void
prim_armageddon(PRIM_PROTOTYPE)
{
    char buf[BUFFER_LEN];

    /* ( s<message> -- ) */

    CHECKOP(1);
    oper1 = POP();

    if (mlev < LBOY)
        abort_interp("W4 primitive.");
    if (oper1->type != PROG_STRING)
        abort_interp("String expected.");

    sprintf(buf, "\r\nImmediate shutdown by %s.\r\n", NAME(player));
    if (oper1->data.string) {
        strcat(buf, SYSWHITE MARK SYSNORMAL);
        strcat(buf, oper1->data.string->data);
        strcat(buf, "\r\n");
    }

    CLEAR(oper1);
    log_status("DDAY(MUF): %s(%d)\n", NAME(player), player);
    fprintf(stderr, "DDAY: %s(%d)\n", NAME(player), player);
    close_sockets(buf);

#ifdef SPAWN_HOST_RESOLVER
    kill_resolver();
#endif

    exit(1);
}

void
prim_sysparm_array(PRIM_PROTOTYPE)
{
    stk_array *nu;

    /* string */
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Expected a string smatch pattern. (1)");

    nu = tune_parms_array(DoNullInd(oper1->data.string), mlev);

    CLEAR(oper1);
    PushArrayRaw(nu);
}
