/* Primitives package */
#include "copyright.h"
#include "config.h"
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
extern struct line *read_program(dbref i);
extern int tune_setparm(const dbref player, const char *parmname,
                        const char *val);
extern struct frame* aForceFrameStack[9];
void
prim_time(PRIM_PROTOTYPE)
{
    struct tm *time_tm;
	int result;
    
    CHECKOFLOW(3);
    {
        time_t lt;
        lt = time(NULL);
        time_tm = localtime(&lt);
        result = time_tm->tm_sec;
        PushInt(result);
        result = time_tm->tm_min;
        PushInt(result);
        result = time_tm->tm_hour;
        PushInt(result);
    }
}
void
prim_date(PRIM_PROTOTYPE)
{
    struct tm *time_tm;
	int result;
    
    CHECKOFLOW(3);
    {
        time_t lt;
        lt = time(NULL);
        time_tm = localtime(&lt);
        result = time_tm->tm_mday;
        PushInt(result);
        result = time_tm->tm_mon + 1;
        PushInt(result);
        result = time_tm->tm_year + 1900;
        PushInt(result);
    }
}
void
prim_gmtoffset(PRIM_PROTOTYPE)
{
	int result;
    
    CHECKOFLOW(1);
    result = (int)get_tz_offset();
    PushInt(result);
}
void
prim_systime(PRIM_PROTOTYPE)
{
	int result;
    
    result = (int)time(NULL);
    CHECKOFLOW(1);
    PushInt(result);
}
void
prim_timesplit(PRIM_PROTOTYPE)
{
    struct tm *time_tm;
	int result;
    time_t tt;
    
                  /* integer: time */
    if (oper[0].type != PROG_INTEGER)
        abort_interp("Invalid argument");
    tt = (time_t)oper[0].data.number;
    time_tm = localtime(&tt);
    CHECKOFLOW(8);
    
    result = time_tm->tm_sec;
    PushInt(result);
    result = time_tm->tm_min;
    PushInt(result);
    result = time_tm->tm_hour;
    PushInt(result);
    result = time_tm->tm_mday;
    PushInt(result);
    result = time_tm->tm_mon + 1;
    PushInt(result);
    result = time_tm->tm_year + 1900;
    PushInt(result);
    result = time_tm->tm_wday + 1;
    PushInt(result);
    result = time_tm->tm_yday + 1;
    PushInt(result);
}
void
prim_timefmt(PRIM_PROTOTYPE)
{
    struct tm *time_tm;
	char buf[BUFFER_LEN];
    time_t tt;
    
                  /* integer: time */
                  /* string: format */
    if (oper[1].type != PROG_STRING)
        abort_interp("Invalid argument (1)");
    if (oper[1].data.string == (struct shared_string *) NULL)
        abort_interp("Illegal NULL string (1)");
    if (oper[0].type != PROG_INTEGER)
        abort_interp("Invalid argument (2)");
    tt = (time_t)oper[0].data.number;
    time_tm = localtime(&tt);
    if (!format_time(buf, BUFFER_LEN, oper[1].data.string->data, time_tm))
        abort_interp("Operation would result in overflow");
    CHECKOFLOW(1);
    
    PushString(buf);
}
void
prim_queue(PRIM_PROTOTYPE)
{
    dbref temproom;
	int result;
	struct inst *oper3;
    /* int dbref string -- */
    
    if (mlev < LM3)
        abort_interp("M3 prim");
    if (oper[2].type != PROG_INTEGER)
        abort_interp("Non-integer argument (1)");
    if (oper[1].type != PROG_OBJECT)
        abort_interp("Argument must be a dbref (2)");
    if (!valid_object(&oper[1]))
        abort_interp("Invalid dbref (2)");
    if (Typeof(oper[1].data.objref) != TYPE_PROGRAM)
        abort_interp("Object must be a program (2)");
    if (oper[0].type != PROG_STRING)
        abort_interp("Non-string argument (3)");
    if ((oper3 = fr->variables + 1)->type != PROG_OBJECT)
        temproom = DBFETCH(PSafe)->location;
    else
        temproom = oper3->data.objref;
    result =
        add_muf_delayq_event(oper[2].data.number, fr->descr, player, temproom,
                             NOTHING, oper[1].data.objref,
                             DoNullInd(oper[0].data.string), "Queued Event.", 0);
    
    PushInt(result);
}
void
prim_enqueue(PRIM_PROTOTYPE)
{
    dbref temproom;
	int result;
	struct inst *oper4;
    /* int dbref string -- */
    
    if (mlev < LARCH)
        abort_interp("W3 prim");
    if (oper[2].type != PROG_INTEGER && oper[2].type != PROG_STRING)
        abort_interp("Argument must be integer or string (1)");
    if (oper[1].type != PROG_OBJECT)
        abort_interp("Argument must be a dbref (2)");
    if (!valid_object(&oper[1]))
        abort_interp("Invalid dbref (2)");
    if (Typeof(oper[1].data.objref) != TYPE_PROGRAM)
        abort_interp("Object must be a program (2)");
    if (oper[0].type != PROG_STRING)
        abort_interp("Non-string argument (3)");
    if (Typeof(oper[3].data.objref) != TYPE_PLAYER && Typeof(oper[3].data.objref) != TYPE_THING)
        abort_interp("Object must be a player or thing. (4)");
    if ((oper4 = fr->variables + 1)->type != PROG_OBJECT)
        temproom = DBFETCH(PSafe)->location;
    else
        temproom = oper4->data.objref;
    if (oper[2].type == PROG_INTEGER) {
    result =
        add_muf_delayq_event(oper[2].data.number, dbref_first_descr(oper[3].data.objref), oper[3].data.objref, temproom,
                             NOTHING, oper[1].data.objref,
                             DoNullInd(oper[0].data.string), "Queued Event.", 0);
    } else {
    result =
        add_muf_delayq_event(0, dbref_first_descr(oper[3].data.objref), oper[3].data.objref, temproom,
                             NOTHING, oper[1].data.objref,
                             DoNullInd(oper[0].data.string), DoNullInd(oper[2].data.string), 0);
    }
    
    PushInt(result);
}
void
prim_kill(PRIM_PROTOTYPE)
{
    /* i -- i */
	int result;
    
    if (oper[0].type != PROG_INTEGER)
        abort_interp("Non-integer argument (1)");
    if (oper[0].data.number == fr->pid) {
        
        do_abort_silent(fr);
    } else {
        if (mlev < LMAGE) {
            if (!control_process(ProgUID, oper[0].data.number)) {
                abort_interp(tp_noperm_mesg);
            }
        }
        result = dequeue_process(oper[0].data.number);
        
    }
    PushInt(result);
}
void
prim_timestamps(PRIM_PROTOTYPE)
{
	int result;
	dbref ref;
    
    if (mlev < LM2)
        abort_interp("M2 prim");
    if (oper[0].type != PROG_OBJECT)
        abort_interp("Non-object argument (1)");
    ref = oper[0].data.objref;
    if (ref >= db_top || ref <= NOTHING)
        abort_interp("Dbref is not an object nor garbage.");
    CHECKREMOTE(ref);
    CHECKOFLOW(4);
    
    result = (int)DBFETCH(ref)->ts.created;
    PushInt(result);
    result = (int)DBFETCH(ref)->ts.modified;
    PushInt(result);
    result = (int)DBFETCH(ref)->ts.lastused;
    PushInt(result);
    result = DBFETCH(ref)->ts.usecount;
    PushInt(result);
}
void
prim_refstamps(PRIM_PROTOTYPE)
{
	int result;
	dbref ref;
    
    if (mlev < LM2)
        abort_interp("M2 prim");
    if (oper[0].type != PROG_OBJECT)
        abort_interp("Non-object argument (1)");
    ref = oper[0].data.objref;
    if (ref >= db_top || ref <= NOTHING)
        abort_interp("Dbref is not an object nor garbage.");
    CHECKREMOTE(ref);
    CHECKOFLOW(3);
    
    result = DBFETCH(ref)->ts.dcreated;
    PushObject(result);
    result = DBFETCH(ref)->ts.dmodified;
    PushObject(result);
    result = DBFETCH(ref)->ts.dlastused;
    PushObject(result);
}
void
prim_touch(PRIM_PROTOTYPE)
{
	dbref ref;
    
    if (mlev < LM3)
        abort_interp("M3 prim");
    if (oper[0].type != PROG_OBJECT)
        abort_interp("Non-object argument (1)");
    ref = oper[0].data.objref;
    if (ref >= db_top || ref <= NOTHING)
        abort_interp("Dbref is not an object nor garbage.");
    CHECKREMOTE(ref);
    ts_useobject(program, ref);
    
}
void
prim_use(PRIM_PROTOTYPE)
{
	dbref ref;
    
    if (mlev < LWIZ)
        abort_interp("W2 prim");
    if (oper[0].type != PROG_OBJECT)
        abort_interp("Non-object argument (1)");
    ref = oper[0].data.objref;
    if (ref >= db_top || ref <= NOTHING)
        abort_interp("Dbref is not an object nor garbage.");
    CHECKREMOTE(ref);
    ts_lastuseobject(program, ref);
    
}
extern int top_pid;
struct forvars *copy_fors(struct forvars *);
struct tryvars *copy_trys(struct tryvars *);
void
prim_fork(PRIM_PROTOTYPE)
{
    int i, result;
    struct frame *tmpfr;
    
    CHECKOFLOW(1);
    if (mlev < LMAGE)
        abort_interp("Mage prim.");
    fr->pc = pc;
    tmpfr = (struct frame *) calloc(1, sizeof(struct frame));
    tmpfr->next = NULL;
    tmpfr->system.top = fr->system.top;
    for (i = 0; i < fr->system.top; i++)
        tmpfr->system.st[i] = fr->system.st[i];
    tmpfr->argument.top = fr->argument.top;
    for (i = 0; i < fr->argument.top; i++)
        copyinst(&fr->argument.st[i], &tmpfr->argument.st[i]);
    tmpfr->caller.top = fr->caller.top;
    for (i = 0; i <= fr->caller.top; i++) {
        tmpfr->caller.st[i] = fr->caller.st[i];
        if (i > 0)
            DBFETCH(fr->caller.st[i])->sp.program.instances++;
    }
    tmpfr->trys.top = fr->trys.top;
    tmpfr->trys.st = copy_trys(fr->trys.st);
    tmpfr->fors.top = fr->fors.top;
    tmpfr->fors.st = copy_fors(fr->fors.st);
    for (i = 0; i < MAX_VAR; i++)
        copyinst(&fr->variables[i], &tmpfr->variables[i]);
    localvar_dupall(tmpfr, fr);
    scopedvar_dupall(tmpfr, fr);
    tmpfr->error.is_flags = fr->error.is_flags;
    if (fr->rndbuf) {
        tmpfr->rndbuf = (void *) malloc(sizeof(unsigned long) * 4);
        if (tmpfr->rndbuf) {
            memcpy(tmpfr->rndbuf, fr->rndbuf, 16);
        }
    } else {
        tmpfr->rndbuf = NULL;
    }
    tmpfr->pc = pc;
    tmpfr->pc++;
    tmpfr->level = fr->level;
    tmpfr->already_created = fr->already_created;
    tmpfr->trig = fr->trig;
    tmpfr->brkpt.debugging = 0;
    tmpfr->brkpt.bypass = 0;
    tmpfr->brkpt.isread = 0;
    tmpfr->brkpt.showstack = 0;
    tmpfr->brkpt.lastline = 0;
    tmpfr->brkpt.lastpc = 0;
    tmpfr->brkpt.lastlisted = 0;
    tmpfr->brkpt.lastcmd = NULL;
    tmpfr->brkpt.breaknum = -1;
    tmpfr->brkpt.lastproglisted = NOTHING;
    tmpfr->brkpt.proglines = NULL;
    tmpfr->brkpt.count = 1;
    tmpfr->brkpt.temp[0] = 1;
    tmpfr->brkpt.level[0] = -1;
    tmpfr->brkpt.line[0] = -1;
    tmpfr->brkpt.linecount[0] = -2;
    tmpfr->brkpt.pc[0] = NULL;
    tmpfr->brkpt.pccount[0] = -2;
    tmpfr->brkpt.prog[0] = program;
    tmpfr->proftime.tv_sec = 0;
    tmpfr->proftime.tv_usec = 0;
    tmpfr->totaltime.tv_sec = 0;
    tmpfr->totaltime.tv_usec = 0;
    tmpfr->prog = program;
    tmpfr->pid = top_pid++;
    tmpfr->multitask = BACKGROUND;
    tmpfr->been_background = 1;
    tmpfr->writeonly = 1;
    tmpfr->started = time(NULL);
    tmpfr->instcnt = 0;
    tmpfr->skip_declare = fr->skip_declare;
    tmpfr->wantsblanks = fr->wantsblanks;
    tmpfr->perms = fr->perms;
    tmpfr->descr = fr->descr;
    tmpfr->events = NULL;
    tmpfr->waiters = NULL;
    tmpfr->waitees = NULL;
    tmpfr->dlogids = NULL;
    tmpfr->timercount = 0;
    /* child process gets a 0 returned on the stack */
    result = 0;
    push(tmpfr->argument.st, &(tmpfr->argument.top), PROG_INTEGER,
         MIPSCAST & result);
    result = add_muf_delay_event(0, fr->descr, player, NOTHING, NOTHING,
                                 program, tmpfr, "BACKGROUND");
    /* parent process gets the child's pid returned on the stack */
    if (!result)
        result = -1;
    PushInt(result);
}
void
prim_pid(PRIM_PROTOTYPE)
{
	int result;
    
    CHECKOFLOW(1);
    result = fr->pid;
    PushInt(result);
}
void
prim_stats(PRIM_PROTOTYPE)
{
	dbref ref;
    /* A WhiteFire special. :) */
    
    if (mlev < LM2)
        abort_interp("M2 prim");
    if (!valid_player(&oper[0]) && (oper[0].data.objref != NOTHING))
        abort_interp("non-player argument (1)");
    ref = oper[0].data.objref;
    
    {
        dbref i;
        int rooms, exits, things, players, programs, garbage;
        /* tmp, ref */
        rooms = exits = things = players = programs = garbage = 0;
        for (i = 0; i < db_top; i++) {
            if (ref == NOTHING || OWNER(i) == ref) {
                switch (Typeof(i)) {
                    case TYPE_ROOM:
                        rooms++;
                        break;
                    case TYPE_EXIT:
                        exits++;
                        break;
                    case TYPE_THING:
                        things++;
                        break;
                    case TYPE_PLAYER:
                        players++;
                        break;
                    case TYPE_PROGRAM:
                        programs++;
                        break;
                    case TYPE_GARBAGE:
                        garbage++;
                        break;
                }
            }
        }
        CHECKOFLOW(7);
        ref = rooms + exits + things + players + programs + garbage;
        PushInt(ref);
        PushInt(rooms);
        PushInt(exits);
        PushInt(things);
        PushInt(programs);
        PushInt(players);
        PushInt(garbage);
        /* push results */
    }
}
void
prim_abort(PRIM_PROTOTYPE)
{
	char buf[BUFFER_LEN];
    
    if (oper[0].type != PROG_STRING)
        abort_interp("Invalid argument");
    strcpy(buf, DoNullInd(oper[0].data.string));
    abort_interp(buf);
}
void
prim_ispidp(PRIM_PROTOTYPE)
{
	int result;
    int nCurFr = 0; 
    /* i -- i */
    
    if (oper[0].type != PROG_INTEGER)
        abort_interp("Non-integer argument (1)");
    if (oper[0].data.number == fr->pid) {
        result = 1;
    } else if ( force_level > 0 ) {
        /* if this was called from a FORCE   
         * search the global force processes stack
         * to see if our PID is in there instead 
         */
        for ( ; nCurFr < 9; ++nCurFr ) {
            if ( aForceFrameStack[nCurFr]) {
                if ( aForceFrameStack[nCurFr]->pid == oper[0].data.number ) {
                    result = 1;
                }
            }
        }
    } else {
        result = in_timequeue(oper[0].data.number);
    }
    
    PushInt(result);
}
void
prim_getpids(PRIM_PROTOTYPE)
{
    stk_array *nw;
	struct inst temp1;
    
    if (mlev < LARCH)
        abort_interp("Archwizard prim.");
    if (oper[0].type != PROG_OBJECT)
        abort_interp("Non-object argument (1)");
    nw = get_pids(oper[0].data.objref);
    if (oper[0].data.objref == -1 || oper[0].data.objref == program) {
        /* We need to include current PID instance too */
        temp1.type = PROG_INTEGER;
        temp1.data.number = fr->pid;
        array_appenditem(&nw, &temp1);
        CLEAR(&temp1);
    }
    
    PushArrayRaw(nw);
}
void
prim_getpidinfo(PRIM_PROTOTYPE)
{
    stk_array *nw;
    struct inst temp1, temp2;
    time_t rtime = current_systime;
    time_t etime = 0;
    double pcnt = 0.0;
    /* int */
    
    if (mlev < LARCH)
        abort_interp("Archwizard prim.");
    if (oper[0].type != PROG_INTEGER)
        abort_interp("Non-integer argument (1)");
    /* This is kind of hacky. Basically, if we are getting the 
     * current program's info, we need to do it here, since it
     * doesn't exist in the timequeue. Otherwise, we need to
     * defer to the timequeue functions for info
     */
    if (oper[0].data.number == fr->pid) {
        nw = new_array_dictionary();
        etime = rtime - fr->started;
        if (etime > 0) {
            pcnt = fr->totaltime.tv_sec;
            pcnt += fr->totaltime.tv_usec / 1000000;
            pcnt = pcnt * 100 / etime;
            if (pcnt > 100.0)
                pcnt = 100.0;
        } else
            pcnt = 0.0;
        temp1.type = PROG_STRING; /* PID */
        temp1.data.string = alloc_prog_string("PID");
        temp2.type = PROG_INTEGER;
        temp2.data.number = fr->pid;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING; /* CALLED_PROG */
        temp1.data.string = alloc_prog_string("CALLED_PROG");
        temp2.type = PROG_OBJECT;
        temp2.data.objref = fr->prog;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING; /* TRIG */
        temp1.data.string = alloc_prog_string("TRIG");
        temp2.type = PROG_OBJECT;
        temp2.data.objref = fr->trig;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING; /* PLAYER */
        temp1.data.string = alloc_prog_string("PLAYER");
        temp2.type = PROG_OBJECT;
        temp2.data.objref = fr->player;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING; /* CALLED_DATA */
        temp1.data.string = alloc_prog_string("CALLED_DATA");
        temp2.type = PROG_STRING;
        temp2.data.string = alloc_prog_string(""); /*not sure on this one */
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING; /* INSTCNT */
        temp1.data.string = alloc_prog_string("INSTCNT");
        temp2.type = PROG_INTEGER;
        temp2.data.number = fr->instcnt;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING; /* DESCR */
        temp1.data.string = alloc_prog_string("DESCR");
        temp2.type = PROG_INTEGER;
        temp2.data.number = fr->descr;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING; /* CPU */
        temp1.data.string = alloc_prog_string("CPU");
        temp2.type = PROG_FLOAT;
        temp2.data.fnumber = (float) pcnt;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING; /* NEXTRUN */
        temp1.data.string = alloc_prog_string("NEXTRUN");
        temp2.type = PROG_INTEGER;
        temp2.data.number = 0;  /* Currently running */
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING; /* STARTED */
        temp1.data.string = alloc_prog_string("STARTED");
        temp2.type = PROG_INTEGER;
        temp2.data.number = (int) fr->started;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING; /* TYPE */
        temp1.data.string = alloc_prog_string("TYPE");
        temp2.type = PROG_STRING;
        temp2.data.string = alloc_prog_string("MUF"); /* always MUF here */
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING; /* SUBTYPE */
        temp1.data.string = alloc_prog_string("SUBTYPE");
        temp2.type = PROG_STRING;
        temp2.data.string = alloc_prog_string(""); /* never a subtype here */
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
    } else                      /* not current PID */
        nw = get_pidinfo(oper[0].data.number);
    
    PushArrayRaw(nw);
}
void
prim_parselock(PRIM_PROTOTYPE)
{
    struct boolexp *lok;
    
                  /* string: lock string */
    CHECKOFLOW(1);
    if (oper[0].type != PROG_STRING)
        abort_interp("Invalid argument");
    if (oper[0].data.string != (struct shared_string *) NULL) {
        lok = parse_boolexp(fr->descr, ProgUID, oper[0].data.string->data, 0);
    } else {
        lok = TRUE_BOOLEXP;
    }
    
    PushLock(lok);
    free_boolexp(lok);
}
void
prim_unparselock(PRIM_PROTOTYPE)
{
    const char *ptr;
    
                  /* lock: lock */
    if (oper[0].type != PROG_LOCK)
        abort_interp("Invalid argument");
    if (oper[0].data.lock != (struct boolexp *) TRUE_BOOLEXP) {
        ptr = unparse_boolexp(ProgUID, oper[0].data.lock, 0);
    } else {
        ptr = NULL;
    }
    CHECKOFLOW(1);
    
    if (ptr) {
        PushString(ptr);
    } else {
        PushNullStr;
    }
}
void
prim_prettylock(PRIM_PROTOTYPE)
{
    const char *ptr;
    
                  /* lock: lock */
    if (oper[0].type != PROG_LOCK)
        abort_interp("Invalid argument");
    ptr = unparse_boolexp(ProgUID, oper[0].data.lock, 1);
    CHECKOFLOW(1);
    
    PushString(ptr);
}
void
prim_cancallp(PRIM_PROTOTYPE)
{
	int result;
    
                  /* string: public function name */
                  /* dbref: Program dbref to check */
    if (oper[1].type != PROG_OBJECT)
        abort_interp("Expected dbref argument. (1)");
    if (!valid_object(&oper[1]))
        abort_interp("Invalid dbref (1)");
    if (Typeof(oper[1].data.objref) != TYPE_PROGRAM)
        abort_interp("Object is not a MUF Program. (1)");
    if (oper[0].type != PROG_STRING)
        abort_interp("Expected string argument. (2)");
    if (!oper[0].data.string)
        abort_interp("Invalid Null string argument. (2)");
    if (!(DBFETCH(oper[1].data.objref)->sp.program.code)) {
        struct line *tmpline;
        tmpline = DBFETCH(oper[1].data.objref)->sp.program.first;
        DBFETCH(oper[1].data.objref)->sp.program.first =
            ((struct line *) read_program(oper[1].data.objref));
        do_compile(fr->descr, OWNER(oper[1].data.objref), oper[1].data.objref, 0);
        free_prog_text(DBFETCH(oper[1].data.objref)->sp.program.first);
        DBFETCH(oper[1].data.objref)->sp.program.first = tmpline;
    }
    result = 0;
    if (ProgMLevel(oper[1].data.objref) > 0 &&
        (mlev >= 4 || OWNER(oper[1].data.objref) == ProgUID
         || Linkable(oper[1].data.objref))
        ) {
        struct publics *pbs;
        pbs = DBFETCH(oper[1].data.objref)->sp.program.pubs;
        while (pbs) {
            if (!string_compare(oper[0].data.string->data, pbs->subname))
                break;
            pbs = pbs->next;
        }
        if (pbs) 
	    if ((!pbs->self && (mlev >= pbs->mlev)) || (pbs->self && (oper[1].data.objref == program)))
        	result = 1;
    }
    CHECKOFLOW(1);
    
    PushInt(result);
}
void
prim_timer_start(PRIM_PROTOTYPE)
{
    
                  /* string: timer id */
                  /* int: delay length in seconds */
    if (fr->timercount > tp_process_timer_limit)
        abort_interp("Too many timers!");
    if (oper[1].type != PROG_INTEGER)
        abort_interp("Expected an integer delay time. (1)");
    if (oper[0].type != PROG_STRING)
        abort_interp("Expected a string timer id. (2)");
    dequeue_timers(fr->pid, (char *) DoNullInd(oper[0].data.string));
    add_muf_timer_event(fr->descr, player, program, fr, oper[1].data.number,
                        (char *) DoNullInd(oper[0].data.string));
    
}
void
prim_timer_stop(PRIM_PROTOTYPE)
{
    
                  /* string: timer id */
    if (oper[0].type != PROG_STRING)
        abort_interp("Expected a string timer id. (2)");
    dequeue_timers(fr->pid, (char *) DoNullInd(oper[0].data.string));
    
}
void
prim_event_exists(PRIM_PROTOTYPE)
{
	int result;
    
                  /* str: eventID to look for */
    if (oper[0].type != PROG_STRING || !oper[0].data.string)
        abort_interp("Expected a non-null string eventid to search for.");
    result = muf_event_exists(fr, oper[0].data.string->data);
    
    PushInt(result);
}
void
prim_event_count(PRIM_PROTOTYPE)
{
	int result;
    CHECKOFLOW(1);
    result = muf_event_count(fr);
    PushInt(result);
}
void
prim_event_send(PRIM_PROTOTYPE)
{
    struct frame *destfr;
    stk_array *arr;
    struct inst temp1;
	char buf[BUFFER_LEN];
    
                  /* any: data to pass */
                  /* string: event id */
                  /* int: process id to send to */
    if (mlev < 3)
        abort_interp("Requires Mucker level 3 or better.");
    if (oper[2].type != PROG_INTEGER)
        abort_interp("Expected an integer process id. (1)");
    if (oper[1].type != PROG_STRING)
        abort_interp("Expected a string event id. (2)");
    if (fr->pid == oper[2].data.number) {
        destfr = fr;
    } else {
        destfr = timequeue_pid_frame(oper[2].data.number);
    }
    if (destfr) {
        arr = new_array_dictionary();
        array_set_strkey(&arr, "data", &oper[0]);
        array_set_strkey_intval(&arr, "caller_pid", fr->pid);
        array_set_strkey_intval(&arr, "descr", fr->descr);
        array_set_strkey_refval(&arr, "caller_prog", program);
        array_set_strkey_refval(&arr, "trigger", fr->trig);
        array_set_strkey_refval(&arr, "prog_uid", ProgUID);
        array_set_strkey_refval(&arr, "player", player);
        temp1.type = PROG_ARRAY;
        temp1.data.array = arr;
        sprintf(buf, "USER.%.32s", DoNullInd(oper[1].data.string));
        muf_event_add(destfr, buf, &temp1, 0);
        CLEAR(&temp1);
    }
    
}
void
prim_pnameokp(PRIM_PROTOTYPE)
{
	int result;
    
    if (oper[0].type != PROG_STRING)
        abort_interp("Player name string expected.");
    if (!oper[0].data.string)
        result = 0;
    else
        result = ok_player_name(oper[0].data.string->data);
    
    PushInt(result);
}
void
prim_nameokp(PRIM_PROTOTYPE)
{
	int result;
    
    if (oper[0].type != PROG_STRING)
        abort_interp("Object name string expected.");
    if (!oper[0].data.string)
        result = 0;
    else
        result = ok_name(oper[0].data.string->data);
    
    PushInt(result);
}
void
prim_watchpid(PRIM_PROTOTYPE)
{
    struct frame *frame;
    
    if (mlev < 3) {
        abort_interp("Mucker level 3 required.");
    }
    if (oper[0].type != PROG_INTEGER) {
        abort_interp("Process PID expected.");
    }
/* Lets see if the batbat catches this one. */
    if (oper[0].data.number == fr->pid) {
        abort_interp("Narcissistic processes not allowed.");
    }
    frame = timequeue_pid_frame(oper[0].data.number);
    if (frame) {
        struct mufwatchpidlist **cur;
        struct mufwatchpidlist *waitee;
        for (cur = &frame->waiters; *cur; cur = &(*cur)->next) {
            if ((*cur)->pid == oper[0].data.number) {
                break;
            }
        }
        if (!*cur) {
            *cur = (struct mufwatchpidlist *) malloc(sizeof(**cur));
            if (!*cur) {
                abort_interp("Internal memory error.\n");
            }
            (*cur)->next = 0;
            (*cur)->pid = fr->pid;
            waitee = (struct mufwatchpidlist *) malloc(sizeof(*waitee));
            if (!waitee) {
                abort_interp("Internal memory error.\n");
            }
            waitee->next = fr->waitees;
            waitee->pid = oper[0].data.number;
            fr->waitees = waitee;
        }
    } else {
        char buf[64];
        sprintf(buf, "PROC.EXIT.%d", oper[0].data.number);
        muf_event_add(fr, buf, &oper[0], 0);
    }
    
}
void
prim_read_wants_blanks(PRIM_PROTOTYPE)
{
    fr->wantsblanks = !(fr->wantsblanks);
}
void
prim_get_read_wants_blanks(PRIM_PROTOTYPE)
{
	int result;
    CHECKOFLOW(1);
	result = (int)fr->wantsblanks;
    PushInt(result);
}
void
prim_debugger_break(PRIM_PROTOTYPE)
{
    int i = 0;
    if (fr->brkpt.count >= MAX_BREAKS)
        abort_interp("Too many breakpoints set.");
    fr->brkpt.force_debugging = 1;
    if (fr->brkpt.count != 1 || fr->brkpt.temp[0] != 1 ||
        fr->brkpt.level[0] != -1 || fr->brkpt.line[0] != -1 ||
        fr->brkpt.linecount[0] != -2 || fr->brkpt.pc[0] != NULL ||
        fr->brkpt.pccount[0] != -2 || fr->brkpt.prog[0] != program) {
        /* need to make initial breakpoint */
        i = fr->brkpt.count++;
        fr->brkpt.temp[i] = 1;
        fr->brkpt.level[i] = -1;
        fr->brkpt.line[i] = -1;
        fr->brkpt.linecount[i] = -2;
        fr->brkpt.pc[i] = NULL;
        fr->brkpt.pccount[i] = 0;
        fr->brkpt.prog[i] = NOTHING;
    }
}
void
prim_debug_on(PRIM_PROTOTYPE)
{
    FLAGS(program) |= DARK;
}
void
prim_debug_off(PRIM_PROTOTYPE)
{
    FLAGS(program) &= ~DARK;
}
void
prim_debug_line(PRIM_PROTOTYPE)
{
    if (!(FLAGS(program) & DARK) && controls(PSafe, program)) {
		char buf[BUFFER_LEN];
        char *mesg = debug_inst(fr, 0, pc, fr->pid, arg, buf, sizeof(buf),
                                *top, program);
        notify_nolisten(PSafe, mesg, 1);
    }
}
void
prim_systime_precise(PRIM_PROTOTYPE)
{
    struct timeval fulltime;
    double dbltime;
    
    gettimeofday(&fulltime, (struct timezone *) 0);
    CHECKOFLOW(1);
    dbltime = fulltime.tv_sec + (((double) fulltime.tv_usec) / 1.0e6);
    PushFloat(dbltime);
}
void
prim_htoi(PRIM_PROTOTYPE)
{
	int result, tmp;
    
    if (oper[0].type != PROG_STRING)
        abort_interp("Non-string argument. (1)");
    result = 0;
    if (oper[0].data.string) {
        for (tmp = 0; oper[0].data.string->data[tmp]; ++tmp) {
            result +=
                (oper[0].data.string->data[tmp] >=
                 'A' ? ((oper[0].data.string->data[tmp] & 0xdf) - 'A') +
                 10 : (oper[0].data.string->data[tmp] - '0'));
            if (oper[0].data.string->data[tmp + 1] != '\0')
                result *= 16;
        }
    }
    
    PushInt(result);
}
void
prim_itoh(PRIM_PROTOTYPE)
{
	char buf[BUFFER_LEN];
    
    if (oper[0].type != PROG_INTEGER)
        abort_interp("Non-integer argument. (1)");
    sprintf(buf, "%.2X", oper[0].data.number);
    
    PushString(buf);
}
void
prim_MD5hash(PRIM_PROTOTYPE)
{
    char hexout[33];
    
    if (oper[0].type != PROG_STRING)
        abort_interp("Non-string argument. (1)");
    if (!oper[0].data.string) {
		MD5hex((char *)hexout, "", 0);
	} else {
		MD5hex((char *)hexout, oper[0].data.string->data, oper[0].data.string->length);
	}
	
    PushString(hexout); 
}
void
prim_SHA1hash(PRIM_PROTOTYPE)
{
    char hexout[41];
    
    if (oper[0].type != PROG_STRING)
        abort_interp("Non-string argument. (1)");
    if (!oper[0].data.string) {
		SHA1hex((char *)hexout, "", 0);
	} else {
		SHA1hex((char *)hexout, oper[0].data.string->data, oper[0].data.string->length);
	}
	
    PushString(hexout); 
}
void
prim_onevent(PRIM_PROTOTYPE)
{
    struct muf_interrupt *e;
    
                  /* keep in queue */
                  /* function addr */
                  /* interrupt id  */
                  /* event id      */
    if (oper[1].type != PROG_ADD)
        abort_interp("Expected a function address. (2)");
    if (oper[2].type != PROG_STRING)
        abort_interp("String argument expected. (3)");
    if (!oper[2].data.string)
        abort_interp("Interrupt ID cannot be null. (3)");
    if (oper[3].type != PROG_STRING)
        abort_interp("String argument expected. (4)");
    if (!oper[3].data.string)
        abort_interp("Event ID cannot be null. (4)");
    if (!OkObj(oper[1].data.addr->progref)
        || (Typeof(oper[1].data.addr->progref) != TYPE_PROGRAM))
        abort_interp("Internal error.  Invalid address. (1)");
    if (program != oper[1].data.addr->progref)
        abort_interp("Destination address outside current program. (1)");
    if ((e = muf_interrupt_find(fr, oper[2].data.string->data))) {
        free((void *) e->event);
    } else {
        e = (struct muf_interrupt *) malloc(sizeof(struct muf_interrupt));
        e->id = alloc_string(oper[2].data.string->data);
        e->prev = NULL;
        e->next = fr->interrupts;
        if (fr->interrupts)
            fr->interrupts->prev = e;
        fr->interrupts = e;
    }
    e->event = alloc_string(oper[3].data.string->data);
    e->addr = oper[1].data.addr->data;
    e->keep = (oper[0].data.number != 0);
    
}
void
prim_interrupt_level(PRIM_PROTOTYPE)
{
    CHECKOFLOW(1);
    PushInt(fr->interrupted);
}
