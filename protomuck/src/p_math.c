/* Primitives package */

#include "copyright.h"
#include "config.h"

#include <math.h>
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
static double tl;
static float fresult, tf1, tf2;

#define MAXINT ~(1<<(sizeof(int)*8)-1)
#define MININT (1<<(sizeof(int)*8)-1)

int
arith_good(double test)
{
	return ((test <= (double) (MAXINT)) && (test >= (double) (MININT)));
}

#define INF (9.9E999)
#define NINF (-9.9E999)

int
nogood(float test)
{
	return (((test == INF) || (test == NINF)));
}

int 
arith_type(struct inst * op1, struct inst * op2)
{
    return ((op1->type == PROG_INTEGER && op2->type == PROG_INTEGER)	/* real stuff */
	    ||(op1->type == PROG_OBJECT && op2->type == PROG_INTEGER)	/* inc. dbref */
	    ||(op1->type == PROG_VAR && op2->type == PROG_INTEGER)	      /* offset array */
	    ||(op1->type == PROG_LVAR && op2->type == PROG_INTEGER)
	    || (op1->type == PROG_FLOAT && op2->type == PROG_FLOAT)
	    || (op1->type == PROG_FLOAT && op2->type == PROG_INTEGER)
 	    || (op1->type == PROG_INTEGER && op2->type == PROG_FLOAT));
}


void 
prim_add(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!arith_type(oper2, oper1))
	abort_interp("Invalid argument type");
	if ((oper1->type == PROG_FLOAT) || (oper2->type == PROG_FLOAT)) {
		tf1 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber : (float) oper1->data.number;
		tf2 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber : (float) oper2->data.number;
		if (!nogood(tf1) && !nogood(tf2)) {
			fresult = tf1 + tf2;
		} else {
			fresult = 0.0;
			fr->error.error_flags.f_bounds = 1;
		}
	} else {
		result = oper1->data.number + oper2->data.number;
		tl = (double) oper1->data.number + (double) oper2->data.number;
		if (!arith_good(tl))
			fr->error.error_flags.i_bounds = 1;
	}
	tmp = (oper2->type == PROG_FLOAT || oper1->type == PROG_FLOAT) ? PROG_FLOAT : oper2->type;
	CLEAR(oper1);
	CLEAR(oper2);
	if (tmp == PROG_FLOAT)
		push(arg, top, tmp, MIPSCAST & fresult);
	else
		push(arg, top, tmp, MIPSCAST & result);
}


void 
prim_subtract(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
	if (!arith_type(oper2, oper1))
		abort_interp("Invalid argument type.");
	if ((oper1->type == PROG_FLOAT) || (oper2->type == PROG_FLOAT)) {
		tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber : (float) oper2->data.number;
		tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber : (float) oper1->data.number;
		if (!nogood(tf1) && !nogood(tf2)) {
			fresult = tf1 - tf2;
		} else {
			fresult = 0.0;
			fr->error.error_flags.f_bounds = 1;
		}
	} else {
		result = oper2->data.number - oper1->data.number;
		tl = (double) oper2->data.number - (double) oper1->data.number;
		if (!arith_good(tl))
			fr->error.error_flags.i_bounds = 1;
	}
	tmp = (oper2->type == PROG_FLOAT || oper1->type == PROG_FLOAT) ? PROG_FLOAT : oper2->type;
	CLEAR(oper1);
	CLEAR(oper2);
	if (tmp == PROG_FLOAT)
		push(arg, top, tmp, MIPSCAST & fresult);
	else
		push(arg, top, tmp, MIPSCAST & result);
}


void 
prim_multiply(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
	if (!arith_type(oper2, oper1))
		abort_interp("Invalid argument type.");
	if ((oper1->type == PROG_FLOAT) || (oper2->type == PROG_FLOAT)) {
		tf1 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber : (float) oper1->data.number;
		tf2 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber : (float) oper2->data.number;
		if (!nogood(tf1) && !nogood(tf2)) {
			fresult = tf1 * tf2;
		} else {
			fresult = 0.0;
			fr->error.error_flags.f_bounds = 1;
		}
	} else {
		result = oper1->data.number * oper2->data.number;
		tl = (double) oper1->data.number * (double) oper2->data.number;
		if (!arith_good(tl))
			fr->error.error_flags.i_bounds = 1;
	}
	tmp = (oper2->type == PROG_FLOAT || oper1->type == PROG_FLOAT) ? PROG_FLOAT : oper2->type;
	CLEAR(oper1);
	CLEAR(oper2);
	if (tmp == PROG_FLOAT)
		push(arg, top, tmp, MIPSCAST & fresult);
	else
		push(arg, top, tmp, MIPSCAST & result);
}


void 
prim_divide(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!arith_type(oper2, oper1))
	abort_interp("Invalid argument type");
	if ((oper1->type == PROG_FLOAT) || (oper2->type == PROG_FLOAT)) {
		if ((oper1->type == PROG_INTEGER && !oper1->data.number) ||
			(oper1->type == PROG_FLOAT && oper1->data.fnumber < SMALL_NUM
			 && oper1->data.fnumber > NSMALL_NUM)) {
			fresult = 0.0;
			fr->error.error_flags.div_zero = 1;
		} else {
			tf1 =
					(oper2->type ==
				   PROG_FLOAT) ? oper2->data.fnumber : (float) oper2->data.number;
			tf2 =
					(oper1->type ==
				   PROG_FLOAT) ? oper1->data.fnumber : (float) oper1->data.number;
			if (!nogood(tf1) && !nogood(tf2)) {
				fresult = tf1 / tf2;
			} else {
				fresult = 0.0;
				fr->error.error_flags.f_bounds = 1;
			}
		}
	} else {
		if (oper1->data.number) {
			result = oper2->data.number / oper1->data.number;
		} else {
			result = 0;
			fr->error.error_flags.div_zero = 1;
		}
	}
	tmp = (oper2->type == PROG_FLOAT || oper1->type == PROG_FLOAT) ? PROG_FLOAT : oper2->type;
	CLEAR(oper1);
	CLEAR(oper2);
	if (tmp == PROG_FLOAT)
		push(arg, top, tmp, MIPSCAST & fresult);
	else
		push(arg, top, tmp, MIPSCAST & result);
}


void 
prim_mod(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
	if ((!arith_type(oper2, oper1)) || (oper1->type == PROG_FLOAT) ||
		(oper2->type == PROG_FLOAT)) abort_interp("Invalid argument type.");
	if (oper1->data.number)
		result = oper2->data.number % oper1->data.number;
	else
		result = 0;
	tmp = oper2->type;
	CLEAR(oper1);
	CLEAR(oper2);
	push(arg, top, tmp, MIPSCAST & result);
}

void 
prim_bitor(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!arith_type(oper2, oper1))
	abort_interp("Invalid argument type");
    result = oper2->data.number | oper1->data.number;
    tmp = oper2->type;
    CLEAR(oper1);
    CLEAR(oper2);
    push(arg, top, tmp, MIPSCAST & result);
}


void 
prim_bitxor(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!arith_type(oper2, oper1))
	abort_interp("Invalid argument type");
    result = oper2->data.number ^ oper1->data.number;
    tmp = oper2->type;
    CLEAR(oper1);
    CLEAR(oper2);
    push(arg, top, tmp, MIPSCAST & result);
}


void 
prim_bitand(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!arith_type(oper2, oper1))
	abort_interp("Invalid argument type");
    result = oper2->data.number & oper1->data.number;
    tmp = oper2->type;
    CLEAR(oper1);
    CLEAR(oper2);
    push(arg, top, tmp, MIPSCAST & result);
}


void 
prim_bitshift(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (!arith_type(oper2, oper1))
	abort_interp("Invalid argument type");
    if (oper1->data.number > 0)
	result = oper2->data.number << oper1->data.number;
    else if (oper1->data.number < 0)
	result = oper2->data.number >> (-(oper1->data.number));
    else
	result = oper2->data.number;
    tmp = oper2->type;
    CLEAR(oper1);
    CLEAR(oper2);
    push(arg, top, tmp, MIPSCAST & result);
}


void 
prim_and(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    result = !false(oper1) && !false(oper2);
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void 
prim_or(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    result = !false(oper1) || !false(oper2);
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void 
prim_not(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    result = false(oper1);
    CLEAR(oper1);
    PushInt(result);
}

int
comp_t(struct inst *op)
{
	return (op->type == PROG_INTEGER || op->type == PROG_FLOAT || op->type == PROG_OBJECT);
}

void 
prim_lessthan(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
	if (!comp_t(oper1) || !comp_t(oper2))
		abort_interp("Invalid argument type.");
	if (oper1->type == PROG_FLOAT || oper2->type == PROG_FLOAT) {
		tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber :
				(oper2->type == PROG_INTEGER) ? (float) oper2->data.number :
				(float) oper2->data.objref;
		tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber :
				(oper1->type == PROG_INTEGER) ? (float) oper1->data.number :
				(float) oper1->data.objref;
		result = tf1 < tf2;
	} else {
		result = (((oper2->type == PROG_INTEGER) ? oper2->data.number : oper2->data.objref)
				  < ((oper1->type == PROG_INTEGER) ? oper1->data.number : oper1->data.objref));
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushInt(result);
}


void 
prim_greathan(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
	if (!comp_t(oper1) || !comp_t(oper2))
		abort_interp("Invalid argument type.");
	if (oper1->type == PROG_FLOAT || oper2->type == PROG_FLOAT) {
		tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber :
				(oper2->type == PROG_INTEGER) ? (float) oper2->data.number :
				(float) oper2->data.objref;
		tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber :
				(oper1->type == PROG_INTEGER) ? (float) oper1->data.number :
				(float) oper1->data.objref;
		result = tf1 > tf2;
	} else {
		result = (((oper2->type == PROG_INTEGER) ? oper2->data.number : oper2->data.objref)
				  > ((oper1->type == PROG_INTEGER) ? oper1->data.number : oper1->data.objref));
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushInt(result);
}


void 
prim_equal(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
	if (!comp_t(oper1) || !comp_t(oper2))
		abort_interp("Invalid argument type.");
	if (oper1->type == PROG_FLOAT || oper2->type == PROG_FLOAT) {
		tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber :
				(oper2->type == PROG_INTEGER) ? (float) oper2->data.number :
				(float) oper2->data.objref;
		tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber :
				(oper1->type == PROG_INTEGER) ? (float) oper1->data.number :
				(float) oper1->data.objref;
		result = tf1 == tf2;
	} else {
		result = (((oper2->type == PROG_INTEGER) ? oper2->data.number : oper2->data.objref)
				  ==
				  ((oper1->type == PROG_INTEGER) ? oper1->data.number : oper1->data.objref));
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushInt(result);
}


void 
prim_lesseq(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
	if (!comp_t(oper1) || !comp_t(oper2))
		abort_interp("Invalid argument type.");
	if (oper1->type == PROG_FLOAT || oper2->type == PROG_FLOAT) {
		tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber :
				(oper2->type == PROG_INTEGER) ? (float) oper2->data.number :
				(float) oper2->data.objref;
		tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber :
				(oper1->type == PROG_INTEGER) ? (float) oper1->data.number :
				(float) oper1->data.objref;
		result = tf1 <= tf2;
	} else {
		result = (((oper2->type == PROG_INTEGER) ? oper2->data.number : oper2->data.objref)
				  <=
				  ((oper1->type == PROG_INTEGER) ? oper1->data.number : oper1->data.objref));
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushInt(result);
}


void 
prim_greateq(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
	if (!comp_t(oper1) || !comp_t(oper2))
		abort_interp("Invalid argument type.");
	if (oper1->type == PROG_FLOAT || oper2->type == PROG_FLOAT) {
		tf1 = (oper2->type == PROG_FLOAT) ? oper2->data.fnumber :
				(oper2->type == PROG_INTEGER) ? (float) oper2->data.number :
				(float) oper2->data.objref;
		tf2 = (oper1->type == PROG_FLOAT) ? oper1->data.fnumber :
				(oper1->type == PROG_INTEGER) ? (float) oper1->data.number :
				(float) oper1->data.objref;
		result = tf1 >= tf2;
	} else {
		result = (((oper2->type == PROG_INTEGER) ? oper2->data.number : oper2->data.objref)
				  >=
				  ((oper1->type == PROG_INTEGER) ? oper1->data.number : oper1->data.objref));
	}
	CLEAR(oper1);
	CLEAR(oper2);
	PushInt(result);
}

void 
prim_random(PRIM_PROTOTYPE)
{
    result = RANDOM();
    CHECKOFLOW(1);
    PushInt(result);
}

void
prim_srand(PRIM_PROTOTYPE)
{
	CHECKOP(0);
	CHECKOFLOW(1);
	if (!(fr->rndbuf)) {
		fr->rndbuf = init_seed(NULL);
	}
	result = (int) rnd(fr->rndbuf);
	PushInt(result);
}

void
prim_getseed(PRIM_PROTOTYPE)
{
	char buf[33];
	char buf2[17];
	int loop;

	CHECKOP(0);
	CHECKOFLOW(1);
	if (!(fr->rndbuf)) {
		PushNullStr;
	} else {
		memcpy(buf2, fr->rndbuf, 16);
		buf2[16] = '\0';
		for (loop = 0; loop < 16; loop++) {
			buf[loop * 2] = (buf2[loop] & 0x0F) + 65;
			buf[(loop * 2) + 1] = ((buf2[loop] & 0xF0) >> 4) + 65;
		}
		buf[32] = '\0';
		PushString(buf);
	}
}

void
prim_setseed(PRIM_PROTOTYPE)
{
	int slen, sloop;
	char holdbuf[33];
	char buf[17];

	CHECKOP(1);
	oper1 = POP();
	if (!(oper1->type == PROG_STRING))
		abort_interp("Invalid argument type.");
	if (fr->rndbuf) {
		delete_seed(fr->rndbuf);
		fr->rndbuf = NULL;
	}
	if (!oper1->data.string) {
		fr->rndbuf = init_seed(NULL);
		CLEAR(oper1);
		return;
	} else {
		slen = strlen(oper1->data.string->data);
		if (slen < 32) {
			for (sloop = 0; sloop < 32; sloop++)
				holdbuf[sloop] = oper1->data.string->data[sloop % slen];
		} else {
			memcpy(holdbuf, oper1->data.string->data, 32);
		}
		for (sloop = 0; sloop < 16; sloop++)
			buf[sloop] = ((holdbuf[sloop * 2] - 65) & 0x0F) |
					(((holdbuf[(sloop * 2) + 1] - 65) & 0x0F) << 4);
		buf[16] = '\0';
		fr->rndbuf = init_seed(buf);
	}
	CLEAR(oper1);
}

void 
prim_int(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
	if (!(oper1->type == PROG_OBJECT || oper1->type == PROG_VAR ||
		  oper1->type == PROG_LVAR || oper1->type == PROG_FLOAT))
		abort_interp("Invalid argument type.");
	if ((!(oper1->type == PROG_FLOAT)) ||
		(oper1->type == PROG_FLOAT && arith_good((double) oper1->data.fnumber))) {
		result = (int) ((oper1->type == PROG_OBJECT) ?
						oper1->data.objref : (oper1->type == PROG_FLOAT) ?
						oper1->data.fnumber : oper1->data.number);
	} else {
		result = 0;
		fr->error.error_flags.i_bounds = 1;
	}
	CLEAR(oper1);
	PushInt(result);
}


