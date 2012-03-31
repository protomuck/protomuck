/* compile using: */
/* gcc -shared -nostartfiles -fPIC -Wall -O2 -g -U_FORTIFY_SOURCE -DHAVE_CONFIG_H -I./inc -o libtest.so libtest.c */
/* don't forget to link against system libraries if protomuck doesn't already include them! */


#include "copyright.h"
#include "config.h"

#include "cgi.h"
#include "defaults.h"
#include "db.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "props.h"
#include "match.h"
#include "mpi.h"
#include "reg.h"
#include "externs.h"
#include "mufevent.h"
#include "interp.h"
#include "newhttp.h"            /* hinoserm */
#include "netresolve.h"         /* hinoserm */

                             /* NAME       DESCRIPTION                             VERSION   DEPENDANCIES */
static struct mod_info minfo = {"libtest", "This module is for testing purposes.", 8,        NULL};

static struct module *mthis;

void __prim_nuku_rp_getstatus_1(PRIM_PROTOTYPE)
{
	char propname[BUFFER_LEN];
    char dir[BUFFER_LEN];
	char cstr[BUFFER_LEN];
	char dstr[BUFFER_LEN];
        char buf[BUFFER_LEN];
        double fresult;


    PropPtr propadr, pptr;
    PropPtr prptr;
	double fout = 0.0;
	signed char debuff = 0;
	char *p;
	int foundit = 0;

	CHECKOP(2);
    oper1 = POP();
	oper2 = POP();
    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument. (1)");
	if (oper2->type != PROG_OBJECT)
        abort_interp("Dbref expected. (2)");
    if (!valid_object(oper2))
		abort_interp("Non-object argument (2)");

	strcpy(buf, DoNullInd(oper1->data.string));

	for (p = buf; *p; p++) {
		if (*p == '/') {
			foundit = 1;
			break;
		}
	}

	if (foundit) {
		sprintf(buf, "/@rp/ATB/%d/status/%s", oper2->data.objref, DoNullInd(oper1->data.string));

		prptr = get_property(oper2->data.objref, buf);
		if (prptr) {
#ifdef DISKBASE
					
#endif
			switch (PropType(prptr)) {
				case PROP_STRTYP:
					sscanf(PropDataUNCStr(prptr), "%lg", &fresult);
					fout = fresult;
					break;
				case PROP_INTTYP:
					fout = PropDataVal(prptr);
					break;
				case PROP_FLTTYP:
					fout = PropDataFVal(prptr);
					break;
			}
		}
	} else {
		int cstr_len;
                char buf2[BUFFER_LEN];
                int q = 0;
                int stoppit = 0;


        strcpy(buf, DoNullInd(oper1->data.string));
        for (p = buf; *p; p++) {
                if (*p == '`')
                   stoppit = 1;
                else
                   buf2[q++] = *p;
        }
        buf2[q++] = '\0';

                strcpy(buf, buf2);

		sprintf(dir, "/@rp/ATB/%d/status", oper2->data.objref);
		sprintf(cstr, " %s", buf2);
		sprintf(dstr, " %sDebuff", buf2);
		cstr_len = strlen(cstr);
	

		propadr = first_prop(oper2->data.objref, dir, &pptr, propname);
		while (propadr) {
			int len = strlen(propname);

			if (len >= cstr_len) {
				int i;
				foundit = 0;

				for (i = (len-cstr_len); i < len; i++) {
					if (tolower(propname[i]) != tolower(cstr[i-(((len-cstr_len)))])) {
						foundit = 0;
						break;
					} else
						foundit = 1;
				}

				if (!foundit) {
					foundit = 0;
					if (len >= cstr_len+6) {
						for (i = (len-(cstr_len+6)); i < len; i++) {
							if (tolower(propname[i]) != tolower(dstr[i-(((len-(cstr_len+6))))])) {
								foundit = 0;
								break;
							} else
								foundit = 1;
						}
					}

					if (foundit)
						debuff = -1;
					else
						debuff = 0;
				} else
					debuff = 1;
			} else
				debuff = 0;

			if (debuff) {
				sprintf(buf, "%s%c%s", dir, PROPDIR_DELIMITER, propname);
				prptr = get_property(oper2->data.objref, buf);
				if (prptr) {
#ifdef DISKBASE
					propfetch(ref, prptr);
#endif
					if (PropType(prptr) != PROP_DIRTYP) {
						sprintf(buf, "%s%c%s%cPower", dir, PROPDIR_DELIMITER, propname, PROPDIR_DELIMITER);
						prptr = get_property(oper2->data.objref, buf);
						if (prptr) {
#ifdef DISKBASE
							propfetch(ref, prptr);
#endif
							switch (PropType(prptr)) {
								case PROP_STRTYP:
									sscanf(PropDataUNCStr(prptr), "%lg", &fresult);
									fout += fresult * debuff;
									break;
								case PROP_INTTYP:
									fout += PropDataVal(prptr) * debuff;
									break;
								case PROP_FLTTYP:
									fout += PropDataFVal(prptr) * debuff;
									break;
							}
                                                        if (stoppit)
                                                           break;

						}
					}
				}
			}
			propadr = next_prop(pptr, propadr, propname);
		} 
	}

	CLEAR(oper1);
	CLEAR(oper2);
    PushFloat(fout);
}


extern timequeue tqhead;

void __prim_funcprof_array(PRIM_PROTOTYPE)
{
	int i;
	stk_array *nw;
        struct inst temp1;
        struct inst temp2;
        char buf[BUFFER_LEN];

	CHECKOP(1);
    oper1 = POP();
	if (oper1->type != PROG_INTEGER && oper1->type != PROG_OBJECT)
        abort_interp("Dbref or integer expected. (1)");	

	if (oper1->type == PROG_OBJECT) {
		nw = new_array_dictionary();

		for (i = db_top; i-- > 0;) {
			if (i == oper1->data.objref && Typeof(i) == TYPE_PROGRAM && DBFETCH(i)->sp.program.code && DBFETCH(i)->sp.program.fprofile) {
				struct funcprof *fpe = DBFETCH(i)->sp.program.fprofile;

				while (fpe) {
					stk_array *nw2 = new_array_dictionary();

					temp1.type = PROG_STRING;
                    temp1.data.string = alloc_prog_string("lasttotal");
					temp2.type = PROG_FLOAT;
					temp2.data.fnumber = fpe->lasttotal;
					array_setitem(&nw2, &temp1, &temp2);
					CLEAR(&temp1); 
					CLEAR(&temp2); 

					temp1.type = PROG_STRING;
                    temp1.data.string = alloc_prog_string("starttime");
					temp2.type = PROG_FLOAT;
					temp2.data.fnumber = fpe->starttime;
					array_setitem(&nw2, &temp1, &temp2);
					CLEAR(&temp1); 

					CLEAR(&temp2); 

					temp1.type = PROG_STRING;
                    temp1.data.string = alloc_prog_string("totaltime");
					temp2.type = PROG_FLOAT;
					temp2.data.fnumber = fpe->totaltime;
					array_setitem(&nw2, &temp1, &temp2);
					CLEAR(&temp1); 
					CLEAR(&temp2); 

					temp1.type = PROG_STRING;
                    temp1.data.string = alloc_prog_string("usecount");
					temp2.type = PROG_INTEGER;
					temp2.data.number = (int)fpe->usecount;
					array_setitem(&nw2, &temp1, &temp2);
					CLEAR(&temp1); 
					CLEAR(&temp2); 

					temp1.type = PROG_STRING;
                    temp1.data.string = alloc_prog_string("usecount_s");
					temp2.type = PROG_STRING;
					sprintf(buf, "%ld", fpe->usecount);
					temp2.data.string = alloc_prog_string(buf);
					array_setitem(&nw2, &temp1, &temp2);
					CLEAR(&temp1); 
					CLEAR(&temp2); 

					temp1.type = PROG_STRING;
                    temp1.data.string = alloc_prog_string("funcname");
					temp2.type = PROG_STRING;
					temp2.data.string = alloc_prog_string(fpe->funcname);
					array_setitem(&nw2, &temp1, &temp2);
					CLEAR(&temp1); 
					CLEAR(&temp2); 

                    temp1.type = PROG_STRING;
                    temp1.data.string = alloc_prog_string(fpe->funcname);
					temp2.type = PROG_ARRAY;
                    temp2.data.array = nw2;
                    array_setitem(&nw, &temp1, &temp2);
                    CLEAR(&temp1);
                    CLEAR(&temp2);

					fpe = fpe->next;
				}
				break;
			}
		}
	} else if (oper1->type == PROG_INTEGER) {
                int pid = oper1->data.number;
		timequeue ptr = tqhead;

		nw = new_array_packed(0);

    while (ptr) {
        if (ptr->eventnum == pid) {
            if (ptr->typ != TQ_MUF_TYP || ptr->subtyp != TQ_MUF_TIMER) {   
                break;
            }
        }
        ptr = ptr->next;
    }

    if (ptr && (ptr->eventnum == pid) && ptr->fr &&
        (ptr->typ != TQ_MUF_TYP || ptr->subtyp != TQ_MUF_TIMER) && ptr->fr->fprofile) {
				struct funcprof *fpe = ptr->fr->fprofile;
				while (fpe) {
					stk_array *nw2 = new_array_dictionary();

					temp1.type = PROG_STRING;
					temp1.data.string = alloc_prog_string("lasttotal");
					temp2.type = PROG_FLOAT;
					temp2.data.fnumber = fpe->lasttotal;
					array_setitem(&nw2, &temp1, &temp2);
					CLEAR(&temp1); 
					CLEAR(&temp2); 

					temp1.type = PROG_STRING;
					temp1.data.string = alloc_prog_string("starttime");
					temp2.type = PROG_FLOAT;
					temp2.data.fnumber = fpe->starttime;
					array_setitem(&nw2, &temp1, &temp2);
					CLEAR(&temp1); 
					CLEAR(&temp2); 

					temp1.type = PROG_STRING;
					temp1.data.string = alloc_prog_string("totaltime");
					temp2.type = PROG_FLOAT;
					temp2.data.fnumber = fpe->totaltime;
					array_setitem(&nw2, &temp1, &temp2);
					CLEAR(&temp1); 
					CLEAR(&temp2); 

					temp1.type = PROG_STRING;
					temp1.data.string = alloc_prog_string("usecount");
					temp2.type = PROG_INTEGER;
					temp2.data.number = (int)fpe->usecount;
					array_setitem(&nw2, &temp1, &temp2);
					CLEAR(&temp1); 
					CLEAR(&temp2); 

					temp1.type = PROG_STRING;
					temp1.data.string = alloc_prog_string("usecount_s");
					temp2.type = PROG_STRING;
					sprintf(buf, "%ld", fpe->usecount);
					temp2.data.string = alloc_prog_string(buf);
					array_setitem(&nw2, &temp1, &temp2);
					CLEAR(&temp1); 
					CLEAR(&temp2); 

					temp1.type = PROG_STRING;
					temp1.data.string = alloc_prog_string("funcname");
					temp2.type = PROG_STRING;
					temp2.data.string = alloc_prog_string(fpe->funcname);
					array_setitem(&nw2, &temp1, &temp2);
					CLEAR(&temp1); 
					CLEAR(&temp2); 

					temp2.type = PROG_ARRAY;
					temp2.data.array = nw2;
					array_appenditem(&nw, &temp2);
					CLEAR(&temp2);

					fpe = fpe->next;
				}
			}


        }		

	PushArrayRaw(nw);
}


void
__prim_notify_descriptor_nocr(PRIM_PROTOTYPE)
{
    char buf[BUFFER_LEN * 2];

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (mlev < LMAGE)
        abort_interp("Mage primitive.");
    if (oper1->type != PROG_STRING)          
        abort_interp("Non-string argument (2)");
    if (oper2->type != PROG_INTEGER)
        abort_interp("Descriptor integer expected. (1)");
    if (!pdescrp(oper2->data.number)) {
        CLEAR(oper1);
        CLEAR(oper2);
        return;                  
    }
    if (oper1->data.string) {    
        strcpy(buf, oper1->data.string->data);
        notify_descriptor_raw(oper2->data.number, buf, strlen(buf)+1);
    }
    CLEAR(oper1);
    CLEAR(oper2);
}

void __prim_test(PRIM_PROTOTYPE)
{
	log_status("THE CAKE IS: %s\r\n", mthis->info->name);
}

void __prim_mod_runprim(PRIM_PROTOTYPE)
{
	struct module *m = modules;

	CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
    if (mlev < LBOY)
        abort_interp("W4 primitive.");
    if (oper1->type != PROG_STRING)          
        abort_interp("Non-string argument (2)");
	if (oper2->type != PROG_STRING)          
        abort_interp("Non-string argument (1)");

	while (m) {
		if (!string_compare(m->info->name, DoNullInd(oper2->data.string)))
			break;

		m = m->next;
	}

	if (m) {
		int i;
		int result = 0;

		for (i = 0; m->prims[i].name; i++) {
			if (!string_compare(m->prims[i].name, DoNullInd(oper1->data.string))) {
				m->prims[i].func(player, program, mlev, pc, arg, &top, fr, oper1, oper2, oper3, oper4, oper5, oper6);
				result = 1;
				break;
			}

		}

		if (!result)
			abort_interp("Unable to locate primitive");
		
	} else
		abort_interp("Unable to locate module");
}

void cake(void)
{
	log_status("YES PLEASE\r\n");
}


static struct mod_primlist prims[] = {
	                           {"__prim_nuku_rp_getstatus_1",       "RP_GETSTATUS",             NULL, 0, NULL, 0},
							   {"__prim_funcprof_array",            "FUNCPROF_ARRAY",           NULL, 0, NULL, 0},
							   {"__prim_test",                      "TEST",                     NULL, 0, NULL, 0},
							   {"__prim_notify_descriptor_nocr",    "NOTIFY_DESCRIPTOR_NOCR",   NULL, 0, NULL, 0},
	                           {"__prim_mod_runprim",               "MOD_RUNPRIM",              NULL, 0, NULL, 0},
							   {NULL,                               NULL,                       NULL, 0, NULL, 0}
							  };

struct mod_info *__get_module_info(struct module *m)
{
	mthis = m;
	return &minfo;
}

struct mod_primlist *__get_prim_list(void)
{
	return prims;
}
