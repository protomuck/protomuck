/* Primitives package */

#include "copyright.h"
#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "db.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "strings.h"
#include "interp.h"

extern struct inst *oper1, *oper2, *oper3, *oper4;
extern struct inst temp1, temp2, temp3;
extern int tmp, result;
extern dbref ref;


void 
prim_awakep(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (!valid_object(oper1))
	abort_interp("invalid argument");
    ref = oper1->data.objref;
    if (Typeof(ref) == TYPE_THING && (FLAGS(ref) & ZOMBIE))
	ref = OWNER(ref);
    if (Typeof(ref) != TYPE_PLAYER)
	abort_interp("invalid argument");
    result = online(ref);
    PushInt(result);
}

void 
prim_online(PRIM_PROTOTYPE)
{
    CHECKOP(0);
    if (mlev < LM3)
	abort_interp("M3 prim");
    result = pcount();
    CHECKOFLOW(result + 1);
    while (result) {
	ref = pdbref(result--);
	PushObject(ref);
    }
    result = pcount();
    PushInt(result);
}


void
prim_online_array(PRIM_PROTOTYPE)
{
	stk_array *new;
	int i;

	CHECKOP(0);
	if (mlev < 3)
		abort_interp("Mucker level 3 primitive.");
	result = pcount();
	CHECKOFLOW(1);

	temp1.type = PROG_INTEGER;
	temp2.type = PROG_OBJECT;
	temp1.line = 0;
	temp2.line = 0;
	new = new_array_packed(result);
	for (i = 0; i < result; i++) {
		temp1.data.number = i;
		temp2.data.number = pdbref(i + 1);
		array_setitem(&new, &temp1, &temp2);
	}
	PushArrayRaw(new);
}


void 
prim_concount(PRIM_PROTOTYPE)
{
    /* -- int */
    CHECKOP(0);
    result = pcount();
    CHECKOFLOW(1);
    PushInt(result);
}

void 
prim_condbref(PRIM_PROTOTYPE)
{
    /* int -- dbref */
    CHECKOP(1);
    oper1 = POP();
    if (mlev < LM3)
	abort_interp("M3 prim");
    if (oper1->type != PROG_INTEGER)
	abort_interp("Argument not an integer (1)");
    result = oper1->data.number;
    if ((result < 1) || (result > pcount()))
	abort_interp("Invalid connection number (1)");
    result = pdbref(result);
    CHECKOFLOW(1);
    CLEAR(oper1);
    PushObject(result);
}


void 
prim_conidle(PRIM_PROTOTYPE)
{
    /* int -- int */
    CHECKOP(1);
    oper1 = POP();
    if (mlev < LM2)
	abort_interp("M2 prim");
    if (oper1->type != PROG_INTEGER)
	abort_interp("Argument not an integer (1)");
    result = oper1->data.number;
    if ((result < 1) || (result > pcount()))
	abort_interp("Invalid connection number (1)");
    result = pidle(result);
    CHECKOFLOW(1);
    CLEAR(oper1);
    PushInt(result);
}


void 
prim_contime(PRIM_PROTOTYPE)
{
    /* int -- int */
    CHECKOP(1);
    oper1 = POP();
    if (mlev < LM2)
	abort_interp("M2 prim");
    if (oper1->type != PROG_INTEGER)
	abort_interp("Argument not an integer (1)");
    result = oper1->data.number;
    if ((result < 1) || (result > pcount()))
	abort_interp("Invalid connection number (1)");
    result = pontime(result);
    CHECKOFLOW(1);
    CLEAR(oper1);
    PushInt(result);
}


void 
prim_conhost(PRIM_PROTOTYPE)
{
    /* int -- char * */
    char   *pname;

    CHECKOP(1);
    oper1 = POP();
    if (mlev < LMAGE)
	abort_interp("Mage prim");
    if (oper1->type != PROG_INTEGER)
	abort_interp("Argument not an integer (1)");
    result = oper1->data.number;
    if ((result < 1) || (result > pcount()))
	abort_interp("Invalid connection number (1)");
    pname = phost(result);
    CHECKOFLOW(1);
    CLEAR(oper1);
    PushString(pname);
}

void 
prim_conuser(PRIM_PROTOTYPE)
{
    /* int -- char * */
    char   *pname;

    CHECKOP(1);
    oper1 = POP();
    if (mlev < LARCH)
       abort_interp("ArchWizard prim");
    if (oper1->type != PROG_INTEGER)
       abort_interp("Argument not an integer (1)");
    result = oper1->data.number;
    if ((result < 1) || (result > pcount()))
       abort_interp("Invalid connection number (1)");
    pname = puser(result);
    CHECKOFLOW(1);
    CLEAR(oper1);
    PushString(pname);
}

void 
prim_conipnum(PRIM_PROTOTYPE)
{
    /* int -- char * */
    char   *pname;

    CHECKOP(1);
    oper1 = POP();
    if (mlev < LMAGE)
       abort_interp("Mage prim");
    if (oper1->type != PROG_INTEGER)
       abort_interp("Argument not an integer (1)");
    result = oper1->data.number;
    if ((result < 1) || (result > pcount()))
       abort_interp("Invalid connection number (1)");
    pname = pipnum(result);
    CHECKOFLOW(1);
    CLEAR(oper1);
    PushString(pname);
}

void 
prim_conport(PRIM_PROTOTYPE)
{
    /* int -- char * */
    char   *pname;

    CHECKOP(1);
    oper1 = POP();
    if (mlev < LARCH)
       abort_interp("Arch prim");
    if (oper1->type != PROG_INTEGER)
       abort_interp("Argument not an integer (1)");
    result = oper1->data.number;
    if ((result < 1) || (result > pcount()))
       abort_interp("Invalid connection number (1)");
    pname = pport(result);
    CHECKOFLOW(1);
    CLEAR(oper1);
    PushString(pname);
}

void
prim_conboot(PRIM_PROTOTYPE)
{
    /* int --  */
    CHECKOP(1);
    if (mlev < LWIZ)
	abort_interp("Wizard prim");
    oper1 = POP();
    if (oper1->type != PROG_INTEGER)
	abort_interp("Argument not an integer (1)");
    result = oper1->data.number;
    if ((result < 1) || (result > pcount()))
	abort_interp("Invalid connection number (1)");
    CLEAR(oper1);
    pboot(result);
}


void 
prim_connotify(PRIM_PROTOTYPE)
{
    /* int string --  */
    CHECKOP(2);
    oper2 = POP();		/* string */
    oper1 = POP();		/* int */
    if (mlev < LM3)
	abort_interp("M3 prim");
    if (oper1->type != PROG_INTEGER)
	abort_interp("Argument not an integer (1)");
    if (oper2->type != PROG_STRING)
	abort_interp("Argument not an string (2)");
    result = oper1->data.number;
    if ((result < 1) || (result > pcount()))
	abort_interp("Invalid connection number (1)");
    if (oper2->data.string)
	pnotify(result, oper2->data.string->data);
    CLEAR(oper1);
    CLEAR(oper2);
}


void 
prim_condescr(PRIM_PROTOTYPE)
{
    /* int -- int */
    CHECKOP(1);
    oper1 = POP();
    if (mlev < LM3)
	abort_interp("M3 prim");
    if (oper1->type != PROG_INTEGER)
	abort_interp("Argument not an integer (1)");
    result = oper1->data.number;
    if ((result < 1) || (result > pcount()))
	abort_interp("Invalid connection number (1)");
    result = pdescr(result);
    CLEAR(oper1);
    PushInt(result);
}


void 
prim_descrcon(PRIM_PROTOTYPE)
{
    /* int -- int */
    CHECKOP(1);
    oper1 = POP();
    if (mlev < LM3)
	abort_interp("M3 prim");
    if (oper1->type != PROG_INTEGER)
	abort_interp("Argument not an integer (1)");
    result = oper1->data.number;
    result = pdescrcon(result);
    CLEAR(oper1);
    PushInt(result);
}

void 
prim_nextdescr(PRIM_PROTOTYPE)
{
    /* int -- int */
    CHECKOP(1);
    oper1 = POP();
    if (mlev < LMAGE)
	abort_interp("Mage prim");
    if (oper1->type != PROG_INTEGER)
	abort_interp("Argument not an integer (1)");
    result = oper1->data.number;
    result = pnextdescr(result);
    CLEAR(oper1);
    PushInt(result);
}


void 
prim_descriptors(PRIM_PROTOTYPE)
{
    int     mydescr, mycount = 0;

    CHECKOP(1);
    oper1 = POP();
    if (mlev < LM3)
	abort_interp("M3");
    if (oper1->type != PROG_OBJECT)
	abort_interp("Argument not a dbref");
    if (oper1->data.objref != NOTHING && !valid_object(oper1))
	abort_interp("Bad dbref");
    ref = oper1->data.objref;
    if ((ref != NOTHING) && (!valid_player(oper1)))
	abort_interp("Non-player argument");
    CLEAR(oper1);
    CHECKOP(0);
    for (result = pcount(); result; result--) {
	if ((ref == NOTHING) || (pdbref(result) == ref)) {
	    CHECKOFLOW(1);
	    mydescr = pdescr(result);
	    PushInt(mydescr);
	    mycount++;
	}
    }
    CHECKOFLOW(1);
    PushInt(mycount);
}

void
prim_descr_array(PRIM_PROTOTYPE)
{
	stk_array *newarr;
	int i;

	CHECKOP(1);
	oper1 = POP();
	if (mlev < 3)
		abort_interp("Mucker level 3 primitive.");
	if (oper1->type != PROG_OBJECT)
		abort_interp("Argument not a dbref.");
	if (oper1->data.objref != NOTHING && !valid_object(oper1))
		abort_interp("Bad dbref.");
	ref = oper1->data.objref;
	if ((ref != NOTHING) && (!valid_player(oper1)))
		abort_interp("Non-player argument.");

	CLEAR(oper1);
	CHECKOFLOW(1);

	result = pcount();
	temp1.type = PROG_INTEGER;
	temp2.type = PROG_INTEGER;
	temp1.line = 0;
	temp2.line = 0;
	newarr = new_array_packed(result);
	for (i = 0; i < result; i++) {
		if (ref == NOTHING || pdbref(i + 1) == ref) {
			temp1.data.number = i;
			temp2.data.number = pdescr(i + 1);
			array_setitem(&newarr, &temp1, &temp2);
		}
	}
	PushArrayRaw(newarr);
}

void
prim_descr_setuser(PRIM_PROTOTYPE)
{
    char *ptr;

    CHECKOP(2);
    oper3 = POP();
    oper2 = POP();
    oper1 = POP();

    if (mlev < LWIZ)
	abort_interp("Wizard prim");
    if (oper1->type != PROG_INTEGER)
	abort_interp("Integer descriptor number expected (1)");
    if (oper2->type != PROG_OBJECT)
	abort_interp("Player dbref expected (2)");
    ref = oper2->data.objref;
    if (ref != NOTHING && !valid_player(oper2))
	abort_interp("Player dbref expected (2)");
    if (oper3->type != PROG_STRING)
	abort_interp("Password string expected");
    ptr = oper3->data.string? oper3->data.string->data : "";
    if (ref != NOTHING && strcmp(ptr, DBFETCH(ref)->sp.player.password))
	abort_interp("Incorrect password");

    if (ref != NOTHING) {
	log_status("SUSR: %d %s(%d) to %s(%d)\n",
		oper1->data.number, NAME(player), player, NAME(ref), ref);
    }
    tmp = oper1->data.number;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    result = pset_user2(tmp, ref);

    PushInt(result);
}

void
prim_descr(PRIM_PROTOTYPE)
{
	/* -- int */
	CHECKOP(0);
	result = fr->descr;
	CHECKOFLOW(1);
	PushInt(result);
}

void
prim_descrflush(PRIM_PROTOTYPE)
{
	CHECKOP(1);
	oper1 = POP();

	if (mlev < 3)
		abort_interp("Requires Mucker Level 3 or better.");
	if (oper1->type != PROG_INTEGER)
		abort_interp("Integer descriptor number expected.");
	tmp = oper1->data.number;
	CLEAR(oper1);
	result = pdescrflush(tmp);
}

void
prim_descr_htmlp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
       abort_interp("Requires Mucker Level 3 or better.");
    if (oper1->type != PROG_INTEGER)
       abort_interp("Integer descriptor number expected.");

    if (!pdescrp(oper1->data.number))
       abort_interp("That is not a valid descriptor.");

    result = ( (pdescrtype(oper1->data.number) == CT_PUEBLO) || (pdescrtype(oper1->data.number) == CT_HTML) );

    CLEAR(oper1);
    PushInt(result);
}

void
prim_descr_pueblop(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
       abort_interp("Requires Mucker Level 3 or better.");
    if (oper1->type != PROG_INTEGER)
       abort_interp("Integer descriptor number expected.");

    if (!pdescrp(oper1->data.number))
       abort_interp("That is not a valid descriptor.");

    result = ( pdescrtype(oper1->data.number) == CT_PUEBLO );

    CLEAR(oper1);
    PushInt(result);
}
 
void
prim_welcome_user(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
       abort_interp("Requires Mucker Level 3 or better.");
    if (oper1->type != PROG_INTEGER)
       abort_interp("Integer descriptor number expected.");

    if (!pdescrp(oper1->data.number))
       abort_interp("That is not a valid descriptor.");

    pdescr_welcome_user(oper1->data.number);
    CLEAR(oper1);
}

void
prim_descrp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
       abort_interp("Requires Mucker Level 3 or better.");
    if (oper1->type != PROG_INTEGER)
       abort_interp("Integer descriptor number expected.");

    result = pdescrp(oper1->data.number);

    CLEAR(oper1);
    PushInt(result);
}




