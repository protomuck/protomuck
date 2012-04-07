/* Primitives package */

#include "copyright.h"
#include "config.h"
#include "params.h"

#include "db.h"
#include "tune.h"
#include "props.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "strings.h"
#include "interp.h"

void
copyobj(dbref player, dbref old, dbref nw)
{
    struct object *newp = DBFETCH(nw);

    NAME(nw) = alloc_string(NAME(old));
    newp->properties = copy_prop(old);
    newp->exits = NOTHING;
    newp->contents = NOTHING;
    newp->next = NOTHING;
    newp->location = NOTHING;
    moveto(nw, player);

#ifdef DISKBASE
    newp->propsfpos = 0;
    newp->propsmode = PROPS_UNLOADED;
    newp->propstime = 0;
    newp->nextold = NOTHING;
    newp->prevold = NOTHING;
    dirtyprops(nw);
#endif

    newp->ts.created = current_systime;
    newp->ts.modified = current_systime;
    newp->ts.lastused = current_systime;
    newp->ts.dcreated = player;
    newp->ts.dmodified = player;
    newp->ts.dlastused = player;
    newp->ts.usecount = 0;

    DBDIRTY(nw);
}

int
check_flag1(char *flag)
{
    if (string_prefix("dark", flag) || string_prefix("debugging", flag))
        return DARK;
    if (string_prefix("sticky", flag) || string_prefix("silent", flag)
        || string_prefix("setuid", flag))
        return STICKY;
    if (string_prefix("abode", flag) || string_prefix("autostart", flag)
        || string_prefix("abate", flag))
        return ABODE;
    if (string_prefix("chown_ok", flag) || string_prefix("color_on", flag)
        || string_prefix("ansi", flag))
        return CHOWN_OK;
    if (string_prefix("256color", flag) || string_prefix("&", flag))
        return F256COLOR;
    if (string_prefix("haven", flag) || string_prefix("harduid", flag)
        || string_prefix("hide", flag))
        return HAVEN;
    if (string_prefix("jump_ok", flag))
        return JUMP_OK;
    if (string_prefix("link_ok", flag))
        return LINK_OK;
    if (string_prefix("listener", flag))
        return LISTENER;
    if (string_prefix("builder", flag) || string_prefix("bound", flag))
        return BUILDER;
    if (string_prefix("interactive", flag))
        return INTERACTIVE;
    if (string_prefix("zombie", flag) || string_prefix("puppet", flag))
        return ZOMBIE;
    if (string_prefix("xforcible", flag) ||
        string_prefix("expanded_debug", flag))
        return XFORCIBLE;
    if (string_prefix("vehicle", flag) || string_prefix("viewable", flag))
        return VEHICLE;
    if (string_prefix("quell", flag))
        return QUELL;
    if (string_prefix("internal", flag))
        return INTERNAL;
    return 0;

}

int
check_flag2(char *flag, int *nbol)
{
    *nbol = 0;

    if (string_prefix("idle", flag))
        return F2IDLE;
    if (string_prefix("guest", flag))
        return F2GUEST;
    if (string_prefix("logwall", flag))
        return F2LOGWALL;
    if (string_prefix("light", flag) || string_prefix("oldcomment", flag))
        return F2LIGHT;
    if (string_prefix("mufcount", flag) || string_prefix("+", flag))
        return F2MUFCOUNT;
    if (string_prefix("protect", flag) || string_prefix("*", flag))
        return F2PROTECT;
    if (string_prefix("antiprotect", flag))
        return F2ANTIPROTECT;
    if (string_prefix("parent", flag) || string_prefix("prog_debug", flag)
        || string_prefix("%", flag))
        return F2PARENT;
#ifdef CONTROLS_SUPPORT
    if (string_prefix("controls", flag) || string_prefix("~", flag))
        return F2CONTROLS;
#endif
    if (string_prefix("immobile", flag) || string_prefix("|", flag))
        return F2IMMOBILE;
    if (string_prefix("hidden", flag) || string_prefix("#", flag))
        return F2HIDDEN;
    if (string_prefix("suspect", flag))
        return F2SUSPECT;
    if (string_prefix("command", flag))
        return F2COMMAND;
    if (string_prefix("no_command", flag))
        return F2NO_COMMAND;
    if (string_prefix("no_optimize", flag))
        return F2NO_COMMAND;
    if (string_prefix("examine_ok", flag))
        return F2EXAMINE_OK;
    if (string_prefix("pueblo", flag))
        return F2PUEBLO;
    if (string_prefix("nhtml", flag))
        return F2HTML;
    if (string_prefix("html", flag)) {
        *nbol = F2HTML;
        return F2PUEBLO;
    }
    if (string_prefix("trueidle", flag)) {
        return F2TRUEIDLE;
    }
    if (!*tp_userflag_name) {
        if (string_prefix("mobile", flag) || string_prefix("offer", flag) ||
            string_prefix("?", flag))
            return F2MOBILE;
    } else {
        if (string_prefix("mobile", flag) || string_prefix("offer", flag) ||
            string_prefix("?", flag) || string_prefix(tp_userflag_name, flag))
            return F2MOBILE;
    }
    return 0;
}

int
check_mlev(char *flag, int *truewiz)
{
    *truewiz = 0;

    if (string_prefix(flag, "true")) {
        *truewiz = 1;
        flag++;
        flag++;
        flag++;
        flag++;
    }

    if (string_prefix("meeper", flag) || string_prefix("mpi", flag))
        return LMPI;
    if (string_prefix("mucker", flag) || string_prefix("mucker1", flag) || string_prefix("m1", flag))
        return LMUF;
    if (string_prefix("mucker2", flag) || string_prefix("m2", flag))
        return LM2;
    if (string_prefix("mucker3", flag) || string_prefix("m3", flag))
        return LM3;
    if (string_prefix("mage", flag) || string_prefix("W1", flag))
        return (tp_multi_wizlevels ? LMAGE : LM3);
    if (string_prefix("wizard", flag) || string_prefix("W2", flag))
        return (tp_multi_wizlevels ? LWIZ : LARCH);
    if (string_prefix("archwizard", flag) || string_prefix("W3", flag))
        return LARCH;
    if (string_prefix("boy", flag) || string_prefix("W4", flag))
        return (tp_multi_wizlevels ? LBOY : LARCH);
    if (string_prefix("man", flag) || string_prefix("W5", flag))
        return LMAN;
    return 0;
}

int
check_flag4(char *flag)
{
    char buf[32];
    int i=0;

    strncpy(buf,flag,32);

    while(*(buf+i)) {
	*(buf+i)=UPCASE(*(buf+i));
	i++;
    }

    if (sscanf(buf, "LFLAG%d", &i) == 1 && i>=0 && i<=31)
	return LFLAGx(i);

    i=0; while (i<32 && string_compare(lflag_name[i],buf)) i++;
    if (i < 32) 
	return LFLAGx(i);

    return 0;
}

int
flag_check_perms(dbref ref, int flag, int mlev)
{
    return 1;
}


int
flag_check_perms2(dbref ref, int flag, int mlev)
{
    if (flag == F2HIDDEN && mlev < LWIZ)
        return 0;
    if (flag == F2LOGWALL && mlev < LWIZ)
        return 0;
    if (flag == F2SUSPECT && mlev < LMAGE)
        return 0;

    return 1;
}

int
flag_set_perms(dbref ref, int flag, int mlev, dbref prog)
{
    if (flag == LMPI && MLevel(ref) > LMPI)
        return 0;
    if ((flag == DARK && mlev < LARCH) &&
        ((Typeof(ref) == TYPE_PLAYER) ||
         (!tp_exit_darking && Typeof(ref) == TYPE_EXIT) ||
         (!tp_thing_darking && Typeof(ref) == TYPE_THING)))
        return 0;
    if (flag == ABODE && Typeof(ref) == TYPE_PROGRAM)
        return 0;
    if (flag == LISTENER)
        return 0;
    if (flag == XFORCIBLE && Typeof(ref) != TYPE_PROGRAM && mlev < LARCH)
        return 0;
    if (flag == QUELL && Typeof(ref) != TYPE_THING)
        return 0;
    if (flag == BUILDER && mlev < LARCH)
        return 0;
    if (((flag == ZOMBIE && ((Typeof(ref) == TYPE_THING &&
                              (FLAGS(OWNER(prog)) & ZOMBIE))
                             || Typeof(ref) == TYPE_PLAYER)) && mlev < LARCH))
        return 0;
    if (flag == INTERACTIVE)
        return 0;

    return 1;
}

int
flag_set_perms2(dbref ref, int flag, int mlev, dbref prog)
{
    if (flag == F2HIDDEN && mlev < LARCH)
        return 0;
    if (flag == F2GUEST && mlev < LMAGE)
        return 0;
    if (flag == F2LOGWALL && mlev < LARCH)
        return 0;
    if (flag == F2PUEBLO && mlev < LMAGE)
        return 0;
    if (flag == F2HTML && mlev < LMAGE)
        return 0;
    if (flag == F2PROTECT && mlev < LBOY)
        return 0;
    if (flag == F2SUSPECT && mlev < LWIZ)
        return 0;
#ifdef CONTROLS_SUPPORT
    if (flag == F2CONTROLS
        && ((Typeof(ref) == TYPE_PLAYER) || (Typeof(ref) == TYPE_PROGRAM))
        && mlev < LBOY)
        return 0;
#endif
    if ((flag == F2IDLE) && (Typeof(ref) != TYPE_PLAYER))
        return 0;
    if (flag == F2TRUEIDLE)
        return 0;
    if (flag == F2COMMAND)
        return 0;
    if (flag == F2NO_COMMAND)
        return (mlev >= LMAGE);
    if (flag == F2MOBILE)
        return (mlev >= tp_userflag_mlev);
    if (flag == F2IMMOBILE && mlev < LWIZ)
        return 0;
    return 1;
}

int
has_flagp(dbref ref, char *flag, int mlev)
{
    int truwiz = 0, tmp1 = 0, tmp2 = 0, lev = 0, tmp5 = 0, tmp4 = 0, rslt = 0, result = 0;

    while (*flag == '!') {
        flag++;
        rslt = (!rslt);
    }
    if (!*flag)
        return -2;

    tmp1 = check_flag1(flag);
    tmp2 = check_flag2(flag, &tmp5);
    tmp4 = check_flag4(flag);
    lev = check_mlev(flag, &truwiz);
    if (!tmp1 && !tmp2 && !tmp4 && !lev) return -1;
    lev = check_mlev(flag, &truwiz);
    if (lev) {
        if (truwiz)
            result = (MLevel(ref) >= lev);
        else
            result = (QLevel(ref) >= lev);
    }    
    if (!result && tmp1) {
        if (!flag_check_perms(ref, tmp1, mlev))
            return -2;
        if (tmp1 == DARK)
            result = ((FLAGS(ref) & DARK) || (FLAG2(ref) & F2HIDDEN));
        else
            result = (FLAGS(ref) & tmp1);
    }
    if (!result && tmp2) {
        if (!flag_check_perms2(ref, tmp2, mlev))
            return -2;
        if (tmp5)
            result = ((FLAG2(ref) & tmp2) && (FLAG2(ref) & tmp5));
        else if (tmp2 == F2COMMAND)
            result = ((FLAG2(ref) & tmp2) && !(FLAG2(ref) & F2NO_COMMAND));
        else if (tmp2 == F2NO_COMMAND)
            result = ((FLAG2(ref) & tmp2) || !(FLAG2(ref) & F2COMMAND));
        else
            result = (FLAG2(ref) & tmp2);
    }
    if (!result && tmp4) {
        result = (FLAG4(ref) & tmp4);
    }
    return (rslt ? !result : result);
}

int
check_power(char *power)
{
    int tmp = 0;

    while (*power == '!') {
        power++;
    }

    if (string_prefix("announce", power)) {
        tmp = POW_ANNOUNCE;
    } else if (string_prefix("all_muf_prims", power)) {
        tmp = POW_ALL_MUF_PRIMS;
    } else if (string_prefix("boot", power)) {
        tmp = POW_BOOT;
    } else if (string_prefix("chown_anything", power)) {
        tmp = POW_CHOWN_ANYTHING;
    } else if (string_prefix("control_all", power)) {
        tmp = POW_CONTROL_ALL;
    } else if (string_prefix("control_muf", power)) {
        tmp = POW_CONTROL_MUF;
    } else if (string_prefix("expanded_who", power)) {
        tmp = POW_EXPANDED_WHO;
    } else if (string_prefix("hide", power)) {
        tmp = POW_HIDE;
    } else if (string_prefix("idle", power)) {
        tmp = POW_IDLE;
    } else if (string_prefix("link_anywhere", power)) {
        tmp = POW_LINK_ANYWHERE;
    } else if (string_prefix("long_fingers", power)) {
        tmp = POW_LONG_FINGERS;
    } else if (string_prefix("no_pay", power)) {
        tmp = POW_NO_PAY;
    } else if (string_prefix("open_anywhere", power)) {
        tmp = POW_OPEN_ANYWHERE;
    } else if (string_prefix("player_create", power)) {
        tmp = POW_PLAYER_CREATE;
    } else if (string_prefix("player_purge", power)) {
        tmp = POW_PLAYER_PURGE;
    } else if (string_prefix("search", power)) {
        tmp = POW_SEARCH;
    } else if (string_prefix("see_all", power)) {
        tmp = POW_SEE_ALL;
    } else if (string_prefix("teleport", power)) {
        tmp = POW_TELEPORT;
    } else if (string_prefix("shutdown", power)) {
        tmp = POW_SHUTDOWN;
    } else if (string_prefix("staff", power)) {
        tmp = POW_STAFF;
    }

    return tmp;
}

void
prim_addpennies(PRIM_PROTOTYPE)
{
	dbref ref;
	int result;

    if (!valid_object(&oper[1]))
        abort_interp("Invalid object");
    if (oper[0].type != PROG_INTEGER)
        abort_interp("Non-integer argument (2)");

    ref = oper[1].data.objref;

    if (Typeof(ref) == TYPE_PLAYER) {
        result = DBFETCH(ref)->sp.player.pennies;
        if ((result + oper[0].data.number) < 0)
            abort_interp("Result would be negative");
        if (mlev < LWIZ)
            if (oper[0].data.number > 0)
                if ((result + oper[0].data.number) > tp_max_pennies)
                    abort_interp("Would exceed MAX_PENNIES");
        result += oper[0].data.number;
        DBFETCH(ref)->sp.player.pennies += oper[0].data.number;
        DBDIRTY(ref);
    } else if (Typeof(ref) == TYPE_THING) {
        if (mlev < LMAGE)
            abort_interp(tp_noperm_mesg);
        result = DBFETCH(ref)->sp.thing.value + oper[0].data.number;
        if (result < 1)
            abort_interp("Result must be positive");
        DBFETCH(ref)->sp.thing.value += oper[0].data.number;
        DBDIRTY(ref);
    } else {
        abort_interp("Invalid object type");
    }
}

void
prim_moveto(PRIM_PROTOTYPE)
{
    if (fr->level > 8)
        abort_interp("Interp call loops not allowed");
    if (!(valid_object(&oper[1])))
        abort_interp("Non-object argument (1)");
    if (!(valid_object(&oper[0])) && !is_home(&oper[0]) && oper[0].data.objref != NIL)
        abort_interp("Non-object argument (2)");
    {
        dbref victim, dest;

        victim = oper[1].data.objref;
        dest = oper[0].data.objref;

        if (dest == NIL) {
            switch (Typeof(victim)) {
                case TYPE_PLAYER:
                    dest = tp_player_start;
                    break;
                case TYPE_THING:
                    dest = OWNER(victim);
                    break;
                case TYPE_ROOM:
                    dest = tp_default_parent;
                    break;
                case TYPE_EXIT:
                    abort_interp("Invalid destination for an exit (1)");
                    break;
                case TYPE_PROGRAM:
                    dest = OWNER(victim);
                    break;
            }
        }

        if (Typeof(dest) == TYPE_EXIT)
            abort_interp("Destination argument is an exit");
        if (Typeof(victim) == TYPE_EXIT && (mlev < LM3))
            abort_interp(tp_noperm_mesg);
        if (!(FLAGS(victim) & JUMP_OK)
            && !permissions(mlev, ProgUID, victim) && (mlev < LM3))
            abort_interp("Object can't be moved");
        if (FLAG2(victim) & F2IMMOBILE)
            if (!(FLAG2(program) & F2IMMOBILE)) {
                envpropqueue(fr->descr, player,
                             OkObj(player) ? getloc(player) : -1, program,
                             program, NOTHING, "@immobile", "Immobile", mlev,
                             1);
                abort_interp
                    ("Object can't be moved, movement IMMOBILE restricted.");
            }
        interp_set_depth(fr);
        switch (Typeof(victim)) {
            case TYPE_PLAYER:
                if (Typeof(dest) != TYPE_ROOM && Typeof(dest) != TYPE_PLAYER &&
                    Typeof(dest) != TYPE_THING)
                    abort_interp("Bad destination");
                /* Check permissions */
                if (parent_loop_check(victim, dest))
                    abort_interp("Things can't contain themselves");
                if ((mlev < LM3)) {
                    if (!(FLAGS(dest) & VEHICLE)
                        && (Typeof(dest) == TYPE_THING
                            || Typeof(dest) == TYPE_PLAYER))
                        abort_interp("Destination is not a vehicle");
                    if (!(FLAGS(DBFETCH(victim)->location) & JUMP_OK)
                        && !permissions(mlev, ProgUID,
                                        DBFETCH(victim)->location))
                        abort_interp("Source not JUMP_OK");
                    if (!is_home(&oper[0]) && !(FLAGS(dest) & JUMP_OK)
                        && !permissions(mlev, ProgUID, dest))
                        abort_interp("Destination not JUMP_OK");
                    if (Typeof(dest) == TYPE_THING
                        && getloc(victim) != getloc(dest))
                        abort_interp("Not in same location as vehicle");
                }
                enter_room(fr->descr, victim, dest, program);
                break;
            case TYPE_THING:
                if (parent_loop_check(victim, dest))
                    abort_interp("A thing cannot contain itself");
                if (mlev < LM3 && (FLAGS(victim) & VEHICLE) &&
                    (FLAGS(dest) & VEHICLE) && Typeof(dest) != TYPE_THING)
                    abort_interp("Destination doesn't accept vehicles");
                if (mlev < LM3 && (FLAGS(victim) & ZOMBIE) &&
                    (FLAGS(dest) & ZOMBIE) && Typeof(dest) != TYPE_THING)
                    abort_interp("Destination doesn't accept zombies");
                ts_lastuseobject(program, victim);
            case TYPE_PROGRAM:
            {
                dbref matchroom = NOTHING;

                if (Typeof(dest) != TYPE_ROOM && Typeof(dest) != TYPE_PLAYER
                    && Typeof(dest) != TYPE_THING)
                    abort_interp("Bad destination");
                if ((mlev < LM3)) {
                    if (permissions(mlev, ProgUID, dest))
                        matchroom = dest;
                    if (permissions(mlev, ProgUID, DBFETCH(victim)->location))
                        matchroom = DBFETCH(victim)->location;
                    if (matchroom != NOTHING && !(FLAGS(matchroom) & JUMP_OK)
                        && !permissions(mlev, ProgUID, victim))
                        abort_interp(tp_noperm_mesg);
                }
            }
                if (Typeof(victim) == TYPE_THING && (FLAGS(victim) & ZOMBIE
                                                     || FLAGS(victim) &
                                                     VEHICLE)) {
                    enter_room(fr->descr, victim, dest, program);
                } else {
                    moveto(victim, dest);
                }
                break;
            case TYPE_EXIT:
                if (!permissions(mlev, ProgUID, victim)
                    || !permissions(mlev, ProgUID, dest))
                    abort_interp(tp_noperm_mesg);
                if (Typeof(dest) != TYPE_ROOM && Typeof(dest) != TYPE_THING &&
                    Typeof(dest) != TYPE_PLAYER)
                    abort_interp("Bad destination object");
                if (OkObj(dest)) {
                    if (!unset_source(ProgUID, getloc(PSafe), victim))
                        break;
                    set_source(ProgUID, victim, dest);
                }
                SetMLevel(victim, 0);
                break;
            case TYPE_ROOM:
                if (Typeof(dest) != TYPE_ROOM)
                    abort_interp("Bad destination");
                if (victim == GLOBAL_ENVIRONMENT)
                    abort_interp(tp_noperm_mesg);
                if (dest == HOME) {
                    if ((mlev < 3) && (!permissions(mlev, ProgUID, victim)
                                       && !permissions(mlev, ProgUID,
                                                       getloc(victim))))
                        abort_interp("Permission denied.");
                    dest = GLOBAL_ENVIRONMENT;
                } else {
                    if (!permissions(mlev, ProgUID, victim)
                        || !can_link_to(ProgUID, NOTYPE, dest))
                        abort_interp(tp_noperm_mesg);
                    if (parent_loop_check(victim, dest)) {
                        abort_interp("Parent room would create a loop");
                    }
                }
                ts_lastuseobject(program, victim);
                moveto(victim, dest);
                break;
            default:
                abort_interp("Invalid object type (1)");
        }
    }
    fr->level--;
    interp_set_depth(fr);
}

void
prim_pennies(PRIM_PROTOTYPE)
{
	int result;

    if (!valid_object(&oper[0]))
        abort_interp("Invalid dbref argument");
    CHECKREMOTE(oper[0].data.objref);
    switch (Typeof(oper[0].data.objref)) {
        case TYPE_PLAYER:
            result = DBFETCH(oper[0].data.objref)->sp.player.pennies;
            break;
        case TYPE_THING:
            result = DBFETCH(oper[0].data.objref)->sp.thing.value;
            break;
        default:
            abort_interp("Invalid object type argument");
    }
    
    PushInt(result);
}


void
prim_dbcomp(PRIM_PROTOTYPE)
{
	int result;

    if (oper[0].type != PROG_OBJECT || oper[1].type != PROG_OBJECT)
        abort_interp("Invalid argument type");
    result = oper[0].data.objref == oper[1].data.objref;
   
    PushInt(result);
}

void
prim_dbref(PRIM_PROTOTYPE)
{
	dbref ref;

    if (oper[0].type != PROG_INTEGER)
        abort_interp("Non-integer argument");
    ref = (dbref) oper[0].data.number;
    
    PushObject(ref);
}

void
prim_contents(PRIM_PROTOTYPE)
{
	dbref ref;
    
    if (!valid_object(&oper[0]))
        abort_interp("Invalid argument type");
    CHECKREMOTE(oper[0].data.objref);
    ref = DBFETCH(oper[0].data.objref)->contents;
    while (mlev < LM2 && ref != NOTHING &&
           (FLAGS(ref) & DARK) && !controls(ProgUID, ref))
        ref = DBFETCH(ref)->next;
    if (Typeof(oper[0].data.objref) != TYPE_PLAYER &&
        Typeof(oper[0].data.objref) != TYPE_PROGRAM)
        ts_lastuseobject(program, oper[0].data.objref);
    
    PushObject(ref);
}

void
prim_exits(PRIM_PROTOTYPE)
{
	dbref ref;
    
    if (!valid_object(&oper[0]))
        abort_interp("Invalid object");
    ref = oper[0].data.objref;
    CHECKREMOTE(ref);
    if ((mlev < LM2) && !permissions(mlev, ProgUID, ref))
        abort_interp(tp_noperm_mesg);
    switch (Typeof(ref)) {
        case TYPE_ROOM:
        case TYPE_THING:
            ts_lastuseobject(program, ref);
        case TYPE_PLAYER:
            ref = DBFETCH(ref)->exits;
            break;
        default:
            abort_interp("Invalid object");
    }
    
    PushObject(ref);
}


void
prim_next(PRIM_PROTOTYPE)
{
	dbref ref;
    
    if (!valid_object(&oper[0]))
        abort_interp("Invalid object");
    CHECKREMOTE(oper[0].data.objref);
    ref = DBFETCH(oper[0].data.objref)->next;
    while (mlev < LM2 && ref != NOTHING && Typeof(ref) != TYPE_EXIT &&
           ((FLAGS(ref) & DARK) || Typeof(ref) == TYPE_ROOM) &&
           !controls(ProgUID, ref))
        ref = DBFETCH(ref)->next;
    
    PushObject(ref);
}

void
prim_truename(PRIM_PROTOTYPE)
{
    const char *msg;
    char *msg2, *tempstr;
    char buf2[BUFFER_LEN];
	char buf[BUFFER_LEN];
	dbref ref;
    
    if ((oper[0].data.objref < 0) || (oper[0].data.objref >= db_top))
        abort_interp("Invalid argument type");
    ref = oper[0].data.objref;
    CHECKREMOTE(ref);
    if ((Typeof(ref) != TYPE_PLAYER) && (Typeof(ref) != TYPE_PROGRAM))
        ts_lastuseobject(program, ref);
    if ((Typeof(ref) == TYPE_PLAYER) || (Typeof(ref) == TYPE_THING)) {
        if ((msg = GETMESG(ref, "%n"))) {
            strcpy(buf, msg);
            
            strcpy(buf2, buf);
            if (lookup_player(buf2) != NOTHING) {
                strcpy(buf, NAME(ref));
            }
            PushString(buf);
            return;
        }
    }
    if (NAME(ref)) {
        strcpy(buf, NAME(ref));
    } else {
        buf[0] = '\0';
    }
    if (Typeof(ref) == TYPE_EXIT) {
        msg2 = strcpy(buf2, buf);
        tempstr = buf;
        while (*msg2 && (*msg2 != ';')) {
            *(tempstr++) = *(msg2++);
        }
        *tempstr = '\0';
    }
    
    PushString(buf);
}

void
prim_name(PRIM_PROTOTYPE)
{
	dbref ref;
	char buf[BUFFER_LEN];

    if (oper[0].type != PROG_OBJECT)
        abort_interp("Arguement (1) is not a dbref.");
    if ((oper[0].data.objref < 0) || (oper[0].data.objref >= db_top))
        abort_interp("Invalid argument type");
    ref = oper[0].data.objref;
    CHECKREMOTE(ref);
    if ((Typeof(ref) != TYPE_PLAYER) && (Typeof(ref) != TYPE_PROGRAM))
        ts_lastuseobject(program, ref);
    if (NAME(ref)) {
        strcpy(buf, PNAME(ref));
    } else {
        buf[0] = '\0';
    }
    
    PushString(buf);
}

void
prim_setname(PRIM_PROTOTYPE)
{
    char *password;
	char buf[BUFFER_LEN];
	dbref ref;

    if (!valid_object(&oper[1]))
        abort_interp("Invalid argument type (1)");
    if (oper[0].type != PROG_STRING)
        abort_interp("Non-string argument (2)");
    ref = oper[1].data.objref;
    if (!permissions(mlev, ProgUID, ref))
        abort_interp(tp_noperm_mesg);
    if (tp_db_readonly)
        abort_interp("Db is read-only");
    {
        const char *b = DoNullInd(oper[0].data.string);

        if (Typeof(ref) == TYPE_PLAYER) {
            strcpy(buf, b);
            b = buf;
            if (mlev < LMAGE)
                abort_interp(tp_noperm_mesg);
            /* split off password */
	    if (tp_spaces_in_playernames)
                for (password = buf; *password && !(*password == '='); password++) ;
	    else
                for (password = buf; *password && !isspace(*password); password++) ;

            /* eat whitespace */
            if (*password) {
                *password++ = '\0'; /* terminate name */
                while (*password && isspace(*password))
                    password++;
            }
            /* check for null password */
            /* The password just has to be 'yes' for players. */
            if (!*password) {
                abort_interp("Player namechange requires \"yes\" appended");
            } else if (strcmp(password, "yes")) {
                abort_interp("Incorrect password");
            } else if (string_compare(b, NAME(ref))
                       && !ok_player_name(b)) {
                abort_interp("You can't give a player that name");
            }
            /* remove any existing alias with this name */
            clear_alias(0, b);

            /* everything ok, notify */
            delete_player(ref);
            if (NAME(ref))
                free((void *) NAME(ref));
            ts_modifyobject(program, ref);
            NAME(ref) = alloc_string(b);
            add_player(ref);
        } else {
            if (!ok_name(b))
                abort_interp("Invalid name");
            if (NAME(ref))
                free((void *) NAME(ref));
            NAME(ref) = alloc_string(b);
            ts_modifyobject(program, ref);
            if (MLevel(ref))
                SetMLevel(ref, 0);
        }
    }
    
    
}

void
prim_match(PRIM_PROTOTYPE)
{
	dbref ref;   
    
    if (oper[0].type != PROG_STRING)
        abort_interp("Non-string argument");
    if (!oper[0].data.string) {
        ref = NOTHING;
        PushObject(ref);
        return;
    } else {
        struct match_data md;

        init_match(fr->descr, PSafe, oper[0].data.string->data, NOTYPE, &md);
        if (oper[0].data.string->data[0] == REGISTERED_TOKEN) {
            match_registered(&md);
        } else if (player != NOTHING) {
            match_all_exits(&md);
            match_neighbor(&md);
            match_possession(&md);
            match_me(&md);
            match_here(&md);
            match_home(&md);
            match_null(&md);
        }
        if (mlev >= LMAGE) {
            match_absolute(&md);
            match_player(&md);
        }
        ref = match_result(&md);
    }
    
    PushObject(ref);
}


void
prim_rmatch(PRIM_PROTOTYPE)
{
	dbref ref;

    if (oper[0].type != PROG_STRING)
        abort_interp("Invalid argument (2)");
    if (oper[1].type != PROG_OBJECT
        || oper[1].data.objref < 0
        || oper[1].data.objref >= db_top
        || Typeof(oper[1].data.objref) == TYPE_PROGRAM
        || Typeof(oper[1].data.objref) == TYPE_EXIT)
        abort_interp("Invalid argument (1)");
    CHECKREMOTE(oper[1].data.objref);
    {
        struct match_data md;

        init_match(fr->descr, PSafe, DoNullInd(oper[0].data.string), TYPE_THING,
                   &md);
        match_rmatch(oper[1].data.objref, &md);
        ref = match_result(&md);
    }
    
    
    PushObject(ref);
}


void
prim_copyobj(PRIM_PROTOTYPE)
{
	dbref ref;
  
    if (!valid_object(&oper[0]))
        abort_interp("Invalid object");
    CHECKREMOTE(oper[0].data.objref);
    if ((mlev < LMAGE) && (fr->already_created))
        abort_interp("Can't create any more objects");
    if (!tp_building || tp_db_readonly)
        abort_interp("Building is currently disabled");
    ref = oper[0].data.objref;
    if (Typeof(ref) != TYPE_THING)
        abort_interp("Invalid object type");
    if ((mlev < LMAGE) && !permissions(mlev, ProgUID, ref))
        abort_interp(tp_noperm_mesg);
    if (!ok_name(NAME(oper[0].data.objref)))
        abort_interp("Invalid name.");
    fr->already_created++;
    {
        dbref newobj;

        newobj = new_object(ProgUID);
        *DBFETCH(newobj) = *DBFETCH(ref);
        copyobj(PSafe, ref, newobj);
        
        PushObject(newobj);
    }
}
void
prim_isflagp(PRIM_PROTOTYPE)
{
    int tmp, result;

    if (oper[0].type != PROG_STRING)
        abort_interp("String expected.");
    result = (check_flag1(oper[0].data.string->data)
	      || check_flag4(oper[0].data.string->data)
              || check_flag2(oper[0].data.string->data, &tmp)
              || check_mlev(oper[0].data.string->data, &tmp));
    
    PushInt(result);
}

void
prim_set(PRIM_PROTOTYPE)
/* SET */
{
    int tmp2 = 0;
    int tmp4 = 0;
    char *flag;
    int tWiz = 0;
    int i, tmp, result = 0;
	dbref ref;
    
    if (oper[0].type != PROG_STRING)
        abort_interp("Invalid argument type (2)");
    if (!(oper[0].data.string))
        abort_interp("Empty string argument (2)");
    if (!valid_object(&oper[1]))
        abort_interp("Invalid object");
    if (tp_db_readonly)
        abort_interp("Db is read-only");
    ref = oper[1].data.objref;
    CHECKREMOTE(ref);
    flag = oper[0].data.string->data;
    tmp = 0;

    while (*flag == '!') {
        flag++;
        result = (!result);
    }

    if (!*flag)
        abort_interp("Empty flag");
#ifdef CONTROLS_SUPPORT
    if (((check_flag1(flag) == CHOWN_OK) && Typeof(ref) != TYPE_PLAYER) || (check_flag2(flag, &tWiz) == F2CONTROLS)) {
        if (!newpermissions(mlev, ProgUID, ref, 1))
            abort_interp(tp_noperm_mesg);
    } else {
        if (!newpermissions(mlev, ProgUID, ref, 0))
            abort_interp(tp_noperm_mesg);
    }
#else
    if (!permissions(mlev, ProgUID, ref))
        abort_interp(tp_noperm_mesg);
#endif
    if (result && Typeof(ref) == TYPE_THING) {
        dbref obj = DBFETCH(ref)->contents;

        for (; obj != NOTHING; obj = DBFETCH(obj)->next) {
            if (Typeof(obj) == TYPE_PLAYER) {
                abort_interp(tp_noperm_mesg);
            }
        }
    }
    tmp = check_flag1(flag);
    tmp2 = check_flag2(flag, &tWiz);
    tmp4 = check_flag4(flag);
    if (!tmp) {
        tmp = check_mlev(flag, &tWiz);
        if (tmp > LMPI || (tmp == LMPI && mlev < LWIZ)) {
            abort_interp(tp_noperm_mesg);
        } 
    }
    if (tmp == LMPI) {
        if (!flag_set_perms(ref, tmp, mlev, ProgUID))
            abort_interp(tp_noperm_mesg);
        if (!result) {
            ts_modifyobject(program, ref);
            SetMLevel(ref, LMPI);
            DBDIRTY(ref);
        } else {
            ts_modifyobject(program, ref);
            SetMLevel(ref, 0);
            DBDIRTY(ref);
        }
    } 
    if (tmp && tmp !=LMPI) {
        if (!flag_set_perms(ref, tmp, mlev, ProgUID))
            abort_interp(tp_noperm_mesg);
        if (!result) {
            ts_modifyobject(program, ref);
            FLAGS(ref) |= tmp;
            DBDIRTY(ref);
        } else {
            ts_modifyobject(program, ref);
            FLAGS(ref) &= ~tmp;
            DBDIRTY(ref);
        }
    } 
    if (tmp2 && !tmp) {
        if (!flag_set_perms2(ref, tmp2, mlev, ProgUID))
            abort_interp(tp_noperm_mesg);
        if (!result) {
            ts_modifyobject(program, ref);
            FLAG2(ref) |= tmp2;
            DBDIRTY(ref);
        } else {
            ts_modifyobject(program, ref);
            FLAG2(ref) &= ~tmp2;
            DBDIRTY(ref);
        }
    } 
    if (tmp4 && !tmp2 && !tmp) {
        i=0; while (1 << i != tmp4) i++;
	if (lflag_mlev[i]!=-1)
	    if ((lflag_mlev[i]==0 && !controls(ProgUID, ref)) &&
		(lflag_mlev[i]>0 && mlev < lflag_mlev[i]))
		abort_interp(tp_noperm_mesg);
        if (!result) {
            ts_modifyobject(program, ref);
            FLAG4(ref) |= tmp4;
            DBDIRTY(ref);
        } else {
            ts_modifyobject(program, ref);
            FLAG4(ref) &= ~tmp4;
            DBDIRTY(ref);
        }
    }
}

void
prim_mlevel(PRIM_PROTOTYPE)
/* MLEVEL */
{
	dbref ref;
	int result;

    if (!valid_object(&oper[0]))
        abort_interp("Invalid object");
    ref = oper[0].data.objref;
    CHECKREMOTE(ref);
    result = MLevel(ref);
    
    PushInt(result);
}

void
prim_flagp(PRIM_PROTOTYPE)
/* FLAG? */
{
	dbref ref;
	int result;
    
    if (oper[0].type != PROG_STRING)
        abort_interp("Invalid argument type (2)");
    if (!(oper[0].data.string))
        abort_interp("Empty string argument (2)");
    if ((oper[1].data.objref < 0) || (oper[1].data.objref >= db_top))
        abort_interp("Invalid object");
    ref = oper[1].data.objref;
    CHECKREMOTE(ref);
    {
        char *flag = oper[0].data.string->data;

        result = has_flagp(ref, flag, mlev);
        if (result == -1)
            result = 0;         /* Return 0 on unknown flags per old behavior. */
        if (result == -2)
            abort_interp("Permission denied");
	result = result ? 1 : 0;
    }
    
    PushInt(result);
}

void
prim_powerp(PRIM_PROTOTYPE)
{
    int pow = 0;
    int result = 0;
    
    if (oper[0].type != PROG_STRING)
        abort_interp("Invalid argument type (2)");
    if (!(oper[0].data.string))
        abort_interp("Empty string argument (2)");
    if (!valid_object(&oper[1]))
        abort_interp("Invalid object");
    if (Typeof(oper[1].data.objref) != TYPE_PLAYER)
        abort_interp("Not a valid player");
    pow = check_power(oper[0].data.string->data);
    if (pow)
        if (POWERS(oper[1].data.objref) & pow)
            result = 1;
        
    PushInt(result);
}

void
prim_ispowerp(PRIM_PROTOTYPE)
{
    int pow = 0;
	int result;
    
    if (oper[0].type != PROG_STRING)
        abort_interp("Invalid argument type (2)");
    if (!(oper[0].data.string))
        abort_interp("Empty string argument (2)");
    pow = check_power(oper[0].data.string->data);
    result = !(!pow);
    
    PushInt(result);
}

void
prim_playerp(PRIM_PROTOTYPE)
{
	int result;
	dbref ref;
    
    if (oper[0].type != PROG_OBJECT)
        abort_interp("Invalid argument type");
    if (!valid_object(&oper[0]) && !is_home(&oper[0])) {
        result = 0;
    } else {
        ref = oper[0].data.objref;
        CHECKREMOTE(ref);
        result = (Typeof(ref) == TYPE_PLAYER);
    }
    PushInt(result);
}


void
prim_thingp(PRIM_PROTOTYPE)
{
	int result;
	dbref ref;
    
    if (oper[0].type != PROG_OBJECT)
        abort_interp("Invalid argument type");
    if (!valid_object(&oper[0]) && !is_home(&oper[0])) {
        result = 0;
    } else {
        ref = oper[0].data.objref;
        CHECKREMOTE(ref);
        result = (Typeof(ref) == TYPE_THING);
    }
    PushInt(result);
}


void
prim_roomp(PRIM_PROTOTYPE)
{
	int result;
	dbref ref;
    
    if (oper[0].type != PROG_OBJECT)
        abort_interp("Invalid argument type");
    if (!valid_object(&oper[0]) && !is_home(&oper[0])) {
        result = 0;
    } else {
        ref = oper[0].data.objref;
        CHECKREMOTE(ref);
        result = (Typeof(ref) == TYPE_ROOM);
    }
    PushInt(result);
}


void
prim_programp(PRIM_PROTOTYPE)
{
	int result;
	dbref ref;
    
    if (oper[0].type != PROG_OBJECT)
        abort_interp("Invalid argument type");
    if (!valid_object(&oper[0]) && !is_home(&oper[0])) {
        result = 0;
    } else {
        ref = oper[0].data.objref;
        CHECKREMOTE(ref);
        result = (Typeof(ref) == TYPE_PROGRAM);
    }
    PushInt(result);
}


void
prim_exitp(PRIM_PROTOTYPE)
{
	int result;
	dbref ref;

    if (oper[0].type != PROG_OBJECT)
        abort_interp("Invalid argument type");
    if (!valid_object(&oper[0]) && !is_home(&oper[0])) {
        result = 0;
    } else {
        ref = oper[0].data.objref;
        CHECKREMOTE(ref);
        result = (Typeof(ref) == TYPE_EXIT);
    }
    PushInt(result);
}


void
prim_okp(PRIM_PROTOTYPE)
{
	int result;
    
    result = (valid_object(&oper[0]));
    PushInt(result);
}

void
prim_location(PRIM_PROTOTYPE)
{
	dbref ref;

    if (!valid_object(&oper[0]))
        abort_interp("Invalid object");
    CHECKREMOTE(oper[0].data.objref);
    ref = DBFETCH(oper[0].data.objref)->location;
    
    PushObject(ref);
}

void
prim_owner(PRIM_PROTOTYPE)
{
	dbref ref;

    if (!valid_object(&oper[0]))
        abort_interp("Invalid object");
    CHECKREMOTE(oper[0].data.objref);
    ref = OWNER(oper[0].data.objref);
    
    PushObject(ref);
}

void
prim_controls(PRIM_PROTOTYPE)
{
	int result;

    if (!valid_object(&oper[0]))
        abort_interp("Invalid object (2)");
    if (!valid_object(&oper[1]))
        abort_interp("Invalid object (1)");
    CHECKREMOTE(oper[0].data.objref);
    result = controls(oper[1].data.objref, oper[0].data.objref);
    
    PushInt(result);
}

void
prim_truecontrols(PRIM_PROTOTYPE)
{
	int result;

    if (!valid_object(&oper[0]))
        abort_interp("Invalid object (2)");
    if (!valid_object(&oper[1]))
        abort_interp("Invalid object (1)");
    CHECKREMOTE(oper[0].data.objref);
    result = truecontrols(oper[1].data.objref, oper[0].data.objref);
    
    PushInt(result);
}

void
prim_getlink(PRIM_PROTOTYPE)
{
	dbref ref;

    if (!valid_object(&oper[0]))
        abort_interp("Invalid object");
    CHECKREMOTE(oper[0].data.objref);
    if (Typeof(oper[0].data.objref) == TYPE_PROGRAM)
        abort_interp("Illegal object referenced");
    switch (Typeof(oper[0].data.objref)) {
        case TYPE_EXIT:
            ref = (DBFETCH(oper[0].data.objref)->sp.exit.ndest) ?
                (DBFETCH(oper[0].data.objref)->sp.exit.dest)[0] : NOTHING;
            break;
        case TYPE_PLAYER:
            ref = DBFETCH(oper[0].data.objref)->sp.player.home;
            break;
        case TYPE_THING:
            ref = DBFETCH(oper[0].data.objref)->sp.thing.home;
            break;
        case TYPE_ROOM:
            ref = DBFETCH(oper[0].data.objref)->sp.room.dropto;
            break;
        default:
            ref = NOTHING;
            break;
    }
    
    PushObject(ref);
}

void
prim_getlinks(PRIM_PROTOTYPE)
{
    int i, count;
    dbref ref2, ref;

    if (!valid_object(&oper[0]))
        abort_interp("Invalid object.");
    CHECKREMOTE(oper[0].data.objref);
    if (Typeof(oper[0].data.objref) == TYPE_PROGRAM)
        abort_interp("Illegal object referenced.");
    ref2 = oper[0].data.objref;
    
    switch (Typeof(ref2)) {
        case TYPE_EXIT:
            count = DBFETCH(ref2)->sp.exit.ndest;
            for (i = 0; i < count; i++) {
                PushObject((DBFETCH(ref2)->sp.exit.dest)[i]);
            }
            PushInt(count);
            break;
        case TYPE_PLAYER:
            ref = DBFETCH(ref2)->sp.player.home;
            count = 1;
            PushObject(ref);
            PushInt(count);
            break;
        case TYPE_THING:
            ref = DBFETCH(ref2)->sp.thing.home;
            count = 1;
            PushObject(ref);
            PushInt(count);
            break;
        case TYPE_ROOM:
            ref = DBFETCH(ref2)->sp.room.dropto;
            if (ref != NOTHING) {
                count = 0;
                PushInt(count);
            } else {
                count = 1;
                PushObject(ref);
                PushInt(count);
            }
            break;
        default:
            count = 0;
            PushInt(count);
            break;
    }
}

int
prog_can_link_to(int mlev, dbref who, object_flag_type what_type, dbref where)
{
    if (where == HOME || where == NIL)
        return 1;
    if (where < 0 || where >= db_top)
        return 0;
    switch (what_type) {
        case TYPE_EXIT:
            return (permissions(mlev, who, where) || (FLAGS(where) & LINK_OK));
            break;
        case TYPE_PLAYER:
            return (Typeof(where) == TYPE_ROOM && (permissions(mlev, who, where)
                                                   || Linkable(where)));
            break;
        case TYPE_ROOM:
            return ((Typeof(where) == TYPE_ROOM || Typeof(where) == TYPE_THING)
                    && (permissions(mlev, who, where) || Linkable(where)));
            break;
        case TYPE_THING:
            return ((Typeof(where) == TYPE_ROOM || Typeof(where) == TYPE_PLAYER
                     || Typeof(where) == TYPE_THING)
                    && (permissions(mlev, who, where) || Linkable(where)));
            break;
        case NOTYPE:
            return (permissions(mlev, who, where) || (FLAGS(where) & LINK_OK) ||
                    (Typeof(where) != TYPE_THING && (FLAGS(where) & ABODE)));
            break;
    }
    return 0;
}


void
prim_setlink(PRIM_PROTOTYPE)
{
	dbref ref;

    if ((oper[0].type != PROG_OBJECT) || (oper[1].type != PROG_OBJECT))
        abort_interp("setlink requires two dbrefs");
    if (!valid_object(&oper[1]))
        abort_interp("Invalid object (1)");
    if (tp_db_readonly)
        abort_interp(DBRO_MESG);
    ref = oper[1].data.objref;
    if (oper[0].data.objref == -1) {
        if (!permissions(mlev, ProgUID, ref))
            abort_interp(tp_noperm_mesg);
        switch (Typeof(ref)) {
            case TYPE_EXIT:
                DBSTORE(ref, sp.exit.ndest, 0);
                if (DBFETCH(ref)->sp.exit.dest) {
                    free((void *) DBFETCH(ref)->sp.exit.dest);
                    DBSTORE(ref, sp.exit.dest, NULL);
                }
                if (MLevel(ref))
                    SetMLevel(ref, 0);
                break;
            case TYPE_ROOM:
                DBSTORE(ref, sp.room.dropto, NOTHING);
                break;
            default:
                abort_interp("Invalid object (1)");
        }
    } else {
        if (!valid_object(&oper[0]) && oper[0].data.objref != HOME && oper[0].data.objref != NIL)
            abort_interp("Invalid object (2)");
        if ((!(Typeof(ref) == TYPE_PLAYER || Typeof(ref) == TYPE_EXIT)) && oper[0].data.objref == NIL)
            abort_interp("Only players and exits can be linked to NIL (1)");
        if (Typeof(ref) == TYPE_PROGRAM)
            abort_interp("Program objects are not linkable (1)");
        if (!prog_can_link_to(mlev, ProgUID, Typeof(ref), oper[0].data.objref))
            abort_interp("Can't link source to destination");
        switch (Typeof(ref)) {
            case TYPE_EXIT:
                if (DBFETCH(ref)->sp.exit.ndest != 0) {
                    if (!permissions(mlev, ProgUID, ref))
                        abort_interp(tp_noperm_mesg);
                    if ((DBFETCH(ref)->sp.exit.dest)[0] != NIL)
                        abort_interp("Exit is already linked");
                }
                if (exit_loop_check(ref, oper[0].data.objref))
                    abort_interp("Link would cause a loop");
                DBFETCH(ref)->sp.exit.ndest = 1;
                DBFETCH(ref)->sp.exit.dest = (dbref *) malloc(sizeof(dbref));
                (DBFETCH(ref)->sp.exit.dest)[0] = oper[0].data.objref;
                break;
            case TYPE_PLAYER:
                if (!permissions(mlev, ProgUID, ref))
                    abort_interp(tp_noperm_mesg);
                DBFETCH(ref)->sp.player.home = oper[0].data.objref;
                break;
            case TYPE_THING:
                if (!permissions(mlev, ProgUID, ref))
                    abort_interp(tp_noperm_mesg);
                if (parent_loop_check(ref, oper[0].data.objref))
                    abort_interp("That would cause a parent paradox");
                DBFETCH(ref)->sp.thing.home = oper[0].data.objref;
                break;
            case TYPE_ROOM:
                if (!permissions(mlev, ProgUID, ref))
                    abort_interp(tp_noperm_mesg);
                DBFETCH(ref)->sp.room.dropto = oper[0].data.objref;
                break;
        }
    }
}

void
prim_setown(PRIM_PROTOTYPE)
{
	dbref ref;

    if (!valid_object(&oper[1]))
        abort_interp("Invalid argument (1)");
    if (!valid_player(&oper[0]))
        abort_interp("Invalid argument (2)");
    if (tp_db_readonly)
        abort_interp(DBRO_MESG);
    ref = oper[1].data.objref;
    if ((mlev < LWIZ) && oper[0].data.objref != player)
        abort_interp(tp_noperm_mesg);
    if ((mlev < MLevel(OWNER(oper[0].data.objref))) ||
        (mlev < MLevel(OWNER(oper[1].data.objref))))
        abort_interp(tp_noperm_mesg);
    if ((mlev < LWIZ) && (!(FLAGS(ref) & CHOWN_OK) ||
                          !test_lock(fr->descr, PSafe, ref, CHLK_PROP)))
        abort_interp(tp_noperm_mesg);
    switch (Typeof(ref)) {
        case TYPE_ROOM:
            if ((mlev < LMAGE) && DBFETCH(PSafe)->location != ref)
                abort_interp(tp_noperm_mesg);
            break;
        case TYPE_THING:
            if ((mlev < LMAGE) && DBFETCH(ref)->location != PSafe)
                abort_interp(tp_noperm_mesg);
            break;
        case TYPE_PLAYER:
            abort_interp(tp_noperm_mesg);
        case TYPE_EXIT:
        case TYPE_PROGRAM:
            break;
        case TYPE_GARBAGE:
            abort_interp("Can't chown garbage");
    }
    OWNER(ref) = OWNER(oper[0].data.objref);
    DBDIRTY(ref);
}

void
prim_newobject(PRIM_PROTOTYPE)
{
	dbref ref;

    if ((mlev < LMAGE) && (fr->already_created))
        abort_interp("Only 1 per run");
    CHECKOFLOW(1);
    ref = oper[1].data.objref;
    if (!valid_object(&oper[1]))
        abort_interp("Invalid argument (1)");
    if (Typeof(ref) != TYPE_ROOM && Typeof(ref) != TYPE_THING
        && Typeof(ref) != TYPE_PLAYER)
        abort_interp("Invalid destination in arguement (1).");
    if (oper[0].type != PROG_STRING)
        abort_interp("Invalid argument (2)");
    CHECKREMOTE(ref);
    if (!permissions(mlev, ProgUID, ref))
        abort_interp(tp_noperm_mesg);
    if (!tp_building || tp_db_readonly)
        abort_interp(NOBUILD_MESG);
    {
        const char *b = DoNullInd(oper[0].data.string);
        dbref loc;

        if (!ok_name(b))
            abort_interp("Invalid name (2)");

        ref = new_object(ProgUID);

        /* initialize everything */
        NAME(ref) = alloc_string(b);
        DBFETCH(ref)->location = oper[1].data.objref;
        OWNER(ref) = OWNER(ProgUID);
        DBFETCH(ref)->sp.thing.value = 1;
        DBFETCH(ref)->exits = NOTHING;
        FLAGS(ref) = TYPE_THING;

        if ((loc = DBFETCH(PSafe)->location) != NOTHING && controls(PSafe, loc)) {
            DBFETCH(ref)->sp.thing.home = loc; /* home */
        } else {
            DBFETCH(ref)->sp.thing.home = DBFETCH(PSafe)->sp.player.home;
            /* set to player's home instead */
        }
    }

    /* link it in */
    PUSH(ref, DBFETCH(oper[1].data.objref)->contents);
    DBDIRTY(oper[1].data.objref);

    PushObject(ref);
}

void
prim_newroom(PRIM_PROTOTYPE)
{
	dbref ref;

    if ((mlev < LMAGE) && (fr->already_created))
        abort_interp("Only 1 per run");
    CHECKOFLOW(1);
    ref = oper[1].data.objref;
    if (!valid_object(&oper[1]) || (Typeof(ref) != TYPE_ROOM))
        abort_interp("Invalid argument (1)");
    if (oper[0].type != PROG_STRING)
        abort_interp("Invalid argument (2)");
    if (!permissions(mlev, ProgUID, ref))
        abort_interp(tp_noperm_mesg);
    if (!tp_building || tp_db_readonly)
        abort_interp(NOBUILD_MESG);
    {
        const char *b = DoNullInd(oper[0].data.string);

        if (!ok_name(b))
            abort_interp("Invalid name (2)");

        ref = new_object(ProgUID);

        /* Initialize everything */
        NAME(ref) = alloc_string(b);
        DBFETCH(ref)->location = oper[1].data.objref;
        OWNER(ref) = OWNER(ProgUID);
        DBFETCH(ref)->exits = NOTHING;
        DBFETCH(ref)->sp.room.dropto = NOTHING;
        FLAGS(ref) = TYPE_ROOM | (FLAGS(PSafe) & JUMP_OK);
        PUSH(ref, DBFETCH(oper[1].data.objref)->contents);
        DBDIRTY(ref);
        DBDIRTY(oper[1].data.objref);

        PushObject(ref);
    }
}

void
prim_newexit(PRIM_PROTOTYPE)
{
	dbref ref;

    CHECKOFLOW(1);
    ref = oper[1].data.objref;
    if (!valid_object(&oper[1])
        || ((!valid_player(&oper[1])) && (Typeof(ref) != TYPE_ROOM)
            && (Typeof(ref) != TYPE_THING)))
        abort_interp("Invalid argument (1)");
    if (oper[0].type != PROG_STRING)
        abort_interp("Invalid argument (2)");
    CHECKREMOTE(ref);
    if (!permissions(mlev, ProgUID, ref))
        abort_interp(tp_noperm_mesg);
    if (!tp_building || tp_db_readonly)
        abort_interp(NOBUILD_MESG);
    {
        const char *b = DoNullInd(oper[0].data.string);

        if (!ok_name(b))
            abort_interp("Invalid name (2)");

        ref = new_object(ProgUID);

        /* initialize everything */
        NAME(ref) = alloc_string(oper[0].data.string->data);
        DBFETCH(ref)->location = oper[1].data.objref;
        OWNER(ref) = OWNER(ProgUID);
        FLAGS(ref) = TYPE_EXIT;
        DBFETCH(ref)->sp.exit.ndest = 0;
        DBFETCH(ref)->sp.exit.dest = NULL;

        /* link it in */
        PUSH(ref, DBFETCH(oper[1].data.objref)->exits);
        DBDIRTY(oper[1].data.objref);

        /* If autolinking, link it to NIL */
        if (tp_autolinking) {
            DBFETCH(ref)->sp.exit.ndest = 1;
            DBFETCH(ref)->sp.exit.dest = (dbref *) malloc(sizeof(dbref));
            (DBFETCH(ref)->sp.exit.dest)[0] = NIL;
        }
        PushObject(ref);
    }
}

void
prim_recycle(PRIM_PROTOTYPE)
{
	int result;

    if (oper[0].type != PROG_OBJECT)
        abort_interp("Non-object argument (1)");
    if (!valid_object(&oper[0]))
        abort_interp("Invalid object (1)");
    result = oper[0].data.objref;
    if ((mlev < LM3) || !permissions(mlev, ProgUID, result))
        abort_interp(tp_noperm_mesg);
    if (tp_db_readonly)
        abort_interp(DBRO_MESG);
    if ((result == tp_player_start) || (result == GLOBAL_ENVIRONMENT))
        abort_interp("Cannot recycle that room");
    if (Typeof(result) == TYPE_PLAYER)
        abort_interp("Cannot recycle a player");
    if (result == program)
        abort_interp("Cannot recycle currently running program.");
    {
        int ii;

        for (ii = 0; ii < fr->caller.top; ii++)
            if (OkObj(fr->caller.st[ii])
                && (Typeof(fr->caller.st[ii]) == TYPE_PROGRAM)
                && (fr->caller.st[ii] == result))
                abort_interp("Cannot recycle active program");
    }
    if (Typeof(result) == TYPE_EXIT)
        if (!unset_source(PSafe, DBFETCH(PSafe)->location, result))
            abort_interp("Cannot recycle old style exits");
    
    recycle(fr->descr, PSafe, result);
}


void
prim_setlockstr(PRIM_PROTOTYPE)
{
	dbref ref;
	int result;

    if (!valid_object(&oper[1]))
        abort_interp("Invalid argument type (1)");
    if (oper[0].type != PROG_STRING)
        abort_interp("Non-string argument (2)");
    ref = oper[1].data.objref;
    if (!permissions(mlev, ProgUID, ref))
        abort_interp(tp_noperm_mesg);
    if (tp_db_readonly)
        abort_interp(DBRO_MESG);
    result = setlockstr(fr->descr, PSafe, ref,
                        oper[0].data.string ? oper[0].data.string->
                        data : (char *) "");
    
    
    PushInt(result);
}


void
prim_getlockstr(PRIM_PROTOTYPE)
{
	dbref ref;

    if ((oper[0].data.objref < 0) || (oper[0].data.objref >= db_top))
        abort_interp("Invalid dbref argument.");
    ref = oper[0].data.objref;
    CHECKREMOTE(ref);
    if (mlev < LM2)
        abort_interp(tp_noperm_mesg);
    {
        char *tmpstr;
        tmpstr = (char *) unparse_boolexp(PSafe, GETLOCK(ref), 0);
        
        PushString(tmpstr);
    }
}


void
prim_part_pmatch(PRIM_PROTOTYPE)
{
    dbref ref;

    if (oper[0].type != PROG_STRING)
        abort_interp("Non-string argument");
    if (!oper[0].data.string)
        abort_interp("Empty string argument");

    ref = partial_pmatch(oper[0].data.string->data);
    
    PushObject(ref);
}


void
prim_checkpassword(PRIM_PROTOTYPE)
{
    char *ptr;
    char pad_char[] = "";
	dbref ref;
	int result;

    if (oper[1].type != PROG_OBJECT)
        abort_interp("Player dbref expected (1)");
    ref = oper[1].data.objref;
    if ((ref != NOTHING && !valid_player(&oper[1])) || ref == NOTHING)
        abort_interp("Player dbref expected (1)");
    if (oper[0].type != PROG_STRING)
        abort_interp("Password string expected (2)");
    ptr = oper[0].data.string ? oper[0].data.string->data : pad_char;
    result = check_password(ref, ptr);

    PushInt(result);
}

void
prim_pmatch(PRIM_PROTOTYPE)
{
    dbref ref;
    char *buff;
    char buf[BUFFER_LEN];
    int result;

    if (oper[0].type != PROG_STRING)
        abort_interp("Non-string argument.");
    if (!oper[0].data.string)
        abort_interp("Empty string argument.");
    buff = strcpy(buf, oper[0].data.string->data);
    while (isspace(*buff))
        buff++;
    if (*buff == '#') {
        (void) buff++;
        if (number(buff)) {
            ref = atoi(buff);
            if (ref <= 0 || ref >= db_top) {
                ref = NOTHING;
            } else {
                if (Typeof(ref) != TYPE_PLAYER) {
                    ref = NOTHING;
                }
            }
        } else {
            ref = NOTHING;
        }
    } else {
        if (!string_compare(buff, "me")) {
            ref = player;
        } else {
            ref = lookup_player(buff);
            if (ref == NOTHING) {
                for (result = pcount(); result; result--) {
                    if (string_prefix(PNAME(pdbref(result)), buff)) {
                        if (ref != NOTHING) {
                            ref = AMBIGUOUS;
                            break;
                        } else {
                            ref = pdbref(result);
                        }
                    }
                }
            }
        }
    }
    
    PushObject(ref);
}

void
prim_nextentrance(PRIM_PROTOTYPE)
{
    dbref linkref, ref;
    int foundref = 0;
    int i, count;  
    
    linkref = oper[1].data.objref;
    ref = oper[0].data.objref;
    if (!valid_object(&oper[1]) && (linkref != NOTHING) && (linkref != HOME))
        abort_interp("Invalid link reference object (2)");
    if (!valid_object(&oper[0]) && ref != NOTHING)
        abort_interp("Invalid reference object (1)");
    if (linkref == HOME)
        linkref = DBFETCH(PSafe)->sp.player.home;
    (void) ref++;
    for (; ref < db_top; ref++) {
        oper[0].data.objref = ref;
        if (valid_object(&oper[0])) {
            switch (Typeof(ref)) {
                case TYPE_PLAYER:
                    if (DBFETCH(ref)->sp.player.home == linkref)
                        foundref = 1;
                    break;
                case TYPE_ROOM:
                    if (DBFETCH(ref)->sp.room.dropto == linkref)
                        foundref = 1;
                    break;
                case TYPE_THING:
                    if (DBFETCH(ref)->sp.thing.home == linkref)
                        foundref = 1;
                    break;
                case TYPE_EXIT:
                    count = DBFETCH(ref)->sp.exit.ndest;
                    for (i = 0; i < count; i++) {
                        if (DBFETCH(ref)->sp.exit.dest[i] == linkref)
                            foundref = 1;
                    }
                    break;
            }
            if (foundref)
                break;
        }
    }
    if (!foundref)
        ref = NOTHING;
    
    PushObject(ref);
}

void
prim_newplayer(PRIM_PROTOTYPE)
{
	dbref ref;

    if (oper[0].type != PROG_STRING)
        abort_interp("Non-string argument.");
    if (!oper[0].data.string)
        abort_interp("Empty string argument.");
    if (oper[1].type != PROG_STRING)
        abort_interp("Non-string argument.");
    if (!oper[1].data.string)
        abort_interp("Empty string argument.");
    if (!ok_player_name(oper[1].data.string->data)
        || !ok_password(oper[0].data.string->data))
        abort_interp("Invalid player name or password.");
    if (tp_db_readonly)
        abort_interp("The MUCK is read only.");

    ref = create_player(ProgUID, oper[1].data.string->data, oper[0].data.string->data);
    if (ref != NOTHING)
        log_status("PCRE[MUF]: %s(%d) by %s(%d)\n", NAME(ref), (int) ref,
                   OkObj(player) ? NAME(player) : "(Login)", player);

    PushObject(ref);
}

void
prim_copyplayer(PRIM_PROTOTYPE)
{
    dbref newplayer, ref;
    char *name, *password;
    struct object *newp;

    if (oper[0].type != PROG_STRING)
        abort_interp("Non-string argument.");
    if (!oper[0].data.string)
        abort_interp("Empty string argument.");
    if (oper[1].type != PROG_STRING)
        abort_interp("Non-string argument.");
    if (!oper[1].data.string)
        abort_interp("Empty string argument.");
    ref = oper[2].data.objref;
    if ((ref != NOTHING && !valid_player(&oper[2])) || ref == NOTHING)
        abort_interp("Player dbref expected (1)");
    CHECKREMOTE(ref);

    name = oper[1].data.string->data;
    password = oper[0].data.string->data;

    if (!ok_player_name(name) || !ok_password(password))
        abort_interp("Invalid player name or password.");
    if (tp_db_readonly)
        abort_interp("The MUCK is read only.");

    /* else he doesn't already exist, create him */
    newplayer = new_object(ProgUID);
    newp = DBFETCH(newplayer);

    /* initialize everything */
    NAME(newplayer) = alloc_string(name);
    FLAGS(newplayer) = TYPE_PLAYER;

    FLAGS(newplayer) = FLAGS(ref);
    FLAG2(newplayer) = FLAG2(ref);

    newp->properties = copy_prop(ref);
    newp->exits = NOTHING;
#ifdef DISKBASE
    newp->propsfpos = 0;
    newp->propsmode = PROPS_UNLOADED;
    newp->propstime = 0;
    newp->nextold = NOTHING;
    newp->prevold = NOTHING;
    dirtyprops(newplayer);
#endif

    OWNER(newplayer) = newplayer;
    newp->sp.player.home = DBFETCH(ref)->sp.player.home;
    newp->sp.player.pennies = DBFETCH(ref)->sp.player.pennies;
    newp->sp.player.password = NULL;
    newp->sp.player.curr_prog = NOTHING;
    DBFETCH(newplayer)->sp.player.insert_mode = 0;

    /* Yet again, set_password */
    set_password(newplayer, password);

    /* link him to player_start */
    PUSH(newplayer, DBFETCH(DBFETCH(ref)->sp.player.home)->contents);
    add_player(newplayer);
    newp->location = DBFETCH(ref)->sp.player.home;
    DBDIRTY(newplayer);
    DBDIRTY(tp_player_start);
    if (MLevel(newplayer) > LM3)
        SetMLevel(newplayer, LM3);

    log_status("PCRE[MUF]: %s(%d) by %s(%d)\n",
               NAME(newplayer), (int) newplayer,
               OkObj(player) ? NAME(player) : "(Login)", player);

    PushObject(newplayer);
}

void
prim_toadplayer(PRIM_PROTOTYPE)
{
    dbref victim;
    dbref recipient;
    dbref stuff;
    char buf[BUFFER_LEN];

    victim = oper[0].data.objref;
    if ((victim != NOTHING && !valid_player(&oper[0])) || victim == NOTHING)
        abort_interp("Player dbref expected for player to be toaded (1)");
    recipient = oper[1].data.objref;
    if ((recipient != NOTHING && !valid_player(&oper[1])) || recipient == NOTHING)
        abort_interp("Player dbref expected for recipient (2)");
    CHECKREMOTE(victim);
    CHECKREMOTE(recipient);

    if (Typeof(victim) != TYPE_PLAYER) {
        abort_interp("You can only frob players.");
        return;
    }
    if (Typeof(recipient) != TYPE_PLAYER) {
        abort_interp("Only players can receive the objects.");
        return;
    }
    if (get_property_class(victim, "@/precious")) {
        abort_interp("That player is precious.");
        return;
    }
    if (TMage(victim)) {
        abort_interp("You can't frob a wizard.");
        return;
    }

    /* we're ok, do it */
    send_contents(fr->descr, victim, HOME);
    for (stuff = 0; stuff < db_top; stuff++) {
        if (OWNER(stuff) == victim) {
            switch (Typeof(stuff)) {
                case TYPE_PROGRAM:
                    dequeue_prog(stuff, 0, NULL); /* dequeue player's progs */
                    FLAGS(stuff) &= ~(ABODE | W1 | W2 | W3 | W4);
                    SetMLevel(stuff, 0);
                case TYPE_ROOM:
                case TYPE_THING:
                case TYPE_EXIT:
                    OWNER(stuff) = recipient;
                    DBDIRTY(stuff);
                    break;
            }
        }
        if (Typeof(stuff) == TYPE_THING &&
            DBFETCH(stuff)->sp.thing.home == victim) {
            DBSTORE(stuff, sp.thing.home, tp_player_start);
        }
    }
    if (DBFETCH(victim)->sp.player.password) {
        free((void *) DBFETCH(victim)->sp.player.password);
        DBFETCH(victim)->sp.player.password = 0;
    }
    dequeue_prog(victim, 0, fr);    /* dequeue progs that player's running */

    log_status("FROB[MUF]: %s(%d) by %s(%d)\n", NAME(victim),
               victim, OkObj(player) ? NAME(player) : "(Login)", player);

    boot_player_off(victim);

    if (DBFETCH(victim)->sp.player.descrs) {
        free(DBFETCH(victim)->sp.player.descrs);
        DBFETCH(victim)->sp.player.descrs = NULL;
        DBFETCH(victim)->sp.player.descr_count = 0;
    }
    delete_player(victim);
    /* reset name */
    sprintf(buf, "The soul of %s", PNAME(victim));
    free((void *) NAME(victim));
    NAME(victim) = alloc_string(buf);
    DBDIRTY(victim);
    FLAGS(victim) = TYPE_THING;
    FLAG2(victim) = 0;
    FLAG3(victim) = 0;
    FLAG4(victim) = 0;
    POWERSDB(victim) = 0;
    POWER2DB(victim) = 0;
    OWNER(victim) = recipient; 
    DBFETCH(victim)->sp.thing.value = 1;

    if (tp_recycle_frobs)
        recycle(fr->descr, recipient, victim);
}

void
prim_objmem(PRIM_PROTOTYPE)
{
    int i;
	dbref ref;

    if (oper[0].type != PROG_OBJECT)
        abort_interp("Argument must be a dbref.");
    ref = oper[0].data.objref;
    if (ref >= db_top || ref <= NOTHING)
        abort_interp("Dbref is not an object nor garbage.");
    
    i = size_object(ref, 0);
    PushInt(i);
}

void
prim_movepennies(PRIM_PROTOTYPE)
{
    int result2, result;
    dbref ref2, ref;

    if (!valid_object(&oper[2]))
        abort_interp("Invalid object. (1)");
    if (!valid_object(&oper[1]))
        abort_interp("Invalid object. (2)");
    if (oper[0].type != PROG_INTEGER)
        abort_interp("Non-integer argument (3)");
    if (oper[0].data.number < 0)
        abort_interp("Invalid argument. (3)");
    ref = oper[2].data.objref;
    ref2 = oper[1].data.objref;
    if (Typeof(ref) == TYPE_PLAYER) {
        result = DBFETCH(ref)->sp.player.pennies;
        if (Typeof(ref2) == TYPE_PLAYER) {
            result2 = DBFETCH(ref2)->sp.player.pennies;
            if (mlev < 4) {
                if (result < (result - oper[0].data.number))
                    abort_interp("Would roll over player's score. (1)");
                if ((result - oper[0].data.number) < 0)
                    abort_interp("Result would be negative. (1)");
                if (result2 > (result2 + oper[0].data.number))
                    abort_interp("Would roll over player's score. (2)");
                if ((result2 + oper[0].data.number) > tp_max_pennies)
                    abort_interp("Would exceed MAX_PENNIES. (2)");
            }
            result2 += oper[0].data.number;
            DBFETCH(ref)->sp.player.pennies += -(oper[0].data.number);
            DBFETCH(ref2)->sp.player.pennies += oper[0].data.number;
            DBDIRTY(ref);
            DBDIRTY(ref2);
        } else if (Typeof(ref2) == TYPE_THING) {
            if (mlev < 4)
                abort_interp(tp_noperm_mesg);
            result2 = DBFETCH(ref2)->sp.thing.value + oper[0].data.number;
            if (result < (result - oper[0].data.number))
                abort_interp("Would roll over player's score. (1)");
            if ((result - oper[0].data.number) < 0)
                abort_interp("Result would be negative. (1)");
            DBFETCH(ref)->sp.player.pennies += -(oper[0].data.number);
            DBFETCH(ref2)->sp.thing.value += oper[0].data.number;
            DBDIRTY(ref);
            DBDIRTY(ref2);
        } else {
            abort_interp("Invalid object type. (2)");
        }
    } else if (Typeof(ref) == TYPE_THING) {
        if (mlev < 4)
            abort_interp(tp_noperm_mesg);
        result = DBFETCH(ref)->sp.thing.value - oper[0].data.number;
        if (result < 1)
            abort_interp("Result must be positive. (1)");
        if (Typeof(ref2) == TYPE_PLAYER) {
            result2 = DBFETCH(ref2)->sp.player.pennies;
            if (result2 > (result2 + oper[0].data.number))
                abort_interp("Would roll over player's score. (2)");
            if ((result2 + oper[0].data.number) > tp_max_pennies)
                abort_interp("Would exceed MAX_PENNIES. (2)");
            DBFETCH(ref)->sp.thing.value += -(oper[0].data.number);
            DBFETCH(ref2)->sp.player.pennies += oper[0].data.number;
            DBDIRTY(ref);
            DBDIRTY(ref2);
        } else if (Typeof(ref2) == TYPE_THING) {
            DBFETCH(ref)->sp.thing.value += -(oper[0].data.number);
            DBFETCH(ref2)->sp.thing.value += oper[0].data.number;
            DBDIRTY(ref);
            DBDIRTY(ref2);
        } else {
            abort_interp("Invalid object type. (2)");
        }
    } else {
        abort_interp("Invalid object type. (1)");
    }
}

void
prim_instances(PRIM_PROTOTYPE)
{
    unsigned short a = 0;
    int b = 0;
	dbref ref;

    if (!valid_object(&oper[0]))
        abort_interp("Invalid object.");

    ref = oper[0].data.objref;
    if (Typeof(ref) != TYPE_PROGRAM)
        abort_interp("Object must be a program.");

    a = DBFETCH(ref)->sp.program.instances;
    b = a;
    PushInt(b);
}

void
prim_compiledp(PRIM_PROTOTYPE)
{
	dbref ref;

    if (!valid_object(&oper[0]))
        abort_interp("Invalid object.");

    ref = oper[0].data.objref;
    if (Typeof(ref) != TYPE_PROGRAM)
        abort_interp("Object must be a program.");
    
    PushInt(DBFETCH(ref)->sp.program.siz);
}


void
prim_setpassword(PRIM_PROTOTYPE)
{
    char *ptr, *ptr2;
    char pad_char[] = "";
	dbref ref;

    if (oper[0].type != PROG_STRING)
        abort_interp("Password string expected");
    if (oper[2].type != PROG_OBJECT)
        abort_interp("Player dbref expected");
    ref = oper[2].data.objref;
    if (ref != NOTHING && !valid_player(&oper[2]))
        abort_interp("Player dbref expected");
    CHECKREMOTE(ref);
    if (oper[1].type != PROG_STRING)
        abort_interp("Password string expected");
#ifdef MALLOC_PROFILING
    if (!oper[0].data.string)
        abort_interp
            ("NULL passwords cannot be set when MALLOC_PROFILING is turned on");
#endif
    ptr = oper[1].data.string ? oper[1].data.string->data : pad_char;
    ptr2 = oper[0].data.string ? oper[0].data.string->data : pad_char;
    if (ref != NOTHING && !check_password(ref, ptr))
        abort_interp("Incorrect password");
    set_password(ref, ptr2);
}

void
prim_newpassword(PRIM_PROTOTYPE)
{
    char *ptr2;
    char pad_char[] = "";
	dbref ref;

    if (oper[0].type != PROG_STRING)
        abort_interp("Password string expected");
    if (oper[2].type != PROG_OBJECT)
        abort_interp("Player dbref expected");
#ifdef MALLOC_PROFILING
    if (!oper[0].data.string)
        abort_interp
            ("NULL passwords cannot be set when MALLOC_PROFILING is turned on");  /* Why? -hinoserm */
#endif
    ptr2 = oper[0].data.string ? oper[0].data.string->data : pad_char;
    ref = oper[2].data.objref;

    if (!valid_player(&oper[2]))
        abort_interp("Player dbref expected");

    CHECKREMOTE(ref);
    if (MLevel(ref) >= mlev || (MLevel(ref) >= LMAGE && mlev < LBOY))
        abort_interp(tp_noperm_mesg);

    set_password(ref, ptr2);
}

void
prim_findnext(PRIM_PROTOTYPE)
{
    struct flgchkdat check;
    dbref who, item, ref, i;
    const char *name;
	char buf[BUFFER_LEN];

    if (oper[0].type != PROG_STRING)
        abort_interp("Expected string argument. (4)");
    if (oper[1].type != PROG_STRING)
        abort_interp("Expected string argument. (3)");
    if (oper[2].type != PROG_OBJECT)
        abort_interp("Expected dbref argument. (2)");
    if (oper[2].data.objref < NOTHING || oper[2].data.objref >= db_top)
        abort_interp("Bad object. (2)");
    if (oper[3].type != PROG_OBJECT)
        abort_interp("Expected dbref argument. (1)");
    if (oper[3].data.objref < NOTHING || oper[3].data.objref >= db_top)
        abort_interp("Bad object. (1)");
    if (oper[2].data.objref != NOTHING &&
        Typeof(oper[2].data.objref) == TYPE_GARBAGE)
        abort_interp("Owner dbref is garbage. (2)");

    item = oper[3].data.objref;
    who = oper[2].data.objref;
    name = DoNullInd(oper[1].data.string);

    if (mlev < 3) {
        if (who == NOTHING) {
            abort_interp
                ("Permission denied.  Owner inspecific searches require Mucker Level 3.");
        } else if (who != ProgUID) {
            abort_interp
                ("Permission denied.  Searching for other people's stuff requires Mucker Level 3.");
        }
    }

    if (item == NOTHING) {
        item = 0;
    } else {
        item++;
    }
    strcpy(buf, name);

    ref = NOTHING;
    init_checkflags(PSafe, DoNullInd(oper[0].data.string), &check);
    for (i = item; i < db_top; i++) {
        if ((who == NOTHING || OWNER(i) == who) &&
            checkflags(i, check) && NAME(i) &&
            (!*name || equalstr(buf, (char *) NAME(i)))) {
            ref = i;
            break;
        }
    }

    PushObject(ref);
}


void
prim_newprogram(PRIM_PROTOTYPE)
{
	dbref ref;

    if (oper[0].type != PROG_STRING)
        abort_interp("Expected string argument.");
    if (!tp_building || tp_db_readonly)
        abort_interp(NOBUILD_MESG);
    if (!oper[0].data.string)
        abort_interp("An empty string was passed.(2)");
    if (!ok_name(oper[0].data.string->data))
        abort_interp("Invalid name (2)");

    ref = new_program(PSafe, oper[0].data.string->data);
    
    PushObject(ref);
}

extern struct line *read_program(dbref prog);

void
prim_compile(PRIM_PROTOTYPE)
{
    dbref ref;
    struct line *tmpline;

    if (!valid_object(&oper[1]))
        abort_interp("No program dbref given.");
    ref = oper[1].data.objref;
    if (Typeof(ref) != TYPE_PROGRAM)
        abort_interp("No program dbref given.");
    if (oper[0].type != PROG_INTEGER)
        abort_interp("No boolean integer given.");

    tmpline = DBFETCH(ref)->sp.program.first;
    DBFETCH(ref)->sp.program.first = (struct line *) read_program(ref);
    do_compile(fr->descr, PSafe, ref, oper[0].data.number, fr);
    free_prog_text(DBFETCH(ref)->sp.program.first);
    DBFETCH(ref)->sp.program.first = tmpline;

    PushInt(DBFETCH(ref)->sp.program.siz);
}


void
prim_uncompile(PRIM_PROTOTYPE)
{
    dbref ref;

    if (!valid_object(&oper[0]))
        abort_interp("No program dbref given.");
    ref = oper[0].data.objref;
    if (Typeof(ref) != TYPE_PROGRAM)
        abort_interp("No program dbref given.");
    uncompile_program(ref);   
}

void
prim_contents_array(PRIM_PROTOTYPE)
{
    struct inst temp1, temp2;
    stk_array *nw;
    int count = 0;
	dbref ref;

    if (!valid_object(&oper[0]))
        abort_interp("Invalid dbref (1)");
    ref = oper[0].data.objref;
    
    if ((Typeof(ref) == TYPE_PROGRAM) || (Typeof(ref) == TYPE_EXIT))
        abort_interp("Dbref cannot be a program nor exit (1)");
    nw = new_array_packed(0);
    ref = DBFETCH(ref)->contents;
    while ((ref > 0) && (ref < db_top)) {
        temp1.type = PROG_INTEGER;
        temp1.data.number = count++;
        temp2.type = PROG_OBJECT;
        temp2.data.objref = ref;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        ref = DBFETCH(ref)->next;
    }
    PushArrayRaw(nw);
}

void
prim_exits_array(PRIM_PROTOTYPE)
{
    struct inst temp1, temp2;
    stk_array *nw;
    int count = 0;
	dbref ref;

    if (!valid_object(&oper[0]))
        abort_interp("Invalid dbref (1)");
    ref = oper[0].data.objref;
    
    if ((Typeof(ref) == TYPE_PROGRAM) || (Typeof(ref) == TYPE_EXIT))
        abort_interp("Dbref cannot be a program nor exit (1)");
    nw = new_array_packed(0);
    ref = DBFETCH(ref)->exits;
    while ((ref > 0) && (ref < db_top)) {
        temp1.type = PROG_INTEGER;
        temp1.data.number = count++;
        temp2.type = PROG_OBJECT;
        temp2.data.objref = ref;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        ref = DBFETCH(ref)->next;
    }
    PushArrayRaw(nw);
}

stk_array *
array_getlinks(dbref obj)
{
    struct inst temp1, temp2;
    stk_array *nw;
    int count = 0;

    nw = new_array_packed(0);
    if ((obj >= NOTHING) && (obj < db_top)) {
        switch (Typeof(obj)) {
            case TYPE_ROOM:{
                temp1.type = PROG_INTEGER;
                temp1.data.number = count++;
                temp2.type = PROG_OBJECT;
                temp2.data.objref = DBFETCH(obj)->sp.room.dropto;
                array_setitem(&nw, &temp1, &temp2);
                CLEAR(&temp1);
                CLEAR(&temp2);
                break;
            }
            case TYPE_THING:{
                temp1.type = PROG_INTEGER;
                temp1.data.number = count++;
                temp2.type = PROG_OBJECT;
                temp2.data.objref = DBFETCH(obj)->sp.thing.home;
                array_setitem(&nw, &temp1, &temp2);
                CLEAR(&temp1);
                CLEAR(&temp2);
                break;
            }
            case TYPE_PLAYER:{
                temp1.type = PROG_INTEGER;
                temp1.data.number = count++;
                temp2.type = PROG_OBJECT;
                temp2.data.objref = DBFETCH(obj)->sp.player.home;
                array_setitem(&nw, &temp1, &temp2);
                CLEAR(&temp1);
                CLEAR(&temp2);
                break;
            }
            case TYPE_EXIT:{
                for (count = 0; count < (DBFETCH(obj)->sp.exit.ndest); count++) {
                    temp1.type = PROG_INTEGER;
                    temp1.data.number = count;
                    temp2.type = PROG_OBJECT;
                    temp2.data.objref = (DBFETCH(obj)->sp.exit.dest)[count];
                    array_setitem(&nw, &temp1, &temp2);
                }
                CLEAR(&temp1);
                CLEAR(&temp2);
                break;
            }
        }
    }
    return nw;
}

void
prim_getlinks_array(PRIM_PROTOTYPE)
{
	dbref ref;

    if (!valid_object(&oper[0]))
        abort_interp("Invalid object dbref (1)");
    ref = oper[0].data.objref;

    PushArrayRaw(array_getlinks(ref));
}

void
prim_getobjinfo(PRIM_PROTOTYPE)
{
    char buf[BUFFER_LEN];
    double fresult;
    struct inst temp1, temp2;
    stk_array *nw;
	dbref ref;

    if (oper[0].type != PROG_OBJECT)
        abort_interp("Invalid object dbref (1)");
    if ((oper[0].data.objref < 0) || (oper[0].data.objref >= db_top))
        abort_interp("Invalid object dbref (1)");
    ref = oper[0].data.objref;

    nw = new_array_dictionary();
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("DBREF");
    temp2.type = PROG_OBJECT;
    temp2.data.objref = ref;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("NEXT");
    temp2.type = PROG_OBJECT;
    temp2.data.objref = DBFETCH(ref)->next;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("EXITS");
    temp2.type = PROG_OBJECT;
    temp2.data.objref = DBFETCH(ref)->exits;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("CONTENTS");
    temp2.type = PROG_OBJECT;
    temp2.data.objref = DBFETCH(ref)->contents;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("LOCATION");
    temp2.type = PROG_OBJECT;
    temp2.data.objref = DBFETCH(ref)->location;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("NAME");
    temp2.type = PROG_STRING;
    temp2.data.string = alloc_prog_string(NAME(ref));
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("CREATED");
    temp2.type = PROG_INTEGER;
    temp2.data.number = (int) DBFETCH(ref)->ts.created;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("MODIFIED");
    temp2.type = PROG_INTEGER;
    temp2.data.number = (int) DBFETCH(ref)->ts.modified;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("LASTUSED");
    temp2.type = PROG_INTEGER;
    temp2.data.number = (int) DBFETCH(ref)->ts.lastused;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("USECOUNT");
    temp2.type = PROG_INTEGER;
    temp2.data.number = DBFETCH(ref)->ts.usecount;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    switch (Typeof(ref)) {
        case TYPE_ROOM:{
            temp1.type = PROG_STRING;
            temp1.data.string = alloc_prog_string("DROPTO");
            temp2.type = PROG_OBJECT;
            temp2.data.objref = DBFETCH(ref)->sp.room.dropto;
            array_setitem(&nw, &temp1, &temp2);
            CLEAR(&temp1);
            CLEAR(&temp2);
            break;
        }
        case TYPE_THING:{
            temp1.type = PROG_STRING;
            temp1.data.string = alloc_prog_string("HOME");
            temp2.type = PROG_OBJECT;
            temp2.data.objref = DBFETCH(ref)->sp.thing.home;
            array_setitem(&nw, &temp1, &temp2);
            CLEAR(&temp1);
            CLEAR(&temp2);
            temp1.type = PROG_STRING;
            temp1.data.string = alloc_prog_string("VALUE");
            temp2.type = PROG_INTEGER;
            temp2.data.objref = DBFETCH(ref)->sp.thing.value;
            array_setitem(&nw, &temp1, &temp2);
            CLEAR(&temp1);
            CLEAR(&temp2);
            break;
        }
        case TYPE_EXIT:{
            temp1.type = PROG_STRING;
            temp1.data.string = alloc_prog_string("DEST");
            temp2.type = PROG_ARRAY;
            temp2.data.array = array_getlinks(ref);
            array_setitem(&nw, &temp1, &temp2);
            CLEAR(&temp1);
            CLEAR(&temp2);
            break;
        }
        case TYPE_PLAYER:{
            temp1.type = PROG_STRING;
            temp1.data.string = alloc_prog_string("HOME");
            temp2.type = PROG_OBJECT;
            temp2.data.objref = DBFETCH(ref)->sp.player.home;
            array_setitem(&nw, &temp1, &temp2);
            CLEAR(&temp1);
            CLEAR(&temp2);
            temp1.type = PROG_STRING;
            temp1.data.string = alloc_prog_string("PENNIES");
            temp2.type = PROG_INTEGER;
            temp2.data.number = DBFETCH(ref)->sp.player.pennies;
            array_setitem(&nw, &temp1, &temp2);
            CLEAR(&temp1);
            CLEAR(&temp2);
            temp1.type = PROG_STRING;
            temp1.data.string = alloc_prog_string("CURR_PROG");
            temp2.type = PROG_OBJECT;
            temp2.data.objref = DBFETCH(ref)->sp.player.curr_prog;
            array_setitem(&nw, &temp1, &temp2);
            CLEAR(&temp1);
            CLEAR(&temp2);
            temp1.type = PROG_STRING;
            temp1.data.string = alloc_prog_string("INSERT_MODE");
            temp2.type = PROG_INTEGER;
            temp2.data.number = (int) (DBFETCH(ref)->sp.player.insert_mode);
            array_setitem(&nw, &temp1, &temp2);
            CLEAR(&temp1);
            CLEAR(&temp2);
            temp1.type = PROG_STRING;
            temp1.data.string = alloc_prog_string("BLOCK");
            temp2.type = PROG_INTEGER;
            temp2.data.number = (int) (DBFETCH(ref)->sp.player.block);
            array_setitem(&nw, &temp1, &temp2);
            CLEAR(&temp1);
            CLEAR(&temp2);
            break;
        }
        case TYPE_PROGRAM:{
            temp1.type = PROG_STRING;
            temp1.data.string = alloc_prog_string("INSTANCES");
            temp2.type = PROG_INTEGER;
            temp2.data.number = (int) (DBFETCH(ref)->sp.program.instances);
            array_setitem(&nw, &temp1, &temp2);
            CLEAR(&temp1);
            CLEAR(&temp2);
            temp1.type = PROG_STRING;
            temp1.data.string = alloc_prog_string("SIZ");
            temp2.type = PROG_INTEGER;
            temp2.data.number = (int) (DBFETCH(ref)->sp.program.siz);
            array_setitem(&nw, &temp1, &temp2);
            CLEAR(&temp1);
            CLEAR(&temp2);
            temp1.type = PROG_STRING;
            temp1.data.string = alloc_prog_string("PROFSTART");
            temp2.type = PROG_INTEGER;
            temp2.data.number = (int) (DBFETCH(ref)->sp.program.profstart);
            array_setitem(&nw, &temp1, &temp2);
            CLEAR(&temp1);
            CLEAR(&temp2);
            temp1.type = PROG_STRING;
            temp1.data.string = alloc_prog_string("PROFUSES");
            temp2.type = PROG_INTEGER;
            temp2.data.number = (int) (DBFETCH(ref)->sp.program.profuses);
            array_setitem(&nw, &temp1, &temp2);
            CLEAR(&temp1);
            CLEAR(&temp2);
            temp1.type = PROG_STRING;
            temp1.data.string = alloc_prog_string("PROFTIME");
            temp2.type = PROG_FLOAT;
            sprintf(buf, "%ld.%06ld",
                    (long) DBFETCH(ref)->sp.program.proftime.tv_sec,
                    (long) DBFETCH(ref)->sp.program.proftime.tv_usec);
            fresult = atof(buf);
            temp2.data.fnumber = fresult;
            array_setitem(&nw, &temp1, &temp2);
            CLEAR(&temp1);
            CLEAR(&temp2);
            break;
        }
    }
    
    PushArrayRaw(nw);
}

void
prim_find_array(PRIM_PROTOTYPE)
{
    struct flgchkdat check;
    dbref ref, who;
    const char *name;
    stk_array *nw;
	char buf[BUFFER_LEN];
	int i = 0;
	struct inst temp;

    if (oper[0].type != PROG_STRING)
        abort_interp("Expected string argument. (3)");
    if (oper[1].type != PROG_STRING)
        abort_interp("Expected string argument. (2)");
    if (oper[2].type != PROG_OBJECT)
        abort_interp("Expected dbref argument. (1)");
    if (oper[2].data.objref < NOTHING || oper[2].data.objref >= db_top)
        abort_interp("Bad object. (1)");

    who = oper[2].data.objref;
    name = DoNullInd(oper[1].data.string);

    strcpy(buf, name);
    init_checkflags(PSafe, DoNullInd(oper[0].data.string), &check);
    nw = new_array_packed(db_top);

    for (ref = (dbref) 0; ref < db_top; ref++) {
        if (((who == NOTHING) ? 1 : (OWNER(ref) == who)) &&
            checkflags(ref, check) && NAME(ref) &&
            (!*name || equalstr(buf, (char *) NAME(ref))))
		{
			temp.type = PROG_OBJECT;
			temp.data.objref = ref;

            array_set_intkey(&nw, i++, &temp);
        }
    }

	array_shrink(nw, i);

    PushArrayRaw(nw);
}

void
prim_entrances_array(PRIM_PROTOTYPE)
{
    stk_array *nw;
    dbref i, j, ref;

    if (!valid_object(&oper[0]))
        abort_interp("Invalid dbref (1)");

    ref = oper[0].data.objref;
    nw = new_array_packed(0);

    for (i = 0; i < db_top; i++) {
        switch (Typeof(i)) {
            case TYPE_EXIT:
                for (j = DBFETCH(i)->sp.exit.ndest; j--;) {
                    if (DBFETCH(i)->sp.exit.dest[j] == ref)
                        array_appendref(&nw, i);
                }
                break;
            case TYPE_PLAYER:
                if (DBFETCH(i)->sp.player.home == ref)
                    array_appendref(&nw, i);
                break;
            case TYPE_THING:
                if (DBFETCH(i)->sp.thing.home == ref)
                    array_appendref(&nw, i);
                break;
            case TYPE_ROOM:
                if (DBFETCH(i)->sp.room.dropto == ref)
                    array_appendref(&nw, i);
                break;
            case TYPE_PROGRAM:
            case TYPE_GARBAGE:
                break;
        }
    }
    
    PushArrayRaw(nw);
}

void
prim_lflags_update(PRIM_PROTOTYPE)
{
    lflags_update();
}

