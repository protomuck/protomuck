#include "copyright.h"
#include "config.h"

#include "db.h"
#include "props.h"
#include "params.h"
#include "tune.h"
#include "interface.h"
#include "match.h"
#include "externs.h"

void 
moveto(dbref what, dbref where)
{
    dbref   loc;

    /* do NOT move garbage */
    if (what != NOTHING && Typeof(what) == TYPE_GARBAGE) {
        return;
    }

    /* remove what from old loc */
    if ((loc = DBFETCH(what)->location) != NOTHING) {
	DBSTORE(loc, contents, remove_first(DBFETCH(loc)->contents, what));
    }
    /* test for special cases */
    switch (where) {
	case NOTHING:
	    DBSTORE(what, location, NOTHING);
	    return;		/* NOTHING doesn't have contents */
	case HOME:
	    switch (Typeof(what)) {
		case TYPE_PLAYER:
		    where = DBFETCH(what)->sp.player.home;
		    break;
		case TYPE_THING:
		    where = DBFETCH(what)->sp.thing.home;
		    if (parent_loop_check(what, where))
			where = DBFETCH(OWNER(what))->sp.player.home;
		    break;
		case TYPE_ROOM:
		    where = GLOBAL_ENVIRONMENT;
		    break;
		case TYPE_PROGRAM:
		    where = OWNER(what);
		    break;
	    }
    }

    /* now put what in where */
    PUSH(what, DBFETCH(where)->contents);
    DBDIRTY(where);
    DBSTORE(what, location, where);
}

dbref   reverse(dbref);
void 
send_contents(int descr, dbref loc, dbref dest)
{
    dbref   first;
    dbref   rest;

    first = DBFETCH(loc)->contents;
    DBSTORE(loc, contents, NOTHING);

    /* blast locations of everything in list */
    DOLIST(rest, first) {
	DBSTORE(rest, location, NOTHING);
    }

    while (first != NOTHING) {
	rest = DBFETCH(first)->next;
	if ((Typeof(first) != TYPE_THING)
		&& (Typeof(first) != TYPE_PROGRAM)) {
	    moveto(first, loc);
	} else {
	    moveto(first, FLAGS(first) & STICKY ? HOME : dest);
	}
	first = rest;
    }

    DBSTORE(loc, contents, reverse(DBFETCH(loc)->contents));
}

void 
maybe_dropto(int descr, dbref loc, dbref dropto)
{
    dbref   thing;

    if (loc == dropto)
	return;			/* bizarre special case */

    /* check for players */
    DOLIST(thing, DBFETCH(loc)->contents) {
	if (Typeof(thing) == TYPE_PLAYER)
	    return;
    }

    /* no players, send everything to the dropto */
    send_contents(descr, loc, dropto);
}

int 
parent_loop_check(dbref source, dbref dest)
{
    if (source == dest)
	return 1;		/* That's an easy one! */
    if (dest == NOTHING)
	return 0;
    if (dest == HOME)
	return 0;
    if (Typeof(dest) == TYPE_THING &&
	    parent_loop_check(source, DBFETCH(dest)->sp.thing.home))
	return 1;
    return parent_loop_check(source, DBFETCH(dest)->location);
}

static int donelook = 0;
void enter_room(int descr, dbref player, dbref loc, dbref exit)
{
    dbref   old;
    dbref   dropto;
    char    buf[BUFFER_LEN];

    /* check for room == HOME */
    if (loc == HOME)
	loc = DBFETCH(player)->sp.player.home;	/* home */

    /* get old location */
    old = DBFETCH(player)->location;

    /* check for self-loop */
    /* self-loops don't do move or other player notification */
    /* but you still get autolook and penny check */
    if (loc != old) {

	/* go there */
	moveto(player, loc);

	if (old != NOTHING) {
	    propqueue(descr, player, old, exit, player, NOTHING,
			 "@depart", "Depart", 1, 1);
	    envpropqueue(descr, player, old, exit, old, NOTHING,
			 "@depart", "Depart", 1, 1);

	    propqueue(descr, player, old, exit, player, NOTHING,
			 "@odepart", "Odepart", 1, 0);
	    envpropqueue(descr, player, old, exit, old, NOTHING,
			 "@odepart", "Odepart", 1, 0);

	    propqueue(descr, player, old, exit, player, NOTHING,
			 "~depart", "Depart", 1, 1);
	    envpropqueue(descr, player, old, exit, old, NOTHING,
			 "~depart", "Depart", 1, 1);

	    propqueue(descr, player, old, exit, player, NOTHING,
			 "~odepart", "Odepart", 1, 0);
	    envpropqueue(descr, player, old, exit, old, NOTHING,
			 "~odepart", "Odepart", 1, 0);

#ifdef ALLOW_OLD_TRIGGERS
	    propqueue(descr, player, old, exit, player, NOTHING,
			 "_depart", "Depart", 1, 1);
	    envpropqueue(descr, player, old, exit, old, NOTHING,
			 "_depart", "Depart", 1, 1);

	    propqueue(descr, player, old, exit, player, NOTHING,
			 "_odepart", "Odepart", 1, 0);
	    envpropqueue(descr, player, old, exit, old, NOTHING,
			 "_odepart", "Odepart", 1, 0);
#endif

	    /* notify others unless DARK */
	    if (!Hidden(player) && !Dark(old) && !Dark(player)
		  /*  && (Typeof(exit) != TYPE_EXIT || !Dark(exit)) */ && !tp_quiet_moves) {
#if !defined(QUIET_MOVES)
		sprintf(buf, CMOVE "%s has left.", PNAME(player));
		anotify_except(DBFETCH(old)->contents, player, buf, player);
#endif
	    }
	}

	/* if old location has STICKY dropto, send stuff through it */
	if (old != NOTHING && Typeof(old) == TYPE_ROOM
		&& (dropto = DBFETCH(old)->sp.room.dropto) != NOTHING
		&& (FLAGS(old) & STICKY)) {
	    maybe_dropto(descr, old, dropto);
	}

	/* tell other folks in new location if not DARK */
	if (!Hidden(player) && !Dark(loc) && !Dark(player)
	/* 	&& (Typeof(exit) != TYPE_EXIT || !Dark(exit)) */ && !tp_quiet_moves) {
#if !defined(QUIET_MOVES)
	    sprintf(buf, CMOVE "%s has arrived.", PNAME(player));
	    anotify_except(DBFETCH(loc)->contents, player, buf, player);
#endif
	}
    }
    /* autolook */
    if (donelook < 8) {
	donelook++;
	if (!( (Typeof(exit) == TYPE_EXIT) && (FLAGS(exit)&HAVEN) )) {
	    /* These 'look's were changed to 'loo' per Grisson's bugfix */
	    if (can_move(descr, player, "loo", 1)) {
		do_move(descr, player, "loo", 1);
	    } else {
		do_look_around (descr, player);
	    }
	}
	donelook--;
    } else
	anotify_nolisten2(player, CINFO "Look aborted because of look action loop.");

    if (tp_penny_rate != 0) {
	/* check for pennies */
	if (!controls(player, loc)
		&& DBFETCH(player)->sp.player.pennies <= tp_max_pennies
		&& RANDOM() % tp_penny_rate == 0) {
	    anotify_fmt(player, CINFO "You found a %s!", tp_penny);
	    DBFETCH(OWNER(player))->sp.player.pennies++;
	    DBDIRTY(OWNER(player));
	}
    }

    if (loc != old) {
	envpropqueue(descr,player,loc,exit,player,NOTHING,"@arrive","Arrive",1,1);
	envpropqueue(descr,player,loc,exit,player,NOTHING,"@oarrive","Oarrive",1,0);
	envpropqueue(descr,player,loc,exit,player,NOTHING,"~arrive","Arrive",1,1);
	envpropqueue(descr,player,loc,exit,player,NOTHING,"~oarrive","Oarrive",1,0);
#ifdef ALLOW_OLD_TRIGGERS
	envpropqueue(descr,player,loc,exit,player,NOTHING,"_arrive","Arrive",1,1);
	envpropqueue(descr,player,loc,exit,player,NOTHING,"_oarrive","Oarrive",1,0);
#endif
    }
}

void 
send_home(int descr, dbref thing, int puppethome)
{
    switch (Typeof(thing)) {
	    case TYPE_PLAYER:
	    /* send his possessions home first! */
	    /* that way he sees them when he arrives */
	    send_contents(descr, thing, HOME);
		enter_room(descr, thing, DBFETCH(thing)->sp.player.home, DBFETCH(thing)->location);
	    break;
	case TYPE_THING:
	    if (puppethome)
		send_contents(descr, thing, HOME);
	    if (FLAGS(thing) & (ZOMBIE | LISTENER)) {
		enter_room(descr, thing, DBFETCH(thing)->sp.player.home, DBFETCH(thing)->location);
		break;
	    }
	    moveto(thing, HOME);	/* home */
	    break;
	case TYPE_PROGRAM:
	    moveto(thing, OWNER(thing));
	    break;
	default:
	    /* no effect */
	    break;
    }
    return;
}

int 
can_move(int descr, dbref player, const char *direction, int lev)
{
    struct match_data md;

    if (!string_compare(direction, "home") && (tp_enable_home == 1))
	return 1;

    /* otherwise match on exits */
    init_match(descr, player, direction, TYPE_EXIT, &md);
    md.match_level = lev;
    match_all_exits(&md);
    return (last_match_result(&md) != NOTHING);
}

/*
 * trigger()
 *
 * This procedure triggers a series of actions, or meta-actions
 * which are contained in the 'dest' field of the exit.
 * Locks other than the first one are over-ridden.
 *
 * `player' is the player who triggered the exit
 * `exit' is the exit triggered
 * `pflag' is a flag which indicates whether player and room exits
 * are to be used (non-zero) or ignored (zero).  Note that
 * player/room destinations triggered via a meta-link are
 * ignored.
 *
 */

void 
trigger(int descr, dbref player, dbref exit, int pflag)
{
    int     i;
    dbref   dest;
    int     sobjact;		/* sticky object action flag, sends home
				 * source obj */
    int     succ;
    struct frame *tmpfr;

    sobjact = 0;
    succ = 0;

    for (i = 0; i < DBFETCH(exit)->sp.exit.ndest; i++) {
	dest = (DBFETCH(exit)->sp.exit.dest)[i];
	if (dest == HOME)
	    dest = DBFETCH(player)->sp.player.home;
	switch (Typeof(dest)) {
	    case TYPE_ROOM:
		if (pflag) {
		    if (parent_loop_check(player, dest)) {
			anotify_nolisten2(player, CINFO "That would cause a paradox.");
			break;
		    }
		    if (!Mage(OWNER(player)) && Typeof(player)==TYPE_THING
			    && (FLAGS(dest)&ZOMBIE)) {
			anotify_nolisten2(player, CFAIL "You can't go that way.");
			break;
		    }
		    if ((FLAGS(player) & VEHICLE) &&
			    ((FLAGS(dest) | FLAGS(exit)) & VEHICLE)) {
			anotify_nolisten2(player, CFAIL "You can't go that way.");
			break;
		    }
		    if (GETDROP(exit))
			exec_or_notify(descr, player, exit, GETDROP(exit), "(@Drop)");
		    if (GETODROP(exit) /*&& !Dark(player)*/) {
			parse_omessage(descr, player, dest, exit, GETODROP(exit),
					PNAME(player), "(@Odrop)");
		    }
		    enter_room(descr, player, dest, exit);
		    succ = 1;
		}
		break;
	    case TYPE_THING:
		if (dest == getloc(exit) && (FLAGS(dest) & VEHICLE)) {
		    if (pflag) {
			if (parent_loop_check(player, dest)) {
			    anotify_nolisten2(player, CINFO "That would cause a paradox.");
			    break;
			}
			if (GETDROP(exit))
			    exec_or_notify(descr, player, exit, GETDROP(exit),
					   "(@Drop)");
			if (GETODROP(exit) /*&& !Dark(player)*/) {
			    parse_omessage(descr, player, dest, exit, GETODROP(exit),
				    PNAME(player), "(@Odrop)");
			}
			enter_room(descr, player, dest, exit);
			succ = 1;
		    }
		} else {
		    if (Typeof(DBFETCH(exit)->location) == TYPE_THING) {
			if (parent_loop_check(dest, getloc(getloc(exit)))) {
			    anotify_nolisten2(player, CINFO "That would cause a paradox.");
			    break;
			}
			moveto(dest, DBFETCH(DBFETCH(exit)->location)->location);
			if (!(FLAGS(exit) & STICKY)) {
			    /* send home source object */
			    sobjact = 1;
			}
		    } else {
			if (parent_loop_check(dest, getloc(exit))) {
			    anotify_nolisten2(player, CINFO "That would cause a paradox.");
			    break;
			}
			moveto(dest, DBFETCH(exit)->location);
		    }
		    if (GETSUCC(exit))
			succ = 1;
		}
		break;
	    case TYPE_EXIT:	/* It's a meta-link(tm)! */
		ts_useobject(dest);
		trigger(descr, player, (DBFETCH(exit)->sp.exit.dest)[i], 0);
		if (GETSUCC(exit))
		    succ = 1;
		break;
	    case TYPE_PLAYER:
		if (pflag && DBFETCH(dest)->location != NOTHING) {
		    if (parent_loop_check(player, dest)) {
			anotify_nolisten2(player, CINFO "That would cause a paradox.");
			break;
		    }
		    succ = 1;
		    if (FLAGS(dest) & JUMP_OK) {
			if (GETDROP(exit)) {
			    exec_or_notify(descr, player, exit,
				    GETDROP(exit), "(@Drop)");
			}
			if (GETODROP(exit) /*&& !Dark(player)*/) {
			    parse_omessage(descr, player, getloc(dest), exit,
				    GETODROP(exit), PNAME(player), "(@Odrop)");
			}
			enter_room(descr, player, DBFETCH(dest)->location, exit);
		    } else {
			anotify_nolisten2(player, CINFO "That player does not wish to be disturbed.");
		    }
		}
		break;
	    case TYPE_PROGRAM:
		tmpfr = interp(descr, player, DBFETCH(player)->location, dest, exit,
			   FOREGROUND, STD_REGUID);
		if (tmpfr) {
			interp_loop(player, dest, tmpfr, 0);
		}
		/* (void) interp(descr, player, DBFETCH(player)->location, dest, exit,
			      FOREGROUND, STD_REGUID, 0); */
		return;
	}
    }
    if (sobjact)
	send_home(descr, DBFETCH(exit)->location, 0);
    if (!succ && pflag)
	anotify_nolisten2(player, CINFO "Done.");
}

void 
do_move(int descr, dbref player, const char *direction, int lev)
{
    dbref   exit;
    dbref   loc;
    char    buf[BUFFER_LEN];
    struct match_data md;

    if (!string_compare(direction, "home") && tp_enable_home) {
	/* send him home */
	if ((loc = DBFETCH(player)->location) != NOTHING) {
	    /* tell everybody else */
	    sprintf(buf, CMOVE "%s goes home.", PNAME(player));
	    anotify_except(DBFETCH(loc)->contents, player, buf, player);
	}
	/* give the player the messages */
        anotify_nolisten2(player, RED "There's no place like home...");
        anotify_nolisten2(player, WHITE "There's no place like home...");
        anotify_nolisten2(player, BLUE "There's no place like home...");
	send_home(descr, player, 1);
    } else {
	/* find the exit */
	init_match_check_keys(descr, player, direction, TYPE_EXIT, &md);
	md.match_level = lev;
	match_all_exits(&md);
	switch (exit = match_result(&md)) {
	    case NOTHING:
		anotify_nolisten2(player, CFAIL "You can't go that way.");
		break;
	    case AMBIGUOUS:
		anotify_nolisten2(player, CINFO "I don't know which way you mean!");
		break;
	    default:
		/* we got one */
		/* check to see if we got through */
		ts_useobject(exit);
		loc = DBFETCH(player)->location;
		if (can_doit(descr, player, exit, "You can't go that way.")) {
		    trigger(descr, player, exit, 1);
		}
		break;
	}
    }
}


void
do_leave(int descr, dbref player)
{
    dbref loc, dest;

    loc = DBFETCH(player)->location;
    if (loc == NOTHING || Typeof(loc) == TYPE_ROOM) {
	anotify_nolisten2(player, CFAIL "You can't go that way.");
	return;
    }

    if (!(FLAGS(loc) & VEHICLE) && !(Typeof(loc) == TYPE_PLAYER)) {
	anotify_nolisten2(player, CFAIL "You can only exit vehicles.");
	return;
    }

    dest = DBFETCH(loc)->location;
    if(dest < 0 || dest >= db_top) return;

    if (Typeof(dest) != TYPE_ROOM && Typeof(dest) != TYPE_THING) {
	anotify_nolisten2(player, CFAIL "You can't exit a vehicle inside of a player.");
	return;
    }

/*
 *  if (Typeof(dest) == TYPE_ROOM && !controls(player, dest)
 *          && !(FLAGS(dest) | JUMP_OK)) {
 *      anotify_nolisten2(player, CFAIL "You can't go that way.");
 *      return;
 *  }
 */

    if (parent_loop_check(player, dest)) {
	anotify_nolisten2(player,CFAIL "You can't go that way.");
	return;
    }

    anotify_nolisten2(player, CSUCC "You exit the vehicle.");
    enter_room(descr, player, dest, loc);
}


void 
do_get(int descr, dbref player, const char *what, const char *obj)
{
    dbref   thing, cont;
    int cando;
    struct match_data md;

    if( tp_db_readonly ) {
	anotify_nolisten2( player, CFAIL DBRO_MESG );
	return;
    }

    init_match_check_keys(descr, player, what, TYPE_THING, &md);
    match_neighbor(&md);
    match_possession(&md);
    if (Mage(OWNER(player)))
	match_absolute(&md);	/* the wizard has long fingers */

    if ((thing = noisy_match_result(&md)) != NOTHING) {
	cont = thing;
	if (obj && *obj) {
	    init_match_check_keys(descr, player, obj, TYPE_THING, &md);
	    match_rmatch(cont, &md);
	    if (Mage(OWNER(player)))
		match_absolute(&md);	/* the wizard has long fingers */
	    if ((thing = noisy_match_result(&md)) == NOTHING) {
		return;
	    }
	    if (Typeof(cont) == TYPE_PLAYER) {
		anotify_nolisten2(player, CFAIL "You can't steal things from players.");
		return;
	    }
	    if (!test_lock_false_default(descr, player, cont, "_/clk")) {
		anotify_nolisten2(player, CFAIL "You can't open that container.");
		return;
	    }
	}
	if (DBFETCH(thing)->location == player) {
	    anotify_nolisten2(player, CINFO "You already have that.");
	    return;
	}
	if (Typeof(cont) == TYPE_PLAYER) {
	    anotify_nolisten2(player, CFAIL "You can't steal stuff from players.");
	    return;
	}
	if (parent_loop_check(thing, player)) {
	    anotify_nolisten2(player, CFAIL "You can't pick yourself up by your bootstraps!");
	    return;
	}
	switch (Typeof(thing)) {
	    case TYPE_THING:
		ts_useobject(thing);
	    case TYPE_PROGRAM:
		if (obj && *obj) {
		    cando = could_doit(descr, player, thing);
		    if (!cando)
			anotify_nolisten2(player, CFAIL "You can't get that.");
		} else {
		    cando=can_doit(descr, player, thing, "You can't pick that up.");
		}
		if (cando) {
		    moveto(thing, player);
		    anotify_nolisten2(player, CSUCC "Taken.");
		}
		break;
	    default:
		anotify_nolisten2(player, CFAIL "You can't take that!");
		break;
	}
    }
}

void 
do_drop(int descr, dbref player, const char *name, const char *obj)
{
    dbref   loc, cont;
    dbref   thing;
    char    buf[BUFFER_LEN];
    struct match_data md;

    if( tp_db_readonly ) {
	anotify_nolisten2( player, CFAIL DBRO_MESG );
	return;
    }

    if ((loc = getloc(player)) == NOTHING)
	return;

    init_match(descr, player, name, NOTYPE, &md);
    match_possession(&md);
    if ((thing = noisy_match_result(&md)) == NOTHING
	    || thing == AMBIGUOUS)
	return;

    cont = loc;
    if (obj && *obj) {
	init_match(descr, player, obj, NOTYPE, &md);
	match_possession(&md);
	match_neighbor(&md);
	if (Mage(OWNER(player)))
	    match_absolute(&md);	/* the wizard has long fingers */
	if ((cont=noisy_match_result(&md))==NOTHING || thing==AMBIGUOUS) {
	    return;
	}
    }
    switch (Typeof(thing)) {
	case TYPE_THING:
	    ts_useobject(thing);
	case TYPE_PROGRAM:
	    if (DBFETCH(thing)->location != player) {
		/* Shouldn't ever happen. */
		anotify_nolisten2(player, CFAIL "You can't drop that.");
		break;
	    }
	    if (Typeof(cont) != TYPE_ROOM && Typeof(cont) != TYPE_PLAYER &&
		    Typeof(cont) != TYPE_THING) {
		anotify_nolisten2(player, CFAIL "You can't put anything in that.");
		break;
	    }
	    if (Typeof(cont)!=TYPE_ROOM &&
		    !test_lock_false_default(descr, player, cont, "_/clk")) {
		anotify_nolisten2(player, CFAIL "You don't have permission to put something in that.");
		break;
	    }
	    if (parent_loop_check(thing, cont)) {
		anotify_nolisten2(player, CFAIL "You can't put something inside of itself.");
		break;
	    }
	    if (Typeof(cont) == TYPE_ROOM && (FLAGS(thing) & STICKY) &&
		    Typeof(thing) == TYPE_THING) {
		send_home(descr, thing, 0);
	    } else {
		int immediate_dropto = (Typeof(cont) == TYPE_ROOM &&
					DBFETCH(cont)->sp.room.dropto!=NOTHING
					&& !(FLAGS(cont) & STICKY));

		moveto(thing, immediate_dropto ? DBFETCH(cont)->sp.room.dropto : cont);
	    }
	    if (Typeof(cont) == TYPE_THING) {
		anotify_nolisten2(player, CSUCC "Put away.");
		return;
            } else if (Typeof(cont) == TYPE_PLAYER) {
		anotify_fmt(cont, CNOTE "%s hands you %s.", PNAME(player), PNAME(thing));
		anotify_fmt(player, CSUCC "You hand %s to %s.",
				    PNAME(thing), PNAME(cont));
		return;
	    }

	    if (GETDROP(thing))
		exec_or_notify(descr, player, thing, GETDROP(thing), "(@Drop)");
	    else
		anotify_nolisten2(player, CSUCC "Dropped.");

	    if (GETDROP(loc))
		exec_or_notify(descr, player, loc, GETDROP(loc), "(@Drop)");

	    if (GETODROP(thing)) {
		parse_omessage(descr, player, loc, thing, GETODROP(thing),
				PNAME(player), "(@Odrop)");
	    } else {
		sprintf(buf, BLUE "%s drops %s.", PNAME(player), PNAME(thing));
	    anotify_except(DBFETCH(loc)->contents, player, buf, player);
	    }

	    if (GETODROP(loc)) {
		parse_omessage(descr, player, loc, loc, GETODROP(loc),
				PNAME(thing), "(@Odrop)");
	    }
	    break;
	default:
	    anotify_nolisten2(player, CFAIL "You can't drop that.");
	    break;
    }
}

void 
do_recycle(int descr, dbref player, const char *name)
{
    dbref   thing;
    char buf[BUFFER_LEN];
    struct match_data md;

    if(Guest(player)) {
	anotify_nolisten2(player, CFAIL NOGUEST_MESG);
	return;
    }

    if( tp_db_readonly ) {
	anotify_nolisten2( player, CFAIL DBRO_MESG );
	return;
    }

    init_match(descr, player, name, TYPE_THING, &md);
    match_all_exits(&md);
    match_neighbor(&md);
    match_possession(&md);
    match_registered(&md);
    match_here(&md);
    if (Mage(OWNER(player))) {
	match_absolute(&md);
    }
    if ((thing = noisy_match_result(&md)) != NOTHING) {
	if ( (!controls(player, thing)) || (Protect(thing) && !(MLevel(player) > MLevel(OWNER(thing)))) ) {
   	    anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	} else {
	    switch (Typeof(thing)) {
		case TYPE_ROOM:
		    if (OWNER(thing) != OWNER(player)) {
			anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
			return;
		    }
		    if (thing == tp_player_start || thing == GLOBAL_ENVIRONMENT) {
			anotify_nolisten2(player, CFAIL "This room may not be recycled.");
			return;
		    }
		    break;
		case TYPE_THING:
		    if (OWNER(thing) != OWNER(player)) {
			anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
			return;
		    }
		    if (thing == player) {
			sprintf(buf, BLUE "%.512s's owner commands it to kill itself.  It blinks a few times in shock, and says, \"But.. but.. WHY?\"  It suddenly clutches it's heart, grimacing with pain..  Staggers a few steps before falling to it's knees, then plops down on it's face.  *thud*  It kicks it's legs a few times, with weakening force, as it suffers a seizure.  It's color slowly starts changing to purple, before it explodes with a fatal *POOF*!", PNAME(thing));
			anotify_except(DBFETCH(getloc(thing))->contents,
				thing, buf, player);
			anotify_nolisten2(OWNER(player), buf);
			anotify_nolisten2(OWNER(player), CINFO "Now don't you feel guilty?");
		    }
		    break;
		case TYPE_EXIT:
		    if (OWNER(thing) != OWNER(player)) {
			anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
			return;
		    }
		    if (!unset_source(player, DBFETCH(player)->location, thing)) {
			anotify_nolisten2(player, CFAIL "You can't do that to an exit in another room.");
			return;
		    }
		    break;
		case TYPE_PLAYER:
		    anotify_nolisten2(player, CFAIL "You can't recycle a player!");
		    return;
		    /* NOTREACHED */
		    break;
		case TYPE_PROGRAM:
		    if (OWNER(thing) != OWNER(player)) {
			anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
			return;
		    }
		    break;
		case TYPE_GARBAGE:
		    anotify_nolisten2(player, CINFO "That's already garbage.");
		    return;
		    /* NOTREACHED */
		    break;
	    }
	    recycle(descr, player, thing);
	    anotify_nolisten2(player, CINFO "Thank you for recycling.");
	}
    }
}

extern int unlink(const char *);
void 
recycle(int descr, dbref player, dbref thing)
{
    extern dbref recyclable;
    static int depth = 0;
    dbref   first;
    dbref   rest;
    char    buf[2048];
    int     looplimit;

    depth++;
    switch (Typeof(thing)) {
	case TYPE_ROOM:
	    if (!Mage(OWNER(thing)))
		DBFETCH(OWNER(thing))->sp.player.pennies += tp_room_cost;
	    DBDIRTY(OWNER(thing));
	    for (first = DBFETCH(thing)->exits; first != NOTHING; first = rest) {
		rest = DBFETCH(first)->next;
		if (DBFETCH(first)->location == NOTHING || DBFETCH(first)->location == thing)
		    recycle(descr, player, first);
	    }
	    anotify_except(DBFETCH(thing)->contents, NOTHING, CNOTE
			  "You feel a wrenching sensation...", player);
	    break;
	case TYPE_THING:
	    if (!Mage(OWNER(thing)))
		DBFETCH(OWNER(thing))->sp.player.pennies +=
		    DBFETCH(thing)->sp.thing.value;
	    DBDIRTY(OWNER(thing));
	    for (first = DBFETCH(thing)->exits; first != NOTHING;
		    first = rest) {
		rest = DBFETCH(first)->next;
		if (DBFETCH(first)->location == NOTHING || DBFETCH(first)->location == thing)
		    recycle(descr, player, first);
	    }
	    break;
	case TYPE_EXIT:
	    if (!Mage(OWNER(thing)))
		DBFETCH(OWNER(thing))->sp.player.pennies += tp_exit_cost;
	    if (!Mage(OWNER(thing)))
		if (DBFETCH(thing)->sp.exit.ndest != 0)
		    DBFETCH(OWNER(thing))->sp.player.pennies += tp_link_cost;
	    DBDIRTY(OWNER(thing));
	    break;
	case TYPE_PROGRAM:
	    dequeue_prog(thing, 0);
	    sprintf(buf, "muf/%d.m", (int) thing);
	    unlink(buf);
	    sprintf(buf, "mcp/%d.mcp", (int) thing);
	    unlink(buf);
	    break;
    }

    for (rest = 0; rest < db_top; rest++) {
	switch (Typeof(rest)) {
	    case TYPE_ROOM:
		if (DBFETCH(rest)->sp.room.dropto == thing) {
		    DBFETCH(rest)->sp.room.dropto = NOTHING;
		    DBDIRTY(rest);
		}
		if (DBFETCH(rest)->exits == thing) {
		    DBFETCH(rest)->exits = DBFETCH(thing)->next;
		    DBDIRTY(rest);
		}
		if (OWNER(rest) == thing) {
		    OWNER(rest) = MAN;
		    DBDIRTY(rest);
		}
		break;
	    case TYPE_THING:
		if (DBFETCH(rest)->sp.thing.home == thing) {
		    if (DBFETCH(OWNER(rest))->sp.player.home == thing)
			DBSTORE(OWNER(rest), sp.player.home, tp_player_start);
		    DBFETCH(rest)->sp.thing.home = DBFETCH(OWNER(rest))->sp.player.home;
		    DBDIRTY(rest);
		}
		if (DBFETCH(rest)->exits == thing) {
		    DBFETCH(rest)->exits = DBFETCH(thing)->next;
		    DBDIRTY(rest);
		}
		if (OWNER(rest) == thing) {
		    OWNER(rest) = MAN;
		    DBDIRTY(rest);
		}
		break;
	    case TYPE_EXIT:
		{
		    int     i, j;

		    for (i = j = 0; i < DBFETCH(rest)->sp.exit.ndest; i++) {
			if ((DBFETCH(rest)->sp.exit.dest)[i] != thing)
			    (DBFETCH(rest)->sp.exit.dest)[j++] =
				(DBFETCH(rest)->sp.exit.dest)[i];
		    }
		    if (j < DBFETCH(rest)->sp.exit.ndest) {
			DBFETCH(OWNER(rest))->sp.player.pennies += tp_link_cost;
			DBDIRTY(OWNER(rest));
			DBFETCH(rest)->sp.exit.ndest = j;
			DBDIRTY(rest);
		    }
		}
		if (OWNER(rest) == thing) {
		    OWNER(rest) = MAN;
		    DBDIRTY(rest);
		}
		break;
	    case TYPE_PLAYER:
		if (Typeof(thing) == TYPE_PROGRAM && (FLAGS(rest) & INTERACTIVE)
			&& (DBFETCH(rest)->sp.player.curr_prog == thing)) {
		    if (FLAGS(rest) & READMODE) {
			anotify_nolisten2(rest, CINFO "The program you were running has been recycled.  Aborting program.");
		    } else {
			free_prog_text(DBFETCH(thing)->sp.program.first);
			DBFETCH(thing)->sp.program.first = NULL;
			DBFETCH(rest)->sp.player.insert_mode = 0;
			FLAGS(thing) &= ~INTERNAL;
			FLAGS(rest) &= ~INTERACTIVE;
			DBFETCH(rest)->sp.player.curr_prog = NOTHING;
			anotify_nolisten2(rest, CINFO "The program you were editing has been recycled.  Exiting Editor.");
		    }
		}
		if (DBFETCH(rest)->sp.player.home == thing) {
		    DBFETCH(rest)->sp.player.home = tp_player_start;
		    DBDIRTY(rest);
		}
		if (DBFETCH(rest)->exits == thing) {
		    DBFETCH(rest)->exits = DBFETCH(thing)->next;
		    DBDIRTY(rest);
		}
		if (DBFETCH(rest)->sp.player.curr_prog == thing)
		    DBFETCH(rest)->sp.player.curr_prog = 0;
		break;
	    case TYPE_PROGRAM:
		if (OWNER(rest) == thing) {
		    OWNER(rest) = MAN;
		    DBDIRTY(rest);
		}
	}
	/*
	 *if (DBFETCH(rest)->location == thing)
	 *    DBSTORE(rest, location, NOTHING);
	 */
	if (DBFETCH(rest)->contents == thing)
	    DBSTORE(rest, contents, DBFETCH(thing)->next);
	if (DBFETCH(rest)->next == thing)
	    DBSTORE(rest, next, DBFETCH(thing)->next);
    }

    looplimit = db_top;
    while ((looplimit-->0) && ((first = DBFETCH(thing)->contents) != NOTHING)){
        if (Typeof(first) == TYPE_PLAYER) {
            enter_room(descr, first, HOME, DBFETCH(thing)->location);
            /* If the room is set to drag players back, there'll be no
             * reasoning with it.  DRAG the player out.
             */
            if (DBFETCH(first)->location == thing) {
                notify_fmt(player, "Escaping teleport loop!");
                moveto(first, HOME);
            }
        } else {
            moveto(first, HOME);
        }
    }


    moveto(thing, NOTHING);

    depth--;

    db_free_object(thing);
    db_clear_object(thing);


    NAME(thing) = "<garbage>";
    SETDESC(thing, "<recyclable>");
    FLAGS(thing) = TYPE_GARBAGE;

    DBFETCH(thing)->next = recyclable;
    recyclable = thing;
    DBDIRTY(thing);
}





