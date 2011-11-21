/* Primitives package */

#include "copyright.h"
#include "config.h"

#include "db.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "strings.h"
#include "interp.h"
#include "netresolve.h"

extern struct inst *oper1, *oper2, *oper3, *oper4, *oper5, *oper6;
extern struct inst temp1, temp2, temp3;
extern int tmp, result;
extern dbref ref;

int
check_descr_flag(char *dflag)
{
/* New function in 1.7. Like the check_flag functions of 
 * p_db.c, identifies flags for descriptor flag support.
 */
    if (string_prefix("df_html", dflag))
        return DF_HTML;
    if (string_prefix("df_pueblo", dflag))
        return DF_PUEBLO;
    if (string_prefix("df_muf", dflag))
        return DF_MUF;
    if (string_prefix("df_idle", dflag))
        return DF_IDLE;
    if (string_prefix("df_trueidle", dflag))
        return DF_TRUEIDLE;
    if (string_prefix("df_color", dflag))
        return DF_COLOR;
    if (string_prefix("df_interactive", dflag))
        return DF_INTERACTIVE;
#ifdef USE_SSL
    if (string_prefix("df_ssl", dflag))
        return DF_SSL;
#endif /* USE_SSL */
#ifdef IPV6
    if (string_prefix("df_ipv6", dflag))
        return DF_IPV6;
#endif /* USE_SSL */
    if (string_prefix("df_suid", dflag))
        return DF_SUID;
    if (string_prefix("df_webclient", dflag))
        return DF_WEBCLIENT;
    if (string_prefix("df_misc", dflag))
        return DF_MISC;
#ifdef MCCP_ENABLED
    if (string_prefix("df_compress", dflag))
        return DF_COMPRESS;
#endif /* MCCP_ENABLED */
    return 0;
}

int
descr_flag_set_perms(int dflag, int mlev, dbref prog)
{
/* Checks to see if a flag is settable for descr_set.
 */
    if (mlev < LARCH)           /* For now all descr flag stuff is W1. */
        return 0;

    /* Standard non-settables */
    if (dflag == DF_HTML || dflag == DF_PUEBLO || dflag == DF_MUF
        || dflag == DF_TRUEIDLE || dflag == DF_INTERACTIVE || dflag == DF_SUID
#ifdef USE_SSL
        || dflag == DF_SSL
#endif /* USE_SSL */
#ifdef MCCP_ENABLED
        || dflag == DF_COMPRESS 
#endif /* MCCP_ENABLED */
        )
        return 0;

    /* Default is settable */
    return 1;
}

int
descr_flag_check_perms(int descr, int dFlag, int mLev)
{
    /* For descr flag support. In the future, will check
     * for flag checking permissions, but for now, there
     * aren't any descr flags that would require limited
     * checking privs.
     */
    return 1;
}

int
has_descr_flag(int descr, char *dFlag, int mLev)
{
/* For descriptor flag support in 1.7. Checks to see
 * if the descriptor has the flag if mlev high enough.
 */
    int flagValue = 0, rslt = 0, result = 0;

    if (mLev < LMAGE)
        return -2;
    result = 0;
    while (*dFlag == '!') {
        dFlag++;
        rslt = (!rslt);
    }
    if (!*dFlag)
        return -1;
    flagValue = check_descr_flag(dFlag);

    if (!flagValue)
        return -1;
    if (!descr_flag_check_perms(descr, flagValue, mLev))
        return -2;

    result = (DR_FLAGS(descr, flagValue));

    if (rslt)
        return !result;
    else
        return result;
}



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

    CLEAR(oper1);
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
    stk_array *nw;
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
    nw = new_array_packed(result);
    for (i = 0; i < result; i++) {
        temp1.data.number = i;
        temp2.data.number = pdbref(i + 1);
        array_setitem(&nw, &temp1, &temp2);
    }
    PushArrayRaw(nw);
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
    result = (int)pidle(result);
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
    result = (int)pontime(result);
    CHECKOFLOW(1);
    CLEAR(oper1);
    PushInt(result);
}

void
prim_conhost(PRIM_PROTOTYPE)
{
    /* int -- char * */
    char *pname;

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
    char *pname;

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
    char *pname;

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
    char *pname;

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
    oper1 = POP();

    if (mlev < LWIZ)
        abort_interp("Wizard prim");

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
    oper2 = POP();              /* string */
    oper1 = POP();              /* int */
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
    if (!pdescrp(oper1->data.number))
        abort_interp("That is not a valid descriptor.");
    result = oper1->data.number;
    result = pnextdescr(result);
    CLEAR(oper1);
    PushInt(result);
}


void
prim_descriptors(PRIM_PROTOTYPE)
{
    int mycount = 0;
    int di, dcount, descr;
    int *darr;

    CHECKOP(1);
    oper1 = POP();
    if (mlev < LM3)
        abort_interp("M3 prim");
    if (oper1->type != PROG_OBJECT)
        abort_interp("Argument not a dbref");
    if (oper1->data.objref != NOTHING && !valid_object(oper1))
        abort_interp("Bad dbref");
    ref = oper1->data.objref;
    if ((ref != NOTHING) && (!valid_player(oper1)))
        abort_interp("Non-player argument");
    CLEAR(oper1);
    CHECKOP(0);

    if (ref == NOTHING) {
        result = pfirstdescr();
        while (result) {
            CHECKOFLOW(1);
            PushInt(result);
            result = pnextdescr(result);
            mycount++;
        }
    } else {
        darr = get_player_descrs(ref, &dcount);
        for (di = 0; di < dcount; di++) {
            CHECKOFLOW(1);
            descr = index_descr(darr[di]);
            PushInt(descr);
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
    int *darr;
    int di, dcount, descr;
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

    temp1.type = PROG_INTEGER;
    temp2.type = PROG_INTEGER;
    temp1.line = 0;
    temp2.line = 0;
    if (ref == NOTHING) {
        result = pfirstdescr();
        i = pdescrcount();
        newarr = new_array_packed(i);
        i = 0;
        while (result) {
            temp1.data.number = i;
            temp2.data.number = result;
            array_setitem(&newarr, &temp1, &temp2);
            result = pnextdescr(result);
            i++;
        }
    } else {
        darr = get_player_descrs(ref, &dcount);
        newarr = new_array_packed(dcount);
        for (di = 0; di < dcount; di++) {
            descr = index_descr(darr[di]);
            temp1.data.number = di;
            temp2.data.number = descr;
            array_setitem(&newarr, &temp1, &temp2);
        }
    }
    PushArrayRaw(newarr);
}


void
prim_descr_setuser(PRIM_PROTOTYPE)
{
    char *ptr;
    char pad_char[] = "";

    CHECKOP(3);
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
    if (!pdescrp(oper1->data.number))
        abort_interp("That is not a valid descriptor.");
    ptr = oper3->data.string ? oper3->data.string->data : pad_char;
    if (ref != NOTHING) {
        const char *passwd = DBFETCH(ref)->sp.player.password;

        if (passwd) {
            if (!check_password(ref, ptr)) {
                CLEAR(oper1);
                CLEAR(oper2);
                CLEAR(oper3);
                result = 0;
                PushInt(result);
            }
        }
    }
    if (ref != NOTHING) {
        log_status("SUSR: %d %s(%d) to %s(%d)\n",
                   oper1->data.number, OkObj(player) ? NAME(player) : "(Login)",
                   player, NAME(ref), ref);
    }
    tmp = oper1->data.number;

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    result = pset_user2(tmp, ref);

    PushInt(result);
}

void
prim_descr_setuser_nopass(PRIM_PROTOTYPE)
{

    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();

    if (mlev < LBOY)
        abort_interp("W4 permissions required.");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Integer descriptor number expected (1)");
    if (oper2->type != PROG_OBJECT)
        abort_interp("Player dbref expected (2)");
    ref = oper2->data.objref;
    if (ref != NOTHING && !valid_player(oper2))
        abort_interp("Player dbref expected (2)");
    if (ref == 1) {
        log_status("SHAM: descr_setuser_nopass to #1 by %s\n", unparse_object(player, player));
        abort_interp("You cannot setuser to #1 without a password.");
    }
    if (!pdescrp(oper1->data.number))
        abort_interp("That is not a valid descriptor.");
    if (ref != NOTHING) {
        log_status("SUSR: %d %s(%d) to %s(%d)\n",
                   oper1->data.number, OkObj(player) ? NAME(player) : "(Login)",
                   player, NAME(ref), ref);
    }
    tmp = oper1->data.number;

    CLEAR(oper1);
    CLEAR(oper2);

    result = pset_user2(tmp, ref);

    PushInt(result);
}

void
prim_suid(PRIM_PROTOTYPE)
{
    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (mlev < LBOY)
        abort_interp("W4 permissions required.");
    if (oper1->type != PROG_OBJECT)
        abort_interp("DBref expected. (1)");
    if (ref != NOTHING && !valid_player(oper1))
        abort_interp("Player dbref expected (1)");

    if (oper2->type != PROG_INTEGER)
        abort_interp("Integer expected. (2)");
    if (oper2->data.number >= 1 && !pdescrp(oper2->data.number))
        abort_interp("-1, 0, or valid descriptor required on item (2).");

    fr->player = oper1->data.objref;
    player = oper1->data.objref;
    fr->variables[0].type = PROG_OBJECT;
    fr->variables[0].data.objref = oper1->data.objref;

    if (oper2->data.number > 0)
        result = pset_user_suid(oper2->data.number, oper1->data.objref);
    else if (oper2->data.number == 0)
        result = pset_user_suid(fr->descr, oper1->data.objref);
    else result = 1;

    CLEAR(oper1);
    CLEAR(oper2);
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
prim_firstdescr(PRIM_PROTOTYPE)
{
    /* ref -- int */
    int *darr;
    int dcount;

    CHECKOP(1);
    oper2 = POP();
    if (mlev < LMAGE)
        abort_interp("Mage level prim.");
    if (oper2->type != PROG_OBJECT)
        abort_interp("Player dbref expected (2)");
    ref = oper2->data.objref;
    if (ref != NOTHING && !valid_player(oper2))
        abort_interp("Player dbref expected (2)");
    if (ref == NOTHING) {
        result = pfirstdescr();
    } else {
        if (Typeof(ref) != TYPE_PLAYER)
            abort_interp("invalid argument");
        if (online(ref)) {
            darr = get_player_descrs(ref, &dcount);
            result = index_descr(darr[dcount - 1]);
        } else {
            result = 0;
        }
    }
    CHECKOFLOW(1);
    CLEAR(oper2);
    PushInt(result);
}

void
prim_lastdescr(PRIM_PROTOTYPE)
{
    /* ref -- int */
    int *darr;
    int dcount;

    CHECKOP(1);
    oper2 = POP();
    if (mlev < LMAGE)
        abort_interp("Mage level prim.");
    if (oper2->type != PROG_OBJECT)
        abort_interp("Player dbref expected (2)");
    ref = oper2->data.objref;
    if (ref != NOTHING && !valid_player(oper2))
        abort_interp("Player dbref expected (2)");
    if (ref == NOTHING) {
        result = plastdescr();
    } else {
        if (Typeof(ref) != TYPE_PLAYER)
            abort_interp("invalid argument");
        if (online(ref)) {
            darr = get_player_descrs(ref, &dcount);
            result = index_descr(darr[0]);
        } else {
            result = 0;
        }
    }
    CHECKOFLOW(1);
    CLEAR(oper2);
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

    result = (pdescrtype(oper1->data.number) == CT_PUEBLO);
    CHECKOFLOW(1);
    CLEAR(oper1);
    CHECKOFLOW(1);
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

    result = (pdescrtype(oper1->data.number) == CT_PUEBLO);

    CHECKOFLOW(1);
    CLEAR(oper1);
    PushInt(result);
}

void
prim_descr_sslp(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Requires Mucker Level 3 or better.");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Integer descriptor number expected.");

    if (!pdescrp(oper1->data.number))
        abort_interp("That is not a valid descriptor.");
#ifdef USE_SSL
    result = (pdescrtype(oper1->data.number) == CT_SSL);
#else
    result = 0;
#endif

    CHECKOFLOW(1);
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

    CHECKOFLOW(1);
    CLEAR(oper1);
    PushInt(result);
}

void
prim_motd_notify(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();
    if (mlev < 3)
        abort_interp("Requires Mucker Level 3 or better.");
    if (!valid_object(oper1))
        abort_interp("invalid argument");
    ref = oper1->data.objref;

    CLEAR(oper1);
    spit_file(ref, MOTD_FILE);
}

void
prim_descr_logout(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (mlev < LARCH)
        abort_interp("Requires Wizard Level 3 (Archwizard) or better.");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Integer descriptor number expected.");

    if (!pdescrp(oper1->data.number))
        abort_interp("That is not a valid descriptor.");

    pdescr_logout(oper1->data.number);
    CLEAR(oper1);
}

void
prim_descrdbref(PRIM_PROTOTYPE)
{
    struct descriptor_data *dr;

    CHECKOP(1);
    oper1 = POP();
    if (mlev < LM3)
        abort_interp("M3 prim");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer (1)");
    if (pdescrp(oper1->data.number)) {
        dr = descrdata_by_descr(oper1->data.number);
        ref = dr->player;
    } else
        ref = NOTHING;

    CHECKOFLOW(1);
    CLEAR(oper1);
    PushObject(ref);
}

void
prim_descridle(PRIM_PROTOTYPE)
{
    struct descriptor_data *dr;
    int result;

    CHECKOP(1);
    oper1 = POP();
    if (mlev < LM2)
        abort_interp("M2 prim");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer (1)");
    if (!pdescrp(oper1->data.number))
        abort_interp("That is not a valid descriptor.");
    dr = descrdata_by_descr(oper1->data.number);
    result = (int)time(NULL);
    result = (result - (int)dr->last_time);
    CHECKOFLOW(1);
    CLEAR(oper1);
    PushInt(result);
}

void
prim_descrtime(PRIM_PROTOTYPE)
{
    struct descriptor_data *dr;
    int result;

    CHECKOP(1);
    oper1 = POP();
    if (mlev < LM2)
        abort_interp("M2 prim");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer (1)");
    if (!pdescrp(oper1->data.number))
        abort_interp("That is not a valid descriptor.");
    dr = descrdata_by_descr(oper1->data.number);
    result = (int)time(NULL);
    result = (result - (int)dr->connected_at);
    CLEAR(oper1);
    CHECKOFLOW(1);
    PushInt(result);
}

void
prim_descrhost(PRIM_PROTOTYPE)
{
    struct descriptor_data *dr;

    CHECKOP(1);
    oper1 = POP();
    if (mlev < LARCH)
        abort_interp("Mage prim");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer (1)");
    if (!pdescrp(oper1->data.number))
        abort_interp("That is not a valid descriptor.");
    dr = descrdata_by_descr(oper1->data.number);
    CLEAR(oper1);
    CHECKOFLOW(1);
    PushString(dr->hu->h->name);
}

void
prim_descruser(PRIM_PROTOTYPE)
{
    struct descriptor_data *dr;

    CHECKOP(1);
    oper1 = POP();
    if (mlev < LARCH)
        abort_interp("Arch prim");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer (1)");
    if (!pdescrp(oper1->data.number))
        abort_interp("That is not a valid descriptor.");
    dr = descrdata_by_descr(oper1->data.number);
    CLEAR(oper1);
    CHECKOFLOW(1);
    PushString(dr->hu->u->user);
}

void
prim_descripnum(PRIM_PROTOTYPE)
{
    struct descriptor_data *dr;
    static char ipnum[40];
    const char *p;

    CHECKOP(1);
    oper1 = POP();
    if (mlev < LMAGE)
        abort_interp("Mage prim");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer (1)");
    if (!pdescrp(oper1->data.number))
        abort_interp("That is not a valid descriptor.");
    dr = descrdata_by_descr(oper1->data.number);
    p = hostToIPex(dr->hu->h);
    strcpy(ipnum, p);
    CLEAR(oper1);
    CHECKOFLOW(1);
    PushString((char *) ipnum);
}

void
prim_descrport(PRIM_PROTOTYPE)
{
    struct descriptor_data *dr;
    static char port[40];

    CHECKOP(1);
    oper1 = POP();
    if (mlev < LARCH)
        abort_interp("Arch prim");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer (1)");
    if (!pdescrp(oper1->data.number))
        abort_interp("That is not a valid descriptor.");
    dr = descrdata_by_descr(oper1->data.number);
    sprintf(port, "%d", dr->hu->u->uport);
    CLEAR(oper1);
    CHECKOFLOW(1);
    PushString((char *) port);
}

void
prim_descrconport(PRIM_PROTOTYPE)
{
    struct descriptor_data *dr;

    CHECKOP(1);
    oper1 = POP();
    if (mlev < LARCH)
        abort_interp("Arch prim");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer (1)");
    if (!pdescrp(oper1->data.number))
        abort_interp("That is not a valid descriptor.");
    dr = descrdata_by_descr(oper1->data.number);
    CLEAR(oper1);
    CHECKOFLOW(1);
    PushInt(dr->cport);
}

void
prim_descrleastidle(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper2 = POP();
    if (oper2->type != PROG_OBJECT)
        abort_interp("Player dbref expected (2)");
    ref = oper2->data.objref;
    if (!valid_player(oper2))
        abort_interp("Player dbref expected (2)");
    result = pdescr(least_idle_player_descr(ref));
    CLEAR(oper2);
    CHECKOFLOW(1);
    PushInt(result);
}

void
prim_descrmostidle(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper2 = POP();

    if (oper2->type != PROG_OBJECT)
        abort_interp("Player dbref expected (2)");
    ref = oper2->data.objref;
    if (!valid_player(oper2))
        abort_interp("Player dbref expected (2)");
    result = pdescr(most_idle_player_descr(ref));
    CLEAR(oper1);
    CHECKOFLOW(1);
    PushInt(result);
}

void
prim_descrboot(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    oper1 = POP();

    if (mlev < LARCH)
        abort_interp("Arch prim");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer (1)");
    if (!pdescrp(oper1->data.number))
        abort_interp("That is not a valid descriptor.");

    pdboot(oper1->data.number);
    CLEAR(oper1);
}

void
prim_getdescrinfo(PRIM_PROTOTYPE)
{
    struct inst temp1, temp2;
    stk_array *nw;
    struct descriptor_data *d;
    int curLen = 0;

    CHECKOP(1);
    oper1 = POP();
    if (mlev < LARCH)
        abort_interp("Arch prim");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer (1)");
    if (!pdescrp(oper1->data.number))
        abort_interp("That is not a valid descriptor.");

    d = descrdata_by_descr(oper1->data.number);

    CHECKOFLOW(1);
    nw = new_array_dictionary();
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("DESCRIPTOR");
    temp2.type = PROG_INTEGER;
    temp2.data.number = d->descriptor;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("CONNECTED");
    temp2.type = PROG_INTEGER;
    temp2.data.number = d->connected;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("CON_NUMBER");
    temp2.type = PROG_INTEGER;
    temp2.data.number = d->con_number;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("DESCRIPTOR");
    temp2.type = PROG_INTEGER;
    temp2.data.number = d->descriptor;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("BOOTED");
    temp2.type = PROG_INTEGER;
    temp2.data.number = d->booted;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("FAILS");
    temp2.type = PROG_INTEGER;
    temp2.data.number = d->fails;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("PLAYER");
    temp2.type = PROG_OBJECT;
    temp2.data.objref = d->player;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("OUTPUT_PREFIX");
    temp2.type = PROG_STRING;
    temp2.data.string = alloc_prog_string(d->output_prefix);
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("OUTPUT_SUFFIX");
    temp2.type = PROG_STRING;
    temp2.data.string = alloc_prog_string(d->output_suffix);
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("OUTPUT_LEN");
    temp2.type = PROG_INTEGER;
    temp2.data.number = d->output_len;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("INPUT_LEN");
    temp2.type = PROG_INTEGER;
    temp2.data.number = d->input_len;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("LAST_TIME");
    temp2.type = PROG_INTEGER;
    temp2.data.number = (int) d->last_time;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("CONNECTED_AT");
    temp2.type = PROG_INTEGER;
    temp2.data.number = (int) d->connected_at;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("HOSTADDR");
    temp2.type = PROG_INTEGER;
    temp2.data.number = d->hu->h->a;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("PORT");
    temp2.type = PROG_INTEGER;
    temp2.data.number = d->hu->u->uport;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
	temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("TERMTYPE");
    temp2.type = PROG_STRING;
	if (d->telopt.termtype)
		temp2.data.string = alloc_prog_string(d->telopt.termtype);
	else
		temp2.data.string = alloc_prog_string("<unknown>");
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
#ifdef IPV6
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("IPV6");
    temp2.type = PROG_INTEGER;
    temp2.data.number = (d->flags & DF_IPV6 ? 1 : 0);
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
#endif
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("HOSTNAME");
    temp2.type = PROG_STRING;
    temp2.data.string = alloc_prog_string(d->hu->h->name);
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("USERNAME");
    temp2.type = PROG_STRING;
    temp2.data.string = alloc_prog_string(d->hu->u->user);
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("COMMANDS");
    temp2.type = PROG_INTEGER;
    temp2.data.number = d->commands;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("TYPE");
    temp2.type = PROG_INTEGER;
    temp2.data.number = d->type;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("CONPORT");
    temp2.type = PROG_INTEGER;
    temp2.data.number = d->cport;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("OUTPUTQUEUE");
    temp2.type = PROG_INTEGER;
    curLen = (int) d->output.lines;
    temp2.data.number = curLen;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("WIDTH");
    temp2.type = PROG_INTEGER;
    curLen = (int) d->telopt.width;
    temp2.data.number = curLen;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("HEIGHT");
    temp2.type = PROG_INTEGER;
    curLen = (int) d->telopt.height;
    temp2.data.number = curLen;
    array_setitem(&nw, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);

    PushArrayRaw(nw);
}

void
prim_descrtype(PRIM_PROTOTYPE)
{
    struct descriptor_data *dr;
    static char dtype[16];
    const char *p;
    int x;

    CHECKOP(1);
    oper1 = POP();
    if (mlev < LM2)
        abort_interp("MUCKER level 2 prim.");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer (1)");
    if (!pdescrp(oper1->data.number))
        abort_interp("That is not a valid descriptor.");
    dr = descrdata_by_descr(oper1->data.number);
    x = dr->type;
    if (x == CT_MUCK) p = "MUCK";
    if (x == CT_MUF) p = "MUF";
    if (x == CT_LISTEN) p = "LISTEN";
    if (x == CT_INBOUND) p = "INBOUND";
    if (x == CT_OUTBOUND) p = "OUTBOUND";
    if (x == CT_PUEBLO) p = "PUEBLO";
#ifdef NEWHTTPD
    if (x == CT_HTTP) p = "HTTP";
#endif
#ifdef USE_SSL
    if (x == CT_SSL) p = "SSL";
#endif
    strcpy(dtype, p);
    CLEAR(oper1);
    CHECKOFLOW(1);
    PushString((char *) dtype);
}

void
prim_descr_set(PRIM_PROTOTYPE)
{
    /* Added to enhance our descriptor flag support. 
     * Not as many permission checks possible, so the
     * prim is W3 now for safety sake.
     */
    char *flag;
    int flagValue = 0;
    int descr;
    struct descriptor_data *d;
    int result = 0;

    CHECKOP(2);
    /* descriptr flag -- int */
    oper1 = POP();              /* flag name */
    oper2 = POP();              /* descriptor */
    if (mlev < LARCH)
        abort_interp("descr_set is a W3 prim.");
    if (oper1->type != PROG_STRING)
        abort_interp("Expected a flag name.");
    if (!(oper1->data.string))
        abort_interp("Empty string arguement.");
    if (oper2->type != PROG_INTEGER)
        abort_interp("Expected a descriptor int. (2)");
    if (!pdescrp(oper2->data.number))
        abort_interp("That is not a valid descriptor.");

    descr = oper2->data.number;
    flag = oper1->data.string->data;

    while (*flag == '!') {
        flag++;
        result = (!result);
    }

    if (!*flag)
        abort_interp("Empty flag.");

    flagValue = check_descr_flag(flag);

    if (!flagValue)
        abort_interp("Unrecognized descriptor flag.");
    if (!descr_flag_set_perms(flagValue, mlev, ProgUID))
        abort_interp(tp_noperm_mesg);

    d = get_descr(descr, NOTHING);
    if (!d)                     /* Just to be safe. Shouldn't ever happen anyway. */
        abort_interp("Invalid descriptor.");

    if (!result)
        DR_RAW_ADD_FLAGS(d, flagValue);
    else
        DR_RAW_REM_FLAGS(d, flagValue);

    CLEAR(oper1);
    CLEAR(oper2);
}

void
prim_descr_flagp(PRIM_PROTOTYPE)
{
    /* DESCR_FLAG? added for Proto's new descriptor flag support.
     * Since checking flags is less dangerous, is W1 prim for now. 
     */
    char *flag = NULL;
    int result = 0;

    CHECKOP(2);
    /* descr flag -- int */
    oper1 = POP();              /* flag */
    oper2 = POP();              /* descriptor */

    if (mlev < LMAGE)
        abort_interp("descr_flag? is a W1 prim.");
    if (oper1->type != PROG_STRING)
        abort_interp("Expected a flag name.");
    if (!(oper1->data.string))
        abort_interp("Empty string arguement.");
    if (oper2->type != PROG_INTEGER)
        abort_interp("Expected a descriptor int. (2)");
    if (!pdescrp(oper2->data.number))
        abort_interp("That is not a valid descriptor.");

    flag = oper1->data.string->data;

    result = has_descr_flag(oper2->data.number, flag, mlev);
    if (result == -1)
        abort_interp("Uknown flag.");
    if (result == -2)
        abort_interp("Permission denied.");

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void
prim_bandwidth(PRIM_PROTOTYPE)
{
    /* BANDWIDTH reports the 3 global stats of bytesIn, bytesOut
     * and commandsTotal
     */
    /* -- int int */

    if (mlev < LMAGE)
        abort_interp("bandwidth is a W1 prim.");

    CHECKOFLOW(3);

    PushInt(bytesIn);
    PushInt(bytesOut);
    PushInt(commandTotal);
}

void
prim_descrbufsize(PRIM_PROTOTYPE)
{
    /* int -- int */
    CHECKOP(1);
    oper1 = POP();
    if (mlev < 3)
        abort_interp("Mucker level 3 prim.");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Argument not an integer (1).");

    result = pdescrbufsize(oper1->data.number);
    if (result < 0)
        abort_interp("Invalid descriptor number (1).");

    CLEAR(oper1);
    PushInt(result);
}

#ifdef MCCP_ENABLED
void mccp_start(struct descriptor_data *d, int version);
void mccp_end(struct descriptor_data *d);
#endif

void
prim_mccp_start(PRIM_PROTOTYPE)
{
    struct descriptor_data *d;
    CHECKOP(1);
    oper1 = POP();

    if (mlev < 3)
        abort_interp("Requires Mucker Level 3 or better.");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Integer descriptor number expected.");
    tmp = oper1->data.number;
    CLEAR(oper1);
    if (!(d = descrdata_by_descr(tmp)))
        abort_interp("Invalid descriptor.");
#ifdef MCCP_ENABLED
    mccp_start(d, 2);
#endif
}

void
prim_mccp_end(PRIM_PROTOTYPE)
{
    struct descriptor_data *d;
    CHECKOP(1);
    oper1 = POP();

    if (mlev < LMAGE)
        abort_interp("Requires W1 or better.");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Integer descriptor number expected.");
    tmp = oper1->data.number;
    CLEAR(oper1);
    if (!(d = descrdata_by_descr(tmp)))
        abort_interp("Invalid descriptor.");
#ifdef MCCP_ENABLED
    mccp_end(d);
#endif
}


void
prim_descr_sendfile(PRIM_PROTOTYPE)
{
#if !defined(FILE_PRIMS) || !defined(DESCRFILE_SUPPORT)
    abort_interp("This primitive not compiled in.");
#else /* FILE_PRIMS && DESCRFILE_SUPPORT */
    struct descriptor_data *d;
    char *filename;

    CHECKOP(4);
    oper1 = POP();              /* (s) file  */
    oper2 = POP();              /* (i) end   */
    oper3 = POP();              /* (i) start */
    oper4 = POP();              /* (i) descr */

    if (oper1->type != PROG_STRING)
        abort_interp("String expected. (4)");
    if (!oper1->data.string)
        abort_interp("Empty string argument (4)");
    if (oper2->type != PROG_INTEGER)
        abort_interp("Integer argument expected. (3)");
    if (oper3->type != PROG_INTEGER)
        abort_interp("Integer argument expected. (2)");
    if (oper4->type != PROG_INTEGER)
        abort_interp("Descriptor integer expected. (1)");
    if (mlev < LBOY)
        abort_interp("W4 prim.");
    if (!(d = descrdata_by_descr(oper4->data.number)))
        abort_interp("Invalid descriptor.");

    filename = oper1->data.string->data;

#ifdef SECURE_FILE_PRIMS
    /* These functions are in p_file.c.  */
    /* Maybe I'll eventually make my own more graceful versions. */
    if (!(valid_name(filename)))
        abort_interp("Invalid file name.");
    if (strchr(filename, '$') == NULL)
        filename = set_directory(filename);
    else
        filename = parse_token(filename);
    if (filename == NULL)
        abort_interp("Invalid shortcut used.");
#endif

    result =
        descr_sendfile(d, oper3->data.number, oper2->data.number, filename,
                       fr->pid);

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    CLEAR(oper4);
    PushInt(result);
#endif /* FILE_PRIMS && DESCRFILE_SUPPORT */
}
