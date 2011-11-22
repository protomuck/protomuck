#include "copyright.h"
#include "config.h"
#include "params.h"

/* commands which look at things */

#include "db.h"
#include "tune.h"
#include "mpi.h"
#include "props.h"
#include "interface.h"
#include "match.h"
#include "externs.h"

#define UPCASE(x) (toupper(x))
#define DOWNCASE(x) (tolower(x))

#define EXEC_SIGNAL '@'         /* Symbol which tells us what we're looking at
                                 * is an execution order and not a message.    */

/* prints owner of something */
static void
print_owner(dbref player, dbref thing)
{
    dbref ref;

    switch (Typeof(thing)) {
        case TYPE_PLAYER:
            anotify_fmt(player, SYSYELLOW "%s is a player.", NAME(thing));
            break;
        case TYPE_ROOM:
        case TYPE_THING:
        case TYPE_PROGRAM:
            anotify_fmt(player, SYSYELLOW "Owner: %s", NAME(OWNER(thing)));
            break;
        case TYPE_EXIT:
            anotify_fmt(player, SYSYELLOW "Owner: %s", NAME(OWNER(thing)));
            if (DBFETCH(thing)->sp.exit.ndest) {
                ref = (DBFETCH(thing)->sp.exit.dest)[0];
                if (ref == NIL || ref == HOME)
                    anotify_fmt(player, SYSCYAN "And is linked to: %s",
                                ansi_unparse_object(MAN, ref));
                else if (Typeof(ref) == TYPE_PROGRAM && FLAGS(ref) & VEHICLE) {
                    anotify_fmt(player, SYSCYAN "And is linked to: %s",
                                ansi_unparse_object(MAN, ref));
                }
            }
            break;
        case TYPE_GARBAGE:
            anotify_fmt(player, SYSBLUE "%s is garbage.", NAME(thing));
            break;
    }
}

void
exec_or_notify_2(int descr, dbref player, dbref thing,
                 const char *message, const char *whatcalled, bool typ)
{
    const char *p;
    char buf[BUFFER_LEN];

    p = message;

    if (*p == EXEC_SIGNAL) {
        char tmpcmd[BUFFER_LEN];
        char tmparg[BUFFER_LEN];
        char *p2;
        char *p3;
        int i;

        if (*(++p) == REGISTERED_TOKEN) {
            strcpy(buf, p);
            for (p2 = buf; *p && !isspace(*p); p++) ;
            if (*p)
                p++;

            for (p3 = buf; *p3 && !isspace(*p3); p3++) ;
            if (*p3)
                *p3 = '\0';

            if (*p2)
                i = (dbref) find_registered_obj(thing, p2);
            else
                i = 0;
        } else {
            i = atoi(p);
            for (; *p && !isspace(*p); p++) ;
            if (*p)
                p++;
        }
        if (i < 0 || i >= db_top || (Typeof(i) != TYPE_PROGRAM)) {
            if (*p)
                notify(player, p);
            else
                notify(player, "You see nothing special.");
        } else {
            struct frame *tmpfr;

            strcpy(tmparg, match_args);
            strcpy(tmpcmd, match_cmdname);
            p = do_parse_mesg(descr, player, thing, p, whatcalled, buf,
                              MPI_ISPRIVATE);
            strcpy(match_args, p);
            strcpy(match_cmdname, whatcalled);
            tmpfr = interp(descr, player, DBFETCH(player)->location, i, thing,
                           PREEMPT, STD_HARDUID, 0);
            if (tmpfr)
                interp_loop(player, i, tmpfr, 0);

            strcpy(match_args, tmparg);
            strcpy(match_cmdname, tmpcmd);
        }
    } else {
        p = do_parse_mesg(descr, player, thing, p, whatcalled, buf,
                          MPI_ISPRIVATE);
        if (typ)
            notify(player, p);
        else
            notify_html(player, p);
    }
}

int
count_details(dbref player, dbref what, const char *propname)
{
    const char *pname;
    char exbuf[BUFFER_LEN];
    char buf[BUFFER_LEN];
    int count;


    count = 0;

    strcpy(buf, propname);
    if (is_propdir(what, buf)) {
        strcat(buf, "/");
        while ((pname = next_prop_name(what, exbuf, buf))) {
            strcpy(buf, pname);
            count++;
        }
    }

    return count;
}

void
look_details(dbref player, dbref what, const char *propname)
{
    const char *pname;
    char *tmpchr;
    char buf[BUFFER_LEN], buf2[BUFFER_LEN];
    char exbuf[BUFFER_LEN];

    strcpy(buf, propname);
    if (is_propdir(what, buf)) {
        strcat(buf, "/");
        while ((pname = next_prop_name(what, exbuf, buf))) {
            strcpy(buf, pname);
            strcpy(buf2, SYSPURPLE);
            strcat(buf2, pname + strlen("_obj/"));
            tmpchr = buf2 + strlen(SYSPURPLE);
            while (((*tmpchr) != '\0')) {
                if (((*tmpchr) == ';'))
                    (*tmpchr) = '\0';
                else
                    tmpchr++;
            }

            if (controls(player, what))
                strcat(buf2, CINFO "(detail)");

            anotify_nolisten(player, buf2, 1);
        }
    }
}

static void
look_contents(dbref player, dbref loc, const char *contents_name)
{
    dbref thing;
    bool can_see_loc;
    bool saw_something = 0;

    /* check to see if he can see the location */
    can_see_loc = (!Dark(loc) || controls(player, loc) || Light(loc));

    /* check to see if there is anything there */
    if (((Typeof(loc) != TYPE_ROOM) || !Dark(loc)))
        DOLIST(thing, DBFETCH(loc)->contents) {
        if (can_see(player, thing, can_see_loc)) {
            /* something exists!  show him everything */
            saw_something = 1;
            anotify_nolisten(player, contents_name, 1);
            DOLIST(thing, DBFETCH(loc)->contents) {
                if (can_see(player, thing, can_see_loc))
                    anotify_nolisten(player, ansi_unparse_object(player, thing),
                                     1);
            }
            break;              /* we're done */
        }
        }
    if (!saw_something) {
        if (count_details(player, loc, "_obj")) {
            anotify_nolisten(player, contents_name, 1);
            look_details(player, loc, "_obj");
        }
    } else
        look_details(player, loc, "_obj");
}

static void
look_simple(int descr, dbref player, dbref thing, const char *name)
{
    if (Html(player) && GETHTMLDESC(thing))
        exec_or_html_notify(descr, player, thing, GETHTMLDESC(thing), name);
    else if (GETDESC(thing))
        exec_or_notify(descr, player, thing, GETDESC(thing), name);
    else
        notify(player, "You see nothing special.");
}

void
look_room(int descr, dbref player, dbref loc)
{
    char obj_num[20];

    /* tell him the name, and the number if he can link to it */
    anotify_nolisten(player, ansi_unparse_object(player, loc), 1);

    /* tell him the description */
    if (Typeof(loc) == TYPE_ROOM) {
        if (Html(player) && GETHTMLDESC(loc))
            exec_or_html_notify(descr, player, loc, GETHTMLDESC(loc),
                                "(@Desc)");
        else if (GETDESC(loc))
            exec_or_notify(descr, player, loc, GETDESC(loc), "(@Desc)");

        /* tell him the appropriate messages if he has the key */
        if (can_doit(descr, player, loc, 0)) {
/* These 2 used to be handled in can_doit, but I had moved them to
 * trigger() to account for the success messages sometimes getting
 * displayed even when the action could not be done. (zombie blocks, 
 * for example). All this stuff for checking permissions in predicates.c
 * needs to be rewritten to make a lot more sense. TODO
 */
            if (GETSUCC(loc))
                exec_or_notify(descr, player, loc, GETSUCC(loc), "(@Succ)");

            if (GETOSUCC(loc) && !Dark(player))
                parse_omessage(descr, player, loc, loc, GETOSUCC(loc),
                               NAME(player), "(@Osucc)");
        }
    } else {
        if (Html(player) && GETIHTMLDESC(loc))
            exec_or_html_notify(descr, player, loc, GETIHTMLDESC(loc),
                                "(@Idesc)");
        else if (GETIDESC(loc))
            exec_or_notify(descr, player, loc, GETIDESC(loc), "(@Idesc)");
    }
    ts_useobject(player, loc);

    /* tell him the contents */

    look_contents(player, loc, SYSBLUE "Contents:");
    if (tp_look_propqueues) {
        sprintf(obj_num, "#%d", loc);
        envpropqueue(descr, player, loc, player, loc, NOTHING, "_lookq",
                     obj_num, 1, 1);
    }
}

void
do_look_around(int descr, dbref player)
{
    dbref loc;

    if ((loc = getloc(player)) == NOTHING)
        return;

    look_room(descr, player, loc);
}

void
do_look_at(int descr, dbref player, const char *name,
           const char *detail)
{
    dbref thing, lastthing;
    bool nomatch;
    struct match_data md;
    char buf[BUFFER_LEN];
    char obj_num[20];

    if (*name == '\0' || !string_compare(name, "here")) {
        if ((thing = getloc(player)) != NOTHING)
            look_room(descr, player, thing);
    } else {

#ifdef DISKBASE
        fetchprops(DBFETCH(player)->location);
#endif

        /* look at a thing here */
        init_match(descr, player, name, NOTYPE, &md);
        match_all_exits(&md);
        match_neighbor(&md);
        match_possession(&md);
        /* match_registered(&md); */
        if (Mage(OWNER(player))) {
            match_absolute(&md);
            match_player(&md);
        }
        match_here(&md);
        match_me(&md);

        thing = match_result(&md);
        if (thing != NOTHING && thing != AMBIGUOUS && !*detail) {
            switch (Typeof(thing)) {
                case TYPE_ROOM:
                    if (getloc(player) != thing
                        && !can_link_to(player, TYPE_ROOM, thing)) {
                        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
                    } else {
                        look_room(descr, player, thing);
                    }
                    break;
                case TYPE_PLAYER:
                    if (getloc(player) != getloc(thing)
                        && !controls(player, thing)) {
                        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
                    } else {
                        look_simple(descr, player, thing, name);
                        look_contents(player, thing, SYSBLUE "Carrying:");
                        if (tp_look_propqueues) {
                            sprintf(obj_num, "#%d", thing);
                            envpropqueue(descr, player, thing, player, thing,
                                         NOTHING, "_lookq", obj_num, 1, 1);
                        }
                    }
                    break;
                case TYPE_THING:
                    if (getloc(player) != getloc(thing)
                        && getloc(thing) != player && !controls(player, thing)) {
                        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
                    } else {
                        look_simple(descr, player, thing, name);
                        if (!(FLAGS(thing) & HAVEN)) {
                            look_contents(player, thing, SYSBLUE "Contains:");
                            ts_useobject(player, thing);
                        }
                        if (tp_look_propqueues) {
                            sprintf(obj_num, "#%d", thing);
                            envpropqueue(descr, player, thing, player, thing,
                                         NOTHING, "_lookq", obj_num, 1, 1);
                        }
                    }
                    break;
                default:
                    look_simple(descr, player, thing, name);
                    if (Typeof(thing) != TYPE_PROGRAM)
                        ts_useobject(player, thing);
                    if (tp_look_propqueues) {
                        sprintf(obj_num, "#%d", thing);
                        envpropqueue(descr, player, thing, player, thing,
                                     NOTHING, "_lookq", obj_num, 1, 1);
                    }
                    break;
            }
        } else if (thing == NOTHING || (*detail && thing != AMBIGUOUS)) {
            char propname[BUFFER_LEN];
            PropPtr propadr, pptr, lastmatch;
            bool ambig;

            lastthing = NOTHING;
            if (thing == NOTHING) {
                nomatch = 1;
                thing = player;
                sprintf(buf, "%s", name);
            } else {
                nomatch = 0;
                sprintf(buf, "%s", detail);
            }

#ifdef DISKBASE
            fetchprops(thing);
#endif

            lastmatch = NULL;
            ambig = 0;

          repeat_match:
            propadr = first_prop(thing, "_obj/", &pptr, propname);
            while (propadr) {
                if (exit_prefix(propname, buf)) {
                    if (lastmatch != NULL) {
                        lastmatch = NULL;
                        ambig = 1;
                        break;
                    } else {
                        lastmatch = propadr;
                        lastthing = thing;
                    }
                }
                propadr = next_prop(pptr, propadr, propname);
            }
            propadr = first_prop(thing, "_det/", &pptr, propname);
            while (propadr) {
                if (exit_prefix(propname, buf)) {
                    if (lastmatch != NULL) {
                        lastmatch = NULL;
                        ambig = 1;
                        break;
                    } else {
                        lastmatch = propadr;
                        lastthing = thing;
                    }
                }
                propadr = next_prop(pptr, propadr, propname);
            }
            if (nomatch && thing == player)
                if ((thing = getloc(player)) != player)
                    goto repeat_match;

            thing = lastthing;
            if (lastmatch != NULL && PropType(lastmatch) == PROP_STRTYP) {
#ifdef DISKBASE
                propfetch(thing, lastmatch); /* DISKBASE PROPVALS */
#endif
                exec_or_notify(descr, player, thing, PropDataUNCStr(lastmatch),
                               "(@detail)");
            } else if (ambig) {
                anotify_nolisten(player, CINFO AMBIGUOUS_MESSAGE, 1);
            } else if (*detail) {
                notify(player, "You see nothing special.");
            } else {
                anotify_nolisten(player, CINFO NOMATCH_MESSAGE, 1);
            }
        } else
            anotify_nolisten(player, CINFO AMBIGUOUS_MESSAGE, 1);
    }
}

#ifdef VERBOSE_EXAMINE

static const char *
flag_description(dbref thing)
{
    static char buf[1024];
    static char buf2[32];
    int i,j;

    strcpy(buf, SYSGREEN "Type: " SYSYELLOW);
    switch (Typeof(thing)) {
        case TYPE_ROOM:
            strcat(buf, "ROOM");
            break;
        case TYPE_EXIT:
            strcat(buf, "EXIT/ACTION");
            break;
        case TYPE_THING:
            strcat(buf, "THING");
            break;
        case TYPE_PLAYER:
            strcat(buf, "PLAYER");
            break;
        case TYPE_PROGRAM:
            strcat(buf, "PROGRAM");
            break;
        case TYPE_GARBAGE:
            strcat(buf, "GARBAGE");
            break;
        default:
            strcat(buf, "***UNKNOWN TYPE***");
            break;
    }

    if (FLAGS(thing) & ~TYPE_MASK || FLAG2(thing)) {
        /* print flags */
        strcat(buf, SYSGREEN "  Flags:" SYSYELLOW);
        if (thing == 1) {
            strcat(buf, " MAN");
        } else {
            switch (RawMLevel(thing)) {
                case LMAN:
                    strcat(buf, " MAN");
                    break;
                case LBOY:
                    strcat(buf, " BOY");
                    break;
                case LARCH:
                    strcat(buf, " ARCHWIZARD");
                    break;
                case LWIZ:
                    strcat(buf, " WIZARD");
                    break;
                case LMAGE:
                    strcat(buf, " MAGE");
                    break;
                case LM3:
                    strcat(buf, " MUCKER3");
                    break;
                case LM2:
                    strcat(buf, " MUCKER2");
                    break;
                case LM1:
                    strcat(buf, " MUCKER");
                    break;
                case LMPI:
                    strcat(buf, " MEEPER");
                    break;
            }
        }
        if (FLAGS(thing) & QUELL)
            strcat(buf, " QUELL");
        if (FLAGS(thing) & STICKY)
            strcat(buf, (Typeof(thing) == TYPE_PROGRAM) ? " SETUID" :
                   (Typeof(thing) == TYPE_PLAYER) ? " SILENT" : " STICKY");
        if (FLAGS(thing) & DARK)
            strcat(buf,
                   (Typeof(thing) == TYPE_PROGRAM) ? " DEBUGGING" : " DARK");
        if (FLAGS(thing) & LINK_OK)
            strcat(buf, " LINK_OK");

        if (FLAGS(thing) & BUILDER)
            strcat(buf,
                   (Typeof(thing) != TYPE_PROGRAM) ? " BUILDER" : " BOUND");
        if (FLAGS(thing) & CHOWN_OK)
            strcat(buf,
                   (Typeof(thing) == TYPE_PLAYER) ? " COLOR_ON" : " CHOWN_OK");
#ifdef CONTROLS_SUPPORT
        if (FLAG2(thing) & F2CONTROLS)
            strcat(buf, " CONTROLS");
#endif
        if (FLAG2(thing) & F2IMMOBILE)
            strcat(buf, " IMMOBILE");
        if (FLAGS(thing) & JUMP_OK)
            strcat(buf, " JUMP_OK");
        if (FLAGS(thing) & VEHICLE)
            strcat(buf,
                   (Typeof(thing) != TYPE_PROGRAM) ? " VEHICLE" : " VIEWABLE");
        if (FLAGS(thing) & XFORCIBLE)
            strcat(buf,
                   (Typeof(thing) !=
                    TYPE_PROGRAM) ? " XFORCIBLE" : " EXPANDED_DEBUG");
        if (FLAGS(thing) & ZOMBIE)
            strcat(buf, " ZOMBIE");
        if (FLAGS(thing) & HAVEN)
            strcat(buf, (Typeof(thing) != TYPE_PROGRAM) ?
                   (Typeof(thing) !=
                    TYPE_THING ? " HAVEN" : "HIDE") : " HARDUID");
        if (FLAGS(thing) & ABODE)
            strcat(buf, (Typeof(thing) != TYPE_PROGRAM) ?
                   (Typeof(thing) !=
                    TYPE_EXIT ? " ABODE" : " ABATE") : " AUTOSTART");
        if (FLAG2(thing) & F2GUEST)
            strcat(buf, " GUEST");
        if (FLAG2(thing) & F2IDLE)
            strcat(buf, " IDLE");
        if (FLAG2(thing) & F2LOGWALL)
            strcat(buf, " LOGWALL");
        if (FLAG2(thing) & F2LIGHT)
            strcat(buf, (Typeof(thing) == TYPE_PROGRAM) ? " OLDCOMMENT" :
                   " LIGHT");
        if (FLAG2(thing) & F2MUFCOUNT)
            strcat(buf, " MUFCOUNT");
        if (FLAG2(thing) & F2PROTECT)
            strcat(buf, " PROTECT");
        if (FLAG2(thing) & F2ANTIPROTECT)
            strcat(buf, " ANTIPROTECT");
        if (FLAG2(thing) & F2PARENT)
            strcat(buf, (Typeof(thing) == TYPE_PLAYER ||
                         Typeof(thing) ==
                         TYPE_PROGRAM) ? " PROG_DEBUG" : " PARENT");
        if (FLAG2(thing) & F2HIDDEN)
            strcat(buf, " HIDDEN");
        if (FLAG2(thing) & F2NO_COMMAND)
            strcat(buf,
                   (Typeof(thing) ==
                    TYPE_PROGRAM) ? " NO_OPTIMIZE" : " NO_COMMAND");
        if (FLAG2(thing) & F2EXAMINE_OK)
            strcat(buf, " EXAMINE_OK");
        if (FLAG2(thing) & F2PUEBLO)
            strcat(buf, " PUEBLO");
        if (FLAG2(thing) & F2HTML)
            strcat(buf, " HTML");
        if (FLAG2(thing) & F2SUSPECT)
            strcat(buf, " SUSPECT");
        if (FLAG2(thing) & F2MOBILE) {
            if (!*tp_userflag_name) {
                strcat(buf,
                       (Typeof(thing) == TYPE_ROOM) ? " OFFER" : " MOBILE");
            } else {
                strcat(buf, " ");
                strcat(buf, tp_userflag_name);
            }
        }

	for (i=0; i<32; i++)
	    if (LFLAG(thing) & LFLAGx(i)) {
		strncpy(buf2, lflag_name[i], 32);
		for (j=0; j<32; j++) buf2[j]=UPCASE(buf2[j]);
                strcat(buf, " ");
		strcat(buf, buf2);
	    }
    }
    return buf;
}

#endif /* VERBOSE_EXAMINE */

const char *
power_description(dbref thing)
{
    static char buf[1024];

    strcpy(buf, SYSGREEN "Powers: " SYSYELLOW);
    if (POWERS(thing) & POW_ALL_MUF_PRIMS)
        strcat(buf, "ALL_MUF_PRIMS ");
    if (POWERS(thing) & POW_ANNOUNCE)
        strcat(buf, "ANNOUNCE ");
    if (POWERS(thing) & POW_BOOT)
        strcat(buf, "BOOT ");
    if (POWERS(thing) & POW_CHOWN_ANYTHING)
        strcat(buf, "CHOWN_ANYTHING ");
    if (POWERS(thing) & POW_CONTROL_ALL)
        strcat(buf, "CONTROL_ALL ");
    if (POWERS(thing) & POW_CONTROL_MUF)
        strcat(buf, "CONTROL_MUF ");
    if (POWERS(thing) & POW_EXPANDED_WHO)
        strcat(buf, "EXPANDED_WHO ");
    if (POWERS(thing) & POW_HIDE)
        strcat(buf, "HIDE ");
    if (POWERS(thing) & POW_IDLE)
        strcat(buf, "IDLE ");
    if (POWERS(thing) & POW_LINK_ANYWHERE)
        strcat(buf, "LINK_ANYWHERE ");
    if (POWERS(thing) & POW_LONG_FINGERS)
        strcat(buf, "LONG_FINGERS ");
    if (POWERS(thing) & POW_NO_PAY)
        strcat(buf, "NO_PAY ");
    if (POWERS(thing) & POW_OPEN_ANYWHERE)
        strcat(buf, "OPEN_ANYWHERE ");
    if (POWERS(thing) & POW_PLAYER_CREATE)
        strcat(buf, "PLAYER_CREATE ");
    if (POWERS(thing) & POW_PLAYER_PURGE)
        strcat(buf, "PLAYER_PURGE ");
    if (POWERS(thing) & POW_SEARCH)
        strcat(buf, "SEARCH ");
    if (POWERS(thing) & POW_SEE_ALL)
        strcat(buf, "SEE_ALL ");
    if (POWERS(thing) & POW_TELEPORT)
        strcat(buf, "TELEPORT ");
    if (POWERS(thing) & POW_SHUTDOWN)
        strcat(buf, "SHUTDOWN ");
    if (POWERS(thing) & POW_STAFF)
        strcat(buf, "STAFF ");
    return buf;
}

int
listprops_wildcard(dbref player, dbref thing, const char *dir, const char *wild)
{
    char propname[BUFFER_LEN];
    char wld[BUFFER_LEN];
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    PropPtr propadr, pptr;
    char *ptr, *wldcrd = wld;
    int i, cnt = 0;
    bool recurse = 0;

    strcpy(wld, wild);
    i = strlen(wld);
    if (i && wld[i - 1] == PROPDIR_DELIMITER)
        strcat(wld, "*");
    for (wldcrd = wld; *wldcrd == PROPDIR_DELIMITER; wldcrd++) ;
    if (!strcmp(wldcrd, "**"))
        recurse = 1;

    for (ptr = wldcrd; *ptr && *ptr != PROPDIR_DELIMITER; ptr++) ;
    if (*ptr)
        *ptr++ = '\0';

    propadr = first_prop(thing, (char *) dir, &pptr, propname);
    while (propadr) {
        if (equalstr(wldcrd, propname)) {
            sprintf(buf, "%s%c%s", dir, PROPDIR_DELIMITER, propname);
            if ((!Prop_Hidden(buf) && !(PropFlags(propadr) & PROP_SYSPERMS))
                || WizHidden(OWNER(player))) {
                if (!*ptr || recurse) {
                    cnt++;
                    displayprop(player, thing, buf, buf2);
                    anotify_nolisten(player, buf2, 1);
                }
                if (recurse)
                    ptr = (char *)"**";
                cnt += listprops_wildcard(player, thing, buf, ptr);
            }
        }
        propadr = next_prop(pptr, propadr, propname);
    }
    return cnt;
}

int
size_object(dbref i, int load)
{
    int byts;

    byts = sizeof(struct object);

    if (NAME(i))
        byts += strlen(NAME(i)) + 1;

    byts += size_properties(i, load);

    if (Typeof(i) == TYPE_EXIT && DBFETCH(i)->sp.exit.dest) {
        byts += sizeof(dbref) * DBFETCH(i)->sp.exit.ndest;
    } else if (Typeof(i) == TYPE_PLAYER && DBFETCH(i)->sp.player.password) {
        byts += strlen(DBFETCH(i)->sp.player.password) + 1;
    } else if (Typeof(i) == TYPE_PROGRAM) {
        byts += size_prog(i);
    }

    return byts;
}

void
do_examine(int descr, dbref player, const char *name, const char *dir)
{
    char buf2[BUFFER_LEN];
    char buf[BUFFER_LEN];
    struct match_data md;
    dbref thing;
    int i, cnt;
    dbref content;
    dbref exit;
    dbref ref_tm;

    struct tm *time_tm;         /* used for timestamps */

    if (Guest(player)) {
        anotify_fmt(player, CFAIL "%s", tp_noguest_mesg);
        return;
    }

    if (*name == '\0') {
        if ((thing = getloc(player)) == NOTHING)
            return;
    } else {
        /* look it up */
        init_match(descr, player, name, NOTYPE, &md);

        match_all_exits(&md);
        match_neighbor(&md);
        match_possession(&md);
        match_absolute(&md);
        match_registered(&md);

        /* only Wizards can examine other players */
        if (Mage(OWNER(player)) || POWERS(OWNER(player)) & POW_LONG_FINGERS)
            match_player(&md);

        match_here(&md);
        match_me(&md);

        /* get result */
        if ((thing = noisy_match_result(&md)) == NOTHING)
            return;
    }

    if ((!(POWERS(player) & POW_SEE_ALL) && !ExamineOk(thing)
         && !can_link(OWNER(player), thing) && !Wiz(OWNER(player)) && !(!*dir
                                                                        &&
                                                                        Mage
                                                                        (OWNER
                                                                         (player))))
        || (Protect(thing) && MLevel(OWNER(thing)) >= LBOY
            && !(MLevel(OWNER(player)) >= LBOY))) {
        print_owner(player, thing);
        return;
    }
    if (*dir) {
        /* show him the properties */
        cnt = listprops_wildcard(player, thing, "", dir);
        sprintf(buf, CINFO "%d propert%s listed.", cnt,
                (cnt == 1 ? "y" : "ies"));
        anotify_nolisten(player, buf, 1);
        return;
    }
    switch (Typeof(thing)) {
        case TYPE_ROOM:
            sprintf(buf, "%.*s" SYSNORMAL "  Owner: %s  Parent: ",
                    (int) (BUFFER_LEN - strlen(NAME(OWNER(thing))) - 35),
                    ansi_unparse_object(OWNER(player), thing),
                    NAME(OWNER(thing)));
            strcat(buf,
                   ansi_unparse_object(OWNER(player),
                                       DBFETCH(thing)->location));
            break;
        case TYPE_THING:
            sprintf(buf, "%.*s" SYSNORMAL "  Owner: %s  Value: %d",
                    (int) (BUFFER_LEN - strlen(NAME(OWNER(thing))) - 35),
                    ansi_unparse_object(OWNER(player), thing),
                    NAME(OWNER(thing)), DBFETCH(thing)->sp.thing.value);
            break;
        case TYPE_PLAYER:
            sprintf(buf, "%.*s" SYSNORMAL "  %s: %d  ",
                    (int) (BUFFER_LEN - strlen(NAME(OWNER(thing))) - 35),
                    ansi_unparse_object(OWNER(player), thing),
                    tp_cpennies, DBFETCH(thing)->sp.player.pennies);
            break;
        case TYPE_EXIT:
        case TYPE_PROGRAM:
            sprintf(buf, "%.*s" SYSNORMAL "  Owner: %s",
                    (int) (BUFFER_LEN - strlen(NAME(OWNER(thing))) - 35),
                    ansi_unparse_object(OWNER(player), thing),
                    NAME(OWNER(thing)));
            break;
        case TYPE_GARBAGE:
            strcpy(buf, ansi_unparse_object(OWNER(player), thing));
            break;
    }
    anotify_nolisten(player, buf, 1);

#ifdef VERBOSE_EXAMINE
    anotify_nolisten(player, flag_description(thing), 1);
    if ((Typeof(thing) == TYPE_PLAYER) && (POWERS(thing)))
        anotify_nolisten(player, power_description(thing), 1);
#endif /* VERBOSE_EXAMINE */

    if (GETDESC(thing)) {
        sprintf(buf, SYSCYAN "DESC:" SYSAQUA " %s", tct(GETDESC(thing), buf2));
        anotify_nolisten(player, buf, 1);
    }
    if (GETIDESC(thing)) {
        sprintf(buf, SYSCYAN "IDESC:" SYSAQUA " %s",
                tct(GETIDESC(thing), buf2));
        anotify_nolisten(player, buf, 1);
    }

    if (GETANSIDESC(thing)) {
        sprintf(buf, SYSCYAN "ANSIDESC:" SYSAQUA " %s",
                tct(GETANSIDESC(thing), buf2));
        anotify_nolisten(player, buf, 1);
    }
    if (GETIANSIDESC(thing)) {
        sprintf(buf, SYSCYAN "IANSIDESC:" SYSAQUA " %s",
                tct(GETIANSIDESC(thing), buf2));
        anotify_nolisten(player, buf, 1);
    }

    if (GETHTMLDESC(thing)) {
        sprintf(buf, SYSCYAN "HTMLDESC:" SYSAQUA " %s",
                tct(GETHTMLDESC(thing), buf2));
        anotify_nolisten(player, buf, 1);
    }
    if (GETIHTMLDESC(thing)) {
        sprintf(buf, SYSCYAN "IHTMLDESC:" SYSAQUA " %s",
                tct(GETIHTMLDESC(thing), buf2));
        anotify_nolisten(player, buf, 1);
    }

    sprintf(buf, SYSVIOLET "Key:" SYSPURPLE " %s",
            unparse_boolexp(OWNER(player), GETLOCK(thing), 1));
    anotify_nolisten(player, buf, 1);

    sprintf(buf, SYSVIOLET "Chown_OK Key:" SYSPURPLE " %s",
            unparse_boolexp(OWNER(player), get_property_lock(thing, CHLK_PROP),
                            1));
    anotify_nolisten(player, buf, 1);

    sprintf(buf, SYSVIOLET "Container Key:" SYSPURPLE " %s",
            unparse_boolexp(OWNER(player), get_property_lock(thing, "_/clk"),
                            1));
    anotify_nolisten(player, buf, 1);

    sprintf(buf, SYSVIOLET "Force Key:" SYSPURPLE " %s",
            unparse_boolexp(OWNER(player), get_property_lock(thing, "@/flk"),
                            1));
    anotify_nolisten(player, buf, 1);

    if (GETSUCC(thing)) {
        sprintf(buf, SYSAQUA "Success:" SYSCYAN " %s",
                tct(GETSUCC(thing), buf2));
        anotify_nolisten(player, buf, 1);
    }
    if (GETFAIL(thing)) {
        sprintf(buf, SYSAQUA "Fail:" SYSCYAN " %s", tct(GETFAIL(thing), buf2));
        anotify_nolisten(player, buf, 1);
    }
    if (GETDROP(thing)) {
        sprintf(buf, SYSAQUA "Drop:" SYSCYAN " %s", tct(GETDROP(thing), buf2));
        anotify_nolisten(player, buf, 1);
    }
    if (GETOSUCC(thing)) {
        sprintf(buf, SYSAQUA "Osuccess:" SYSCYAN " %s",
                tct(GETOSUCC(thing), buf2));
        anotify_nolisten(player, buf, 1);
    }
    if (GETOFAIL(thing)) {
        sprintf(buf, SYSAQUA "Ofail:" SYSCYAN " %s",
                tct(GETOFAIL(thing), buf2));
        anotify_nolisten(player, buf, 1);
    }
    if (GETODROP(thing)) {
        sprintf(buf, SYSAQUA "Odrop:" SYSCYAN " %s",
                tct(GETODROP(thing), buf2));
        anotify_nolisten(player, buf, 1);
    }

    if (tp_who_doing && GETDOING(thing)) {
        sprintf(buf, SYSAQUA "Doing:" SYSCYAN " %s",
                tct(GETDOING(thing), buf2));
        anotify_nolisten(player, buf, 1);
    }
    if (GETOECHO(thing)) {
        sprintf(buf, SYSAQUA "Oecho:" SYSCYAN " %s",
                tct(GETOECHO(thing), buf2));
        anotify_nolisten(player, buf, 1);
    }
    if ((Typeof(thing) == TYPE_THING) && (FLAGS(thing) & ZOMBIE)
        && GETPECHO(thing)) {
        sprintf(buf, SYSAQUA "Pecho:" SYSCYAN " %s",
                tct(GETPECHO(thing), buf2));
        anotify_nolisten(player, buf, 1);
    }

    /* Timestamps */
    /* ex: time_tm = localtime((time_t *)(&(DBFETCH(thing)->ts.created))); */

    time_tm = localtime((&(DBFETCH(thing)->ts.created)));
    ref_tm = (DBFETCH(thing)->ts.dcreated);
    (void) format_time(buf, BUFFER_LEN,
                       (char *) SYSFOREST "Created:" SYSGREEN
                       "  %a %b %e %T %Z %Y", time_tm);
    sprintf(buf, "%s by %s", buf, ref_tm < 0 ? "*NOTHING*" : ansi_unparse_object(OWNER(player), ref_tm));
    anotify_nolisten(player, buf, 1);
    time_tm = localtime((&(DBFETCH(thing)->ts.modified)));
    ref_tm = (DBFETCH(thing)->ts.dmodified);
    (void) format_time(buf, BUFFER_LEN,
                       (char *) SYSFOREST "Modified:" SYSGREEN
                       " %a %b %e %T %Z %Y", time_tm);
    sprintf(buf, "%s by %s", buf, ref_tm < 0 ? "*NOTHING*" : ansi_unparse_object(OWNER(player), ref_tm));
    anotify_nolisten(player, buf, 1);
    time_tm = localtime((&(DBFETCH(thing)->ts.lastused)));
    ref_tm = (DBFETCH(thing)->ts.dlastused);
    (void) format_time(buf, BUFFER_LEN,
                       (char *) SYSFOREST "Lastused:" SYSGREEN
                       " %a %b %e %T %Z %Y", time_tm);
    sprintf(buf, "%s by %s", buf, ref_tm < 0 ? "*NOTHING*" : ansi_unparse_object(OWNER(player), ref_tm));
    anotify_nolisten(player, buf, 1);
    if (Typeof(thing) == TYPE_PROGRAM) {
        sprintf(buf, SYSFOREST "Usecount:" SYSGREEN " %d     "
                SYSFOREST "Instances:" SYSGREEN " %d",
                DBFETCH(thing)->ts.usecount,
                DBFETCH(thing)->sp.program.instances);
    } else
        sprintf(buf, SYSFOREST "Usecount:" SYSGREEN " %d",
                DBFETCH(thing)->ts.usecount);
    anotify_nolisten(player, buf, 1);

    sprintf(buf, SYSVIOLET "In Memory:" SYSPURPLE " %d bytes",
            size_object(thing, 0));
    anotify_nolisten(player, buf, 1);

    anotify_nolisten(player, SYSYELLOW
                     "[ Use 'examine <object>=/' to list root properties. ]",
                     1);

    /* show him the contents */
    if (DBFETCH(thing)->contents != NOTHING) {
        if (Typeof(thing) == TYPE_PLAYER)
            anotify_nolisten(player, SYSBLUE "Carrying:", 1);
        else
            anotify_nolisten(player, SYSBLUE "Contents:", 1);
        DOLIST(content, DBFETCH(thing)->contents) {
            anotify_nolisten(player,
                             ansi_unparse_object(OWNER(player), content), 1);
        }
    }
    switch (Typeof(thing)) {
        case TYPE_ROOM:
            /* tell him about exits */
            if (DBFETCH(thing)->exits != NOTHING) {
                anotify_nolisten(player, SYSBLUE "Exits:", 1);
                DOLIST(exit, DBFETCH(thing)->exits) {
                    strcpy(buf, ansi_unparse_object(OWNER(player), exit));
                    anotify_fmt(player, "%s " SYSCYAN "to %s", buf,
                                ansi_unparse_object(OWNER(player),
                                                    DBFETCH(exit)->sp.exit.
                                                    ndest >
                                                    0 ? DBFETCH(exit)->sp.exit.
                                                    dest[0] : NOTHING)
                        );
                }
            } else {
                anotify_nolisten(player, SYSBLUE "No exits.", 1);
            }

            /* print dropto if present */
            if (DBFETCH(thing)->sp.room.dropto != NOTHING) {
                sprintf(buf, SYSAQUA "Dropped objects go to: %s",
                        ansi_unparse_object(OWNER(player),
                                            DBFETCH(thing)->sp.room.dropto));
                anotify_nolisten(player, buf, 1);
            }
            break;
        case TYPE_THING:
            /* print home */
            sprintf(buf, SYSAQUA "Home: %s", ansi_unparse_object(OWNER(player), DBFETCH(thing)->sp.thing.home)); /* home */
            anotify_nolisten(player, buf, 1);
            /* print location if player can link to it */
            if (DBFETCH(thing)->location != NOTHING
                && (controls(OWNER(player), DBFETCH(thing)->location)
                    || can_link_to(OWNER(player), NOTYPE,
                                   DBFETCH(thing)->location))) {
                sprintf(buf, SYSAQUA "Location: %s",
                        ansi_unparse_object(OWNER(player),
                                            DBFETCH(thing)->location));
                anotify_nolisten(player, buf, 1);
            }
            /* print thing's actions, if any */
            if (DBFETCH(thing)->exits != NOTHING) {
                anotify_nolisten(player, SYSBLUE "Actions/exits:", 1);
                DOLIST(exit, DBFETCH(thing)->exits) {
                    strcpy(buf, ansi_unparse_object(OWNER(player), exit));
                    anotify_fmt(player, "%s " SYSCYAN "to %s", buf,
                                ansi_unparse_object(OWNER(player),
                                                    DBFETCH(exit)->sp.exit.
                                                    ndest >
                                                    0 ? DBFETCH(exit)->sp.exit.
                                                    dest[0] : NOTHING)
                        );
                }
            } else {
                anotify_nolisten(player, SYSBLUE "No actions attached.", 1);
            }
            break;
        case TYPE_PLAYER:

            /* print home */
            sprintf(buf, SYSAQUA "Home: %s", ansi_unparse_object(OWNER(player), DBFETCH(thing)->sp.player.home)); /* home */
            anotify_nolisten(player, buf, 1);

            /* print location if player can link to it */
            if (DBFETCH(thing)->location != NOTHING
                && (controls(OWNER(player), DBFETCH(thing)->location)
                    || can_link_to(OWNER(player), NOTYPE,
                                   DBFETCH(thing)->location))) {
                sprintf(buf, SYSAQUA "Location: %s",
                        ansi_unparse_object(OWNER(player),
                                            DBFETCH(thing)->location));
                anotify_nolisten(player, buf, 1);
            }
            /* print player's actions, if any */
            if (DBFETCH(thing)->exits != NOTHING) {
                anotify_nolisten(player, SYSBLUE "Actions/exits:", 1);
                DOLIST(exit, DBFETCH(thing)->exits) {
                    strcpy(buf, ansi_unparse_object(OWNER(player), exit));
                    anotify_fmt(player, "%s " SYSCYAN "to %s", buf,
                                ansi_unparse_object(player,
                                                    DBFETCH(exit)->sp.exit.
                                                    ndest >
                                                    0 ? DBFETCH(exit)->sp.exit.
                                                    dest[0] : NOTHING)
                        );
                }
            } else {
                anotify_nolisten(player, SYSBLUE "No actions attached.", 1);
            }
            break;
        case TYPE_EXIT:
            if (DBFETCH(thing)->location != NOTHING) {
                sprintf(buf, SYSAQUA "Source: %s",
                        ansi_unparse_object(OWNER(player),
                                            DBFETCH(thing)->location));
                anotify_nolisten(player, buf, 1);
            }
            /* print destinations */
            if (DBFETCH(thing)->sp.exit.ndest == 0)
                break;
            for (i = 0; i < DBFETCH(thing)->sp.exit.ndest; i++) {
                switch ((DBFETCH(thing)->sp.exit.dest)[i]) {
                    case NOTHING:
                        break;
                    case HOME:
                        anotify_nolisten(player, SYSAQUA "Destination: *HOME*",
                                         1);
                        break;
                    default:
                        sprintf(buf, SYSAQUA "Destination: %s",
                                ansi_unparse_object(OWNER(player),
                                                    (DBFETCH(thing)->sp.exit.
                                                     dest)[i]));
                        anotify_nolisten(player, buf, 1);
                        break;
                }
            }
            break;
        case TYPE_PROGRAM:
            if (DBFETCH(thing)->sp.program.siz) {
                struct timeval tv = DBFETCH(thing)->sp.program.proftime;

                sprintf(buf, SYSVIOLET "Program compiled size: " SYSPURPLE
                        "%d instructions", DBFETCH(thing)->sp.program.siz);
                anotify_nolisten(player, buf, 1);
                sprintf(buf, SYSVIOLET "Cumulative runtime: " SYSPURPLE
                        "%ld.%06ld seconds", tv.tv_sec, (long int)tv.tv_usec);
                anotify_nolisten(player, buf, 1);
            } else {
                anotify_nolisten(player, SYSVIOLET "Program not compiled.", 1);
            }

            /* print location if player can link to it */
            if (DBFETCH(thing)->location != NOTHING
                && (controls(OWNER(player), DBFETCH(thing)->location)
                    || can_link_to(OWNER(player), NOTYPE,
                                   DBFETCH(thing)->location))) {
                sprintf(buf, SYSAQUA "Location: %s",
                        ansi_unparse_object(OWNER(player),
                                            DBFETCH(thing)->location));
                anotify_nolisten(player, buf, 1);
            }
            break;
        default:
            /* do nothing */
            break;
    }
}


void
do_score(dbref player)
{
    char buf[BUFFER_LEN];

    sprintf(buf, CINFO "You have %d %s.", DBFETCH(player)->sp.player.pennies,
            DBFETCH(player)->sp.player.pennies == 1 ? tp_penny : tp_pennies);

    anotify_nolisten(player, buf, 1);
}

void
do_inventory(dbref player)
{
    dbref thing;

    if ((thing = DBFETCH(player)->contents) == NOTHING &&
        !count_details(player, player, "_obj")) {
        anotify_nolisten(player, SYSBLUE "You aren't carrying anything.", 1);
    } else {
        anotify_nolisten(player, SYSBLUE "You are carrying:", 1);
        if (thing != NOTHING) {
            DOLIST(thing, thing) {
                anotify_nolisten(player, ansi_unparse_object(player, thing), 1);
            }
        }
        look_details(player, player, "_obj");
    }

    do_score(player);
}

char
init_checkflags(dbref player, const char *flags,
                struct flgchkdat *check)
{
    char buf[BUFFER_LEN];
    char *cptr;
    char output_type = 0;
    char mode = 0;
    bool inflags = 1;

    strcpy(buf, flags);
    for (cptr = buf; *cptr && (*cptr != '='); cptr++) ;
    if (*cptr == '=')
        *(cptr++) = '\0';
    flags = buf;

    while (*cptr && isspace(*cptr))
        cptr++;

    if (!*cptr) {
        output_type = 0;
    } else if (string_prefix("owners", cptr)) {
        output_type = 1;
    } else if (string_prefix("locations", cptr)) {
        output_type = 3;
    } else if (string_prefix("links", cptr)) {
        output_type = 2;
    } else if (string_prefix("count", cptr)) {
        output_type = 4;
    } else if (string_prefix("size", cptr)) {
        output_type = 5;
    } else {
        output_type = 0;
    }

    check->fortype = 0;
    check->istype = 0;
    check->isnotroom = 0;
    check->isnotexit = 0;
    check->isnotthing = 0;
    check->isnotplayer = 0;
    check->isnotprog = 0;
    check->setflags = 0;
    check->clearflags = 0;
    check->setflag2 = 0;
    check->clearflag2 = 0;
    check->setpowers = 0;
    check->clearpowers = 0;

    check->forlevel = 0;
    check->islevel = 0;
    check->isnotzero = 0;
    check->isnotone = 0;
    check->isnottwo = 0;
    check->isnotthree = 0;
    check->isnotfour = 0;
    check->isnotfive = 0;
    check->isnotsix = 0;
    check->isnotseven = 0;
    check->isnoteight = 0;
    check->isnotnine = 0;
    check->isnotwiz = 0;
    check->iswiz = 0;

    check->anypower = 0;
    check->forlink = 0;
    check->islinked = 0;
    check->forold = 0;
    check->isold = 0;

    check->loadedsize = 0;
    check->issize = 0;
    check->size = 0;

    while (*flags) {
        if (*flags == ':') {
            inflags = !(inflags);
            check->anypower = 1;
        } else {
            switch (inflags ? UPCASE(*flags) : DOWNCASE(*flags)) {
                case '!':
                    if (mode)
                        mode = 0;
                    else
                        mode = 2;
                    break;
                case 'R':
                    if (mode) {
                        check->isnotroom = 1;
                    } else {
                        check->fortype = 1;
                        check->istype = TYPE_ROOM;
                    }
                    break;
                case 'T':
                    if (mode) {
                        check->isnotthing = 1;
                    } else {
                        check->fortype = 1;
                        check->istype = TYPE_THING;
                    }
                    break;
                case 'E':
                    if (mode) {
                        check->isnotexit = 1;
                    } else {
                        check->fortype = 1;
                        check->istype = TYPE_EXIT;
                    }
                    break;
                case 'P':
                    if (mode) {
                        check->isnotplayer = 1;
                    } else {
                        check->fortype = 1;
                        check->istype = TYPE_PLAYER;
                    }
                    break;
                case 'F':
                    if (mode) {
                        check->isnotprog = 1;
                    } else {
                        check->fortype = 1;
                        check->istype = TYPE_PROGRAM;
                    }
                    break;
                case '^':
                    check->loadedsize = (Mage(OWNER(player)) && *flags == '^');
                    check->size = atoi(flags + 1);
                    check->issize = !mode;
                    while (isdigit(flags[1]))
                        flags++;
                    break;
                case 'U':
                    check->forlink = 1;
                    if (mode) {
                        check->islinked = 1;
                    } else {
                        check->islinked = 0;
                    }
                    break;
                case '@':
                    check->forold = 1;
                    if (mode) {
                        check->isold = 0;
                    } else {
                        check->isold = 1;
                    }
                    break;
                case '0':
                    if (mode) {
                        check->isnotzero = 1;
                    } else {
                        check->forlevel = 1;
                        check->islevel = 0;
                    }
                    break;
                case '1':
                    if (mode) {
                        check->isnotone = 1;
                    } else {
                        check->forlevel = 1;
                        check->islevel = 1;
                    }
                    break;
                case '2':
                    if (mode) {
                        check->isnottwo = 1;
                    } else {
                        check->forlevel = 1;
                        check->islevel = 2;
                    }
                    break;
                case '3':
                    if (mode) {
                        check->isnotthree = 1;
                    } else {
                        check->forlevel = 1;
                        check->islevel = 3;
                    }
                    break;
                case '4':
                    if (mode) {
                        check->isnotfour = 1;
                    } else {
                        check->forlevel = 1;
                        check->islevel = 4;
                    }
                    break;
                case '5':
                    if (mode) {
                        check->isnotfive = 1;
                    } else {
                        check->forlevel = 1;
                        check->islevel = tp_multi_wizlevels ? 5 : 4;
                    }
                    break;
                case '6':
                    if (mode) {
                        check->isnotsix = 1;
                    } else {
                        check->forlevel = 1;
                        check->islevel = tp_multi_wizlevels ? 6 : 7;
                    }
                    break;
                case '7':
                    if (mode) {
                        check->isnotseven = 1;
                    } else {
                        check->forlevel = 1;
                        check->islevel = 7;
                    }
                    break;
                case '8':
                    if (mode) {
                        check->isnoteight = 1;
                    } else {
                        check->forlevel = 1;
                        check->islevel = tp_multi_wizlevels ? 8 : 7;
                    }
                    break;
                case '9':
                    if (mode) {
                        check->isnotnine = 1;
                    } else {
                        check->forlevel = 1;
                        check->islevel = 9;
                    }
                    break;
                case 'M':
                    if (mode) {
                        check->forlevel = 1;
                        check->islevel = 0;
                    } else {
                        check->isnotzero = 1;
                    }
                    break;
                case 'A':
                    if (mode)
                        check->clearflags |= ABODE;
                    else
                        check->setflags |= ABODE;
                    break;
                case 'B':
                    if (mode)
                        check->clearflags |= BUILDER;
                    else
                        check->setflags |= BUILDER;
                    break;
                case 'C':
                    if (mode)
                        check->clearflags |= CHOWN_OK;
                    else
                        check->setflags |= CHOWN_OK;
                    break;
                case 'D':
                    if (mode)
                        check->clearflags |= DARK;
                    else
                        check->setflags |= DARK;
                    break;
                case 'H':
                    if (mode)
                        check->clearflags |= HAVEN;
                    else
                        check->setflags |= HAVEN;
                    break;
                case 'J':
                    if (mode)
                        check->clearflags |= JUMP_OK;
                    else
                        check->setflags |= JUMP_OK;
                    break;
                case 'L':
                    if (mode)
                        check->clearflags |= LINK_OK;
                    else
                        check->setflags |= LINK_OK;
                    break;
                case 'Q':
                    if (mode)
                        check->clearflags |= QUELL;
                    else
                        check->setflags |= QUELL;
                    break;
                case 'S':
                    if (mode)
                        check->clearflags |= STICKY;
                    else
                        check->setflags |= STICKY;
                    break;
                case 'V':
                    if (mode)
                        check->clearflags |= VEHICLE;
                    else
                        check->setflags |= VEHICLE;
                    break;
                case 'Z':
                    if (mode)
                        check->clearflags |= ZOMBIE;
                    else
                        check->setflags |= ZOMBIE;
                    break;
                case 'W':
                    if (mode)
                        check->isnotwiz = 1;
                    else
                        check->iswiz = 1;
                    break;
                case 'X':
                    if (mode)
                        check->clearflags |= XFORCIBLE;
                    else
                        check->setflags |= XFORCIBLE;
                    break;
                case 'G':
                    if (mode)
                        check->clearflag2 |= F2GUEST;
                    else
                        check->setflag2 |= F2GUEST;
                    break;
                case '%':
                    if (mode)
                        check->clearflag2 |= F2PARENT;
                    else
                        check->setflag2 |= F2PARENT;
                    break;
                case '#':
                    if (mode)
                        check->clearflag2 |= F2HIDDEN;
                    else
                        check->setflag2 |= F2HIDDEN;
                    break;
                case 'O':
                    if (mode)
                        check->clearflag2 |= F2LIGHT;
                    else
                        check->setflag2 |= F2LIGHT;
                    break;
                case '*':
                    if (mode)
                        check->clearflag2 |= F2PROTECT;
                    else
                        check->setflag2 |= F2PROTECT;
                    break;
                case '+':
                    if (mode)
                        check->clearflag2 |= F2MUFCOUNT;
                    else
                        check->setflag2 |= F2MUFCOUNT;
                    break;
                case 'K':
                    if (mode)
                        check->clearflag2 |= F2ANTIPROTECT;
                    else
                        check->setflag2 |= F2ANTIPROTECT;
                    break;
                case 'I':
                    if (mode)
                        check->clearflag2 |= F2IDLE;
                    else
                        check->setflag2 |= F2IDLE;
                    break;
                case 'N':
                    if (mode)
                        check->clearflag2 |= F2NO_COMMAND;
                    else
                        check->setflag2 |= F2NO_COMMAND;
                    break;
                case 'Y':
                    if (mode)
                        check->clearflag2 |= F2EXAMINE_OK;
                    else
                        check->setflag2 |= F2EXAMINE_OK;
                    break;
                case '?':
                    if (mode)
                        check->clearflag2 |= F2MOBILE;
                    else
                        check->setflag2 |= F2MOBILE;
                    break;
#ifdef CONTROLS_SUPPORT
                case '~':
                    if (mode)
                        check->clearflag2 |= F2CONTROLS;
                    else
                        check->setflag2 |= F2CONTROLS;
                    break;
#endif /* CONTROLS_SUPPORT */
                case '|':
                    if (mode)
                        check->clearflag2 |= F2IMMOBILE;
                    else
                        check->setflag2 |= F2IMMOBILE;
                    break;
                case 'a':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_ANNOUNCE;
                    else
                        check->setpowers |= POW_ANNOUNCE;
                    break;
                case 'b':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_BOOT;
                    else
                        check->setpowers |= POW_BOOT;
                    break;
                case 'c':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_CHOWN_ANYTHING;
                    else
                        check->setpowers |= POW_CHOWN_ANYTHING;
                    break;
                case 'x':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_EXPANDED_WHO;
                    else
                        check->setpowers |= POW_EXPANDED_WHO;
                    break;
                case 'h':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_HIDE;
                    else
                        check->setpowers |= POW_HIDE;
                    break;
                case 'i':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_IDLE;
                    else
                        check->setpowers |= POW_IDLE;
                    break;
                case 'l':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_LINK_ANYWHERE;
                    else
                        check->setpowers |= POW_LINK_ANYWHERE;
                    break;
                case 'g':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_LONG_FINGERS;
                    else
                        check->setpowers |= POW_LONG_FINGERS;
                    break;
                case 'n':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_NO_PAY;
                    else
                        check->setpowers |= POW_NO_PAY;
                    break;
                case 'o':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_OPEN_ANYWHERE;
                    else
                        check->setpowers |= POW_OPEN_ANYWHERE;
                    break;
                case 'p':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_PLAYER_CREATE;
                    else
                        check->setpowers |= POW_PLAYER_CREATE;
                    break;
                case 'u':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_PLAYER_PURGE;
                    else
                        check->setpowers |= POW_PLAYER_PURGE;
                case 's':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_SEARCH;
                    else
                        check->setpowers |= POW_SEARCH;
                    break;
                case 'e':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_SEE_ALL;
                    else
                        check->setpowers |= POW_SEE_ALL;
                    break;
                case 't':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_TELEPORT;
                    else
                        check->setpowers |= POW_TELEPORT;
                    break;
                case 'd':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_SHUTDOWN;
                    else
                        check->setpowers |= POW_SHUTDOWN;
                    break;
                case 'f':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_CONTROL_MUF;
                    else
                        check->setpowers |= POW_CONTROL_MUF;
                    break;
                case 'r':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_CONTROL_ALL;
                    else
                        check->setpowers |= POW_CONTROL_ALL;
                    break;
                case 'm':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_ALL_MUF_PRIMS;
                    else
                        check->setpowers |= POW_ALL_MUF_PRIMS;
                    break;
                case 'w':
                    check->anypower = 0;
                    if (mode)
                        check->clearpowers |= POW_STAFF;
                    else
                        check->setpowers |= POW_STAFF;
                    break;
                case ' ':
                    if (mode)
                        mode = 2;
                    break;
            }
        }
        if (mode)
            mode--;
        flags++;
    }
    return output_type;
}


bool
checkflags(dbref what, struct flgchkdat check)
{
    if (check.fortype && (Typeof(what) != check.istype))
        return (0);
    if (check.isnotroom && (Typeof(what) == TYPE_ROOM))
        return (0);
    if (check.isnotexit && (Typeof(what) == TYPE_EXIT))
        return (0);
    if (check.isnotthing && (Typeof(what) == TYPE_THING))
        return (0);
    if (check.isnotplayer && (Typeof(what) == TYPE_PLAYER))
        return (0);
    if (check.isnotprog && (Typeof(what) == TYPE_PROGRAM))
        return (0);

    if (check.forlevel && (MLevel(what) != check.islevel))
        return (0);
    if (check.isnotzero && (MLevel(what) == 0))
        return (0);
    if (check.isnotone && (MLevel(what) >= 1))
        return (0);
    if (check.isnottwo && (MLevel(what) >= 2))
        return (0);
    if (check.isnotthree && (MLevel(what) >= 3))
        return (0);
    if (check.isnotfour && (MLevel(what) >= 4))
        return (0);
    if (check.isnotfive && (MLevel(what) >= 5))
        return (0);
    if (check.isnotsix && (MLevel(what) >= 6))
        return (0);
    if (check.isnotseven && (MLevel(what) >= 7))
        return (0);
    if (check.isnoteight && (MLevel(what) >= 8))
        return (0);
    if (check.isnotnine && (MLevel(what) >= 9))
        return (0);
    if (check.isnotwiz && (MLevel(what) >= LMAGE))
        return (0);
    if (check.iswiz && (MLevel(what) < LMAGE))
        return (0);

    if (FLAGS(what) & check.clearflags)
        return (0);
    if ((~FLAGS(what)) & check.setflags)
        return (0);

    if (FLAG2(what) & check.clearflag2)
        return (0);
    if ((~FLAG2(what)) & check.setflag2)
        return (0);

    if (Typeof(what) == TYPE_PLAYER) {
        if (POWERS(what) & check.clearpowers)
            return (0);
        if ((~POWERS(what)) & check.setpowers)
            return (0);
        if (check.anypower)
            if (!(POWERS(what)))
                return (0);
    } else {
        if (check.anypower || check.clearpowers || check.setpowers)
            return (0);
    }

    if (check.forlink) {
        switch (Typeof(what)) {
            case TYPE_ROOM:
                if ((DBFETCH(what)->sp.room.dropto == NOTHING) !=
                    (!check.islinked))
                    return (0);
                break;
            case TYPE_EXIT:
                if ((!DBFETCH(what)->sp.exit.ndest) != (!check.islinked))
                    return (0);
                break;
            case TYPE_PLAYER:
            case TYPE_THING:
                if (!check.islinked)
                    return (0);
                break;
            default:
                if (check.islinked)
                    return (0);
        }
    }
    if (check.forold) {
        if (((((current_systime) - DBFETCH(what)->ts.lastused) < tp_aging_time)
             || (((current_systime) - DBFETCH(what)->ts.modified) <
                 tp_aging_time))
            != (!check.isold))
            return (0);
    }
    if (check.size) {
        if ((size_object(what, check.loadedsize) < check.size)
            != (!check.issize))
            return 0;
    }
    return (1);
}


void
display_objinfo(dbref player, dbref obj,
                char output_type)
{
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];

    strcpy(buf2, ansi_unparse_object(player, obj));
    strcat(buf2, NORMAL);

    switch (output_type) {
        case 1:                /* owners */
            sprintf(buf, "%-38.512s  %.512s", buf2,
                    ansi_unparse_object(player, OWNER(obj)));
            break;
        case 2:                /* links */
            switch (Typeof(obj)) {
                case TYPE_ROOM:
                    sprintf(buf, "%-38.512s  %.512s", buf2,
                            ansi_unparse_object(player,
                                                DBFETCH(obj)->sp.room.dropto));
                    break;
                case TYPE_EXIT:
                    if (DBFETCH(obj)->sp.exit.ndest == 0) {
                        sprintf(buf, "%-38.512s  %.512s", buf2, "*UNLINKED*");
                        break;
                    }
                    if (DBFETCH(obj)->sp.exit.ndest > 1) {
                        sprintf(buf, "%-38.512s  %.512s", buf2, "*METALINKED*");
                        break;
                    }
                    sprintf(buf, "%-38.512s  %.512s", buf2,
                            ansi_unparse_object(player,
                                                DBFETCH(obj)->sp.exit.dest[0]));
                    break;
                case TYPE_PLAYER:
                    sprintf(buf, "%-38.512s  %.512s", buf2,
                            ansi_unparse_object(player,
                                                DBFETCH(obj)->sp.player.home));
                    break;
                case TYPE_THING:
                    sprintf(buf, "%-38.512s  %.512s", buf2,
                            ansi_unparse_object(player,
                                                DBFETCH(obj)->sp.thing.home));
                    break;
                default:
                    sprintf(buf, "%-38.512s  %.512s", buf2, "N/A");
                    break;
            }
            break;
        case 3:                /* locations */
            sprintf(buf, "%-38.512s  %.512s", buf2,
                    ansi_unparse_object(player, DBFETCH(obj)->location));
            break;
        case 4:
            return;
        case 5:
            sprintf(buf, "%-38.512s  %d bytes.", buf2, size_object(obj, 0));
            break;
        case 0:
        default:
            strcpy(buf, buf2);
            break;
    }
    anotify_nolisten(player, buf, 1);
}


void
do_find(dbref player, const char *name, const char *flags)
{
    char output_type;
    char buf[BUFFER_LEN + 2];
    int total = 0;
    struct flgchkdat check;
    dbref i;

    if (Guest(player)) {
        anotify_fmt(player, CFAIL "%s", tp_noguest_mesg);
        return;
    }

    strcpy(buf, "*");
    strcat(buf, name);
    strcat(buf, "*");

    if (!payfor(player, tp_lookup_cost)) {
        anotify_fmt(player, CFAIL "You don't have enough %s.", tp_pennies);
    } else {
        output_type = init_checkflags(player, flags, &check);
        for (i = 0; i < db_top; i++) {
            if ((Wiz(OWNER(player)) || (POWERS(player) & POW_SEARCH)
                 || OWNER(i) == OWNER(player)) && checkflags(i, check)
                && NAME(i) && (!*name || equalstr(buf, (char *) NAME(i)))) {
                display_objinfo(player, i, output_type);
                total++;
            }
        }

        anotify_nolisten(player, CINFO "***End of List***", 1);
        anotify_fmt(player, CSUCC "%d objects found.", total);
    }
}


void
do_owned(dbref player, const char *name, const char *flags)
{
    char output_type;
    dbref victim, i;
    struct flgchkdat check;
    int total = 0;

    if (Guest(player)) {
        anotify_fmt(player, CFAIL "%s", tp_noguest_mesg);
        return;
    }

    if (!payfor(player, tp_lookup_cost)) {
        anotify_fmt(player, CFAIL "You don't have enough %s.", tp_pennies);
        return;
    }

    if ((Mage(OWNER(player)) && *name) || (POWERS(player) & POW_SEARCH)) {
        if ((victim = strcmp(name, "me") ? lookup_player(name) : player)
            == NOTHING) {
            anotify_nolisten(player, CINFO "Who?", 1);
            return;
        }
    } else
        victim = player;

    output_type = init_checkflags(player, flags, &check);

    for (i = 0; i < db_top; i++) {
        if ((OWNER(i) == OWNER(victim)) && checkflags(i, check)) {
            display_objinfo(player, i, output_type);
            total++;
        }
    }

    anotify_nolisten(player, CINFO "***End of List***", 1);
    anotify_fmt(player, CSUCC "%d objects found.", total);
}

void
do_trace(int descr, dbref player, const char *name, int depth)
{
    struct match_data md;
    dbref thing;
    int i;

    if (Guest(player)) {
        anotify_fmt(player, CFAIL "%s", tp_noguest_mesg);
        return;
    }

    init_match(descr, player, name, NOTYPE, &md);
    match_absolute(&md);
    match_here(&md);
    match_me(&md);
    match_neighbor(&md);
    match_possession(&md);
    match_registered(&md);

    if ((thing = noisy_match_result(&md)) == NOTHING || thing == AMBIGUOUS)
        return;

    for (i = 0; (!depth || i < depth) && thing != NOTHING; i++) {
        if (controls(player, thing) || can_link_to(player, NOTYPE, thing))
            anotify_nolisten(player, ansi_unparse_object(player, thing), 1);
        else
            anotify_nolisten(player, CINFO "**Missing**", 1);
        thing = DBFETCH(thing)->location;
    }
    anotify_nolisten(player, CINFO "***End of List***", 1);
}

void
do_entrances(int descr, dbref player, const char *name,
             const char *flags)
{
    dbref i, thing;
    dbref j;
    struct match_data md;
    struct flgchkdat check;
    int total = 0;
    char output_type;

    if (Guest(player)) {
        anotify_fmt(player, CFAIL "%s", tp_noguest_mesg);
        return;
    }

    if (*name == '\0') {
        thing = getloc(player);
    } else {
        init_match(descr, player, name, NOTYPE, &md);
        match_all_exits(&md);
        match_neighbor(&md);
        match_possession(&md);
        match_registered(&md);
        if (Mage(OWNER(player)) || POWERS(player) & POW_SEARCH) {
            match_absolute(&md);
            match_player(&md);
        }
        match_here(&md);
        match_me(&md);

        thing = noisy_match_result(&md);
    }

    if (thing == NOTHING) {
        anotify_nolisten(player, CINFO "I don't know what object you mean.", 1);
        return;
    }

    if (!controls(OWNER(player), thing) && !(POWERS(player) & POW_SEARCH)) {
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
        return;
    }

    output_type = init_checkflags(player, flags, &check);

    for (i = 0; i < db_top; i++) {
        if (checkflags(i, check)) {
            switch (Typeof(i)) {
                case TYPE_EXIT:
                    for (j = DBFETCH(i)->sp.exit.ndest; j--;) {
                        if (DBFETCH(i)->sp.exit.dest[j] == thing) {
                            display_objinfo(player, i, output_type);
                            total++;
                        }
                    }
                    break;
                case TYPE_PLAYER:
                    if (DBFETCH(i)->sp.player.home == thing) {
                        display_objinfo(player, i, output_type);
                        total++;
                    }
                    break;
                case TYPE_THING:
                    if (DBFETCH(i)->sp.thing.home == thing) {
                        display_objinfo(player, i, output_type);
                        total++;
                    }
                    break;
                case TYPE_ROOM:
                    if (DBFETCH(i)->sp.room.dropto == thing) {
                        display_objinfo(player, i, output_type);
                        total++;
                    }
                    break;
                case TYPE_PROGRAM:
                case TYPE_GARBAGE:
                    break;
            }
        }
    }

    anotify_nolisten(player, CINFO "***End of List***", 1);
    anotify_fmt(player, CSUCC "%d objects found.", total);
}

void
do_contents(int descr, dbref player, const char *name,
            const char *flags)
{
    dbref thing;
    dbref i;
    struct match_data md;
    struct flgchkdat check;
    int total = 0;
    char output_type;

    if (Guest(player)) {
        anotify_fmt(player, CFAIL "%s", tp_noguest_mesg);
        return;
    }

    if (*name == '\0') {
        thing = getloc(player);
    } else {
        init_match(descr, player, name, NOTYPE, &md);
        match_me(&md);
        match_here(&md);
        match_all_exits(&md);
        match_neighbor(&md);
        match_possession(&md);
        match_registered(&md);
        if (Mage(OWNER(player)) || POWERS(player) & POW_SEARCH) {
            match_absolute(&md);
            match_player(&md);
        }

        thing = noisy_match_result(&md);
    }

    if (thing == NOTHING)
        return;

    if (!controls(OWNER(player), thing) && !(POWERS(player) & POW_SEARCH)) {
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
        return;
    }

    output_type = init_checkflags(player, flags, &check);
    DOLIST(i, DBFETCH(thing)->contents) {
        if (checkflags(i, check)) {
            display_objinfo(player, i, output_type);
            total++;
        }
    }
    switch (Typeof(thing)) {
        case TYPE_EXIT:
        case TYPE_PROGRAM:
        case TYPE_GARBAGE:
            i = NOTHING;
            break;
        case TYPE_ROOM:
        case TYPE_THING:
        case TYPE_PLAYER:
            i = DBFETCH(thing)->exits;
            break;
    }
    DOLIST(i, i) {
        if (checkflags(i, check)) {
            display_objinfo(player, i, output_type);
            total++;
        }
    }
    anotify_nolisten(player, CINFO "***End of List***", 1);
    anotify_fmt(player, CSUCC "%d objects found.", total);
}

static bool
exit_matches_name(dbref exit, const char *name)
{
    char buf[BUFFER_LEN];
    char *ptr, *ptr2;
    
    if (!OkObj(exit)) return 0;
    if (!DBFETCH(exit)->sp.exit.ndest) return 0;
    if (!OkObj((DBFETCH(exit)->sp.exit.dest)[0])) return 0;
    
    strcpy(buf, NAME(exit));
    for (ptr2 = ptr = buf; *ptr; ptr = ptr2) {
        while (*ptr2 && *ptr2 != ';')
            ptr2++;
        if (*ptr2)
            *ptr2++ = '\0';
        while (*ptr2 == ';')
            ptr2++;
        if (string_prefix(name, ptr) && DBFETCH(exit)->sp.exit.ndest &&
            Typeof((DBFETCH(exit)->sp.exit.dest)[0]) == TYPE_PROGRAM)
            return 1;
    }
    return 0;
}

void
exit_match_exists(dbref player, dbref obj, const char *name)
{
    dbref exit;
    char buf[BUFFER_LEN];

    exit = DBFETCH(obj)->exits;
    while (exit != NOTHING) {
        if (exit_matches_name(exit, name)) {
            sprintf(buf, "  %ss are trapped on %.2048s",
                    name, ansi_unparse_object(player, obj));
            anotify_nolisten(player, buf, 1);
        }
        exit = DBFETCH(exit)->next;
    }
}

void
do_sweep(int descr, dbref player, const char *name)
{
    dbref ref;
    dbref thing, loc;
    bool flag, tellflag;
    struct match_data md;
    char buf[BUFFER_LEN];

    if (Guest(player)) {
        anotify_fmt(player, CFAIL "%s", tp_noguest_mesg);
        return;
    }

    if (*name == '\0') {
        thing = getloc(player);
    } else {
        init_match(descr, player, name, NOTYPE, &md);
        match_me(&md);
        match_here(&md);
        match_all_exits(&md);
        match_neighbor(&md);
        match_possession(&md);
        match_registered(&md);
        if (Mage(OWNER(player))) {
            match_absolute(&md);
            match_player(&md);
        }
        thing = noisy_match_result(&md);
    }
    if (thing == NOTHING) {
        anotify_nolisten(player, CINFO "I don't know what object you mean.", 1);
        return;
    }

    if (*name
        && (!controls(OWNER(player), thing) && !(POWERS(OWNER(player)) &
            POW_SEARCH))) {
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
        return;
    }

    sprintf(buf, CINFO "Listeners in %s:", ansi_unparse_object(player, thing));
    anotify_nolisten(player, buf, 1);

    ref = DBFETCH(thing)->contents;
    for (; ref != NOTHING; ref = DBFETCH(ref)->next) {
        switch (Typeof(ref)) {
            case TYPE_PLAYER:
                if ( /* !Dark(thing) || */ online(ref)) {
                    sprintf(buf, "  %s" NORMAL " is a %splayer.",
                            ansi_unparse_object(player, ref),
                            online(ref) ? "" : "sleeping ");
                    anotify_nolisten(player, buf, 1);
                }
                break;
            case TYPE_THING:
                if (FLAGS(ref) & (VEHICLE | ZOMBIE | LISTENER)) {
                    tellflag = 0;
                    sprintf(buf, "  %.255s" NORMAL " is a",
                            ansi_unparse_object(player, ref));
                    if (FLAGS(ref) & ZOMBIE) {
                        tellflag = 1;
                        if (!online(OWNER(ref))) {
                            tellflag = 0;
                            strcat(buf, " sleeping");
                        }
                        strcat(buf, " zombie");
                    }
                    if (FLAGS(ref) & VEHICLE) {
                        tellflag = 1;
                        strcat(buf, " vehicle");
                    }
                    if ((FLAGS(ref) & LISTENER) &&
                        (get_property(ref, "_listen") ||
                         get_property(ref, "_olisten") ||
                         get_property(ref, "~listen") ||
                         get_property(ref, "~olisten") ||
                         get_property(ref, "@olisten") ||
                         get_property(ref, "@listen"))) {
                        strcat(buf, " listener");
                        tellflag = 1;
                    }
                    strcat(buf, " object owned by ");
                    strcat(buf, ansi_unparse_object(player, OWNER(ref)));
                    strcat(buf, NORMAL ".");
                    if (tellflag)
                        anotify_nolisten(player, buf, 1);
                }
                exit_match_exists(player, ref, "page");
                exit_match_exists(player, ref, "whisper");
                exit_match_exists(player, ref, "pose");
                exit_match_exists(player, ref, "say");
                break;
        }
    }
    flag = 0;
    loc = thing;
    while (loc != NOTHING) {
        if (!flag) {
            anotify_nolisten(player,
                             CINFO "Listening rooms down the environment:", 1);
            flag = 1;
        }

        if ((FLAGS(loc) & LISTENER) &&
            (get_property(loc, "@listen") ||
             get_property(loc, "@olisten") ||
             get_property(loc, "~listen") ||
             get_property(loc, "~olisten") ||
             get_property(loc, "_listen") ||
             get_property(loc, "_alisten") ||
             get_property(loc, "_aolisten") ||
             get_property(loc, "~alisten") ||
             get_property(loc, "~aolisten") ||
             get_property(loc, "@alisten") ||
             get_property(loc, "~aolisten") || get_property(loc, "_olisten"))) {
            sprintf(buf, "  %s" NORMAL " is a listening room.",
                    ansi_unparse_object(player, loc));
            anotify_nolisten(player, buf, 1);
        }

        exit_match_exists(player, loc, "page");
        exit_match_exists(player, loc, "whisper");
        exit_match_exists(player, loc, "pose");
        exit_match_exists(player, loc, "say");
        loc = getparent(loc);
    }
    anotify_nolisten(player, CINFO "**End of list**", 1);
}
