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

extern struct inst *oper1, *oper2, *oper3, *oper4, *oper5, *oper6;
extern struct inst temp1, temp2, temp3;
extern double fresult;
extern int tmp, result;
extern dbref ref;
extern char buf[BUFFER_LEN];
                             /* NAME       DESCRIPTION                             VERSION   DEPENDANCIES */
static struct mod_info minfo = {"libtest", "This module is for testing purposes.", 8,        NULL};

static struct module *mthis;

#define FILTER_PROP_INT           0x1 /* 1 */
#define FILTER_PROP_FLOAT         0x2 /* 2 */
#define FILTER_PROP_NUMBER        0x3 /* 3 */
#define FILTER_PROP_DBREF         0x4 /* 4 */
#define FILTER_PROP_STRING        0x8 /* 8 */
#define FILTER_PROP_LOCK         0x10 /* 16 */
#define FILTER_PROPNAME_SMATCH   0x20 /* 32 */
#define FILTER_GREATER           0x40 /* 64 */
#define FILTER_LESSER            0x80 /* 128 */
#define FILTER_EQUAL            0x100 /* 256 */
#define FILTER_NOTEQUAL         0x200 /* 512 */
#define FILTER_SMATCH           0x400 /* 1024 */
#define FILTER_CASELESS         0x800 /* 2048 */
#define FILTER_PROP_EXISTS     0x1000 /* 4096 */

extern int prop_read_perms(dbref player, dbref obj, const char *name, int mlev);

void __prim_array_filter_smart(PRIM_PROTOTYPE)
{
	char buf[BUFFER_LEN], buf2[BUFFER_LEN];
    struct inst *in;
    struct inst temp1;
    stk_array *arr;
    stk_array *nu;
    char* pat;
    char* prop;
    const char* ptr;
PropPtr prptr;
	int fflags;
	int val_int = 0;
	double val_flt = 0.0;
	dbref val_ref = 0;
	char val_str[BUFFER_LEN];
	char *temp;
	bool isyes = 0;

	strcpy(val_str, "");
	
	CHECKOP(4);
    oper1 = POP(); /* x1 */
	oper2 = POP(); /* s1 - propname */
	oper3 = POP(); /* i1 - flags */
	oper4 = POP(); /* a1 - array */
	
	if (oper4->type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");
    if (!array_is_homogenous(oper4->data.array, PROG_OBJECT))
        abort_interp("Argument not an array of dbrefs. (1)");
    if (oper2->type != PROG_STRING || !oper2->data.string)
        abort_interp("Argument not a non-null string. (3)");
    if (oper3->type != PROG_INTEGER)
        abort_interp("Argument not an integer. (2)");

	fflags = oper3->data.number;

	if (fflags & FILTER_PROPNAME_SMATCH)
		abort_interp("FILTER_PROPNAME_SMATCH is not currently supported");
	if (fflags & FILTER_PROP_LOCK)
		abort_interp("FILTER_PROP_LOCK is not currently supported");

	if (!(fflags & FILTER_PROP_EXISTS)) {
		switch (oper1->type) {
			case PROG_STRING:
				if (oper1->data.string) {
					strcpy(val_str, oper1->data.string->data);
					val_int = atoi(val_str);
					val_ref = (dbref)atoi(val_str);
					if (ifloat(val_str))
						sscanf(val_str, "%lg", &val_flt);
				}
				break;
			case PROG_INTEGER:
				val_int = oper1->data.number;
				val_ref = oper1->data.number;
				val_flt = oper1->data.number;
				sprintf(val_str, "%d", oper1->data.number);
				break;
			case PROG_FLOAT:
				val_int = oper1->data.fnumber;
				val_ref = oper1->data.fnumber;
				val_flt = oper1->data.fnumber;
				sprintf(val_str, "%#.15g", oper1->data.fnumber);
				break;
			case PROG_OBJECT:
				val_int = oper1->data.objref;
				val_ref = oper1->data.objref;
				val_flt = oper1->data.objref;
				sprintf(val_str, "#%d", oper1->data.objref);
				break;
			default:
				abort_interp("Unmatchable type specified (4)");
				break;
		}
	}

    ptr = oper2->data.string->data;
    while ((ptr = index(ptr, PROPDIR_DELIMITER)))
        if (!(*(++ptr)))
            abort_interp("Cannot access a propdir directly.");


    nu = new_array_packed(0);
    arr = oper4->data.array;
    prop = (char *) DoNullInd(oper2->data.string);

    if (array_first(arr, &temp1)) {
        do {
            in = array_getitem(arr, &temp1);
			isyes = 0;
            if (valid_object(in)) {
                ref = in->data.objref;
                CHECKREMOTE(ref);
                if (prop_read_perms(ProgUID, ref, prop, mlev)) {
					prptr = get_property(ref, prop);
					if (prptr) {
						if (fflags & FILTER_PROP_EXISTS) {
							isyes = 1;
						} else {
#ifdef DISKBASE
							propfetch(oper2->data.objref, prptr);
#endif
							switch(PropType(prptr)) {
								case PROP_STRTYP:
									if (fflags & FILTER_PROP_STRING) {
										temp = (char *)PropDataUNCStr(prptr);
										if (fflags & FILTER_SMATCH)
											if (equalstr(val_str, temp))
												isyes = 1;

										if (fflags & FILTER_CASELESS) {
											result = string_compare(temp, val_str);
										} else {
											result = strcmp(temp, val_str);
										}

										if (fflags & FILTER_EQUAL && !result)
											isyes = 1;

										if (fflags & FILTER_GREATER && result > 0)
											isyes = 1;

										if (fflags & FILTER_LESSER && result < 0)
											isyes = 1;

										if (fflags & FILTER_NOTEQUAL && result)
											isyes = 1;
									}
									break;
								case PROP_INTTYP:
									 if (fflags & FILTER_PROP_INT) {
										result = PropDataVal(prptr);

										if (fflags & FILTER_EQUAL && (result == val_int))
											isyes = 1;

										if (fflags & FILTER_GREATER && (result > val_int))
											isyes = 1;

										if (fflags & FILTER_LESSER && (result < val_int))
											isyes = 1;

										if (fflags & FILTER_NOTEQUAL && (result != val_int))
											isyes = 1;	 
									 }
									 break;
								case PROP_FLTTYP:
									if (fflags & FILTER_PROP_FLOAT) {
										fresult = PropDataFVal(prptr);

										if (fflags & FILTER_EQUAL && (fresult == val_flt))
											isyes = 1;

										if (fflags & FILTER_GREATER && (fresult > val_flt))
											isyes = 1;

										if (fflags & FILTER_LESSER && (fresult < val_flt))
											isyes = 1;

										if (fflags & FILTER_NOTEQUAL && (fresult != val_flt))
											isyes = 1;
									 }
									 break;
								case PROP_REFTYP:
									if (fflags & FILTER_PROP_FLOAT) {
										ref = PropDataRef(prptr);

										if (fflags & FILTER_EQUAL && (ref == val_ref))
											isyes = 1;

										if (fflags & FILTER_GREATER && (ref > val_ref))
											isyes = 1;

										if (fflags & FILTER_LESSER && (ref < val_ref))
											isyes = 1;

										if (fflags & FILTER_NOTEQUAL && (ref != val_ref))
											isyes = 1;
									}
									break;
								default:
									break;
							}
						}

						if (isyes)
							array_appenditem(&nu, in);
					}
                }
            }
        } while (array_next(arr, &temp1));
    }
	CLEAR(oper4);
    CLEAR(oper3);
    CLEAR(oper2);
    CLEAR(oper1);
    PushArrayRaw(nu);
}

void __prim_nuku_rp_getstatus_1(PRIM_PROTOTYPE)
{
	char propname[BUFFER_LEN];
    char dir[BUFFER_LEN];
	char cstr[BUFFER_LEN];
	char dstr[BUFFER_LEN];

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
		result = 0;

		for (i = 0; m->prims[i].name; i++) {
			if (!string_compare(m->prims[i].name, DoNullInd(oper1->data.string))) {
				m->prims[i].func(player, program, mlev, pc, arg, &tmp, fr);
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
                               {"__prim_array_filter_smart",        "ARRAY_FILTER_SMART",       NULL, 0, NULL, 0},
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
