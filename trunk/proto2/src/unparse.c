#include "copyright.h"
#include "config.h"

#include "db.h"
#include "externs.h"
#include "params.h"
#include "tune.h"
#include "interface.h"
#include "props.h"

static char upb[BUFFER_LEN];

const char *
unparse_flags(dbref thing, char buf[BUFFER_LEN])
{
    char *p;
    const char *type_codes = "R-EPFG";

    p = buf;
    if (Typeof(thing) != TYPE_THING)
        *p++ = type_codes[Typeof(thing)];
    if (FLAGS(thing) & ~TYPE_MASK) {
        /* print flags */
        if (FLAGS(thing) & BUILDER)
            *p++ = 'B';
        if (FLAGS(thing) & JUMP_OK)
            *p++ = 'J';
        if (FLAGS(thing) & LINK_OK)
            *p++ = 'L';
        if (FLAGS(thing) & DARK)
            *p++ = 'D';
        if (FLAGS(thing) & STICKY)
            *p++ = 'S';
        if (FLAGS(thing) & QUELL)
            *p++ = 'Q';
        if (FLAGS(thing) & CHOWN_OK)
            *p++ = 'C';
        if (FLAGS(thing) & HAVEN)
            *p++ = 'H';
        if (FLAGS(thing) & ABODE)
            *p++ = 'A';
        if (FLAGS(thing) & VEHICLE)
            *p++ = 'V';
        if (FLAGS(thing) & XFORCIBLE)
            *p++ = 'X';
        if (FLAGS(thing) & ZOMBIE)
            *p++ = 'Z';
    }
    if (FLAG2(thing)) {
        if (FLAG2(thing) & F2GUEST)
            *p++ = 'G';
        if (FLAG2(thing) & F2IDLE)
            *p++ = 'I';
        if (FLAG2(thing) & F2LOGWALL)
            *p++ = '!';
        if (FLAG2(thing) & F2MUFCOUNT)
            *p++ = '+';
        if (FLAG2(thing) & F2LIGHT)
            *p++ = 'O';
        if (FLAG2(thing) & F2PARENT)
            *p++ = '%';
        if (FLAG2(thing) & F2PROTECT)
            *p++ = '*';
        if (FLAG2(thing) & F2ANTIPROTECT)
            *p++ = 'K';
        if (FLAG2(thing) & F2EXAMINE_OK)
            *p++ = 'Y';
        if (FLAG2(thing) & F2NO_COMMAND)
            *p++ = 'N';
        if (FLAG2(thing) & F2HIDDEN)
            *p++ = '#';
        if (FLAG2(thing) & F2PUEBLO)
            *p++ = '$';
        if (FLAG2(thing) & F2HTML)
            *p++ = '&';
        if (FLAG2(thing) & F2MOBILE)
            *p++ = '?';
#ifdef CONTROLS_SUPPORT
        if (FLAG2(thing) & F2CONTROLS)
            *p++ = '~';
#endif
    }
    if (thing == 1) {
        *p++ = 'W';
        *p++ = '5';
    } else {
        switch (RawMLevel(thing)) {
            case 0:
                break;
            case LMPI:
                *p++ = 'M';
                break;
            case LMAN:
                *p++ = 'W';
                *p++ = '5';
                break;
            case LBOY:
                *p++ = 'W';
                *p++ = '4';
                break;
            case LARCH:
                *p++ = 'W';
                if (tp_multi_wizlevels) {
                    *p++ = '3';
                }
                break;
            case LWIZ:
                *p++ = 'W';
                *p++ = '2';
                break;
            case LMAGE:
                *p++ = 'W';
                break;
            case LM3:
                *p++ = 'M';
                *p++ = '3';
                break;
            case LM2:
                *p++ = 'M';
                *p++ = '2';
                break;
            case LM1:
                *p++ = 'M';
                *p++ = '1';
                break;
        }
    }
    if ((Typeof(thing) == TYPE_PLAYER) && POWERS(thing)) {
        *p++ = ':';
        if (POWERS(thing) & POW_ANNOUNCE)
            *p++ = 'a';
        if (POWERS(thing) & POW_BOOT)
            *p++ = 'b';
        if (POWERS(thing) & POW_CHOWN_ANYTHING)
            *p++ = 'c';
        if (POWERS(thing) & POW_EXPANDED_WHO)
            *p++ = 'x';
        if (POWERS(thing) & POW_HIDE)
            *p++ = 'h';
        if (POWERS(thing) & POW_IDLE)
            *p++ = 'i';
        if (POWERS(thing) & POW_LINK_ANYWHERE)
            *p++ = 'l';
        if (POWERS(thing) & POW_LONG_FINGERS)
            *p++ = 'g';
        if (POWERS(thing) & POW_NO_PAY)
            *p++ = 'n';
        if (POWERS(thing) & POW_OPEN_ANYWHERE)
            *p++ = 'o';
        if (POWERS(thing) & POW_PLAYER_CREATE)
            *p++ = 'p';
        if (POWERS(thing) & POW_PLAYER_PURGE)
            *p++ = 'u';
        if (POWERS(thing) & POW_SEARCH)
            *p++ = 's';
        if (POWERS(thing) & POW_SEE_ALL)
            *p++ = 'e';
        if (POWERS(thing) & POW_TELEPORT)
            *p++ = 't';
        if (POWERS(thing) & POW_SHUTDOWN)
            *p++ = 'd';
        if (POWERS(thing) & POW_CONTROL_MUF)
            *p++ = 'f';
        if (POWERS(thing) & POW_CONTROL_ALL)
            *p++ = 'r';
        if (POWERS(thing) & POW_ALL_MUF_PRIMS)
            *p++ = 'm';
        if (POWERS(thing) & POW_STAFF)
            *p++ = 'w';
    }
    *p = '\0';
    return buf;
}

char
flag_2char(char *flag)
{
    if (string_prefix("m", flag))
        return 'M';
    if (string_prefix("w", flag))
        return 'W';
    if (string_prefix("builder", flag) || string_prefix("bound", flag))
        return 'B';
    if (string_prefix("jump_ok", flag))
        return 'J';
    if (string_prefix("link_ok", flag))
        return 'L';
    if (string_prefix("dark", flag) || string_prefix("debugging", flag))
        return 'D';
    if (string_prefix("sticky", flag) || string_prefix("silent", flag) ||
        string_prefix("setuid", flag))
        return 'S';
    if (string_prefix("quell", flag))
        return 'Q';
    if (string_prefix("chown_ok", flag) || string_prefix("color_on", flag)
        || string_prefix("ansi", flag))
        return 'C';
    if (string_prefix("haven", flag) || string_prefix("harduid", flag)
        || string_prefix("hide", flag))
        return 'H';
    if (string_prefix("abode", flag) || string_prefix("autostart", flag)
        || string_prefix("abate", flag))
        return 'A';
    if (string_prefix("vehicle", flag) || string_prefix("viewable", flag))
        return 'V';
    if (string_prefix("xforcible", flag) ||
        string_prefix("expanded_debug", flag))
        return 'X';
    if (string_prefix("zombie", flag) || string_prefix("puppet", flag))
        return 'Z';
    if (string_prefix("guest", flag))
        return 'G';
    if (string_prefix("idle", flag))
        return 'I';
#ifdef CONTROLS_SUPPORT
    if (string_prefix("controls", flag))
        return '~';
#endif
    if (string_prefix("mufcount", flag))
        return '+';
    if (string_prefix("light", flag) || string_prefix("oldcomment", flag))
        return 'O';
    if (string_prefix("parent", flag) || string_prefix("prog_debug", flag))
        return '%';
    if (string_prefix("protect", flag))
        return '*';
    if (string_prefix("antiprotect", flag))
        return 'K';
    if (string_prefix("examine_ok", flag))
        return 'Y';
    if (string_prefix("no_command", flag))
        return 'N';
    if (string_prefix("hidden", flag))
        return '#';
    if (string_prefix("pueblo", flag))
        return '$';
    if (string_prefix("nhtml", flag) || string_prefix("html", flag))
        return '&';
    if (string_prefix("meeper", flag) || string_prefix("mpi", flag))
        return LMPI + '0';
    if (string_prefix("mucker", flag) || string_prefix("mucker1", flag) ||
        string_prefix("m1", flag))
        return LM1 + '0';
    if (string_prefix("nucker", flag) || string_prefix("mucker2", flag) ||
        string_prefix("m2", flag))
        return LM2 + '0';
    if (string_prefix("sucker", flag) || string_prefix("mucker3", flag)
        || string_prefix("m3", flag))
        return LM3 + '0';
    if (string_prefix("mage", flag) || string_prefix("W1", flag))
        return (tp_multi_wizlevels ? LMAGE : LM3) + '0';
    if (string_prefix("wizard", flag) || string_prefix("W2", flag))
        return (tp_multi_wizlevels ? LWIZ : LARCH) + '0';
    if (string_prefix("archwizard", flag) || string_prefix("W3", flag))
        return LARCH + '0';
    if (string_prefix("boy", flag) || string_prefix("W4", flag))
        return (tp_multi_wizlevels ? LBOY : LARCH) + '0';
    if (string_prefix("man", flag) || string_prefix("W5", flag))
        return LMAN + '0';
    if (string_prefix("mobile", flag) || string_prefix("offer", flag))
        return '?';

    return 0;
}

char
power_2char(char *p)
{
    if (string_prefix("ALL_MUF_PRIMS", p)) {
        return 'm';
    } else if (string_prefix("ANNOUNCE", p)) {
        return 'a';
    } else if (string_prefix("BOOT", p)) {
        return 'b';
    } else if (string_prefix("CHOWN_ANYTHING", p)) {
        return 'c';
    } else if (string_prefix("CONTROL_ALL", p)) {
        return 'r';
    } else if (string_prefix("CONTROL_MUF", p)) {
        return 'f';
    } else if (string_prefix("EXPANDED_WHO", p)) {
        return 'x';
    } else if (string_prefix("HIDE", p)) {
        return 'h';
    } else if (string_prefix("IDLE", p)) {
        return 'i';
    } else if (string_prefix("LINK_ANYWHERE", p)) {
        return 'l';
    } else if (string_prefix("LONG_FINGERS", p)) {
        return 'g';
    } else if (string_prefix("NO_PAY", p)) {
        return 'n';
    } else if (string_prefix("OPEN_ANYWHERE", p)) {
        return 'o';
    } else if (string_prefix("PLAYER_CREATE", p)) {
        return 'p';
    } else if (string_prefix("SEARCH", p)) {
        return 's';
    } else if (string_prefix("SEE_ALL", p)) {
        return 'e';
    } else if (string_prefix("SHUTDOWN", p)) {
        return 'd';
    } else if (string_prefix("TELEPORT", p)) {
        return 't';
    } else if (string_prefix("STAFF", p)) {
        return 'w';
    } else {
        return 0;
    }
}

const char *
unparse_object(dbref player, dbref loc)
{
    char tbuf[BUFFER_LEN];

    if (Typeof(player) != TYPE_PLAYER)
        player = OWNER(player);
    switch (loc) {
        case NOTHING:
            return "*NOTHING*";
        case AMBIGUOUS:
            return "*AMBIGUOUS*";
        case HOME:
            return "*HOME*";
        default:
            if (loc < 0 || loc > db_top)
#ifdef SANITY
            {
                sprintf(upb, "*INVALID*(#%d)", loc);
                return upb;
            }
#else
                return "*INVALID*";
#endif
#ifndef SANITY
            if (!(FLAGS(player) & STICKY) &&
                (TMage(player) || POWERS(player) & POW_SEE_ALL ||
                 can_link_to(player, NOTYPE, loc) ||
                 controls_link(player, loc) ||
                 ((Typeof(loc) != TYPE_PLAYER) && (FLAGS(loc) & CHOWN_OK))
                )) {
                /* show everything */
#endif
                sprintf(upb, "%s(#%d%s)", NAME(loc), loc,
                        unparse_flags(loc, tbuf));
                return upb;
#ifndef SANITY
            } else {
                /* show only the name */
                return NAME(loc);
            }
#endif
    }
}


const char *
ansiname(dbref loc, char buf[BUFFER_LEN])
{
    char tbuf[BUFFER_LEN];

    *buf = '\0';
    switch (Typeof(loc)) {
        case TYPE_PLAYER:
            strcpy(buf, SYSGREEN);
            break;
        case TYPE_THING:
            strcpy(buf, SYSPURPLE);
            break;
        case TYPE_EXIT:
            strcpy(buf, SYSBLUE);
            break;
        case TYPE_PROGRAM:
            strcpy(buf, SYSRED);
            break;
        case TYPE_ROOM:
            strcpy(buf, SYSCYAN);
            break;
        default:
            strcpy(buf, SYSNORMAL);
    }

    strcat(buf, tct(NAME(loc), tbuf));
    return buf;
}

const char *
ansi_unparse_object(dbref player, dbref loc)
{
    char tbuf[BUFFER_LEN], tbuf2[BUFFER_LEN];

    if (Typeof(player) != TYPE_PLAYER)
        player = OWNER(player);
    switch (loc) {
        case NOTHING:
            return SYSNORMAL "*NOTHING*";
        case AMBIGUOUS:
            return SYSPURPLE "*AMBIGUOUS*";
        case HOME:
            return SYSWHITE "*HOME*";
        default:
            if (loc < 0 || loc > db_top)
                return SYSRED "*INVALID*";
#ifndef SANITY
            if (!(FLAGS(player) & STICKY) &&
                (TMage(player) || POWERS(player) & POW_SEE_ALL ||
                 POWERS(player) & POW_SEARCH ||
                 can_link_to(player, NOTYPE, loc) ||
                 controls_link(player, loc) ||
                 ((Typeof(loc) != TYPE_PLAYER) && (FLAGS(loc) & CHOWN_OK))
                )) {
#endif
                /* show everything */
                sprintf(upb, "%s" SYSYELLOW "(#%d%s)",
                        ansiname(loc, tbuf), loc, unparse_flags(loc, tbuf2));
                return upb;
#ifndef SANITY
            } else {
                /* show only the name */
                return ansiname(loc, upb);
            }
#endif
    }
}

static char boolexp_buf[BUFFER_LEN];
static char *buftop;

static void
unparse_boolexp1(dbref player, struct boolexp *b,
                 boolexp_type outer_type, int fullname)
{
    if ((buftop - boolexp_buf) > (BUFFER_LEN / 2))
        return;
    if (b == TRUE_BOOLEXP) {
        strcpy(buftop, "*UNLOCKED*");
        buftop += strlen(buftop);
    } else {
        switch (b->type) {
            case BOOLEXP_AND:
                if (outer_type == BOOLEXP_NOT) {
                    *buftop++ = '(';
                }
                unparse_boolexp1(player, b->sub1, b->type, fullname);
                *buftop++ = AND_TOKEN;
                unparse_boolexp1(player, b->sub2, b->type, fullname);
                if (outer_type == BOOLEXP_NOT) {
                    *buftop++ = ')';
                }
                break;
            case BOOLEXP_OR:
                if (outer_type == BOOLEXP_NOT || outer_type == BOOLEXP_AND) {
                    *buftop++ = '(';
                }
                unparse_boolexp1(player, b->sub1, b->type, fullname);
                *buftop++ = OR_TOKEN;
                unparse_boolexp1(player, b->sub2, b->type, fullname);
                if (outer_type == BOOLEXP_NOT || outer_type == BOOLEXP_AND) {
                    *buftop++ = ')';
                }
                break;
            case BOOLEXP_NOT:
                *buftop++ = '!';
                unparse_boolexp1(player, b->sub1, b->type, fullname);
                break;
            case BOOLEXP_CONST:
                if (fullname) {
#ifndef SANITY
                    strcpy(buftop, unparse_object(player, b->thing));
#endif
                } else {
                    sprintf(buftop, "#%d", b->thing);
                }
                buftop += strlen(buftop);
                break;
            case BOOLEXP_PROP:
                strcpy(buftop, PropName(b->prop_check));
                strcat(buftop, ":");
                if (PropType(b->prop_check) == PROP_STRTYP)
                    strcat(buftop, PropDataStr(b->prop_check));
                buftop += strlen(buftop);
                break;
            default:
                fprintf(stderr,
                        "WARN: Invalid Bool type. unparseboolexp1().\n");
                /* abort(); *//* bad type */
                break;
        }
    }
}

const char *
unparse_boolexp(dbref player, struct boolexp *b, int fullname)
{
    buftop = boolexp_buf;
    unparse_boolexp1(player, b, BOOLEXP_CONST, fullname); /* no outer type */
    *buftop++ = '\0';

    return boolexp_buf;
}
