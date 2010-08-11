/* Primitives package */

#include "copyright.h"
#include "config.h"

#include "db.h"
#include "mpi.h"
#include "props.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "strings.h"
#include "interp.h"
#include "msgparse.h"

extern struct inst *oper1, *oper2, *oper3, *oper4, *oper5, *oper6;
extern struct inst temp1, temp2, temp3;
extern int tmp, result;
extern dbref ref;
extern char buf[BUFFER_LEN];

int
prop_read_perms(dbref player, dbref obj, const char *name, int mlev)
{ 
    if (Prop_SysPerms(obj, name))
      return 0;
    if ((mlev < LM3) && Prop_Private(name) && !permissions(mlev, player, obj))
	return 0;
    if ((mlev < (tp_wizhidden_access_bit+4)) && Prop_Hidden(name))
	return 0;
    return 1;
}

int
prop_write_perms(dbref player, dbref obj, const char *name, int mlev)
{
    if (Prop_SysPerms(obj, name))
      return 0;
    if (mlev < LWIZ) {
	if (Prop_SeeOnly(name)) return 0;
	if (!permissions(mlev, player, obj)) {
	    if (Prop_Private(name)) return 0;
	    if (Prop_ReadOnly(name)) return 0;
	    if (!string_compare(name, tp_sex_prop)) return 0;
	}
      if (string_prefix(name, "_msgmacs/")) return 0;
    }
    if (mlev < LWIZ) {
        if (Prop_Hidden(name)) return 0;
    }
    return 1;
}

void
prim_getpropfval(PRIM_PROTOTYPE)
{
	double fresult;

	CHECKOP(2);
	oper1 = POP();
	oper2 = POP();
	if (oper1->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (!oper1->data.string)
		abort_interp("Empty string argument (2)");
        if (oper2->type != PROG_OBJECT)
                abort_interp("Arguement (2) is not a dbref.");
	if ((oper2->data.objref < 0) || (oper2->data.objref >= db_top))
		abort_interp("Non-object argument (1)");
	CHECKREMOTE(oper2->data.objref);
	{
		char *type;

		type = oper1->data.string->data;
		while ((type = index(type, PROPDIR_DELIMITER)))
			if (!(*(++type)))
				abort_interp("Cannot access a propdir directly.");
	}

	if (!prop_read_perms(ProgUID, oper2->data.objref, oper1->data.string->data, mlev))
		abort_interp(tp_noperm_mesg);

	{
		char type[BUFFER_LEN];

		strcpy(type, oper1->data.string->data);
		fresult = get_property_fvalue(oper2->data.objref, type);

#ifdef LOG_PROPS
		log2file("props.log", "#%d (%d) GETPROPFVAL: o=%d n=\"%s\" v=%d",
				 program, pc->line, oper2->data.objref, type, result);
#endif

		/* if (Typeof(oper2->data.objref) != TYPE_PLAYER)
		   ts_lastuseobject(oper2->data.objref); */
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushFloat(fresult);
}

void 
prim_getpropval(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper1->data.string)
	abort_interp("Empty string argument (2)");
    if (oper2->type != PROG_OBJECT)
        abort_interp("Arguement (2) is not a dbref.");
    if ((oper2->data.objref < 0) || (oper2->data.objref >= db_top))
	abort_interp("Non-object argument (1)");
    CHECKREMOTE(oper2->data.objref);
    {
	char   *type;

	type = oper1->data.string->data;
	while ((type = index(type, PROPDIR_DELIMITER)))
	    if (!(*(++type)))
		abort_interp("Cannot access a propdir directly");
    }

    if (!prop_read_perms(ProgUID, oper2->data.objref,
			 oper1->data.string->data, mlev))
	abort_interp(tp_noperm_mesg);

    {
	char   type[BUFFER_LEN];

	strcpy(type, oper1->data.string->data);
	result = get_property_value(oper2->data.objref, type);

#ifdef LOG_PROPS
	log2file("props.log", "#%d (%d) GETPROPVAL: o=%d n=\"%s\" v=%d",
		 program, pc->line, oper2->data.objref, type, result);
#endif

	/* if (Typeof(oper2->data.objref) != TYPE_PLAYER)
	    ts_lastuseobject(oper2->data.objref); */
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}


void 
prim_getprop(PRIM_PROTOTYPE)
{
    PropPtr prptr;
    dbref obj2;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper1->data.string)
	abort_interp("Empty string argument (2)");
    if ((oper2->data.objref < 0) || (oper2->data.objref >= db_top))
	abort_interp("Non-object argument (1)");
    CHECKREMOTE(oper2->data.objref);
    {
	char   type[BUFFER_LEN];
	char  *tmpptr;

	tmpptr = oper1->data.string->data;
	while ((tmpptr = index(tmpptr, PROPDIR_DELIMITER)))
	    if (!(*(++tmpptr)))
		abort_interp("Cannot access a propdir directly");

	if (!prop_read_perms(ProgUID, oper2->data.objref,
			     oper1->data.string->data, mlev))
	    abort_interp(tp_noperm_mesg);

	strcpy(type, oper1->data.string->data);
	obj2 = oper2->data.objref;
	prptr = get_property(obj2, type);

#ifdef LOG_PROPS
	log2file("props.log", "#%d (%d) GETPROP: o=%d n=\"%s\"",
		 program, pc->line, oper2->data.objref, type);
#endif

	CLEAR(oper1);
	CLEAR(oper2);
	if (prptr) {
#ifdef DISKBASE
	    propfetch(obj2, prptr);
#endif
	    switch(PropType(prptr)) {
	      case PROP_STRTYP:
		  PushString(PropDataUNCStr(prptr));
		  break;
	      case PROP_LOKTYP:
		  if (PropFlags(prptr) & PROP_ISUNLOADED) {
		      PushLock(TRUE_BOOLEXP);
		  } else {
		      PushLock(PropDataLok(prptr));
	 	  }
		  break;
	      case PROP_REFTYP:
		  PushObject(PropDataRef(prptr));
		  break;
	      case PROP_INTTYP:
		  PushInt(PropDataVal(prptr));
		  break;
            case PROP_FLTTYP:
              PushFloat(PropDataFVal(prptr));
              break;
	      default:
		  result = 0;
		  PushInt(result);
		break;
	    }
	} else {
	    result = 0;
	    PushInt(result);
	}

	/* if (Typeof(oper2->data.objref) != TYPE_PLAYER)
	    ts_lastuseobject(oper2->data.objref); */
    }
}


void 
prim_getpropstr(PRIM_PROTOTYPE)
{
    const char *temp;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper1->data.string)
	abort_interp("Empty string argument (2)");
    if (oper2->type != PROG_OBJECT)
        abort_interp("Arguement (2) is not a dbref.");
    if ((oper2->data.objref < 0) || (oper2->data.objref >= db_top))
	abort_interp("Non-object argument (1)");
    CHECKREMOTE(oper2->data.objref);
    {
	char   type[BUFFER_LEN];
	char  *tmpptr;
	PropPtr ptr;

	tmpptr = oper1->data.string->data;
	while ((tmpptr = index(tmpptr, PROPDIR_DELIMITER)))
	    if (!(*(++tmpptr)))
		abort_interp("Cannot access a propdir directly");

	if (!prop_read_perms(ProgUID, oper2->data.objref,
			     oper1->data.string->data, mlev))
	    abort_interp(tp_noperm_mesg);

	strcpy(type, oper1->data.string->data);
	ptr = get_property(oper2->data.objref, type);
	if (!ptr) {
	    temp = "";
	} else {
#ifdef DISKBASE
	    propfetch(oper2->data.objref, ptr);
#endif
	    switch(PropType(ptr)) {
		case PROP_STRTYP:
		    temp = PropDataUNCStr(ptr);
		    break;
		case PROP_INTTYP:
		     sprintf(buf, "%d", PropDataVal(ptr));
		     temp = buf;
		     break;
		case PROP_FLTTYP:
		     sprintf(buf, "%#.15g", PropDataFVal(ptr));
            temp = buf;
		     break;
		case PROP_REFTYP:
		    sprintf(buf, "#%d", PropDataRef(ptr));
		    temp = buf;
		    break;
		case PROP_LOKTYP:
		    if (PropFlags(ptr) & PROP_ISUNLOADED) {
		        temp = "*UNLOCKED*";
		    } else {
			temp = unparse_boolexp(ProgUID, PropDataLok(ptr), 1);
		    }
		    break;
		default:
		    temp = "";
		    break;
	    }
	}

#ifdef LOG_PROPS
	log2file("props.log", "#%d (%d) GETPROPSTR: o=%d n=\"%s\" s=\"%s\"",
		 program, pc->line, oper2->data.objref, type, temp);
#endif

	/* if (Typeof(oper2->data.objref) != TYPE_PLAYER)
	 *     ts_lastuseobject(oper2->data.objref); */
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushString(temp);
}


void 
prim_remove_prop(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!valid_object(oper2))
	abort_interp("Non-object argument (1)");
    if (tp_db_readonly)
        abort_interp(DBRO_MESG);

    CHECKREMOTE(oper2->data.objref);
    strncpy(buf, DoNullInd(oper1->data.string), BUFFER_LEN);
    buf[BUFFER_LEN - 1] = '\0';
    {
        int len = strlen(buf);
        char *ptr = buf + len;
     
        while((--len >= 0) && (*--ptr == PROPDIR_DELIMITER))
            *ptr = '\0'; /* remove trailing / marks. */
    }
    if (!*buf)
        abort_interp("Can't remove root propdir. (2)");

    if (!prop_write_perms(ProgUID, oper2->data.objref, buf, mlev))
	abort_interp(tp_noperm_mesg);

    remove_property(oper2->data.objref, buf);

#ifdef LOG_PROPS
	log2file("props.log", "#%d (%d) REMOVEPROP: o=%d n=\"%s\"",
		 program, pc->line, oper1->data.objref, buf);
#endif

	ts_modifyobject(program, oper2->data.objref);
    
    CLEAR(oper1);
    CLEAR(oper2);
}


void 
prim_envprop(PRIM_PROTOTYPE)
{
    double fresult;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper1->data.string)
	abort_interp("Empty string argument (2)");
    if (oper2->type != PROG_OBJECT)
        abort_interp("Arguement (2) is not a dbref.");
    if ((oper2->data.objref < 0) || (oper2->data.objref >= db_top))
	abort_interp("Non-object argument (1)");
    CHECKREMOTE(oper2->data.objref);
    {
	char   *type;
	char    tname[BUFFER_LEN];
	dbref   what;
	PropPtr ptr;

	type = oper1->data.string->data;
	while ((type = index(type, PROPDIR_DELIMITER)))
	    if (!(*(++type)))
		abort_interp("Cannot access a propdir directly");
	strcpy(tname, oper1->data.string->data);
	what = oper2->data.objref;
	ptr = envprop(&what, tname, 0);
	if (what != NOTHING) {
	    if (!prop_read_perms(ProgUID,what,oper1->data.string->data,mlev))
		abort_interp(tp_noperm_mesg);
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushObject(what);

	if (!ptr) {
	    result = 0;
	    PushInt(result);
	} else {
#ifdef DISKBASE
	    propfetch(what, ptr);
#endif
	    switch(PropType(ptr)) {
		case PROP_STRTYP:
		    PushString(PropDataUNCStr(ptr));
		    break;
		case PROP_INTTYP:
		    result = PropDataVal(ptr);
		    PushInt(result);
		    break;
		case PROP_FLTTYP:
		    fresult = PropDataFVal(ptr);
                    PushFloat(fresult);
		    break;
		case PROP_REFTYP:
		    ref = PropDataRef(ptr);
		    PushObject(ref);
		    break;
		case PROP_LOKTYP:
		    if (PropFlags(ptr) & PROP_ISUNLOADED) {
		        PushLock(TRUE_BOOLEXP);
		    } else {
			PushLock(PropDataLok(ptr));
		    }
		    break;
		default:
		    result = 0;
		    PushInt(result);
		    break;
	    }
	}
    }
}


void 
prim_envpropstr(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (oper1->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper1->data.string)
	abort_interp("Empty string argument (2)");
    if (oper2->type != PROG_OBJECT)
        abort_interp("Arguement (2) is not a dbref.");
    if ((oper2->data.objref < 0) || (oper2->data.objref >= db_top))
	abort_interp("Non-object argument (1)");
    CHECKREMOTE(oper2->data.objref);
    {
	char   *type;
	char    tname[BUFFER_LEN];
	dbref   what;
	PropPtr ptr;
	const char *temp;

	type = oper1->data.string->data;
	while ((type = index(type, PROPDIR_DELIMITER)))
	    if (!(*(++type)))
		abort_interp("Cannot access a propdir directly");
	strcpy(tname, oper1->data.string->data);
	what = oper2->data.objref;
	ptr = envprop(&what, tname, 0);
	if (!ptr) {
	    temp = "";
	} else {
#ifdef DISKBASE
	    propfetch(what, ptr);
#endif
	    switch(PropType(ptr)) {
		case PROP_STRTYP:
		    temp = PropDataUNCStr(ptr);
		    break;
		/*
		 *case PROP_INTTYP:
		 *    sprintf(buf, "%d", PropDataVal(ptr));
		 *    temp = buf;
		 *    break;
		 */
		case PROP_REFTYP:
		    sprintf(buf, "#%d", PropDataRef(ptr));
		    temp = buf;
		    break;
		case PROP_LOKTYP:
		    if (PropFlags(ptr) & PROP_ISUNLOADED) {
		        temp = "*UNLOCKED*";
		    } else {
			temp = unparse_boolexp(ProgUID, PropDataLok(ptr), 1);
		    }
		    break;
		default:
		    temp = "";
		    break;
	    }
	}

#ifdef LOG_PROPS
	log2file("props.log", "#%d (%d) ENVPROPSTR: o=%d so=%d n=\"%s\" s=\"%s\"",
		 program, pc->line, what, oper2->data.objref, tname, temp);
#endif

	if (what != NOTHING) {
	    if (!prop_read_perms(ProgUID, what, oper1->data.string->data,mlev))
		abort_interp(tp_noperm_mesg);
	    /* if (Typeof(what) != TYPE_PLAYER)
	     *     ts_lastuseobject(what); */
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushObject(what);
	PushString(temp);
    }
}



void 
prim_setprop(PRIM_PROTOTYPE)
{
    PData pdat;

    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();
    if ((oper1->type != PROG_STRING) &&
	    (oper1->type != PROG_INTEGER) &&
	    (oper1->type != PROG_LOCK) &&
	    (oper1->type != PROG_OBJECT) &&
          (oper1->type != PROG_FLOAT))
	abort_interp("Invalid argument type (3)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper2->data.string)
	abort_interp("Empty string argument (2)");
    if (!valid_object(oper3))
	abort_interp("Non-object argument (1)");
    if (tp_db_readonly)
        abort_interp(DBRO_MESG);
    CHECKREMOTE(oper3->data.objref);

    if ((mlev < LM2) && (!permissions(mlev, ProgUID, oper3->data.objref)))
	abort_interp(tp_noperm_mesg);

    if (!prop_write_perms(ProgUID, oper3->data.objref,
			 oper2->data.string->data, mlev))
	abort_interp(tp_noperm_mesg);

    {
	char   *tmpe;
	char    tname[BUFFER_LEN];

	tmpe = oper2->data.string->data;
	while (*tmpe && *tmpe != '\r' && *tmpe != ':' && *tmpe != '\n') tmpe++;
	if (*tmpe)
	    abort_interp("Illegal propname");

	tmpe = oper2->data.string->data;
	while ((tmpe = index(tmpe, PROPDIR_DELIMITER)))
	    if (!(*(++tmpe)))
		abort_interp("Cannot access a propdir directly");

	strcpy(tname, oper2->data.string->data);

    switch (oper1->type) {
        case PROG_STRING:
            pdat.flags = PROP_STRTYP;
            pdat.data.str = oper1->data.string ? oper1->data.string->data : NULL;
            break;
        case PROG_INTEGER:
            pdat.flags = PROP_INTTYP;
            pdat.data.val = oper1->data.number;
            break;
        case PROG_FLOAT:
            pdat.flags = PROP_FLTTYP;
            pdat.data.fval = oper1->data.fnumber;
            break;
        case PROG_OBJECT:
            pdat.flags = PROP_REFTYP;
            pdat.data.ref = oper1->data.objref;
            break;
        case PROG_LOCK:
            pdat.flags = PROP_LOKTYP;
            pdat.data.lok = copy_bool(oper1->data.lock);
            break;
    }
    set_property(oper3->data.objref, tname, &pdat);

#ifdef LOG_PROPS
	log2file("props.log", "#%d (%d) SETPROP: o=%d n=\"%s\"",
		 program, pc->line, oper3->data.objref, tname);
#endif

	ts_modifyobject(program, oper3->data.objref);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
}


void 
prim_addprop(PRIM_PROTOTYPE)
{
    CHECKOP(4);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();
    oper4 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Non-integer argument (4)");
    if (oper2->type != PROG_STRING)
	abort_interp("Non-string argument (3)");
    if (oper3->type != PROG_STRING)
	abort_interp("Non-string argument (2)");
    if (!oper3->data.string)
	abort_interp("Empty string argument (2)");
    if (!valid_object(oper4))
	abort_interp("Non-object argument (1)");
    CHECKREMOTE(oper4->data.objref);

    if ((mlev < LM2) && (!permissions(mlev, ProgUID, oper4->data.objref)))
	abort_interp(tp_noperm_mesg);

    if (!prop_write_perms(ProgUID, oper4->data.objref,
			 oper3->data.string->data, mlev))
	abort_interp(tp_noperm_mesg);

    {
	const char *temp;
	char   *tmpe;
	char    tname[BUFFER_LEN];

	temp = (oper2->data.string ? oper2->data.string->data : 0);
	tmpe = oper3->data.string->data;
	while (*tmpe && *tmpe != '\r') tmpe++;
	if (*tmpe)
	    abort_interp("CRs not allowed in propname");

	tmpe = oper3->data.string->data;

	while ((tmpe = index(tmpe, PROPDIR_DELIMITER)))
	    if (!(*(++tmpe)))
		abort_interp("Cannot access a propdir directly");

	strcpy(tname, oper3->data.string->data);

	/* if ((temp) || (oper1->data.number)) */
	{
	    add_property(oper4->data.objref, tname, temp, oper1->data.number);

#ifdef LOG_PROPS
	    log2file("props.log", "#%d (%d) ADDPROP: o=%d n=\"%s\" s=\"%s\" v=%d",
		     program, pc->line, oper4->data.objref, tname, temp,
		     oper1->data.number);
#endif

	    ts_modifyobject(program, oper4->data.objref);
	}
    }
    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    CLEAR(oper4);
}



void 
prim_nextprop(PRIM_PROTOTYPE)
{
    /* dbref pname -- pname */
    char   *pname;
    char    exbuf[BUFFER_LEN];

    CHECKOP(2);
    oper2 = POP();		/* pname */
    oper1 = POP();		/* dbref */
    if (mlev < LM2)
	abort_interp("M2 prim");
    if (oper2->type != PROG_STRING)
	abort_interp("String required (2)");
    if (oper1->type != PROG_OBJECT)
	abort_interp("Dbref required (1)");
    if ((oper1->data.objref < 0) || (oper1->data.objref >= db_top))
	abort_interp("Invalid dbref (1)");

    ref = oper1->data.objref;
    (void) strcpy(buf, ((oper2->data.string) && (oper2->data.string->data)) ?
		  oper2->data.string->data : "");
    CLEAR(oper1);
    CLEAR(oper2);

    {
	char   *tmpname;

	pname = next_prop_name(ref, exbuf, buf);

#ifdef LOG_PROPS
	log2file("props.log", "#%d (%d) NEXTPROP: o=%d n=\"%s\" on=\"%s\"",
		 program, pc->line, ref, pname, buf);
#endif

	if (mlev < LARCH) {
	    while (pname && !prop_read_perms(ProgUID, ref, pname, mlev)) {
		tmpname = next_prop_name(ref, exbuf, pname);

#ifdef LOG_PROPS
		log2file("props.log", "#%d (%d) NEXTPROP: o=%d n=\"%s\" on=\"%s\"",
			 program, pc->line, ref, tmpname, pname);
#endif

		pname = tmpname;
	    }
	}
    }
    if (pname) {
	PushString(pname);
    } else {
	PushNullStr;
    }
}


void 
prim_propdirp(PRIM_PROTOTYPE)
{
    /* dbref dir -- int */
    CHECKOP(2);
    oper2 = POP();		/* prop name */
    oper1 = POP();		/* dbref */
    if (mlev < LM2)
	abort_interp("M2 prim");
    if (oper1->type != PROG_OBJECT)
	abort_interp("Argument must be a dbref (1)");
    if ((oper1->data.objref < 0) || (oper1->data.objref >= db_top))
	abort_interp("Invalid dbref (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("Argument not a string (2)");
    if (!oper2->data.string)
	abort_interp("Null string not allowed (2)");
    ref = oper1->data.objref;
    (void) strcpy(buf, oper2->data.string->data ? oper2->data.string->data : "");
    CLEAR(oper1);
    CLEAR(oper2);

    result = is_propdir(ref, buf);

#ifdef LOG_PROPS
    log2file("props.log", "#%d (%d) PROPDIR?: o=%d n=\"%s\" v=%d",
	     program, pc->line, ref, buf, result);
#endif

    PushInt(result);
}

void 
prim_parsempi(PRIM_PROTOTYPE)
{
    const char *temp;
    char *ptr;
    struct inst *oper1 = NULL, *oper2 = NULL, *oper3 = NULL, *oper4 = NULL; 
    char buf[BUFFER_LEN];

    CHECKOP(4);
    oper4 = POP();  /* int */
    oper2 = POP();  /* arg str */
    oper1 = POP();  /* mpi str */
    oper3 = POP();  /* object dbref */
    if (mlev < LWIZ)
        abort_interp("Wiz prim");
    if (oper3->type != PROG_OBJECT)
        abort_interp("Non-object argument (1)");
    if ((oper3->data.objref < 0) || (oper3->data.objref >= db_top))
        abort_interp("Invalid object (1)");
    if (oper2->type != PROG_STRING)
        abort_interp("String expected (3)");
    if (oper1->type != PROG_STRING)
        abort_interp("String expected (2)");
    /* if (!oper1->data.string)
        abort_interp("Empty string argument (2)"); */
    if (oper4->type != PROG_INTEGER)
        abort_interp("Integer expected (4)");
    if (oper4->data.number < 0 || oper4->data.number > 1)
        abort_interp("Integer of 0 or 1 expected (4)");
    CHECKREMOTE(oper3->data.objref);

    temp = (oper1->data.string)? oper1->data.string->data : (const char *) "";
    ptr  = (char *) ((oper2->data.string)? (char *) oper2->data.string->data : "");
    if(temp && *temp && ptr) {
	result = oper4->data.number & (~MPI_ISLISTENER);
        ptr = do_parse_mesg(fr->descr, player, oper3->data.objref, temp,
			      ptr, buf, result);
	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
	CLEAR(oper4);
        PushString(buf); /* Was buf to remove unwanted \r's */
    } else {
	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
	CLEAR(oper4);
        PushNullStr;
    }
}

void 
prim_parseprop(PRIM_PROTOTYPE)
{
    const char *temp;
    char *ptr;
    struct inst *oper1 = NULL, *oper2 = NULL, *oper3 = NULL, *oper4 = NULL; 
    char buf[BUFFER_LEN];

    CHECKOP(4);
    oper4 = POP();  /* int */
    oper2 = POP();  /* arg str */
    oper1 = POP();  /* propname str */
    oper3 = POP();  /* object dbref */
    if (mlev < 3)
        abort_interp("Mucker level 3 or greater required.");
    if (oper3->type != PROG_OBJECT)
        abort_interp("Non-object argument. (1)");
    if ((oper3->data.objref < 0) || (oper3->data.objref >= db_top))
        abort_interp("Invalid object. (1)");
    if (oper2->type != PROG_STRING)
        abort_interp("String expected. (3)");
    if (oper1->type != PROG_STRING)
        abort_interp("String expected. (2)");
    if (!oper1->data.string)
        abort_interp("Empty string argument (2)");
    if (oper4->type != PROG_INTEGER)
        abort_interp("Integer expected. (4)");
    if (oper4->data.number < 0 || oper4->data.number > 1)
        abort_interp("Integer of 0 or 1 expected. (4)");
    CHECKREMOTE(oper3->data.objref);
    {
        char   type[BUFFER_LEN];
        char  *tmpptr;

        tmpptr = oper1->data.string->data;
        while ((tmpptr = index(tmpptr, PROPDIR_DELIMITER)))
            if (!(*(++tmpptr)))
                abort_interp("Cannot access a propdir directly.");

	if (!prop_read_perms(ProgUID, oper3->data.objref,
			     oper1->data.string->data, mlev))
	    abort_interp(tp_noperm_mesg);

	if (mlev < 3 && !permissions(mlev, PSafe, oper3->data.objref) &&
		prop_write_perms(ProgUID, oper3->data.objref,
				 oper1->data.string->data, mlev))
	    abort_interp(tp_noperm_mesg);

        strcpy(type, oper1->data.string->data);
        temp = get_property_class(oper3->data.objref, type);

#ifdef LOG_PROPS
        log2file("props.log", "#%d (%d) GETPROPSTR: o=%d n=\"%s\" s=\"%s\"",
                 program, pc->line, oper3->data.objref, type, temp);
#endif

    }
    ptr = (char *) ((oper2->data.string)? (char *) oper2->data.string->data : "");
    if(temp) {
	result = oper4->data.number & (~MPI_ISLISTENER);
      if(tp_old_parseprop) {
        ptr = do_parse_mesg_2(fr->descr, player, oper3->data.objref, (dbref)program, temp,
			    ptr, buf, result);
      } else {
        ptr = do_parse_mesg(fr->descr, player, oper3->data.objref, temp,
			    ptr, buf, result);
      }
	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
	CLEAR(oper4);
        PushString(ptr);
    } else {
	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
	CLEAR(oper4);
        PushNullStr;
    }
}


void
prim_propqueue(PRIM_PROTOTYPE)
{
   struct inst *oper1 = NULL, *oper2 = NULL, *oper3 = NULL, *oper4 = NULL;
   if (mlev < LARCH)
      abort_interp("Archwizards or above only.");
   CHECKOP(3);
   oper1 = POP();
   oper2 = POP();
   oper3 = POP();
   oper4 = POP();
   if (oper1->type != PROG_STRING)
      abort_interp("String expected. (4)");
   if (!oper1->data.string)
      abort_interp("Empty string argument (4)");
   if (oper2->type != PROG_OBJECT)
      abort_interp("Non-object argument. (3)");
   if ((oper2->data.objref < 0) || (oper2->data.objref >= db_top))
      abort_interp("Invalid object. (3)");
   if (oper3->type != PROG_STRING)
      abort_interp("String expected. (2)");
   if (!oper3->data.string)
      abort_interp("Empty string argument (2)");
   if (oper4->type != PROG_OBJECT)
      abort_interp("Non-object argument. (1)");
   if (!valid_object(oper4))
      abort_interp("Invalid object. (1)");
   
   propqueue(fr->descr, player, OkObj(player) ? getloc(player) : -1, oper4->data.objref, 
             oper2->data.objref, NOTHING, oper3->data.string->data,
             oper1->data.string->data, 1, 1);

   CLEAR(oper1);
   CLEAR(oper2);
   CLEAR(oper3);
   CLEAR(oper4);
}

void
prim_envpropqueue(PRIM_PROTOTYPE)
{
   struct inst *oper1 = NULL, *oper2 = NULL, *oper3 = NULL, *oper4 = NULL;
   if (mlev < LARCH)
      abort_interp("Archwizards or above only.");
   CHECKOP(3);
   oper1 = POP();
   oper2 = POP();
   oper3 = POP();
   oper4 = POP();
   if (oper1->type != PROG_STRING)
      abort_interp("String expected. (4)");
   if (!oper1->data.string)
      abort_interp("Empty string argument (4)");
   if (oper2->type != PROG_OBJECT)
      abort_interp("Non-object argument. (3)");
   if ((oper2->data.objref < 0) || (oper2->data.objref >= db_top))
      abort_interp("Invalid object. (3)");
   if (oper3->type != PROG_STRING)
      abort_interp("String expected. (2)");
   if (!oper3->data.string)
      abort_interp("Empty string argument (2)");
   if (oper4->type != PROG_OBJECT)
      abort_interp("Non-object argument. (1)");
   if (!valid_object(oper4))
      abort_interp("Invalid object. (1)");
   
   envpropqueue(fr->descr, player, OkObj(player) ? getloc(player) : -1, oper4->data.objref, 
             oper2->data.objref, NOTHING, oper3->data.string->data,
             oper1->data.string->data, 1, 1);

   CLEAR(oper1);
   CLEAR(oper2);
   CLEAR(oper3);
   CLEAR(oper4);
}

void
prim_testlock(PRIM_PROTOTYPE)
{
    /* d d - i */
    CHECKOP(2);
    oper1 = POP();              /* boolexp lock */
    oper2 = POP();              /* player dbref */
    if (fr->level > 8)
        abort_interp("Interp call loops not allowed");
    if (!valid_object(oper2))
        abort_interp("Invalid argument (1).");
    if (Typeof(oper2->data.objref) != TYPE_PLAYER &&
        Typeof(oper2->data.objref) != TYPE_THING )
    {
        abort_interp("Invalid object type (1).");
    }
    CHECKREMOTE(oper2->data.objref);
    if (oper1->type != PROG_LOCK)
        abort_interp("Invalid argument (2)");

    interp_set_depth(fr);
    result = eval_boolexp(fr->descr, oper2->data.objref, oper1->data.lock, 
                          PSafe);
    fr->level--;
    interp_set_depth(fr);

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void 
prim_lockedp(PRIM_PROTOTYPE)
{   
    dbref ref2;
    /* d d - i */
    CHECKOP(2);
    oper1 = POP();              /* objdbref */
    oper2 = POP();              /* player dbref */
    if (fr->level > 8)
        abort_interp("Interp call loops not allowed");
    if (!valid_object(oper2))
        abort_interp("invalid object (1)");
    if (!valid_player(oper2) && Typeof(oper2->data.objref) != TYPE_THING)
        abort_interp("Non-player argument (1)");
    CHECKREMOTE(oper2->data.objref);
    if (!valid_object(oper1))
        abort_interp("invalid object (2)");
    CHECKREMOTE(oper1->data.objref);

    ref = oper2->data.objref;
    ref2 = oper1->data.objref;

    CLEAR(oper1);
    CLEAR(oper2);
    nargs -= 2;

    interp_set_depth(fr);
    result = !could_doit(fr->descr, ref, ref2);
    fr->level--; 
    interp_set_depth(fr);
    PushInt(result);
}

void
prim_islockedp(PRIM_PROTOTYPE)
{
    /* d d s - i */

    char  *tmpptr;
    dbref ref2;

    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();

    if (fr->level > 8)
        abort_interp("Interp call loops not allowed");

    if ((oper3->data.objref < 0) || (oper3->data.objref >= db_top))
        abort_interp("Invalid argument (1).");
    if (Typeof(oper3->data.objref) != TYPE_PLAYER &&
          Typeof(oper3->data.objref) != TYPE_THING )
        abort_interp("Invalid object type (1).");

    if ((oper2->data.objref < 0) || (oper2->data.objref >= db_top))
        abort_interp("Invalid argument (2).");

    if (oper1->type != PROG_STRING)
        abort_interp("Invalid argument. (3)");

     CHECKREMOTE(oper2->data.objref);
     CHECKREMOTE(oper3->data.objref);

     strcpy(buf, oper1->data.string->data);
     ref = oper3->data.objref;
     ref2 = oper2->data.objref;

     CLEAR(oper1);
     CLEAR(oper2);
     CLEAR(oper3);
     nargs -= 3;

     tmpptr = buf;
     while ((tmpptr = index(tmpptr, PROPDIR_DELIMITER)))
         if (!(*(++tmpptr)))
             abort_interp("Cannot access a propdir directly");

     interp_set_depth(fr);
     result = !(could_doit2(fr->descr, ref, ref2,  buf, 0));
     fr->level--;
     interp_set_depth(fr);

     PushInt(result);
}

void
prim_checklock(PRIM_PROTOTYPE)
{
    /* d d s - i */
    /* This is just a copy and paste of the islocked? code. The only
     * difference is that it will try MUF called from the lock. */
    char  *tmpptr;
    dbref ref2;

    CHECKOP(3);

    oper1 = POP();
    oper2 = POP();
    oper3 = POP();

    if (fr->level > 8)
        abort_interp("Interp call loops not allowed");

    if ((oper3->data.objref < 0) || (oper3->data.objref >= db_top))
        abort_interp("Invalid argument (1).");
    if (Typeof(oper3->data.objref) != TYPE_PLAYER &&
          Typeof(oper3->data.objref) != TYPE_THING )
        abort_interp("Invalid object type (1).");

    if ((oper2->data.objref < 0) || (oper2->data.objref >= db_top))
        abort_interp("Invalid argument (2).");

    if (oper1->type != PROG_STRING)
        abort_interp("Invalid argument. (3)");

     CHECKREMOTE(oper2->data.objref);
     CHECKREMOTE(oper3->data.objref);

     strcpy(buf, oper1->data.string->data);
     ref = oper3->data.objref;
     ref2 = oper2->data.objref;

     CLEAR(oper1);
     CLEAR(oper2);
     CLEAR(oper3);
     nargs -= 3;

     tmpptr = buf;
     while ((tmpptr = index(tmpptr, PROPDIR_DELIMITER)))
         if (!(*(++tmpptr)))
             abort_interp("Cannot access a propdir directly");

     interp_set_depth(fr);
     result = !(could_doit2(fr->descr, ref, ref2, buf, 1));
     fr->level--;
     interp_set_depth(fr);

     PushInt(result);
}

void
prim_array_filter_prop(PRIM_PROTOTYPE)
{
    char buf[BUFFER_LEN], buf2[BUFFER_LEN];
    struct inst *in;
    struct inst temp1;
    stk_array *arr;
    stk_array *nu;
    char* pat;
    char* prop;
    const char* ptr;
    CHECKOP(3);
    oper3 = POP();    /* str     pattern */
    oper2 = POP();    /* str     propname */
    oper1 = POP();    /* refarr  Array */
    if (oper1->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");
    if (!array_is_homogenous(oper1->data.array, PROG_OBJECT))
        abort_interp("Argument not an array of dbrefs. (1)");
    if (oper2->type != PROG_STRING || !oper2->data.string)
        abort_interp("Argument not a non-null string. (2)");
    if (oper3->type != PROG_STRING)
        abort_interp("Argument not a string pattern. (3)");
    ptr = oper2->data.string->data;
    while ((ptr = index(ptr, PROPDIR_DELIMITER)))
        if (!(*(++ptr)))
            abort_interp("Cannot access a propdir directly.");
    nu = new_array_packed(0);
    arr = oper1->data.array;
    prop = (char *) DoNullInd(oper2->data.string);
    pat = (char *) DoNullInd(oper3->data.string);
    strcpy(buf2, pat);
    if (array_first(arr, &temp1)) {
        do {
            in = array_getitem(arr, &temp1);
            if (valid_object(in)) {
                ref = in->data.objref;
                CHECKREMOTE(ref);
                if (prop_read_perms(ProgUID, ref, prop, mlev)) {
                    ptr = get_property_class(ref, prop);
                    if (ptr)
                        strcpy(buf, ptr);
                    else
                        strcpy(buf, "");

                    if (equalstr(buf2, buf)) {
                        array_appenditem(&nu, in);
                    }
                }
            }
        } while (array_next(arr, &temp1));
    }
    CLEAR(oper3);
    CLEAR(oper2);
    CLEAR(oper1);
    PushArrayRaw(nu);
}

void
prim_reflist_find(PRIM_PROTOTYPE)
{
	CHECKOP(3);
	oper3 = POP();   /* dbref */
	oper2 = POP();   /* propname */
	oper1 = POP();   /* propobj */
	if (!valid_object(oper1))
		abort_interp("Non-object argument (1)");
	if (oper2->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (!oper2->data.string)
		abort_interp("Empty string argument (2)");
	if (oper3->type != PROG_OBJECT)
		abort_interp("Non-object argument (3)");
	if (!prop_read_perms(ProgUID, oper1->data.objref, oper2->data.string->data, mlev))
		abort_interp("Permission denied.");
	CHECKREMOTE(oper1->data.objref);

	result = reflist_find(oper1->data.objref, oper2->data.string->data, oper3->data.objref);

	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
	PushInt(result);
}

void
prim_reflist_add(PRIM_PROTOTYPE)
{
	CHECKOP(3);
	oper3 = POP();   /* dbref */
	oper2 = POP();   /* propname */
	oper1 = POP();   /* propobj */
	if (!valid_object(oper1))
		abort_interp("Non-object argument (1)");
	if (oper2->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (!oper2->data.string)
		abort_interp("Empty string argument (2)");
	if (oper3->type != PROG_OBJECT)
		abort_interp("Non-object argument (3)");
	if (!prop_write_perms(ProgUID, oper1->data.objref, oper2->data.string->data, mlev))
		abort_interp("Permission denied.");
	CHECKREMOTE(oper1->data.objref);

	reflist_add(oper1->data.objref, oper2->data.string->data, oper3->data.objref);

	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
}

void
prim_reflist_del(PRIM_PROTOTYPE)
{
	CHECKOP(3);
	oper3 = POP();   /* dbref */
	oper2 = POP();   /* propname */
	oper1 = POP();   /* propobj */
	if (!valid_object(oper1))
		abort_interp("Non-object argument (1)");
	if (oper2->type != PROG_STRING)
		abort_interp("Non-string argument (2)");
	if (!oper2->data.string)
		abort_interp("Empty string argument (2)");
	if (oper3->type != PROG_OBJECT)
		abort_interp("Non-object argument (3)");
	if (!prop_write_perms(ProgUID, oper1->data.objref, oper2->data.string->data, mlev))
		abort_interp("Permission denied.");
	CHECKREMOTE(oper1->data.objref);

	reflist_del(oper1->data.objref, oper2->data.string->data, oper3->data.objref);

	CLEAR(oper1);
	CLEAR(oper2);
	CLEAR(oper3);
}

void
prim_parsepropex(PRIM_PROTOTYPE)
{
    stk_array *vars;
    const char *mpi;
    char *str = 0;
    array_iter idx;
    extern int varc; /* from msgparse.c */
    char *buffers = NULL;
    char buf[BUFFER_LEN];
    int mvarcnt = 0, novars, hashow = 0, i;

    CHECKOP(4);
    /* ref:Object str:Prop dict:Vars int:Private */
    oper4 = POP(); /* int:Private */
    oper3 = POP(); /* dict:Vars */
    oper2 = POP(); /* str:Prop */
    oper1 = POP(); /* ref:Object */

    if (mlev < LMAGE)
        abort_interp("W1 or greater required.");

    if (oper1->type != PROG_OBJECT)
        abort_interp("Non-object argument. (1)");
    if (oper2->type != PROG_STRING)
        abort_interp("Non-string argument. (2)");
    if (oper3->type != PROG_ARRAY)
        abort_interp("Non-array argument. (3)");
    if (oper3->data.array && (oper3->data.array->type != ARRAY_DICTIONARY))
        abort_interp("Dictionary array expected. (3)");
    if (oper4->type != PROG_INTEGER)
        abort_interp("Non-integer argument. (4)");

    if (!valid_object(oper1))
        abort_interp("Invalid object.(1)");
    if (!oper2->data.string)
        abort_interp("Empty string argument. (2)");
    if (oper4->data.number != 0 && oper4->data.number != 1)
        abort_interp("Integer of 1 or 0 expected. (4)");

    CHECKREMOTE(oper1->data.objref);

    if (has_suffix_char(oper2->data.string->data, PROPDIR_DELIMITER))
        abort_interp("Cannot access a propdir directly.");

    if (!prop_read_perms(ProgUID, oper1->data.objref, oper2->data.string->data,
                          mlev))
        abort_interp("Permission denied.");

    mpi = get_property_class(oper1->data.objref, oper2->data.string->data);
    vars = oper3->data.array;
    novars = array_count(vars);

    if (check_mvar_overflow(novars))
        abort_interp("Out of MPI variables. (3)");

    if (array_first(vars, &idx)) {
        do {
            array_data *val = array_getitem(vars, &idx);
            if (idx.type != PROG_STRING) {
                CLEAR(&idx);
                abort_interp("Only string keys supported. (3)");
            }
            if (idx.data.string == NULL) {
                CLEAR(&idx);
                abort_interp("Empty string keys not supported. (3)");
            }
            if (strlen(idx.data.string->data) > MAX_MFUN_NAME_LEN) {
                CLEAR(&idx);
                abort_interp("Key too long to be an MPI variable. (3)");
            }

            switch(val->type) {
                case PROG_INTEGER:
                case PROG_FLOAT:
                case PROG_OBJECT:
                case PROG_STRING:
                case PROG_LOCK:
                    break;
                default:
                    CLEAR(&idx);
                    abort_interp("Values must be int, float, dbref, string, or lock. (3)");
                    break;
            }

            if (string_compare(idx.data.string->data, "how") == 0)
                hashow = 1;
        } while (array_next(vars, &idx));
    }
    if (mpi && *mpi) {
        if (novars > 0 ) {
            mvarcnt = varc;
            if(!(buffers = (char *)malloc(novars * BUFFER_LEN)))
                abort_interp("Out of memory.");

            if (array_first(vars, &idx)) {
                i = 0;
                do {
                    char *var_buf = buffers + (i++ *BUFFER_LEN);
                    array_data *val;
                    
                    val = array_getitem(vars, &idx);
                    switch(val->type) {
                        case PROG_INTEGER:
                            snprintf(var_buf, BUFFER_LEN, "%i", 
                                      val->data.number);
                            break;
                        case PROG_FLOAT:
                            snprintf(var_buf, BUFFER_LEN, "%f", 
                                      val->data.fnumber);
                            break;
                        case PROG_OBJECT:
                            snprintf(var_buf, BUFFER_LEN, "#%i", 
                                      val->data.objref);
                            break;
                        case PROG_STRING:
                            strncpy(var_buf, DoNullInd(val->data.string),
                                      BUFFER_LEN);
                            break;
                        case PROG_LOCK:
                            strncpy(var_buf, unparse_boolexp(ProgUID, 
                                      val->data.lock, 1), BUFFER_LEN);
                            break;
                        default:
                            var_buf[0] = '\0';
                            break;
                    }
                    var_buf[BUFFER_LEN - 1] = '\0';
                    new_mvar(idx.data.string->data, var_buf);
                } while (array_next(vars, &idx));
            }
        }

        result = 0;

        if (oper4->data.number)
            result |= MPI_ISPRIVATE;
        if (hashow)
            result |= MPI_NOHOW;

        str = do_parse_mesg(fr->descr, player, oper1->data.objref, mpi,
                            "(parsepropex)", buf, result);
       
        if (novars > 0) {
            if (array_first(vars, &idx)) {
                i = 0;
                do {
                    char *var_buf = buffers + (i++ * BUFFER_LEN);
                    struct inst temp;
                    
                    temp.type = PROG_STRING;
                    temp.data.string = alloc_prog_string(var_buf);
                    array_setitem(&vars, &idx, &temp);
                    CLEAR(&temp);
                } while(array_next(vars, &idx));
            }
            free(buffers);
            varc = mvarcnt;
        }
    }

    oper3->data.array = NULL;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    CLEAR(oper4);

    PushArrayRaw(vars);
    PushString(str);
}

/* copyprops():
 * Copy a property (sub)tree from one object to another. This is used by
 * the COPYPROPS primitive. Based muchly on FB6's copy_props function used
 * by @CLONE, with slight modifications to work with ProtoMUCK.  -Hinoserm
 */
int 
copy_props(dbref source, dbref destination, const char *sourcedir, const char *destdir, int recurse) 
{ 
    char propname[BUFFER_LEN]; 
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    PropPtr propadr, pptr, currprop;
    PData pdat;

    int propcount = 0;

#ifdef DISKBASE
    fetchprops(destination);
#endif

    /* loop through all properties in the current propdir */ 
    propadr = first_prop(source, (char *)sourcedir, &pptr, propname); 
    while (propadr) { 
        /* generate name for current property */
        snprintf(buf, sizeof(buf), "%s%c%s", sourcedir, PROPDIR_DELIMITER, propname);
        snprintf(buf2, sizeof(buf2), "%s%c%s", destdir, PROPDIR_DELIMITER, propname);

        /* read property from source object */
        currprop = get_property(source, buf); 
#ifdef DISKBASE 
        propfetch(source, currprop); 
#endif
        if (currprop) {
    		pdat.flags = currprop->flags;

            switch(PropType(currprop)) {
                case PROP_LOKTYP:
                    pdat.data.lok = copy_bool((currprop->data).lok);
                case PROP_DIRTYP:
                    break;
                default:
                    pdat.data = currprop->data;
                    break;
            }
            set_property_nofetch(destination, buf2, &pdat, 1);
            propcount++;
       
            /* This has to go here, or it'll miss propdirs with values */
            if (recurse) propcount += copy_props(source, destination, buf, buf2, recurse);
        }

        /* find next property in current dir */ 
        propadr = next_prop(pptr, propadr, propname);
    } 

    return propcount; 
} 

void
prim_copyprops(PRIM_PROTOTYPE)
{
    CHECKOP(5);
    oper1 = POP(); /* Recurse          (INT) */
    oper2 = POP(); /* Dest Prop(dir)   (STR) */
    oper3 = POP(); /* Dest Object      (REF) */
    oper4 = POP(); /* Source Prop(dir) (STR) */
    oper5 = POP(); /* Source Object    (REF) */ 

    if (mlev < LARCH)
        abort_interp("Arch prim");

    if (oper1->type != PROG_INTEGER)
        abort_interp("Non-integer argument (5)");

    if (oper2->type != PROG_STRING)
        abort_interp("Non-string argument (4)");
    if (!oper2->data.string)
        abort_interp("Empty string argument (4)");

    if (!(valid_object(oper3)))
        abort_interp("Invalid object argument (3)");

    if (oper4->type != PROG_STRING)
        abort_interp("Non-string argument (2)");
    if (!oper4->data.string)
        abort_interp("Empty string argument (2)");

    if (!(valid_object(oper5)))
        abort_interp("Invalid object argument (1)");

    result = copy_props(oper5->data.objref, oper3->data.objref, oper4->data.string->data, oper2->data.string->data, oper1->data.number); 

    CLEAR(oper1); CLEAR(oper2);
    CLEAR(oper3); CLEAR(oper4);
    CLEAR(oper5);

    CHECKOFLOW(1);
    PushInt(result);
}

