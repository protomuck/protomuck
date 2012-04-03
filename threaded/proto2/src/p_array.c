/* Primitives Package */

#include "copyright.h"
#include "config.h"

#include "db.h"
#include "tune.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "strings.h"
#include "interp.h"
#include "props.h"

extern int prop_read_perms(dbref player, dbref obj, const char *name, int mlev);
extern int prop_write_perms(dbref player, dbref obj, const char *name,
                            int mlev);

void
prim_array_make(PRIM_PROTOTYPE)
{
	struct inst temp1;
    stk_array *nw;
    int i, result;

    /* integer - stackrange count */
    if (oper[0].type != PROG_INTEGER)
        abort_interp("Invalid item count.");
    result = oper[0].data.number;
    if (result < 0)
        abort_interp("Item count must be non-negative.");

    if (*top < result)
        abort_interp("Stack underflow.");

    temp1.type = PROG_INTEGER;
    temp1.line = 0;
    nw = new_array_packed(result);

    for (i = result; i-- > 0;) {
		struct inst *oper1;
        CHECKOP(1);
        temp1.data.number = i;
        oper1 = POP();
        array_setitem(&nw, &temp1, oper1);
        CLEAR(oper1);
    }

    PushArrayRaw(nw);
}



void
prim_array_make_dict(PRIM_PROTOTYPE)
{
    stk_array *nw;
    int i, result;

    /* integer - stackrange count */
    if (oper[0].type != PROG_INTEGER)
        abort_interp("Invalid item count.");
    result = oper[0].data.number;
    if (result < 0)
        abort_interp("Item count must be positive.");

    if (*top < (2 * result))
        abort_interp("Stack underflow.");

    nw = new_array_dictionary();
    for (i = result; i-- > 0;) {
        struct inst *oper2;
        struct inst *oper1;
        CHECKOP(2);
        oper1 = POP();          /* val */
        oper2 = POP();          /* key */
        if (oper2->type != PROG_INTEGER && oper2->type != PROG_STRING) {
            array_free(nw);
            abort_interp("Keys must be integers or strings.");
        }
        array_setitem(&nw, oper2, oper1);
        CLEAR(oper1);
        CLEAR(oper2);
    }

    PushArrayRaw(nw);
}

void
prim_array_explode(PRIM_PROTOTYPE)
{
    stk_array *arr;
	struct inst temp1, temp2;
	int result;

    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array.");
    copyinst(&oper[0], &temp2);
    arr = temp2.data.array;
    result = array_count(arr);
    CHECKOFLOW(((2 * result) + 1));

    if (array_first(arr, &temp1)) {
        do {
            struct inst *oper2;
            copyinst(&temp1, &arg[((*top)++)]);
            oper2 = array_getitem(arr, &temp1);
            copyinst(oper2, &arg[((*top)++)]);
        } while (array_next(arr, &temp1));
    }

    CLEAR(&temp2);
    PushInt(result);
}



void
prim_array_vals(PRIM_PROTOTYPE)
{
    stk_array *arr;
	struct inst temp1, temp2;
	int result;

    /* array */
    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array.");
    copyinst(&oper[0], &temp2);
    arr = temp2.data.array;
    result = array_count(arr);
    CHECKOFLOW((result + 1));

    if (array_first(arr, &temp1)) {
        do {
            struct inst *oper2;
            oper2 = array_getitem(arr, &temp1);
            copyinst(oper2, &arg[((*top)++)]);
        } while (array_next(arr, &temp1));
    }

    CLEAR(&temp2);
    PushInt(result);
}



void
prim_array_keys(PRIM_PROTOTYPE)
{
    stk_array *arr;
    struct inst temp1, temp2;
	int result;

    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array.");
    copyinst(&oper[0], &temp2);
    arr = temp2.data.array;
    result = array_count(arr);
    CHECKOFLOW((result + 1));

    if (array_first(arr, &temp1)) {
        do {
            copyinst(&temp1, &arg[((*top)++)]);
        } while (array_next(arr, &temp1));
    }

    CLEAR(&temp2);
    PushInt(result);
}



void
prim_array_count(PRIM_PROTOTYPE)
{
	int result;

    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array.");
    result = array_count(oper[0].data.array);

    PushInt(result);
}



void
prim_array_first(PRIM_PROTOTYPE)
{
	struct inst temp1;
	int result;

    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array.");

    temp1.type = PROG_INTEGER;
    temp1.data.number = 0;
    result = array_first(oper[0].data.array, &temp1);

    if (result) {
        copyinst(&temp1, &arg[((*top)++)]);
    } else {
        result = 0;
        PushInt(result);
    }
    PushInt(result);
}



void
prim_array_last(PRIM_PROTOTYPE)
{
	struct inst temp1;
	int result;

    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array.");

    temp1.type = PROG_INTEGER;
    temp1.data.number = 0;
    result = array_last(oper[0].data.array, &temp1);

    if (result) {
        copyinst(&temp1, &arg[((*top)++)]);
    } else {
        result = 0;
        PushInt(result);
    }
    PushInt(result);
}



void
prim_array_prev(PRIM_PROTOTYPE)
{
	struct inst temp1;
	int result;

    /* ???  previous index */
    /* arr  Array */
    if (oper[0].type != PROG_INTEGER && oper[0].type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array.(1)");

    copyinst(&oper[0], &temp1);
    result = array_prev(oper[1].data.array, &temp1);

    if (result) {
        copyinst(&temp1, &arg[((*top)++)]);
        CLEAR(&temp1);
    } else {
        result = 0;
        PushInt(result);
    }
    PushInt(result);
}

void
prim_array_next(PRIM_PROTOTYPE)
{
	struct inst temp1;
	int result;

/* ???  previous index */
/* arr  Array */
    if (oper[0].type != PROG_INTEGER && oper[0].type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array.(1)");

    copyinst(&oper[0], &temp1);
    result = array_next(oper[1].data.array, &temp1);

    if (result) {
        copyinst(&temp1, &arg[((*top)++)]);
        CLEAR(&temp1);
    } else {
        result = 0;
        PushInt(result);
    }
    PushInt(result);
}



void
prim_array_getitem(PRIM_PROTOTYPE)
{
    struct inst *in;
    struct inst temp;


/* ???  index */
/* arr  Array */
    if (oper[0].type != PROG_INTEGER && oper[0].type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");
    in = array_getitem(oper[1].data.array, &oper[0]);

    /* copy data to a temp inst before clearing the containing array */
    if (in) {
        copyinst(in, &temp);
    } else {
        temp.type = PROG_INTEGER;
        temp.data.number = 0;
    }

    /* copy data to stack, then clear temp inst */
    copyinst(&temp, &arg[(*top)++]);
    CLEAR(&temp);
}

void
prim_array_setitem(PRIM_PROTOTYPE)
{
    stk_array *nw;
	int result;

/* ???  index to store at */
/* arr  Array to store in */
/* ???  item to store     */

    if (oper[0].type != PROG_INTEGER && oper[0].type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    result = array_setitem(&oper[1].data.array, &oper[0], &oper[2]);
    if (result < 0) {
        char tBuf[BUFFER_LEN];

		if (result == -4)
			abort_interp("Argument not an integer. (2)");
		if (result == -5)
			abort_interp("Array index out of range. (2)");

        sprintf(tBuf, "Internal Error Code: %d", result);
        abort_interp(tBuf);
        /* abort_interp("Index out of array bounds. (3)"); */
    }

    nw = oper[1].data.array;
    oper[1].data.array = NULL;

    PushArrayRaw(nw);
}

void
prim_array_appenditem(PRIM_PROTOTYPE)
{
    stk_array *nw;
	int result;

 /* arr  Array to store in */
 /* ???  item to store     */

    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array. (2)");
    if (oper[0].data.array && oper[0].data.array->type != ARRAY_PACKED)
        abort_interp("Argument must be a list type array. (2)");

    result = array_appenditem(&oper[0].data.array, &oper[1]);
    if (result == -1)
        abort_interp("Internal Error!");

    nw = oper[0].data.array;
    oper[0].data.array = NULL;

    PushArrayRaw(nw);
}

void
prim_array_insertitem(PRIM_PROTOTYPE)
{
    stk_array *nw;
	int result;

/* ???  index to store at */
/* arr  Array to store in */
/* ???  item to store     */

    if (oper[0].type != PROG_INTEGER && oper[0].type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    result = array_insertitem(&oper[1].data.array, &oper[0], &oper[2]);
    if (result < 0)
        abort_interp("Index out of array bounds. (3)");

    nw = oper[1].data.array;
    oper[1].data.array = NULL;

    PushArrayRaw(nw);
}



void
prim_array_getrange(PRIM_PROTOTYPE)
{
    stk_array *nw;

/* int  range end item */
/* int  range start item */
/* arr  Array   */
    if (oper[0].type != PROG_INTEGER && oper[0].type != PROG_STRING)
        abort_interp("Argument not an integer or string. (3)");
    if (oper[1].type != PROG_INTEGER && oper[1].type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper[2].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    nw = array_getrange(oper[2].data.array, &oper[1], &oper[0]);
    if (!nw)
        nw = new_array_packed(0);

    PushArrayRaw(nw);
}



void
prim_array_setrange(PRIM_PROTOTYPE)
{
    stk_array *nw;
	int result;

/* arr  array to insert */
/* ???  starting pos for lists */
/* arr  Array   */
    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array. (3)");
    if (oper[1].type != PROG_INTEGER && oper[1].type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper[2].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    result = array_setrange(&oper[2].data.array, &oper[1], oper[0].data.array);
    if (result < 0)
        abort_interp("Index out of array bounds. (2)");

    nw = oper[2].data.array;
    oper[2].data.array = NULL;

    PushArrayRaw(nw);
}


void
prim_array_insertrange(PRIM_PROTOTYPE)
{
    stk_array *nw;
	int result;

/* arr  array to insert */
/* ???  starting pos for lists */
/* arr  Array   */
    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array. (3)");
    if (oper[1].type != PROG_INTEGER && oper[1].type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper[2].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    result = array_insertrange(&oper[2].data.array, &oper[1], oper[0].data.array);
    if (result < 0)
        abort_interp("Index out of array bounds. (2)");

    nw = oper[2].data.array;
    oper[2].data.array = NULL;

    PushArrayRaw(nw);
}

void
prim_array_delitem(PRIM_PROTOTYPE)
{
    stk_array *nw;

/* int  item to delete */
/* arr  Array   */
    if (oper[0].type != PROG_INTEGER && oper[0].type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    array_delitem(&oper[1].data.array, &oper[0]);

    nw = oper[1].data.array;
    oper[1].data.array = NULL;

    PushArrayRaw(nw);
}

void
prim_array_delrange(PRIM_PROTOTYPE)
{
    stk_array *nw;
	int result;
/* int  range end item */
/* int  range start item */
/* arr  Array   */
    if (oper[0].type != PROG_INTEGER && oper[0].type != PROG_STRING)
        abort_interp("Argument not an integer or string. (3)");
    if (oper[1].type != PROG_INTEGER && oper[1].type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper[2].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    result = array_delrange(&oper[2].data.array, &oper[1], &oper[0]);
    if (result < 0)
        abort_interp("Bad array range specified.");

    nw = oper[2].data.array;
    oper[2].data.array = NULL;

    PushArrayRaw(nw);
}

/*\
|*| Eeep!
\*/

void
prim_array_n_union(PRIM_PROTOTYPE)
{
    stk_array *new_union;
    stk_array *new_mash;
    int num_arrays, result;

/* integer - stackrange count */
    if (oper[0].type != PROG_INTEGER)
        abort_interp("Invalid item count.");
    result = oper[0].data.number;
    if (result < 0)
        abort_interp("Item count must be positive.");

    if (*top < result)
        abort_interp("Stack underflow.");

    if (result > 0) {
        new_mash = new_array_dictionary();
        for (num_arrays = 0; num_arrays < result; num_arrays++) {
            struct inst *oper1;
            
            CHECKOP(1);
            oper1 = POP();
            if (oper1->type != PROG_ARRAY) {
                array_free(new_mash);
                abort_interp("Arguement not an array.");
            }
            array_mash(oper1->data.array, &new_mash, 1);
            CLEAR(oper1);
        }
        new_union = array_demote_only(new_mash, 1);
        array_free(new_mash);
    } else {
        new_union = new_array_packed(0);
    }

    PushArrayRaw(new_union);
}

void
prim_array_n_intersection(PRIM_PROTOTYPE)
{
	stk_array *new_union;
	stk_array *new_mash;
	stk_array *temp_mash;
	int num_arrays;
	int result;

	if (oper[0].type != PROG_INTEGER)
		abort_interp("Invalid item count.");
	result = oper[0].data.number;
	if (result < 0)
		abort_interp("Item count must be positive.");

	if (*top < result)
		abort_interp("Stack underflow.");

	if (result > 0) {
		new_mash = new_array_dictionary();
		for (num_arrays = 0; num_arrays < result; num_arrays++) {
            struct inst *oper1;

			oper1 = POP();
			if (oper1->type != PROG_ARRAY) {
				array_free(new_mash);
				abort_interp("Argument not an array.");
			}
			temp_mash = new_array_dictionary();
			array_mash(oper1->data.array, &temp_mash, 1);
			CLEAR(oper1);
			new_union = array_demote_only(temp_mash, 1);
			array_free(temp_mash);
			PushArrayRaw(new_union);
			oper1 = POP();
			array_mash(oper1->data.array, &new_mash, 1);
			CLEAR(oper1);
		}
		new_union = array_demote_only(new_mash, result);
		array_free(new_mash);
	} else {
		new_union = new_array_packed(0);
	}

	PushArrayRaw(new_union);
}

void
prim_array_n_difference(PRIM_PROTOTYPE)
{
    stk_array *new_union;
    stk_array *new_mash;
    int num_arrays, result;

    if (oper[0].type != PROG_INTEGER)
        abort_interp("Invalid item count.");
    result = oper[0].data.number;
    if (result < 0)
        abort_interp("Item count must be positive.");

    if (*top < result)
        abort_interp("Stack underflow.");

    if (result > 0) {
        struct inst *oper1;

        new_mash = new_array_dictionary();

        oper1 = POP();
        if (oper1->type != PROG_ARRAY) {
            array_free(new_mash);
            abort_interp("Arguement not an array.");
        }
        array_mash(oper1->data.array, &new_mash, 1);
        CLEAR(oper1);

        for (num_arrays = 1; num_arrays < result; num_arrays++) {
            CHECKOP(1);
            oper1 = POP();
            if (oper1->type != PROG_ARRAY) {
                array_free(new_mash);
                abort_interp("Arguement not an array.");
            }
            array_mash(oper1->data.array, &new_mash, -1);
            CLEAR(oper1);
        }
        new_union = array_demote_only(new_mash, 1);
        array_free(new_mash);
    } else {
        new_union = new_array_packed(0);
    }

    PushArrayRaw(new_union);
}

void
prim_array_notify(PRIM_PROTOTYPE)
{
    stk_array *strarr;
    stk_array *refarr;
    struct inst temp1, temp2;
	char buf[BUFFER_LEN];

    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array of strings. (1)");
    if (!array_is_homogenous(oper[1].data.array, PROG_STRING))
        abort_interp("Argument not an array of strings. (1)");
    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array of dbrefs. (2)");
    if (!array_is_homogenous(oper[0].data.array, PROG_OBJECT))
        abort_interp("Argument not an array of dbrefs. (2)");
    strarr = oper[1].data.array;
    refarr = oper[0].data.array;

    if (array_first(strarr, &temp2)) {
        struct inst *oper3, *oper4;
        do {
            oper4 = array_getitem(strarr, &temp2);
            strcpy(buf, DoNullInd(oper4->data.string));

            if (tp_m1_name_notify && mlev < 2) {
                if ( !tp_mush_format_escapes )
                    prefix_message(buf, DoNullInd(oper4->data.string),
                                   NAME(PSafe), BUFFER_LEN, 1);
                else
                    prefix_message_mush(buf, DoNullInd(oper4->data.string),
                                        NAME(PSafe), BUFFER_LEN, 1);
            } else {
                if ( !tp_mush_format_escapes )
                    strcpy(buf, DoNullInd(oper4->data.string));
                else
                    strcpy_mush(buf, DoNullInd(oper4->data.string));
                }

            if (array_first(refarr, &temp1)) {
                do {
                    oper3 = array_getitem(refarr, &temp1);
                    if (valid_object(oper3))
                        notify_listeners(fr->descr, PSafe,
                                         program, oper3->data.objref,
                                         getloc(oper3->data.objref), buf, 1);
                } while (array_next(refarr, &temp1));
            }
        } while (array_next(strarr, &temp2));
    }
}

void
prim_array_ansi_notify(PRIM_PROTOTYPE)
{
    stk_array *strarr;
    stk_array *refarr;
    struct inst temp1, temp2;
    char buf2[BUFFER_LEN * 2];
	char buf[BUFFER_LEN];

    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array of strings. (1)");
    if (!array_is_homogenous(oper[1].data.array, PROG_STRING))
        abort_interp("Argument not an array of strings. (1)");
    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array of dbrefs. (2)");
    if (!array_is_homogenous(oper[0].data.array, PROG_OBJECT))
        abort_interp("Argument not an array of dbrefs. (2)");
    strarr = oper[1].data.array;
    refarr = oper[0].data.array;

    if (array_first(strarr, &temp2)) {
        struct inst *oper3, *oper4;

        do {
            oper4 = array_getitem(strarr, &temp2);
            strcpy(buf, DoNullInd(oper4->data.string));

            if (tp_m1_name_notify && mlev < 2) {
                if ( !tp_mush_format_escapes )
                    strcpy(buf2, NAME(PSafe));
                else
                    strcpy_mush(buf2, NAME(PSafe));
                strcat(buf2, " ");
                if (!string_prefix(buf, buf2)) {
                    strcat(buf2, buf);
                    buf2[BUFFER_LEN - 1] = '\0';
                    strcpy(buf, buf2);
                }
            }
            if (array_first(refarr, &temp1)) {
                do {
                    oper3 = array_getitem(refarr, &temp1);
                    if (valid_object(oper3))
                        ansi_notify_listeners(fr->descr,
                                              PSafe, program,
                                              oper3->data.objref,
                                              getloc(oper3->data.objref), buf,
                                              1);

                } while (array_next(refarr, &temp1));
            }
        } while (array_next(strarr, &temp2));
    }
}



void
prim_array_notify_html(PRIM_PROTOTYPE)
{
    stk_array *strarr;
    stk_array *refarr;
    struct inst temp1, temp2;
    char buf2[BUFFER_LEN * 2];
	char buf[BUFFER_LEN];

    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array of strings. (1)");
    if (!array_is_homogenous(oper[1].data.array, PROG_STRING))
        abort_interp("Argument not an array of strings. (1)");
    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array of dbrefs. (2)");
    if (!array_is_homogenous(oper[0].data.array, PROG_OBJECT))
        abort_interp("Argument not an array of dbrefs. (2)");
    strarr = oper[1].data.array;
    refarr = oper[0].data.array;

    if (array_first(strarr, &temp2)) {
        struct inst *oper3, *oper4;

        do {
            oper4 = array_getitem(strarr, &temp2);
            if ( !tp_mush_format_escapes )
                strcpy(buf, DoNullInd(oper4->data.string));
            else
                strcpy_mush(buf, DoNullInd(oper4->data.string));
            if (tp_m1_name_notify && mlev < 2) {
                strcpy(buf2, NAME(PSafe));
                strcat(buf2, " ");
                if (!string_prefix(buf, buf2)) {
                    strcat(buf2, buf);
                    buf2[BUFFER_LEN - 1] = '\0';
                    strcpy(buf, buf2);
                }
            }
            if (array_first(refarr, &temp1)) {
                do {
                    oper3 = array_getitem(refarr, &temp1);

                    notify_html_listeners(fr->descr, PSafe, program,
                                          oper3->data.objref,
                                          getloc(oper3->data.objref), buf, 1);
                } while (array_next(refarr, &temp1));
            }
        } while (array_next(strarr, &temp2));
    }
}



void
prim_array_reverse(PRIM_PROTOTYPE)
{
    stk_array *arr;
    stk_array *nw;
    int i, result;
	struct inst temp1, temp2;

    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array.");
    arr = oper[0].data.array;
    if (arr->type != ARRAY_PACKED)
        abort_interp("Argument must be a list type array.");

    result = array_count(arr);
    nw = new_array_packed(result);

    temp1.type = PROG_INTEGER;
    temp2.type = PROG_INTEGER;
    for (i = 0; i < result; i++) {
        temp1.data.number = i;
        temp2.data.number = (result - i) - 1;
        array_setitem(&nw, &temp1, array_getitem(arr, &temp2));
    }

    PushArrayRaw(nw);
}

static int sortflag_caseinsens = 0;
static int sortflag_descending = 0;
static int sortflag_natural = 0;
static struct inst *sortflag_index = NULL;

int
sortcomp_generic(const void *x, const void *y)
{
    struct inst *a;
    struct inst *b;

    if (!sortflag_descending) {
        a = *(struct inst **) x;
        b = *(struct inst **) y;
    } else {
        a = *(struct inst **) y;
        b = *(struct inst **) x;
    }
    if (sortflag_index) {
        /* This should only be set if comparators are all arrays. */
        a = array_getitem(a->data.array, sortflag_index);
        b = array_getitem(b->data.array, sortflag_index);
        if (!a && !b) {
            return 0;
        } else if (!a) {
            return -1;
        } else if (!b) {
            return 1;
        }
    }
    return (array_idxcmp_case(a, b, (sortflag_caseinsens ? 0 : 1), 1, sortflag_natural));
}


int
sortcomp_shuffle(const void *x, const void *y)
{
    return (((RANDOM() >> 8) % 5) - 2);
}


/* Sort types:
 * 1: case, ascending
 * 2: nocase, ascending
 * 3: case, descending
 * 4: nocase, descending
 * 5: randomize
 */
void
prim_array_sort(PRIM_PROTOTYPE)
{
    stk_array *arr;
    stk_array *nu;
    int count, i;
    int (*comparator) (const void *, const void *);
    struct inst **tmparr = NULL;
	struct inst temp1;

/* int  sort_type   */
/* arr  Array   */
    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");
    arr = oper[1].data.array;
    if (arr->type != ARRAY_PACKED)
        abort_interp("Argument must be a list type array. (1)");

    if (oper[0].type != PROG_INTEGER)
        abort_interp("Expected integer argument for sort type. (2)");

    temp1.type = PROG_INTEGER;
    count = array_count(arr);
    nu = new_array_packed(count);
    tmparr = (struct inst **) malloc(count * sizeof(struct inst *));

    for (i = 0; i < count; i++) {
        temp1.data.number = i;
        tmparr[i] = array_getitem(arr, &temp1);
    }

    sortflag_caseinsens = (oper[0].data.number & SORTTYPE_CASEINSENS) ? 1 : 0;
    sortflag_descending = (oper[0].data.number & SORTTYPE_DESCENDING) ? 1 : 0;
    sortflag_natural    = (oper[0].data.number & SORTTYPE_NATURAL)    ? 1 : 0;
    sortflag_index = NULL;

    if ((oper[0].data.number & SORTTYPE_SHUFFLE)) {
        comparator = sortcomp_shuffle;
    } else {
        comparator = sortcomp_generic;
    }

    qsort(tmparr, count, sizeof(struct inst *), comparator);
    /* WORK: if we go multithreaded, the mutex should be released here. */
    /*       Share this mutex with ARRAY_SORT_INDEXED. */

    for (i = 0; i < count; i++) {
        temp1.data.number = i;
        array_setitem(&nu, &temp1, tmparr[i]);
    }
    free(tmparr);
    PushArrayRaw(nu);
}

/* Sort types:
 ** 1: case, ascending
 ** 2: nocase, ascending
 ** 3: case, descending
 ** 4: nocase, descending
 ** 5: randomize
 **/
void
prim_array_sort_indexed(PRIM_PROTOTYPE)
{
    stk_array *arr;
    stk_array *nu;
    int count, i;
    int (*comparator) (const void *, const void *);
    struct inst **tmparr = NULL;
	struct inst temp1;

/* idx  index_key   */
/* int  sort_type   */
/* arr  Array   */
    if (oper[2].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");
    arr = oper[2].data.array;
    if (arr->type != ARRAY_PACKED)
        abort_interp("Argument must be a list type array. (1)");
    if (!array_is_homogenous(arr, PROG_ARRAY))
        abort_interp("Argument must be a list array of arrays. (1)");

    if (oper[1].type != PROG_INTEGER)
        abort_interp("Expected integer argument for sort type. (2)");

    if (oper[0].type != PROG_INTEGER && oper[0].type != PROG_STRING)
        abort_interp("Index argument not an integer or string. (3)");

    temp1.type = PROG_INTEGER;
    count = array_count(arr);
    nu = new_array_packed(count);
    tmparr = (struct inst **) malloc(count * sizeof(struct inst *));

    for (i = 0; i < count; i++) {
        temp1.data.number = i;
        tmparr[i] = array_getitem(arr, &temp1);
    }

    sortflag_caseinsens = (oper[1].data.number & SORTTYPE_CASEINSENS) ? 1 : 0;
    sortflag_descending = (oper[1].data.number & SORTTYPE_DESCENDING) ? 1 : 0;
    sortflag_natural    = (oper[1].data.number & SORTTYPE_NATURAL)    ? 1 : 0;
    sortflag_index = &oper[0];

    if ((oper[1].data.number & SORTTYPE_SHUFFLE)) {
        comparator = sortcomp_shuffle;
    } else {
        comparator = sortcomp_generic;
    }

    qsort(tmparr, count, sizeof(struct inst *), comparator);
    /* WORK: if we go multithreaded, the mutex should be released here. */
    /*       Share this mutex with ARRAY_SORT. */

    for (i = 0; i < count; i++) {
        temp1.data.number = i;
        array_setitem(&nu, &temp1, tmparr[i]);
    }
    free(tmparr);
    PushArrayRaw(nu);
}

void
prim_array_get_propdirs(PRIM_PROTOTYPE)
{
    stk_array *nw;
    char propname[BUFFER_LEN];
    char dir[BUFFER_LEN];
    PropPtr propadr, pptr;
    PropPtr prptr;
    int count = 0;
    int len = 0;
	dbref ref;
	char buf[BUFFER_LEN];
	struct inst temp1, temp2;

    /* dbref strPropDir -- array */
    if (oper[1].type != PROG_OBJECT)
        abort_interp("Dbref required. (1)");
    if (!valid_object(&oper[1]))
        abort_interp("Invalid dbref. (1)");
    if (oper[0].type != PROG_STRING)
        abort_interp("String required. (2)");

    ref = oper[1].data.objref;
    strcpy(dir, DoNullInd(oper[0].data.string));
    if (!*dir)
        strcpy(dir, "/");

    len = strlen(dir) - 1;
    if (len > 0 && dir[len] == PROPDIR_DELIMITER)
        dir[len] = '\0';

    nw = new_array_packed(0);
    propadr = first_prop(ref, dir, &pptr, propname);
    while (propadr) {
        sprintf(buf, "%s%c%s", dir, PROPDIR_DELIMITER, propname);
        if (prop_read_perms(ProgUID, ref, buf, mlev)) {
            prptr = get_property(ref, buf);
            if (prptr) {
#ifdef DISKBASE
                propfetch(ref, prptr);
#endif
                if (PropDir(prptr)) {
                    temp2.type = PROG_STRING;
                    temp2.data.string = alloc_prog_string(propname);
                    temp1.type = PROG_INTEGER;
                    temp1.data.number = count++;
                    array_setitem(&nw, &temp1, &temp2);
                    CLEAR(&temp1);
                    CLEAR(&temp2);
                }
            }
        }
        propadr = next_prop(pptr, propadr, propname);
    }

    PushArrayRaw(nw);
}


void
prim_array_get_propvals(PRIM_PROTOTYPE)
{
    stk_array *nw;
    char propname[BUFFER_LEN];
    char dir[BUFFER_LEN];
    PropPtr propadr, pptr;
    PropPtr prptr;
	dbref ref;
	struct inst temp1, temp2;
	char buf[BUFFER_LEN];

    /* dbref strPropDir -- array */

    if (oper[1].type != PROG_OBJECT)
        abort_interp("Dbref required. (1)");
    if (!valid_object(&oper[1]))
        abort_interp("Invalid dbref. (1)");
    if (oper[0].type != PROG_STRING)
        abort_interp("String required. (2)");

    ref = oper[1].data.objref;
    strcpy(dir, DoNullInd(oper[0].data.string));

    if (!*dir)
        strcpy(dir, "/");

    nw = new_array_dictionary();
    propadr = first_prop(ref, dir, &pptr, propname);
    while (propadr) {
        sprintf(buf, "%s%c%s", dir, PROPDIR_DELIMITER, propname);
        if (prop_read_perms(ProgUID, ref, buf, mlev)) {
            prptr = get_property(ref, buf);
            if (prptr) {
                int goodflag = 1;

#ifdef DISKBASE
                propfetch(ref, prptr);
#endif
                switch (PropType(prptr)) {
                    case PROP_STRTYP:
                        temp2.type = PROG_STRING;
                        temp2.data.string =
                            alloc_prog_string(PropDataUNCStr(prptr));
                        break;
                    case PROP_LOKTYP:
                        temp2.type = PROG_LOCK;
                        if (PropFlags(prptr) & PROP_ISUNLOADED) {
                            temp2.data.lock = TRUE_BOOLEXP;
                        } else {
                            temp2.data.lock = copy_bool(PropDataLok(prptr));
                        }
                        break;
                    case PROP_REFTYP:
                        temp2.type = PROG_OBJECT;
                        temp2.data.number = PropDataRef(prptr);
                        break;
                    case PROP_INTTYP:
                        temp2.type = PROG_INTEGER;
                        temp2.data.number = PropDataVal(prptr);
                        break;
                    case PROP_FLTTYP:
                        temp2.type = PROG_FLOAT;
                        temp2.data.fnumber = PropDataFVal(prptr);
                        break;
                    default:
                        goodflag = 0;
                        break;
                }

                if (goodflag) {
                    temp1.type = PROG_STRING;
                    temp1.data.string = alloc_prog_string(propname);
                    array_setitem(&nw, &temp1, &temp2);
                    CLEAR(&temp1);
                    CLEAR(&temp2);
                }
            }
        }
        propadr = next_prop(pptr, propadr, propname);
    }

    PushArrayRaw(nw);
}


void
prim_array_get_proplist(PRIM_PROTOTYPE)
{
    stk_array *nw;
    const char *m;
    char propname[BUFFER_LEN];
    char dir[BUFFER_LEN];
    PropPtr prptr;
    int lines = 0, i;
	dbref ref;
	struct inst temp2;

    /* dbref strPropDir -- array */

    if (oper[1].type != PROG_OBJECT)
        abort_interp("Dbref required. (1)");
    if (!valid_object(&oper[1]))
        abort_interp("Invalid dbref. (1)");
    if (oper[0].type != PROG_STRING)
        abort_interp("String required. (2)");

    ref = oper[1].data.objref;
    strcpy(dir, DoNullInd(oper[0].data.string));
    if (!*dir)
        strcpy(dir, "/");

    sprintf(propname, "%s#", dir);
    if (!(lines = get_property_value(ref, propname))) {
        if ((m = get_property_class(ref, propname)))
            lines = atoi(m);

        if (!lines) {
            sprintf(propname, "%s", dir);
            if (!(lines = get_property_value(ref, propname)))
                if ((m = get_property_class(ref, propname)))
                    lines = atoi(m);

        }
        if (!lines) {
            sprintf(propname, "%s%c#", dir, PROPDIR_DELIMITER);
            if (!(lines = get_property_value(ref, propname)))
                if ((m = get_property_class(ref, propname)))
                    lines = atoi(m);
        }

    }

    nw = new_array_packed(0);

    if (lines) {
        for (i = 1; i <= lines; i++) {
            sprintf(propname, "%s#%c%d", dir, PROPDIR_DELIMITER, i);
            if (!(prptr = get_property(ref, propname))) {
                sprintf(propname, "%s%c%d", dir, PROPDIR_DELIMITER, i);
                if (!(prptr = get_property(ref, propname))) {
                    sprintf(propname, "%s%d", dir, i);
                    prptr = get_property(ref, propname);
                }
            }

            if (prop_read_perms(ProgUID, ref, propname, mlev)) {
                if (!prptr) {
                    temp2.type = PROG_STRING;
                    temp2.data.string = NULL;
                } else {
#ifdef DISKBASE
                    propfetch(ref, prptr);
#endif
                    switch (PropType(prptr)) {
                        case PROP_STRTYP:
                            temp2.type = PROG_STRING;
                            temp2.data.string =
                                alloc_prog_string(PropDataUNCStr(prptr));
                            break;
                        case PROP_LOKTYP:
                            temp2.type = PROG_LOCK;
                            if (PropFlags(prptr) & PROP_ISUNLOADED) {
                                temp2.data.lock = TRUE_BOOLEXP;
                            } else {
                                temp2.data.lock = copy_bool(PropDataLok(prptr));
                            }
                            break;
                        case PROP_REFTYP:
                            temp2.type = PROG_OBJECT;
                            temp2.data.number = PropDataRef(prptr);
                            break;
                        case PROP_INTTYP:
                            temp2.type = PROG_INTEGER;
                            temp2.data.number = PropDataVal(prptr);
                            break;
                        case PROP_FLTTYP:
                            temp2.type = PROG_FLOAT;
                            temp2.data.fnumber = PropDataFVal(prptr);
                            break;
                        default:
                            temp2.type = PROG_STRING;
                            temp2.data.string = NULL;
                            break;
                    }
                }
                array_appenditem(&nw, &temp2);
                CLEAR(&temp2);
            }
        }
    }

    PushArrayRaw(nw);
}

void
prim_array_put_propvals(PRIM_PROTOTYPE)
{
    char propname[BUFFER_LEN];
    stk_array *arr;
    PData pdat;
	dbref ref;
	struct inst temp1;

    /* dbref strPropDir array -- */

    if (oper[2].type != PROG_OBJECT)
        abort_interp("Dbref required. (1)");
    if (!valid_object(&oper[2]))
        abort_interp("Invalid dbref. (1)");
    if (oper[1].type != PROG_STRING)
        abort_interp("String required. (2)");
    if (oper[0].type != PROG_ARRAY)
        abort_interp("Array required. (3)");

    ref = oper[2].data.objref;
    arr = oper[0].data.array;

    if (array_first(arr, &temp1)) {
        struct inst *oper4;

        do {
            oper4 = array_getitem(arr, &temp1);
            switch (temp1.type) {
                case PROG_STRING:
                    sprintf(propname, "%s%c%s", DoNullInd(oper[1].data.string),
                            PROPDIR_DELIMITER, DoNullInd(temp1.data.string));
                    break;
                case PROG_INTEGER:
                    sprintf(propname, "%s%c%d", DoNullInd(oper[1].data.string),
                            PROPDIR_DELIMITER, temp1.data.number);
                    break;
                case PROG_FLOAT:
                    sprintf(propname, "%s%c%.15g",
                            DoNullInd(oper[1].data.string), PROPDIR_DELIMITER,
                            temp1.data.fnumber);
                    break;
                default:
                    *propname = '\0';
            }

            if (!prop_write_perms(ProgUID, ref, propname, mlev))
                abort_interp
                    ("Permission denied while trying to set protected property.");

            switch (oper4->type) {
                case PROG_STRING:
                    pdat.flags = PROP_STRTYP;
                    pdat.data.str =
                        oper4->data.string ? oper4->data.string->data : NULL;
                    break;
                case PROG_INTEGER:
                    pdat.flags = PROP_INTTYP;
                    pdat.data.val = oper4->data.number;
                    break;
                case PROG_FLOAT:
                    pdat.flags = PROP_FLTTYP;
                    pdat.data.fval = oper4->data.fnumber;
                    break;
                case PROG_OBJECT:
                    pdat.flags = PROP_REFTYP;
                    pdat.data.ref = oper4->data.objref;
                    break;
                case PROG_LOCK:
                    pdat.flags = PROP_LOKTYP;
                    pdat.data.lok = copy_bool(oper4->data.lock);
                    break;
            }
            set_property(ref, propname, &pdat);
        } while (array_next(arr, &temp1));
    }
}


void
prim_array_put_proplist(PRIM_PROTOTYPE)
{
    stk_array *arr;
    char propname[BUFFER_LEN];
    char dir[BUFFER_LEN];
    const char *fmtin;
    char *fmtout;
    int dirlen;
    int count;
    PData pdat;
	dbref ref;
	struct inst temp1;
	char buf[BUFFER_LEN];
	
	/* dbref strPropDir array -- */

    if (oper[2].type != PROG_OBJECT)
        abort_interp("Dbref required. (1)");
    if (!valid_object(&oper[2]))
        abort_interp("Invalid dbref. (1)");
    if (oper[1].type != PROG_STRING)
        abort_interp("String required. (2)");
    if (oper[0].type != PROG_ARRAY)
        abort_interp("Array required. (3)");
    if (oper[0].data.array && oper[0].data.array->type != ARRAY_PACKED)
        abort_interp("Argument must be a list type array. (3)");
    ref = oper[2].data.objref;
    strcpy(dir, DoNullInd(oper[1].data.string));
    arr = oper[0].data.array;
    dirlen = strlen(dir);
    fmtout = propname;
    fmtin = tp_proplist_counter_fmt;
    while (*fmtin) {
        if (*fmtin == 'P') {
            if ((fmtout + dirlen) - propname > sizeof(propname))
                break;
            strcpy(fmtout, dir);
            fmtout = &fmtout[strlen(fmtout)];
        } else {
            *fmtout++ = *fmtin;
        }
        fmtin++;
    }
    *fmtout++ = '\0';

    if (!prop_write_perms(ProgUID, ref, propname, mlev))
        abort_interp
            ("Permission denied while trying to set protected property.");

    if (tp_proplist_int_counter) {
        /* Alynna - Fix a bug where it wont set the proper value if you try to write an int directly.. */
        pdat.flags = PROP_INTTYP;
        pdat.data.val = array_count(arr);
        set_property(ref, propname, &pdat);
    } else {
        sprintf(buf, "%d", array_count(arr));
        pdat.flags = PROP_STRTYP;
        pdat.data.str = buf;
        set_property(ref, propname, &pdat);
    }

    if (array_first(arr, &temp1)) {
        struct inst *oper4;

        do {
            oper4 = array_getitem(arr, &temp1);
            fmtout = propname;
            fmtin = tp_proplist_entry_fmt;
            while (*fmtin) {
                if (*fmtin == 'N') {
                    if ((fmtout + 18) - propname > sizeof(propname))
                        break;
                    sprintf(fmtout, "%d", temp1.data.number + 1);
                    fmtout = &fmtout[strlen(fmtout)];
                } else if (*fmtin == 'P') {
                    if ((fmtout + dirlen) - propname > sizeof(propname))
                        break;
                    strcpy(fmtout, dir);
                    fmtout = &fmtout[strlen(fmtout)];
                } else {
                    *fmtout++ = *fmtin;
                }
                fmtin++;
            }
            *fmtout++ = '\0';

            if (!prop_write_perms(ProgUID, ref, propname, mlev))
                abort_interp
                    ("Permission denied while trying to set protected property.");

            switch (oper4->type) {
                case PROG_STRING:
                    pdat.flags = PROP_STRTYP;
                    pdat.data.str =
                        oper4->data.string ? oper4->data.string->data : NULL;
                    break;
                case PROG_INTEGER:
                    pdat.flags = PROP_INTTYP;
                    pdat.data.val = oper4->data.number;
                    break;
                case PROG_FLOAT:
                    pdat.flags = PROP_FLTTYP;
                    pdat.data.fval = oper4->data.fnumber;
                    break;
                case PROG_OBJECT:
                    pdat.flags = PROP_REFTYP;
                    pdat.data.ref = oper4->data.objref;
                    break;
                case PROG_LOCK:
                    pdat.flags = PROP_LOKTYP;
                    pdat.data.lok = copy_bool(oper4->data.lock);
                    break;
            }
            set_property(ref, propname, &pdat);

        } while (array_next(arr, &temp1));
    }
    count = temp1.data.number;
    for (;;) {
        count++;
        fmtout = propname;
        fmtin = tp_proplist_entry_fmt;
        while (*fmtin) {
            if (*fmtin == 'N') {
                if ((fmtout + 18) - propname > sizeof(propname))
                    break;
                sprintf(fmtout, "%d", count + 1);
                fmtout = &fmtout[strlen(fmtout)];
            } else if (*fmtin == 'P') {
                if ((fmtout + dirlen) - propname > sizeof(propname))
                    break;
                strcpy(fmtout, dir);
                fmtout = &fmtout[strlen(fmtout)];
            } else {
                *fmtout++ = *fmtin;
            }
            fmtin++;
        }
        *fmtout++ = '\0';
        if (get_property(ref, propname)) {
            remove_property(ref, propname);
        } else {
            break;
        }
    }
}

void
prim_array_get_reflist(PRIM_PROTOTYPE)
{
    stk_array *nw;
    const char *rawstr;
    char dir[BUFFER_LEN];
    int count = 0;
	int result;
	dbref ref;
	struct inst temp1, temp2;

    /* dbref strPropDir -- array */

    if (oper[1].type != PROG_OBJECT)
        abort_interp("Dbref required. (1)");
    if (!valid_object(&oper[1]))
        abort_interp("Invalid dbref. (1)");
    if (oper[0].type != PROG_STRING)
        abort_interp("String required. (2)");
    if (!oper[0].data.string)
        abort_interp("Non-null string required. (2)");

    ref = oper[1].data.objref;
    strcpy(dir, oper[0].data.string->data);

    if (!prop_read_perms(ProgUID, ref, dir, mlev))
        abort_interp(tp_noperm_mesg);

    nw = new_array_packed(0);
    rawstr = get_property_class(ref, dir);

    if (rawstr) {
        while (isspace(*rawstr))
            rawstr++;
        while (*rawstr) {
            if (*rawstr == '#')
                rawstr++;
            if (!isdigit(*rawstr) && (*rawstr != '-'))
                break;
            result = atoi(rawstr);
            while (*rawstr && !isspace(*rawstr))
                rawstr++;
            while (isspace(*rawstr))
                rawstr++;

            temp1.type = PROG_INTEGER;
            temp1.data.number = count;

            temp2.type = PROG_OBJECT;
            temp2.data.number = result;

            array_setitem(&nw, &temp1, &temp2);
            count++;

            CLEAR(&temp1);
            CLEAR(&temp2);
        }
    }

    PushArrayRaw(nw);
}


void
prim_array_put_reflist(PRIM_PROTOTYPE)
{
    stk_array *arr;
    char buf2[BUFFER_LEN];
    char dir[BUFFER_LEN];
    char *out;
    int len;
    PData pdat;
	dbref ref;
	char buf[BUFFER_LEN];
	struct inst temp1;

    /* dbref strPropDir array -- */

    if (oper[2].type != PROG_OBJECT)
        abort_interp("Dbref required. (1)");
    if (!valid_object(&oper[2]))
        abort_interp("Invalid dbref. (1)");
    if (oper[1].type != PROG_STRING)
        abort_interp("String required. (2)");
    if (!oper[1].data.string)
        abort_interp("Non-null string required. (2)");
    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument must be a list array of dbrefs. (3)");
    if (oper[0].data.array && oper[0].data.array->type != ARRAY_PACKED)
        abort_interp("Argument must be a list array of dbrefs. (3)");
    if (!array_is_homogenous(oper[0].data.array, PROG_OBJECT))
        abort_interp("Argument must be a list array of dbrefs. (3)");

    ref = oper[2].data.objref;
    strcpy(dir, DoNullInd(oper[1].data.string));

    remove_property(ref, dir);
    arr = oper[0].data.array;
    buf[0] = '\0';

    if (!prop_write_perms(ProgUID, ref, dir, mlev))
        abort_interp(tp_noperm_mesg);

    out = buf;
    if (array_first(arr, &temp1)) {
        struct inst *oper4;

        do {
            oper4 = array_getitem(arr, &temp1);
            len = sprintf(buf2, "#%d", oper4->data.objref);

            if (out + len - buf >= BUFFER_LEN - 3)
                abort_interp
                    ("Operation would result in string length overflow.");

            if (*buf)
                *out++ = ' ';
            strcpy(out, buf2);
            out += len;
        } while (array_next(arr, &temp1));
    }

    pdat.flags = PROP_STRTYP;
    pdat.data.str = buf;
    set_property(ref, dir, &pdat);
}

void
prim_array_findval(PRIM_PROTOTYPE)
{
    struct inst *in;
    struct inst temp1;
    stk_array *arr;
    stk_array *nw;
    int found = 0;

    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    nw = new_array_packed(0);
    arr = oper[1].data.array;
    if (array_first(arr, &temp1)) {
        do {
            in = array_getitem(arr, &temp1);
            if (!array_idxcmp(in, &oper[0])) {
                array_appenditem(&nw, &temp1);
            }
        } while (!found && array_next(arr, &temp1));
    }

    PushArrayRaw(nw);
}

void
prim_array_excludeval(PRIM_PROTOTYPE)
{
    struct inst *in;
    stk_array *arr;
    stk_array *nw;
    int found = 0;
	struct inst temp1;

    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    nw = new_array_packed(0);
    arr = oper[1].data.array;
    if (array_first(arr, &temp1)) {
        do {
            in = array_getitem(arr, &temp1);
            if (array_idxcmp(in, &oper[0])) {
                array_appenditem(&nw, &temp1);
            }
        } while (!found && array_next(arr, &temp1));
    }

    PushArrayRaw(nw);
}

void
prim_explode_array(PRIM_PROTOTYPE)
{
    stk_array *nu;
    char *tempPtr;
    char *lastPtr;
	struct inst temp3;
	char buf[BUFFER_LEN];

    if (oper[0].type != PROG_STRING)
        abort_interp("Non-string argument (2)");
    if (oper[1].type != PROG_STRING)
        abort_interp("Non-string argument (1)");
    if (!oper[0].data.string)
        abort_interp("Empty string argument (2)");
    CHECKOFLOW(1);

    {
        const char *delimit = oper[0].data.string->data;
        int delimlen = oper[0].data.string->length;

        nu = new_array_packed(0);
        if (!oper[1].data.string) {
            lastPtr = (char *)"";
        } else {
            strcpy(buf, oper[1].data.string->data);
            tempPtr = lastPtr = buf;
            while (*tempPtr) {
                if (!strncmp(tempPtr, delimit, delimlen)) {
                    *tempPtr = '\0';
                    tempPtr += delimlen;

                    temp3.type = PROG_STRING;
                    temp3.data.string = alloc_prog_string(lastPtr);
                    array_appenditem(&nu, &temp3);
                    CLEAR(&temp3);

                    lastPtr = tempPtr;
                } else {
                    tempPtr++;
                }
            }
        }
    }

    temp3.type = PROG_STRING;
    temp3.data.string = alloc_prog_string(lastPtr);
    array_appenditem(&nu, &temp3);

    CLEAR(&temp3);

    PushArrayRaw(nu);
}

void
prim_array_join(PRIM_PROTOTYPE)
{
    struct inst *in;
    stk_array *arr;
    char outbuf[BUFFER_LEN];
    char *ptr;
    const char *text;
    char *delim;
    int tmplen;
    int done;
	struct inst temp1;
	char buf[BUFFER_LEN];

    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");
    if (oper[0].type != PROG_STRING)
        abort_interp("Argument not a string pattern. (2)");

    arr = oper[1].data.array;
    delim = (char *) DoNullInd(oper[0].data.string);
    ptr = outbuf;
    *outbuf = '\0';
    done = !array_first(arr, &temp1);
    while (!done) {
        in = array_getitem(arr, &temp1);
        switch (in->type) {
            case PROG_STRING:
                text = DoNullInd(in->data.string);
                break;
            case PROG_INTEGER:
                sprintf(buf, "%d", in->data.number);
                text = buf;
                break;
            case PROG_OBJECT:
                sprintf(buf, "#%d", in->data.number);
                text = buf;
                break;
            case PROG_FLOAT:
                sprintf(buf, "%.15g", in->data.fnumber);
                text = buf;
                break;
            case PROG_LOCK:
                text = unparse_boolexp(ProgUID, in->data.lock, 1);
                break;
            default:
                text = "<UNSUPPORTED>";
                break;
        }
        tmplen = strlen(text);
        if (tmplen > BUFFER_LEN - (ptr - outbuf) - 1) {
            strncpy(ptr, text, BUFFER_LEN - (ptr - outbuf) - 1);
            outbuf[BUFFER_LEN - 1] = '\0';
            break;
        } else {
            strcpy(ptr, text);
            ptr += tmplen;
        }
        done = !array_next(arr, &temp1);
        if (!done) {
            tmplen = strlen(delim);
            if (tmplen > BUFFER_LEN - (ptr - outbuf) - 1) {
                strncpy(ptr, delim, BUFFER_LEN - (ptr - outbuf) - 1);
                outbuf[BUFFER_LEN - 1] = '\0';
                break;
            } else {
                strcpy(ptr, delim);
                ptr += tmplen;
            }
        }
    }

    PushString(outbuf);
}

void
prim_array_interpret(PRIM_PROTOTYPE)
{
    struct inst *in;
    stk_array *arr;
    char outbuf[BUFFER_LEN];
    char *ptr;
    const char *text;
	struct inst temp1;
	char buf[BUFFER_LEN];
    int tmplen;
    int done;

    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    arr = oper[0].data.array;
    ptr = outbuf;
    *outbuf = '\0';
    done = !array_first(arr, &temp1);
    while (!done) {
        in = array_getitem(arr, &temp1);
        switch (in->type) {
            case PROG_STRING:
                text = DoNullInd(in->data.string);
                break;
            case PROG_INTEGER:
                sprintf(buf, "%d", in->data.number);
                text = buf;
                break;
            case PROG_OBJECT:
                if (in->data.number == -1) {
                    text = "*NOTHING*";
                    break;
                }
                if (in->data.number == -2) {
                    text = "*AMBIGUOUS(#-2)*";
                    break;
                }
                if (in->data.number == -3) {
                    text = "*HOME*";
                    break;
                }
                if (in->data.number == -4) {
                    text = "*NIL*";
                    break;
                }
                if (in->data.number < -4) {
                    sprintf(buf, "*INVALID(#%d)*", in->data.number);
                    text = buf;
                    break;
                }
                if (in->data.number >= db_top) {
                    sprintf(buf, "*INVALID(#%d)*", in->data.number);
                    text = buf;
                    break;
                }
                sprintf(buf, "%s", NAME(in->data.number));
                text = buf;
                break;
            case PROG_FLOAT:
                sprintf(buf, "%.15g", in->data.fnumber);
                text = buf;
                break;
            case PROG_LOCK:
                text = unparse_boolexp(ProgUID, in->data.lock, 1);
                break;
            default:
                text = "<UNSUPPORTED>";
                break;
        }

        tmplen = strlen(text);
        if (tmplen > BUFFER_LEN - (ptr - outbuf) - 1) {
            strncpy(ptr, text, BUFFER_LEN - (ptr - outbuf) - 1);
            outbuf[BUFFER_LEN - 1] = '\0';
            break;
        } else {
            strcpy(ptr, text);
            ptr += tmplen;
        }
        done = !array_next(arr, &temp1);
    }

    PushString(outbuf);
}

void
prim_array_matchkey(PRIM_PROTOTYPE)
{
    struct inst *in;
    stk_array *arr;
    stk_array *nw;
	struct inst temp1;

    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");
    if (oper[0].type != PROG_STRING)
        abort_interp("Argument not a string pattern. (2)");

    nw = new_array_dictionary();
    arr = oper[1].data.array;
    if (array_first(arr, &temp1)) {
        do {
            if (temp1.type == PROG_STRING) {
                if (equalstr
                    ((char *) DoNullInd(oper[0].data.string),
                     (char *) DoNullInd(temp1.data.string))) {
                    in = array_getitem(arr, &temp1);
                    array_setitem(&nw, &temp1, in);
                }
            }
        } while (array_next(arr, &temp1));
    }

    PushArrayRaw(nw);
}

void
prim_array_matchval(PRIM_PROTOTYPE)
{
    struct inst *in;
    stk_array *arr;
    stk_array *nw;
	struct inst temp1;

    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");
    if (oper[0].type != PROG_STRING)
        abort_interp("Argument not a string pattern. (2)");

    nw = new_array_dictionary();
    arr = oper[1].data.array;
    if (array_first(arr, &temp1)) {
        do {
            in = array_getitem(arr, &temp1);
            if (in->type == PROG_STRING) {
                if (equalstr((char *) DoNullInd(oper[0].data.string),
                             (char *) DoNullInd(in->data.string))) {
                    array_setitem(&nw, &temp1, in);
                }
            } else if (in->type == PROG_OBJECT) {
                if (equalstr((char *) DoNullInd(oper[0].data.string),
                             (char *) NAME(in->data.objref))) {
                    array_setitem(&nw, &temp1, in);
                }
            }
        } while (array_next(arr, &temp1));
    }

    PushArrayRaw(nw);
}

void
prim_array_extract(PRIM_PROTOTYPE)
{
    struct inst *in;
    struct inst *key;
    stk_array *arr;
    stk_array *karr;
    stk_array *nw;
	struct inst temp1;

    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");
    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array. (2)");

    nw = new_array_dictionary();
    arr = oper[1].data.array;
    karr = oper[0].data.array;
    if (array_first(karr, &temp1)) {
        do {
            key = array_getitem(karr, &temp1);
            if (key) {
                in = array_getitem(arr, key);
                if (in) {
                    array_setitem(&nw, key, in);
                }
            }
        } while (array_next(karr, &temp1));
    }

    PushArrayRaw(nw);
}

void
prim_array_cut(PRIM_PROTOTYPE)
{
    stk_array *nu1 = NULL;
    stk_array *nu2 = NULL;
    struct inst temps;
    struct inst tempc;
    struct inst tempe;
	int result;

    if (oper[0].type != PROG_INTEGER && oper[0].type != PROG_STRING)
        abort_interp("Argument not an integer or string. (2)");
    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    temps.type = PROG_INTEGER;
    temps.data.number = 0;
    result = array_first(oper[1].data.array, &temps);

    if (result) {
        copyinst(&oper[0], &tempc);
        result = array_prev(oper[1].data.array, &tempc);
        if (result) {
            nu1 = array_getrange(oper[1].data.array, &temps, &tempc);
            CLEAR(&tempc);
        }

        result = array_last(oper[1].data.array, &tempe);
        if (result) {
            nu2 = array_getrange(oper[1].data.array, &oper[0], &tempe);
            CLEAR(&tempe);
        }

        CLEAR(&temps);
    }

    if (!nu1)
        nu1 = new_array_packed(0);
    if (!nu2)
        nu2 = new_array_packed(0);

    PushArrayRaw(nu1);
    PushArrayRaw(nu2);
}

void
prim_array_compare(PRIM_PROTOTYPE)
{
    struct inst *val1;
    struct inst *val2;
    stk_array *arr1;
    stk_array *arr2;
    int res1, res2, result;
	struct inst temp1, temp2;

    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");
    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array. (2)");

    arr1 = oper[1].data.array;
    arr2 = oper[0].data.array;
    res1 = array_first(arr1, &temp1);
    res2 = array_first(arr2, &temp2);

    if (!res1 && !res2) {
        result = 0;
    } else if (!res1) {
        result = -1;
    } else if (!res2) {
        result = 1;
    } else {
        do {
            result = array_idxcmp(&temp1, &temp2);
            if (result)
                break;

            val1 = array_getitem(arr1, &temp1);
            val2 = array_getitem(arr2, &temp2);
            result = array_idxcmp(val1, val2);
            if (result)
                break;

            res1 = array_next(arr1, &temp1);
            res2 = array_next(arr2, &temp2);
        } while (res1 && res2);

        if (!res1 && !res2) {
            result = 0;
        } else if (!res1) {
            result = -1;
        } else if (!res2) {
            result = 1;
        }
    }

    PushInt(result);
}

void
prim_array_filter_flags(PRIM_PROTOTYPE)
{
    struct flgchkdat check;
    stk_array *nw, *arr;
    struct inst *in;
	struct inst temp1;

    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");
    if (!array_is_homogenous(oper[1].data.array, PROG_OBJECT))
        abort_interp("Argument not an array of dbrefs. (1)");
    if (oper[0].type != PROG_STRING || !oper[0].data.string)
        abort_interp("Argument not a non-null string. (2)");

    arr = oper[1].data.array;
    nw = new_array_packed(0);

    init_checkflags(PSafe, DoNullInd(oper[0].data.string), &check);

    if (array_first(arr, &temp1)) {
        do {
            in = array_getitem(arr, &temp1);
            if (valid_object(in) && checkflags(in->data.objref, check))
                array_appenditem(&nw, in);
        } while (array_next(arr, &temp1));
    }

    PushArrayRaw(nw);
}

void
prim_array_nested_get(PRIM_PROTOTYPE)
{
    struct inst *idx;
    struct inst *dat;
    struct inst temp;
    stk_array *idxarr;
    int i, idxcnt;
	struct inst temp1;

    idx = NULL;

    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array of indexes. (2)");
    if (!oper[0].data.array || oper[0].data.array->type == ARRAY_DICTIONARY)
        abort_interp("Argument not an array of indexes. (2)");
    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    idxarr = oper[0].data.array;
    idxcnt = array_count(idxarr);
    dat = &oper[1];
    for (i = 0; dat && i < idxcnt; i++) {
        temp1.type = PROG_INTEGER;
        temp1.data.number = i;
        idx = array_getitem(idxarr, &temp1);
        if (idx->type != PROG_INTEGER && idx->type != PROG_STRING)
            abort_interp("Argument not an array of indexes. (2)");
        if (dat->type != PROG_ARRAY)
            abort_interp("Mid-level nested item was not an array. (1)");
        dat = array_getitem(dat->data.array, idx);
    }

    /* copy data to a temp inst before clearing the containing array */
    if (dat) {
        copyinst(dat, &temp);
    } else {
        temp.type = PROG_INTEGER;
        temp.data.number = 0;
    }

    /* copy data to stack, then clear temp inst */
    copyinst(&temp, &arg[(*top)++]);
    CLEAR(&temp);
}

void
prim_array_nested_set(PRIM_PROTOTYPE)
{
    struct inst nest[16];
    struct inst *idx;
    struct inst *dat;
    struct inst temp;
    stk_array *arr;
    stk_array *idxarr;
    int i, idxcnt;

    idx = NULL;

    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array of indexes. (3)");
    if (!oper[0].data.array || oper[0].data.array->type == ARRAY_DICTIONARY)
        abort_interp("Argument not an array of indexes. (3)");
    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array. (2)");

    idxarr = oper[0].data.array;
    idxcnt = array_count(idxarr);
    if (idxcnt > sizeof(nest) / sizeof(struct inst))
        abort_interp("Nesting would be too deep. (3)");

    if (idxcnt == 0) {
        copyinst(&oper[1], &nest[0]);
    } else {
        dat = &oper[1];
        for (i = 0; i < idxcnt; i++) {
            copyinst(dat, &nest[i]);
            temp.type = PROG_INTEGER;
            temp.data.number = i;
            idx = array_getitem(idxarr, &temp);
            if (idx->type != PROG_INTEGER && idx->type != PROG_STRING)
                abort_interp("Argument not an array of indexes. (3)");
            if (i < idxcnt - 1) {
                dat = array_getitem(nest[i].data.array, idx);
                if (dat) {
                    if (dat->type != PROG_ARRAY)
                        abort_interp
                            ("Mid-level nested item was not an array. (2)");
                } else {
                    if (idx->type == PROG_INTEGER && idx->data.number == 0) {
                        arr = new_array_packed(1);
                    } else {
                        arr = new_array_dictionary();
                    }
                    arr->links = 0;
                    temp.type = PROG_ARRAY;
                    temp.data.array = arr;
                    dat = &temp;
                }
            }
        }

        array_setitem(&nest[idxcnt - 1].data.array, idx, &oper[2]);
        for (i = idxcnt - 1; i-- > 0;) {
            temp.type = PROG_INTEGER;
            temp.data.number = i;
            idx = array_getitem(idxarr, &temp);
            array_setitem(&nest[i].data.array, idx, &nest[i + 1]);
            CLEAR(&nest[i + 1]);
        }
    }

    /* copy data to stack, then clear temp inst */
    copyinst(&nest[0], &arg[(*top)++]);
    CLEAR(&nest[0]);
}

void
prim_array_nested_del(PRIM_PROTOTYPE)
{
    struct inst nest[32];
    struct inst *idx;
    struct inst *dat;
    struct inst temp;
    stk_array *idxarr;
    int i, idxcnt;

    idx = NULL;

    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array of indexes. (2)");
    if (!oper[0].data.array || oper[0].data.array->type == ARRAY_DICTIONARY)
        abort_interp("Argument not an array of indexes. (2)");
    if (oper[1].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    idxarr = oper[0].data.array;
    idxcnt = array_count(idxarr);
    if (idxcnt > sizeof(nest) / sizeof(struct inst))
        abort_interp("Nesting would be too deep. (2)");

    if (idxcnt == 0) {
        copyinst(&oper[1], &nest[0]);
    } else {
        int doneearly = 0;

        dat = &oper[1];
        for (i = 0; i < idxcnt; i++) {
            copyinst(dat, &nest[i]);
            temp.type = PROG_INTEGER;
            temp.data.number = i;
            idx = array_getitem(idxarr, &temp);
            if (idx->type != PROG_INTEGER && idx->type != PROG_STRING)
                abort_interp("Argument not an array of indexes. (2)");
            if (i < idxcnt - 1) {
                dat = array_getitem(nest[i].data.array, idx);
                if (dat) {
                    if (dat->type != PROG_ARRAY)
                        abort_interp
                            ("Mid-level nested item was not an array. (1)");
                } else {
                    doneearly = 1;
                    break;
                }
            }
        }
        if (!doneearly) {
            array_delitem(&nest[idxcnt - 1].data.array, idx);
            for (i = idxcnt - 1; i-- > 0;) {
                temp.type = PROG_INTEGER;
                temp.data.number = i;
                idx = array_getitem(idxarr, &temp);
                array_setitem(&nest[i].data.array, idx, &nest[i + 1]);
                CLEAR(&nest[i + 1]);
            }
        }
    }

    /* copy data to stack, then clear temp inst */
    copyinst(&nest[0], &arg[(*top)++]);
    CLEAR(&nest[0]);
}

void
prim_array_sum(PRIM_PROTOTYPE)
{
    struct inst *in;
    stk_array *arr;
    double facc;
    register int iacc;
    int tiacc;
    int done;
    register int floaty;
	struct inst temp1;

    if (oper[0].type != PROG_ARRAY)
        abort_interp("Argument not an array. (1)");

    floaty = 0;
    iacc = 0;
    facc = 0.0;
    arr = oper[0].data.array;
    done = !array_first(arr, &temp1);
    while (!done) {
        in = array_getitem(arr, &temp1);
        switch (in->type) {
            case PROG_INTEGER:
                iacc += in->data.number;
                break;
            case PROG_FLOAT:
                facc += in->data.fnumber;
                floaty++;
                break;
            default:
                abort_interp
                    ("Invalid datatype in array (not integer or float)");
                break;
        }
        done = !array_next(arr, &temp1);
    }

    if (floaty) {
        facc += iacc;
        PushFloat(facc);
    } else {
        /* Need this to be out of a register for the push */
        tiacc = iacc;
        PushInt(tiacc);
    }

}

void
prim_array_string_fragment(PRIM_PROTOTYPE)
{
    stk_array *nu;
    char *tempPtr;
    int nChunkSize = 0;
    int nCount = 0; 
    int nStrLen = 0;   
	struct inst temp1, temp2, temp3;
	static char buf[BUFFER_LEN];

    temp1 = oper[0];
    temp2 = oper[1];
    if ( temp1.type != PROG_INTEGER )
        abort_interp("Non-integer argument (2)");
    if ( temp2.type != PROG_STRING )
        abort_interp("Non-string argument (1)");
    if ( temp1.data.number < 1 )
        abort_interp("Argument must be greater than 0. (2)");
    CHECKOFLOW(1);
    
    nChunkSize = temp1.data.number;
    
    nu = new_array_packed(0);
    
    if ( temp2.data.string ) {
        nStrLen = temp2.data.string->length;
        tempPtr = temp2.data.string->data;

        while ( nCount + nChunkSize < nStrLen ) {
            strncpy(buf, tempPtr, nChunkSize );        
            *(buf+nChunkSize) = '\0';        
    
            temp3.type = PROG_STRING;
            temp3.data.string = alloc_prog_string(buf);
            array_appenditem(&nu, &temp3);
            CLEAR(&temp3);
        
            nCount += nChunkSize;
            tempPtr += nChunkSize;
        }
 
        if ( nCount != nStrLen ){
            /* One last bit of string to process */
            strncpy(buf, tempPtr, nStrLen - nCount);
            *(buf+(nStrLen-nCount)) = '\0';
 
            temp3.type = PROG_STRING;
            temp3.data.string = alloc_prog_string(buf);
            array_appenditem(&nu, &temp3);
            CLEAR(&temp3);
        }
    }
    else {
        temp3.type = PROG_STRING;
        temp3.data.string = alloc_prog_string("");
        array_appenditem(&nu, &temp3);
        CLEAR(&temp3);
    }
   
    PushArrayRaw(nu);
}      
        

