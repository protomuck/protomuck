#include "copyright.h"
#include "config.h"

#include "interface.h"
#include "db.h"
#include "props.h"
#include "params.h"
#include "tune.h"
#include "match.h"
#include "externs.h"
#include "reg.h"
#include "maillib.h"
#include "strings.h"
#include "netresolve.h"

#define anotify_nolisten2(x, y) anotify_nolisten(x, y, 1);

void
get_inquotes(char *buf, char *retbuf, int which)
{
    int i;

    retbuf[0] = '\0';

    for (i = 0; i < (2 * (which - 1) + 1); i++, buf++)
        while (*buf != '\'')
            if (*buf == '\0')
                return;
            else
                buf++;

    while (*buf != '\'' && (*buf) != '\0')
        (*(retbuf++)) = (*(buf++));
    *retbuf = '\0';
}

void
strreplace(char *nw, const char *base, const char *orig, const char *replace)
{
    char xbuf[BUFFER_LEN];
    char buf[BUFFER_LEN];
    int i = 0, j = 0;
    int k;

    k = strlen(replace);

    bcopy(base, xbuf, strlen(base) + 1);

    buf[0] = '\0';

    while (xbuf[i]) {
        if (!strncmp(xbuf + i, orig, strlen(orig))) {
            if ((j + k + 1) < BUFFER_LEN) {
                strcat(buf, replace);
                i += strlen(orig);
                j += k;
            }
        } else {
            if ((j + 1) < BUFFER_LEN) {
                buf[j++] = xbuf[i++];
                buf[j] = '\0';
            }
        }
    }
    sprintf(nw, "%s", buf);
}


void
do_wizchat(dbref player, const char *arg)
{
    char buf[BUFFER_LEN], buf2[BUFFER_LEN], buf3[BUFFER_LEN];

    if (!Mage(OWNER(player))) {
        anotify_nolisten2(player, SYSRED NOPERM_MESG);
        return;
    }

    switch (arg[0]) {

        case '|':
            sprintf(buf, SYSBLUE "WizChat" SYSPURPLE "> " SYSAQUA "%s",
                    tct(arg + 1, buf2));
            break;

        case '#':
            sprintf(buf, SYSBLUE "WizChat" SYSPURPLE "> "
                    SYSAQUA "%s", tct(arg + 1, buf2));
            break;

        case ':':
        case ';':
            sprintf(buf, SYSBLUE "WizChat" SYSPURPLE "> " SYSAQUA
                    "%s %s", tct(NAME(player), buf2), tct(arg + 1, buf3));
            break;

        case '?':
            show_wizards(player);
            return;

        default:
            sprintf(buf, SYSBLUE "WizChat" SYSPURPLE "> " SYSAQUA
                    "%s says, \"" SYSYELLOW "%s" SYSAQUA "\"",
                    tct(NAME(player), buf2), tct(arg, buf3));
            break;
    }
    ansi_wall_wizards(buf);
}

void
do_teleport(int descr, dbref player, const char *arg1, const char *arg2)
{
    char buf[BUFFER_LEN];
    dbref victim;
    dbref destination;
    const char *to;
    struct match_data md;

    if (Guest(player)) {
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
        if (Wiz(OWNER(player)) || POWERS(OWNER(player)) & POW_TELEPORT)
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
    match_null(&md);
    if (Wiz(OWNER(player)) || POWERS(OWNER(player)) & POW_TELEPORT) {
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
	case NIL:
	    if (destination == HOME) {
            switch (Typeof(victim)) {
                case TYPE_PLAYER:
                    destination = DBFETCH(victim)->sp.player.home;
                    if (parent_loop_check(victim, destination))
                        destination = DBFETCH(OWNER(victim))->sp.player.home;
                    break;
                case TYPE_THING:
                    destination = DBFETCH(victim)->sp.thing.home;
                    if (parent_loop_check(victim, destination)) {
                        destination = DBFETCH(OWNER(victim))->sp.player.home;
                        if (parent_loop_check(victim, destination)) {
                            destination = (dbref) 0;
                        }
                    }
                    break;
                case TYPE_ROOM:
                    destination = GLOBAL_ENVIRONMENT;
                    break;
                case TYPE_PROGRAM:
                    destination = OWNER(victim);
                    break;
                default:
                    destination = tp_player_start; /* caught in the next
                                                    * switch anyway */
                    break;
            }
	    } else {
            switch (Typeof(victim)) {
                case TYPE_PLAYER:
                    destination = tp_player_start;
                    break;
                case TYPE_THING:
                    destination = OWNER(victim);
                    break;
                case TYPE_ROOM:
                    destination = tp_default_parent;
                    break;
                case TYPE_PROGRAM:
                    destination = OWNER(victim);
                    break;
            } }
        default:
            switch (Typeof(victim)) {
                case TYPE_PLAYER:
                    if (!controls(player, victim) ||
                        ((!controls(player, destination)) &&
                         (!(FLAGS(destination) & JUMP_OK)) &&
                         (destination != DBFETCH(victim)->sp.player.home)
                        ) ||
                        ((!controls(player, getloc(victim))) &&
                         (!(FLAGS(getloc(victim)) & JUMP_OK)) &&
                         (getloc(victim) != DBFETCH(victim)->sp.player.home)
                        ) ||
                        ((Typeof(destination) == TYPE_THING) &&
                         !controls(player, getloc(destination))
                        )
                        ) {
                        if (!(POWERS(OWNER(player)) & POW_TELEPORT)) {
                            anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
                            break;
                        }
                    }
                    if (Typeof(destination) != TYPE_ROOM &&
                        Typeof(destination) != TYPE_PLAYER &&
                        Typeof(destination) != TYPE_THING) {
                        anotify_nolisten2(player, CFAIL "Bad destination.");
                        break;
                    }
                    if (!Wiz(victim) &&
                        (Typeof(destination) == TYPE_THING &&
                         !(FLAGS(destination) & VEHICLE))) {
                        if (!(POWERS(OWNER(player)) & POW_TELEPORT)) {
                            anotify_nolisten2(player,
                                              CFAIL
                                              "Destination object is not a vehicle.");
                            break;
                        }
                    }
                    if (parent_loop_check(victim, destination)) {
                        anotify_nolisten2(player,
                                          CFAIL
                                          "Objects can't contain themselves.");
                        break;
                    }
                    if (Typeof(destination) == TYPE_PLAYER) {
                        destination = DBFETCH(destination)->location;
                    }
                    if ((Typeof(destination) == TYPE_ROOM) && Guest(player) &&
                        (tp_guest_needflag ? !(FLAG2(destination) & F2GUEST)
                         : (FLAG2(destination) & F2GUEST))) {
                        anotify_nolisten2(player,
                                          CFAIL "Guests aren't allowed there.");
                        break;
                    }
                    anotify_nolisten2(victim,
                                      CNOTE
                                      "You feel a wrenching sensation...");
                    enter_room(descr, victim, destination,
                               DBFETCH(victim)->location);
                    sprintf(buf, CSUCC "%s teleported to %s.",
                            unparse_object(player, victim), NAME(destination));
                    anotify_nolisten2(player, buf);
                    break;
                case TYPE_THING:
                    if (parent_loop_check(victim, destination)) {
                        anotify_nolisten2(player,
                                          CFAIL
                                          "You can't make a container contain itself!");
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
                        if (!(POWERS(OWNER(player)) & POW_TELEPORT)) {
                            anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
                            break;
                        }
                    }
                    /* check for non-sticky dropto */
                    if (Typeof(destination) == TYPE_ROOM
                        && DBFETCH(destination)->sp.room.dropto != NOTHING
                        && !(FLAGS(destination) & STICKY))
                        destination = DBFETCH(destination)->sp.room.dropto;
                    moveto(victim, destination);
                    sprintf(buf, CSUCC "%s teleported to %s.",
                            unparse_object(player, victim),
                            (destination == -3 ? "HOME" : NAME(destination))
                        );
                    anotify_nolisten2(player, buf);
                    break;
                case TYPE_ROOM:
                    if (Typeof(destination) != TYPE_ROOM) {
                        anotify_nolisten2(player, CFAIL "Bad destination.");
                        break;
                    }
                    if (!controls(player, victim)
                        || (!can_link_to(player, NOTYPE, destination)
                            && !(FLAG2(destination) & F2PARENT))
                        || victim == GLOBAL_ENVIRONMENT) {
                        if (!(POWERS(OWNER(player)) & POW_TELEPORT)) {
                            anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
                            break;
                        }
                    }
                    if (parent_loop_check(victim, destination)) {
                        anotify_nolisten2(player,
                                          CFAIL "Parent would create a loop.");
                        break;
                    }
                    moveto(victim, destination);
                    sprintf(buf, CSUCC "Parent of %s set to %s.",
                            unparse_object(player, victim), NAME(destination));
                    anotify_nolisten2(player, buf);
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
    dbref victim, loc;
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

    if (!tp_zombies && ((!Wiz(player) && QLevel(player) >= MLevel(victim))
                        || Typeof(player) != TYPE_PLAYER)) {
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
    if (MLevel(player) < MLevel(victim)) {
        anotify_nolisten2(player,
                          CFAIL "You cannot force higher level wizards.");
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
    if (!Wiz(player)
        && !test_lock_false_default(descr, player, victim, "@/flk")) {
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
            anotify_nolisten2(player,
                              CFAIL "Puppets cannot have a player's name.");
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
    int rooms = 0;
    int exits = 0;
    int things = 0;
    int players = 0;
    int programs = 0;
    int garbage = 0;
    int tgsize = 0;
    int total = 0;
    int altered = 0;
    int oldobjs = 0;

#ifdef DISKBASE
    int loaded = 0;
    int changed = 0;
#endif
    int currtime = (int) current_systime;
    int tosize = 0;
    int tpsize = 0;
    int tpcnt = 0;
    int tocnt = 0;
    int memtemp = 0;
    int memtemp2 = 0;
    dbref i;
    dbref owner = NOTHING;

    if (name != NULL && *name != '\0') {
        if (!strcmp(name, "me"))
            owner = player;
        else
            owner = lookup_player(name);
        if (owner == NOTHING) {
            anotify_nolisten2(player, CINFO "Who?");
            return;
        }
        if ((!Mage(OWNER(player)))
            && (OWNER(player) != owner)) {
            anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
            return;
        }
    } else
        owner = NOTHING;

    for (i = 0; i < db_top; i++) {

#ifdef DISKBASE
        if (((owner == NOTHING) || (OWNER(i) == owner)) &&
            DBFETCH(i)->propsmode != PROPS_UNLOADED)
            loaded++;
        if (((owner == NOTHING) || (OWNER(i) == owner)) &&
            DBFETCH(i)->propsmode == PROPS_CHANGED)
            changed++;
#endif

        /* count objects marked as changed. */
        if (((owner == NOTHING) || (OWNER(i) == owner))
            && (FLAGS(i) & OBJECT_CHANGED))
            altered++;

        /* if unused for 90 days, inc oldobj count */
        if (((owner == NOTHING) || (OWNER(i) == owner)) &&
            (currtime - DBFETCH(i)->ts.lastused) > tp_aging_time)
            oldobjs++;

        if ((owner == NOTHING) || (OWNER(i) == owner))
            switch (Typeof(i)) {
                case TYPE_ROOM:
                    tocnt++, total++, rooms++;
                    tosize += size_object(i, 0);
                    break;

                case TYPE_EXIT:
                    tocnt++, total++, exits++;
                    tosize += size_object(i, 0);
                    break;

                case TYPE_THING:
                    tocnt++, total++, things++;
                    tosize += size_object(i, 0);
                    break;

                case TYPE_PLAYER:
                    tocnt++, total++, players++;
                    tosize += size_object(i, 0);
                    break;

                case TYPE_PROGRAM:
                    total++, programs++;

                    if (DBFETCH(i)->sp.program.siz > 0) {
                        tpcnt++;
                        tpsize += size_object(i, 0);
                    } else {
                        tocnt++;
                        tosize += size_object(i, 0);
                    }
                    break;
            }
        if ((owner == NOTHING) && Typeof(i) == TYPE_GARBAGE) {
            total++;
            garbage++;
            tgsize += size_object(i, 0);
        }
    }

    anotify_fmt(player,
                SYSYELLOW "%7d room%s        %7d exit%s        %7d thing%s",
                rooms, (rooms == 1) ? " " : "s", exits,
                (exits == 1) ? " " : "s", things, (things == 1) ? " " : "s");

    anotify_fmt(player,
                SYSYELLOW "%7d program%s     %7d player%s      %7d garbage",
                programs, (programs == 1) ? " " : "s", players,
                (players == 1) ? " " : "s", garbage);

    anotify_fmt(player, SYSBLUE
                "%7d total object%s                     %7d old & unused",
                total, (total == 1) ? " " : "s", oldobjs);

#ifdef DISKBASE
    if (Mage(OWNER(player))) {
        anotify_fmt(player, SYSWHITE
                    "%7d proploaded object%s                %7d propchanged object%s",
                    loaded, (loaded == 1) ? " " : "s",
                    changed, (changed == 1) ? "" : "s");

    }
#endif

/* #ifdef DELTADUMPS */
    {
        char buf[BUFFER_LEN];
        struct tm *time_tm;
        time_t lasttime = (time_t) get_property_value(0, "~sys/lastdumptime");

        time_tm = localtime(&lasttime);
#ifndef WIN32
        (void) format_time(buf, 40, "%a %b %e %T %Z\0", time_tm);
#else
        (void) format_time(buf, 40, "%a %b %e %T\0", time_tm);
#endif
        anotify_fmt(player, SYSRED "%7d unsaved object%s     Last dump: %s",
                    altered, (altered == 1) ? "" : "s", buf);
    }
/* #endif */

    if (garbage > 0)
        anotify_fmt(player, SYSNORMAL
                    "%7d piece%s of garbage %s using %d bytes of RAM.",
                    garbage, (garbage == 1) ? "" : "s",
                    (garbage == 1) ? "is" : "are", tgsize);

    if (tpcnt > 0)
        anotify_fmt(player, SYSPURPLE
                    "%7d active program%s %s using %d bytes of RAM.",
                    tpcnt, (tpcnt == 1) ? "" : "s",
                    (tpcnt == 1) ? "is" : "are", tpsize);

    anotify_fmt(player, SYSGREEN
                "%7d %sobject%s %s using %d bytes of RAM.",
                tocnt, tpcnt ? "other " : "",
                (tocnt == 1) ? "" : "s", (tocnt == 1) ? "is" : "are", tosize);
    anotify_fmt(player, SYSYELLOW
                "%7ld cached host %s using %ld bytes of RAM.",
                hostdb_count, (hostdb_count == 1L) ? "entry is" : "entries are",
                host_hostdb_size() + host_userdb_size());
    /* Bandwidth Usage Stats */
    if (Mage(OWNER(player))) {
        memtemp = bytesIn / 1000;
        memtemp2 = bytesOut / 1000;
        anotify_fmt(player, SYSBROWN
                    "%7d %s input             " SYSAQUA "%7d %s output",
                    memtemp < 100000 ? memtemp : memtemp / 1000,
                    memtemp < 100000 ? "Kbytes" : "Mbytes",
                    memtemp2 < 100000 ? memtemp2 : memtemp2 / 1000,
                    memtemp2 < 100000 ? "Kbytes" : "Mbytes");
        memtemp = bytesIn + bytesOut;
        memtemp = memtemp / 1000;
        anotify_fmt(player, SYSFOREST
                    "%7d %s total traffic     " SYSVIOLET "%7d %s",
                    memtemp < 100000 ? memtemp : memtemp / 1000,
                    memtemp < 100000 ? "KBytes" : "Mbytes",
                    commandTotal < 100000 ? commandTotal : commandTotal / 1000,
                    commandTotal < 100000 ? "commands" : "thousand commands");
    }

}

void
do_boot(dbref player, const char *name)
{
    dbref victim;

    if (Typeof(player) != TYPE_PLAYER)
        return;

    if (!Mage(player) && !(POWERS(player) & POW_BOOT)) {
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
    if (Boy(victim) && !Man(player)) {
        anotify_nolisten2(player, CFAIL "Only the man can boot boys.");
        return;
    }
    if (!Boy(player) && TMage(victim)) {
        anotify_nolisten2(player, CFAIL "You can't boot wizards.");
        return;
    }

    anotify_nolisten2(victim, SYSBLUE "Shaaawing!  See ya!");
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
    dbref victim;
    dbref recipient;
    dbref stuff;
    char buf[BUFFER_LEN];

    if (!
        (Typeof(player) == TYPE_PLAYER
         && (Arch(player) || (POWERS(player) & POW_PLAYER_PURGE)))) {
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
        return;
    }
    /* Why is this disabled? -Hinoserm */
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
    if (get_property_class(victim, "@/precious")) {
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
    send_contents(descr, victim, HOME);
    for (stuff = 0; stuff < db_top; stuff++) {
        if (OWNER(stuff) == victim) {
            switch (Typeof(stuff)) {
                case TYPE_PROGRAM:
                    dequeue_prog(stuff, 0); /* dequeue player's progs */
                    FLAGS(stuff) &= ~(ABODE | W1 | W2 | W3);
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
    dequeue_prog(victim, 0);    /* dequeue progs that player's running */

    anotify_nolisten2(victim,
                      SYSBLUE "You have been frobbed!  Been nice knowing you.");
    anotify_fmt(player, CSUCC "You frob %s.", PNAME(victim));
    log_status("FROB: %s(%d) by %s(%d)\n", NAME(victim),
               victim, NAME(player), player);
    if (Typeof(victim) != TYPE_PLAYER)
        return;
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
    OWNER(victim) = player;     /* you get it */
    DBFETCH(victim)->sp.thing.value = 1;

    if (tp_recycle_frobs)
        recycle(descr, player, victim);
}

void
do_purge(int descr, dbref player, const char *arg1, const char *arg2)
{
    dbref victim, thing;
    int count = 0;

    if (tp_db_readonly)
        return;

    if (Guest(player)) {
        anotify_fmt(player, CFAIL "%s", tp_noguest_mesg);
        return;
    }

    if (*arg1 == '\0') {
        notify(player, "Usage: @purge me=yourpassword");
        notify(player, "@purge deletes EVERYTHING you own.");
        notify(player, "Do this at your own risk!");
        notify(player,
               "You must now use your password to ensure that Joe Blow doesn't");
        notify(player,
               "delete all your prized treasures while you take a nature break.");
        return;
    }

    if (!strcmp(arg1, "me"))
        victim = player;
    else if ((victim = lookup_player(arg1)) == NOTHING) {
        anotify_nolisten2(player, CINFO "Who?");
        return;
    }
    if ((!controls(player, victim) && !(POWERS(player) & POW_PLAYER_PURGE)) ||
        Typeof(player) != TYPE_PLAYER ||
        Typeof(victim) != TYPE_PLAYER || TMage(victim)
        ) {
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
        return;
    }

    if (get_property_class(victim, "@/precious")) {
        anotify_nolisten2(player, CFAIL "That player is precious.");
        return;
    }

    if ((!DBFETCH(victim)->sp.player.password ||
         check_password(victim, arg2)) &&
        (strcmp(arg2, "yes") ||
         !(Arch(player) || POWERS(player) & POW_PLAYER_PURGE))
        ) {
        anotify_nolisten2(player, CFAIL "Wrong password.");
        return;
    }

    for (thing = 2; thing < db_top; thing++)
        if (victim == OWNER(thing)) {
            switch (Typeof(thing)) {
                case TYPE_GARBAGE:
                    anotify_fmt(player, CFAIL "Player owns garbage object #%d.",
                                thing);
                case TYPE_PLAYER:
                    break;
                case TYPE_ROOM:
                    if (thing == tp_player_start || thing == GLOBAL_ENVIRONMENT) {
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
                    anotify_fmt(player, CFAIL "Unknown object type for #%d.",
                                thing);
            }
        }
    anotify_fmt(player, CSUCC "%d objects purged.", count);
}

void
do_newpassword(dbref player, const char *name, const char *password)
{
    dbref victim;

    if ((!Arch(player) && !(POWERS(player) & POW_PLAYER_CREATE))
        || Typeof(player) != TYPE_PLAYER) {
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
        return;
    } else if ((victim = lookup_player(name)) == NOTHING) {
        anotify_nolisten2(player, CINFO "Who?");
    } else if (*password != '\0' && !ok_password(password)) {
        /* Wiz can set null passwords, but not bad passwords */
        anotify_nolisten2(player, CFAIL "Poor password.");

    } else if (Boy(victim)) {
        anotify_nolisten2(player,
                          CFAIL
                          "You can't change a MAN's nor a BOY's password!");
        return;
    } else {
        if (TMage(victim) && !Boy(player)) {
            anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
            return;
        }

        /* it's ok, do it */
        set_password(victim, password);
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
    dbref newguy;

    if (!
        (Typeof(player) == TYPE_PLAYER
         && (Arch(player) || (POWERS(player) & POW_PLAYER_CREATE)))) {
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
        return;
    }
    newguy = create_player(player, user, password);
    if (newguy == NOTHING) {
        anotify_nolisten2(player, CFAIL "Create failed.");
        anotify_nolisten2(player, BOLD "Syntax: @pcreate <name>=<password>");
    } else {
        log_status("PCRE: %s(%d) by %s(%d)\n",
                   NAME(newguy), (int) newguy, NAME(player), (int) player);
        anotify_fmt(player, CSUCC "Player %s(%d) created.", user, newguy);
    }
}

void
do_powers(int descr, dbref player, const char *name, const char *power)
{
    dbref thing;
    const char *p;
    char buf[BUFFER_LEN];
    object_flag_type pow = 0;

    if (tp_db_readonly)
        return;
    if (!Boy(player) || Typeof(player) != TYPE_PLAYER) {
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
        return;
    }

    if (tp_db_readonly) {
        anotify_nolisten2(player, CFAIL DBRO_MESG);
        return;
    }

    if (force_level) {
        anotify_nolisten2(player,
                          CFAIL "Can't set a @power from a @force or {force}.");
        return;
    }

    if (!name || !*name) {
        anotify_nolisten2(player, CNOTE "Powers List");
        anotify_nolisten2(player,
                          CMOVE
                          "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        anotify_nolisten2(player,
                          "ALL_MUF_PRIMS   - [m] Gives full access to MUF primitives");
        anotify_nolisten2(player,
                          "ANNOUNCE        - [a] Can use @wall and dwall commands");
        anotify_nolisten2(player,
                          "BOOT            - [b] Can use @boot and dboot commands");
        anotify_nolisten2(player,
                          "CHOWN_ANYTHING  - [c] Can @chown anything, unless it is PROTECTed");
        anotify_nolisten2(player,
                          "CONTROL_ALL     - [r] Has control over any object.");
        anotify_nolisten2(player,
                          "CONTROL_MUF     - [f] Has control over any MUF object.");
        anotify_nolisten2(player,
                          "EXPANDED_WHO    - [x] Gets the wizard version of WHO");
        anotify_nolisten2(player,
                          "HIDE            - [h] Can set themselves DARK or login HIDDEN");
        anotify_nolisten2(player,
                          "IDLE            - [i] Not effected by the idle limit");
        anotify_nolisten2(player,
                          "LINK_ANYWHERE   - [l] Can @link an exit to anywhere");
        anotify_nolisten2(player,
                          "LONG_FINGERS    - [g] Can do anything from a long distance");
        anotify_nolisten2(player, "NO_PAY          - [n] Infinite money");
        anotify_nolisten2(player,
                          "OPEN_ANYWHERE   - [o] Can @open an exit from any location");
        anotify_nolisten2(player,
                          "PLAYER_CREATE   - [p] @pcreate, @newpassword, @name");
        anotify_nolisten2(player,
                          "PLAYER_PURGE    - [u] @toad and @purge power.");
        anotify_nolisten2(player,
                          "SEARCH          - [s] Can use @find, @entrances, @own, and @contents");
        anotify_nolisten2(player,
                          "SEE_ALL         - [e] Can examine any object, and @list any program");
        anotify_nolisten2(player,
                          "STAFF           - [w] Special staff bit for use in MUF");
        anotify_nolisten2(player,
                          "SHUTDOWN        - [d] Can run @shutdown or @restart");
        anotify_nolisten2(player,
                          "TELEPORT        - [t] Unrestricted use of @teleport");
        anotify_nolisten2(player,
                          CMOVE
                          "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
        anotify_nolisten2(player, "Syntax: @power <player>=[!]<power>");
        anotify_nolisten2(player, CINFO "Done.");
        return;
    }

    /* find thing */
    if (!string_compare(name, "me"))
        name = NAME(player);
    if ((thing = lookup_player(name)) == NOTHING) {
        anotify_nolisten2(player, CFAIL "I can't find that player.");
        return;
    }

    /* move p past NOT_TOKEN if present */
    for (p = power; *p && (*p == NOT_TOKEN || isspace(*p)); p++) ;

    if (*p == '\0') {
        sprintf(buf, CINFO "%s:", unparse_object(MAN, thing));
        anotify_nolisten2(player, buf);
        anotify_nolisten2(player, power_description(thing));
        anotify_nolisten2(player, CINFO "Done.");
        return;
    } else if (string_prefix("ALL_MUF_PRIMS", p)) {
        pow = POW_ALL_MUF_PRIMS;
    } else if (string_prefix("ANNOUNCE", p)) {
        pow = POW_ANNOUNCE;
    } else if (string_prefix("BOOT", p)) {
        pow = POW_BOOT;
    } else if (string_prefix("CHOWN_ANYTHING", p)) {
        pow = POW_CHOWN_ANYTHING;
    } else if (string_prefix("CONTROL_ALL", p)) {
        pow = POW_CONTROL_ALL;
    } else if (string_prefix("CONTROL_MUF", p)) {
        pow = POW_CONTROL_MUF;
    } else if (string_prefix("EXPANDED_WHO", p)) {
        pow = POW_EXPANDED_WHO;
    } else if (string_prefix("HIDE", p)) {
        pow = POW_HIDE;
    } else if (string_prefix("IDLE", p)) {
        pow = POW_IDLE;
    } else if (string_prefix("LINK_ANYWHERE", p)) {
        pow = POW_LINK_ANYWHERE;
    } else if (string_prefix("LONG_FINGERS", p)) {
        pow = POW_LONG_FINGERS;
    } else if (string_prefix("NO_PAY", p)) {
        pow = POW_NO_PAY;
    } else if (string_prefix("OPEN_ANYWHERE", p)) {
        pow = POW_OPEN_ANYWHERE;
    } else if (string_prefix("PLAYER_CREATE", p)) {
        pow = POW_PLAYER_CREATE;
    } else if (string_prefix("PLAYER_PURGE", p)) {
        pow = POW_PLAYER_PURGE;
    } else if (string_prefix("SEARCH", p)) {
        pow = POW_SEARCH;
    } else if (string_prefix("SEE_ALL", p)) {
        pow = POW_SEE_ALL;
    } else if (string_prefix("SHUTDOWN", p)) {
        pow = POW_SHUTDOWN;
    } else if (string_prefix("TELEPORT", p)) {
        pow = POW_TELEPORT;
    } else if (string_prefix("STAFF", p)) {
        pow = POW_STAFF;
    } else {
        anotify_nolisten2(player, CINFO "I don't recognize that power.");
        return;
    }

    if (*power == NOT_TOKEN) {
        ts_modifyobject(player, thing);
        POWERS(thing) &= ~pow;
        DBDIRTY(thing);
        anotify_nolisten2(player, CSUCC "Power removed.");
    } else {
        ts_modifyobject(player, thing);
        POWERS(thing) |= pow;
        DBDIRTY(thing);
        anotify_nolisten2(player, CSUCC "Power given.");
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
long max_open_files(void);       /* from interface.c */

void
do_usage(dbref player)
{
    int pid;

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

#endif /* NO_USAGE_COMMAND */



void
do_muf_topprofs(dbref player, char *arg1)
{
    struct profnode {
        struct profnode *next;
        dbref prog;
        double proftime;
        double pcnt;
        long comptime;
        long usecount;
    } *tops = NULL;

    struct profnode *curr = NULL;
    int nodecount = 0;
    char buf[BUFFER_LEN];
    dbref i = NOTHING;
    int count = atoi(arg1);
    time_t current_systime = time(NULL);

    if (!Mage(OWNER(player))) {
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
        return;
    }

    if (!string_compare(arg1, "reset")) {
        for (i = db_top; i-- > 0;) {
            if (Typeof(i) == TYPE_PROGRAM) {
                DBFETCH(i)->sp.program.proftime.tv_sec = 0;
                DBFETCH(i)->sp.program.proftime.tv_usec = 0;
                DBFETCH(i)->sp.program.profstart = current_systime;
                DBFETCH(i)->sp.program.profuses = 0;
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

    for (i = db_top; i-- > 0;) {
        if (Typeof(i) == TYPE_PROGRAM && DBFETCH(i)->sp.program.code) {
            struct profnode *newnode =
                (struct profnode *) malloc(sizeof(struct profnode));
            struct timeval tmpt = DBFETCH(i)->sp.program.proftime;

            newnode->next = NULL;
            newnode->prog = i;
            newnode->proftime = tmpt.tv_sec;
            newnode->proftime += (tmpt.tv_usec / 1000000.0);
            newnode->comptime =
                current_systime - DBFETCH(i)->sp.program.profstart;
            newnode->usecount = DBFETCH(i)->sp.program.profuses;
            if (newnode->comptime > 0) {
                newnode->pcnt = 100.0 * newnode->proftime / newnode->comptime;
            } else {
                newnode->pcnt = 0.0;
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
    anotify_nolisten2(player, CINFO "     %CPU   TotalTime  UseCount  Program");
    while (tops) {
        curr = tops;
        sprintf(buf, "%10.3f %10.3f %9ld %s", curr->pcnt, curr->proftime,
                curr->usecount, unparse_object(player, curr->prog));
        notify(player, buf);
        tops = tops->next;
        free(curr);
    }
    sprintf(buf,
            CNOTE "Profile Length (sec): " NORMAL "%5ld  " CNOTE "%%idle: "
            NORMAL "%5.2f%%  " CNOTE "Total Cycles: " NORMAL "%5lu",
            (current_systime - sel_prof_start_time),
            ((double) (sel_prof_idle_sec + (sel_prof_idle_usec / 1000000.0)) *
             100.0) / (double) ((current_systime - sel_prof_start_time) + 0.01),
            sel_prof_idle_use);
    anotify_nolisten2(player, buf);
    anotify_nolisten2(player, CINFO "Done.");
}


void
do_mpi_topprofs(dbref player, char *arg1)
{
    struct profnode {
        struct profnode *next;
        dbref prog;
        double proftime;
        double pcnt;
        long comptime;
        long usecount;
    } *tops = NULL;

    struct profnode *curr = NULL;
    int nodecount = 0;
    char buf[BUFFER_LEN];
    dbref i = NOTHING;
    int count = atoi(arg1);
    time_t current_systime = time(NULL);

    if (!Mage(OWNER(player))) {
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
        return;
    }

    if (!string_compare(arg1, "reset")) {
        for (i = db_top; i-- > 0;) {
            if (DBFETCH(i)->mpi_prof_use) {
                DBFETCH(i)->mpi_prof_use = 0;
                DBFETCH(i)->mpi_proftime.tv_usec = 0;
                DBFETCH(i)->mpi_proftime.tv_sec = 0;
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

    for (i = db_top; i-- > 0;) {
        if (DBFETCH(i)->mpi_prof_use) {
            struct profnode *newnode =
                (struct profnode *) malloc(sizeof(struct profnode));
            newnode->next = NULL;
            newnode->prog = i;
            newnode->proftime = DBFETCH(i)->mpi_proftime.tv_sec;
            newnode->proftime += (DBFETCH(i)->mpi_proftime.tv_usec / 1000000.0);
            newnode->comptime = current_systime - mpi_prof_start_time;
            newnode->usecount = DBFETCH(i)->mpi_prof_use;
            if (newnode->comptime > 0) {
                newnode->pcnt = 100.0 * newnode->proftime / newnode->comptime;
            } else {
                newnode->pcnt = 0.0;
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
    anotify_nolisten2(player, CINFO "     %CPU   TotalTime  UseCount  Object");
    while (tops) {
        curr = tops;
        sprintf(buf, "%10.3f %10.3f %9ld %s", curr->pcnt, curr->proftime,
                curr->usecount, unparse_object(player, curr->prog));
        notify(player, buf);
        tops = tops->next;
        free(curr);
    }
    sprintf(buf,
            CNOTE "Profile Length (sec): " NORMAL "%5ld  " CNOTE "%%idle: "
            NORMAL "%5.2f%%  " CNOTE "Total Cycles: " NORMAL "%5lu",
            (current_systime - sel_prof_start_time),
            (((double) sel_prof_idle_sec +
              (sel_prof_idle_usec / 1000000.0)) * 100.0) /
            (double) ((current_systime - sel_prof_start_time) + 0.01),
            sel_prof_idle_use);
    anotify_nolisten2(player, buf);
    anotify_nolisten2(player, CINFO "Done.");
}


void
do_all_topprofs(dbref player, char *arg1)
{
    struct profnode {
        struct profnode *next;
        dbref prog;
        double proftime;
        double pcnt;
        long comptime;
        long usecount;
        short type;
    } *tops = NULL;

    struct profnode *curr = NULL;
    int nodecount = 0;
    char buf[BUFFER_LEN];
    dbref i = NOTHING;
    int count = atoi(arg1);
    time_t current_systime = time(NULL);

    if (!Mage(OWNER(player))) {
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
        return;
    }

    if (!string_compare(arg1, "reset")) {
        for (i = db_top; i-- > 0;) {
            if (DBFETCH(i)->mpi_prof_use) {
                DBFETCH(i)->mpi_prof_use = 0;
                DBFETCH(i)->mpi_proftime.tv_usec = 0;
                DBFETCH(i)->mpi_proftime.tv_sec = 0;
            }
            if (Typeof(i) == TYPE_PROGRAM) {
                DBFETCH(i)->sp.program.proftime.tv_sec = 0;
                DBFETCH(i)->sp.program.proftime.tv_usec = 0;
                DBFETCH(i)->sp.program.profstart = current_systime;
                DBFETCH(i)->sp.program.profuses = 0;
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

    for (i = db_top; i-- > 0;) {
        if (DBFETCH(i)->mpi_prof_use) {
            struct profnode *newnode =
                (struct profnode *) malloc(sizeof(struct profnode));
            newnode->next = NULL;
            newnode->prog = i;
            newnode->proftime = DBFETCH(i)->mpi_proftime.tv_sec;
            newnode->proftime += (DBFETCH(i)->mpi_proftime.tv_usec / 1000000.0);
            newnode->comptime = current_systime - mpi_prof_start_time;
            newnode->usecount = DBFETCH(i)->mpi_prof_use;
            newnode->type = 0;
            if (newnode->comptime > 0) {
                newnode->pcnt = 100.0 * newnode->proftime / newnode->comptime;
            } else {
                newnode->pcnt = 0.0;
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
            struct profnode *newnode =
                (struct profnode *) malloc(sizeof(struct profnode));
            struct timeval tmpt = DBFETCH(i)->sp.program.proftime;

            newnode->next = NULL;
            newnode->prog = i;
            newnode->proftime = tmpt.tv_sec;
            newnode->proftime += (tmpt.tv_usec / 1000000.0);
            newnode->comptime =
                current_systime - DBFETCH(i)->sp.program.profstart;
            newnode->usecount = DBFETCH(i)->sp.program.profuses;
            newnode->type = 1;
            if (newnode->comptime > 0) {
                newnode->pcnt = 100.0 * newnode->proftime / newnode->comptime;
            } else {
                newnode->pcnt = 0.0;
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
    anotify_nolisten2(player,
                      CINFO "     %CPU   TotalTime  UseCount  Type  Object");
    while (tops) {
        curr = tops;
        sprintf(buf, "%10.3f %10.3f %9ld%5s   %s", curr->pcnt, curr->proftime,
                curr->usecount, curr->type ? "MUF" : "MPI",
                unparse_object(player, curr->prog));
        notify(player, buf);
        tops = tops->next;
        free(curr);
    }
    sprintf(buf,
            CNOTE "Profile Length (sec): " NORMAL "%5ld  " CNOTE "%%idle: "
            NORMAL "%5.2f%%  " CNOTE "Total Cycles: " NORMAL "%5lu",
            (current_systime - sel_prof_start_time),
            ((double) (sel_prof_idle_sec + (sel_prof_idle_usec / 1000000.0)) *
             100.0) / (double) ((current_systime - sel_prof_start_time) + 0.01),
            sel_prof_idle_use);
    anotify_nolisten2(player, buf);
    anotify_nolisten2(player, CINFO "Done.");
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
        notify_fmt(who, "Total allocated from system:   %6dk",
                   (mi.arena / 1024));
        notify_fmt(who, "Number of non-inuse chunks:    %6d", mi.ordblks);
        notify_fmt(who, "Small block count:             %6d", mi.smblks);
        notify_fmt(who, "Small block mem alloced:       %6dk",
                   (mi.usmblks / 1024));
        notify_fmt(who, "Small block memory free:       %6dk",
                   (mi.fsmblks / 1024));
#ifdef HAVE_STRUCT_MALLINFO_HBLKS
        notify_fmt(who, "Number of mmapped regions:     %6d", mi.hblks);
#endif
        notify_fmt(who, "Total memory mmapped:          %6dk",
                   (mi.hblkhd / 1024));
        notify_fmt(who, "Total memory malloced:         %6dk",
                   (mi.uordblks / 1024));
        notify_fmt(who, "Mem allocated overhead:        %6dk",
                   ((mi.arena - mi.uordblks) / 1024));
        notify_fmt(who, "Memory free:                   %6dk",
                   (mi.fordblks / 1024));
#ifdef HAVE_STRUCT_MALLINFO_KEEPCOST
        notify_fmt(who, "Top-most releasable chunk size:%6dk",
                   (mi.keepcost / 1024));
#endif
#ifdef HAVE_STRUCT_MALLINFO_TREEOVERHEAD
        notify_fmt(who, "Memory free overhead:    %6dk",
                   (mi.treeoverhead / 1024));
#endif
#ifdef HAVE_STRUCT_MALLINFO_GRAIN
        notify_fmt(who, "Small block grain: %6d", mi.grain);
#endif
#ifdef HAVE_STRUCT_MALLINFO_ALLOCATED
        notify_fmt(who, "Mem chunks alloced:%6d", mi.allocated);
#endif
    }
# endif /* HAVE_MALLINFO */
#endif /* NO_MEMORY_COMMAND */

#ifdef MALLOC_PROFILING
    notify(who, "  ");
    CrT_summarize(who);
    CrT_summarize_to_file("malloc_log", "Manual Checkpoint");
#endif

    anotify_nolisten2(who, CINFO "Done.");
}

void
do_fixw(dbref player, const char *msg)
{
    int i;

    if (force_level) {
        anotify_nolisten2(player, CFAIL "Can't @force @fixwizbits.");
        return;
    }
    if (!Man(player)) {
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
        return;
    }
    if (strcmp(msg, "Convert DB to new level system.")) {
        anotify_nolisten2(player, CINFO "What's the magic phrase?");
        return;
    }
    for (i = 0; i < db_top; i++) {
        if (FLAGS(i) & W3)
            SetMLevel(i, LWIZ);
        else if ((FLAGS(i) & (W2)) && (FLAGS(i) & (W1)))
            SetMLevel(i, LM3);
        else if (FLAGS(i) & (W1))
            SetMLevel(i, LM2);
        else if (FLAGS(i) & (W2))
            SetMLevel(i, LM1);
    }
    anotify_nolisten2(player, CINFO "Done.");
}
