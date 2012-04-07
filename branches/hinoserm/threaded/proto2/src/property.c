#include "copyright.h"
#include "config.h"
#include "params.h"

#include "db.h"
#include "tune.h"
#include "mpi.h"
#include "props.h"
#include "externs.h"
#include "interface.h"
#include "nan.h"

#ifdef COMPRESS
extern const char *pcompress(const char *);

#ifdef ARCHAIC_DATABASES
extern const char *old_uncompress(const char *);
#endif

#define alloc_compressed(x) alloc_string(pcompress(x))
#else /* !COMPRESS */
#define alloc_compressed(x) alloc_string(x)
#endif /* COMPRESS */

/* property.c
   A whole new lachesis mod.
   Adds property manipulation routines to TinyMUCK.   */

/* Completely rewritten by darkfox and Foxen, for propdirs and other things */

#define fltostr(x,y) (sprintf(x, "%.15g", y) ? x : x)

void
set_property_nofetch(dbref object, const char *pname, PData * dat,
                     bool pure)
{
    char buf[BUFFER_LEN];
    char *n;
    PropPtr p;

    /* Make sure that we are passed a valid property name */
    if (!pname) {
        return;
	}

    while (*pname == PROPDIR_DELIMITER)
        pname++;

    strcpy(buf, pname);

    /* truncate propnames with a ':' in them at the ':' */
    if ((n = index(buf, PROP_DELIMITER)))
        *n = '\0';

    /* truncate propnames with a '\n' in them at the '\n' */
    if ((n = index(buf, '\n')))
        *n = '\0';

    if (!*buf)
        return;

    if ((!(FLAGS(object) & LISTENER)) &&
        (string_prefix(buf, "_listen") ||
         string_prefix(buf, "~listen") ||
         string_prefix(buf, "@listen") ||
         string_prefix(buf, "_olisten") ||
         string_prefix(buf, "~olisten") ||
         string_prefix(buf, "@olisten") ||
         string_prefix(buf, "_alisten") ||
         string_prefix(buf, "~alisten") ||
         string_prefix(buf, "@alisten") ||
         string_prefix(buf, "_aolisten") ||
         string_prefix(buf, "~aolisten") || string_prefix(buf, "@aolisten"))) {
        FLAGS(object) |= LISTENER;
    }

    if ((!(FLAG2(object) & F2COMMAND)) &&
        (string_prefix(buf, "_command") ||
         string_prefix(buf, "~command") ||
         string_prefix(buf, "@command") ||
         string_prefix(buf, "_ocommand") ||
         string_prefix(buf, "~ocommand") || string_prefix(buf, "@ocommand"))) {
        FLAG2(object) |= F2COMMAND;
    }

	DBLOCK(object);

    p = propdir_new_elem(&(DBFETCH(object)->properties), buf);

    /* free up any old values */
    clear_propnode(p);

    SetPFlagsRaw(p, dat->flags);
    if (PropFlags(p) & PROP_ISUNLOADED) {
        SetPDataUnion(p, dat->data);
		DBUNLOCK(object);
        return;
    }

    switch (PropType(p)) {
        case PROP_STRTYP:
            if (!dat->data.str || !*dat->data.str) {
                SetPType(p, PROP_DIRTYP);
                SetPDataStr(p, NULL);
                if (!PropDir(p)) {
                    remove_property_nofetch(object, pname);
                }
            } else {
#ifdef COMPRESS
                if (!pure) {
                    if (!(dat->flags & PROP_NOCOMPRESS)
                        && isascii_str(dat->data.str)) {
                        SetPDataStr(p, alloc_compressed(dat->data.str));
                    } else {
                        dat->flags |= PROP_NOCOMPRESS;
                        SetPDataStr(p, alloc_string(dat->data.str));
                    }
                    SetPFlagsRaw(p, (dat->flags | PROP_COMPRESSED));
                } else
#endif
                    SetPDataStr(p, alloc_string(dat->data.str));
            }
            break;
        case PROP_INTTYP:
            SetPDataVal(p, dat->data.val);
            if (!dat->data.val) {
                SetPType(p, PROP_DIRTYP);
                if (!PropDir(p))
                    remove_property_nofetch(object, pname);
            }
            break;
        case PROP_FLTTYP:
            SetPDataFVal(p, dat->data.fval);
            if (dat->data.fval == 0.0) {
                SetPType(p, PROP_DIRTYP);
                if (!PropDir(p))
                    remove_property_nofetch(object, pname);
            }
            break;
        case PROP_REFTYP:
            SetPDataRef(p, dat->data.ref);
            if (dat->data.ref == NOTHING) {
                SetPType(p, PROP_DIRTYP);
                SetPDataRef(p, 0);
                if (!PropDir(p))
                    remove_property_nofetch(object, pname);
            }
            break;
        case PROP_LOKTYP:
            SetPDataLok(p, dat->data.lok);
            break;
        case PROP_DIRTYP:
            SetPDataVal(p, 0);
            if (!PropDir(p))
                remove_property_nofetch(object, pname);
            break;
    }

	DBUNLOCK(object);
}


void
set_property(dbref object, const char *pname, PData * dat)
{
	DBLOCK(object);
#ifdef DISKBASE
    fetchprops(object);
    set_property_nofetch(object, pname, dat, 0);
    dirtyprops(object);
#else
    set_property_nofetch(object, pname, dat, 0);
#endif
    DBDIRTY(object);
	DBUNLOCK(object);
}

/* adds a new property to an object */
void
add_prop_nofetch(dbref player, const char *pname, const char *type, int value)
{
    PData pdat;

    if (type && *type) {
        pdat.flags = PROP_STRTYP;
        pdat.data.str = (char *) type;
    } else if (value) {
        pdat.flags = PROP_INTTYP;
        pdat.data.val = value;
    } else {
        pdat.flags = PROP_DIRTYP;
        pdat.data.str = NULL;
    }
    set_property_nofetch(player, pname, &pdat, 0);
}

/* adds a new property to an object */
void
add_property(dbref player, const char *type, const char *pclass, int value)
{
	DBLOCK(player);
#ifdef DISKBASE
    fetchprops(player);
    add_prop_nofetch(player, type, pclass, value);
    dirtyprops(player);
#else
    add_prop_nofetch(player, type, pclass, value);
#endif
    DBDIRTY(player);
	DBUNLOCK(player);
}

void
remove_proplist_item(dbref player, PropPtr p, int allp)
{
    const char *ptr;

    /* if( tp_db_readonly ) return; *//* Why did we remove this? */

    if (!p)
        return;

    ptr = PropName(p);
    if (!allp) {
        if (Prop_SeeOnly(ptr))
            return;
        if (Prop_Hidden(ptr))
            return;

        if (ptr[0] == '_' && ptr[1] == '\0')
            return;
        if (PropFlags(p) & PROP_SYSPERMS)
            return;
    }
    /* notify(player, ptr); *//* Why did we put this here? */
    remove_property(player, ptr);
}


/* removes property list --- if it's not there then ignore */
void
remove_property_list(dbref player, int all)
{
    PropPtr l;
    PropPtr p;
    PropPtr n;

    /* if( tp_db_readonly ) return; *//* Why did we remove this? */

	DBLOCK(player);

#ifdef DISKBASE
    fetchprops(player);
#endif

    if ((l = DBFETCH(player)->properties)) {
        p = first_node(l);
        while (p) {
            n = next_node(l, PropName(p));
            remove_proplist_item(player, p, all);
            l = DBFETCH(player)->properties;
            p = n;
        }
    }
#ifdef DISKBASE
    dirtyprops(player);
#endif

    DBDIRTY(player);
	DBUNLOCK(player);
}



/* removes property --- if it's not there then ignore */
void
remove_property_nofetch(dbref player, const char *type)
{
    PropPtr l;
    char buf[BUFFER_LEN];
    char *w;

    /* if( tp_db_readonly ) return; *//* Why did we remove this? */

	DBLOCK(player);

    w = strcpy(buf, type);

    l = DBFETCH(player)->properties;
    l = propdir_delete_elem(l, w);
    DBFETCH(player)->properties = l;
    if ((FLAGS(player) & LISTENER) && !(get_property(player, "_listen") ||
                                        get_property(player, "_olisten") ||
                                        get_property(player, "~listen") ||
                                        get_property(player, "~olisten") ||
                                        get_property(player, "@olisten") ||
                                        get_property(player, "@listen") ||
                                        get_property(player, "_alisten") ||
                                        get_property(player, "_aolisten") ||
                                        get_property(player, "~alisten") ||
                                        get_property(player, "~aolisten") ||
                                        get_property(player, "@aolisten") ||
                                        get_property(player, "@alisten")
        )) {
        FLAGS(player) &= ~LISTENER;
    }
    if ((FLAG2(player) & F2COMMAND) && !(get_property(player, "_command") ||
                                         get_property(player, "_ocommand") ||
                                         get_property(player, "~command") ||
                                         get_property(player, "~ocommand") ||
                                         get_property(player, "@ocommand") ||
                                         get_property(player, "@command")
        )) {
        FLAG2(player) &= ~F2COMMAND;
    }
    DBDIRTY(player);
	DBUNLOCK(player);
}


void
remove_property(dbref player, const char *type)
{

    /* if( tp_db_readonly ) return; *//* Why did we remove this? *//* That is a good question. -Hinoserm 04-04-2012 */

	DBLOCK(player);

#ifdef DISKBASE
    fetchprops(player);
#endif
    remove_property_nofetch(player, type);
#ifdef DISKBASE
    dirtyprops(player);
#endif

	DBUNLOCK(player);
}


PropPtr
get_property(dbref player, const char *type)
{
    PropPtr p;
    char buf[BUFFER_LEN];
    char *w;

	DBLOCK(player);

#ifdef DISKBASE
    fetchprops(player);
#endif

    w = strcpy(buf, type);

    p = propdir_get_elem(DBFETCH(player)->properties, w);

	DBUNLOCK(player);

    return (p);
}


/* checks if object has property, returning 1 if it or any of it's contents has
   the property stated                                                      */
int
has_property(int descr, dbref player, dbref what, const char *type,
             const char *pclass, int value)
{
    dbref things;

	DBLOCK(-1);

    if (has_property_strict(descr, player, what, type, pclass, value)) {
		DBUNLOCK(-1);
        return 1;
	}

    for (things = DBFETCH(what)->contents; things != NOTHING; things = DBFETCH(things)->next) {
        if (has_property(descr, player, things, type, pclass, value)) {
			DBUNLOCK(-1);
            return 1;
		}
    }

    if (tp_lock_envcheck) {
        things = getparent(what);
        while (things != NOTHING) {
            if (has_property_strict(descr, player, things, type, pclass, value)) {
				DBUNLOCK(-1);
                return 1;
			}
            things = getparent(things);
        }
    }

	DBUNLOCK(-1);
    return 0;
}


/* checks if object has property, returns 1 if it has the property */
int
has_property_strict(int descr, dbref player, dbref what, const char *type,
                    const char *pclass, int value)
{
    PropPtr p;
    const char *str;
    char *ptr;
    char buf[BUFFER_LEN];

    p = get_property(what, type);

    if (p) {
#ifdef DISKBASE
        propfetch(what, p);
#endif
        if (PropType(p) == PROP_STRTYP) {
            str = DoNull(PropDataUNCStr(p));
            ptr = do_parse_mesg(descr, player, what, str, "(Lock)",
                                buf, (MPI_ISPRIVATE | MPI_ISLOCK), "", "");
            return (equalstr((char *) pclass, ptr));
        } else if (PropType(p) == PROP_INTTYP) {
            return (value == PropDataVal(p));
        } else {
            return 0;
        }
    }
    return 0;
}

/* return class (string value) of property */
const char *
get_property_class(dbref player, const char *type)
{
    PropPtr p;

	DBLOCK(player);

    p = get_property(player, type);
    if (p) {
#ifdef DISKBASE
        propfetch(player, p);
#endif
        if (PropType(p) != PROP_STRTYP) {
			DBUNLOCK(player);
            return (char *) NULL;
		}

		DBUNLOCK(player);
        return (PropDataUNCStr(p));
    } else {
		DBUNLOCK(player);
        return (char *) NULL;
    }
}

/* return value of property */
int
get_property_value(dbref player, const char *type)
{
    PropPtr p;

	DBLOCK(player);

    p = get_property(player, type);

    if (p) {
#ifdef DISKBASE
        propfetch(player, p);
#endif
        if (PropType(p) != PROP_INTTYP) {
			DBUNLOCK(player);
            return 0;
		}

		DBUNLOCK(player);
        return (PropDataVal(p));
    } else {
		DBUNLOCK(player);
        return 0;
    }
}

/* return float value of a property */
double
get_property_fvalue(dbref player, const char *pname)
{
    PropPtr p;

	DBLOCK(player);

    p = get_property(player, pname);
    if (p) {
#ifdef DISKBASE
        propfetch(player, p);
#endif
        if (PropType(p) != PROP_FLTTYP) {
			DBUNLOCK(player);
            return 0.0;
		}

		DBUNLOCK(player);
        return (PropDataFVal(p));
    } else {
		DBUNLOCK(player);
        return 0.0;
    }
}

/* return boolexp lock of property */
dbref
get_property_dbref(dbref player, const char *pclass)
{
    PropPtr p;

	DBLOCK(player);

    p = get_property(player, pclass);
    if (!p) {
		DBUNLOCK(player);
        return NOTHING;
	}
#ifdef DISKBASE
    propfetch(player, p);
#endif
    if (PropType(p) != PROP_REFTYP) {
		DBUNLOCK(player);
        return NOTHING;
	}

	DBUNLOCK(player);
    return PropDataRef(p);
}


/* return boolexp lock of property */
struct boolexp *
get_property_lock(dbref player, const char *pclass)
{
    PropPtr p;

	DBLOCK(player);

    p = get_property(player, pclass);
    if (!p) {
		DBUNLOCK(player);
        return TRUE_BOOLEXP;
	}
#ifdef DISKBASE
    propfetch(player, p);
    if (PropFlags(p) & PROP_ISUNLOADED) {
		DBUNLOCK(player);
        return TRUE_BOOLEXP;
	}
#endif
    if (PropType(p) != PROP_LOKTYP) {
		DBUNLOCK(player);
        return TRUE_BOOLEXP;
	}

	DBUNLOCK(player);
    return PropDataLok(p);
}


/* return flags of property */
int
get_property_flags(dbref player, const char *type)
{
    PropPtr p;

	DBLOCK(player);

    p = get_property(player, type);

    if (p) {
		DBUNLOCK(player);
        return (PropFlags(p));
    }
	
	DBUNLOCK(player);
	return 0;
}


/* return type of property */
int
get_property_type(dbref player, const char *type)
{
    PropPtr p;

	DBLOCK(player);

    p = get_property(player, type);

    if (p) {
		DBUNLOCK(player);
        return (PropType(p));
    } else {
		DBUNLOCK(player);
        return 0;
    }
}

PropPtr
copy_prop(dbref old)
{
    PropPtr p, n = NULL;

	DBLOCK(old);
#ifdef DISKBASE
    fetchprops(old);
#endif

    p = DBFETCH(old)->properties;
    copy_proplist(old, &n, p);

	DBUNLOCK(old);
    return (n);
}

/* return old gender values for pronoun substitution code */
int
genderof(int descr, dbref player)
{
    const char *sexstr = NULL;
    sexstr = get_property_class(player, tp_sex_prop);
    while (sexstr && isspace(*sexstr)) sexstr++;

    if (!sexstr || !*sexstr)
        return GENDER_UNASSIGNED;
    if (string_compare(sexstr, "male") == 0)
        return GENDER_MALE;
    if (string_compare(sexstr, "female") == 0)
        return GENDER_FEMALE;
    if (string_compare(sexstr, "neuter") == 0)
        return GENDER_NEUTER;
    return GENDER_UNASSIGNED;
}

/* return a pointer to the first property in a propdir and duplicates the
   property name into 'name'.  returns 0 if the property list is empty
   or does not exist. */
PropPtr
first_prop_nofetch(dbref player, const char *dir, PropPtr *list, char *name)
{
    char buf[BUFFER_LEN];
    PropPtr p;

	DBLOCK(player);

    if (dir) {
        while (*dir && *dir == PROPDIR_DELIMITER) {
            dir++;
        }
    }
    if (!dir || !*dir) {
        *list = DBFETCH(player)->properties;
        p = first_node(*list);
        if (p) {
            strcpy(name, PropName(p));
        } else {
            *name = '\0';
        }

		DBUNLOCK(player);
        return (p);
    }

    strcpy(buf, dir);
    *list = p = propdir_get_elem(DBFETCH(player)->properties, buf);
    if (!p) {
        *name = '\0';
		DBUNLOCK(player);
        return NULL;
    }
    *list = PropDir(p);
    p = first_node(*list);
    if (p) {
        strcpy(name, PropName(p));
    } else {
        *name = '\0';
    }

	DBUNLOCK(player);
    return (p);
}


/* first_prop() returns a pointer to the first property.
 * player    dbref of object that the properties are on.
 * dir       pointer to string name of the propdir
 * list      pointer to a proplist pointer.  Returns the root node.
 * name      printer to a string.  Returns the name of the first node.
 */

PropPtr
first_prop(dbref player, const char *dir, PropPtr *list, char *name)
{

#ifdef DISKBASE
    fetchprops(player);
#endif

    return (first_prop_nofetch(player, dir, list, name));
}



/* next_prop() returns a pointer to the next property node.
 * list    Pointer to the root node of the list.
 * prop    Pointer to the previous prop.
 * name    Pointer to a string.  Returns the name of the next property.
 */

PropPtr
next_prop(PropPtr list, PropPtr prop, char *name)
{
    PropPtr p = prop;

    if (!p || !(p = next_node(list, PropName(p))))
        return ((PropPtr) 0);

    strcpy(name, PropName(p));
    return (p);
}



/* next_prop_name() returns a ptr to the string name of the next property.
 * player   object the properties are on.
 * outbuf   pointer to buffer to return the next prop's name in.
 * name     pointer to the name of the previous property.
 *
 * Returns null if propdir doesn't exist, or if no more properties in list.
 * Call with name set to "" to get the first property of the root propdir.
 */

char *
next_prop_name(dbref player, char *outbuf, char *name)
{
    char *ptr;
    char buf[BUFFER_LEN];
    PropPtr p, l;

	DBLOCK(player);

#ifdef DISKBASE
    fetchprops(player);
#endif

    strcpy(buf, name);
    if (!*name || name[strlen(name) - 1] == PROPDIR_DELIMITER) {
        l = DBFETCH(player)->properties;
        p = propdir_first_elem(l, buf);
        if (!p) {
            *outbuf = '\0';
			DBUNLOCK(player);
            return NULL;
        }
        strcat(strcpy(outbuf, name), PropName(p));
    } else {
/*        if (!(get_property(player,name))) {
	   *outbuf = '\0';
	   return NULL;
	}
*/
        l = DBFETCH(player)->properties;
        p = propdir_next_elem(l, buf);
        if (!p) {
            *outbuf = '\0';
			DBUNLOCK(player);
            return NULL;
        }
        strcpy(outbuf, name);
        ptr = rindex(outbuf, PROPDIR_DELIMITER);
        if (!ptr)
            ptr = outbuf;
        *(ptr++) = PROPDIR_DELIMITER;
        strcpy(ptr, PropName(p));
    }

	DBUNLOCK(player);
    return outbuf;
}


int
size_properties(dbref player, int load)
{
#ifdef DISKBASE
    if (load) {
        fetchprops(player);
        fetch_propvals(player, "/");
    }
#endif
    return size_proplist(DBFETCH(player)->properties);
}


/* return true if a property contains a propdir */
int
is_propdir_nofetch(dbref player, const char *type)
{
    PropPtr p;
    char w[BUFFER_LEN];

    strcpy(w, type);
    p = propdir_get_elem(DBFETCH(player)->properties, w);
    if (!p)
        return 0;
    return (PropDir(p) != (PropPtr) NULL);
}


int
is_propdir(dbref player, const char *type)
{

#ifdef DISKBASE
    fetchprops(player);
#endif

    return (is_propdir_nofetch(player, type));
}


PropPtr
regenvprop(dbref *where, const char *propname, int typ)
{
    PropPtr temp;

	DBLOCK(-1);

    temp = get_property(0, propname);
#ifdef DISKBASE
    if (temp)
        propfetch(0, temp);
#endif
    if (temp && (!typ || PropType(temp) == typ)) {
		DBUNLOCK(-1);
        return temp;
	}

    while (*where != NOTHING && *where != 0) {
        temp = get_property(*where, propname);
#ifdef DISKBASE
        if (temp)
            propfetch(*where, temp);
#endif
        if (temp && (!typ || PropType(temp) == typ)) {
			DBUNLOCK(-1);
            return temp;
		}
        *where = getparent(*where);
    }

	DBUNLOCK(-1);
    return NULL;
}


PropPtr
envprop(dbref *where, const char *propname, int typ)
{
    PropPtr temp;

	DBLOCK(-1);

    while (*where != NOTHING) {
        temp = get_property(*where, propname);
#ifdef DISKBASE
        if (temp)
            propfetch(*where, temp);
#endif
        if (temp && (!typ || PropType(temp) == typ)) {
			DBUNLOCK(-1);
            return temp;
		}
        *where = getparent(*where);
    }

	DBUNLOCK(-1);
    return NULL;
}


PropPtr
envprop_cmds(dbref *where, const char *propname, int typ)
{
    PropPtr temp;

	DBLOCK(-1);

    while (*where != NOTHING) {
        if (typ ? 1
            : ((FLAG2(*where) & F2COMMAND) && !(FLAG2(*where) & F2NO_COMMAND)
               && ((FLAGS(OWNER(*where)) & BUILDER)
                   || (MLevel(OWNER(*where)) >= LMAGE)))) {
            temp = get_property(*where, propname);
#ifdef DISKBASE
            if (temp)
                propfetch(*where, temp);
#endif
            if (temp) {
				DBUNLOCK(-1);
                return temp;
			}
        }
        *where = getparent(*where);
    }

	DBUNLOCK(-1);
    return NULL;
}


const char *
envpropstr(dbref *where, const char *propname)
{
    PropPtr temp;

    temp = envprop(where, propname, PROP_STRTYP);
    if (!temp)
        return NULL;
    if (PropType(temp) == PROP_STRTYP)
        return (PropDataUNCStr(temp));
    return NULL;
}


#ifndef SANITY
char *
displayprop(dbref player, dbref obj, const char *name, char *buf)
{
    char mybuf[BUFFER_LEN], tbuf[BUFFER_LEN];
    int pdflag;
    PropPtr p = get_property(obj, name);

    if (!p) {
        sprintf(buf, SYSGLOOM "%s: No such property.", name);
        return buf;
    }
#ifdef DISKBASE
    propfetch(obj, p);
#endif
    pdflag = (PropDir(p) != NULL);
    sprintf(tbuf, "%.*s%c", (BUFFER_LEN / 4), name,
            (pdflag) ? PROPDIR_DELIMITER : '\0');
    tct(tbuf, mybuf);
    switch (PropType(p)) {
        case PROP_STRTYP:
            sprintf(buf, SYSAQUA "str " SYSGREEN "%s" SYSRED ":" SYSCYAN
                    "%.*s", mybuf, (BUFFER_LEN / 2),
                    tct(PropDataUNCStr(p), tbuf));
            break;
        case PROP_REFTYP:
			{
				char bufu[BUFFER_LEN];
				sprintf(buf, SYSBROWN "ref " SYSGREEN "%s" SYSRED ":%s", mybuf,
						ansi_unparse_object(player, PropDataRef(p), bufu));
			}
            break;
        case PROP_INTTYP:
            sprintf(buf, SYSFOREST "int " SYSGREEN "%s" SYSRED ":" SYSYELLOW
                    "%d", mybuf, PropDataVal(p));
            break;
        case PROP_FLTTYP:
            sprintf(buf, SYSNAVY "flt " SYSGREEN "%s" SYSRED ":" SYSBROWN
                    "%.15g", mybuf, PropDataFVal(p));
            break;
        case PROP_LOKTYP:
            if (PropFlags(p) & PROP_ISUNLOADED) {
                sprintf(buf, SYSCRIMSON "lok " SYSGREEN "%s" SYSRED ":"
                        SYSPURPLE "*UNLOCKED*", mybuf);
            } else {
                sprintf(buf, SYSCRIMSON "lok " SYSGREEN "%s" SYSRED ":"
                        SYSPURPLE "%.*s", mybuf, (BUFFER_LEN / 2),
                        tct(unparse_boolexp(player, PropDataLok(p), 1), tbuf));
            }
            break;
        case PROP_DIRTYP:
            sprintf(buf, SYSWHITE "dir " SYSGREEN "%s" SYSRED ":", mybuf);
            break;
    }
    return buf;
}
#endif

int
db_get_single_prop(FILE * f, dbref obj, int pos)
{
    char getprop_buf[BUFFER_LEN * 3];
    char *name, *flags, *value, *p;
    int tpos = 0;
    bool do_diskbase_propvals;
    PData pdat;

#ifdef DISKBASE
    do_diskbase_propvals = tp_diskbase_propvals;
#else
    do_diskbase_propvals = 0;
#endif

    if (pos) {
        fseek(f, pos, 0);
    } else if (do_diskbase_propvals) {
        tpos = ftell(f);
    }
    name = fgets(getprop_buf, sizeof(getprop_buf), f);
    if (!name) {
        fprintf(stderr, "PANIC: Error reading property in from db file.\n");
        abort();
    }
    if (*name == '*') {
        if (!strcmp(name, "*End*\n")) {
            return 0;
        }
    }

    flags = index(name, PROP_DELIMITER);
    if (!flags) {               /* this usually means the db file has new lines in the
                                 * middle of properties. Something like this can be
                                 * salvaged by editing the props in question.
                                 */
        fprintf(stderr,
                "PANIC: Unable to find prop flags while loading props.\n");
        abort();
    }
    *flags++ = '\0';

    value = index(flags, PROP_DELIMITER);
    if (!value) {
        fprintf(stderr,
                "PANIC: Unable to find prop value while loading props.\n");
        abort();
    }
    *value++ = '\0';

    p = index(value, '\n');
    if (p)
        *p = '\0';

    if (!number(flags)) {
        fprintf(stderr, "PANIC: Prop flag was not a number. DB error.\n");
        abort();
    }

    pdat.flags = atoi(flags);

    switch (pdat.flags & PROP_TYPMASK) {
        case PROP_STRTYP:
            if (!do_diskbase_propvals || pos) {
                pdat.flags &= ~PROP_ISUNLOADED;
#if defined(COMPRESS) && defined(ARCHAIC_DATABASES)
                if (!(pdat.flags & PROP_COMPRESSED))
                    value = (char *) old_uncompress(value);
#endif
                pdat.data.str = value;
            } else {
                pdat.flags |= PROP_ISUNLOADED;
                pdat.data.pos = tpos;
            }
            break;
        case PROP_LOKTYP:
            if (!do_diskbase_propvals || pos) {
                pdat.flags &= ~PROP_ISUNLOADED;
                pdat.data.lok = parse_boolexp(-1, (dbref) 1, value, 32767);
            } else {
                pdat.flags |= PROP_ISUNLOADED;
                pdat.data.pos = tpos;
            }
            break;
        case PROP_INTTYP:
            if (!number(value)) {
                fprintf(stderr, "PANIC: INT prop had non-int value in db.\n");
                abort();
            }
            pdat.data.val = atoi(value);
            break;
        case PROP_FLTTYP:
            if (!number(value) && !ifloat(value)) {
                char *tpnt = value;
                int dtemp = 0;

                if ((*tpnt == '+') || (*tpnt == '-')) {
                    if (*tpnt == '+') {
                        dtemp = 0;
                    } else {
                        dtemp = 1;
                    }
                    tpnt++;
                }
                tpnt[0] = toupper(tpnt[0]);
                tpnt[1] = toupper(tpnt[1]);
                tpnt[2] = toupper(tpnt[2]);
                if (!strncmp(tpnt, "INF", 3)) {
                    if (!dtemp)
                        pdat.data.fval = INF;
                    else
                        pdat.data.fval = NINF;
                } else {
                    if (!strncmp(tpnt, "NAN", 3)) {
                        pdat.data.fval = NAN;
                    } else {
                        fprintf(stderr,
                                "PANIC:Float prop contained invalid value.\n");
                        abort();
                    }
                }
            } else {
                sscanf(value, "%lg", (double *) &pdat.data.fval);
            }
            break;
        case PROP_REFTYP:
            if (!number(value)) {
                fprintf(stderr,
                        "PANIC:Ref prop contained non-numeric value.\n");
                abort();
            }
            pdat.data.ref = atoi(value);
            break;
        case PROP_DIRTYP:
            break;
    }
    set_property_nofetch(obj, name, &pdat, 1);

    return 1;
}


void
db_getprops(FILE * f, dbref obj)
{
    while (db_get_single_prop(f, obj, 0L)) ;
}


void
db_putprop(FILE * f, const char *dir, PropPtr p)
{
    char buf[BUFFER_LEN * 2];
    char fbuf[BUFFER_LEN];
    char num[16];
    char *ptr;
    const char *ptr2;


    if (PropType(p) == PROP_DIRTYP)
        return;

    for (ptr = buf, ptr2 = dir + 1; *ptr2;)
        *ptr++ = *ptr2++;
    for (ptr2 = PropName(p); *ptr2;)
        *ptr++ = *ptr2++;
    *ptr++ = PROP_DELIMITER;

    ptr2 =
        intostr(num,
                PropFlagsRaw(p) & ~(PROP_TOUCHED | PROP_ISUNLOADED |
                                    PROP_NOASCIICHK));
    while (*ptr2)
        *ptr++ = *ptr2++;
    *ptr++ = PROP_DELIMITER;

    ptr2 = "";
    switch (PropType(p)) {
        case PROP_INTTYP:
            if (!PropDataVal(p))
                return;
            ptr2 = intostr(num, PropDataVal(p));
            break;
        case PROP_FLTTYP:
            if (!PropDataFVal(p))
                return;
            ptr2 = fltostr(fbuf, PropDataFVal(p));
            break;
        case PROP_REFTYP:
            if (PropDataRef(p) == NOTHING)
                return;
            ptr2 = intostr(num, (int) PropDataRef(p));
            break;
        case PROP_STRTYP:
            if (!*PropDataStr(p))
                return;
            if (db_decompression_flag) {
                ptr2 = PropDataUNCStr(p);
            } else {
                ptr2 = PropDataStr(p);
            }
            break;
        case PROP_LOKTYP:
            if (PropFlags(p) & PROP_ISUNLOADED)
                return;
            if (PropDataLok(p) == TRUE_BOOLEXP)
                return;
            ptr2 = unparse_boolexp((dbref) 1, PropDataLok(p), 0);
            break;
    }
    while (*ptr2)
        if (*ptr2 != '\n')
            *ptr++ = *ptr2++;
        else {
            *ptr++ = '\\';
            *ptr++ = 'n';
            ptr2++;
        }
    *ptr++ = '\n';
    *ptr++ = '\0';
    if (fputs(buf, f) == EOF) {
        fprintf(stderr, "PANIC: Unable to write to db to write prop.\n");
        abort();
    }
}


void
db_dump_props_rec(dbref obj, FILE * f, const char *dir, PropPtr p)
{
    char buf[BUFFER_LEN];

#ifdef DISKBASE
    int tpos = 0;
    int flg;
    short wastouched = 0;
#endif

    if (!p)
        return;

    db_dump_props_rec(obj, f, dir, AVL_LF(p));
#ifdef DISKBASE
    if (tp_diskbase_propvals) {
        tpos = ftell(f);
        wastouched = (PropFlags(p) & PROP_TOUCHED);
    }
    if (propfetch(obj, p)) {
        fseek(f, 0L, 2);
    }
#endif
    db_putprop(f, dir, p);
#ifdef DISKBASE
    if (tp_diskbase_propvals && !wastouched) {
        if (PropType(p) == PROP_STRTYP || PropType(p) == PROP_LOKTYP) {
            flg = PropFlagsRaw(p) | PROP_ISUNLOADED;
            clear_propnode(p);
            SetPFlagsRaw(p, flg);
            SetPDataVal(p, tpos);
        }
    }
#endif
    if (PropDir(p)) {
        sprintf(buf, "%s%s%c", dir, PropName(p), PROPDIR_DELIMITER);
        db_dump_props_rec(obj, f, buf, PropDir(p));
    }
    db_dump_props_rec(obj, f, dir, AVL_RT(p));
}


void
db_dump_props(FILE * f, dbref obj)
{
    db_dump_props_rec(obj, f, "/", DBFETCH(obj)->properties);
}


void
untouchprop_rec(PropPtr p)
{
    if (!p)
        return;
    SetPFlags(p, (PropFlags(p) & ~PROP_TOUCHED));
    untouchprop_rec(AVL_LF(p));
    untouchprop_rec(AVL_RT(p));
    untouchprop_rec(PropDir(p));
}

static dbref untouch_lastdone = 0;
void
untouchprops_incremental(int limit)
{
    PropPtr p;

    while (untouch_lastdone < db_top) {
        /* clear the touch flags */
        p = DBFETCH(untouch_lastdone)->properties;
        if (p) {
            if (!limit--)
                return;
            untouchprop_rec(p);
        }
        untouch_lastdone++;
    }
    untouch_lastdone = 0;
}


int
Prop_SysPerms(dbref player, const char *type)
{
    if (get_property_flags(player, type) & PROP_SYSPERMS)
        return 1;
    else
        return 0;
}

void
reflist_add(dbref obj, const char *propname, dbref toadd)
{
    PropPtr ptr;
    const char *temp;
    const char *list;
    int count = 0;
    int charcount = 0;
    char buf[BUFFER_LEN];
    char outbuf[BUFFER_LEN];

	DBLOCK(obj);

    ptr = get_property(obj, propname);
    if (ptr) {
        const char *pat = NULL;

#ifdef DISKBASE
        propfetch(obj, ptr);
#endif
        switch (PropType(ptr)) {
            case PROP_STRTYP:
                *outbuf = '\0';
                list = temp = PropDataUNCStr(ptr);
                sprintf(buf, "%d", toadd);
                while (*temp) {
                    if (*temp == '#') {
                        pat = buf;
                        count++;
                        charcount = temp - list;
                    } else if (pat) {
                        if (!*pat) {
                            if (!*temp || *temp == ' ') {
                                break;
                            }
                            pat = NULL;
                        } else if (*pat != *temp) {
                            pat = NULL;
                        } else {
                            pat++;
                        }
                    }
                    temp++;
                }
                if (pat && !*pat) {
                    if (charcount > 0) {
                        strncpy(outbuf, list, charcount - 1);
                        outbuf[charcount - 1] = '\0';
                    }
                    strcat(outbuf, temp);
                } else {
                    strcpy(outbuf, list);
                }
                sprintf(buf, " #%d", toadd);
                if (strlen(outbuf) + strlen(buf) < BUFFER_LEN) {
                    strcat(outbuf, buf);
                    for (temp = outbuf; isspace(*temp); temp++) ;
                    add_property(obj, propname, temp, 0);
                }
                break;
            case PROP_REFTYP:
                if (PropDataRef(ptr) != toadd) {
                    sprintf(outbuf, "#%d #%d", PropDataRef(ptr), toadd);
                    add_property(obj, propname, outbuf, 0);
                }
                break;
            default:
                sprintf(outbuf, "#%d", toadd);
                add_property(obj, propname, outbuf, 0);
                break;
        }
    } else {
        sprintf(outbuf, "#%d", toadd);
        add_property(obj, propname, outbuf, 0);
    }

	DBUNLOCK(obj);
}


void
reflist_del(dbref obj, const char *propname, dbref todel)
{
    PropPtr ptr;
    const char *temp;
    const char *list;
    int count = 0;
    int charcount = 0;
    char buf[BUFFER_LEN];
    char outbuf[BUFFER_LEN];

	DBLOCK(obj);

    ptr = get_property(obj, propname);
    if (ptr) {
        const char *pat = NULL;

#ifdef DISKBASE
        propfetch(obj, ptr);
#endif
        switch (PropType(ptr)) {
            case PROP_STRTYP:
                *outbuf = '\0';
                list = temp = PropDataUNCStr(ptr);
                sprintf(buf, "%d", todel);
                while (*temp) {
                    if (*temp == '#') {
                        pat = buf;
                        count++;
                        charcount = temp - list;
                    } else if (pat) {
                        if (!*pat) {
                            if (!*temp || *temp == ' ') {
                                break;
                            }
                            pat = NULL;
                        } else if (*pat != *temp) {
                            pat = NULL;
                        } else {
                            pat++;
                        }
                    }
                    temp++;
                }
                if (pat && !*pat) {
                    if (charcount > 0) {
                        strncpy(outbuf, list, charcount - 1);
                        outbuf[charcount - 1] = '\0';
                    }
                    strcat(outbuf, temp);
                    for (temp = outbuf; isspace(*temp); temp++) ;
                    add_property(obj, propname, temp, 0);
                }
                break;
            case PROP_REFTYP:
                if (PropDataRef(ptr) == todel) {
                    add_property(obj, propname, "", 0);
                }
                break;
            default:
                break;
        }
    }
	DBUNLOCK(obj);
}


int
reflist_find(dbref obj, const char *propname, dbref tofind)
{
    PropPtr ptr;
    const char *temp;
    int pos = 0;
    int count = 0;
    char buf[BUFFER_LEN];

	DBLOCK(obj);

    ptr = get_property(obj, propname);
    if (ptr) {
        const char *pat = NULL;

#ifdef DISKBASE
        propfetch(obj, ptr);
#endif
        switch (PropType(ptr)) {
            case PROP_STRTYP:
                temp = PropDataUNCStr(ptr);
                sprintf(buf, "%d", tofind);
                while (*temp) {
                    if (*temp == '#') {
                        pat = buf;
                        count++;
                    } else if (pat) {
                        if (!*pat) {
                            if (!*temp || *temp == ' ') {
                                break;
                            }
                            pat = NULL;
                        } else if (*pat != *temp) {
                            pat = NULL;
                        } else {
                            pat++;
                        }
                    }
                    temp++;
                }
                if (pat && !*pat) {
                    pos = count;
                }
                break;
            case PROP_REFTYP:
                if (PropDataRef(ptr) == tofind)
                    pos = 1;
                break;
            default:
                break;
        }
    }

	DBUNLOCK(obj);
    return pos;
}
