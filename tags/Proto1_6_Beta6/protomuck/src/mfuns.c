
#include "config.h"

#ifdef MPI

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


/***** Insert MFUNs here *****/

const char *
mfn_func(MFUNARGS)
{
    char *funcname;
    char *ptr=NULL, *def;
    int i;

    funcname = MesgParse(argv[0], argv[0]);
    CHECKRETURN(ptr,"FUNC","Name argument (1)");

    def = argv[argc-1];
    for (i = 1; i < argc - 1; i++) {
	ptr = MesgParse(argv[i], argv[i]);
	CHECKRETURN(ptr,"FUNC","variable name argument");
	sprintf(buf,"{with:%.*s,{:%d},%.*s}", MAX_MFUN_NAME_LEN, ptr, i,
		(BUFFER_LEN - MAX_MFUN_NAME_LEN - 20), def);
	strcpy(def, buf);
    }
    i = new_mfunc(funcname, def);
    if (i == 1)
	ABORT_MPI("FUNC","Function Name too long.");
    if (i == 2)
	ABORT_MPI("FUNC","Too many functions defined.");

    return "";
}


const char *
mfn_muckname(MFUNARGS)
{
    return tp_muckname;
}


const char *
mfn_version(MFUNARGS)
{
    return VERSION;
}


const char *
mfn_prop(MFUNARGS)
{
    dbref   obj = what;
    const char   *ptr, *pname;

    pname = argv[0];
    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1]);
    }
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("PROP","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("PROP",tp_noperm_mesg);
    ptr = safegetprop(player, obj, perms, pname);
    if (!ptr) ABORT_MPI("PROP","Failed read.");
    return ptr;
}


const char *
mfn_propbang(MFUNARGS)
{
    dbref   obj = what;
    const char   *ptr, *pname;

    pname = argv[0];
    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1]);
    }
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("PROP","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("PROP",tp_noperm_mesg);
    ptr = safegetprop_strict(player, obj, perms, pname);
    if (!ptr) ABORT_MPI("PROP","Failed read.");
    return ptr;
}


const char *
mfn_store(MFUNARGS)
{
    dbref   obj = what;
    char   *ptr, *pname;

    pname = argv[1];
    ptr = argv[0];
    if (argc > 2) {
        obj = mesg_dbref_strict(descr, player, what, perms, argv[2]);
    }
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("STORE","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("STORE",tp_noperm_mesg);
    if (!safeputprop(obj, perms, pname, ptr))
        ABORT_MPI("STORE",tp_noperm_mesg);
    return ptr;
}


const char *
mfn_delprop(MFUNARGS)
{
    dbref   obj = what;
    char   *pname;

    pname = argv[0];
    if (argc > 1) {
        obj = mesg_dbref_strict(descr, player, what, perms, argv[1]);
    }
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("DELPROP","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("DELPROP",tp_noperm_mesg);
    if (!safeputprop(obj, perms, pname, NULL))
        ABORT_MPI("DELPROP",tp_noperm_mesg);
    return "";
}


const char *
mfn_exec(MFUNARGS)
{
    dbref   trg, obj = what;
    const char   *ptr, *pname;

    pname = argv[0];
    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1]);
    }
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("EXEC","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("EXEC",tp_noperm_mesg);
    while (*pname == PROPDIR_DELIMITER) pname++;
    ptr = safegetprop(player, obj, perms, pname);
    if (!ptr) ABORT_MPI("EXEC","Failed read.");
    trg = what;
    if (Prop_ReadOnly(pname) || Prop_Private(pname) || Prop_SeeOnly(pname))
        trg = obj;
    ptr = mesg_parse(descr, player, obj, trg, ptr, buf, BUFFER_LEN, mesgtyp);
    CHECKRETURN(ptr,"EXEC","propval");
    return ptr;
}


const char *
mfn_execbang(MFUNARGS)
{
    dbref   trg, obj = what;
    const char   *ptr, *pname;

    pname = argv[0];
    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1]);
    }
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("EXEC!","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("EXEC!",tp_noperm_mesg);
    while (*pname == PROPDIR_DELIMITER) pname++;
    ptr = safegetprop_strict(player, obj, perms, pname);
    if (!ptr) ABORT_MPI("EXEC!","Failed read.");
    trg = what;
    if (Prop_ReadOnly(pname) || Prop_Private(pname) || Prop_SeeOnly(pname))
        trg = obj;
    ptr = mesg_parse(descr, player, obj, trg, ptr, buf, BUFFER_LEN, mesgtyp);
    CHECKRETURN(ptr,"EXEC!","propval");
    return ptr;
}


const char *
mfn_index(MFUNARGS)
{
    dbref   obj = what;
    dbref   tmpobj;
    const char   *pname, *ptr;

    pname = argv[0];
    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1]);
    }
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
	ABORT_MPI("INDEX","Match failed.");
    if (obj == PERMDENIED)
	ABORT_MPI("INDEX",tp_noperm_mesg);
    tmpobj = obj;
    ptr = safegetprop(player, obj, perms, pname);
    if (!ptr) ABORT_MPI("INDEX","Failed read.");
    if (!*ptr) return "";
    obj = tmpobj;
    ptr = safegetprop(player, obj, perms, ptr);
    if (!ptr) ABORT_MPI("INDEX","Failed read.");
    return ptr;
}



const char *
mfn_indexbang(MFUNARGS)
{
    dbref   obj = what;
    dbref   tmpobj;
    const char   *pname, *ptr;

    pname = argv[0];
    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1]);
    }
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
	ABORT_MPI("INDEX","Match failed.");
    if (obj == PERMDENIED)
	ABORT_MPI("INDEX",tp_noperm_mesg);
    tmpobj = obj;
    ptr = safegetprop_strict(player, obj, perms, pname);
    if (!ptr) ABORT_MPI("INDEX","Failed read.");
    if (!*ptr) return "";
    obj = tmpobj;
    ptr = safegetprop_strict(player, obj, perms, ptr);
    if (!ptr) ABORT_MPI("INDEX","Failed read.");
    return ptr;
}



const char *
mfn_propdir(MFUNARGS)
{
    dbref obj = what;
    const char *pname;

    pname = argv[0];
    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1]);
    }
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("PROPDIR","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("PROPDIR",tp_noperm_mesg);

    if (is_propdir(obj, pname)) {
        return "1";
    } else {
        return "0";
    }
}


const char *
mfn_listprops(MFUNARGS)
{
    dbref         obj = what;
    char         *ptr, *pname;
    char         *endbuf, *pattern;
    char          tmpbuf[BUFFER_LEN];
    char          patbuf[BUFFER_LEN];
    int           flag;

    pname = argv[0];
    if (argc > 1) {
        obj = mesg_dbref(descr, player, what, perms, argv[1]);
    }
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("LISTPROPS","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("LISTPROPS",tp_noperm_mesg);

    if (argc > 2) {
        pattern = argv[2];
    } else {
        pattern = NULL;
    }

    endbuf = pname + strlen(pname);
    if (endbuf != pname) {
        endbuf--;
    }
    if (*endbuf != PROPDIR_DELIMITER &&
	(endbuf - pname) < (BUFFER_LEN - 2))
    {
        if (*endbuf) endbuf++;
        *endbuf++ = PROPDIR_DELIMITER;
        *endbuf++ = '\0';
    }

    *buf = '\0';
    endbuf = buf;
    do {
	ptr = next_prop_name(obj, tmpbuf, pname);
	if (ptr && *ptr) {
	    flag = 1;
	    if (!Wizperms(what)) {
	        if (Prop_Hidden(ptr)) {
	            flag = 0;
	        }
	        if (Prop_Private(ptr) && OWNER(what) != OWNER(obj)) {
	            flag = 0;
	        }
	        if (obj != player && OWNER(obj) != OWNER(what)) {
	            flag = 0;
	        }
	    }
	    if (flag && pattern) {
	        char *nptr;

	        nptr = rindex(ptr, PROPDIR_DELIMITER);
	        if (nptr && *nptr) {
		    strcpy(patbuf, ++nptr);
		    if (!equalstr(pattern, patbuf)) {
			flag = 0;
		    }
	        }
	    }
	    if (flag) {
              int entrylen = strlen(ptr);
              if ((endbuf - buf) + entrylen + 2 < BUFFER_LEN) {
	           if (*buf) {
		       *endbuf++ = '\r';
	           }
	           strcpy(endbuf, ptr);
	           endbuf += entrylen;
              }
	    }
	}
	pname = ptr;
    } while (ptr && *ptr);

    return buf;
}


const char *
mfn_concat(MFUNARGS)
{
    dbref   obj = what;
    char   *pname;
    const char *ptr;

    pname = argv[0];
    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1]);
    }
    if (obj == PERMDENIED)
	ABORT_MPI("CONCAT",tp_noperm_mesg);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
	ABORT_MPI("CONCAT","Match failed.");
    ptr = get_concat_list(player, what, perms, obj, pname, buf, BUFFER_LEN, 1);
    if (!ptr) ABORT_MPI("CONCAT","Failed list read.");
    return ptr;
}



const char *
mfn_select(MFUNARGS)
{
	char origprop[BUFFER_LEN];
	char propname[BUFFER_LEN];
	char bestname[BUFFER_LEN];
	dbref obj = what;
	dbref bestobj = NOTHING;
	char *pname;
	const char *ptr;
	char *out, *in;
	int i, targval, bestval;
	int baselen;
	int limit;

	pname = argv[1];
	if (argc == 3) {
		obj = mesg_dbref(descr, player, what, perms, argv[2]);
	}
	if (obj == PERMDENIED)
		ABORT_MPI("LIST", tp_noperm_mesg);
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
		ABORT_MPI("LIST", "Match failed.");

	/*
	 * Search contiguously for a bit, looking for a best match.
	 * This allows fast hits on LARGE lists.
	 */

	limit = 18;
	i = targval = atoi(argv[0]);
	do {
		ptr = get_list_item(player, obj, perms, pname, i--);
	} while (limit-->0 && i >= 0 && ptr && !*ptr);
	if (!ptr)
		ABORT_MPI("SELECT", "Failed list read.");
	if (*ptr)
		return ptr;

	/*
	 * If we didn't find it before, search only existing props.
	 * This gets fast hits on very SPARSE lists.
	 */

	/* First, normalize the base propname */
	out = origprop;
	in = argv[1];
	while (*in) {
		*out++ = PROPDIR_DELIMITER;
		while (*in == PROPDIR_DELIMITER) in++;
		while (*in && *in != PROPDIR_DELIMITER) *out++ = *in++;
	}
	*out++ = '\0';

	i = targval;
	bestname[0] = '\0';
	bestval = 0;
	baselen = strlen(origprop);
	for (; obj != NOTHING; obj = getparent(obj)) {
		pname = next_prop_name(obj, propname, origprop);
		while (pname && string_prefix(pname, origprop)) {
			ptr = pname + baselen;
			if (*ptr == '#') ptr++;
			if (!*ptr && is_propdir(obj, pname)) {
				char propname2[BUFFER_LEN];
				char *pname2;
				int sublen = strlen(pname);

				pname2 = strcpy(propname2, pname);
				propname2[sublen++] = PROPDIR_DELIMITER;
				propname2[sublen] = '\0';

				pname2 = next_prop_name(obj, propname2, pname2);
				while (pname2) {
					ptr = pname2 + sublen;
					if (number(ptr)) {
						i = atoi(ptr);
						if (bestval < i && i <= targval) {
							bestval = i;
							bestobj = obj;
							strcpy(bestname, pname2);
						}
					}
					pname2 = next_prop_name(obj, propname2, pname2);
				}
			}
			ptr = pname + baselen;
			if (number(ptr)) {
				i = atoi(ptr);
				if (bestval < i && i <= targval) {
					bestval = i;
					bestobj = obj;
					strcpy(bestname, pname);
				}
			}
			pname = next_prop_name(obj, propname, pname);
		}
	}
	
	if (*bestname) {
		ptr = safegetprop_strict(player, bestobj, perms, bestname);
		if (!ptr)
			ABORT_MPI("SELECT", "Failed property read.");
	} else {
		ptr = "";
	}
	return ptr;
}


const char *
mfn_list(MFUNARGS)
{
    dbref   obj = what;
    char   *pname;
    const char *ptr;

    pname = argv[0];
    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1]);
    }
    if (obj == PERMDENIED)
	ABORT_MPI("LIST",tp_noperm_mesg);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
	ABORT_MPI("LIST","Match failed.");
    ptr = get_concat_list(player, what, perms, obj, pname, buf, BUFFER_LEN, 0);
    if (!ptr) ptr = "";
    return ptr;
}


const char *
mfn_lexec(MFUNARGS)
{
    dbref   trg, obj = what;
    char   *pname;
    const char *ptr;

    pname = argv[0];
    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1]);
    }
    if (obj == PERMDENIED)
	ABORT_MPI("LEXEC",tp_noperm_mesg);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
	ABORT_MPI("LEXEC","Match failed.");
    while (*pname == PROPDIR_DELIMITER) pname++;
    ptr = get_concat_list(player, what, perms, obj, pname, buf, BUFFER_LEN, 2);
    if (!ptr) ptr = "";
    trg = what;
    if (Prop_ReadOnly(pname) || Prop_Private(pname) || Prop_SeeOnly(pname))
        trg = obj;
    ptr = mesg_parse(descr, player, obj, trg, ptr, buf, BUFFER_LEN, mesgtyp);
    CHECKRETURN(ptr,"LEXEC","listval");
    return ptr;
}



const char *
mfn_rand(MFUNARGS)
{
    int     num;
    dbref   obj = what;
    const char   *pname, *ptr;

    pname = argv[0];
    if (argc == 2) {
        obj = mesg_dbref(descr, player, what, perms, argv[1]);
    }
    if (obj == PERMDENIED)
	ABORT_MPI("RAND",tp_noperm_mesg);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
	ABORT_MPI("RAND","Match failed.");
    num = get_list_count(what, obj, perms, pname);
    if (!num) ABORT_MPI("RAND","Failed list read.");
    ptr = get_list_item(what, obj, perms, pname, (((RANDOM() / 256) % num) + 1));
    if (!ptr) ABORT_MPI("RAND","Failed list read.");
    return ptr;
}


const char *
mfn_timesub(MFUNARGS)
{
    int     num;
    dbref   obj = what;
    const char   *pname, *ptr;
    int period;
    time_t offset;

    period = atoi(argv[0]);
    offset = atoi(argv[1]);
    pname = argv[2];
    if (argc == 4) {
        obj = mesg_dbref(descr, player, what, perms, argv[3]);
    }
    if (obj == PERMDENIED)
	ABORT_MPI("TIMESUB",tp_noperm_mesg);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
	ABORT_MPI("TIMESUB","Match failed.");
    num = get_list_count(what, obj, perms, pname);
    if (!num) ABORT_MPI("TIMESUB","Failed list read.");
    if (period < 1)
	ABORT_MPI("TIMESUB", "Time period too short.");
    offset = ((((time_t)time(NULL) + offset) % period) * num) / period;
    if (offset < 0) offset = -offset;
    ptr = get_list_item(what, obj, perms, pname, offset + 1);
    if (!ptr) ABORT_MPI("TIMESUB","Failed list read.");
    return ptr;
}


const char *
mfn_nl(MFUNARGS)
{
    return "\r";
}


const char *
mfn_lit(MFUNARGS)
{
    int i, len, len2;
    strcpy(buf, argv[0]);
    len = strlen(buf);
    for (i = 1; i < argc; i++) {
	len2 = strlen(argv[i]);
	if (len2 + len + 3 >= BUFFER_LEN) {
	    if (len + 3 < BUFFER_LEN) {
		strncpy(buf+len, argv[i], (BUFFER_LEN - len - 3));
		buf[BUFFER_LEN - 3] = '\0';
	    }
	    break;
	}
	strcpy(buf + len, ",");
	strcpy(buf + len, argv[i]);
	len += len2;
    }
    return buf;
}


const char *
mfn_strip(MFUNARGS)
{
    int i, len, len2;
    char *ptr;

    for (ptr = argv[0]; *ptr == ' '; ptr++);
    strcpy(buf, ptr);
    len = strlen(buf);
    for (i = 1; i < argc; i++) {
	len2 = strlen(argv[i]);
	if (len2 + len + 3 >= BUFFER_LEN) {
	    if (len + 3 < BUFFER_LEN) {
		strncpy(buf+len, argv[i], (BUFFER_LEN - len - 3));
		buf[BUFFER_LEN - 3] = '\0';
	    }
	    break;
	}
	strcpy(buf + len, ",");
	strcpy(buf + len, argv[i]);
	len += len2;
    }
    ptr = &buf[strlen(buf) - 1];
    while (ptr >= buf && isspace(*ptr)) *(ptr--) = '\0';
    return buf;
}


const char *
mfn_mklist(MFUNARGS)
{
    int i, len, tlen;
    tlen = 0;
    *buf = '\0';
    for (i = 0; i < argc; i++) {
        len = strlen(argv[i]);
        if (tlen + len + 2 < BUFFER_LEN) {
            if (*buf) strcat(buf, "\r");
            strcat(buf, argv[i]);
            tlen += len;
        } else {
            ABORT_MPI("MKLIST","Max string length exceeded.");
        }
    }
    return buf;
}


const char *
mfn_pronouns(MFUNARGS)
{
    dbref obj = player;
    if (argc > 1) {
	obj = mesg_dbref_local(descr, player, what, perms, argv[1]);
	if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
	    ABORT_MPI("PRONOUNS","Match failed.");
	if (obj == PERMDENIED)
	    ABORT_MPI("PRONOUNS",tp_noperm_mesg);
    }
    return pronoun_substitute(descr, obj, argv[0]);
}


const char *
mfn_ontime(MFUNARGS)
{
    dbref obj = mesg_dbref_raw(descr, player, what, perms, argv[0]);
    int conn;
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
	return "-1";
    if (obj == PERMDENIED)
	ABORT_MPI("ONTIME", tp_noperm_mesg);
    if (Typeof(obj) != TYPE_PLAYER) obj = OWNER(obj);
    conn = least_idle_player_descr(obj);
    if (!conn) return "-1";
    sprintf(buf, "%d", pontime(conn));
    return buf;
}


const char *
mfn_idle(MFUNARGS)
{
    dbref obj = mesg_dbref_raw(descr, player, what, perms, argv[0]);
    int conn;
    if (obj == PERMDENIED)
	ABORT_MPI("IDLE",tp_noperm_mesg);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
	return "-1";
    if (Typeof(obj) != TYPE_PLAYER) obj = OWNER(obj);
    conn = least_idle_player_descr(obj);
    if (!conn) return "-1";
    sprintf(buf, "%d", pidle(conn));
    return buf;
}


const char *
mfn_online(MFUNARGS)
{
    int list_limit = MAX_MFUN_LIST_LEN;
    int count = pcount();
    char buf2[BUFFER_LEN];

    if (!Wizperms(what))
        ABORT_MPI("ONLINE",tp_noperm_mesg);
    *buf = '\0';
    while (count && list_limit--) {
        if (*buf) strcat(buf, "\r");
        ref2str(pdbref(count), buf2);
        if ((strlen(buf) + strlen(buf2)) >= (BUFFER_LEN - 3))
            break;
        strcat(buf, buf2);
        count--;
    }
    return buf;
}


int
msg_compare(const char *s1, const char *s2)
{
    if (*s1 && *s2 && number(s1) && number(s2)) {
        return (atoi(s1) - atoi(s2));
    } else {
        return string_compare(s1, s2);
    }
}


const char *
mfn_contains(MFUNARGS)
{
    dbref obj2 = mesg_dbref_raw(descr, player, what, perms, argv[0]);
    dbref obj1 = player;
    if (argc > 1)
	obj1 = mesg_dbref_local(descr, player, what, perms, argv[1]);

    if (obj2 == UNKNOWN || obj2 == AMBIGUOUS || obj2 == NOTHING || obj2 == HOME)
	ABORT_MPI("CONTAINS","Match failed (1).");
    if (obj2 == PERMDENIED)
	ABORT_MPI("CONTAINS",tp_noperm_mesg);

    if (obj1 == UNKNOWN || obj1 == AMBIGUOUS || obj1 == NOTHING || obj1 == HOME)
	ABORT_MPI("CONTAINS","Match failed (2).");
    if (obj1 == PERMDENIED)
	ABORT_MPI("CONTAINS",tp_noperm_mesg);

    while (obj2 != NOTHING && obj2 != obj1) obj2 = getloc(obj2);
    if (obj1 == obj2) {
        return "1";
    } else {
        return "0";
    }
}


const char *
mfn_holds(MFUNARGS)
{
    dbref obj1 = mesg_dbref_raw(descr, player, what, perms, argv[0]);
    dbref obj2 = player;
    if (argc > 1)
	obj2 = mesg_dbref_local(descr, player, what, perms, argv[1]);

    if (obj1 == UNKNOWN || obj1 == AMBIGUOUS || obj1 == NOTHING || obj1 == HOME)
	ABORT_MPI("HOLDS","Match failed (1).");
    if (obj1 == PERMDENIED)
	ABORT_MPI("HOLDS",tp_noperm_mesg);

    if (obj2 == UNKNOWN || obj2 == AMBIGUOUS || obj2 == NOTHING || obj2 == HOME)
	ABORT_MPI("HOLDS","Match failed (2).");
    if (obj2 == PERMDENIED)
	ABORT_MPI("HOLDS",tp_noperm_mesg);

    if (obj2 == getloc(obj1)) {
        return "1";
    } else {
        return "0";
    }
}


const char *
mfn_dbeq(MFUNARGS)
{
    dbref obj1 = mesg_dbref_raw(descr, player, what, perms, argv[0]);
    dbref obj2 = mesg_dbref_raw(descr, player, what, perms, argv[1]);

    if (obj1 == UNKNOWN || obj1 == PERMDENIED)
	ABORT_MPI("DBEQ","Match failed (1).");
    if (obj2 == UNKNOWN || obj2 == PERMDENIED)
	ABORT_MPI("DBEQ","Match failed (2).");

    if (obj1 == obj2) {
        return "1";
    } else {
        return "0";
    }
}


const char *
mfn_ne(MFUNARGS)
{
    if (msg_compare(argv[0], argv[1]) == 0) {
        return "0";
    } else {
        return "1";
    }
}


const char *
mfn_eq(MFUNARGS)
{
    if (msg_compare(argv[0], argv[1]) == 0) {
        return "1";
    } else {
        return "0";
    }
}


const char *
mfn_gt(MFUNARGS)
{
    if (msg_compare(argv[0], argv[1]) > 0) {
        return "1";
    } else {
        return "0";
    }
}


const char *
mfn_lt(MFUNARGS)
{
    if (msg_compare(argv[0], argv[1]) < 0) {
        return "1";
    } else {
        return "0";
    }
}


const char *
mfn_ge(MFUNARGS)
{
    if (msg_compare(argv[0], argv[1]) >= 0) {
        return "1";
    } else {
        return "0";
    }
}


const char *
mfn_le(MFUNARGS)
{
    if (msg_compare(argv[0], argv[1]) <= 0) {
        return "1";
    } else {
        return "0";
    }
}


const char *
mfn_min(MFUNARGS)
{
    int j, i = atoi(argv[0]);
    for (j = 0; j < argc; j++)
        if ( j == 0 ) {
           i = atoi(argv[0]) ;
        } else {
           if ( atoi(argv[j]) < i ) { i = atoi(argv[j]) ; }
        }
    sprintf(buf, "%d", i);
    return (buf);
}


const char *
mfn_max(MFUNARGS)
{
    int j, i = atoi(argv[0]);
    for (j = 0; j < argc; j++)
        if ( j == 0 ) {
           i = atoi(argv[0]) ;
        } else {
           if ( atoi(argv[j]) > i ) { i = atoi(argv[j]) ; }
        }
    sprintf(buf, "%d", i);
    return (buf);
}


const char *
mfn_isnum(MFUNARGS)
{
    if (*argv[0] && number(argv[0])) {
        return "1";
    } else {
        return "0";
    }
}


const char *
mfn_isdbref(MFUNARGS)
{
    dbref obj;
    char *ptr = argv[0];
    while (isspace(*ptr)) ptr++;
    if (*ptr++ != '#') return "0";
    if (!number(ptr)) return "0";
    obj = (dbref)atoi(ptr);
    if (obj < 0 || obj >= db_top) return "0";
    if (Typeof(obj) == TYPE_GARBAGE) return "0";
    return "1";
}



const char *
mfn_inc(MFUNARGS)
{
    int x = 1;
    char *ptr = get_mvar(argv[0]);
    if (!ptr)
        ABORT_MPI("INC","No such variable currently defined.");
    if (argc > 1)
         x = atoi(argv[1]);
    sprintf(buf, "%d", (atoi(ptr) + x));
    strcpy(ptr, buf);
    return (buf);
}


const char *
mfn_dec(MFUNARGS)
{
    int x = 1;
    char *ptr = get_mvar(argv[0]);
    if (!ptr)
        ABORT_MPI("DEC","No such variable currently defined.");
    if (argc > 1)
         x = atoi(argv[1]);
    sprintf(buf, "%d", (atoi(ptr) - x));
    strcpy(ptr, buf);
    return (buf);
}


const char *
mfn_add(MFUNARGS)
{
    int j, i = atoi(argv[0]);
    for (j = 1; j < argc; j++)
        i += atoi(argv[j]);
    sprintf(buf, "%d", i);
    return (buf);
}


const char *
mfn_subt(MFUNARGS)
{
    int j, i = atoi(argv[0]);
    for (j = 1; j < argc; j++)
        i -= atoi(argv[j]);
    sprintf(buf, "%d", i);
    return (buf);
}


const char *
mfn_mult(MFUNARGS)
{
    int j, i = atoi(argv[0]);
    for (j = 1; j < argc; j++)
        i *= atoi(argv[j]);
    sprintf(buf, "%d", i);
    return (buf);
}


const char *
mfn_div(MFUNARGS)
{
    int k, j, i = atoi(argv[0]);
    for (j = 1; j < argc; j++) {
	k = atoi(argv[j]);
	if (!k) {
	    i = 0;
	} else {
	    i /= k;
	}
    }
    sprintf(buf, "%d", i);
    return (buf);
}


const char *
mfn_mod(MFUNARGS)
{
    int k, j, i = atoi(argv[0]);
    for (j = 1; j < argc; j++) {
	k = atoi(argv[j]);
	if (!k) {
	    i = 0;
	} else {
	    i %= k;
	}
    }
    sprintf(buf, "%d", i);
    return (buf);
}



const char *
mfn_abs(MFUNARGS)
{
    int val = atoi(argv[0]);
    if (val == 0) {
        return "0";
    } if (val < 0) {
	val = -val;
    }
    sprintf(buf, "%d", val);
    return (buf);
}


const char *
mfn_sign(MFUNARGS)
{
    int val;
    val = atoi(argv[0]);
    if (val == 0) {
        return "0";
    } else if (val < 0) {
        return "-1";
    } else {
        return "1";
    }
}



const char *
mfn_dist(MFUNARGS)
{
    int a, b, c;
    int a2, b2, c2;
    double result;
    a2 = b2 = c = c2 = 0;
    a = atoi(argv[0]);
    b = atoi(argv[1]);
    if (argc == 3) {
        c = atoi(argv[2]);
    } else if (argc == 4) {
        a2 = atoi(argv[2]);
        b2 = atoi(argv[3]);
    } else if (argc == 6) {
        c = atoi(argv[2]);
        a2 = atoi(argv[3]);
        b2 = atoi(argv[4]);
        c2 = atoi(argv[5]);
    } else if (argc != 2) {
        ABORT_MPI("DIST","Takes 2,3,4, or 6 arguments.");
    }
    a -= a2; b -= b2; c -= c2;
    result = sqrt((double)(a*a) + (double)(b*b) + (double)(c*c));

    sprintf(buf, "%.0f", floor(result + 0.5));
    return buf;
}


const char *
mfn_not(MFUNARGS)
{
    if (truestr(argv[0])) {
        return "0";
    } else {
        return "1";
    }
}


const char *
mfn_or(MFUNARGS)
{
    char *ptr;
    char buf2[16];
    int i;

    for (i = 0; i < argc; i++) {
	ptr = MesgParse(argv[i], buf);
	sprintf(buf2, "arg %d", i+1);
	CHECKRETURN(ptr,"OR",buf2);
	if (truestr(ptr)) {
	    return "1";
	}
    }
    return "0";
}


const char *
mfn_xor(MFUNARGS)
{
    if (truestr(argv[0]) && !truestr(argv[1])) {
        return "1";
    }
    if (!truestr(argv[0]) && truestr(argv[1])) {
        return "1";
    }
    return "0";
}


const char *
mfn_and(MFUNARGS)
{
    char *ptr;
    char buf2[16];
    int i;

    for (i = 0; i < argc; i++) {
	ptr = MesgParse(argv[i], buf);
	sprintf(buf2, "arg %d", i+1);
	CHECKRETURN(ptr,"AND",buf2);
	if (!truestr(ptr)) {
	    return "0";
	}
    }
    return "1";
}


const char *
mfn_dice(MFUNARGS)
{
    int     num = 1;
    int     sides = 1;
    int     offset = 0;
    int     total = 0;

    if (argc >= 3)
        offset = atoi(argv[2]);
    if (argc >= 2)
        num = atoi(argv[1]);
    sides = atoi(argv[0]);
    if (num > 8888)
	ABORT_MPI("DICE","Too many dice!");
    if (sides == 0) return "0";
    while (num-- > 0)
        total += (((RANDOM() / 256) % sides) + 1);
    sprintf(buf, "%d", (total + offset));
    return buf;
}


const char *
mfn_default(MFUNARGS)
{
    char *ptr;

    *buf = '\0';
    ptr = MesgParse(argv[0], buf);
    CHECKRETURN(ptr,"IF","arg 1");
    if (ptr && truestr(buf)) {
        if (!ptr) ptr = "";
    } else {
        ptr = MesgParse(argv[1], buf);
        CHECKRETURN(ptr,"IF","arg 2");
    }
    return ptr;
}


const char *
mfn_if(MFUNARGS)
{
    char   *fbr, *ptr;

    if (argc == 3) {
        fbr = argv[2];
    } else {
        fbr = "";
    }
    ptr = MesgParse(argv[0], buf);
    CHECKRETURN(ptr,"IF","arg 1");
    if (ptr && truestr(buf)) {
        ptr = MesgParse(argv[1], buf);
        CHECKRETURN(ptr,"IF","arg 2");
    } else if (*fbr) {
        ptr = MesgParse(fbr, buf);
        CHECKRETURN(ptr,"IF","arg 3");
    } else {
        *buf = '\0';
        ptr = "";
    }
    return ptr;
}


const char *
mfn_while(MFUNARGS)
{
    int iter_limit = MAX_MFUN_LIST_LEN;
    char buf2[BUFFER_LEN];
    char *ptr;

    *buf = '\0';
    while (1) {
        ptr = MesgParse(argv[0], buf2);
        CHECKRETURN(ptr,"WHILE","arg 1");
        if (!truestr(ptr)) break;
        ptr = MesgParse(argv[1], buf);
        CHECKRETURN(ptr,"WHILE","arg 2");
        if (!(--iter_limit))
            ABORT_MPI("WHILE","Iteration limit exceeded");
    }
    return buf;
}


const char *
mfn_null(MFUNARGS)
{
    return "";
}


const char *
mfn_tzoffset(MFUNARGS)
{
    sprintf(buf, "%ld", get_tz_offset());
    return buf;
}


const char *
mfn_time(MFUNARGS)
{
    time_t  lt;
    struct tm *tm;

    lt = time((time_t *) 0);
    if (argc == 1) {
        lt += (3600 * atoi(argv[0]));
        lt += get_tz_offset();
    }
    tm = localtime(&lt);
    format_time(buf, BUFFER_LEN - 1, "%H:%M:%S\0", tm);
    return buf;
}


const char *
mfn_date(MFUNARGS)
{
    time_t  lt;
    struct tm *tm;

    lt = time((time_t *) 0);
    if (argc == 1) {
        lt += (3600 * atoi(argv[0]));
        lt += get_tz_offset();
    }
    tm = localtime(&lt);
    format_time(buf, BUFFER_LEN - 1, "%D\0", tm);
    return buf;
}


const char *
mfn_ftime(MFUNARGS)
{
    time_t  lt;
    struct tm *tm;

    if (argc == 3) {
        lt = atol(argv[2]);
    } else {
        time(&lt);
    }
    if (argc == 2) {
        lt += (3600 * atoi(argv[1]));
        lt += get_tz_offset();
    }
    tm = localtime(&lt);
    format_time(buf, BUFFER_LEN - 1, argv[0], tm);
    return buf;
}


const char *
mfn_convtime(MFUNARGS)
{
    struct tm otm;
    int mo, dy, yr, hr, mn, sc;
    yr = 70;
    mo = dy = 1;
    hr = mn = sc = 0;
    if(sscanf(argv[0], "%d:%d:%d %d/%d/%d", &hr, &mn, &sc, &mo, &dy, &yr) != 6)
        ABORT_MPI("CONVTIME","Needs HH:MM:SS MO/DY/YR time string format.");
    if (hr < 0 || hr > 23) ABORT_MPI("CONVTIME","Bad Hour");
    if (mn < 0 || mn > 59) ABORT_MPI("CONVTIME","Bad Minute");
    if (sc < 0 || sc > 59) ABORT_MPI("CONVTIME","Bad Second");
    if (yr < 0 || yr > 99) ABORT_MPI("CONVTIME","Bad Year");
    if (mo < 1 || mo > 12) ABORT_MPI("CONVTIME","Bad Month");
    if (dy < 1 || dy > 31) ABORT_MPI("CONVTIME","Bad Day");
    otm.tm_mon = mo - 1;
    otm.tm_mday = dy;
    otm.tm_hour = hr;
    otm.tm_min = mn;
    otm.tm_sec = sc;
    otm.tm_year = (yr >= 70) ? yr : (yr + 100);
#ifdef SUNOS
    sprintf(buf, "%ld", timelocal(&otm));
#else
    sprintf(buf, "%ld", mktime(&otm));
#endif
    return buf;
}


const char *
mfn_ltimestr(MFUNARGS)
{
    int tm = atol(argv[0]);
    int wk, dy, hr, mn;

    wk = dy = hr = mn = 0;
    if (tm >= 86400 * 7) {
        wk = (tm / 86400) / 7; /* Weeks */
        tm %= (86400 * 7);
    }
    if (tm >= 86400) {
        dy = tm / 86400;        /* Days */
        tm %= 86400;
    }
    if (tm >= 3600) {
        hr = tm / 3600;        /* Hours */
        tm %= 3600;
    }
    if (tm >= 60) {
        mn = tm / 60;        /* Minutes */
        tm %= 60;            /* Seconds */
    }

    *buf = '\0';
    if (wk) {
        sprintf(buf, "%d week%s", wk, (wk == 1)? "" : "s");
    }
    if (dy) {
        if (*buf) {
            sprintf(buf, "%s, %d day%s", buf, dy, (dy == 1)? "" : "s");
        } else {
            sprintf(buf, "%d day%s", dy, (dy == 1)? "" : "s");
        }
    }
    if (hr) {
        if (*buf) {
            sprintf(buf, "%s, %d hour%s", buf, hr, (hr == 1)? "" : "s");
        } else {
            sprintf(buf, "%d hour%s", hr, (hr == 1)? "" : "s");
        }
    }
    if (mn) {
        if (*buf) {
            sprintf(buf, "%s, %d min%s", buf, mn, (mn == 1)? "" : "s");
        } else {
            sprintf(buf, "%d min%s", mn, (mn == 1)? "" : "s");
        }
    }
    if (tm || !*buf) {
        if (*buf) {
            sprintf(buf, "%s, %d sec%s", buf, tm, (tm == 1)? "" : "s");
        } else {
            sprintf(buf, "%d sec%s", tm, (tm == 1)? "" : "s");
        }
    }
    return buf;
}


const char *
mfn_timestr(MFUNARGS)
{
    int tm = atol(argv[0]);
    int dy, hr, mn;

    dy = hr = mn = 0;
    if (tm >= 86400) {
        dy = tm / 86400;        /* Days */
        tm %= 86400;
    }
    if (tm >= 3600) {
        hr = tm / 3600;        /* Hours */
        tm %= 3600;
    }
    if (tm >= 60) {
        mn = tm / 60;        /* Minutes */
        tm %= 60;            /* Seconds */
    }

    *buf = '\0';
    if (dy) {
        sprintf(buf, "%dd %02d:%02d", dy, hr, mn);
    } else {
        sprintf(buf, "%02d:%02d", hr, mn);
    }
    return buf;
}


const char *
mfn_stimestr(MFUNARGS)
{
    int tm = atol(argv[0]);
    int dy, hr, mn;

    dy = hr = mn = 0;
    if (tm >= 86400) {
        dy = tm / 86400;        /* Days */
        tm %= 86400;
    }
    if (tm >= 3600) {
        hr = tm / 3600;        /* Hours */
        tm %= 3600;
    }
    if (tm >= 60) {
        mn = tm / 60;        /* Minutes */
        tm %= 60;            /* Seconds */
    }

    *buf = '\0';
    if (dy) {
        sprintf(buf, "%dd", dy);
	return buf;
    }
    if (hr) {
        sprintf(buf, "%dh", hr);
	return buf;
    }
    if (mn) {
        sprintf(buf, "%dm", mn);
	return buf;
    }
    sprintf(buf, "%ds", tm);
    return buf;
}


const char *
mfn_secs(MFUNARGS)
{
    time_t  lt;
    time(&lt);
    sprintf(buf, "%ld", lt);
    return buf;
}


const char *
mfn_convsecs(MFUNARGS)
{
    time_t  lt;
    lt = atol(argv[0]);
    sprintf(buf, "%s", ctime(&lt));
    buf[strlen(buf) - 1] = '\0';
    return buf;
}


const char *
mfn_loc(MFUNARGS)
{
    dbref   obj;
    
    obj = mesg_dbref_local(descr, player, what, perms, argv[0]);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("LOC","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("LOC",tp_noperm_mesg);
    return ref2str(getloc(obj), buf);
}


const char *
mfn_nearby(MFUNARGS)
{
    dbref   obj;
    dbref   obj2;
    
    obj = mesg_dbref_raw(descr, player, what, perms, argv[0]);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING)
        ABORT_MPI("NEARBY","Match failed (arg1).");
    if (obj == PERMDENIED)
        ABORT_MPI("NEARBY","Permission denied (arg1).");
    if (obj == HOME) obj = DBFETCH(player)->sp.player.home;
    if (argc > 1) {
        obj2 = mesg_dbref_raw(descr, player, what, perms, argv[1]);
	if (obj2 == UNKNOWN || obj2 == AMBIGUOUS || obj2 == NOTHING)
            ABORT_MPI("NEARBY","Match failed (arg2).");
        if (obj2 == PERMDENIED)
            ABORT_MPI("NEARBY","Permission denied (arg2).");
	if (obj2 == HOME) obj2 = DBFETCH(player)->sp.player.home;
    } else {
        obj2 = what;
    }
    if (!Wizperms(what) && !isneighbor(obj, what) && !isneighbor(obj2, what) &&
	    !isneighbor(obj, player) && !isneighbor(obj2, player)
    ) {
	ABORT_MPI("NEARBY","Permission denied.  Neither object is local.");
    }
    if (isneighbor(obj, obj2)) {
        return "1";
    } else {
        return "0";
    }
}


const char *
mfn_money(MFUNARGS)
{
    dbref obj = mesg_dbref(descr, player, what, perms, argv[0]);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("MONEY","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("MONEY",tp_noperm_mesg);
    switch (Typeof(obj)) {
        case TYPE_THING:
            sprintf(buf, "%d", DBFETCH(obj)->sp.thing.value);
            break;
        case TYPE_PLAYER:
            sprintf(buf, "%d", DBFETCH(obj)->sp.player.pennies);
            break;
        default:
            strcpy(buf, "0");
            break;
    }
    return buf;
}


const char *
mfn_flags(MFUNARGS)
{
    dbref obj = mesg_dbref_mage(descr, player, what, perms, argv[0]);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("FLAGS","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("FLAGS",tp_noperm_mesg);
    return unparse_flags(obj, buf);
}


const char *
mfn_ansi(MFUNARGS)
{
    char buf2[BUFFER_LEN];
    char *ptr, *ptr2;
    dbref obj = player;
    if (argc > 1)
	obj = mesg_dbref_local(descr, player, what, perms, argv[1]);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("ANSI","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("ANSI",tp_noperm_mesg);
    if ((mesgtyp & MPI_ISLISTENER) && (Typeof(what) != TYPE_ROOM))
        ABORT_MPI("ANSI",tp_noperm_mesg);
    *buf = '\0';
    strcpy(buf2, argv[0]);
    for (ptr = buf2; *ptr; ptr = ptr2) {
	ptr2 = index(ptr, '\r');
	if (ptr2) {
	    *ptr2++ = '\0';
	} else {
	    ptr2 = ptr + strlen(ptr);
	}
	if (Typeof(what)==TYPE_ROOM || OWNER(what) == obj || player == obj ||
		Mageperms(what) ||
		(Typeof(what)==TYPE_EXIT && Typeof(getloc(what))==TYPE_ROOM) ||
		string_prefix(argv[0], NAME(player))) {
	    sprintf(buf, "%.4093s", ptr);
	} else {
	    sprintf(buf, "%s%.16s%s%.4078s",
		    ((obj == OWNER(perms) || obj==player)? "" : "> "),
		    NAME(player),
		    ((*argv[0] == '\'' || isspace(*argv[0]))? "" : " "), ptr);
	}
	anotify_from_echo(player, obj, buf, 1);
    }
    strcpy(buf2, argv[0]);
    unparse_ansi(buf, buf2);
    return buf;
}


const char *
mfn_html(MFUNARGS)
{
    char buf2[BUFFER_LEN];
    char *ptr, *ptr2;
    dbref obj = player;
    if (argc > 1)
	obj = mesg_dbref_local(descr, player, what, perms, argv[1]);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("HTML","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("HTML",tp_noperm_mesg);
    if ((mesgtyp & MPI_ISLISTENER) && (Typeof(what) != TYPE_ROOM))
        ABORT_MPI("HTML",tp_noperm_mesg);
    *buf = '\0';
    strcpy(buf2, argv[0]);
    for (ptr = buf2; *ptr; ptr = ptr2) {
	ptr2 = index(ptr, '\r');
	if (ptr2) {
	    *ptr2++ = '\0';
	} else {
	    ptr2 = ptr + strlen(ptr);
	}
	if (Typeof(what)==TYPE_ROOM || OWNER(what) == obj || player == obj ||
		Mageperms(what) ||
		(Typeof(what)==TYPE_EXIT && Typeof(getloc(what))==TYPE_ROOM) ||
		string_prefix(argv[0], NAME(player))) {
	    sprintf(buf, "%.4093s", ptr);
	} else {
	    sprintf(buf, "%s%.16s%s%.4078s",
		    ((obj == OWNER(perms) || obj==player)? "" : "> "),
		    NAME(player),
		    ((*argv[0] == '\'' || isspace(*argv[0]))? "" : " "), ptr);
	}
	notify_html_from_echo(player, obj, buf, 1);
/*      notify_html_nolisten(player, buf, 1); */
        if (NHtml(OWNER(obj))) notify_html_from_echo(player, obj, "<BR>", 1); /* notify_html_nolisten(player, buf, 1); */
    }
    return argv[0];
}

const char *
mfn_tell(MFUNARGS)
{
    char buf2[BUFFER_LEN];
    char *ptr, *ptr2;
    dbref obj = player;
    if (argc > 1)
	obj = mesg_dbref_local(descr, player, what, perms, argv[1]);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("TELL","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("TELL",tp_noperm_mesg);
    if ((mesgtyp & MPI_ISLISTENER) && (Typeof(what) != TYPE_ROOM))
        ABORT_MPI("TELL",tp_noperm_mesg);
    *buf = '\0';
    strcpy(buf2, argv[0]);
    for (ptr = buf2; *ptr; ptr = ptr2) {
	ptr2 = index(ptr, '\r');
	if (ptr2) {
	    *ptr2++ = '\0';
	} else {
	    ptr2 = ptr + strlen(ptr);
	}
	if (Typeof(what)==TYPE_ROOM || OWNER(what) == obj || player == obj ||
		Mageperms(what) ||
		(Typeof(what)==TYPE_EXIT && Typeof(getloc(what))==TYPE_ROOM) ||
		string_prefix(argv[0], NAME(player))) {
	    sprintf(buf, "%.4093s", ptr);
	} else {
	    sprintf(buf, "%s%.16s%s%.4078s",
		    ((obj == OWNER(perms) || obj==player)? "" : "> "),
		    NAME(player),
		    ((*argv[0] == '\'' || isspace(*argv[0]))? "" : " "), ptr);
	}
	notify_from_echo(player, obj, buf, 1);
    }
    return argv[0];
}

const char *
mfn_telldescr(MFUNARGS)
{
    char buf2[BUFFER_LEN];
    char *ptr, *ptr2;
    dbref obj = descr;
    if (argc > 1) {
      if (!string_compare(argv[1], "descr")) {
         obj = descr;
      } else {
  	   obj = atoi(argv[1]);
      }
    }
    if (!pdescrp(obj))
        ABORT_MPI("TELLDESCR","Invalid descriptor.");
    if (obj == PERMDENIED)
        ABORT_MPI("TELLDESCR",tp_noperm_mesg);
    if ((mesgtyp & MPI_ISLISTENER) && (Typeof(what) != TYPE_ROOM))
        ABORT_MPI("TELLDESCR",tp_noperm_mesg);
    *buf = '\0';
    strcpy(buf2, argv[0]);
    for (ptr = buf2; *ptr; ptr = ptr2) {
	ptr2 = index(ptr, '\r');
	if (ptr2) {
	    *ptr2++ = '\0';
	} else {
	    ptr2 = ptr + strlen(ptr);
	}
	if (Typeof(what)==TYPE_ROOM || OWNER(what) == obj || player == obj ||
		Mageperms(what) ||
		(Typeof(what)==TYPE_EXIT && Typeof(getloc(what))==TYPE_ROOM) ||
		string_prefix(argv[0], NAME(player))) {
	    sprintf(buf, "%.4093s", ptr);
	} else {
	    sprintf(buf, "%s%.16s%s%.4078s",
		    ((obj == OWNER(perms) || obj==player)? "" : "> "),
		    NAME(player),
		    ((*argv[0] == '\'' || isspace(*argv[0]))? "" : " "), ptr);
	}
	notify_descriptor(obj, buf);
    }
    return argv[0];
}
 
const char *
mfn_otell(MFUNARGS)
{
    char buf2[BUFFER_LEN];
    char *ptr, *ptr2;
    dbref obj = getloc(player);
    dbref eobj = player;
    dbref thing;

    if (argc > 1)
	obj = mesg_dbref_local(descr, player, what, perms, argv[1]);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("OTELL","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("OTELL",tp_noperm_mesg);
    if ((mesgtyp & MPI_ISLISTENER) && (Typeof(what) != TYPE_ROOM))
        ABORT_MPI("OTELL",tp_noperm_mesg);
    if (argc > 2)
	eobj = mesg_dbref_raw(descr, player, what, perms, argv[2]);
    strcpy(buf2, argv[0]);
    for (ptr = buf2; *ptr; ptr = ptr2) {
	ptr2 = index(ptr, '\r');
	if (ptr2) {
	    *ptr2++ = '\0';
	} else {
	    ptr2 = ptr + strlen(ptr);
	}
	if (Typeof(what) == TYPE_ROOM || Mageperms(what) ||
		(Typeof(what)==TYPE_EXIT && Typeof(getloc(what))==TYPE_ROOM) ||
		string_prefix(argv[0], NAME(player))) {
	    strcpy(buf, ptr);
	} else {
	    sprintf(buf, "%.16s%s%.4078s", NAME(player),
		    ((*argv[0] == '\'' || isspace(*argv[0]))? "" : " "), ptr);
	}
	thing = DBFETCH(obj)->contents;
	while (thing != NOTHING) {
	    if (thing != eobj)
		notify_from_echo(player, thing, buf, 1);
	    thing = DBFETCH(thing)->next;
	}
    }
    return argv[0];
}


const char *
mfn_oansi(MFUNARGS)
{
    char buf2[BUFFER_LEN];
    char *ptr, *ptr2;
    dbref obj = getloc(player);
    dbref eobj = player;
    dbref thing;

    if (argc > 1)
	obj = mesg_dbref_local(descr, player, what, perms, argv[1]);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("OANSI","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("OANSI",tp_noperm_mesg);
    if ((mesgtyp & MPI_ISLISTENER) && (Typeof(what) != TYPE_ROOM))
        ABORT_MPI("OANSI",tp_noperm_mesg);
    if (argc > 2)
	eobj = mesg_dbref_raw(descr, player, what, perms, argv[2]);
    strcpy(buf2, argv[0]);
    for (ptr = buf2; *ptr; ptr = ptr2) {
	ptr2 = index(ptr, '\r');
	if (ptr2) {
	    *ptr2++ = '\0';
	} else {
	    ptr2 = ptr + strlen(ptr);
	}
	if (Typeof(what) == TYPE_ROOM || Mageperms(what) ||
		(Typeof(what)==TYPE_EXIT && Typeof(getloc(what))==TYPE_ROOM) ||
		string_prefix(argv[0], NAME(player))) {
	    strcpy(buf, ptr);
	} else {
	    sprintf(buf, "%.16s%s%.4078s", NAME(player),
		    ((*argv[0] == '\'' || isspace(*argv[0]))? "" : " "), ptr);
	}
	thing = DBFETCH(obj)->contents;
	while (thing != NOTHING) {
	    if (thing != eobj)
		anotify_from_echo(player, thing, buf, 1);
	    thing = DBFETCH(thing)->next;
	}
    }
    return argv[0];
}


const char *
mfn_ohtml(MFUNARGS)
{
    char buf2[BUFFER_LEN];
    char *ptr, *ptr2;
    dbref obj = getloc(player);
    dbref eobj = player;
    dbref thing;

    if (argc > 1)
	obj = mesg_dbref_local(descr, player, what, perms, argv[1]);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("OHTML","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("OHTML",tp_noperm_mesg);
    if ((mesgtyp & MPI_ISLISTENER) && (Typeof(what) != TYPE_ROOM))
        ABORT_MPI("OHTML",tp_noperm_mesg);
    if (argc > 2)
	eobj = mesg_dbref_raw(descr, player, what, perms, argv[2]);
    strcpy(buf2, argv[0]);
    for (ptr = buf2; *ptr; ptr = ptr2) {
	ptr2 = index(ptr, '\r');
	if (ptr2) {
	    *ptr2++ = '\0';
	} else {
	    ptr2 = ptr + strlen(ptr);
	}
	if (Typeof(what) == TYPE_ROOM || Mageperms(what) ||
		(Typeof(what)==TYPE_EXIT && Typeof(getloc(what))==TYPE_ROOM) ||
		string_prefix(argv[0], NAME(player))) {
	    strcpy(buf, ptr);
	} else {
	    sprintf(buf, "%.16s%s%.4078s", NAME(player),
		    ((*argv[0] == '\'' || isspace(*argv[0]))? "" : " "), ptr);
	}
	thing = DBFETCH(obj)->contents;
	while (thing != NOTHING) {
	    if (thing != eobj)
            {
		notify_html_from_echo(player, thing, buf, 1);
                if (NHtml(OWNER(thing)))
                notify_html_from_echo(player, thing, "<BR>", 1); 
            }
	    thing = DBFETCH(thing)->next;
	}
    }
    return argv[0];
}


const char *
mfn_right(MFUNARGS)
{
    /* {right:string,fieldwidth,padstr} */
    /* Right justify string to a fieldwidth, filling with padstr */

    char *ptr;
    char *fptr;
    int i, len;
    char *fillstr;
    char def_pad[] = " ";

    len = (argc > 1)? atoi(argv[1]) : 78;
    if (len + 1 > BUFFER_LEN)
        ABORT_MPI("RIGHT","Fieldwidth too big.");
    fillstr = (argc > 2)? argv[2] : def_pad;
    if (!*fillstr)
	ABORT_MPI("RIGHT","Null pad string.");
    for (ptr = buf, fptr = fillstr, i = strlen(argv[0]); i < len; i++) {
	*ptr++ = *fptr++;
	if (!*fptr) fptr = fillstr;
    }
    strcpy(ptr, argv[0]);
    return buf;
}


const char *
mfn_created(MFUNARGS)
{
    dbref   obj;

    obj = mesg_dbref(descr, player, what, perms, argv[0]);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("CREATED","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("CREATED",tp_noperm_mesg);

    sprintf(buf, "%ld", DBFETCH(obj)->ts.created);

    return buf;
}


const char *
mfn_lastused(MFUNARGS)
{
    dbref   obj;

    obj = mesg_dbref(descr, player, what, perms, argv[0]);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("LASTUSED","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("LASTUSED",tp_noperm_mesg);

    sprintf(buf, "%ld", DBFETCH(obj)->ts.lastused);

    return buf;
}


const char *
mfn_modified(MFUNARGS)
{
    dbref   obj;

    obj = mesg_dbref(descr, player, what, perms, argv[0]);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("MODIFIED","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("MODIFIED",tp_noperm_mesg);

    sprintf(buf, "%ld", DBFETCH(obj)->ts.modified);

    return buf;
}


const char *
mfn_usecount(MFUNARGS)
{
    dbref   obj;

    obj = mesg_dbref(descr, player, what, perms, argv[0]);
    if (obj == UNKNOWN || obj == AMBIGUOUS || obj == NOTHING || obj == HOME)
        ABORT_MPI("USECOUNT","Match failed.");
    if (obj == PERMDENIED)
        ABORT_MPI("USECOUNT",tp_noperm_mesg);

    sprintf(buf, "%d", DBFETCH(obj)->ts.usecount);

    return buf;
}

const char *
mfn_left(MFUNARGS)
{
    /* {left:string,fieldwidth,padstr} */
    /* Left justify string to a fieldwidth, filling with padstr */

    char *ptr;
    char *fptr;
    int i, len;
    char *fillstr;
    char def_pad[] = " ";

    len = (argc > 1)? atoi(argv[1]) : 78;
    if (len + 1 > BUFFER_LEN)
        ABORT_MPI("RIGHT","Fieldwidth too big.");
    fillstr = (argc > 2)? argv[2] : def_pad;
    if (!*fillstr)
	ABORT_MPI("RIGHT","Null pad string.");
    strcpy(buf, argv[0]);
    for (i = strlen(argv[0]), ptr = &buf[i], fptr = fillstr; i < len; i++) {
	*ptr++ = *fptr++;
	if (!*fptr) fptr = fillstr;
    }
    *ptr = '\0';
    return buf;
}


const char *
mfn_center(MFUNARGS)
{
    /* {center:string,fieldwidth,padstr} */
    /* Center justify string to a fieldwidth, filling with padstr */

    char *ptr;
    char *fptr;
    int i, len, halflen;
    char *fillstr;
    char def_pad[] = " ";

    len = (argc > 1)? atoi(argv[1]) : 78;
    if (len + 1 > BUFFER_LEN)
        ABORT_MPI("RIGHT","Fieldwidth too big.");
    halflen = len / 2;

    fillstr = (argc > 2)? argv[2] : def_pad;
    if (!*fillstr)
	ABORT_MPI("RIGHT","Null pad string.");

    for (ptr = buf, fptr = fillstr, i = strlen(argv[0])/2; i < halflen; i++) {
	*ptr++ = *fptr++;
	if (!*fptr) fptr = fillstr;
    }
    strcpy(ptr, argv[0]);
    for (i = strlen(buf), ptr = &buf[i], fptr = fillstr; i < len; i++) {
	*ptr++ = *fptr++;
	if (!*fptr) fptr = fillstr;
    }
    *ptr = '\0';
    return buf;
}

#endif /* MPI */








