#include "copyright.h"
#include "config.h"

#ifdef MUF_EDIT_PRIMS

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include "db.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "mufevent.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "strings.h"
#include "interp.h"

extern struct inst *oper1, *oper2, *oper3, *oper4, *oper5, *oper6;
extern struct inst temp1, temp2, temp3;
extern int tmp, result;
extern dbref ref;
extern char buf[BUFFER_LEN];
struct tm *time_tm;

/* Some externs to functions elsewhere in the code. */
extern int kill_macro(const char *, dbref, struct macrotable **);

void
prim_kill_macro(PRIM_PROTOTYPE)
{
    /* name -- result */
    char tmp[BUFFER_LEN];

    result = 0;
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument. (1)");
    if (!oper1->data.string)
        abort_interp("Null argument. (1)");
    if (mlev < LWIZ)
        abort_interp("Permission denied.");

    strcpy(tmp, (const char *) oper1->data.string->data);
    CLEAR(oper1);
    result = kill_macro(tmp, player, &macrotop);

    PushInt(result);
}

extern int insert_macro(const char *, const char *, dbref,
                        struct macrotable **);

void
prim_insert_macro(PRIM_PROTOTYPE)
{
    /* name def -- result */
    int result = 0;
    char namebuf[BUFFER_LEN];
    char defbuf[BUFFER_LEN];

    CHECKOP(2);
    oper1 = POP();              /* definition */
    oper2 = POP();              /* macro name */

    if (oper1->type != PROG_STRING || oper2->type != PROG_STRING)
        abort_interp("Requires two string arguments.");
    if (!oper1->data.string || !oper1->data.string)
        abort_interp("Arguments may not be empty strings.");
    if (mlev < LWIZ)
        abort_interp("Permission denied.");

    strcpy(namebuf, (const char *) oper2->data.string->data);
    strcpy(defbuf, (const char *) oper1->data.string->data);
    CLEAR(oper1);
    CLEAR(oper2);
    result = insert_macro(namebuf, defbuf, player, &macrotop);

    PushInt(result);
}

stk_array *
make_macros_array(stk_array *dict, struct macrotable *node)
{
    if (!node)
        return 0;

    make_macros_array(dict, node->left);
    array_set_strkey_strval(&dict, node->name, node->definition);
    make_macros_array(dict, node->right);
    return dict;

}

void
prim_get_macros_array(PRIM_PROTOTYPE)
{
    stk_array *nw;

    /* -- dict<full macro table> */

    if (mlev < LMAGE)
        abort_interp("Permission denied.");

    CHECKOFLOW(1);

    nw = new_array_dictionary();
    make_macros_array(nw, macrotop);

    PushArrayRaw(nw);
}

void
prim_program_linecount(PRIM_PROTOTYPE)
{
    struct line *curr;

    /* dbref -- int */
    CHECKOP(1);
    oper1 = POP();              /* dbref */

    if (!valid_object(oper1))
        abort_interp("Non-object argument. (1)");
    ref = oper1->data.objref;
    if (Typeof(ref) != TYPE_PROGRAM)
        abort_interp("Object must be a program object. (1)");

    if (mlev < LWIZ && (!controls(player, ref) && !Viewable(ref)))
        abort_interp("Permission denied.");

    if (!(FLAGS(ref) & INTERNAL)) /* no one's editing it */
        DBSTORE(ref, sp.program.first, read_program(ref));

    curr = DBFETCH(ref)->sp.program.first;
    result = 0;

    while (curr) {
        result++;
        curr = curr->next;
    }

    if (!(FLAGS(ref) & INTERNAL))
        free_prog_text(DBFETCH(ref)->sp.program.first);

    CLEAR(oper1);
    PushInt(result);
}


void
prim_program_getlines(PRIM_PROTOTYPE)
{
    stk_array *ary;
    int start = 0;
    int end = 0;
    int i, count;
    struct line *curr;          /* current line */
    struct line *first;         /* first line in program */
    struct line *segment;       /* starting line in our segment of interest */

    /* dbref start stop -- arr */
    CHECKOP(3);
    oper3 = POP();              /* stop */
    oper2 = POP();              /* start */
    oper1 = POP();              /* dbref */

    if (!valid_object(oper1))
        abort_interp("Invalid object dbref (1).");
    if (oper2->type != PROG_INTEGER)
        abort_interp("Expected integer. (2)");
    if (oper3->type != PROG_INTEGER)
        abort_interp("Expected integer. (3)");

    ref = oper1->data.objref;
    start = oper2->data.number;
    end = oper3->data.number;

    if (Typeof(ref) != TYPE_PROGRAM)
        abort_interp("Non-program object. (1)");
    if (mlev < LWIZ && !controls(ProgUID, ref) && !Viewable(ref))
        abort_interp("Permission denied.");
    if (FLAG2(ref) & F2PROTECT && !controls(ProgUID, ref))
        abort_interp("Program is set PROTECT.");
    if (start < 0 || end < 0)
        abort_interp("Line indexes must be non-negative.");

    if (start == 0)
        start = 1;
    if (end && start > end)
        abort_interp("Illogical line range.");

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    curr = first = read_program(ref); /* load the code */

    /* find our line */
    for (i = 1; curr && i < start; ++i)
        curr = curr->next;

    if (!curr) {                /* no lines to list */
        free_prog_text(first);
        PushNullArray;
        return;
    }

    segment = curr;             /* need to keep segment line for after count */

    for (; curr && (!end || i < end); ++i)
        curr = curr->next;

    count = i - start + 1;

    if (!curr)                  /* if we don't have a curr we counted 1 beyond end
                                 * of the program, so back up one in the count. */
        --count;
    ary = new_array_packed(count);
    for (curr = segment, i = 0; count--; ++i, curr = curr->next)
        array_set_intkey_strval(&ary, i, curr->this_line);

    free_prog_text(first);

    PushArrayRaw(ary);
}

void
prim_program_deletelines(PRIM_PROTOTYPE)
{
    dbref theprog;
    struct line *curr;
    struct line *temp;
    int start = 0;
    int end = 0;
    int count = 0;
    int i = 0;

    /* ref start end -- i<lines actually deleted> */
    CHECKOP(3);
    oper3 = POP();              /* end */
    oper2 = POP();              /* start */
    oper1 = POP();              /* program */

    if (mlev < LBOY)
        abort_interp("Permission denied.");
    if (oper3->type != PROG_INTEGER || oper2->type != PROG_INTEGER)
        abort_interp("The range arguments need to be integers. (2) (3)");
    if (!valid_object(oper1))
        abort_interp("Expected a dbref. (1)");
    theprog = oper1->data.objref;
    start = oper2->data.number;
    end = oper3->data.number;
    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);

    if (Typeof(theprog) != TYPE_PROGRAM)
        abort_interp("Object must be a program. (1)");
    if (start < 1 || end < 1 || end < start)
        abort_interp("Range to delete doesn't make sense. (2) (3)");
    if (FLAGS(theprog) & INTERNAL)
        abort_interp("That program is currently being edited.");

    DBSTORE(theprog, sp.program.first, read_program(theprog));

    i = start - 1;              /* array offset for first line to delete */
    for (curr = DBFETCH(theprog)->sp.program.first; curr && i; --i)
        curr = curr->next;      /* move to the first line to delete */

    if (!curr) {                /* line doesn't exist, free code and return 0 */
        count = 0;
        free_prog_text(DBFETCH(theprog)->sp.program.first);
        PushInt(count);
        return;
    }

    i = end - start + 1;        /* number of lines to delete */

    while (i && curr) {
        temp = curr;
        if (curr->prev)
            curr->prev->next = curr->next; /* relink prev line */
        else                    /* or relink start of program */
            DBFETCH(theprog)->sp.program.first = curr->next;
        if (curr->next)
            curr->next->prev = curr->prev;
        curr = curr->next;      /* move curr to next line */
        free_line(temp);        /* finally release the current line */
        --i;
        ++count;
    }                           /* Write and free program, push result */
    write_program(DBFETCH(theprog)->sp.program.first, theprog);
    free_prog_text(DBFETCH(theprog)->sp.program.first);
    DBFETCH(theprog)->sp.program.first = NULL;

    PushInt(count);

}

void
prim_program_insertlines(PRIM_PROTOTYPE)
{
    dbref theprog;
    int i = 0;
    struct line *curr;
    struct line *prev = NULL;
    struct line *new_line;
    int start = 0;
    stk_array *lines;
    struct inst temp1;
    int endline = 0;
    int replacedFirst = 0;      /* this keeps us from inserting at first over */

    /* ref i<start> arr<lines> */
    CHECKOP(3);
    oper3 = POP();              /* lines */
    oper2 = POP();              /* start line */
    oper1 = POP();              /* program ref */

    if (mlev < LBOY)
        abort_interp("Permission denied.");
    if (!valid_object(oper1))
        abort_interp("Program dbref expected. (1)");
    if (oper2->type != PROG_INTEGER)
        abort_interp("Integer start line expected. (2)");
    if (oper3->type != PROG_ARRAY)
        abort_interp("Argument not an array of strings. (3)");
    if (!array_is_homogenous(oper3->data.array, PROG_STRING))
        abort_interp("Argument is not an array of all strings. (3)");

    start = oper2->data.number;
    if (start < 1)
        abort_interp("Start point must be greater than 0.");
    theprog = oper1->data.objref;
    CLEAR(oper1);
    CLEAR(oper2);
    lines = oper3->data.array;
    if (Typeof(theprog) != TYPE_PROGRAM)
        abort_interp("Object must be a program. (1)");
    if (FLAGS(theprog) & INTERNAL)
        abort_interp("This program is already being edited. (1)");
    if (start < 1)
        abort_interp("Start line must be greater than 0. (2)");

    DBSTORE(theprog, sp.program.first, read_program(theprog));
    i = start - 1;              /*offset in array to insert lines */
    DBFETCH(theprog)->sp.program.curr_line = 1;
    for (curr = DBFETCH(theprog)->sp.program.first; curr && i; --i) {
        prev = curr;
        DBFETCH(theprog)->sp.program.curr_line++;
        curr = curr->next;      /* move to the insert point */
    }

    if (array_first(lines, &temp1)) {
        do {
            new_line = get_new_line(); /* allocate new line */
            oper4 = array_getitem(lines, &temp1);
            if (oper4->data.string)
                new_line->this_line = alloc_string(oper4->data.string->data);
            else
                new_line->this_line = alloc_string(" ");
            oper4 = NULL;
            if (!DBFETCH(theprog)->sp.program.first) { /* no code yet */
                DBFETCH(theprog)->sp.program.curr_line = 1;
                DBFETCH(theprog)->sp.program.first = new_line;
                prev = DBFETCH(theprog)->sp.program.first;
                curr = NULL;    /* curr = NULL so we keep inserting at the end */
                continue;
            }
            if (!curr) {        /* insert at the end */
                DBFETCH(theprog)->sp.program.curr_line++;
                new_line->prev = prev;
                prev->next = new_line;
                prev = new_line;
                curr = NULL;    /* Keep curr = NULL to keep inserting at the end */
                continue;
            }
            if (!curr->prev && !replacedFirst) { /*inserting at the beginning */
                DBFETCH(theprog)->sp.program.curr_line = 1;
                curr->prev = new_line;
                new_line->next = curr;
                DBFETCH(theprog)->sp.program.first = new_line;
                curr = new_line; /* move curr to insert after it next */
                replacedFirst = 1;
                continue;
            }
            /* inserting in the middle */
            DBFETCH(theprog)->sp.program.curr_line++;
            new_line->prev = curr;
            new_line->next = curr->next;
            if (new_line->next)
                new_line->next->prev = new_line;
            curr->next = new_line;
            curr = new_line;    /* move curr to insert after it next */
        } while (array_next(lines, &temp1));
    }
    log_status("PROGRAM EDITED: %s by %s(%d)\n",
               unparse_object(PSafe, theprog),
               OkObj(player) ? NAME(player) : "(login)", player);
    if (tp_log_programs)
        log_program_text(DBFETCH(theprog)->sp.program.first, player,
                         oper1->data.objref);
    endline = DBFETCH(theprog)->sp.program.curr_line;
    CLEAR(oper3);
    write_program(DBFETCH(theprog)->sp.program.first, theprog);
    free_prog_text(DBFETCH(theprog)->sp.program.first);
    DBFETCH(theprog)->sp.program.first = NULL;
    DBDIRTY(program);
    PushInt(endline);
}

#endif /* MUF_EDIT_PRIMS */
