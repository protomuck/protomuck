/* Primitives package */
#include "copyright.h"
#include "config.h"
#include "db.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#ifndef NAN
#include "nan.h"
#endif
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "strings.h"
#include "interp.h"
#ifndef WIN_VC
# define MAXINT ~(1<<((sizeof(int)*8)-1))
# define MININT (1<<((sizeof(int)*8)-1))
#endif
#define no_good(x) ((x == INF) || (x == NINF) || (x == NAN))
#define ISINF(x) ((x == INF) || (x == NINF))
#define ISNAN(x) (x == NAN)
int
arith_good(double test)
{
    return (((test) <= ((double) (MAXINT))) && ((test) >= ((double) (MININT))));
}
int
arith_type(struct inst *op1, struct inst *op2)
{
    return ((op1->type == PROG_INTEGER && op2->type == PROG_INTEGER) /* real stuff */
            ||(op1->type == PROG_OBJECT && op2->type == PROG_INTEGER) /* inc. dbref */
            ||(op1->type == PROG_VAR && op2->type == PROG_INTEGER) /* offset array */
            ||(op1->type == PROG_LVAR && op2->type == PROG_INTEGER)
            || (op1->type == PROG_FLOAT && op2->type == PROG_FLOAT)
            || (op1->type == PROG_FLOAT && op2->type == PROG_INTEGER)
            || (op1->type == PROG_INTEGER && op2->type == PROG_FLOAT));
}
void
prim_add(PRIM_PROTOTYPE)
{
	double fresult, tf1, tf2, tl;
	int tmp, result;
    
    if (!arith_type(&oper[1], &oper[0]))
        abort_interp("Invalid argument type");
    if ((oper[0].type == PROG_FLOAT) || (oper[1].type == PROG_FLOAT)) {
        tf1 =
            (oper[0].type ==
             PROG_FLOAT) ? oper[0].data.fnumber : oper[0].data.number;
        tf2 =
            (oper[1].type ==
             PROG_FLOAT) ? oper[1].data.fnumber : oper[1].data.number;
        if (!no_good(tf1) && !no_good(tf2)) {
            fresult = tf1 + tf2;
        } else {
            if (ISNAN(tf1) || ISNAN(tf2)) {
                fresult = tp_alt_infinity_handler ? NAN : 0.0;
                if (!tp_alt_infinity_handler)
                    fr->error.error_flags.nan = 1;
            } else {
                fresult = tp_alt_infinity_handler ? (tf1 + tf2) : 0.0;
                if (!tp_alt_infinity_handler)
                    fr->error.error_flags.f_bounds = 1;
            }
        }
    } else {
        result = oper[0].data.number + oper[1].data.number;
        tl = (double) oper[0].data.number + (double) oper[1].data.number;
        if (!arith_good(tl))
            fr->error.error_flags.i_bounds = 1;
    }
    tmp = (oper[1].type == PROG_FLOAT
           || oper[0].type == PROG_FLOAT) ? PROG_FLOAT : oper[1].type;
    
    if (tmp == PROG_FLOAT)
        push(arg, top, tmp, MIPSCAST & fresult);
    else
        push(arg, top, tmp, MIPSCAST & result);
}
void
prim_subtract(PRIM_PROTOTYPE)
{
	double fresult, tf1, tf2, tl;
	int tmp, result;
    
    if (!arith_type(&oper[1], &oper[0]))
        abort_interp("Invalid argument type.");
    if ((oper[0].type == PROG_FLOAT) || (oper[1].type == PROG_FLOAT)) {
        tf1 =
            (oper[1].type ==
             PROG_FLOAT) ? oper[1].data.fnumber : oper[1].data.number;
        tf2 =
            (oper[0].type ==
             PROG_FLOAT) ? oper[0].data.fnumber : oper[0].data.number;
        if (!no_good(tf1) && !no_good(tf2)) {
            fresult = tf1 - tf2;
        } else {
            if (ISNAN(tf1) || ISNAN(tf2)) {
                fresult = tp_alt_infinity_handler ? NAN : 0.0;
                if (!tp_alt_infinity_handler)
                    fr->error.error_flags.nan = 1;
            } else {
                fresult = tp_alt_infinity_handler ? (tf1 + tf2) : 0.0;
                if (!tp_alt_infinity_handler)
                    fr->error.error_flags.f_bounds = 1;
            }
        }
    } else {
        result = oper[1].data.number - oper[0].data.number;
        tl = (double) oper[1].data.number - (double) oper[0].data.number;
        if (!arith_good(tl))
            fr->error.error_flags.i_bounds = 1;
    }
    tmp = (oper[1].type == PROG_FLOAT
           || oper[0].type == PROG_FLOAT) ? PROG_FLOAT : oper[1].type;
    
    if (tmp == PROG_FLOAT)
        push(arg, top, tmp, MIPSCAST & fresult);
    else
        push(arg, top, tmp, MIPSCAST & result);
}
void
prim_multiply(PRIM_PROTOTYPE)
{
	double fresult, tf1, tf2, tl;
	int tmp, result;
    
    if (!arith_type(&oper[1], &oper[0]))
        abort_interp("Invalid argument type.");
    if ((oper[0].type == PROG_FLOAT) || (oper[1].type == PROG_FLOAT)) {
        tf1 =
            (oper[0].type ==
             PROG_FLOAT) ? oper[0].data.fnumber : oper[0].data.number;
        tf2 =
            (oper[1].type ==
             PROG_FLOAT) ? oper[1].data.fnumber : oper[1].data.number;
        if (!no_good(tf1) && !no_good(tf2)) {
            fresult = tf1 * tf2;
        } else {
            if (ISNAN(tf1) || ISNAN(tf2)) {
                fresult = tp_alt_infinity_handler ? NAN : 0.0;
                if (!tp_alt_infinity_handler)
                    fr->error.error_flags.nan = 1;
            } else {
                fresult = tp_alt_infinity_handler ? (tf1 * tf2) : 0.0;
                if (!tp_alt_infinity_handler)
                    fr->error.error_flags.f_bounds = 1;
            }
        }
    } else {
        result = oper[0].data.number * oper[1].data.number;
        tl = (double) oper[0].data.number * (double) oper[1].data.number;
        if (!arith_good(tl))
            fr->error.error_flags.i_bounds = 1;
    }
    tmp = (oper[1].type == PROG_FLOAT
           || oper[0].type == PROG_FLOAT) ? PROG_FLOAT : oper[1].type;
    
    if (tmp == PROG_FLOAT)
        push(arg, top, tmp, MIPSCAST & fresult);
    else
        push(arg, top, tmp, MIPSCAST & result);
}
void
prim_divide(PRIM_PROTOTYPE)
{
	double fresult, tf1, tf2;
	int result, tmp;
    
    if (!arith_type(&oper[1], &oper[0]))
        abort_interp("Invalid argument type");
    if ((oper[0].type == PROG_FLOAT) || (oper[1].type == PROG_FLOAT)) {
        if ((oper[0].type == PROG_INTEGER && !oper[0].data.number) ||
            (oper[0].type == PROG_FLOAT && fabs(oper[0].data.fnumber)
             < DBL_EPSILON)) {
            fresult = tp_alt_infinity_handler ?
                ((oper[1].type ==
                  PROG_INTEGER) ? oper[1].data.number : oper[1].data.fnumber) *
                INF : 0.0;
            fr->error.error_flags.div_zero = 1;
        } else {
            tf1 =
                (oper[1].type ==
                 PROG_FLOAT) ? oper[1].data.fnumber : oper[1].data.number;
            tf2 =
                (oper[0].type ==
                 PROG_FLOAT) ? oper[0].data.fnumber : oper[0].data.number;
            if (!no_good(tf1) && !no_good(tf2)) {
                fresult = tf1 / tf2;
            } else {
                if (ISNAN(tf1) || ISNAN(tf2)) {
                    fresult = tp_alt_infinity_handler ? NAN : 0.0;
                    if (!tp_alt_infinity_handler)
                        fr->error.error_flags.nan = 1;
                } else {
                    fresult = tp_alt_infinity_handler ? (tf1 / tf2) : 0.0;
                    if (!tp_alt_infinity_handler)
                        fr->error.error_flags.f_bounds = 1;
                }
            }
        }
    } else {
        if (oper[0].data.number) {
            result = oper[1].data.number / oper[0].data.number;
        } else {
            result = 0;
            fr->error.error_flags.div_zero = 1;
        }
    }
    tmp = (oper[1].type == PROG_FLOAT
           || oper[0].type == PROG_FLOAT) ? PROG_FLOAT : oper[1].type;
    
    if (tmp == PROG_FLOAT)
        push(arg, top, tmp, MIPSCAST & fresult);
    else
        push(arg, top, tmp, MIPSCAST & result);
}
void
prim_mod(PRIM_PROTOTYPE)
{
	int result, tmp;
    
    if ((!arith_type(&oper[1], &oper[0])) || (oper[0].type == PROG_FLOAT) ||
        (oper[1].type == PROG_FLOAT))
        abort_interp("Invalid argument type.");
    if (oper[0].data.number)
        result = oper[1].data.number % oper[0].data.number;
    else
        result = 0;
    tmp = oper[1].type;
    
    push(arg, top, tmp, MIPSCAST & result);
}
void
prim_bitor(PRIM_PROTOTYPE)
{
	int result, tmp;
    
    if (!arith_type(&oper[1], &oper[0]))
        abort_interp("Invalid argument type");
    result = oper[1].data.number | oper[0].data.number;
    tmp = oper[1].type;
    
    push(arg, top, tmp, MIPSCAST & result);
}
void
prim_bitxor(PRIM_PROTOTYPE)
{
	int result, tmp;
    
    if (!arith_type(&oper[1], &oper[0]))
        abort_interp("Invalid argument type");
    result = oper[1].data.number ^ oper[0].data.number;
    tmp = oper[1].type;
    
    push(arg, top, tmp, MIPSCAST & result);
}
void
prim_bitand(PRIM_PROTOTYPE)
{
	int result, tmp;
    
    if (!arith_type(&oper[1], &oper[0]))
        abort_interp("Invalid argument type");
    result = oper[1].data.number & oper[0].data.number;
    tmp = oper[1].type;
    
    push(arg, top, tmp, MIPSCAST & result);
}
void
prim_bitshift(PRIM_PROTOTYPE)
{
	int result, tmp;
    
    if (!arith_type(&oper[1], &oper[0]))
        abort_interp("Invalid argument type");
    if (oper[0].data.number > 0)
        result = oper[1].data.number << oper[0].data.number;
    else if (oper[0].data.number < 0)
        result = oper[1].data.number >> (-(oper[0].data.number));
    else
        result = oper[1].data.number;
    tmp = oper[1].type;
    
    push(arg, top, tmp, MIPSCAST & result);
}
void
prim_and(PRIM_PROTOTYPE)
{
	int result;
    
    result = !logical_false(&oper[0]) && !logical_false(&oper[1]);
    
    PushInt(result);
}
void
prim_or(PRIM_PROTOTYPE)
{
	int result;
    
    result = !logical_false(&oper[0]) || !logical_false(&oper[1]);
    
    PushInt(result);
}
void
prim_xor(PRIM_PROTOTYPE)
{
	int result;
    
    if (logical_false(&oper[0]))
        result = !logical_false(&oper[1]);
    else
        result = logical_false(&oper[1]);
    
    PushInt(result);
}
void
prim_not(PRIM_PROTOTYPE)
{
	int result;
    
    result = logical_false(&oper[0]);
    
    PushInt(result);
}
int
comp_t(struct inst *op)
{
    return (op->type == PROG_INTEGER || op->type == PROG_FLOAT
            || op->type == PROG_OBJECT);
}
void
prim_lessthan(PRIM_PROTOTYPE)
{	
	double tf1, tf2;
	int result;
    
    if (!comp_t(&oper[0]) || !comp_t(&oper[1]))
        abort_interp("Invalid argument type.");
    if (oper[0].type == PROG_FLOAT || oper[1].type == PROG_FLOAT) {
        tf1 = (oper[1].type == PROG_FLOAT) ? oper[1].data.fnumber :
            (oper[1].type == PROG_INTEGER) ? oper[1].data.number :
            oper[1].data.objref;
        tf2 = (oper[0].type == PROG_FLOAT) ? oper[0].data.fnumber :
            (oper[0].type == PROG_INTEGER) ? oper[0].data.number :
            oper[0].data.objref;
        result = tf1 < tf2;
    } else {
        result =
            (((oper[1].type ==
               PROG_INTEGER) ? oper[1].data.number : oper[1].data.objref)
             < ((oper[0].type == PROG_INTEGER) ? oper[0].data.number : oper[0].
                data.objref));
    }
    
    PushInt(result);
}
void
prim_greathan(PRIM_PROTOTYPE)
{
	double tf1, tf2;
	int result;
    
    if (!comp_t(&oper[0]) || !comp_t(&oper[1]))
        abort_interp("Invalid argument type.");
    if (oper[0].type == PROG_FLOAT || oper[1].type == PROG_FLOAT) {
        tf1 = (oper[1].type == PROG_FLOAT) ? oper[1].data.fnumber :
            (oper[1].type == PROG_INTEGER) ? oper[1].data.number :
            oper[1].data.objref;
        tf2 = (oper[0].type == PROG_FLOAT) ? oper[0].data.fnumber :
            (oper[0].type == PROG_INTEGER) ? oper[0].data.number :
            oper[0].data.objref;
        result = tf1 > tf2;
    } else {
        result =
            (((oper[1].type ==
               PROG_INTEGER) ? oper[1].data.number : oper[1].data.objref)
             > ((oper[0].type == PROG_INTEGER) ? oper[0].data.number : oper[0].
                data.objref));
    }
    
    PushInt(result);
}
void
prim_equal(PRIM_PROTOTYPE)
{
	double tf1, tf2;
	int result;
    
    if (!comp_t(&oper[0]) || !comp_t(&oper[1]))
        abort_interp("Invalid argument type.");
    if (oper[0].type == PROG_FLOAT || oper[1].type == PROG_FLOAT) {
        tf1 = (oper[1].type == PROG_FLOAT) ? oper[1].data.fnumber :
            (oper[1].type == PROG_INTEGER) ? oper[1].data.number :
            oper[1].data.objref;
        tf2 = (oper[0].type == PROG_FLOAT) ? oper[0].data.fnumber :
            (oper[0].type == PROG_INTEGER) ? oper[0].data.number :
            oper[0].data.objref;
        result = tf1 == tf2;
    } else {
        result =
            (((oper[1].type ==
               PROG_INTEGER) ? oper[1].data.number : oper[1].data.objref)
             == ((oper[0].type == PROG_INTEGER) ? oper[0].data.number : oper[0].
                 data.objref));
    }
    
    PushInt(result);
}
void
prim_noteq(PRIM_PROTOTYPE)
{
	double tf1, tf2;
	int result;
    
    if (!comp_t(&oper[0]) || !comp_t(&oper[1]))
        abort_interp("Invalid argument type.");
    if (oper[0].type == PROG_FLOAT || oper[1].type == PROG_FLOAT) {
        tf1 = (oper[1].type == PROG_FLOAT) ? oper[1].data.fnumber :
            (oper[1].type == PROG_INTEGER) ? oper[1].data.number :
            oper[1].data.objref;
        tf2 = (oper[0].type == PROG_FLOAT) ? oper[0].data.fnumber :
            (oper[0].type == PROG_INTEGER) ? oper[0].data.number :
            oper[0].data.objref;
        result = tf1 != tf2;
    } else {
        result =
            (((oper[1].type ==
               PROG_INTEGER) ? oper[1].data.number : oper[1].data.objref)
             != ((oper[0].type == PROG_INTEGER) ? oper[0].data.number : oper[0].
                 data.objref));
    }
    
    PushInt(result);
}
void
prim_lesseq(PRIM_PROTOTYPE)
{
	double tf1, tf2;
	int result;
    
    if (!comp_t(&oper[0]) || !comp_t(&oper[1]))
        abort_interp("Invalid argument type.");
    if (oper[0].type == PROG_FLOAT || oper[1].type == PROG_FLOAT) {
        tf1 = (oper[1].type == PROG_FLOAT) ? oper[1].data.fnumber :
            (oper[1].type == PROG_INTEGER) ? oper[1].data.number :
            oper[1].data.objref;
        tf2 = (oper[0].type == PROG_FLOAT) ? oper[0].data.fnumber :
            (oper[0].type == PROG_INTEGER) ? oper[0].data.number :
            oper[0].data.objref;
        result = tf1 <= tf2;
    } else {
        result =
            (((oper[1].type ==
               PROG_INTEGER) ? oper[1].data.number : oper[1].data.objref)
             <= ((oper[0].type == PROG_INTEGER) ? oper[0].data.number : oper[0].
                 data.objref));
    }
    
    PushInt(result);
}
void
prim_greateq(PRIM_PROTOTYPE)
{
	double tf1, tf2;
	int result;
    
    if (!comp_t(&oper[0]) || !comp_t(&oper[1]))
        abort_interp("Invalid argument type.");
    if (oper[0].type == PROG_FLOAT || oper[1].type == PROG_FLOAT) {
        tf1 = (oper[1].type == PROG_FLOAT) ? oper[1].data.fnumber :
            (oper[1].type == PROG_INTEGER) ? oper[1].data.number :
            oper[1].data.objref;
        tf2 = (oper[0].type == PROG_FLOAT) ? oper[0].data.fnumber :
            (oper[0].type == PROG_INTEGER) ? oper[0].data.number :
            oper[0].data.objref;
        result = tf1 >= tf2;
    } else {
        result =
            (((oper[1].type ==
               PROG_INTEGER) ? oper[1].data.number : oper[1].data.objref)
             >= ((oper[0].type == PROG_INTEGER) ? oper[0].data.number : oper[0].
                 data.objref));
    }
    
    PushInt(result);
}
void
prim_random(PRIM_PROTOTYPE)
{
    int result = RANDOM();
    CHECKOFLOW(1);
    PushInt(result);
}
void
prim_srand(PRIM_PROTOTYPE)
{
	int result;
    
    CHECKOFLOW(1);
    if (!(fr->rndbuf)) {
        fr->rndbuf = init_seed(NULL);
    }
    result = (int) rndm(fr->rndbuf);
    PushInt(result);
}
void
prim_getseed(PRIM_PROTOTYPE)
{
    char buf[33];
    char buf2[17];
    int loop;
    
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
    
    if (!(oper[0].type == PROG_STRING))
        abort_interp("Invalid argument type.");
    if (fr->rndbuf) {
        delete_seed(fr->rndbuf);
        fr->rndbuf = NULL;
    }
    if (!oper[0].data.string) {
        fr->rndbuf = init_seed(NULL);
        
        return;
    } else {
        slen = strlen(oper[0].data.string->data);
        if (slen < 32) {
            for (sloop = 0; sloop < 32; sloop++)
                holdbuf[sloop] = oper[0].data.string->data[sloop % slen];
        } else {
            memcpy(holdbuf, oper[0].data.string->data, 32);
        }
        for (sloop = 0; sloop < 16; sloop++)
            buf[sloop] = ((holdbuf[sloop * 2] - 65) & 0x0F) |
                (((holdbuf[(sloop * 2) + 1] - 65) & 0x0F) << 4);
        buf[16] = '\0';
        fr->rndbuf = init_seed(buf);
    }
    
}
void
prim_int(PRIM_PROTOTYPE)
{
	int result;
    
    if (!(oper[0].type == PROG_OBJECT || oper[0].type == PROG_VAR ||
          oper[0].type == PROG_LVAR || oper[0].type == PROG_FLOAT))
        abort_interp("Invalid argument type.");
    if ((!(oper[0].type == PROG_FLOAT)) ||
        (oper[0].type == PROG_FLOAT
         && arith_good((double) oper[0].data.fnumber))) {
        result =
            (int) ((oper[0].type ==
                    PROG_OBJECT) ? oper[0].data.objref : (oper[0].type ==
                                                         PROG_FLOAT) ? oper[0].
                   data.fnumber : oper[0].data.number);
    } else {
        result = 0;
        fr->error.error_flags.i_bounds = 1;
    }
    
    PushInt(result);
}
void
prim_plusplus(PRIM_PROTOTYPE)
{
    struct inst *tmp;
    int vnum, result;
	double fresult;
	struct inst temp1, temp2;
    
    temp1 = oper[0];
    if (oper[0].type == PROG_VAR || oper[0].type == PROG_SVAR
        || oper[0].type == PROG_LVAR)
        if (oper[0].data.number > MAX_VAR || oper[0].data.number < 0)
            abort_interp("Variable number out of range.");
    switch (oper[0].type) {
        case PROG_VAR:
            copyinst(&(fr->variables[temp1.data.number]), &temp2);
            break;
        case PROG_SVAR:
            tmp = scopedvar_get(fr, 0, temp1.data.number);
            copyinst(tmp, &temp2);
            break;
        case PROG_LVAR:{
            struct localvars *tmp2 = localvars_get(fr, program);
            copyinst(&(tmp2->lvars[temp1.data.number]), &temp2);
            break;
        }
        case PROG_INTEGER:
            oper[0].data.number++;
            result = oper[0].data.number;
            
            PushInt(result);
            return;
        case PROG_OBJECT:
            oper[0].data.objref++;
            result = oper[0].data.objref;
            
            PushObject(result);
            return;
        case PROG_FLOAT:
            oper[0].data.fnumber++;
            fresult = oper[0].data.fnumber;
            
            PushFloat(fresult);
            return;
        default:
            abort_interp("Invalid datatype.");
    }
    vnum = temp1.data.number;
    switch (temp2.type) {
        case PROG_INTEGER:
            temp2.data.number++;
            result = temp2.data.number;
            break;
        case PROG_OBJECT:
            temp2.data.objref++;
            result = temp2.data.objref;
            break;
        case PROG_FLOAT:
            temp2.data.fnumber++;
            fresult = temp2.data.fnumber;
            break;
        default:
            abort_interp("Invalid datatype in variable.");
    }
    switch (temp1.type) {
        case PROG_VAR:{
            CLEAR(&(fr->variables[vnum]));
            copyinst(&temp2, &(fr->variables[vnum]));
            break;
        }
        case PROG_SVAR:{
            struct inst *tmp2;
            tmp2 = scopedvar_get(fr, 0, vnum);
            if (!tmp2)
                abort_interp("Scoped variable number out of range.");
            CLEAR(tmp2);
            copyinst(&temp2, tmp2);
            break;
        }
        case PROG_LVAR:{
            struct localvars *tmp2 = localvars_get(fr, program);
            CLEAR(&(tmp2->lvars[vnum]));
            copyinst(&temp2, &(tmp2->lvars[vnum]));
            break;
        }
    }
    
}
void
prim_minusminus(PRIM_PROTOTYPE)
{
    struct inst *tmp;
    int vnum, result;
	double fresult;
	struct inst temp1, temp2;
    temp1 = oper[0];
    if (oper[0].type == PROG_VAR || oper[0].type == PROG_SVAR
        || oper[0].type == PROG_LVAR)
        if (oper[0].data.number > MAX_VAR || oper[0].data.number < 0)
            abort_interp("Variable number out of range.");
    switch (oper[0].type) {
        case PROG_VAR:
            copyinst(&(fr->variables[temp1.data.number]), &temp2);
            break;
        case PROG_SVAR:
            tmp = scopedvar_get(fr, 0, temp1.data.number);
            copyinst(tmp, &temp2);
            break;;
        case PROG_LVAR:{
            struct localvars *tmp2 = localvars_get(fr, program);
            copyinst(&(tmp2->lvars[temp1.data.number]), &temp2);
            break;
        }
        case PROG_INTEGER:
            oper[0].data.number--;
            result = oper[0].data.number;
            
            PushInt(result);
            return;
        case PROG_OBJECT:
            oper[0].data.objref--;
            result = oper[0].data.objref;
            
            PushObject(result);
            return;
        case PROG_FLOAT:
            oper[0].data.fnumber--;
            fresult = oper[0].data.fnumber;
            
            PushFloat(fresult);
            return;
        default:
            abort_interp("Invalid datatype.");
    }
    vnum = temp1.data.number;
    switch (temp2.type) {
        case PROG_INTEGER:
            temp2.data.number--;
            result = temp2.data.number;
            break;
        case PROG_OBJECT:
            temp2.data.objref--;
            result = temp2.data.objref;
            break;
        case PROG_FLOAT:
            temp2.data.fnumber--;
            fresult = temp2.data.fnumber;
            break;
        default:
            abort_interp("Invalid datatype in variable.");
    }
    switch (temp1.type) {
        case PROG_VAR:{
            CLEAR(&(fr->variables[vnum]));
            copyinst(&temp2, &(fr->variables[vnum]));
            break;
        }
        case PROG_SVAR:{
            struct inst *tmp2;
            tmp2 = scopedvar_get(fr, 0, vnum);
            if (!tmp2)
                abort_interp("Scoped variable number out of range.");
            CLEAR(tmp2);
            copyinst(&temp2, tmp2);
            break;
        }
        case PROG_LVAR:{
            struct localvars *tmp2 = localvars_get(fr, program);
            CLEAR(&(tmp2->lvars[vnum]));
            copyinst(&temp2, &(tmp2->lvars[vnum]));
            break;
        }
    }
    
}
void
prim_abs(PRIM_PROTOTYPE)
{
	int result;
    
    if (oper[0].type != PROG_INTEGER)
        abort_interp("Non-integer argument.");
    result = oper[0].data.number;
    if (result < 0)
        result = -result;
    
    PushInt(result);
}
void
prim_sign(PRIM_PROTOTYPE)
{
	int result;
    
    if (oper[0].type != PROG_INTEGER)
        abort_interp("Non-integer argument.");
    if (oper[0].data.number > 0)
        result = 1;
    else if (oper[0].data.number < 0)
        result = -1;
    else
        result = 0;
    
    PushInt(result);
}
