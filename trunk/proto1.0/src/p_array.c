/* Primitives Package */

#include "copyright.h"
#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <ctype.h>
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

static struct inst *oper1, *oper2, *oper3, *oper4;
static struct inst temp1, temp2, temp3;
static int result;
static dbref ref;
static char buf[BUFFER_LEN];



void
prim_array_make(PRIM_PROTOTYPE)
{
        stk_array *new;
        int i;

        CHECKOP(1);
        oper1 = POP();
        if (oper1->type != PROG_INTEGER)
                abort_interp("Invalid item count.");
        result = oper1->data.number;
        if (result < 0)
                abort_interp("Item count must be non-negative.");
        CLEAR(oper1);
        nargs = 0;

        if (*top < result)
                abort_interp("Stack underflow.");

        temp1.type = PROG_INTEGER;
        temp1.line = 0;
        new = new_array_packed(result);
        for (i = result; i-- > 0;) {
                temp1.data.number = i;
                oper1 = POP();
                array_setitem(&new, &temp1, oper1);
                CLEAR(oper1);
        }

        PushArrayRaw(new);
}



void
prim_array_make_dict(PRIM_PROTOTYPE)
{
        stk_array *new;
        int i;

        CHECKOP(1);
        oper1 = POP();
        if (oper1->type != PROG_INTEGER)
                abort_interp("Invalid item count.");
        result = oper1->data.number;
        if (result < 0)
                abort_interp("Item count must be positive.");
        CLEAR(oper1);
        nargs = 0;

        if (*top < (2 * result))
                abort_interp("Stack underflow.");
        nargs = 2;

        new = new_array_dictionary();
        for (i = result; i-- > 0;) {
                oper1 = POP();                  /* val */
                oper2 = POP();                  /* key */
                if (oper2->type != PROG_INTEGER && oper2->type != PROG_STRING) {
                        array_free(new);
                        abort_interp("Keys must be integers or strings.");
                }
                array_setitem(&new, oper2, oper1);
                CLEAR(oper1);
                CLEAR(oper2);
        }

        PushArrayRaw(new);
}



void
prim_array_explode(PRIM_PROTOTYPE)
{
        stk_array *arr;

        CHECKOP(1);
        oper1 = POP();
        if (oper1->type != PROG_ARRAY)
                abort_interp("Argument not an array.");
        copyinst(oper1, &temp2);
        arr = temp2.data.array;
        result = array_count(arr);
        CHECKOFLOW(((2 * result) + 1));
        CLEAR(oper1);

        if (array_first(arr, &temp1)) {
                do {
                        copyinst(&temp1, &arg[((*top)++)]);
                        oper2 = array_getitem(arr, &temp1);
                        copyinst(oper2, &arg[((*top)++)]);
                        oper2 = NULL;
                } while (array_next(arr, &temp1));
        }

        CLEAR(&temp1);
        CLEAR(&temp2);
        PushInt(result);
}



void
prim_array_vals(PRIM_PROTOTYPE)
{
        stk_array *arr;

        CHECKOP(1);
        oper1 = POP();
        if (oper1->type != PROG_ARRAY)
                abort_interp("Argument not an array.");
        copyinst(oper1, &temp2);
        arr = temp2.data.array;
        result = array_count(arr);
        CHECKOFLOW((result + 1));
        CLEAR(oper1);

        if (array_first(arr, &temp1)) {
                do {
                        oper2 = array_getitem(arr, &temp1);
                        copyinst(oper2, &arg[((*top)++)]);
                        oper2 = NULL;
                } while (array_next(arr, &temp1));
        }

        CLEAR(&temp1);
        CLEAR(&temp2);
        PushInt(result);
}



void
prim_array_keys(PRIM_PROTOTYPE)
{
        stk_array *arr;

        CHECKOP(1);
        oper1 = POP();
        if (oper1->type != PROG_ARRAY)
                abort_interp("Argument not an array.");
        copyinst(oper1, &temp2);
        arr = temp2.data.array;
        result = array_count(arr);
        CHECKOFLOW((result + 1));
        CLEAR(oper1);

        if (array_first(arr, &temp1)) {
                do {
                        copyinst(&temp1, &arg[((*top)++)]);
                } while (array_next(arr, &temp1));
        }

        CLEAR(&temp1);
        CLEAR(&temp2);
        PushInt(result);
}



void
prim_array_count(PRIM_PROTOTYPE)
{
        CHECKOP(1);
        oper1 = POP();
        if (oper1->type != PROG_ARRAY)
                abort_interp("Argument not an array.");
        result = array_count(oper1->data.array);

        CLEAR(oper1);
        PushInt(result);
}



void
prim_array_first(PRIM_PROTOTYPE)
{
        CHECKOP(1);
        oper1 = POP();                          /* arr  Array */
        if (oper1->type != PROG_ARRAY)
                abort_interp("Argument not an array.");

        temp1.type = PROG_INTEGER;
        temp1.data.number = 0;
        result = array_first(oper1->data.array, &temp1);

        CLEAR(oper1);
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
        CHECKOP(1);
        oper1 = POP();                          /* arr  Array */
        if (oper1->type != PROG_ARRAY)
                abort_interp("Argument not an array.");

        temp1.type = PROG_INTEGER;
        temp1.data.number = 0;
        result = array_last(oper1->data.array, &temp1);

        CLEAR(oper1);
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
        CHECKOP(2);
        oper1 = POP();                          /* ???  previous index */
        oper2 = POP();                          /* arr  Array */
        if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
                abort_interp("Argument not an integer or string. (2)");
        if (oper2->type != PROG_ARRAY)
                abort_interp("Argument not an array.(1)");

        copyinst(oper1, &temp1);
        result = array_prev(oper2->data.array, &temp1);

        CLEAR(oper1);
        CLEAR(oper2);

        if (result) {
                copyinst(&temp1, &arg[((*top)++)]);
        } else {
                result = 0;
                PushInt(result);
        }
        CLEAR(&temp1);
        PushInt(result);
}



void
prim_array_next(PRIM_PROTOTYPE)
{

        CHECKOP(2);
        oper1 = POP();                          /* ???  previous index */
        oper2 = POP();                          /* arr  Array */
        if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
                abort_interp("Argument not an integer or string. (2)");
        if (oper2->type != PROG_ARRAY)
                abort_interp("Argument not an array.(1)");

        copyinst(oper1, &temp1);
        result = array_next(oper2->data.array, &temp1);

        CLEAR(oper1);
        CLEAR(oper2);

        if (result) {
                copyinst(&temp1, &arg[((*top)++)]);
        } else {
                result = 0;
                PushInt(result);
        }
        CLEAR(&temp1);
        PushInt(result);
}



void
prim_array_getitem(PRIM_PROTOTYPE)
{
        struct inst *in;
        struct inst temp;

        CHECKOP(2);
        oper1 = POP();                          /* ???  index */
        oper2 = POP();                          /* arr  Array */
        if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
                abort_interp("Argument not an integer or string. (2)");
        if (oper2->type != PROG_ARRAY)
                abort_interp("Argument not an array. (1)");
        in = array_getitem(oper2->data.array, oper1);

        /* copy data to a temp inst before clearing the containing array */
        if (in) {
                copyinst(in, &temp);
        } else {
                temp.type = PROG_INTEGER;
                temp.data.number = 0;
        }

        CLEAR(oper1);
        CLEAR(oper2);

        /* copy data to stack, then clear temp inst */
        copyinst(&temp, &arg[(*top)++]);
        CLEAR(&temp);
}



void
prim_array_setitem(PRIM_PROTOTYPE)
{
        stk_array *new;

        CHECKOP(3);
        oper1 = POP();                          /* ???  index to store at */
        oper2 = POP();                          /* arr  Array to store in */
        oper3 = POP();                          /* ???  item to store     */

        if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
                abort_interp("Argument not an integer or string. (2)");
        if (oper2->type != PROG_ARRAY)
                abort_interp("Argument not an array. (1)");

        result = array_setitem(&oper2->data.array, oper1, oper3);
        if (result < 0)
                abort_interp("Index out of array bounds. (3)");

        new = oper2->data.array;
        oper2->data.array = NULL;

        CLEAR(oper1);
        CLEAR(oper2);
        CLEAR(oper3);

        PushArrayRaw(new);
}

void
prim_array_appenditem(PRIM_PROTOTYPE)
{
        stk_array *new;

        CHECKOP(2);
        oper2 = POP();                          /* arr  Array to store in */
        oper1 = POP();                          /* ???  item to store     */

        if (oper2->type != PROG_ARRAY)
                abort_interp("Argument not an array. (2)");
        if (oper2->data.array && oper2->data.array->type != ARRAY_PACKED)
                abort_interp("Argument must be a list type array. (2)");

        result = array_appenditem(&oper2->data.array, oper1);
        if (result == -1)
                abort_interp("Internal Error!");

        new = oper2->data.array;
        oper2->data.array = NULL;

        CLEAR(oper1);
        CLEAR(oper2);

        PushArrayRaw(new);
}

void
prim_array_insertitem(PRIM_PROTOTYPE)
{
        stk_array *new;

        CHECKOP(3);
        oper1 = POP();                          /* ???  index to store at */
        oper2 = POP();                          /* arr  Array to store in */
        oper3 = POP();                          /* ???  item to store     */
        if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
                abort_interp("Argument not an integer or string. (2)");
        if (oper2->type != PROG_ARRAY)
                abort_interp("Argument not an array. (1)");

        result = array_insertitem(&oper2->data.array, oper1, oper3);
        if (result < 0)
                abort_interp("Index out of array bounds. (3)");

        new = oper2->data.array;
        oper2->data.array = NULL;

        CLEAR(oper1);
        CLEAR(oper2);
        CLEAR(oper3);

        PushArrayRaw(new);
}



void
prim_array_getrange(PRIM_PROTOTYPE)
{
        stk_array *new;

        CHECKOP(3);
        oper1 = POP();                          /* int  range end item */
        oper2 = POP();                          /* int  range start item */
        oper3 = POP();                          /* arr  Array   */
        if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
                abort_interp("Argument not an integer or string. (3)");
        if (oper2->type != PROG_INTEGER && oper2->type != PROG_STRING)
                abort_interp("Argument not an integer or string. (2)");
        if (oper3->type != PROG_ARRAY)
                abort_interp("Argument not an array. (1)");

        new = array_getrange(oper3->data.array, oper2, oper1);
        if (!new)
                new = new_array_packed(0);

        CLEAR(oper1);
        CLEAR(oper2);
        CLEAR(oper3);

        PushArrayRaw(new);
}



void
prim_array_setrange(PRIM_PROTOTYPE)
{
        stk_array *new;

        CHECKOP(3);
        oper1 = POP();                          /* arr  array to insert */
        oper2 = POP();                          /* ???  starting pos for lists */
        oper3 = POP();                          /* arr  Array   */
        if (oper1->type != PROG_ARRAY)
                abort_interp("Argument not an array. (3)");
        if (oper2->type != PROG_INTEGER && oper2->type != PROG_STRING)
                abort_interp("Argument not an integer or string. (2)");
        if (oper3->type != PROG_ARRAY)
                abort_interp("Argument not an array. (1)");

        result = array_setrange(&oper3->data.array, oper2, oper1->data.array);
        if (result < 0)
                abort_interp("Index out of array bounds. (2)");

        new = oper3->data.array;
        oper3->data.array = NULL;;

        CLEAR(oper1);
        CLEAR(oper2);
        CLEAR(oper3);

        PushArrayRaw(new);
}


void
prim_array_insertrange(PRIM_PROTOTYPE)
{
        stk_array *new;

        CHECKOP(3);
        oper1 = POP();                          /* arr  array to insert */
        oper2 = POP();                          /* ???  starting pos for lists */
        oper3 = POP();                          /* arr  Array   */
        if (oper1->type != PROG_ARRAY)
                abort_interp("Argument not an array. (3)");
        if (oper2->type != PROG_INTEGER && oper2->type != PROG_STRING)
                abort_interp("Argument not an integer or string. (2)");
        if (oper3->type != PROG_ARRAY)
                abort_interp("Argument not an array. (1)");

        result = array_insertrange(&oper3->data.array, oper2, oper1->data.array);
        if (result < 0)
                abort_interp("Index out of array bounds. (2)");

        new = oper3->data.array;
        oper3->data.array = NULL;

        CLEAR(oper1);
        CLEAR(oper2);
        CLEAR(oper3);

        PushArrayRaw(new);
}



void
prim_array_delitem(PRIM_PROTOTYPE)
{
        stk_array *new;

        CHECKOP(2);
        oper1 = POP();                          /* int  item to delete */
        oper2 = POP();                          /* arr  Array   */
        if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
                abort_interp("Argument not an integer or string. (2)");
        if (oper2->type != PROG_ARRAY)
                abort_interp("Argument not an array. (1)");

        result = array_delitem(&oper2->data.array, oper1);
        if (result < 0)
                abort_interp("Bad array index specified.");

        new = oper2->data.array;
        oper2->data.array = NULL;

        CLEAR(oper1);
        CLEAR(oper2);

        PushArrayRaw(new);
}



void
prim_array_delrange(PRIM_PROTOTYPE)
{
        stk_array *new;

        CHECKOP(3);
        oper1 = POP();                          /* int  range end item */
        oper2 = POP();                          /* int  range start item */
        oper3 = POP();                          /* arr  Array   */
        if (oper1->type != PROG_INTEGER && oper1->type != PROG_STRING)
                abort_interp("Argument not an integer or string. (3)");
        if (oper2->type != PROG_INTEGER && oper2->type != PROG_STRING)
                abort_interp("Argument not an integer or string. (2)");
        if (oper3->type != PROG_ARRAY)
                abort_interp("Argument not an array. (1)");

        result = array_delrange(&oper3->data.array, oper2, oper1);
        if (result < 0)
                abort_interp("Bad array range specified.");

        new = oper3->data.array;
        oper3->data.array = NULL;

        CLEAR(oper1);
        CLEAR(oper2);
        CLEAR(oper3);

        PushArrayRaw(new);
}


/*\
|*| Eeep!
\*/


void
prim_array_n_union(PRIM_PROTOTYPE)
{
        stk_array *new_union;
        stk_array *new_mash;
        int num_arrays;

        CHECKOP(1);
        oper1 = POP();
        if (oper1->type != PROG_INTEGER)
                abort_interp("Invalid item count.");
        result = oper1->data.number;
        if (result < 0)
                abort_interp("Item count must be positive.");
        CLEAR(oper1);
        nargs = 0;

        if (*top < result)
                abort_interp("Stack underflow.");

        if (result > 0) {
                new_mash = new_array_dictionary();
                for (num_arrays = 0; num_arrays < result; num_arrays++) {
                        oper1 = POP();
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
        int num_arrays;

        CHECKOP(1);
        oper1 = POP();
        if (oper1->type != PROG_INTEGER)
                abort_interp("Invalid item count.");
        result = oper1->data.number;
        if (result < 0)
                abort_interp("Item count must be positive.");
        CLEAR(oper1);
        nargs = 0;

        if (*top < result)
                abort_interp("Stack underflow.");

        if (result > 0) {
                new_mash = new_array_dictionary();
                for (num_arrays = 0; num_arrays < result; num_arrays++) {
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
        int num_arrays;

        CHECKOP(1);
        oper1 = POP();
        if (oper1->type != PROG_INTEGER)
                abort_interp("Invalid item count.");
        result = oper1->data.number;
        if (result < 0)
                abort_interp("Item count must be positive.");
        CLEAR(oper1);
        nargs = 0;

        if (*top < result)
                abort_interp("Stack underflow.");

        if (result > 0) {
                new_mash = new_array_dictionary();

                oper1 = POP();
                array_mash(oper1->data.array, &new_mash, 1);
                CLEAR(oper1);

                for (num_arrays = 1; num_arrays < result; num_arrays++) {
                        oper1 = POP();
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
        struct inst *oper1, *oper2, *oper3, *oper4;
        struct inst temp1, temp2;
        char buf2[BUFFER_LEN * 2];

        CHECKOP(2);
        oper2 = POP();
        oper1 = POP();
        if (oper1->type != PROG_ARRAY)
                abort_interp("Argument not an array of strings. (1)");
        if (!array_is_homogenous(oper1->data.array, PROG_STRING))
                abort_interp("Argument not an array of strings. (1)");
        if (oper2->type != PROG_ARRAY)
                abort_interp("Argument not an array of dbrefs. (2)");
        if (!array_is_homogenous(oper2->data.array, PROG_OBJECT))
                abort_interp("Argument not an array of dbrefs. (2)");
        strarr = oper1->data.array;
        refarr = oper2->data.array;

        if (array_first(strarr, &temp2)) {
                do {
                        oper4 = array_getitem(strarr, &temp2);
                        strcpy(buf, DoNullInd(oper4->data.string));
                        if (tp_m1_name_notify && mlev < 2) {
                                strcpy(buf2, PNAME(player));
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

                                        notify_listeners(player, program, oper3->data.objref,
                                                                         getloc(oper3->data.objref), buf, 1);

                                        oper3 = NULL;
                                } while (array_next(refarr, &temp1));
                        }
                        oper4 = NULL;
                } while (array_next(strarr, &temp2));
        }

        CLEAR(oper1);
        CLEAR(oper2);
}



void
prim_array_reverse(PRIM_PROTOTYPE)
{
        stk_array *arr;
        stk_array *new;
        int i;

        CHECKOP(1);
        oper1 = POP();                          /* arr  Array   */
        if (oper1->type != PROG_ARRAY)
                abort_interp("Argument not an array.");
        arr = oper1->data.array;
        if (arr->type != ARRAY_PACKED)
                abort_interp("Argument must be a list type array.");

        result = array_count(arr);
        new = new_array_packed(result);

        temp1.type = PROG_INTEGER;
        temp2.type = PROG_INTEGER;
        for (i = 0; i < result; i++) {
                temp1.data.number = i;
                temp2.data.number = (result - i) - 1;
                array_setitem(&new, &temp1, array_getitem(arr, &temp2));
        }

        CLEAR(oper1);
        PushArrayRaw(new);
}


void
prim_array_get_propdirs(PRIM_PROTOTYPE)
{
        stk_array *new;
        char *tmpname;
        char *pname;
        char propname[BUFFER_LEN];
        char dir[BUFFER_LEN];
        PropPtr propadr, pptr;
        PropPtr prptr;
        int count = 0;

        /* dbref strPropDir -- array */
        CHECKOP(2);
        oper2 = POP();
        oper1 = POP();
        if (mlev < 3)
                abort_interp("Permission denied.");
        if (oper1->type != PROG_OBJECT)
                abort_interp("Dbref required. (1)");
        if (!valid_object(oper1))
                abort_interp("Invalid dbref. (1)");
        if (oper2->type != PROG_STRING)
                abort_interp("String required. (2)");

        ref = oper1->data.objref;
        strcpy(dir, DoNullInd(oper2->data.string));
        if (!*dir)
                strcpy(dir, "/");

        new = new_array_packed(0);
        propadr = first_prop(ref, dir, &pptr, propname);
        while (propadr) {
                sprintf(buf, "%s%c%s", dir, PROPDIR_DELIMITER, propname);
                if (prop_read_perms(ProgUID, ref, buf, mlev)) {
                        prptr = get_property(ref, propname);
                        if (prptr) {
#ifdef DISKBASE
                                propfetch(ref, prptr);
#endif
                                if (PropDir(prptr)) {
                                        if (count >= 511) {
                                                array_free(new);
                                                abort_interp("Too many propdirs to put in an array!");
                                        }

                                        temp2.type = PROG_STRING;
                                        temp2.data.string = alloc_prog_string(propname);

                                        temp1.type = PROG_INTEGER;
                                        temp1.data.number = count++;

                                        array_setitem(&new, &temp1, &temp2);
                                        CLEAR(&temp1);
                                        CLEAR(&temp2);
                                }
                        }
                }
                propadr = next_prop(pptr, propadr, propname);
        }

        PushArrayRaw(new);
}


void
prim_array_get_propvals(PRIM_PROTOTYPE)
{
        stk_array *new;
        char *tmpname;
        char *pname;
        char propname[BUFFER_LEN];
        char dir[BUFFER_LEN];
        PropPtr propadr, pptr;
        PropPtr prptr;
        int count = 0;

        /* dbref strPropDir -- array */
        CHECKOP(2);
        oper2 = POP();
        oper1 = POP();
        if (mlev < 3)
                abort_interp("Permission denied.");
        if (oper1->type != PROG_OBJECT)
                abort_interp("Dbref required. (1)");
        if (!valid_object(oper1))
                abort_interp("Invalid dbref. (1)");
        if (oper2->type != PROG_STRING)
                abort_interp("String required. (2)");

        ref = oper1->data.objref;
        strcpy(dir, DoNullInd(oper2->data.string));
        if (!*dir)
                strcpy(dir, "/");

        new = new_array_dictionary();
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
                                        temp2.data.string = alloc_prog_string(get_uncompress(PropDataStr(prptr)));
                                        break;
                                case PROP_LOKTYP:
                                        temp2.type = PROG_LOCK;
                                        if (PropFlags(prptr) & PROP_ISUNLOADED) {
                                                temp2.data.lock = TRUE_BOOLEXP;
                                        } else {
                                                temp2.data.lock = PropDataLok(prptr);
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
                                        if (count++ >= 511) {
                                                array_free(new);
                                                abort_interp("Too many properties to put in an array!");
                                        }
                                        temp1.type = PROG_STRING;
                                        temp1.data.string = alloc_prog_string(propname);
                                        array_setitem(&new, &temp1, &temp2);
                                        CLEAR(&temp1);
                                        CLEAR(&temp2);
                                }
                        }
                }
                propadr = next_prop(pptr, propadr, propname);
        }

        PushArrayRaw(new);
}


void
prim_array_get_proplist(PRIM_PROTOTYPE)
{
        stk_array *new;
        char *tmpname;
        char *pname;
        const char *strval;
        char propname[BUFFER_LEN];
        char dir[BUFFER_LEN];
        PropPtr propadr, pptr;
        PropPtr prptr;
        int count = 1;
        int maxcount;

        /* dbref strPropDir -- array */
        CHECKOP(2);
        oper2 = POP();
        oper1 = POP();
        if (oper1->type != PROG_OBJECT)
                abort_interp("Dbref required. (1)");
        if (!valid_object(oper1))
                abort_interp("Invalid dbref. (1)");
        if (oper2->type != PROG_STRING)
                abort_interp("String required. (2)");

        ref = oper1->data.objref;
        strcpy(dir, DoNullInd(oper2->data.string));
        if (!*dir)
                strcpy(dir, "/");

        sprintf(propname, "%s#", dir);
        maxcount = get_property_value(ref, propname);
        if (!maxcount) {
                strval = get_property_class(ref, propname);
                if (strval) {
                        maxcount = atoi(strval);
                }
                if (!maxcount) {
                        sprintf(propname, "%s%c#", dir, PROPDIR_DELIMITER);
                        maxcount = get_property_value(ref, propname);
                        if (!maxcount) {
                                strval = get_property_class(ref, propname);
                                if (strval) {
                                        maxcount = atoi(strval);
                                }
                        }
                }
        }
        if (!maxcount) {
                maxcount = 511;
        }

        new = new_array_packed(0);
        while (1) {
                sprintf(propname, "%s#%c%d", dir, PROPDIR_DELIMITER, count);
                prptr = get_property(ref, propname);
                if (!prptr) {
                        sprintf(propname, "%s%c%d", dir, PROPDIR_DELIMITER, count);
                        prptr = get_property(ref, propname);
                        if (!prptr) {
                                sprintf(propname, "%s%d", dir, count);
                                prptr = get_property(ref, propname);
                        }
                }
                if (!prptr || count > maxcount) {
                        break;
                }
                if (prop_read_perms(ProgUID, ref, propname, mlev)) {
#ifdef DISKBASE
                        propfetch(ref, prptr);
#endif
                        switch (PropType(prptr)) {
                        case PROP_STRTYP:
                                temp2.type = PROG_STRING;
                                temp2.data.string = alloc_prog_string(get_uncompress(PropDataStr(prptr)));
                                break;
                        case PROP_LOKTYP:
                                temp2.type = PROG_LOCK;
                                if (PropFlags(prptr) & PROP_ISUNLOADED) {
                                        temp2.data.lock = TRUE_BOOLEXP;
                                } else {
                                        temp2.data.lock = PropDataLok(prptr);
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
                                temp2.type = PROG_INTEGER;
                                temp2.data.number = 0;
                                break;
                        }

                        temp1.type = PROG_INTEGER;
                        temp1.data.number = count;
                        array_appenditem(&new, &temp2);

                        CLEAR(&temp1);
                        CLEAR(&temp2);
                } else {
                        break;
                }
                count++;
        }

        PushArrayRaw(new);
}

void
prim_array_put_propvals(PRIM_PROTOTYPE)
{
        stk_array *arr;
        char propname[BUFFER_LEN];
        char dir[BUFFER_LEN];
        int protoflags;
/*      PData propdat; */

        /* dbref strPropDir array -- */
        CHECKOP(3);
        oper3 = POP();
        oper2 = POP();
        oper1 = POP();
        if (oper1->type != PROG_OBJECT)
                abort_interp("Dbref required. (1)");
        if (!valid_object(oper1))
                abort_interp("Invalid dbref. (1)");
        if (oper2->type != PROG_STRING)
                abort_interp("String required. (2)");
        if (oper3->type != PROG_ARRAY)
                abort_interp("Array required. (3)");

        ref = oper1->data.objref;
        strcpy(dir, DoNullInd(oper2->data.string));
        arr = oper3->data.array;

        if (array_first(arr, &temp1)) {
                do {
                        oper4 = array_getitem(arr, &temp1);
                        switch (temp1.type) {
                        case PROG_STRING:
                                sprintf(propname, "%s%c%s", dir, PROPDIR_DELIMITER,
                                                DoNullInd(temp1.data.string));
                                break;
                        case PROG_INTEGER:
                                sprintf(propname, "%s%c%d", dir, PROPDIR_DELIMITER, temp1.data.number);
                                break;
                        case PROG_FLOAT:
                                sprintf(propname, "%s%c%g", dir, PROPDIR_DELIMITER, temp1.data.fnumber);
                                break;
                        default:
                                *propname = '\0';
                        }

                        if (!prop_write_perms(ProgUID, ref, propname, mlev))
                                abort_interp("Permission denied while trying to set protected property.");

                        switch (oper4->type) {
                        case PROG_STRING:
                                protoflags = PROP_STRTYP;
                                sprintf(buf, "%s", oper4->data.string ? oper4->data.string->data : "");
                                break;
                        case PROG_INTEGER:
                                protoflags = PROP_INTTYP;
                                sprintf(buf, "%d", oper4->data.number);
                                break;
                        case PROG_FLOAT:
                                protoflags = PROP_FLTTYP;
                                sprintf(buf, "%h", oper4->data.fnumber);
                                break;
                        case PROG_OBJECT:
                                protoflags = PROP_REFTYP;
                                sprintf(buf, "%d", oper4->data.objref);
                                break;
                        case PROG_LOCK:
                                protoflags = PROP_LOKTYP;
                                sprintf(buf, "%s", unparse_boolexp(player, copy_bool(oper4->data.lock), 1));
                                break;
                        default:
                                *propname = '\0';
                        }
                        if (*propname) {
                                set_property(ref, propname, protoflags, buf);
                        }
                } while (array_next(arr, &temp1));
        }

        CLEAR(oper1);
        CLEAR(oper2);
        CLEAR(oper3);
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
        int protoflags;

        /* dbref strPropDir array -- */
        CHECKOP(3);
        oper3 = POP();
        oper2 = POP();
        oper1 = POP();
        if (oper1->type != PROG_OBJECT)
                abort_interp("Dbref required. (1)");
        if (!valid_object(oper1))
                abort_interp("Invalid dbref. (1)");
        if (oper2->type != PROG_STRING)
                abort_interp("String required. (2)");
        if (oper3->type != PROG_ARRAY)
                abort_interp("Array required. (3)");
        if (oper3->data.array && oper3->data.array->type != ARRAY_PACKED)
                abort_interp("Argument must be a list type array. (3)");
        ref = oper1->data.objref;
        strcpy(dir, DoNullInd(oper2->data.string));
        arr = oper3->data.array;
        dirlen = strlen(dir);
        fmtout = propname;
        fmtin = tp_proplist_counter_fmt;
/*        fmtin = "P#"; */
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
                abort_interp("Permission denied while trying to set protected property.");
        if (tp_proplist_int_counter) {
                protoflags = PROP_INTTYP;
            sprintf(buf, "%d", array_count(arr));
        } else {
                sprintf(buf, "%d", array_count(arr));
                protoflags = PROP_STRTYP;
        }
        set_property(ref, propname, protoflags, buf);
        if (array_first(arr, &temp1)) {
                do {
                        oper4 = array_getitem(arr, &temp1);
                        fmtout = propname;
                        fmtin = tp_proplist_entry_fmt;
/*                        fmtin = "P#/N"; */
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
                                abort_interp("Permission denied while trying to set protected property.");
                        switch (oper4->type) {
                        case PROG_STRING:
                                protoflags = PROP_STRTYP;
                                sprintf(buf, "%s", oper4->data.string ? oper4->data.string->data : "0");
                                break;
                        case PROG_INTEGER:
                                protoflags = PROP_INTTYP;
                                sprintf(buf, "%d", oper4->data.number);
                                break;
                        case PROG_FLOAT:
                                protoflags = PROP_FLTTYP;
                                sprintf(buf, "%h", oper4->data.fnumber);
                                break;
                        case PROG_OBJECT:
                                protoflags = PROP_REFTYP;
                                sprintf(buf, "%d", oper4->data.objref);
                                break;
                        case PROG_LOCK:
                                protoflags = PROP_LOKTYP;
                                sprintf(buf, "%s", unparse_boolexp(player, copy_bool(oper4->data.lock), 1));
                                break;
                        default:
                                protoflags = PROP_INTTYP;
                                sprintf(buf, "%d", 0);
                        }
                        set_property(ref, propname, protoflags, buf);
                } while (array_next(arr, &temp1));
        }
        count = temp1.data.number;
        for (;;) {
                count++;
                fmtout = propname;
                fmtin = tp_proplist_entry_fmt;
/*              fmtin = "P#/N"; */
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
        CLEAR(oper1);
        CLEAR(oper2);
        CLEAR(oper3);
}


void
prim_array_get_reflist(PRIM_PROTOTYPE)
{
        stk_array *new;
        const char *rawstr;
        char dir[BUFFER_LEN];
        int count = 0;

        /* dbref strPropDir -- array */
        CHECKOP(2);
        oper2 = POP();
        oper1 = POP();
        if (oper1->type != PROG_OBJECT)
                abort_interp("Dbref required. (1)");
        if (!valid_object(oper1))
                abort_interp("Invalid dbref. (1)");
        if (oper2->type != PROG_STRING)
                abort_interp("String required. (2)");
        if (!oper2->data.string)
                abort_interp("Non-null string required. (2)");

        ref = oper1->data.objref;
        strcpy(dir, oper2->data.string->data);

        if (!prop_read_perms(ProgUID, ref, dir, mlev))
                abort_interp(tp_noperm_mesg);

        new = new_array_packed(0);
        rawstr = get_property_class(ref, dir);
        rawstr = get_uncompress(rawstr);

        if (rawstr) {
                while (isspace(*rawstr))
                        rawstr++;
                while (*rawstr) {
                        if (*rawstr == '#')
                                rawstr++;
                        if (!isdigit(*rawstr))
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

                        array_setitem(&new, &temp1, &temp2);
                        count++;

                        CLEAR(&temp1);
                        CLEAR(&temp2);
                }
        }

        PushArrayRaw(new);
}


void
prim_array_put_reflist(PRIM_PROTOTYPE)
{
        stk_array *arr;
        char buf2[BUFFER_LEN];
        char dir[BUFFER_LEN];
        char *out;
        int len;
        int protoflags;

        /* dbref strPropDir array -- */
        CHECKOP(3);
        oper3 = POP();
        oper2 = POP();
        oper1 = POP();
        if (oper1->type != PROG_OBJECT)
                abort_interp("Dbref required. (1)");
        if (!valid_object(oper1))
                abort_interp("Invalid dbref. (1)");
        if (oper2->type != PROG_STRING)
                abort_interp("String required. (2)");
        if (!oper2->data.string)
                abort_interp("Non-null string required. (2)");
        if (oper3->type != PROG_ARRAY)
                abort_interp("Argument must be a list array of dbrefs. (3)");
        if (oper3->data.array && oper3->data.array->type != ARRAY_PACKED)
                abort_interp("Argument must be a list array of dbrefs. (3)");
        if (!array_is_homogenous(oper3->data.array, PROG_OBJECT))
                abort_interp("Argument must be a list array of dbrefs. (3)");

        ref = oper1->data.objref;
        strcpy(dir, DoNullInd(oper2->data.string));

        remove_property(ref, dir);
        arr = oper3->data.array;
        buf[0] = '\0';

        if (!prop_write_perms(ProgUID, ref, dir, mlev))
                abort_interp(tp_noperm_mesg);

        out = buf;
        if (array_first(arr, &temp1)) {
                do {
                        oper4 = array_getitem(arr, &temp1);
                        len = sprintf(buf2, "#%d", oper4->data.objref);

                        if (out + len - buf >= BUFFER_LEN - 3)
                                abort_interp("Operation would result in string length overflow.");

                        if (*buf)
                                *out++ = ' ';
                        strcpy(out, buf2);
                        out += len;
                } while (array_next(arr, &temp1));
        }

        protoflags = PROP_STRTYP;
        set_property(ref, dir, protoflags, buf);

        CLEAR(oper1);
        CLEAR(oper2);
        CLEAR(oper3);
}



void
prim_array_findval(PRIM_PROTOTYPE)
{
        struct inst *in;
        struct inst temp;
        stk_array *arr;
        stk_array *new;
        int found = 0;

        CHECKOP(2);
        oper2 = POP();                          /* ???  index */
        oper1 = POP();                          /* arr  Array */
        if (oper1->type != PROG_ARRAY)
                abort_interp("Argument not an array. (1)");

        new = new_array_packed(0);
        arr = oper1->data.array;
        if (array_first(arr, &temp1)) {
                do {
                        in = array_getitem(arr, &temp1);
                        if (!array_idxcmp(in, oper2)) {
                                array_appenditem(&new, &temp1);
                        }
                } while (!found && array_next(arr, &temp1));
        }

        CLEAR(oper2);
        CLEAR(oper1);

        PushArrayRaw(new);
}

void
prim_array_excludeval(PRIM_PROTOTYPE)
{
        struct inst *in;
        struct inst temp;
        stk_array *arr;
        stk_array *new;
        int found = 0;

        CHECKOP(2);
        oper2 = POP();                          /* ???  index */
        oper1 = POP();                          /* arr  Array */
        if (oper1->type != PROG_ARRAY)
                abort_interp("Argument not an array. (1)");

        new = new_array_packed(0);
        arr = oper1->data.array;
        if (array_first(arr, &temp1)) {
                do {
                        in = array_getitem(arr, &temp1);
                        if (array_idxcmp(in, oper2)) {
                                array_appenditem(&new, &temp1);
                        }
                } while (!found && array_next(arr, &temp1));
        }

        CLEAR(oper2);
        CLEAR(oper1);

        PushArrayRaw(new);
}

void
prim_explode_array(PRIM_PROTOTYPE)
{
    stk_array *new;
    char *tempPtr;
    CHECKOP(2);
    temp1 = *(oper1 = POP());
    temp2 = *(oper2 = POP());
    oper1 = &temp1;
    oper2 = &temp2;
    if (temp1.type != PROG_STRING)
        abort_interp("Non-string argument (2)");
    if (temp2.type != PROG_STRING)
        abort_interp("Non-string argument (1)");
    if (!temp1.data.string)
        abort_interp("Empty string argument (2)");
    {
        int     i;
        const char *delimit = temp1.data.string->data;
        new = new_array_packed(0);  
        if (!temp2.data.string) {
            result = 1;
            CLEAR(&temp1);
            CLEAR(&temp2);
            temp3.type = PROG_STRING;
            temp3.data.string = alloc_prog_string("");
            array_appenditem(&new, &temp3);
            CLEAR(&temp3);
            PushArrayRaw(new);  
            return;
        } else {
            result = 0;
            bcopy(temp2.data.string->data, buf, temp2.data.string->length + 1);
            for (i = temp2.data.string->length - 1; i >= 0; i--) {
                if (!strncmp(buf + i, delimit, temp1.data.string->length)) {
                    buf[i] = '\0';
                    temp3.type = PROG_STRING;
                    tempPtr = (buf + i + temp1.data.string->length);
                    temp3.data.string = alloc_prog_string(tempPtr);
                    array_appenditem(&new, &temp3 ); 
                    result++;
                }
            }
            CHECKOFLOW(1);
            tempPtr = buf;
            temp3.type = PROG_STRING;
            temp3.data.string = alloc_prog_string(tempPtr);
            array_appenditem(&new, &temp3);
            PushArrayRaw(new);
            result++;
        }
    }
    CLEAR(&temp1);
    CLEAR(&temp2);
    CLEAR(&temp3);
} 

