#include "copyright.h"
#include "config.h"

#include "db.h"
#include "mpi.h"
#include "interface.h"
#include "match.h"
#include "params.h"
#include "tune.h"
#include "props.h"
#include "externs.h"
#include <ctype.h>

/* Commands which involve speaking */

int     blank(const char *s);

void 
do_say(int descr, dbref player, const char *message)
{
    dbref   loc;
    char    buf[BUFFER_LEN], buf2[BUFFER_LEN];

    if ((loc = getloc(player)) == NOTHING)
	return;
//Removed the MPI parsing from say/pose. Yesh. -Akari
//    if(Meeper(OWNER(player))) {
//	do_parse_mesg(descr, player, player, message, "(say)", buf, MPI_ISPRIVATE);
//	tct(buf,buf2);
//    } else {
	tct(message,buf2);
//    }

    /* notify everybody */
    sprintf(buf, AQUA "You say, \"" YELLOW "%s" AQUA "\"", buf2);
    anotify(player, buf);

    sprintf(buf, AQUA "%s says, \"" YELLOW "%s" AQUA "\"", PNAME(player), buf2);
    anotify_except(DBFETCH(loc)->contents, player, buf, player);
}

void 
do_whisper(int descr, dbref player, const char *arg1, const char *arg2)
{
    dbref   who;
    char    buf[BUFFER_LEN], buf2[BUFFER_LEN];
    struct match_data md;

    if(Guest(player)) {
	anotify_fmt(player, CFAIL "%s", tp_noguest_mesg);
	return;
    }

    init_match(descr, player, arg1, TYPE_PLAYER, &md);
    match_neighbor(&md);
    match_me(&md);
    if (Mage(player) && Typeof(player) == TYPE_PLAYER) {
	match_absolute(&md);
	match_player(&md);
    }
    switch (who = match_result(&md)) {
	case NOTHING:
	    anotify_nolisten2(player, CINFO "Who?");
	    break;
	case AMBIGUOUS:
	    anotify_nolisten2(player, CINFO "I don't know who you mean!");
	    break;
	default:
	    if(Meeper(OWNER(player))) {
		do_parse_mesg(descr, player, player, arg2, "(whisper)", buf, MPI_ISPRIVATE);
		tct(buf,buf2);
	    } else {
		tct(arg2,buf2);
	    }

	    if (buf2[0] == ':' || buf2[0] == ';') {
		sprintf(buf, BLUE "%s whispers, \"" PURPLE "%s %s" BLUE "\"",
					PNAME(player), PNAME(player), buf2+1);
		if (!anotify_from(player, who, buf)) {
		    sprintf(buf, BLUE "%s is not connected.", PNAME(who));
		    anotify_nolisten2(player, buf);
		    break;
		}
		sprintf(buf, BLUE "You whisper, \"" PURPLE "%s %s" BLUE "\" to %s.",
					 PNAME(player), buf2+1, PNAME(who));
		anotify(player, buf);
		break;
	    } else { 
		sprintf(buf, BLUE "%s whispers, \"" PURPLE "%s" BLUE "\"", PNAME(player), buf2);
		if (!anotify_from(player, who, buf)) {
		    sprintf(buf, BLUE "%s is not connected.", PNAME(who));
		    anotify_nolisten2(player, buf);
		    break;
		}
		sprintf(buf, BLUE "You whisper, \"" PURPLE "%s" BLUE "\" to %s.", buf2, PNAME(who));
		anotify(player, buf);
		break;
	    }
    }
}

void 
do_pose(int descr, dbref player, const char *message)
{
    dbref   loc;
    char    buf[BUFFER_LEN], buf2[BUFFER_LEN];

    if ((loc = getloc(player)) == NOTHING)
	return;

    if(Meeper(OWNER(player))) {
	do_parse_mesg(descr, player, player, message, "(pose)", buf, MPI_ISPRIVATE);
	tct(buf,buf2);
    } else {
	tct(message,buf2);
    }

    /* notify everybody */
    sprintf(buf, AQUA "%s %s", PNAME(player), buf2);
    anotify_except(DBFETCH(loc)->contents, NOTHING, buf, player);
}

void 
do_wall(dbref player, const char *message)
{
    char    buf[BUFFER_LEN];

    if ((Mage(player) || (POWERS(player) & POW_ANNOUNCE)) && Typeof(player) == TYPE_PLAYER) {
	if (!*message) {
	    anotify_nolisten2(player, CINFO "Shout what?");
	    return;
	}
	switch(message[0]) {
	    case ':':
	    case ';':
		sprintf(buf, MARK "%s %s", NAME(player), message+1);
		break;
	    case '#':
		sprintf(buf, MARK "%s", message+1);
		break;
	    default:
		sprintf(buf, MARK "%s shouts, \"%s\"", NAME(player), message);
	}
	wall_all( buf );
	/* log_status("WALL: %s(%d): %s\n", NAME(player), player, buf); */
    } else {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
    }
}

void 
do_gripe(dbref player, const char *message)
{
    dbref   loc;
    char buf[BUFFER_LEN];

    if(Guest(player)) {
	anotify_fmt(player, CFAIL "%s", tp_noguest_mesg);
	return;
    }

    if (!message || !*message) {
	if (Wiz(player)) {
	    spit_file(player, LOG_GRIPE);
	} else {
	    anotify_nolisten2(player, CINFO "What's wrong?");
	}
	return;
    }

    loc = DBFETCH(player)->location;
    log_gripe("%s(%d) in %s(%d): %s\n",
	      NAME(player), player, NAME(loc), loc, message);

    anotify_nolisten2(player, CINFO "Your complaint has been filed.");

    sprintf(buf, MARK "Gripe from %s: %s", NAME(player), message);
    wall_wizards(buf);
}

/* doesn't really belong here, but I couldn't figure out where else */
void 
do_page(int descr, dbref player, const char *arg1, const char *arg2)
{
    char    buf[BUFFER_LEN], buf2[BUFFER_LEN];
    dbref   target;

    if (!payfor(player, tp_lookup_cost)) {
	anotify_fmt(player, CFAIL "You don't have enough %s.", tp_pennies);
	return;
    }
    if ( strcmp(arg1, "me") ) {
	if ((target = lookup_player(arg1)) == NOTHING) {
	    anotify_nolisten2(player, CINFO "Who?");
	    return;
	}
    } else target = player;

    if(Guest(player)) {
	if(!Mage(target)) {
	    anotify_nolisten2(player, CINFO "Guests can only page wizards, type 'wizzes'.");
	    return;
	}
    }

    if (FLAGS(target) & HAVEN) {
	anotify_nolisten2(player, CFAIL "That player is haven.");
	return;
    }
    if(Meeper(OWNER(player))) {
	do_parse_mesg(descr, player, player, arg2, "(page)", buf, MPI_ISPRIVATE);
	tct(buf,buf2);
    } else {
	tct(arg2,buf2);
    }

    if (!*buf2) {
	sprintf(buf, CSUCC "You sense that %s is looking for you in %s.",
			PNAME(player), NAME(DBFETCH(player)->location));
    } else {
	if(buf2[0] == ':' || buf2[0] == ';') {
	    sprintf(buf, GREEN "%s pages \"" YELLOW "%s %s" GREEN "\"",
			PNAME(player), PNAME(player), buf2);
	} else {
	    sprintf(buf, GREEN "%s pages \"" YELLOW "%s" GREEN "\"",
			PNAME(player), buf2);
	}
    }
    if (anotify_from(player, target, buf))
	anotify_nolisten2(player, CSUCC "Your message has been sent.");
    else {
	sprintf(buf, CSUCC "%s is not connected.", PNAME(target));
	anotify_nolisten2(player, buf);
    }
}

int 
notify_listeners(dbref who, dbref xprog, dbref obj,
		 dbref room, const char *msg, int isprivate)
{
    char buf[BUFFER_LEN];
    dbref ref;

    if (obj == NOTHING)
	return;

    if (tp_listeners && (tp_listeners_obj || Typeof(obj) == TYPE_ROOM)) {
	listenqueue(-1, who,room,obj,obj,xprog,"_listen",msg, tp_listen_mlev,1,0);
	listenqueue(-1, who,room,obj,obj,xprog,"_olisten",msg,tp_listen_mlev,0,0);
	listenqueue(-1, who,room,obj,obj,xprog,"~listen",msg, tp_listen_mlev,1,1);
	listenqueue(-1, who,room,obj,obj,xprog,"~olisten",msg,tp_listen_mlev,0,1);
	listenqueue(-1, who,room,obj,obj,xprog,"@listen",msg, tp_listen_mlev,1,1);
	listenqueue(-1, who,room,obj,obj,xprog,"@olisten",msg,tp_listen_mlev,0,1);
    }

    if (tp_zombies && Typeof(obj) == TYPE_THING && !isprivate) {
	if (FLAGS(obj) & VEHICLE) {
	    if (getloc(who) == getloc(obj)) {
		char pbuf[BUFFER_LEN];
		const char *prefix;

		prefix = GETOECHO(obj);
		if (prefix && *prefix) {
		    prefix = do_parse_mesg(-1, who, obj, prefix,
				    "(@Oecho)", pbuf, MPI_ISPRIVATE
			     );
		}
		if (!prefix || !*prefix)
		    prefix = "Outside>";
               sprintf(buf, "%s %.*s", prefix,
                       (int)(BUFFER_LEN - 2 - strlen(prefix)), msg
		);
		ref = DBFETCH(obj)->contents;
		while(ref != NOTHING) {
		    notify_nolisten(ref, buf, isprivate);
		    ref = DBFETCH(ref)->next;
		}
	    }
	}
    }

    if (Typeof(obj) == TYPE_PLAYER || Typeof(obj) == TYPE_THING) {
	notify_nolisten(obj, msg, isprivate);
    } else {
      return 0;
    }
}


int 
ansi_notify_listeners(dbref who, dbref xprog, dbref obj,
		 dbref room, const char *msg, int isprivate)
{
    char buf[BUFFER_LEN], buf2[BUFFER_LEN], *noabuf;
    dbref ref;

    if (obj == NOTHING)
	return;

    noabuf = unparse_ansi(buf2, msg);

    if (tp_listeners && (tp_listeners_obj || Typeof(obj) == TYPE_ROOM)) {
	listenqueue(-1, who,room,obj,obj,xprog,"_listen",noabuf, tp_listen_mlev,1,0);
	listenqueue(-1, who,room,obj,obj,xprog,"_olisten",noabuf,tp_listen_mlev,0,0);
	listenqueue(-1, who,room,obj,obj,xprog,"~listen",noabuf, tp_listen_mlev,1,1);
	listenqueue(-1, who,room,obj,obj,xprog,"~olisten",noabuf,tp_listen_mlev,0,1);
	listenqueue(-1, who,room,obj,obj,xprog,"@listen",noabuf, tp_listen_mlev,1,1);
	listenqueue(-1, who,room,obj,obj,xprog,"@olisten",noabuf,tp_listen_mlev,0,1);
    }

    if (tp_zombies && Typeof(obj) == TYPE_THING && !isprivate) {
	if (FLAGS(obj) & VEHICLE) {
	    if (getloc(who) == getloc(obj)) {
		char pbuf[BUFFER_LEN];
		const char *prefix;

		prefix = GETOECHO(obj);
		if (prefix && *prefix) {
		    prefix = do_parse_mesg(-1, who, obj, prefix,
				    "(@Oecho)", pbuf, MPI_ISPRIVATE
			     );
		}
		if (!prefix || !*prefix)
		    prefix = "Outside>";
               sprintf(buf, "%s %.*s", prefix,
                       (int)(BUFFER_LEN - 2 - strlen(prefix)), msg
		);
		ref = DBFETCH(obj)->contents;
		while(ref != NOTHING) {
		    anotify_nolisten(ref, buf, isprivate);
		    ref = DBFETCH(ref)->next;
		}
	    }
	}
    }

    if (Typeof(obj) == TYPE_PLAYER || Typeof(obj) == TYPE_THING) {
	return anotify_nolisten(obj, msg, isprivate);
    } else {
      return 0;
    }
}

int 
notify_html_listeners(dbref who, dbref xprog, dbref obj,
		 dbref room, const char *msg, int isprivate)
{
    char buf[BUFFER_LEN], *nohbuf;
    dbref ref;

    if (obj == NOTHING)
	return;

    nohbuf = html_escape(msg);

    if (tp_listeners && (tp_listeners_obj || Typeof(obj) == TYPE_ROOM)) {
	listenqueue(-1, who,room,obj,obj,xprog,"_listen",nohbuf, tp_listen_mlev,1,0);
	listenqueue(-1, who,room,obj,obj,xprog,"_olisten",nohbuf,tp_listen_mlev,0,0);
	listenqueue(-1, who,room,obj,obj,xprog,"~listen",nohbuf, tp_listen_mlev,1,1);
	listenqueue(-1, who,room,obj,obj,xprog,"~olisten",nohbuf,tp_listen_mlev,0,1);
	listenqueue(-1, who,room,obj,obj,xprog,"@listen",nohbuf, tp_listen_mlev,1,1);
	listenqueue(-1, who,room,obj,obj,xprog,"@olisten",nohbuf,tp_listen_mlev,0,1);
    }

    if (tp_zombies && Typeof(obj) == TYPE_THING && !isprivate) {
	if (FLAGS(obj) & VEHICLE) {
	    if (getloc(who) == getloc(obj)) {
		char pbuf[BUFFER_LEN];
		const char *prefix;

		prefix = GETOECHO(obj);
		if (prefix && *prefix) {
		    prefix = do_parse_mesg(-1, who, obj, prefix,
				    "(@Oecho)", pbuf, MPI_ISPRIVATE
			     );
		}
		if (!prefix || !*prefix)
		    prefix = "Outside>";
               sprintf(buf, "%s %.*s", prefix,
                       (int)(BUFFER_LEN - 2 - strlen(prefix)), msg
		);
		ref = DBFETCH(obj)->contents;
		while(ref != NOTHING) {
		    notify_html_nolisten(ref, buf, isprivate);
		    ref = DBFETCH(ref)->next;
		}
	    }
	}
    }

    if (Typeof(obj) == TYPE_PLAYER || Typeof(obj) == TYPE_THING) {
	notify_html_nolisten(obj, msg, isprivate);
    } else {
      return 0;
    }
}


void 
notify_except(dbref first, dbref exception, const char *msg, dbref who)
{
    dbref   room, srch;

    if (first != NOTHING) {

	srch = room = DBFETCH(first)->location;

	if (tp_listeners) {
	    notify_from_echo(who, srch, msg, 0);

	    if (tp_listeners_env) {
		srch = DBFETCH(srch)->location;
		while (srch != NOTHING) {
		    notify_from_echo(who, srch, msg, 0);
		    srch = getparent(srch);
		}
	    }
	}

	DOLIST(first, first) {
	    if ((Typeof(first) != TYPE_ROOM) && (first != exception)) {
		/* don't want excepted player or child rooms to hear */
		notify_from_echo(who, first, msg, 0);
	    }
	}
    }
}


void 
notify_html_except(dbref first, dbref exception, const char *msg, dbref who)
{
    dbref   room, srch;

    if (first != NOTHING) {

	srch = room = DBFETCH(first)->location;

	if (tp_listeners) {
	    notify_from_echo(who, srch, msg, 0);

	    if (tp_listeners_env) {
		srch = DBFETCH(srch)->location;
		while (srch != NOTHING) {
		    notify_html_from_echo(who, srch, msg, 0);
		    srch = getparent(srch);
		}
	    }
	}

	DOLIST(first, first) {
	    if ((Typeof(first) != TYPE_ROOM) && (first != exception)) {
		/* don't want excepted player or child rooms to hear */
		notify_html_from_echo(who, first, msg, 0);
	    }
	}
    }
}



void 
anotify_except(dbref first, dbref exception, const char *msg, dbref who)
{
    dbref   room, srch;

    if (first != NOTHING) {

	srch = room = DBFETCH(first)->location;

	if (tp_listeners) {
	    anotify_from_echo(who, srch, msg, 0);

	    if (tp_listeners_env) {
		srch = DBFETCH(srch)->location;
		while (srch != NOTHING) {
		    anotify_from_echo(who, srch, msg, 0);
		    srch = getparent(srch);
		}
	    }
	}

	DOLIST(first, first) {
	    if ((Typeof(first) != TYPE_ROOM) && (first != exception)) {
		/* don't want excepted player or child rooms to hear */
		anotify_from_echo(who, first, msg, 0);
	    }
	}
    }
}


void
parse_omessage(int descr, dbref player, dbref dest, dbref exit, const char *msg, const char *prefix, const char *whatcalled)
{
    char buf[BUFFER_LEN * 2];
    char *ptr;

    do_parse_mesg(descr, player, exit, msg, whatcalled, buf, MPI_ISPUBLIC);
    ptr = pronoun_substitute(descr, player, buf);
    if (!*ptr) return;
    if (*ptr == '\'' || *ptr == ' ' || *ptr == ',' || *ptr == '-') {
	sprintf(buf, "%s%s", NAME(player), ptr);
    } else {
	sprintf(buf, "%s %s", NAME(player), ptr);
    }
    notify_except(DBFETCH(dest)->contents, player, buf, player);
}


int
blank(const char *s)
{
    while (*s && isspace(*s))
	s++;

    return !(*s);
}






