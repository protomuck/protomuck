#include "config.h"

#include <stdio.h>
#ifdef HAVE_SYS_RESOURCE_H
# include <sys/resource.h>
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#endif

#ifndef MALLOC_PROFILING
#  ifndef HAVE_MALLOC_H
#    include <stdlib.h>
#  else
#    include <malloc.h>
#  endif
#endif

#include "copyright.h"
#include "interface.h"
#include "db.h"
#include "props.h"
#include "params.h"
#include "tune.h"
#include "match.h"
#include "externs.h"
#include "reg.h"
#include "maillib.h"

extern	int resolver_sock[2];

void
get_inquotes( char *buf, char *retbuf, int which )
{
	int i;
	retbuf[0]='\0';

	for( i=0; i<(2*(which-1)+1); i++, buf++)
	    while(*buf != '\'') if(*buf == '\0') return; else buf++;

	while(*buf != '\'' && (*buf) != '\0') (*(retbuf++)) = (*(buf++));
	*retbuf = '\0';
}

void strreplace(char *new, const char *base, const char *orig, 
                const char *replace) 
{
    char xbuf[BUFFER_LEN];
    char buf[BUFFER_LEN];
    int i = 0, j = 0;
    int k;

    k = strlen(replace);

    bcopy(base, xbuf, strlen(base) + 1);

    buf[0] = '\0';

    while (xbuf[i])
    {
       if (!strncmp(xbuf + i, orig, strlen(orig)))
       {
          if ((j + k + 1) < BUFFER_LEN)
          {
              strcat(buf, replace);
              i += strlen(orig);
              j += k;
          }
       } else
       {
          if ((j + 1) < BUFFER_LEN)
          {
              buf[j++] = xbuf[i++];
              buf[j] = '\0';
          }
       }
    }
    sprintf(new, "%s", buf);
}

int
send_email ( const char *email, const char *charname, char *charpass )
{
   char outputbuf[2048];
   char buf[128];
   const char *m = NULL;
   char *m2 = NULL;
   int lines, mysock;
   dbref what = 0;

   sprintf(buf, "%s Character Request", tp_muckname);

   if (!email_start_send(email,tp_reg_email,&buf[0],&mysock)) {

	   m2 = malloc(BUFFER_LEN);
	   lines = 0;
	   while ((lines < 256) && (!lines || (m && *m))) {
		   lines++;
		   sprintf(buf, "@new-user-mail#/%d", lines);
		   m = get_property_class(what, buf);
#ifdef COMPRESS
		   if (m && *m) m = uncompress(m);
#endif
		   if (m && *m) {
			   sprintf(m2, "%s", m);
			   strreplace(m2, m2, "%user%", charname);
			   strreplace(m2, m2, "%pass%", charpass);
			   sprintf(outputbuf, "%s", m2);
			   if (email_send_line(outputbuf, mysock)) { 
				   free(m2);
				   return 1;
			   }
		   }
	   }
	   free(m2);
	   if (!email_end_send(mysock)) return 0; else return 1;
   } else return 1;
}

void
email_newbie( const char *name, const char *email, const char *rlname )
{
	dbref newguy;
	char pw[ 64 ];

	if( tp_reg_wiz < 0 || tp_reg_wiz >= db_top )
	    return; /* Pooh on them! */

	/* Create random password to be mailed user: */
	reg_make_password( pw );
	if (!ok_player_name(name)) {
	    anotify( tp_reg_wiz, RED "AutoReg> Sorry, that name is invalid or in use." );
	} else if (!strchr(email,'@') || !strchr(email,'.')) {
	    anotify( tp_reg_wiz, RED "AutoReg> That isn't a valid email address." );
	} else if (email[0]=='<') {
	    anotify( tp_reg_wiz, RED "AutoReg> Can't have <>s around the email address." );
	} else {
	    if ((newguy = create_player(name, pw)) == NOTHING) {
		anotify( tp_reg_wiz, RED "AutoReg> Name exists or illegal name." );
	    } else {
		log_status("PCRE: %s(%d) by %s(%d)\n",
			   NAME(newguy), (int) newguy, NAME(tp_reg_wiz), (int) tp_reg_wiz);
		    anotify_fmt(tp_reg_wiz,
			GREEN "AutoReg> Player %s created as object #%d.",
			NAME(newguy), newguy);

		/* Record email address on new char: */
		{   char buf[ 1024 ];
		    char date[  40 ];
		    /* Compute date in '93Dec01' format: */
		    #include <time.h>
		    time_t t = current_systime;
		    format_time( date,32,"%y%b%d\0", localtime(&t));
		    sprintf(buf,"%s:%s:%s:%s:%s",
			NAME(newguy), rlname, email, date, NAME(tp_reg_wiz)
		    );
		    add_property(newguy, "@/id", buf, 0 );
		}

		/* Mail charname and password to user: */
		{ 
/*
                    char buf[1024];
		    sprintf(buf, "./newuser %s %s %s &",
			email, NAME(newguy), pw
		    );
		    system( buf );
 */
                    if (send_email(email, NAME(newguy), pw) != 0)
                       log_status("PCRE: E-mail failure to %s for %s.", 
                           email, NAME(newguy));
		}

	    }
	}
}


void
hop_newbie( dbref player, int entry )
{
	char  line[BUFFER_LEN];
	char  name[BUFFER_LEN];
	char  email[BUFFER_LEN];
	char  rlname[BUFFER_LEN];

	get_file_line( LOG_HOPPER, line, entry );
	if( line[0] == '\0' )
	{
	    anotify( player, RED "Invalid hopper entry." );
	    return;
	}

	get_inquotes(line, name, 1);
	get_inquotes(line, email, 2);
	get_inquotes(line, rlname, 3);

	if( (*name) == '\0' || (*email) == '\0' || (*rlname) == '\0' )
	{
	    anotify( player, RED "Mangled hopper entry." );
	    return;
	}

	anotify_fmt(player, GREEN "Name: '%s' Email: '%s' RLName: '%s'",
		name, email, rlname);

	email_newbie(name, email, rlname);
}

int hop_count( )
{
    FILE   *f;
    char    buf[BUFFER_LEN];
    char   *p;
    int     currline;

    currline = 0;

    if ((f = fopen(LOG_HOPPER, "r")) == NULL) {
	return 0;
    } else {
	while (fgets(buf, sizeof buf, f)) {
	    for (p = buf; *p; p++)
		if (*p == '\n') {
		    *p = '\0';
		    break;
		}
	    currline++;
	}
	fclose(f);
	return( currline );
    }
}

void
do_hopper( dbref player, const char *arg )
{
	char buf[BUFFER_LEN];
	char *a, *p;
	int e;

	a = strcpy( buf, arg );
      a = strcat( buf, "\0");

	if( !Arch( OWNER( player ) ) ) {
		anotify_nolisten2( player, RED NOPERM_MESG ); return;
	}

	if( *a == '\0' ) {
notify( player,	"Type:" );
notify( player, "@hopper list num1-num2 -- list 'num1' to 'num2' registrations" );
notify( player, "@hopper count          -- show number of regs in hopper" );
notify( player, "@hopper clear          -- clear the hopper" );
notify( player, "@hopper reg entry      -- perform a registration" );
notify( player, "  " );
notify( player, "To add a bad email to the jerk list:" );
notify( player, "@set #0=@/jerks/email@address:DYMonYR:YourWizName:Reason" );
		return;
	}

	p = a;
	while( (*p) != '\0' && (*p) != ' ' ) p++;
	if( *p == ' ' ) *(p++) = '\0'; 
	while( (*p) == ' ' ) p++;

	if( !string_compare( a, "count" ) ) {
		anotify_fmt( player, CNOTE
			"There are %d registrations in the hopper.",
			hop_count()
		);
		return;
	}

	if( !string_compare( a, "list" ) ) {
		if( hop_count() > 0 ) {
		    spit_file_segment_lines( player, LOG_HOPPER, p );
		    anotify_nolisten2( player, CINFO "Done." );
		} else
		    anotify_nolisten2( player, CINFO "The registration hopper is empty." );
		return;
	}

	if( tp_reg_wiz != player ) {
		anotify_nolisten2( player, CFAIL "You are not set as the registration wizard." );
		anotify_nolisten2( player, CINFO "To process or clear registrations, type:" );
		anotify_nolisten2( player, CNOTE "@tune reg_wiz=me" );
		return;
	}

	if( !string_compare( a, "reg" ) ) {
		e = atoi(p);

		if( e <= 0 || (*p) == '\0' ) {
			anotify_nolisten2( player, CFAIL "Missing or invalid file entry number." );
			return;
		}
		hop_newbie( player, e );
/*            anotify_nolisten2(player, CFAIL "The hopper is disabled due to instability."); */
		return;
	}

	if( !string_compare( a, "clear" ) ) {
		unlink( LOG_HOPPER );
		anotify_nolisten2( player, CSUCC "Registration hopper cleared." );
		return;
	}

	anotify_nolisten2( player, CINFO "Unknown option, type '@hopper' for help." );
}

void
do_wizchat(dbref player, const char *arg)
{
	char buf[BUFFER_LEN], buf2[BUFFER_LEN], buf3[BUFFER_LEN];

	if( !Mage(OWNER(player)) )
	{
		anotify_nolisten2( player, RED NOPERM_MESG );
		return;
	}

	switch( arg[0] ){

        case '|': sprintf( buf, BLUE "WizChat" PURPLE "> " AQUA "%s", tct(arg+1, buf2) );
                  break;

	  case '#':	sprintf( buf, BLUE "WizChat" PURPLE "> " AQUA "%s", tct(arg+1, buf2) );
			break;

	  case ':':
	  case ';':	sprintf( buf, BLUE "WizChat" PURPLE "> " AQUA "%s %s", tct(NAME(player), buf2), tct(arg+1, buf3) );
			break;

	  case '?':
	  		show_wizards( player );
	  		return;

	  default:	sprintf( buf, BLUE "WizChat" PURPLE "> " AQUA "%s says, \"" YELLOW "%s" AQUA "\"",
	  			tct(NAME(player), buf2), tct(arg, buf3) );
			break;
	}
	ansi_wall_wizards( buf );
}

void 
do_teleport(int descr, dbref player, const char *arg1, const char *arg2)
{
    dbref   victim;
    dbref   destination;
    const char *to;
    struct match_data md;

    if(Guest(player)) {
	anotify_fmt(player, CFAIL "%s", tp_noguest_mesg);
	return;
    }

    /* get victim, destination */
    if (*arg2 == '\0') {
	victim = player;
	to = arg1;
    } else {
	init_match(descr, player, arg1, NOTYPE, &md);
	match_neighbor(&md);
	match_possession(&md);
	match_me(&md);
	match_here(&md);
	match_absolute(&md);
	match_registered(&md);
	if (Wiz(OWNER(player)))
	    match_player(&md);

	if ((victim = noisy_match_result(&md)) == NOTHING) {
	    return;
	}
	to = arg2;
    }

    /* get destination */
    init_match(descr, player, to, TYPE_PLAYER, &md);
    match_neighbor(&md);
    match_possession(&md);
    match_me(&md);
    match_here(&md);
    match_home(&md);
    match_absolute(&md);
    match_registered(&md);
    if (Wiz(OWNER(player))) {
	match_player(&md);
    }
    switch (destination = match_result(&md)) {
	case NOTHING:
	    anotify_nolisten2(player, CINFO "Send it where?");
	    break;
	case AMBIGUOUS:
	    anotify_nolisten2(player, CINFO "I don't know where you mean!");
	    break;
	case HOME:
	    switch (Typeof(victim)) {
		case TYPE_PLAYER:
		    destination = DBFETCH(victim)->sp.player.home;
		    if (parent_loop_check(victim, destination))
			destination = DBFETCH(OWNER(victim))->sp.player.home;
		    break;
		case TYPE_THING:
		    destination = DBFETCH(victim)->sp.thing.home;
		    if (parent_loop_check(victim, destination))
			destination = DBFETCH(OWNER(victim))->sp.player.home;
		    break;
		case TYPE_ROOM:
		    destination = GLOBAL_ENVIRONMENT;
		    break;
		case TYPE_PROGRAM:
		    destination = OWNER(victim);
		    break;
		default:
		    destination = tp_player_start;	/* caught in the next
							 * switch anyway */
		    break;
	    }
	default:
	    switch (Typeof(victim)) {
		case TYPE_PLAYER:
		    if (    !controls(player, victim) || 
			    ( (!controls(player, destination)) &&
			      (!(FLAGS(destination)&JUMP_OK)) &&
			      (destination!=DBFETCH(victim)->sp.player.home)
			    ) ||
			    ( (!controls(player, getloc(victim))) &&
			      (!(FLAGS(getloc(victim))&JUMP_OK)) &&
			      (getloc(victim)!=DBFETCH(victim)->sp.player.home)
			    ) ||
			    ( (Typeof(destination) == TYPE_THING) &&
			      !controls(player, getloc(destination))
			    )
		    ) {
			anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
			break;
		    }
		    if ( Typeof(destination) != TYPE_ROOM &&
			 Typeof(destination) != TYPE_PLAYER &&
			 Typeof(destination) != TYPE_THING) {
			anotify_nolisten2(player, CFAIL "Bad destination.");
			break;
		    }
                    if (!Wiz(victim) &&
                            (Typeof(destination) == TYPE_THING &&
                                !(FLAGS(destination) & VEHICLE))) {
                        anotify_nolisten2(player, CFAIL "Destination object is not a vehicle.");
                        break;
                    }
		    if (parent_loop_check(victim, destination)) {
			anotify_nolisten2(player, CFAIL "Objects can't contain themselves.");
			break;
		    }
                    if(Typeof(destination)==TYPE_PLAYER) {
                        destination = DBFETCH(destination)->location;
                    }
		    if((Typeof(destination)==TYPE_ROOM) && Guest(player) &&
#ifdef G_NEEDFLAG
					!(FLAG2(destination)&F2GUEST)) {
#else
					(FLAG2(destination)&F2GUEST)) {
#endif
			anotify_nolisten2(player, CFAIL "Guests aren't allowed there.");
			break;
		    }
		    anotify_nolisten2(victim, CNOTE "You feel a wrenching sensation...");
		    enter_room(descr, victim, destination, DBFETCH(victim)->location);
		    anotify_nolisten2(player, CSUCC "Teleported.");
		    break;
		case TYPE_THING:
		    if (parent_loop_check(victim, destination)) {
			anotify_nolisten2(player, CFAIL "You can't make a container contain itself!");
			break;
		    }
		case TYPE_PROGRAM:
		    if (Typeof(destination) != TYPE_ROOM
			    && Typeof(destination) != TYPE_PLAYER
			    && Typeof(destination) != TYPE_THING) {
			anotify_nolisten2(player, CFAIL "Bad destination.");
			break;
		    }
		    if (!((controls(player, destination) ||
			    can_link_to(player, NOTYPE, destination)) &&
			    (controls(player, victim) ||
			    controls(player, DBFETCH(victim)->location)))) {
			anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
			break;
		    }
		    /* check for non-sticky dropto */
		    if (Typeof(destination) == TYPE_ROOM
			    && DBFETCH(destination)->sp.room.dropto != NOTHING
			    && !(FLAGS(destination) & STICKY))
			destination = DBFETCH(destination)->sp.room.dropto;
		    moveto(victim, destination);
		    anotify_nolisten2(player, CSUCC "Teleported.");
		    break;
		case TYPE_ROOM:
		    if (Typeof(destination) != TYPE_ROOM) {
			anotify_nolisten2(player, CFAIL "Bad destination.");
			break;
		    }
		    if (!controls(player, victim)
			    || !can_link_to(player, NOTYPE, destination)
			    || victim == GLOBAL_ENVIRONMENT) {
			anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
			break;
		    }
		    if (parent_loop_check(victim, destination)) {
			anotify_nolisten2(player, CFAIL "Parent would create a loop.");
			break;
		    }
		    moveto(victim, destination);
		    anotify_nolisten2(player, CSUCC "Parent set.");
		    break;
		case TYPE_GARBAGE:
		    anotify_nolisten2(player, CFAIL "That is garbage.");
		    break;
		default:
		    anotify_nolisten2(player, CFAIL "You can't teleport that.");
		    break;
	    }
	    break;
    }
    return;
}

void 
do_force(int descr, dbref player, const char *what, char *command)
{
    dbref   victim, loc;
    struct match_data md;

    if (force_level) {
        anotify_nolisten2(player, CFAIL "Can't @force an @force.");
        return;
    }

    if (!tp_zombies && (!Wiz(player) || Typeof(player) != TYPE_PLAYER)) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }

    /* get victim */
    init_match(descr, player, what, NOTYPE, &md);
    match_neighbor(&md);
    match_possession(&md);
    match_me(&md);
    match_here(&md);
    match_absolute(&md);
    match_registered(&md);
    match_player(&md);

    if ((victim = noisy_match_result(&md)) == NOTHING) {
	return;
    }

    if (!tp_zombies && ((!Wiz(player) && QLevel(player) >= MLevel(victim)) || Typeof(player) != TYPE_PLAYER)) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }

    if (Typeof(victim) != TYPE_PLAYER && Typeof(victim) != TYPE_THING) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }

    if (Man(victim)) {
	anotify_nolisten2(player, CFAIL "You cannot force the man.");
	return;
    }

    if (!controls(player, victim)) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }

    if (!Wiz(player) && !(FLAGS(victim) & XFORCIBLE)) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }
    if (!Wiz(player) && !test_lock_false_default(descr,player,victim,"@/flk")) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }

    loc = getloc(victim);
    if (!Wiz(player) && Typeof(victim) == TYPE_THING && loc != NOTHING &&
	    (FLAGS(loc) & ZOMBIE) && Typeof(loc) == TYPE_ROOM) {
	anotify_nolisten2(player, CFAIL "It is in a no-puppet zone.");
	return;
    }

    if (!Wiz(OWNER(player)) && Typeof(victim) == TYPE_THING) {
	const char *ptr = NAME(victim);
	char objname[BUFFER_LEN], *ptr2;
	if ((FLAGS(player) & ZOMBIE)) {
	    anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	    return;
	}
	if (FLAGS(victim) & DARK) {
	    anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	    return;
	}
	for (ptr2 = objname; *ptr && !isspace(*ptr);)
	    *(ptr2++) = *(ptr++);
	*ptr2 = '\0';
	if (lookup_player(objname) != NOTHING) {
	    anotify_nolisten2(player, CFAIL "Puppets cannot have a player's name.");
	    return;
	}
    }

    /* log_status("FORC: %s(%d) by %s(%d): %s\n", NAME(victim),
	       victim, NAME(player), player, command);
    */
    /* force victim to do command */
    force_level++;
    process_command(dbref_first_descr(victim), victim, command);
    force_level--;
}

void 
do_stats(dbref player, const char *name)
{
    int     rooms = 0;
    int     exits = 0;
    int     things = 0;
    int     players = 0;
    int     programs = 0;
    int     garbage = 0;
    int     tgsize = 0;
    int     total = 0;
    int     altered = 0;
    int     oldobjs = 0;
#ifdef DISKBASED
    int     loaded = 0;
    int     changed = 0;
#endif
    int     currtime = (int)current_systime;
    int     tosize=0;
    int     tpsize=0;
    int     tpcnt=0;
    int     tocnt=0;
    dbref   i;
    dbref   owner;

	if (name != NULL && *name != '\0') {
	    if( !strcmp(name, "me") )
		owner = player;
	    else
		owner = lookup_player(name);
	    if (owner == NOTHING) {
		anotify_nolisten2(player, CINFO "Who?");
		return;
	    }
	    if (   (!Mage(OWNER(player)))
	        && (OWNER(player) != owner)) {
		anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
		return;
	    }
	} else owner = NOTHING;

	for (i = 0; i < db_top; i++) {

#ifdef DISKBASE
		if (((owner == NOTHING) || (OWNER(i) == owner)) &&
			DBFETCH(i)->propsmode != PROPS_UNLOADED) loaded++;
		if (((owner == NOTHING) || (OWNER(i) == owner)) &&
			DBFETCH(i)->propsmode == PROPS_CHANGED) changed++;
#endif

		/* count objects marked as changed. */
		if (((owner == NOTHING) || (OWNER(i) == owner)) && (FLAGS(i) & OBJECT_CHANGED))
		    altered++;

		/* if unused for 90 days, inc oldobj count */
		if (((owner == NOTHING) || (OWNER(i) == owner)) &&
		    (currtime - DBFETCH(i)->ts.lastused) > tp_aging_time)
		    oldobjs++;

		if ((owner == NOTHING) || (OWNER(i) == owner))
		  switch (Typeof(i)) {
		    case TYPE_ROOM:
			tocnt++, total++, rooms++;
			tosize += size_object(i,0);
			break;

		    case TYPE_EXIT:
			tocnt++, total++, exits++;
			tosize += size_object(i,0);
			break;

		    case TYPE_THING:
			tocnt++, total++, things++;
			tosize += size_object(i,0);
			break;

		    case TYPE_PLAYER:
			tocnt++, total++, players++;
			tosize += size_object(i,0);
			break;

		    case TYPE_PROGRAM:
			total++, programs++;
			
			if (DBFETCH(i)->sp.program.siz > 0) {
			    tpcnt++; tpsize += size_object(i,0);
			} else {
			    tocnt++; tosize += size_object(i,0);
			}
			break;
		}
		if ((owner == NOTHING) && Typeof(i) == TYPE_GARBAGE) {
		    total++; garbage++;
		    tgsize += size_object(i,0);
		}
	}

	anotify_fmt(player, YELLOW "%7d room%s        %7d exit%s        %7d thing%s",
		   rooms, (rooms == 1) ? " " : "s",
		   exits, (exits == 1) ? " " : "s",
		   things, (things == 1) ? " " : "s");

	anotify_fmt(player, YELLOW "%7d program%s     %7d player%s      %7d garbage",
		   programs, (programs == 1) ? " " : "s",
		   players, (players == 1) ? " " : "s",
		   garbage);

	anotify_fmt(player, BLUE
		   "%7d total object%s                     %7d old & unused",
		   total, (total == 1) ? " " : "s", oldobjs);

#ifdef DISKBASE
	if (Mage(OWNER(player))) {
	    anotify_fmt(player, WHITE
		   "%7d proploaded object%s                %7d propchanged object%s",
		   loaded, (loaded == 1) ? " " : "s",
		   changed, (changed == 1) ? "" : "s");

	}
#endif

/* #ifdef DELTADUMPS */
        {
            char buf[BUFFER_LEN];
            struct tm *time_tm;
            time_t lasttime=(time_t)get_property_value(0,"~sys/lastdumptime");

            time_tm = localtime(&lasttime);
#ifndef WIN32
            (void)format_time(buf, 40, "%a %b %e %T %Z\0", time_tm);
#else
            (void)format_time(buf, 40, "%a %b %e %T\0", time_tm);
#endif
            anotify_fmt(player, RED "%7d unsaved object%s     Last dump: %s",
	        altered, (altered == 1) ? "" : "s", buf);
        }
/* #endif */

	if( garbage > 0 )
	    anotify_fmt(player, NORMAL
		"%7d piece%s of garbage %s using %d bytes of RAM.",
		garbage, (garbage == 1) ? "" : "s",
		(garbage == 1) ? "is" : "are", tgsize );

	if( tpcnt > 0 )
	    anotify_fmt(player, PURPLE
		"%7d active program%s %s using %d bytes of RAM.",
		tpcnt, (tpcnt == 1) ? "" : "s",
		(tpcnt == 1) ? "is" : "are", tpsize );

	anotify_fmt(player, GREEN
	    "%7d %sobject%s %s using %d bytes of RAM.",
		tocnt, tpcnt ? "other " : "",
		(tocnt == 1) ? "" : "s",
		(tocnt == 1) ? "is" : "are", tosize );

}

void 
do_boot(dbref player, const char *name)
{
    dbref   victim;

    if ( Typeof(player) != TYPE_PLAYER ) return;

    if (!Mage(player)) {
	anotify_nolisten2(player, CFAIL "Only wizards can boot someone off.");
	return;
    }
    if ((victim = lookup_player(name)) == NOTHING) {
	anotify_nolisten2(player, CINFO "Who?");
	return;
    }
    if (Typeof(victim) != TYPE_PLAYER) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }

    if (Man(victim)) {
	anotify_nolisten2(player, CFAIL "You can't boot the man!");
	return;
    }
    if (!Man(player) && TMage(victim)) {
	anotify_nolisten2(player, CFAIL "You can't boot wizards.");
	return;
    }

	anotify_nolisten2(victim, BLUE "Shaaawing!  See ya!");
	if (boot_off(victim)) {
	    log_status("BOOT: %s(%d) by %s(%d)\n", NAME(victim),
		victim, NAME(player), player);
	    if (player != victim)
		anotify_fmt(player, CSUCC "You booted %s off!", PNAME(victim));
	} else
	    anotify_fmt(player, CFAIL "%s is not connected.", PNAME(victim));
}

void 
do_frob(int descr, dbref player, const char *name, const char *recip)
{
    dbref   victim;
    dbref   recipient;
    dbref   stuff;
    char    buf[BUFFER_LEN];

    if (!Arch(player) || Typeof(player) != TYPE_PLAYER) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }
    /* if(tp_db_readonly) {
	anotify_nolisten2(player, CFAIL DBRO_MESG);
	return;
    } */
    if ((victim = lookup_player(name)) == NOTHING) {
	anotify_nolisten2(player, CINFO "Who?");
	return;
    }
    if (Typeof(victim) != TYPE_PLAYER) {
	anotify_nolisten2(player, CFAIL "You can only frob players.");
	return;
    }
    if (get_property_class( victim, "@/precious" )) {
	anotify_nolisten2(player, CFAIL "That player is precious.");
	return;
    }
    if (TMage(victim)) {
	anotify_nolisten2(player, CFAIL "You can't frob a wizard.");
	return;
    }
    if (!*recip) {
	recipient = MAN;
    } else {
	if ((recipient = lookup_player(recip)) == NOTHING
		|| recipient == victim) {
	    anotify_nolisten2(player, CINFO "Give their stuff to who?");
	    return;
	}
    }

    /* we're ok, do it */
	send_contents(descr, player, HOME);
	for (stuff = 0; stuff < db_top; stuff++) {
	    if (OWNER(stuff) == victim) {
		switch (Typeof(stuff)) {
		    case TYPE_PROGRAM:
			dequeue_prog(stuff, 0);  /* dequeue player's progs */
			FLAGS(stuff) &= ~(ABODE | W1 | W2 | W3);
			SetMLevel(stuff,0);
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
	dequeue_prog(victim, 0);  /* dequeue progs that player's running */

	anotify_nolisten2(victim, BLUE "You have been frobbed!  Been nice knowing you.");
	anotify_fmt(player, CSUCC "You frob %s.", PNAME(victim));
	log_status("FROB: %s(%d) by %s(%d)\n", NAME(victim),
		   victim, NAME(player), player);

	boot_player_off_too(victim);
	delete_player(victim);
	/* reset name */
	sprintf(buf, "The soul of %s", PNAME(victim));
	free((void *) NAME(victim));
	NAME(victim) = alloc_string(buf);
	DBDIRTY(victim);
	FLAGS(victim) = (FLAGS(victim) & ~TYPE_MASK) | TYPE_THING;
	OWNER(victim) = player;	/* you get it */
	DBFETCH(victim)->sp.thing.value = 1;

	if(tp_recycle_frobs) recycle(descr, player, victim);
}

void 
do_purge(int descr, dbref player, const char *arg1, const char *arg2)
{
    dbref   victim, thing;
    int count=0;

    if(tp_db_readonly) return;

    if(Guest(player)) {
	anotify_fmt(player, CFAIL "%s", tp_noguest_mesg);
	return;
    }

    if( *arg1 == '\0' ) {
	notify(player, "Usage: @purge me=yourpassword");
	notify(player, "@purge deletes EVERYTHING you own.");
	notify(player, "Do this at your own risk!");
	notify(player, "You must now use your password to ensure that Joe Blow doesn't");
	notify(player, "delete all your prized treasures while you take a nature break.");
	return;
    }

    if (!strcmp( arg1, "me" ))
    	victim = player;
    else if ((victim = lookup_player(arg1)) == NOTHING) {
	anotify_nolisten2(player, CINFO "Who?");
	return;
    }
    if (
	!controls(player, victim) ||
	Typeof(player) != TYPE_PLAYER ||
    	Typeof(victim) != TYPE_PLAYER || 
	TMage(victim)
    ) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }

    if (get_property_class( victim, "@/precious" )) {
	anotify_nolisten2(player, CFAIL "That player is precious.");
	return;
    }

    if(
	(!DBFETCH(victim)->sp.player.password ||
	  strcmp(arg2, DBFETCH(victim)->sp.player.password) ) &&
	( strcmp( arg2, "yes" ) ||
	  !Arch(player) )
    ) {
	anotify_nolisten2(player, CFAIL "Wrong password.");
	return;
    }
	
    for (thing = 2; thing < db_top; thing++) if (victim == OWNER(thing))
    {
	switch (Typeof(thing)) {
	    case TYPE_GARBAGE:
		anotify_fmt(player, CFAIL "Player owns garbage object #%d.", thing );
	    case TYPE_PLAYER:
		break;
	    case TYPE_ROOM:
		if (thing == tp_player_start || thing == GLOBAL_ENVIRONMENT)
		{
			anotify_nolisten2(player, CFAIL
				"Cannot recycle player start or global environment.");
			break;
		}
	    case TYPE_THING:
	    case TYPE_EXIT:
	    case TYPE_PROGRAM:
		recycle(descr, player, thing);
		count++;
		break;
	    default:
		anotify_fmt(player, CFAIL "Unknown object type for #%d.", thing );
	}
    }
    anotify_fmt(player, CSUCC "%d objects purged.", count);
}

void 
do_newpassword(dbref player, const char *name, const char *password)
{
    dbref   victim;

    if (!Arch(player) || Typeof(player) != TYPE_PLAYER) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    } else if ((victim = lookup_player(name)) == NOTHING) {
	anotify_nolisten2(player, CINFO "Who?");
    } else if (*password != '\0' && !ok_password(password)) {
	/* Wiz can set null passwords, but not bad passwords */
	anotify_nolisten2(player, CFAIL "Poor password.");

    } else if (Boy(victim)) {
	anotify_nolisten2(player, CFAIL "You can't change a MAN's nor a BOY's password!");
	return;
    } else {
	if (TMage(victim) && !Boy(player)) {
	    anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	    return;
	}

	/* it's ok, do it */
	if (DBFETCH(victim)->sp.player.password)
	    free((void *) DBFETCH(victim)->sp.player.password);
	DBSTORE(victim, sp.player.password, alloc_string(password));
	anotify_nolisten2(player, CSUCC "Password changed.");
	anotify_fmt(victim, CNOTE
		"Your password has been changed by %s.", NAME(player));
	log_status("NPAS: %s(%d) by %s(%d)\n", NAME(victim), (int) victim,
		   NAME(player), (int) player);
    }
}

void
do_pcreate(dbref player, const char *user, const char *password)
{
    dbref   newguy;

    if (!Arch(player) || Typeof(player) != TYPE_PLAYER) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }
    newguy = create_player(user, password);
    if (newguy == NOTHING) {
	anotify_nolisten2(player, CFAIL "Create failed.");
    } else {
	log_status("PCRE: %s(%d) by %s(%d)\n",
		   NAME(newguy), (int) newguy, NAME(player), (int) player);
	anotify_fmt(player, CSUCC "Player %s(%d) created.", user, newguy);
    }
}



#ifdef DISKBASE
extern int propcache_hits;
extern int propcache_misses;
#endif

void
do_serverdebug(int descr, dbref player, const char *arg1, const char *arg2)
{
    if (!Mage(OWNER(player))) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }

#ifdef DISKBASE
    if (!*arg1 || string_prefix(arg1, "cache")) {
	anotify_nolisten2(player, CINFO "Cache info:");
	diskbase_debug(player);
    }
#endif

    anotify_nolisten2(player, CINFO "Done.");
}


#ifndef NO_USAGE_COMMAND
int max_open_files(void); 	/* from interface.c */

void 
do_usage(dbref player)
{
    int     pid;
#ifdef HAVE_GETRUSAGE
    struct rusage usage;
    int psize;
#endif

    if (!Mage(OWNER(player))) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }
    pid = getpid();
#ifdef HAVE_GETRUSAGE
    psize = getpagesize();
    getrusage(RUSAGE_SELF, &usage);
#endif

    notify_fmt(player, "Compiled on: %s", UNAME_VALUE);
    notify_fmt(player, "Process ID: %d", pid);
    notify_fmt(player, "Max descriptors/process: %ld", max_open_files());
#ifdef HAVE_GETRUSAGE
    notify_fmt(player, "Performed %d input servicings.", usage.ru_inblock);
    notify_fmt(player, "Performed %d output servicings.", usage.ru_oublock);
    notify_fmt(player, "Sent %d messages over a socket.", usage.ru_msgsnd);
    notify_fmt(player, "Received %d messages over a socket.", usage.ru_msgrcv);
    notify_fmt(player, "Received %d signals.", usage.ru_nsignals);
    notify_fmt(player, "Page faults NOT requiring physical I/O: %d",
	       usage.ru_minflt);
    notify_fmt(player, "Page faults REQUIRING physical I/O: %d",
	       usage.ru_majflt);
    notify_fmt(player, "Swapped out of main memory %d times.", usage.ru_nswap);
    notify_fmt(player, "Voluntarily context switched %d times.",
	       usage.ru_nvcsw);
    notify_fmt(player, "Involuntarily context switched %d times.",
	       usage.ru_nivcsw);
    notify_fmt(player, "User time used: %d sec.", usage.ru_utime.tv_sec);
    notify_fmt(player, "System time used: %d sec.", usage.ru_stime.tv_sec);
    notify_fmt(player, "Pagesize for this machine: %d", psize);
    notify_fmt(player, "Maximum resident memory: %dk",
	       (usage.ru_maxrss * (psize / 1024)));
    notify_fmt(player, "Integral resident memory: %dk",
	       (usage.ru_idrss * (psize / 1024)));
#endif /* HAVE_GETRUSAGE */
}

#endif				/* NO_USAGE_COMMAND */



void
do_muf_topprofs(dbref player, char *arg1)
{
    struct profnode {
        struct profnode *next;
        dbref  prog;
        double proftime;
        double pcnt;
        long   comptime;
        long   usecount;
    } *tops = NULL;
    struct profnode *curr = NULL;
    int nodecount = 0;
    char buf[BUFFER_LEN];
    dbref i = NOTHING;
    int count = atoi(arg1);

    if (!Mage(OWNER(player))) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }
    if (!string_compare(arg1, "reset")) {
        for (i = db_top; i-->0; ) {
	    if (Typeof(i) == TYPE_PROGRAM && DBFETCH(i)->sp.program.code) {
		struct inst *first = DBFETCH(i)->sp.program.code;
		first->data.number = 0;
		first[1].data.number = 0;
		first[2].data.number = current_systime;
		first[3].data.number = 0;
	    }
        }
        anotify_nolisten2(player, CSUCC "MUF profiling statistics cleared.");
        return;
    }
    if (count < 0) {
	anotify_nolisten2(player, CFAIL "Count has to be a positive number.");
	return;
    } else if (count == 0) {
        count = 10;
    }

    for (i = db_top; i-->0; ) {
        if (Typeof(i) == TYPE_PROGRAM && DBFETCH(i)->sp.program.code) {
            struct inst *first = DBFETCH(i)->sp.program.code;
            struct profnode *newnode = (struct profnode *)malloc(sizeof(struct profnode));
	    newnode->next = NULL;
            newnode->prog = i;
            newnode->proftime = first->data.number;
            newnode->proftime += (first[1].data.number / 1000000.0);
            newnode->comptime = current_systime - first[2].data.number;
            newnode->usecount = first[3].data.number;
            if (newnode->comptime > 0) {
		newnode->pcnt = 100.0 * newnode->proftime / newnode->comptime;
            } else {
		newnode->pcnt =  0.0;
            }
	    if (!tops) {
		tops = newnode;
		nodecount++;
	    } else if (newnode->pcnt < tops->pcnt) {
		if (nodecount < count) {
		    newnode->next = tops;
		    tops = newnode;
		    nodecount++;
		} else {
		    free(newnode);
		}
	    } else {
	        if (nodecount >= count) {
	            curr = tops;
	            tops = tops->next;
	            free(curr);
	        } else {
		    nodecount++;
		}
		if (!tops) {
		    tops = newnode;
		} else if (newnode->pcnt < tops->pcnt) {
		    newnode->next = tops;
		    tops = newnode;
		} else {
		    for (curr = tops; curr->next; curr = curr->next) {
			if (newnode->pcnt < curr->next->pcnt) {
			    break;
			}
		    }
		    newnode->next = curr->next;
		    curr->next = newnode;
		}
	    }
        }
    }
    anotify_nolisten2(player, YELLOW "     %CPU   TotalTime  UseCount  Program");
    while (tops) {
        curr = tops;
        sprintf(buf, "%10.3f %10.3f %9d %s", curr->pcnt, curr->proftime, curr->usecount, unparse_object(player, curr->prog));
        notify(player, buf);
        tops = tops->next;
        free(curr);
    }
    sprintf(buf, WHITE "Profile Length (sec): " NORMAL "%5ld  " WHITE "%%idle: " NORMAL "%5.2f%%  " WHITE "Total Cycles: " NORMAL "%5lu",
	    (current_systime-sel_prof_start_time),
            ((double)(sel_prof_idle_sec+(sel_prof_idle_usec/1000000.0))*100.0)/
	    (double)((current_systime-sel_prof_start_time)+0.01),
	    sel_prof_idle_use);
    anotify_nolisten2(player,buf);
    anotify_nolisten2(player, YELLOW "Done.");
}

void
do_mpi_topprofs(dbref player, char *arg1)
{
    struct profnode {
        struct profnode *next;
        dbref  prog;
        double proftime;
        double pcnt;
        long   comptime;
        long   usecount;
    } *tops = NULL;
    struct profnode *curr = NULL;
    int nodecount = 0;
    char buf[BUFFER_LEN];
    dbref i = NOTHING;
    int count = atoi(arg1);

    if (!Mage(OWNER(player))) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }
    if (!string_compare(arg1, "reset")) {
        for (i = db_top; i-->0; ) {
	    if (DBFETCH(i)->mpi_prof_use) {
	      DBFETCH(i)->mpi_prof_use = 0;
	      DBFETCH(i)->mpi_prof_usec = 0;
	      DBFETCH(i)->mpi_prof_sec = 0;
	    }
        }
	mpi_prof_start_time = current_systime;
        anotify_nolisten2(player, CSUCC "MPI profiling statistics cleared.");
        return;
    }
    if (count < 0) {
	anotify_nolisten2(player, CFAIL "Count has to be a positive number.");
	return;
    } else if (count == 0) {
        count = 10;
    }

    for (i = db_top; i-->0; ) {
        if (DBFETCH(i)->mpi_prof_use) {
            struct profnode *newnode = (struct profnode *)malloc(sizeof(struct profnode));
	    newnode->next = NULL;
            newnode->prog = i;
            newnode->proftime = DBFETCH(i)->mpi_prof_sec;
            newnode->proftime += (DBFETCH(i)->mpi_prof_usec / 1000000.0);
            newnode->comptime = current_systime - mpi_prof_start_time;
            newnode->usecount = DBFETCH(i)->mpi_prof_use;
            if (newnode->comptime > 0) {
		newnode->pcnt = 100.0 * newnode->proftime / newnode->comptime;
            } else {
		newnode->pcnt =  0.0;
            }
	    if (!tops) {
		tops = newnode;
		nodecount++;
	    } else if (newnode->pcnt < tops->pcnt) {
		if (nodecount < count) {
		    newnode->next = tops;
		    tops = newnode;
		    nodecount++;
		} else {
		    free(newnode);
		}
	    } else {
	        if (nodecount >= count) {
	            curr = tops;
	            tops = tops->next;
	            free(curr);
	        } else {
		    nodecount++;
		}
		if (!tops) {
		    tops = newnode;
		} else if (newnode->pcnt < tops->pcnt) {
		    newnode->next = tops;
		    tops = newnode;
		} else {
		    for (curr = tops; curr->next; curr = curr->next) {
			if (newnode->pcnt < curr->next->pcnt) {
			    break;
			}
		    }
		    newnode->next = curr->next;
		    curr->next = newnode;
		}
	    }
        }
    }
    anotify_nolisten2(player, YELLOW "     %CPU   TotalTime  UseCount  Object");
    while (tops) {
        curr = tops;
        sprintf(buf, "%10.3f %10.3f %9d %s", curr->pcnt, curr->proftime, curr->usecount, unparse_object(player, curr->prog));
        notify(player, buf);
        tops = tops->next;
        free(curr);
    }
    sprintf(buf, WHITE "Profile Length (sec): " NORMAL "%5ld  " WHITE "%%idle: " NORMAL "%5.2f%%  " WHITE "Total Cycles: " NORMAL "%5lu",
	    (current_systime-sel_prof_start_time),
            (((double)sel_prof_idle_sec+(sel_prof_idle_usec/1000000.0))*100.0)/
	    (double)((current_systime-sel_prof_start_time)+0.01),
	    sel_prof_idle_use);
    anotify_nolisten2(player,buf);
    anotify_nolisten2(player, YELLOW "Done.");
}

void
do_all_topprofs(dbref player, char *arg1)
{
    struct profnode {
        struct profnode *next;
        dbref  prog;
        double proftime;
        double pcnt;
        long   comptime;
        long   usecount;
        short  type;
    } *tops = NULL;
    struct profnode *curr = NULL;
    int nodecount = 0;
    char buf[BUFFER_LEN];
    dbref i = NOTHING;
    int count = atoi(arg1);

    if (!Mage(OWNER(player))) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }
    if (!string_compare(arg1, "reset")) {
        for (i = db_top; i-->0; ) {
	    if (DBFETCH(i)->mpi_prof_use) {
	      DBFETCH(i)->mpi_prof_use = 0;
	      DBFETCH(i)->mpi_prof_usec = 0;
	      DBFETCH(i)->mpi_prof_sec = 0;
	    }
	    if (Typeof(i) == TYPE_PROGRAM && DBFETCH(i)->sp.program.code) {
                struct inst *first = DBFETCH(i)->sp.program.code;
                first->data.number = 0;
                first[1].data.number = 0;
                first[2].data.number = current_systime;
                first[3].data.number = 0;
            }
        }
	sel_prof_idle_sec = 0;
	sel_prof_idle_usec = 0;
	sel_prof_start_time = current_systime;
	sel_prof_idle_use = 0;
	mpi_prof_start_time = current_systime;
        anotify_nolisten2(player, CSUCC "All profiling statistics cleared.");
        return;
    }
    if (count < 0) {
	anotify_nolisten2(player, CFAIL "Count has to be a positive number.");
	return;
    } else if (count == 0) {
        count = 10;
    }

    for (i = db_top; i-->0; ) {
        if (DBFETCH(i)->mpi_prof_use) {
            struct profnode *newnode = (struct profnode *)malloc(sizeof(struct profnode));
	    newnode->next = NULL;
            newnode->prog = i;
            newnode->proftime = DBFETCH(i)->mpi_prof_sec;
            newnode->proftime += (DBFETCH(i)->mpi_prof_usec / 1000000.0);
            newnode->comptime = current_systime - mpi_prof_start_time;
            newnode->usecount = DBFETCH(i)->mpi_prof_use;
	    newnode->type = 0;
            if (newnode->comptime > 0) {
		newnode->pcnt = 100.0 * newnode->proftime / newnode->comptime;
            } else {
		newnode->pcnt =  0.0;
            }
	    if (!tops) {
		tops = newnode;
		nodecount++;
	    } else if (newnode->pcnt < tops->pcnt) {
		if (nodecount < count) {
		    newnode->next = tops;
		    tops = newnode;
		    nodecount++;
		} else {
		    free(newnode);
		}
	    } else {
	        if (nodecount >= count) {
	            curr = tops;
	            tops = tops->next;
	            free(curr);
	        } else {
		    nodecount++;
		}
		if (!tops) {
		    tops = newnode;
		} else if (newnode->pcnt < tops->pcnt) {
		    newnode->next = tops;
		    tops = newnode;
		} else {
		    for (curr = tops; curr->next; curr = curr->next) {
			if (newnode->pcnt < curr->next->pcnt) {
			    break;
			}
		    }
		    newnode->next = curr->next;
		    curr->next = newnode;
		}
	    }
        }
        if (Typeof(i) == TYPE_PROGRAM && DBFETCH(i)->sp.program.code) {
            struct inst *first = DBFETCH(i)->sp.program.code;
            struct profnode *newnode = (struct profnode *)malloc(sizeof(struct profnode));
            newnode->next = NULL;
            newnode->prog = i;
            newnode->proftime = first->data.number;
            newnode->proftime += (first[1].data.number / 1000000.0);
            newnode->comptime = current_systime - first[2].data.number;
            newnode->usecount = first[3].data.number;
	    newnode->type = 1;
            if (newnode->comptime > 0) {
                newnode->pcnt = 100.0 * newnode->proftime / newnode->comptime;
            } else {
                newnode->pcnt =  0.0;
            }
            if (!tops) {
                tops = newnode;
                nodecount++;
            } else if (newnode->pcnt < tops->pcnt) {
                if (nodecount < count) {
                    newnode->next = tops;
                    tops = newnode;
                    nodecount++;
                } else {
                    free(newnode);
                }
            } else {
                if (nodecount >= count) {
                    curr = tops;
                    tops = tops->next;
                    free(curr);
                } else {
                    nodecount++;
                }
                if (!tops) {
                    tops = newnode;
                } else if (newnode->pcnt < tops->pcnt) {
                    newnode->next = tops;
                    tops = newnode;
                } else {
                    for (curr = tops; curr->next; curr = curr->next) {
                        if (newnode->pcnt < curr->next->pcnt) {
                            break;
                        }
                    }
                    newnode->next = curr->next;
                    curr->next = newnode;
                }
            }
        }
    }
    anotify_nolisten2(player, YELLOW "     %CPU   TotalTime  UseCount  Type  Object");
    while (tops) {
        curr = tops;
        sprintf(buf, "%10.3f %10.3f %9d%5s   %s", curr->pcnt, curr->proftime, curr->usecount, curr->type?"MUF":"MPI",unparse_object(player, curr->prog));
        notify(player, buf);
        tops = tops->next;
        free(curr);
    }
    sprintf(buf, WHITE "Profile Length (sec): " NORMAL "%5ld  " WHITE "%%idle: " NORMAL "%5.2f%%  " WHITE "Total Cycles: " NORMAL "%5lu",
	    (current_systime-sel_prof_start_time),
            ((double)(sel_prof_idle_sec+(sel_prof_idle_usec/1000000.0))*100.0)/
	    (double)((current_systime-sel_prof_start_time)+0.01),
	    sel_prof_idle_use);
    anotify_nolisten2(player,buf);
    anotify_nolisten2(player, YELLOW "Done.");
}

void 
do_memory(dbref who)
{
    if (!Mage(OWNER(who))) {
	anotify_fmt(who, CFAIL "%s", tp_noperm_mesg);
	return;
    }

#ifndef NO_MEMORY_COMMAND
# ifdef HAVE_MALLINFO
    {
	struct mallinfo mi;

	mi = mallinfo();
	notify_fmt(who,"Total memory arena size: %6dk", (mi.arena / 1024));
	notify_fmt(who,"Small block mem alloced: %6dk", (mi.usmblks / 1024));
	notify_fmt(who,"Small block memory free: %6dk", (mi.fsmblks / 1024));
	notify_fmt(who,"Small block mem overhead:%6dk", (mi.hblkhd / 1024));

	notify_fmt(who,"Memory allocated:        %6dk", (mi.uordblks / 1024));
	notify_fmt(who,"Mem allocated overhead:  %6dk", 
				    ((mi.uordbytes - mi.uordblks) / 1024));
	notify_fmt(who,"Memory free:             %6dk", (mi.fordblks / 1024));
	notify_fmt(who,"Memory free overhead:    %6dk",(mi.treeoverhead/1024));

	notify_fmt(who, "Small block grain: %6d", mi.grain);
	notify_fmt(who, "Small block count: %6d", mi.smblks);
	notify_fmt(who, "Memory chunks:     %6d", mi.ordblks);
	notify_fmt(who, "Mem chunks alloced:%6d", mi.allocated);
    }
# endif /* HAVE_MALLINFO */
#endif /* NO_MEMORY_COMMAND */

#ifdef MALLOC_PROFILING
    notify(who, "  ");
    CrT_summarize(who);
    CrT_summarize_to_file("malloc_log", "Manual Checkpoint");
#endif

    anotify_nolisten2(who, CINFO "Done.", 1);
}

void
do_fixw(dbref player, const char *msg)
{
    int i;

    if( !Boy(player) ) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }
    if( strcmp(msg, "Convert DB to new level system.") ) {
	anotify_nolisten2(player, CINFO "What's the magic phrase?");
	return;
    }
    if( RawMLevel(player) != LM3 ) {
	anotify_nolisten2(player, CFAIL "If you want to do @fixw, you must be set M3.");
    }
    for( i = 0; i < db_top; i++ ) {
	if(FLAGS(i) & W3)
	    SetMLevel(i, LWIZ);
	else if((FLAGS(i) & (W2)) && (FLAGS(i) & (W1)))
	    SetMLevel(i, LM3);
	else if(FLAGS(i) & (W1))
	    SetMLevel(i, LM2);
	else if(FLAGS(i) & (W2))
	    SetMLevel(i, LM1);
    }
    anotify_nolisten2(player, CINFO "Done.");
}




