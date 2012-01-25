#include "copyright.h"
#include "config.h"
#include "params.h"
#ifdef JSON_SUPPORT
#include <jansson.h>
#include "db.h"
#include "tune.h"
#include "props.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "strings.h"
#include "interp.h"
#include "json.h"
#include "p_json.h"

extern struct inst *oper1, *oper2, *oper3, *oper4, *oper5, *oper6;
extern struct inst temp1;

void
prim_array_jencode(PRIM_PROTOTYPE) {
    json_t *newjson;
    char *jsontext;
    int flags;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (oper1->type != PROG_INTEGER) {
        abort_interp("Argument not an integer. (1)");
    }
    if (oper1->data.number < 0) {
        abort_interp("Argument is a negative number. (1)");
    }
    if (oper2->type != PROG_ARRAY) {
        abort_interp("Argument not an array. (2)");
    }
    /* type of array doesn't matter */

    flags = oper1->data.number;
    newjson = array_to_json(oper2->data.array, player);

    if (!newjson) {
        /* Hardcode error. User shouldn't be able to break the parser. */
        abort_interp("Internal error during array parse.");
    }
    
    jsontext = json_dumps(newjson, flags);

    if (!jsontext) {
        /* Definitely a hardcode error. Parse succeeded but export failed. */
        abort_interp("Internal error during string dump.");
    }

    CLEAR(oper1);
    CLEAR(oper2);

    PushString(jsontext);
}

void
prim_array_jdecode(PRIM_PROTOTYPE) {
    stk_array *strarr, *jsonarr, *errarr;
    json_t *newjson;
    struct inst *in;
    char buf[BUFFER_LEN * 2];
    int flags;
    json_error_t error;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();

    if (oper1->type != PROG_INTEGER) {
        abort_interp("Argument not an integer. (1)");
    }
    if (oper1->data.number < 0) {
        abort_interp("Argument is a negative number. (1)");
    }
    if (oper2->type != PROG_ARRAY) {
        abort_interp("Argument not an array of strings. (2)");
    }
    if (!array_is_homogenous(oper2->data.array, PROG_STRING)) {
        abort_interp("Argument not an array of strings. (2)");
    }

    flags = oper1->data.number;
    strarr = oper2->data.array;

    if (array_first(strarr, &temp1)) {
       buf[0] = '\0';
       do {
           /* REDO: rework this to be less lazy later */
           in = array_getitem(strarr, &temp1);
           sprintf(buf, "%s%s\n", buf, DoNullInd(in->data.string));
       } while (array_next(strarr,&temp1));
    } else {
        abort_interp("Argument is an empty array. (1)");
    }

    newjson = json_loads(buf, flags, &error);

    if (newjson) {
        jsonarr = json_to_array(newjson, 2);
        errarr = new_array_dictionary();
    } else {
        jsonarr = new_array_dictionary();
        errarr = get_json_error(&error);
    }

    json_decref(newjson);
    CLEAR(oper1);
    CLEAR(oper2);
    PushArrayRaw(jsonarr);
    PushArrayRaw(errarr);
}




#ifdef FILE_PRIMS

/* decode a JSON encoded file to an array (based on prim_fread) */
void
prim_jdecode_file(PRIM_PROTOTYPE)
{
    char *filename;
    int flags;
    json_t *newjson;
    json_error_t error;
    stk_array *jsonarr, *errarr;
    struct inst *in;

    CHECKOP(2);
    oper1 = POP();   /* int: JSON decoding options */
    oper2 = POP();   /* str: path to file */

#ifndef PROTO_AS_ROOT
        if (getuid() == 0) {
                    abort_interp("Muck is running under root privs, file prims disabled.");
        }
#endif
    if (mlev < LBOY) {
        abort_interp("BOY primitive only.");
    }
    if (oper1->type != PROG_INTEGER) {
        abort_interp("Argument not an integer. (1)");
    }
    if (oper1->data.number < 0) {
        abort_interp("Argument is a negative number. (1)");
    }
    if (oper2->type != PROG_STRING) {
        abort_interp("Argument not a string. (2)");
    }
    if (!oper2->data.string) {
        abort_interp("Argument is a null string. (2)");
    }
    flags = oper1->data.number;
    filename = oper2->data.string->data;
#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(filename))) {
        abort_interp("Invalid file name.");
    }
    if (strchr(filename, '$') == NULL) {
        filename = set_directory(filename);
    } else {
        filename = parse_token(filename);
    }
    if (filename == NULL) {
        abort_interp("Invalid shortcut used.");
    }
#endif
    newjson = json_load_file(filename, flags, &error);
    
    if (newjson) {
        jsonarr = json_to_array(newjson, 2);
        errarr = new_array_dictionary();
    } else {
        jsonarr = new_array_dictionary();
        errarr = get_json_error(&error);

        /* not sure why linecount is off by one when importing from file,
           but this is where we fix it. */
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("line");
        in = array_getitem(errarr, &temp1);
        in->data.number--;
        CLEAR(&temp1);
    }

    json_decref(newjson);
    CLEAR(oper1);
    CLEAR(oper2);
    PushArrayRaw(jsonarr);
    PushArrayRaw(errarr);
}
    
/* encode an array as JSON and write to a file */
void
prim_jencode_file(PRIM_PROTOTYPE)
{
    char *filename = NULL;
    int flags, success;
    json_t *jsonout;

    CHECKOP(3);
    oper1 = POP();  /* arr: array to encode */ 
    oper2 = POP();  /* str: path to file */ 
    oper3 = POP();  /* int: JSON decoding options */ 


#ifndef PROTO_AS_ROOT
        if (getuid() == 0) {
                    abort_interp("Muck is running under root privs, file prims disabled.");
        }
#endif
    if (mlev < LBOY) {
        abort_interp("BOY primitive only.");
    }
    if (oper1->type != PROG_INTEGER) {
        abort_interp("Argument not an integer. (1)");
    }
    if (oper1->data.number < 0) {
        abort_interp("Argument is a negative number. (1)");
    }
    if (oper2->type != PROG_STRING) {
        abort_interp("Argument not a string. (2)");
    }
    if (!oper2->data.string) {
        abort_interp("Argument is a null string. (2)");
    }
    if (oper3->type != PROG_ARRAY) {
        abort_interp("Argument not an array. (3)");
    }
    /* type of array doesn't matter */

    flags = oper1->data.number;
    filename = oper2->data.string->data;
#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(filename))) {
        abort_interp("Invalid file name.");
    }
    if (strchr(filename, '$') == NULL) {
        filename = set_directory(filename);
    } else {
        filename = parse_token(filename);
    }
    if (filename == NULL) {
        abort_interp("Invalid shortcut used.");
    }
#endif
    jsonout = array_to_json(oper3->data.array, player);

    success = json_dump_file(jsonout, filename, flags);

    temp1.type = PROG_INTEGER;
    if (success == 0) {
        temp1.data.number = 1;
    } else {
        temp1.data.number = 0;
    }

    json_decref(jsonout);
    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushInt(temp1);
}

#endif /* FILE_PRIMS */

#endif /* JSON_SUPPORT */
