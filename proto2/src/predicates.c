#include "copyright.h"
#include "config.h"

/* Predicates for testing various conditions */

#include <ctype.h>

#include "db.h"
#include "props.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "externs.h"
#include "reg.h"

bool
can_link_to(register dbref who, object_flag_type what_type,
            register dbref where)
{
    if (where == HOME)
        return 1;

    if (where == NIL)
        return 1;

    if (!OkObj(who) || !OkObj(where))
        return 0;

    switch (what_type) {
        case TYPE_EXIT:
            return (controls(who, where) || (FLAGS(where) & LINK_OK)
                    || (POWERS(who) & POW_LINK_ANYWHERE));
            /* NOTREACHED */
            break;
        case TYPE_PLAYER:
            return (Typeof(where) == TYPE_ROOM && (controls(who, where)
                                                   || Linkable(where)
                                                   || (POWERS(who) &
                                                       POW_LINK_ANYWHERE)));
            /* NOTREACHED */
            break;
        case TYPE_ROOM:
            return ((Typeof(where) == TYPE_ROOM || Typeof(where) == TYPE_THING)
                    && (controls(who, where) || Linkable(where)
                        || (POWERS(who) & POW_LINK_ANYWHERE)));
            /* NOTREACHED */
            break;
        case TYPE_THING:
            return ((Typeof(where) == TYPE_ROOM || Typeof(where) == TYPE_PLAYER
                     || Typeof(where) == TYPE_THING)
                    && (controls(who, where) || Linkable(where)
                        || (POWERS(who) & POW_LINK_ANYWHERE)));
            /* NOTREACHED */
            break;
        case NOTYPE:
            return (controls(who, where) || (FLAGS(where) & LINK_OK)
                    || (POWERS(who) & POW_LINK_ANYWHERE)
                    || (Typeof(where) != TYPE_THING && (FLAGS(where) & ABODE)));
            /* NOTREACHED */
            break;
    }

    return 0;
}

bool
can_link(register dbref who, register dbref what)
{
    return (controls(who, what) || ((Typeof(what) == TYPE_EXIT)
                                    && DBFETCH(what)->sp.exit.ndest == 0));
}

/*
 * Revision 1.2 -- SECURE_TELEPORT
 * you can only jump with an action from rooms that you own
 * or that are jump_ok, and you cannot jump to players that are !jump_ok.
 */

bool
could_doit(int descr, register dbref player, register dbref thing)
{
    register dbref source, dest, owner;

    if (Typeof(thing) == TYPE_EXIT) {
        if (DBFETCH(thing)->sp.exit.ndest == 0) {
            return 0;
        }

        owner = OWNER(thing);
        source = DBFETCH(player)->location;
        dest = *(DBFETCH(thing)->sp.exit.dest);

        if (dest == NIL)        /* unless its locked, anyone can use #-4 */
            return (eval_boolexp(descr, player, GETLOCK(thing), thing));

        if (Typeof(dest) == TYPE_PLAYER) {
            dbref destplayer = dest;

            dest = DBFETCH(dest)->location;
            if (!(FLAGS(destplayer) & JUMP_OK) || (FLAGS(dest) & BUILDER)) {
                return 0;
            }
        }
/*
    if (OkObj(thing)) if
       ( ((FLAG2(player) & F2IMMOBILE) && !(FLAG2(thing) & F2IMMOBILE)) && 
         (!(Typeof(dest) == TYPE_PROGRAM) && !(dest == NIL))
       ) {
          envpropqueue(descr, player, OkObj(player) ? getloc(player) : -1,
	              thing, thing, NOTHING, "@immobile", "Immobile", 1, 1);
    	  return 0;
         }

*/
/*        if( (dest != HOME) &&
            (Typeof(dest)==TYPE_ROOM) &&
            (FLAGS(player) & ZOMBIE) && (Typeof(player) == TYPE_THING) &&
            (FLAGS(dest) & ZOMBIE) ) 
            return 0;
 Saving this part for a revision of trigger() and this function. */
        if ((dest != HOME) && (Typeof(dest) == TYPE_ROOM) && Guest(player)
            && (tp_guest_needflag ? !(FLAG2(dest) & F2GUEST)
                : (FLAG2(dest) & F2GUEST))) {
/*	    anotify_nolisten(player, CFAIL "Guests aren't allowed there.", 1); */
            return 0;
        }

        /* for actions */
        if ((DBFETCH(thing)->location != NOTHING) &&
            (Typeof(DBFETCH(thing)->location) != TYPE_ROOM)) {

            if ((Typeof(dest) == TYPE_ROOM || Typeof(dest) == TYPE_PLAYER) &&
                (FLAGS(source) & BUILDER))
                return 0;

            if (tp_secure_teleport && Typeof(dest) == TYPE_ROOM) {
                if ((dest != HOME) && (!controls(owner, source))
                    && ((FLAGS(source) & JUMP_OK) == 0)) {
                    return 0;
                }
            }
        }
    }


    return (eval_boolexp(descr, player, GETLOCK(thing), thing));
}


bool
could_doit2(int descr, register dbref player, register dbref thing, char *prop,
            register bool tryprog)
{
    register dbref source, dest, owner;

    if (Typeof(thing) == TYPE_EXIT) {
        if (DBFETCH(thing)->sp.exit.ndest == 0) {
            return 0;
        }

        owner = OWNER(thing);
        source = DBFETCH(player)->location;
        dest = *(DBFETCH(thing)->sp.exit.dest);

        if (dest == NIL)        /* unless its locked, anyone can use #-4 */
            return (eval_boolexp(descr, player, GETLOCK(thing), thing));

        if (Typeof(dest) == TYPE_PLAYER) {
            dbref destplayer = dest;

            dest = DBFETCH(dest)->location;
            if (!(FLAGS(destplayer) & JUMP_OK) || (FLAGS(dest) & BUILDER)) {
                return 0;
            }
        }
/*
    if (OkObj(thing)) if
       ( ((FLAG2(player) & F2IMMOBILE) && !(FLAG2(thing) & F2IMMOBILE)) && 
         (!(Typeof(dest) == TYPE_PROGRAM) && !(dest == NIL))
       ) {
          envpropqueue(descr, player, OkObj(player) ? getloc(player) : -1,
	              thing, thing, NOTHING, "@immobile", "Immobile", 1, 1);
    	  return 0;
         }
*/
        if ((dest != HOME) && (Typeof(dest) == TYPE_ROOM) && Guest(player)
            && (tp_guest_needflag ? !(FLAG2(dest) & F2GUEST)
                : (FLAG2(dest) & F2GUEST))) {
            anotify_nolisten(player, CFAIL "Guests aren't allowed there.", 1);
            return 0;
        }

        /* for actions */
        if ((DBFETCH(thing)->location != NOTHING) &&
            (Typeof(DBFETCH(thing)->location) != TYPE_ROOM)) {

            if ((Typeof(dest) == TYPE_ROOM || Typeof(dest) == TYPE_PLAYER) &&
                (FLAGS(source) & BUILDER))
                return 0;

            if (tp_secure_teleport && Typeof(dest) == TYPE_ROOM) {
                if ((dest != HOME) && (!controls(owner, source))
                    && ((FLAGS(source) & JUMP_OK) == 0)) {
                    return 0;
                }
            }
        }


    }

    if (tryprog)
        return (eval_boolexp(descr, player,
                             get_property_lock(thing, prop), thing));
    else
        return (eval_boolexp2(descr, player,
                              get_property_lock(thing, prop), thing));

}


bool
test_lock(register int descr, register dbref player, register dbref thing,
          register const char *lockprop)
{
    register struct boolexp *lokptr;

    lokptr = get_property_lock(thing, lockprop);
    return (eval_boolexp(descr, player, lokptr, thing));
}


bool
test_lock_false_default(register int descr, register dbref player,
                        register dbref thing, register const char *lockprop)
{
    register struct boolexp *lok = get_property_lock(thing, lockprop);

    if (lok == TRUE_BOOLEXP)
        return 0;

    return (eval_boolexp(descr, player, lok, thing));
}


bool
can_doit(int descr, register dbref player, register dbref thing,
         const char *default_fail_msg)
{
    register dbref loc;

    if ((loc = getloc(player)) == NOTHING)
        return 0;

    if (OkObj(thing)) {
        dbref dest = Typeof(thing) == TYPE_EXIT ? (DBFETCH(thing)->sp.exit.ndest ? DBFETCH(thing)->sp.exit.dest[0] : NOTHING) : NOTHING;

        if (((FLAG2(player) & F2IMMOBILE) && !(FLAG2(thing) & F2IMMOBILE)) &&
            (!OkObj(dest) || Typeof(dest) != TYPE_PROGRAM)
            ) {
            envpropqueue(descr, player, OkObj(player) ? getloc(player) : -1,
                         thing, thing, NOTHING, "@immobile", "Immobile", 1, 1);
            return 0;
        }
    }
    if (!TMage(OWNER(player)) && Typeof(player) == TYPE_THING &&
        (FLAGS(thing) & ZOMBIE)) {
        notify(player, "Sorry, but zombies can't do that.");
        return 0;
    }
    if (!could_doit(descr, player, thing)) {
        /* can't do it */
        if (GETFAIL(thing)) {
            exec_or_notify(descr, player, thing, GETFAIL(thing), "(@Fail)");
        } else if (default_fail_msg) {
            notify(player, default_fail_msg);
        }
        if (GETOFAIL(thing) && !Dark(player)) {
            parse_omessage(descr, player, loc, thing, GETOFAIL(thing),
                           PNAME(player), "(@Ofail)");
        }
        return 0;
    } else {
        /* can do it */
/* I moved these to the 'trigger()' function. -Akari */
/*	if (GETSUCC(thing)) {
	    exec_or_notify(descr, player, thing, GETSUCC(thing), "(@Succ)");
	}
	if (GETOSUCC(thing) && !Dark(player)) {
	    parse_omessage(descr, player, loc, thing, GETOSUCC(thing),
			    NAME(player), "(@Osucc)");
	}
 */
        return 1;
    }
}

bool
can_see(register dbref player, register dbref thing, register bool can_see_loc)
{
    if (!OkObj(player) || !OkObj(thing))
        return 0;

    if (player == thing || Typeof(thing) == TYPE_EXIT
        || Typeof(thing) == TYPE_ROOM)
        return 0;

    if (Light(thing))
        return 1;

    if (can_see_loc) {
        switch (Typeof(thing)) {
            case TYPE_PROGRAM:
                return ((FLAGS(thing) & LINK_OK) || controls(player, thing)
                        || (POWERS(player) & POW_SEE_ALL));
            case TYPE_PLAYER:
                if (tp_dark_sleepers) {
                    return (!Dark(thing) || online(thing)
                            || (POWERS(player) & POW_SEE_ALL));
                }
            default:
                return (!Dark(thing) || (POWERS(player) & POW_SEE_ALL) ||
                        (controls(player, thing) && !(FLAGS(player) & STICKY)));

        }
    } else {
        /* can't see loc */
        return (controls(player, thing) && !(FLAGS(player) & STICKY));
    }
}

#ifdef CONTROLS_SUPPORT
#   define controlsEx(W, H) ((FLAG2(H) & F2CONTROLS) && \
    test_lock_false_default(-1, OWNER(W), H, "_/chlk"))
#endif

bool
newcontrols(register dbref who, register dbref what, register bool true_c)
{
    register dbref index;

    /* No one controls invalid objects */
    /* if (what < 0 || what >= db_top)  -- not good enough */
    if (!OkObj(what) || !OkObj(who))
        return 0;

    /* Garbage controls nothing. */
    if (Typeof(who) == TYPE_GARBAGE)
        return 0;

    /* No one controls garbage */
    if (Typeof(what) == TYPE_GARBAGE)
        return 0;

    /* Puppets are based on owner */
    if (Typeof(who) != TYPE_PLAYER)
        who = OWNER(who);

    /* owners control their own stuff */
    /* Makes stuff faster here. -Hinoserm */
    if (who == OWNER(what))
        return 1;

    /* CONTROL_ALL controls all objects */
    if ((POWERS(who) & POW_CONTROL_ALL) && !Protect(what))
        return 1;

    /* CONTROL_MUF power controls all MUF objects */
    if ((POWERS(who) & POW_CONTROL_MUF) && (Typeof(what) == TYPE_PROGRAM)
        && (!(Protect(what))))
        return 1;

    /* Wizard controls (most) everything else */
    if (Wiz(who) && (!(Protect(what) && MLevel(OWNER(what)) >= LBOY)
                     || MLevel(who) >= LBOY))
        if (tp_fb_controls ? (MLevel(who) >= LWIZ)
            : (MLevel(who) >= MLevel(OWNER(what))))
            return 1;

    /* If realms control is enabled, the player will Control anything 
     * contained in a parent room he owns with at W1 bit or higher on it.
     * This gives him power to affect any object in his w-bitted parent room.
     */

    /* 
     *  Read the manual (help CONTROLS) about this new stuff.
     */

#ifdef CONTROLS_SUPPORT
    if (!true_c) {
        if (controlsEx(who, what))
            return 1;

        if (tp_realms_control) {
            if (!tp_wiz_realms) {
                if (Typeof(what) != TYPE_PLAYER)
                    for (index = what; index != NOTHING; index = getloc(index))
                        if ((controlsEx(who, index))
                            && (Typeof(index) == TYPE_ROOM
                                && ((FLAGS(index) & BUILDER) || Mage(index))))
                            return 1;
            } else {
                if (Typeof(what) != TYPE_PLAYER)
                    for (index = what; index != NOTHING; index = getloc(index))
                        if ((controlsEx(who, index))
                            && (Typeof(index) == TYPE_ROOM && (Mage(index))))
                            return 1;
            }
        }
    } else {
#endif

        if (tp_realms_control && (Typeof(what) != TYPE_PLAYER))
            for (index = what; index != NOTHING; index = getloc(index))
                if ((OWNER(index) == who) && (Typeof(index) == TYPE_ROOM
                                              && Mage(index)))
                    return 1;
#ifdef CONTROLS_SUPPORT
    }
#endif

    return 0;
}

/* Indicates if a flag can or cannot be set.
 * Returns 1 if the flag can't be set, 0 if it can.
 * Note that this function only handles flagset 1. restricted2 for flagset2
 */
bool
restricted(register dbref player, register dbref thing, object_flag_type flag)
{
    switch (flag) {
        case ABODE:
            return (!Mage(OWNER(player)) && (Typeof(thing) == TYPE_PROGRAM));
            break;
        case ZOMBIE:
            if (tp_wiz_puppets)
                if (Typeof(thing) == TYPE_THING)
                    return (!Mage(OWNER(player)));
            if (Typeof(thing) == TYPE_PLAYER)
                return (!Mage(OWNER(player)));
            if ((Typeof(thing) == TYPE_THING)
                && (FLAGS(OWNER(player)) & ZOMBIE))
                return (!Mage(OWNER(player)));
            return (0);
        case VEHICLE:
            if (Typeof(thing) == TYPE_PLAYER)
                return (!Mage(OWNER(player)));
            if (tp_wiz_vehicles) {
                if (Typeof(thing) == TYPE_THING)
                    return (!Mage(OWNER(player)));
            } else {
                if ((Typeof(thing) == TYPE_THING) && (FLAGS(player) & VEHICLE))
                    return (!Mage(OWNER(player)));
            }
            return (0);
        case DARK:
            if (!Arch(OWNER(player)) && !(POWERS(player) & POW_HIDE)) {
                if (Typeof(thing) == TYPE_PLAYER)
                    return (1);
                if (!tp_exit_darking && Typeof(thing) == TYPE_EXIT)
                    return (1);
                if (!tp_thing_darking && Typeof(thing) == TYPE_THING)
                    return (1);
            }
            return (0);
            break;
        case QUELL:
            return (TMage(thing) && (thing != player) &&
                    (Typeof(thing) == TYPE_PLAYER));
            break;
        case BUILDER:
            if ((Typeof(thing) == TYPE_PLAYER)
                || (Typeof(thing) == TYPE_PROGRAM))
                return (!Mage(OWNER(player)));
            else
                return (!truecontrols(player, thing));
            break;
        case CHOWN_OK:
            if (Typeof(thing) == TYPE_PLAYER)
                return !((OWNER(thing) == player) || Mage(player));
            else
                return !truecontrols(player, thing);
            break;
        case W1:               /* We use @set to make our own rules for these */
        case W2:
        case W3:
        case W4:
            return 1;
            break;
        default:
            return 0;
            break;
    }
}

/* determines if a player can set a flag based on permission level 
 * 0 indicates they can, 1 indicates they cannot. Only checks flagset2.
 */
bool
restricted2(register dbref player, register dbref thing, object_flag_type flag)
{
    switch (flag) {
        case F2GUEST:
            return (!Mage(OWNER(player)));
            break;
        case F2LOGWALL:
            return (!Arch(OWNER(player)));
        case F2HIDDEN:         /* can only be set on players currently */
            if (Typeof(thing) == TYPE_PLAYER)
                return (!Arch(OWNER(player)) && !(POWERS(player) & POW_HIDE));
            else
                return 1;
        case F2ANTIPROTECT:
            if (Typeof(thing) == TYPE_PLAYER)
                return (!Boy(OWNER(player)));
            else
                return 1;
#ifdef CONTROLS_SUPPORT
        case F2CONTROLS:
            /* only by W4.  Not even by things they own. */
            if (Typeof(thing) == TYPE_PLAYER)
                return (!Boy(player));
            if (Typeof(thing) == TYPE_PROGRAM)
                return (!Wizard(player));
            /* anything else, if I truly control it. */
            return !truecontrols(player, thing);
            break;
#endif
        case F2MOBILE:
            return !(MLevel(OWNER(player)) >= tp_userflag_mlev);
            break;
        default:
            return 0;
            break;
    }
}

/* Given a dbref and a cost, subtracts the cost from the player's
 * pennies and returns 1. Returns 0 if the player could not afford
 * it.
 */
bool
payfor(register dbref who, register int cost)
{
    if (who == NOTHING)
        return 1;

    who = OWNER(who);
    if (Mage(who) || (POWERS(who) & POW_NO_PAY)) {
        return 1;

    } else if (DBFETCH(who)->sp.player.pennies >= cost) {
        DBFETCH(who)->sp.player.pennies -= cost;
        DBDIRTY(who);
        return 1;
    } else
        return 0;
}

bool
word_start(register const char *str, register const char let)
{
    register bool chk;

    for (chk = 1; *str; str++) {
        if (chk && *str == let)
            return 1;

        chk = (*str == ' ');
    }

    return 0;
}

bool
ok_name(register const char *name)
{
    return (name
            && *name
            && *name != LOOKUP_TOKEN
            && *name != REGISTERED_TOKEN
            && *name != NUMBER_TOKEN && !index(name, ARG_DELIMITER)
/*	    && !index(name, AND_TOKEN) */
            && !index(name, OR_TOKEN)
            && !index(name, '^')
            && !index(name, '\r')
            && !index(name, '\n')
            && !word_start(name, NOT_TOKEN)
            && string_compare(name, "me")
/*	    && string_compare(name, "home") */
            && string_compare(name, "here"));
}

bool
ok_player_name(register const char *name)
{
    register const char *scan;

    if (!ok_name(name) || strlen(name) > PLAYER_NAME_LIMIT)
        return 0;

    for (scan = name; *scan; scan++) {
        if (!(isprint(*scan) && !isspace(*scan))) /* was isgraph(*scan) */
            return 0;
    }

    if (name_is_bad(name))
        return 0;

    /* lookup name to avoid conflicts */
    return (lookup_player(name) == NOTHING);
}

bool
ok_password(register const char *password)
{
    register const char *scan;

    if (*password == '\0')
        return 0;

    for (scan = password; *scan; scan++) {
        if (*scan == '=' || !(isprint(*scan) && !isspace(*scan)))
            return 0;
    }

    return 1;
}
