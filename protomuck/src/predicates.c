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

int 
can_link_to(dbref who, object_flag_type what_type, dbref where)
{
    if (where == HOME)
	return 1;
    if (where < 0 || where > db_top)
	return 0;
    switch (what_type) {
	case TYPE_EXIT:
	    return (controls(who, where) || (FLAGS(where) & LINK_OK) || (POWERS(who) & POW_LINK_ANYWHERE));
	    /* NOTREACHED */
	    break;
	case TYPE_PLAYER:
	    return (Typeof(where) == TYPE_ROOM && (controls(who, where)
						   || Linkable(where) || (POWERS(who) & POW_LINK_ANYWHERE)));
	    /* NOTREACHED */
	    break;
	case TYPE_ROOM:
            return ((Typeof(where) == TYPE_ROOM || Typeof(where) == TYPE_THING)
		    && (controls(who, where) || Linkable(where) || (POWERS(who) & POW_LINK_ANYWHERE)));
	    /* NOTREACHED */
	    break;
	case TYPE_THING:
            return ((Typeof(where) == TYPE_ROOM || Typeof(where) == TYPE_PLAYER || Typeof(where) == TYPE_THING)
		    && (controls(who, where) || Linkable(where) || (POWERS(who) & POW_LINK_ANYWHERE)));
	    /* NOTREACHED */
	    break;
	case NOTYPE:
	    return (controls(who, where) || (FLAGS(where) & LINK_OK) || (POWERS(who) & POW_LINK_ANYWHERE) ||
		    (Typeof(where) != TYPE_THING && (FLAGS(where) & ABODE)));
	    /* NOTREACHED */
	    break;
    }
    return 0;
}

int 
can_link(dbref who, dbref what)
{
    return (controls(who, what) || ((Typeof(what) == TYPE_EXIT)
				    && DBFETCH(what)->sp.exit.ndest == 0));
}

/*
 * Revision 1.2 -- SECURE_TELEPORT
 * you can only jump with an action from rooms that you own
 * or that are jump_ok, and you cannot jump to players that are !jump_ok.
 */

int 
could_doit(int descr, dbref player, dbref thing)
{
    dbref   source, dest, owner;

    if (Typeof(thing) == TYPE_EXIT) {
	if (DBFETCH(thing)->sp.exit.ndest == 0) {
	    return 0;
	}

	owner = OWNER(thing);
	source = DBFETCH(player)->location;
	dest = *(DBFETCH(thing)->sp.exit.dest);

	if (Typeof(dest) == TYPE_PLAYER) {
	    dbref destplayer = dest;
	    dest = DBFETCH(dest)->location;
	    if (!(FLAGS(destplayer) & JUMP_OK) || (FLAGS(dest) & BUILDER)) {
		return 0;
	    }
	}

	if( (dest != HOME) &&  
	    (Typeof(dest)==TYPE_ROOM) &&
	    Guest(player) &&
#ifdef G_NEEDFLAG
	    !(FLAG2(dest)&F2GUEST)
#else
	    (FLAG2(dest)&F2GUEST)
#endif
	) {
	    notify(player, "Guests aren't allowed there.");
	    return 0;
	}

	/* for actions */
	if ((DBFETCH(thing)->location != NOTHING) &&
		(Typeof(DBFETCH(thing)->location) != TYPE_ROOM))
	{

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


int 
could_doit2(int descr, dbref player, dbref thing, char *prop)
{
    dbref   source, dest, owner;

    if (Typeof(thing) == TYPE_EXIT) {
	if (DBFETCH(thing)->sp.exit.ndest == 0) {
	    return 0;
	}

	owner = OWNER(thing);
	source = DBFETCH(player)->location;
	dest = *(DBFETCH(thing)->sp.exit.dest);

	if (Typeof(dest) == TYPE_PLAYER) {
	    dbref destplayer = dest;
	    dest = DBFETCH(dest)->location;
	    if (!(FLAGS(destplayer) & JUMP_OK) || (FLAGS(dest) & BUILDER)) {
		return 0;
	    }
	}

	if( (dest != HOME) &&  
	    (Typeof(dest)==TYPE_ROOM) &&
	    Guest(player) &&
#ifdef G_NEEDFLAG
	    !(FLAG2(dest)&F2GUEST)
#else
	    (FLAG2(dest)&F2GUEST)
#endif
	) {
	    notify(player, "Guests aren't allowed there.");
	    return 0;
	}

	/* for actions */
	if ((DBFETCH(thing)->location != NOTHING) &&
		(Typeof(DBFETCH(thing)->location) != TYPE_ROOM))
	{

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
    return (eval_boolexp2(descr, player, get_property_lock(thing, prop), thing));
}


int
test_lock(int descr, dbref player, dbref thing, const char *lockprop)
{
    struct boolexp *lokptr;

    lokptr = get_property_lock(thing, lockprop);
    return (eval_boolexp(descr, player, lokptr, thing));
}


int
test_lock_false_default(int descr, dbref player, dbref thing, const char *lockprop)
{
    struct boolexp *lok = get_property_lock(thing, lockprop);

    if (lok == TRUE_BOOLEXP) return 0;
    return (eval_boolexp(descr, player, lok, thing));
}


int 
can_doit(int descr, dbref player, dbref thing, const char *default_fail_msg)
{
    dbref   loc;

    if ((loc = getloc(player)) == NOTHING)
	return 0;

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
	if (GETSUCC(thing)) {
	    exec_or_notify(descr, player, thing, GETSUCC(thing), "(@Succ)");
	}
	if (GETOSUCC(thing) && !Dark(player)) {
	    parse_omessage(descr, player, loc, thing, GETOSUCC(thing),
			    NAME(player), "(@Osucc)");
	}
	return 1;
    }
}

int 
can_see(dbref player, dbref thing, int can_see_loc)
{
    if (player == thing || Typeof(thing) == TYPE_EXIT
	    || Typeof(thing) == TYPE_ROOM)
	return 0;

    if (Light(thing)) return 1;

    if (can_see_loc) {
	switch (Typeof(thing)) {
	    case TYPE_PROGRAM:
		return ((FLAGS(thing) & LINK_OK) || controls(player, thing) || (POWERS(player) & POW_SEE_ALL));
            case TYPE_PLAYER:
		if (tp_dark_sleepers) {
notify(1, "seeing dark players.");
		    return (!Dark(thing) || online(thing) || (POWERS(player) & POW_SEE_ALL));
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

int 
controls(dbref who, dbref what)
{
    /* No one controls invalid objects */
    if (what < 0 || what >= db_top)
	return 0;

    /* No one controls garbage */
    if (Typeof(what) == TYPE_GARBAGE)
	return 0;

    if (Typeof(who) != TYPE_PLAYER)
	who = OWNER(who);

    /* Wizard controls (most) everything else */
    if (Wiz(who) && (!(Protect(what) && MLevel(OWNER(what)) >= LBOY) || MLevel(who) >= LBOY))
#ifdef FB_CONTROLS
        if(MLevel(who) >= LWIZ)
#else
	if(MLevel(who) >= MLevel(OWNER(what)))
#endif
	    return 1;

    /* owners control their own stuff */
    return (who == OWNER(what));
}

int 
restricted(dbref player, dbref thing, object_flag_type flag)
{
    switch (flag) {
	case ABODE:
	    return (!Mage(OWNER(player)) &&
		    (Typeof(thing) == TYPE_PROGRAM));
	    break;
	case ZOMBIE:
	    if (Typeof(thing) == TYPE_PLAYER)
		return(!Mage(OWNER(player)));
	    if ((Typeof(thing) == TYPE_THING) &&
		    (FLAGS(OWNER(player)) & ZOMBIE))
		return(!Mage(OWNER(player)));
	    return(0);
	case VEHICLE:
	    if (Typeof(thing) == TYPE_PLAYER)
		return(!Mage(OWNER(player)));
	    if (tp_wiz_vehicles) {
		if (Typeof(thing) == TYPE_THING)
		    return(!Mage(OWNER(player)));
	    } else {
		if ((Typeof(thing) == TYPE_THING) && (FLAGS(player) & VEHICLE))
		    return(!Mage(OWNER(player)));
	    }
	    return(0);
	case DARK:
            if (!Arch(OWNER(player)) && !(POWERS(player) & POW_HIDE)) {
		if (Typeof(thing) == TYPE_PLAYER)
		    return(1);
		if (!tp_exit_darking && Typeof(thing) == TYPE_EXIT)
		    return(1);
		if (!tp_thing_darking && Typeof(thing) == TYPE_THING)
		    return(1);
	    }
            return(0);
	    break;
	case QUELL:
	    return (TMage(thing) && (thing != player) &&
		    (Typeof(thing) == TYPE_PLAYER));
	    break;
	case BUILDER:
	    return (!Mage(OWNER(player)));
	    break;
	case W1:	/* We use @set to make our own rules for these */
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


int 
restricted2(dbref player, dbref thing, object_flag_type flag)
{
    switch (flag) {
	case F2GUEST:
	    return (!Mage(OWNER(player)));
	    break;
	case F2LOGWALL:
	    return (!Arch(OWNER(player)));
	case F2HIDDEN:
	    if (Typeof(thing) == TYPE_PLAYER) { return (!Arch(OWNER(player)) && !(POWERS(player) & POW_HIDE)); } else { return 1; }
      case F2ANTIPROTECT:
          if (Typeof(thing) == TYPE_PLAYER) { return (!Boy(OWNER(player))); } else { return 1; }

	case F2MUFCOUNT:
	default:
	    return 0;
	    break;
    }
}


int 
payfor(dbref who, int cost)
{
    who = OWNER(who);
    if (Mage(who) || (POWERS(who) & POW_NO_PAY)) {
	return 1;
    } else if (DBFETCH(who)->sp.player.pennies >= cost) {
	DBFETCH(who)->sp.player.pennies -= cost;
	DBDIRTY(who);
	return 1;
    } else {
	return 0;
    }
}

int 
word_start(const char *str, const char let)
{
    int     chk;

    for (chk = 1; *str; str++) {
	if (chk && *str == let)
	    return 1;
	chk = *str == ' ';
    }
    return 0;
}

int 
ok_name(const char *name)
{
    return (name
	    && *name
	    && *name != LOOKUP_TOKEN
	    && *name != REGISTERED_TOKEN
	    && *name != NUMBER_TOKEN
	    && !index(name, ARG_DELIMITER)
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

int 
ok_player_name(const char *name)
{
    const char *scan;

    if (!ok_name(name) || strlen(name) > PLAYER_NAME_LIMIT)
	return 0;

    for (scan = name; *scan; scan++) {
	if (!(isprint(*scan) && !isspace(*scan))) {	/* was isgraph(*scan) */
	    return 0;
	}
    }

   if(name_is_bad(name)) return 0;

    /* lookup name to avoid conflicts */
    return (lookup_player(name) == NOTHING);
}

int 
ok_password(const char *password)
{
    const char *scan;

    if (*password == '\0')
	return 0;

    for (scan = password; *scan; scan++) {
	if (!(isprint(*scan) && !isspace(*scan))) {
	    return 0;
	}
    }

    return 1;
}





