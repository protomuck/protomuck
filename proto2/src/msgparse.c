#include "config.h"
#include <math.h>
#include <ctype.h>
#include "params.h"
#include "db.h"
#include "tune.h"

#include "mpi.h"
#include "externs.h"
#include "props.h"
#include "match.h"
#include "interp.h"
#include "interface.h"
#include "msgparse.h"
#include "mfun.h"

#define smnotify(X, Y, Z) { if (OkObj(Y))                       \
                                notify_nolisten(Y, Z, 1);       \
                            else if (X > -1)                    \
                                notify_descriptor(X, Z);        \
                          }

time_t mpi_prof_start_time;

bool
Archperms(register dbref what)
{
    return (Arch(what) && TArch(OWNER(what)));
}


bool
Wizperms(register dbref what)
{
    return (Wiz(what) && TWiz(OWNER(what)));
}


bool
Mageperms(register dbref what)
{
    return (Mage(what) && TMage(OWNER(what)));
}

bool
safeputprop(dbref obj, dbref perms, register const char *buf, const char *val)
{
    register const char *ptr;

    if (!buf || !*buf)
        return 0;

    while (*buf == PROPDIR_DELIMITER)
        buf++;

    if (!*buf || tp_db_readonly)
        return 0;

    /* disallow CR's and :'s in prop names. */
    for (ptr = buf; *ptr; ptr++)
        if (*ptr == '\r' || *ptr == PROP_DELIMITER || !isascii(*ptr)) /* hinoserm */
            return 0;

    if (!Archperms(perms) && Prop_Hidden(buf))
        return 0;

    if (!Wizperms(perms))
        if (Prop_SeeOnly(buf) || string_prefix(buf, "_msgmacs/"))
            return 0;

    if (!val)
        remove_property(obj, buf);
    else
        add_property(obj, buf, val, 0);

    return 1;
}

const char *
safegetprop_limited(dbref player, register dbref what, dbref whom, dbref perms,
                    const char *inbuf)
{
    register const char *ptr;

    while (what != NOTHING) {
        if (OWNER(what) == whom || Wizard(what) ||
            safegetprop_strict(player, what, perms, "~mpi_macros_ok")
            ) {
            ptr = safegetprop_strict(player, what, perms, inbuf);
            if (!ptr || *ptr)
                return ptr;
        }
        what = getparent(what);
    }

    return "";
}

const char *
safegetprop_strict(dbref player, dbref what, dbref perms, const char *inbuf)
{
    register const char *ptr;
    static char vl[16];

    if (!inbuf) {
        notify_nolisten(player, "PropFetch: Propname required.", 1);
        return NULL;
    }

    while (*inbuf == PROPDIR_DELIMITER)
        inbuf++;

    if (!*inbuf) {
        notify_nolisten(player, "PropFetch: Propname required.", 1);
        return NULL;
    }

    if (!Archperms(perms)) {
        if (Prop_Hidden(inbuf)
            || (Prop_Private(inbuf) && OWNER(perms) != OWNER(what))) {
            notify_nolisten(player, "PropFetch: Permission denied.", 1);
            return NULL;
        }
    }

    if (!(ptr = get_property_class(what, inbuf))) {
        register int i;

        if (!(i = get_property_value(what, inbuf))) {
            dbref dd;

            if ((dd = get_property_dbref(what, inbuf)) == NOTHING) {
                *vl = '\0';
                ptr = vl;
                return ptr;
            } else {
                sprintf(vl, "#%d", dd);
                ptr = vl;
            }
        } else {
            sprintf(vl, "%d", i);
            ptr = vl;
        }
    }

    return ptr;
}


const char *
safegetprop(dbref player, register dbref what, dbref perms, const char *inbuf)
{
    register const char *ptr;

    while (what != NOTHING) {
        ptr = safegetprop_strict(player, what, perms, inbuf);
        if (!ptr || *ptr)
            return ptr;

        what = getparent(what);
    }

    return "";
}


const char *
stripspaces(register char *buf, register char *in)
{
    register char *ptr;

    for (ptr = in; isspace(*ptr); ptr++) ;

    strcpy(buf, ptr);
    ptr = strlen(buf) + buf - 1;

    while (*ptr == ' ' && ptr > buf)
        *(ptr--) = '\0';

    return buf;
}

const char *
string_substitute(const char *str, const char *oldstr, const char *newstr,
                  char *buf, int maxlen)
{
    register const char *ptr = str;
    register char *ptr2 = buf;
    register const char *ptr3;
    register int len = strlen(oldstr);

    if (!len) {
        strcpy(buf, str);
        return buf;
    }

    while (*ptr) {
        if (!strncmp(ptr, oldstr, len)) {
            for (ptr3 = newstr; ((ptr2 - buf) < (maxlen - 2)) && *ptr3;)
                *(ptr2++) = *(ptr3++);

            ptr += len;
        } else
            *(ptr2++) = *(ptr++);
    }

    *ptr2 = '\0';

    return buf;
}

const char *
get_list_item(dbref player, dbref what, dbref perms, const char *listname,
              int itemnum)
{
    char buf[BUFFER_LEN];
    register const char *ptr;

    sprintf(buf, "%.512s#/%d", listname, itemnum);
    ptr = safegetprop(player, what, perms, buf);
    if (!ptr || *ptr)
        return ptr;

    sprintf(buf, "%.512s/%d", listname, itemnum);
    ptr = safegetprop(player, what, perms, buf);
    if (!ptr || *ptr)
        return ptr;

    sprintf(buf, "%.512s%d", listname, itemnum);
    return (safegetprop(player, what, perms, buf));
}

int
get_list_count(dbref player, dbref obj, dbref perms, const char *listname)
{
    char buf[BUFFER_LEN];
    register const char *ptr;
    register int i;

    sprintf(buf, "%.512s#", listname);
    ptr = safegetprop(player, obj, perms, buf);
    if (ptr && *ptr)
        return (atoi(ptr));

    sprintf(buf, "%.512s/#", listname);
    ptr = safegetprop(player, obj, perms, buf);
    if (ptr && *ptr)
        return (atoi(ptr));

    for (i = 1; i < MAX_MFUN_LIST_LEN; i++) {
        if (!(ptr = get_list_item(player, obj, perms, listname, i)))
            return 0;

        if (!*ptr)
            break;
    }

    if (i-- < MAX_MFUN_LIST_LEN)
        return i;

    return MAX_MFUN_LIST_LEN;
}

const char *
get_concat_list(dbref player, dbref what, dbref perms, dbref obj,
                const char *listname, char *buf, int maxchars, int mode)
{
    register int line_limit = MAX_MFUN_LIST_LEN;
    register int i;
    const char *ptr;
    char *pos = buf;
    int cnt = get_list_count(what, obj, perms, listname);

    if (!cnt)
        return NULL;

    maxchars -= 2;
    *buf = '\0';
    for (i = 1; ((pos - buf) < (maxchars - 1)) && i <= cnt && line_limit--; i++) {
        if ((ptr = get_list_item(what, obj, perms, listname, i))) {
            while (mode && isspace(*ptr))
                ptr++;

            if (pos > buf) {
                if (!mode) {
                    *(pos++) = '\r';
                    *pos = '\0';
                } else if (mode == 1) {
                    char ch = *(pos - 1);

                    if ((pos - buf) >= (maxchars - 2))
                        break;

                    if (ch == '.' || ch == '?' || ch == '!')
                        *(pos++) = ' ';

                    *(pos++) = ' ';
                    *pos = '\0';
                } else {
                    *pos = '\0';
                }
            }

            while (((pos - buf) < (maxchars - 1)) && *ptr)
                *(pos++) = *(ptr++);

            if (mode)
                while (pos > buf && *(pos - 1) == ' ')
                    pos--;

            *pos = '\0';
            if ((pos - buf) >= (maxchars - 1))
                break;
        }
    }

    return (buf);
}


bool
mesg_read_perms(register dbref player, register dbref perms, register dbref obj)
{
    return (!obj || obj == player || obj == perms || OWNER(perms) == OWNER(obj)
            || controls(OWNER(perms), obj) || Wizperms(perms));
}

bool
isneighbor(register dbref d1, register dbref d2)
{
    if (d1 == d2)
        return 1;

    if (Typeof(d1) != TYPE_ROOM)
        if (getloc(d1) == d2)
            return 1;

    if (Typeof(d2) != TYPE_ROOM)
        if (getloc(d2) == d1)
            return 1;

    if (Typeof(d1) != TYPE_ROOM && Typeof(d2) != TYPE_ROOM)
        if (getloc(d1) == getloc(d2))
            return 1;

    return 0;
}

bool
mesg_local_perms(register dbref player, register dbref perms,
                 register dbref obj)
{
    return ((getloc(obj) != NOTHING && OWNER(perms) == OWNER(getloc(obj)))
            || isneighbor(perms, obj) || isneighbor(player, obj)
            || Mageperms(perms) || mesg_read_perms(player, perms, obj));
}

dbref
mesg_dbref_raw(int descr, dbref player, dbref what, dbref perms,
               const char *buf)
{
    struct match_data md;
    dbref obj = UNKNOWN;

    if (buf && *buf) {
        if (!string_compare(buf, "this")) {
            obj = what;
        } else if (!string_compare(buf, "me")) {
            obj = player;
        } else if (!string_compare(buf, "here")) {
            obj = getloc(player);
        } else if (!string_compare(buf, "home")) {
            obj = HOME;
        } else {
            init_match(descr, player, buf, NOTYPE, &md);
            match_absolute(&md);
            match_all_exits(&md);
            match_neighbor(&md);
            match_possession(&md);
            match_registered(&md);
            obj = match_result(&md);
            if (obj == NOTHING) {
                init_match_remote(descr, player, what, buf, NOTYPE, &md);
                match_player(&md);
                match_all_exits(&md);
                match_neighbor(&md);
                match_possession(&md);
                match_registered(&md);
                obj = match_result(&md);
            }
        }
    }

    if (obj < 0 || obj >= db_top || Typeof(obj) == TYPE_GARBAGE)
        obj = UNKNOWN;

    return obj;
}

dbref
mesg_dbref(int descr, dbref player, dbref what, dbref perms, char *buf)
{
    register dbref obj = mesg_dbref_raw(descr, player, what, perms, buf);

    if (obj == UNKNOWN)
        return obj;

    if (!mesg_read_perms(player, perms, obj))
        obj = PERMDENIED;

    return obj;
}

dbref
mesg_dbref_mage(int descr, dbref player, dbref what, dbref perms, char *buf)
{
    register dbref obj = mesg_dbref_raw(descr, player, what, perms, buf);

    if (obj == UNKNOWN || controls(OWNER(perms), obj))
        return obj;

    if (!Mageperms(perms) && OWNER(perms) != OWNER(obj))
        obj = PERMDENIED;

    return obj;
}

dbref
mesg_dbref_strict(int descr, dbref player, dbref what, dbref perms, char *buf)
{
    register dbref obj = mesg_dbref_raw(descr, player, what, perms, buf);

    if (obj == UNKNOWN)
        return obj;

    if (!Wizperms(perms) && OWNER(perms) != OWNER(obj))
        obj = PERMDENIED;

    return obj;
}

dbref
mesg_dbref_local(int descr, dbref player, dbref what, dbref perms, char *buf)
{
    register dbref obj = mesg_dbref_raw(descr, player, what, perms, buf);

    if (obj == UNKNOWN)
        return obj;

    if (!mesg_local_perms(player, perms, obj))
        obj = PERMDENIED;

    return obj;
}

const char *
ref2str(register dbref obj, register char *buf)
{
    if (obj < -3 || obj >= db_top) {
        sprintf(buf, "Bad");
        return buf;
    }

    if (obj >= 0 && Typeof(obj) == TYPE_PLAYER)
        sprintf(buf, "*%s", NAME(obj));
    else
        sprintf(buf, "#%d", obj);

    return buf;
}

bool
truestr(register const char *buf)
{
    while (isspace(*buf))
        buf++;

    return !(!*buf || (number(buf) && !atoi(buf)));
}

/******** MPI Variable handling ********/

struct mpivar {
    char name[MAX_MFUN_NAME_LEN + 1];
    char *buf;
};

static struct mpivar varv[MPI_MAX_VARIABLES];
int varc = 0;

bool
check_mvar_overflow(register int count)
{
    return (varc + count) > MPI_MAX_VARIABLES;
}

bool
new_mvar(register const char *varname, register char *buf)
{
    if (strlen(varname) > MAX_MFUN_NAME_LEN)
        return 1;

    if (varc >= MPI_MAX_VARIABLES)
        return 2;

    strcpy(varv[varc].name, varname);
    varv[varc++].buf = buf;

    return 0;
}

char *
get_mvar(register const char *varname)
{
    register int i = 0;

    for (i = varc - 1; i >= 0 && string_compare(varname, varv[i].name); i--) ;

    if (i < 0)
        return NULL;

    return varv[i].buf;
}

bool
free_top_mvar(void)
{
    if (varc < 1)
        return 1;

    varc--;
    return 0;
}

/***** MPI function handling *****/

struct mpifunc {
    char name[MAX_MFUN_NAME_LEN + 1];
    char *buf;
};

static struct mpifunc funcv[MPI_MAX_FUNCTIONS];
int funcc = 0;

bool
new_mfunc(register const char *funcname, register const char *buf)
{
    if (strlen(funcname) > MAX_MFUN_NAME_LEN)
        return 1;

    if (funcc > MPI_MAX_FUNCTIONS)
        return 2;

    strcpy(funcv[funcc].name, funcname);
    funcv[funcc++].buf = (char *) string_dup(buf);
    return 0;
}

const char *
get_mfunc(register const char *funcname)
{
    register int i = 0;

    for (i = funcc - 1; i >= 0 && string_compare(funcname, funcv[i].name);
         i--) ;

    if (i < 0)
        return NULL;

    return funcv[i].buf;
}

bool
free_mfuncs(register int downto)
{
    if (funcc < 1 || downto < 0)
        return 1;

    while (funcc > downto)
        free(funcv[--funcc].buf);

    return 0;
}

/*** End of MFUNs. ***/

bool
msg_is_macro(dbref player, dbref what, dbref perms, const char *name)
{
    register const char *ptr;
    register dbref obj;
    char buf2[BUFFER_LEN];

    if (!*name)
        return 0;

    if (name[0] == '@')
        return 0;

    sprintf(buf2, "_msgmacs/%s", name);
    obj = what;
    ptr = get_mfunc(name);

    if (!ptr || !*ptr)
        ptr = safegetprop_strict(player, OWNER(obj), perms, buf2);

    if (!ptr || !*ptr)
        ptr = safegetprop_limited(player, obj, OWNER(obj), perms, buf2);

    if (!ptr || !*ptr)
        ptr = safegetprop_strict(player, 0, perms, buf2);

    return (ptr && *ptr);
}

void
msg_unparse_macro(dbref player, dbref what, dbref perms, char *name, int argc,
                  argv_typ argv, char *rest, int maxchars)
{
    register const char *ptr;
    register char *ptr2;
    register int i, p = 0;
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    dbref obj;

    strcpy(buf, rest);
    sprintf(buf2, "_msgmacs/%s", name);
    obj = what;
    ptr = get_mfunc(name);

    if (!ptr || !*ptr)
        ptr = safegetprop_strict(player, OWNER(obj), perms, buf2);

    if (!ptr || !*ptr)
        ptr = safegetprop_limited(player, obj, OWNER(obj), perms, buf2);

    if (!ptr || !*ptr)
        ptr = safegetprop_strict(player, 0, perms, buf2);

    while (ptr && *ptr && p < (maxchars - 1)) {
        if (*ptr == '\\') {
            if (*(ptr + 1) == 'r') {
                rest[p++] = '\r';
                ptr++;
                ptr++;
            } else {
                rest[p++] = *(ptr++);
                rest[p++] = *(ptr++);
            }
        } else if (*ptr == MFUN_LEADCHAR) {
            if (*(ptr + 1) == MFUN_ARGSTART && isdigit(*(ptr + 2)) &&
                *(ptr + 3) == MFUN_ARGEND) {
                ptr++;
                ptr++;
                i = *(ptr++) - '1';
                ptr++;
                if (i >= argc || i < 0) {
                    ptr2 = NULL;
                } else {
                    ptr2 = argv[i];
                }
                while (ptr2 && *ptr2 && p < (maxchars - 1)) {
                    rest[p++] = *(ptr2++);
                }
            } else {
                rest[p++] = *(ptr++);
            }
        } else {
            rest[p++] = *(ptr++);
        }
    }
    ptr2 = buf;
    while (ptr2 && *ptr2 && p < (maxchars - 1)) {
        rest[p++] = *(ptr2++);
    }

    rest[p] = '\0';
}

#ifndef MSGHASHSIZE
#define MSGHASHSIZE 256
#endif

static hash_tab msghash[MSGHASHSIZE];

int
find_mfn(register const char *name)
{
    hash_data *exp = find_hash(name, msghash, MSGHASHSIZE);

    if (exp)
        return (exp->ival);

    return (0);
}

void
insert_mfn(register const char *name, register int i)
{
    hash_data hd;

    free_hash(name, msghash, MSGHASHSIZE);
    hd.ival = i;
    add_hash(name, hd, msghash, MSGHASHSIZE);
}

void
purge_mfns(void)
{
    kill_hash(msghash, MSGHASHSIZE, 0);
}

#include "mfunlist.h"

/******** HOOK ********/
void
mesg_init(void)
{
    register int i;

    for (i = 0; mfun_list[i].name; i++)
        insert_mfn(mfun_list[i].name, i + 1);

    mpi_prof_start_time = time(NULL);
}

/******** HOOK ********/
int
mesg_args(char *wbuf, argv_typ argv, char ulv, char sep, char dlv, char quot,
          int maxargs)
{
    register int r, lev, argc = 0;
    register bool litflag = 0;
    register char *ptr;
    char buf[BUFFER_LEN];

    /* for (ptr = wbuf; ptr && isspace(*ptr); ptr++); */
    strcpy(buf, wbuf);
    ptr = buf;
    for (lev = r = 0; (r < (BUFFER_LEN - 2)); r++) {
        if (buf[r] == '\0') {
            return (-1);
        } else if (buf[r] == '\\') {
            r++;
            continue;
        } else if (buf[r] == quot) {
            litflag = (!litflag);
        } else if (!litflag && buf[r] == ulv) {
            lev++;
            continue;
        } else if (!litflag && buf[r] == dlv) {
            lev--;
            if (lev < 0) {
                buf[r] = '\0';
                strcpy(argv[argc++], ptr);
                ptr = buf + r + 1;
                break;
            }
            continue;
        } else if (!litflag && lev < 1 && buf[r] == sep) {
            if (argc < maxargs - 1) {
                buf[r] = '\0';
                strcpy(argv[argc++], ptr);
                ptr = buf + r + 1;
            }
        }
    }
    buf[BUFFER_LEN - 1] = '\0';
    strcpy(wbuf, ptr);
    return argc;
}

const char *
cr2slash(register char *buf, register const char *in)
{
    register char *ptr = buf;
    register const char *ptr2 = in;

    do {
        if (*ptr2 == '\r') {
            ptr2++;
            *(ptr++) = '\\';
            *(ptr++) = 'r';
        } else if (*ptr2 == '\\') {
            ptr2++;
            *(ptr++) = '\\';
            *(ptr++) = '\\';
        } else {
            *(ptr++) = *(ptr2++);
        }
    } while (*(ptr - 1) && (ptr - buf < BUFFER_LEN - 3));
    buf[BUFFER_LEN - 1] = '\0';

    return buf;
}

static int mesg_rec_cnt = 0;
static int mesg_instr_cnt = 0;

/******** HOOK ********/
char *
mesg_parse(int descr, dbref player, dbref what, dbref perms, const char *inbuf,
           char *outbuf, int maxchars, int mesgtyp)
{
    char wbuf[BUFFER_LEN];
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    char dbuf[BUFFER_LEN];
    char ebuf[BUFFER_LEN];
    char cmdbuf[MAX_MFUN_NAME_LEN + 1];
    char argv[9][BUFFER_LEN];
    register const char *ptr;
    register const char *dptr;
    register int p, q, s, i;
    bool showtextflag = 0;
    bool literalflag = 0;
    int argc;

    mesg_rec_cnt++;
    if (mesg_rec_cnt > 26) {
        char *zptr = get_mvar("how");

        snprintf(dbuf, sizeof(dbuf), "%s Recursion limit exceeded.", zptr);
        smnotify(descr, player, dbuf);

        mesg_rec_cnt--;
        outbuf[0] = '\0';

        return NULL;
    }

    if (OkObj(player) && Typeof(player) == TYPE_GARBAGE) {
        mesg_rec_cnt--;
        outbuf[0] = '\0';
        return NULL;
    }

    if (!OkObj(what) || Typeof(what) == TYPE_GARBAGE) {
        smnotify(descr, player, "MPI Error: Garbage trigger.");
        mesg_rec_cnt--;
        outbuf[0] = '\0';

        return NULL;
    }

    strcpy(wbuf, inbuf);
    memset(outbuf, sizeof(outbuf), 0);
    for (p = q = 0; wbuf[p] && (p < maxchars - 1) && q < (maxchars - 1); p++) {
        if (wbuf[p] == '\\') {
            p++;
            showtextflag = 1;
            if (wbuf[p] == 'r') {
                outbuf[q++] = '\r';
            } else if (wbuf[p] == '[') {
                outbuf[q++] = ESCAPE_CHAR;
            } else {
                outbuf[q++] = wbuf[p];
            }
        } else if (wbuf[p] == MFUN_LITCHAR) {
            literalflag = (!literalflag);
        } else if (!literalflag && wbuf[p] == MFUN_LEADCHAR) {
            if (wbuf[p + 1] == MFUN_LEADCHAR) {
                showtextflag = 1;
                outbuf[q++] = wbuf[p++];
                ptr = "";
            } else {
                ptr = wbuf + (++p);
                s = 0;
                while (wbuf[p] && wbuf[p] != MFUN_LEADCHAR &&
                       !isspace(wbuf[p]) && wbuf[p] != MFUN_ARGSTART &&
                       wbuf[p] != MFUN_ARGEND && s < MAX_MFUN_NAME_LEN) {
                    p++;
                    s++;
                }
                if (s < MAX_MFUN_NAME_LEN &&
                    (wbuf[p] == MFUN_ARGSTART || wbuf[p] == MFUN_ARGEND)) {
                    int varflag;

                    strncpy(cmdbuf, ptr, s);
                    cmdbuf[s] = '\0';

                    varflag = 0;
                    if (*cmdbuf == '&') {
                        s = find_mfn("sublist");
                        varflag = 1;
                    } else if (*cmdbuf) {
                        s = find_mfn(cmdbuf);
                    } else {
                        s = 0;
                    }
                    if (s) {
                        s--;
                        if (++mesg_instr_cnt > tp_mpi_max_commands) {
                            char *zptr = get_mvar("how");

                            sprintf(dbuf,
                                    "%s %c%s%c: Instruction limit exceeded.",
                                    zptr, MFUN_LEADCHAR,
                                    (varflag ? cmdbuf : mfun_list[s].name),
                                    MFUN_ARGEND);
                            smnotify(descr, player, dbuf);
                            return NULL;
                        }
                        if (wbuf[p] == MFUN_ARGEND) {
                            argc = 0;
                        } else {
                            argc = mfun_list[s].maxargs;
                            if (argc < 0) {
                                argc = mesg_args((wbuf + p + 1),
                                                 &argv[(varflag ? 1 : 0)],
                                                 MFUN_LEADCHAR, MFUN_ARGSEP,
                                                 MFUN_ARGEND, MFUN_LITCHAR,
                                                 (-argc) + (varflag ? 1 : 0));
                            } else {
                                argc = mesg_args((wbuf + p + 1),
                                                 &argv[(varflag ? 1 : 0)],
                                                 MFUN_LEADCHAR, MFUN_ARGSEP,
                                                 MFUN_ARGEND, MFUN_LITCHAR,
                                                 (varflag ? 8 : 9));
                            }
                            if (argc == -1) {
                                char *zptr = get_mvar("how");

                                sprintf(ebuf, "%s %c%s%c: End brace not found.",
                                        zptr, MFUN_LEADCHAR, cmdbuf,
                                        MFUN_ARGEND);
                                smnotify(descr, player, ebuf);
                                return NULL;
                            }
                        }
                        if (varflag) {
                            char *zptr;

                            argc++;
                            zptr = get_mvar(cmdbuf + 1);
                            if (!zptr) {
                                zptr = get_mvar("how");
                                sprintf(ebuf,
                                        "%s %c%s%c: Unrecognized variable.",
                                        zptr, MFUN_LEADCHAR, cmdbuf,
                                        MFUN_ARGEND);
                                smnotify(descr, player, ebuf);

                                return NULL;
                            }
                            strcpy(argv[0], zptr);
                        }
                        if (mesgtyp & MPI_ISDEBUG) {
                            char *zptr = get_mvar("how");

                            sprintf(dbuf, "%s %*s%c%s%c", zptr,
                                    (mesg_rec_cnt * 2 - 4), "", MFUN_LEADCHAR,
                                    (varflag ? cmdbuf : mfun_list[s].name),
                                    MFUN_ARGSTART);
                            for (i = (varflag ? 1 : 0); i < argc; i++) {
                                if (i)
                                    sprintf(dbuf, "%.512s%c ", dbuf,
                                            MFUN_ARGSEP);

                                cr2slash(ebuf, argv[i]);
                                if (strlen(ebuf) > 512)
                                    sprintf(dbuf, "%.512s\"%.512s...\"", dbuf,
                                            ebuf);
                                else
                                    sprintf(dbuf, "%.512s\"%s\"", dbuf, ebuf);
                            }
                            sprintf(dbuf, "%.512s%c", dbuf, MFUN_ARGEND);
                            smnotify(descr, player, dbuf);
                        }
                        if (mfun_list[s].stripp)
                            for (i = (varflag ? 1 : 0); i < argc; i++)
                                strcpy(argv[i], stripspaces(buf, argv[i]));

                        if (mfun_list[s].parsep) {
                            for (i = (varflag ? 1 : 0); i < argc; i++) {
                                ptr = MesgParse(argv[i], argv[i]);
                                if (!ptr) {
                                    char *zptr = get_mvar("how");

                                    sprintf(dbuf, "%s %c%s%c (arg %d)", zptr,
                                            MFUN_LEADCHAR,
                                            (varflag ? cmdbuf : mfun_list[s].
                                             name), MFUN_ARGEND, i + 1);
                                    smnotify(descr, player, dbuf);
                                    return NULL;
                                }
                            }
                        }
                        if (mesgtyp & MPI_ISDEBUG) {
                            char *zptr = get_mvar("how");

                            sprintf(dbuf, "%.512s %*s%c%.512s%c", zptr,
                                    (mesg_rec_cnt * 2 - 4), "", MFUN_LEADCHAR,
                                    (varflag ? cmdbuf : mfun_list[s].name),
                                    MFUN_ARGSTART);

                            for (i = (varflag ? 1 : 0); i < argc; i++) {
                                if (i)
                                    sprintf(dbuf, "%.512s%c ", dbuf,
                                            MFUN_ARGSEP);

                                cr2slash(ebuf, argv[i]);
                                if (strlen(ebuf) > 128)
                                    sprintf(dbuf, "%.512s\"%.128s...\"", dbuf,
                                            ebuf);
                                else
                                    sprintf(dbuf, "%.512s\"%s\"", dbuf, ebuf);
                            }
                            sprintf(dbuf, "%s%c", dbuf, MFUN_ARGEND);
                        }
                        if (argc < mfun_list[s].minargs) {
                            char *zptr = get_mvar("how");

                            sprintf(ebuf, "%s %c%s%c: Too few arguments",
                                    zptr, MFUN_LEADCHAR,
                                    (varflag ? cmdbuf : mfun_list[s].name),
                                    MFUN_ARGEND);
                            smnotify(descr, player, ebuf);

                            return NULL;
                        } else if (mfun_list[s].maxargs > 0 &&
                                   argc > mfun_list[s].maxargs) {
                            char *zptr = get_mvar("how");

                            sprintf(ebuf, "%s %c%s%c: Too many arguments",
                                    zptr, MFUN_LEADCHAR,
                                    (varflag ? cmdbuf : mfun_list[s].name),
                                    MFUN_ARGEND);
                            smnotify(descr, player, ebuf);

                            return NULL;
                        } else {
                            ptr =
                                mfun_list[s].mfn(descr, player, what, perms,
                                                 argc, argv, buf, mesgtyp);
                            if (!ptr) {
                                outbuf[q] = '\0';
                                return NULL;
                            }

                            if (mfun_list[s].postp) {
                                dptr = MesgParse(ptr, buf);
                                if (!dptr) {
                                    char *zptr = get_mvar("how");

                                    sprintf(ebuf, "%s %c%s%c (returned string)",
                                            zptr, MFUN_LEADCHAR,
                                            (varflag ? cmdbuf : mfun_list[s].
                                             name), MFUN_ARGEND);
                                    smnotify(descr, player, ebuf);

                                    return NULL;
                                }
                                ptr = dptr;
                            }
                        }
                        if (mesgtyp & MPI_ISDEBUG) {
                            sprintf(dbuf, "%.512s = \"%.512s\"", dbuf,
                                    cr2slash(ebuf, ptr));
                            smnotify(descr, player, dbuf);
                        }
                    } else if (msg_is_macro(player, what, perms, cmdbuf)) {
                        if (wbuf[p] == MFUN_ARGEND) {
                            argc = 0;
                            p++;
                        } else {
                            p++;
                            argc = mesg_args(wbuf + p, argv, MFUN_LEADCHAR,
                                             MFUN_ARGSEP, MFUN_ARGEND,
                                             MFUN_LITCHAR, 9);
                            if (argc == -1) {
                                char *zptr = get_mvar("how");

                                sprintf(ebuf, "%s %c%s%c: End brace not found.",
                                        zptr, MFUN_LEADCHAR, cmdbuf,
                                        MFUN_ARGEND);
                                smnotify(descr, player, ebuf);

                                return NULL;
                            }
                        }
                        msg_unparse_macro(player, what, perms, cmdbuf, argc,
                                          argv, (wbuf + p), (BUFFER_LEN - p));
                        p--;
                        ptr = NULL;
                    } else {
                        /* unknown function */
                        char *zptr = get_mvar("how");

                        sprintf(ebuf, "%s %c%s%c: Unrecognized function.",
                                zptr, MFUN_LEADCHAR, cmdbuf, MFUN_ARGEND);
                        smnotify(descr, player, ebuf);

                        return NULL;
                    }
                } else {
                    showtextflag = 1;
                    ptr--;
                    i = s + 1;
                    while (ptr && *ptr && i-- && q < (maxchars - 1))
                        outbuf[q++] = *(ptr++);

                    outbuf[q] = '\0';
                    p = (int) (ptr - wbuf) - 1;
                    ptr = "";   /* unknown substitution type */

                    /*  showtextflag = 1;
                       p = (int) (ptr - wbuf);
                       if (q < (maxchars - 1))
                       outbuf[q++] = MFUN_LEADCHAR;
                       ptr = "";  unknown substitution type */
                }
                while (ptr && *ptr && q < (maxchars - 1))
                    outbuf[q++] = *(ptr++);
            }
        } else {
            outbuf[q++] = wbuf[p];
            showtextflag = 1;
        }
    }
    outbuf[q] = '\0';
    if ((mesgtyp & MPI_ISDEBUG) && showtextflag) {
        char *zptr = get_mvar("how");

        sprintf(dbuf, "%s %*s\"%s\"", zptr, (mesg_rec_cnt * 2 - 4), "",
                cr2slash(buf2, outbuf));
        smnotify(descr, player, dbuf);
    }
    mesg_rec_cnt--;
    return (outbuf);
}

char *
do_parse_mesg_2(int descr, dbref player, dbref what, dbref perms,
                const char *inbuf, const char *abuf, char *outbuf, int mesgtyp)
{

    char howvar[BUFFER_LEN];
    char cmdvar[BUFFER_LEN];
    char argvar[BUFFER_LEN];
    char tmparg[BUFFER_LEN];
    char tmpcmd[BUFFER_LEN];
    register const char *dptr;
    register int mvarcnt = varc;
    register int mfunccnt = funcc;
    register int tmprec_cnt = mesg_rec_cnt;
    register int tmpinst_cnt = mesg_instr_cnt;

    if (tp_do_mpi_parsing) {
        /* *outbuf = '\0'; */ memset(outbuf, sizeof(outbuf), 0);
        if ((mesgtyp & MPI_NOHOW) == 0) {
            if (new_mvar("how", howvar))
                return outbuf;
            strcpy(howvar, abuf);
        }

        if (new_mvar("cmd", cmdvar))
            return outbuf;

        strcpy(cmdvar, match_cmdname);
        strcpy(tmpcmd, match_cmdname);

        if (new_mvar("arg", argvar))
            return outbuf;

        strcpy(argvar, match_args);
        strcpy(tmparg, match_args);

        dptr = MesgParse(inbuf, outbuf);
        if (!dptr) {
            *outbuf = '\0';
        }

        varc = mvarcnt;
        free_mfuncs(mfunccnt);
        mesg_rec_cnt = tmprec_cnt;
        mesg_instr_cnt = tmpinst_cnt;

        strcpy(match_cmdname, tmpcmd);
        strcpy(match_args, tmparg);
    } else
        strcpy(outbuf, inbuf);

    return outbuf;
}


char *
do_parse_mesg(int descr, dbref player, dbref what, const char *inbuf,
              const char *abuf, char *outbuf, int mesgtyp)
{
    if (tp_do_mpi_parsing && (tp_mpi_needflag ? Meeper(what) : 1)) {
        register char *tmp = NULL;
        struct timeval st, et;

        gettimeofday(&st, NULL);
        tmp =
            do_parse_mesg_2(descr, player, what, what, inbuf, abuf, outbuf,
                            mesgtyp);
        gettimeofday(&et, NULL);

        if (strcmp(tmp, inbuf)) {
            if (st.tv_usec > et.tv_usec) {
                et.tv_usec += 1000000;
                et.tv_sec -= 1;
            }
            et.tv_usec -= st.tv_usec;
            et.tv_sec -= st.tv_sec;
            DBFETCH(what)->mpi_proftime.tv_sec += et.tv_sec;
            DBFETCH(what)->mpi_proftime.tv_usec += et.tv_usec;
            if (DBFETCH(what)->mpi_proftime.tv_usec >= 1000000) {
                DBFETCH(what)->mpi_proftime.tv_usec -= 1000000;
                DBFETCH(what)->mpi_proftime.tv_sec += 1;
            }
            DBFETCH(what)->mpi_prof_use++;
        }

        return (tmp);
    } else
        strcpy(outbuf, inbuf);

    return outbuf;
}
