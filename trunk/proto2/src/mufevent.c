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
#include "mufevent.h"

struct mufevent_process *mufevent_processes;

static void
muf_event_queue_pfree(struct mufevent_process *ptr)
{
    int i;

    ptr->prev = NULL;
    ptr->next = NULL;
    ptr->player = NOTHING;
    ptr->prog = NOTHING;
    if (ptr->fr) {
        prog_clean(ptr->fr);
        ptr->fr = NULL;
    }
    if (ptr->filters) {
        for (i = 0; i < ptr->filtercount; i++) {
            if (ptr->filters[i]) {
                free(ptr->filters[i]);
                ptr->filters[i] = NULL;
            }
        }
        free(ptr->filters);
        ptr->filters = NULL;
        ptr->filtercount = 0;
    }
    free(ptr);
}

/* static void muf_event_process_free(struct mufevent* ptr)
 * Frees up a mufevent_process once you are done with it.
 * This shouldn't be used outside this module.
 */
static void
muf_event_process_free(struct mufevent_process *ptr)
{
    if (ptr->next) {
        ptr->next->prev = ptr->prev;
    }
    if (ptr->prev) {
        ptr->prev->next = ptr->next;
    } else {
        mufevent_processes = ptr->next;
    }

    muf_event_queue_pfree(ptr);
}

/* void muf_event_register_specific(dbref player, dbref prog, struct frame* fr, int eventcount, char** eventids)
 * Called when a MUF program enters EVENT_WAITFOR, to register that
 * the program is ready to process MUF events of the given ID type.
 * This duplicates the eventids list for itself, so the caller is
 * responsible for freeing the original eventids list passed.
 */
void
muf_event_register_specific(dbref player, dbref prog, struct frame *fr,
                            int eventcount, char **eventids)
{
    struct mufevent_process *newproc;
    struct mufevent_process *ptr;
    struct descriptor_data *curdescr = NULL;
    int i;

    newproc =
        (struct mufevent_process *) malloc(sizeof(struct mufevent_process));

    newproc->prev = NULL;
    newproc->next = NULL;
    newproc->player = player;
    newproc->descr = fr->descr;
    newproc->prog = prog;
    newproc->fr = fr;
    newproc->filtercount = eventcount;
    if (!OkObj(player)) {
        if ((curdescr = get_descr(fr->descr, NOTHING))) {
            curdescr->interactive = 2;
            DR_RAW_ADD_FLAGS(curdescr, DF_INTERACTIVE);
        }
    }
    if (eventcount > 0) {
        newproc->filters = (char **) malloc(eventcount * sizeof(char **));
        for (i = 0; i < eventcount; i++) {
            newproc->filters[i] = string_dup(eventids[i]);
        }
    } else {
        newproc->filters = NULL;
    }

    ptr = mufevent_processes;
    while (ptr && ptr->next) {
        ptr = ptr->next;
    }
    if (!ptr) {
        mufevent_processes = newproc;
    } else {
        ptr->next = newproc;
        newproc->prev = ptr;
    }
}


/* void muf_event_register(dbref player, dbref prog, struct frame* fr)
 * Called when a MUF program enters EVENT_WAIT, to register that
 * the program is ready to process any type of MUF events.
 */
void
muf_event_register(dbref player, dbref prog, struct frame *fr)
{
    muf_event_register_specific(player, prog, fr, 0, NULL);
}


/* int muf_event_read_notify(int descr, dbref player)
 * Sends a "READ" event to the first foreground or preempt muf process
 * that is owned by the given player.  Returns 1 if an event was sent,
 * 0 otherwise.
 */
int
muf_event_read_notify(int descr, dbref player, const char *cmd)
{
    struct mufevent_process *ptr;

    ptr = mufevent_processes;
    while (ptr) {
        if (ptr->player == player) {
            if (ptr->fr && ptr->fr->multitask != BACKGROUND) {
                /* Added the following condition. It will be used
                 * to keep the program from getting events from
                 * use of in-server globals like WHO or QUIT -Akari
                 */
                if (*cmd || ptr->fr->wantsblanks) {
                    struct inst temp;

                    temp.type = PROG_INTEGER;
                    temp.data.number = descr;
                    muf_event_add(ptr->fr, "READ", &temp, 1);
                    return 1;
                }
            }
        }
        ptr = ptr->next;
    }
    return 0;
}

/* int muf_event_dequeue_frame():
 * removes the program frame(s) specified from the EVENT_WAITFOR queue.
 */
int
muf_event_dequeue_frame(struct frame *fr)
{
    struct mufevent_process *proc = mufevent_processes, *tmp;
    register int count = 0;

    while (proc) {
        tmp = proc->next;
        if (proc->fr == fr) {
            if (OkObj(proc->player) && !proc->fr->been_background)
                DBFETCH(proc->player)->sp.player.block = 0;
            proc->fr = NULL;
            muf_event_process_free(proc);
            count++;
        }
        proc = tmp;
    }

    return count;
}

/* int muf_event_dequeue_pid(int pid)
 * removes the MUF program with the given PID from the EVENT_WAIT queue.
 */
int
muf_event_dequeue_pid(int pid)
{
    struct mufevent_process *proc, *tmp;
    int count = 0;

    proc = mufevent_processes;
    while (proc) {
        tmp = proc;
        proc = proc->next;
        if (tmp->fr->pid == pid) {
            if (OkObj(tmp->player) && !tmp->fr->been_background) {
                DBFETCH(tmp->player)->sp.player.block = 0;
            }
            muf_event_purge(tmp->fr);
            muf_event_process_free(tmp);
            ++count;
        }
    }

    return count;
}


/* static int event_has_refs(dbref program, struct frame* fr)
 * Checks the MUF event queue for address references on the stack or
 * dbref references on the callstack
 */
static int
event_has_refs(dbref program, struct frame *fr)
{
    int loop;

    for (loop = 1; loop < fr->caller.top; loop++) {
        if (fr->caller.st[loop] == program) {
            return 1;
        }
    }

    for (loop = 0; loop < fr->argument.top; loop++) {
        if (fr->argument.st[loop].type == PROG_ADD &&
            fr->argument.st[loop].data.addr->progref == program) {
            return 1;
        }
    }

    return 0;
}


/* int muf_event_dequeue(dbref prog, int sleeponly)
 * Deregisters a program from any instances of it in the EVENT_WAIT queue.
 * Sleeponly values:
 * 0 - All matching processes
 * 1 - Only matching sleeping processes
 * 2 - Only matching foreground processes
 * Note that the 'prog' parameter is named misleadingly. It may be a 
 * program dbref or a player dbref that actually gets passed in. -Akari
 */
int
muf_event_dequeue(dbref prog, int sleeponly)
{
    struct mufevent_process *proc, *tmp;
    int count = 0;

    proc = mufevent_processes;
    while (proc) {
        tmp = proc;
        proc = proc->next;
        if (prog == NOTHING ||
            tmp->player == prog ||
            tmp->prog == prog || event_has_refs(prog, tmp->fr)) {
            /* Added respect for the sleeponly parameter that is used
             * in other process dequeueing code. - Akari
             */
            if (sleeponly == 0 ||
                (sleeponly == 2 && tmp->fr->multitask != BACKGROUND)) {
                if (OkObj(tmp->player) && !tmp->fr->been_background) {
                    DBFETCH(tmp->player)->sp.player.block = 0;
                }
                muf_event_purge(tmp->fr);
                muf_event_process_free(tmp);
                ++count;
            }
        }
    }
    return count;
}


/* int muf_event_dequeue(int descr, int sleeponly)
 * Just like muf_event_dequeue except that it only checks for
 * matching descriptors instead of the other criteria that the 
 * other function checks for. Since descr's could be the same 
 * value as a program or player dbref, this needed to be handled
 * seperately. In order to get this to work, a descr field was added
 * to struct mufevent_process.
 * Added the sleeponly parameter so that we can stop treating MUF
 * events as only background processes. -Akari
 */
int
muf_event_dequeue_descr(int descr, int sleeponly)
{
    struct mufevent_process *proc, *tmp;
    int count = 0;

    proc = mufevent_processes;
    while (proc) {
        tmp = proc;
        proc = proc->next;
        if (tmp->descr == descr) {
            if (sleeponly == 0 ||
                (sleeponly == 2 && tmp->fr->multitask != BACKGROUND)) {
                muf_event_purge(tmp->fr);
                muf_event_process_free(tmp);
                ++count;
            }
        }
    }
    return count;
}

struct frame *
muf_event_pid_frame(int pid)
{
    struct mufevent_process *ptr = mufevent_processes;

    while (ptr) {
        if (ptr->fr && ptr->fr->pid == pid)
            return ptr->fr;
        ptr = ptr->next;
    }
    return NULL;
}


/* int muf_event_controls(dbref player, int pid)
 * Returns true if the given player controls the given PID.
 */
int
muf_event_controls(dbref player, int pid)
{
    struct mufevent_process *proc = mufevent_processes;

    while (proc && pid != proc->fr->pid) {
        proc = proc->next;
    }
    if (!proc)                  /* process not found */
        return 0;

    if (!controls(player, proc->prog) && player != proc->player)
        return 0;               /*player doesn't own prog, nor is running it */

    return 1;
}


/* int muf_event_list(dbref player, char* pat)
 * List all processes in the EVENT_WAIT queue that the given player controls.
 * This is used by the @ps command.
 */
int
muf_event_list(dbref player, char *pat)
{
    char buf[BUFFER_LEN];
    int count = 0;
    time_t rtime = time((time_t *) NULL);
    time_t etime;
    double pcnt = 0;
    struct mufevent_process *proc = mufevent_processes;

    while (proc) {
        if (proc->fr) {
            etime = rtime - proc->fr->started;
            if (etime > 0) {
                pcnt = proc->fr->totaltime.tv_sec;
                pcnt += proc->fr->totaltime.tv_usec / 1000000;
                pcnt = pcnt * 100 / etime;
                if (pcnt > 100.0) {
                    pcnt = 100.0;
                }
            } else {
                pcnt = 0.0;
            }
        }
        sprintf(buf, pat,
                proc->fr->pid, "--",
                time_format_2((long) (rtime - proc->fr->started)),
                (proc->fr->instcnt / 1000), pcnt, proc->prog,
                (OkObj(proc->player)) ? NAME(proc->player) : "(Login)",
                "EVENT_WAITFOR");
        if (Wiz(OWNER(player)) || (OWNER(proc->prog) == OWNER(player))
            || (proc->player == player))
            notify_nolisten(player, buf, 1);
        count++;
        proc = proc->next;
    }
    return count;
}


/* int muf_event_count(struct frame* fr)
 * Returns how many events are waiting to be processed.
 */
int
muf_event_count(struct frame *fr)
{
    struct mufevent *ptr;
    int count = 0;

    for (ptr = fr->events; ptr; ptr = ptr->next)
        count++;

    return count;
}


/* int muf_event_exists(struct frame* fr, const char* eventid)
 * Returns how many events of the given event type are waiting to be processed.
 * The eventid passed can be an smatch string.
 */
int
muf_event_exists(struct frame *fr, const char *eventid)
{
    struct mufevent *ptr;
    int count = 0;
    char pattern[BUFFER_LEN];

    strcpy(pattern, eventid);

    for (ptr = fr->events; ptr; ptr = ptr->next)
        if (equalstr(pattern, ptr->event))
            count++;

    return count;
}


/* static void muf_event_free(struct mufevent* ptr)
 * Frees up a MUF event once you are done with it.  This shouldn't be used
 * outside this module.
 */
static void
muf_event_free(struct mufevent *ptr)
{
    CLEAR(&ptr->data);
    free(ptr->event);
    ptr->event = NULL;
    ptr->next = NULL;
    free(ptr);
}


/* void muf_event_add(struct frame* fr, char* event, struct inst* val, int exclusive)
 * Adds a MUF event to the event queue for the given program instance.
 * If the exclusive flag is true, and if an item of the same event type
 * already exists in the queue, the new one will NOT be added.
 */
void
muf_event_add(struct frame *fr, char *event, struct inst *val, int exclusive)
{
    struct mufevent *newevent;
    struct mufevent *ptr;

    ptr = fr->events;
    while (ptr && ptr->next) {
        if (exclusive && !strcmp(event, ptr->event)) {
            return;
        }
        ptr = ptr->next;
    }

    if (exclusive && ptr && !strcmp(event, ptr->event))
        return;

    /* check for and process interrupts */
    if (muf_interrupt_check_byevent(fr, event, val))
        return;

    newevent = (struct mufevent *) malloc(sizeof(struct mufevent));
    newevent->event = string_dup(event);
    copyinst(val, &newevent->data);
    newevent->next = NULL;

    if (!ptr) {
        fr->events = newevent;
    } else {
        ptr->next = newevent;
    }
}



/* struct mufevent* muf_event_pop_specific(struct frame* fr, int eventcount, const char** events)
 * Removes the first event of one of the specified types from the event queue
 * of the given program instance.
 * Returns a pointer to the removed event to the caller.
 * Returns NULL if no matching events are found.
 * You will need to call muf_event_free() on the returned data when you
 * are done with it and wish to free it from memory.
 */
struct mufevent *
muf_event_pop_specific(struct frame *fr, int eventcount, char **events)
{
    struct mufevent *tmp = NULL;
    struct mufevent *ptr = NULL;
    int i;

    for (i = 0; i < eventcount; i++) {
        if (fr->events && equalstr(events[i], fr->events->event)) {
            tmp = fr->events;
            fr->events = tmp->next;
            return tmp;
        }
    }

    ptr = fr->events;
    while (ptr && ptr->next) {
        for (i = 0; i < eventcount; i++) {
            if (equalstr(events[i], ptr->next->event)) {
                tmp = ptr->next;
                ptr->next = tmp->next;
                return tmp;
            }
        }
        ptr = ptr->next;
    }

    return NULL;
}



/* void muf_event_remove(struct frame* fr, char* event, int doall)
 * Removes a given MUF event type from the event queue of the given
 * program instance.  If which is MUFEVENT_ALL, all instances are removed.
 * If which is MUFEVENT_FIRST, only the first instance is removed.
 * If which is MUFEVENT_LAST, only the last instance is removed.
 */
void
muf_event_remove(struct frame *fr, char *event, int which)
{
    struct mufevent *tmp = NULL;
    struct mufevent *ptr = NULL;

    while (fr->events && !strcmp(event, fr->events->event)) {
        if (which == MUFEVENT_LAST) {
            tmp = fr->events;
            break;
        } else {
            tmp = fr->events;
            fr->events = tmp->next;
            muf_event_free(tmp);
            if (which == MUFEVENT_FIRST) {
                return;
            }
        }
    }

    ptr = fr->events;
    while (ptr && ptr->next) {
        if (!strcmp(event, ptr->next->event)) {
            if (which == MUFEVENT_LAST) {
                tmp = ptr;
                ptr = ptr->next;
            } else {
                tmp = ptr->next;
                ptr->next = tmp->next;
                muf_event_free(tmp);
                if (which == MUFEVENT_FIRST) {
                    return;
                }
            }
        } else {
            ptr = ptr->next;
        }
    }
}



/* struct mufevent* muf_event_peek(struct frame* fr)
 * This returns a pointer to the top muf event of the given
 * program instance's event queue.  The event is not removed
 * from the queue.
 */
struct mufevent *
muf_event_peek(struct frame *fr)
{
    return fr->events;
}



/* static struct mufevent* muf_event_pop(struct frame* fr)
 * This pops the top muf event off of the given program instance's
 * event queue, and returns it to the caller.  The caller should
 * call muf_event_free() on the data when it is done with it.
 */
static struct mufevent *
muf_event_pop(struct frame *fr)
{
    struct mufevent *ptr = NULL;

    if (fr->events) {
        ptr = fr->events;
        fr->events = fr->events->next;
    }
    return ptr;
}

/* void muf_event_purge(struct frame* fr)
 * purges all muf events from the given program instance's event queue.
 */
void
muf_event_purge(struct frame *fr)
{
    while (fr->events) {
        muf_event_free(muf_event_pop(fr));
    }
}

/* void muf_event_process()
 * For all program instances who are in the EVENT_WAIT queue,
 * check to see if they have any items in their event queue.
 * If so, then process one each.  Up to ten programs can have
 * events processed at a time.
 */
void
muf_event_process(void)
{
    int limit = 10;
    struct mufevent_process *proc, *tmp;
    struct mufevent *ev;
    struct descriptor_data *curdescr = NULL;

    proc = mufevent_processes;
    while (proc && limit > 0) {
        tmp = proc->next;
        if (proc->fr) {
            if (proc->filtercount > 0) {
                /* Search prog's event list for the apropriate event type. */
                /* HACK:  This is probably inefficient to be walking this
                 * queue over and over. Hopefully it's usually a short list.
                 */
                ev = muf_event_pop_specific(proc->fr, proc->filtercount,
                                            proc->filters);
            } else {
                /* Pop first event off of prog's event queue. */
                ev = muf_event_pop(proc->fr);
            }
            if (ev) {
                limit--;
                if (proc->fr->argument.top + 1 >= STACK_SIZE) {
                    /*
                     * Uh oh! That MUF program's stack is full!
                     * Print an error, free the frame, and exit.
                     */
                    if (OkObj(proc->player))
                        notify_nolisten(proc->player, "Program stack overflow.",
                                        1);
                    prog_clean(proc->fr);
                } else {
                    dbref current_program = NOTHING;
                    short block = 0, is_fg;

                    if (OkObj(proc->player)) {
                        current_program =
                            DBFETCH(proc->player)->sp.player.curr_prog;
                        block = DBFETCH(proc->player)->sp.player.block;
                    } else {
                        if ((curdescr = get_descr(proc->descr, NOTHING)))
                            block = curdescr->block;
                    }
                    is_fg = (proc->fr->multitask != BACKGROUND);

                    copyinst(&ev->data,
                             &(proc->fr->argument.st[proc->fr->argument.top]));
                    proc->fr->argument.top++;
                    push(proc->fr->argument.st, &(proc->fr->argument.top),
                         PROG_STRING, MIPSCAST alloc_prog_string(ev->event));

                    interp_loop(proc->player, proc->prog, proc->fr, 0);

                    if (!is_fg) {
                        if (OkObj(proc->player)) {
                            DBFETCH(proc->player)->sp.player.block = block;
                            DBFETCH(proc->player)->sp.player.curr_prog =
                                current_program;
                        } else {
                            if ((curdescr = get_descr(proc->descr, NOTHING)))
                                curdescr->block = block;
                        }
                    }
                }
                muf_event_free(ev);

                tmp = proc->next; /* proc->next might have changed */
                proc->fr = NULL; /* don't want to free the program itself */
                muf_event_process_free(proc);
            }
        }
        proc = tmp;
    }
}

/* called from get_pids() in timequeue.c for use in 
 * the GETPIDS prim. */
stk_array *
get_mufevent_pids(stk_array *nw, dbref ref)
{
    struct inst temp1;

    struct mufevent_process *proc = mufevent_processes;

    while (proc) {
        if (proc->player == ref || proc->prog == ref || proc->fr->trig == ref
            || ref < 0) {
            temp1.type = PROG_INTEGER;
            temp1.data.number = proc->fr->pid;
            array_appenditem(&nw, &temp1);
            CLEAR(&temp1);
        }
        proc = proc->next;
    }

    return nw;
}

stk_array *
get_mufevent_pidinfo(stk_array *nw, int pid)
{
    struct inst temp1, temp2;
    stk_array *arr;
    time_t rtime = time(NULL);
    time_t etime = 0;
    double pcnt = 0.0;
    int i;

    struct mufevent_process *proc = mufevent_processes;

    while (proc && (proc->fr->pid != pid))
        proc = proc->next;

    if (proc && (proc->fr->pid == pid)) {
        if (proc->fr) {
            etime = rtime - proc->fr->started;
            if (etime > 0) {
                pcnt = proc->fr->totaltime.tv_sec;
                pcnt += proc->fr->totaltime.tv_usec / 1000000;
                pcnt = pcnt * 100 / etime;
                if (pcnt > 100.0) {
                    pcnt = 100.0;
                }
            } else {
                pcnt = 0.0;
            }
        }
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("PID");
        temp2.type = PROG_INTEGER;
        temp2.data.number = proc->fr->pid;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("CALLED_PROG");
        temp2.type = PROG_OBJECT;
        temp2.data.objref = proc->prog;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("TRIG");
        temp2.type = PROG_OBJECT;
        temp2.data.objref = proc->fr->trig;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("PLAYER");
        temp2.type = PROG_OBJECT;
        temp2.data.objref = proc->player;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("CALLED_DATA");
        temp2.type = PROG_STRING;
        temp2.data.string = alloc_prog_string("EVENT_WAITFOR");
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("INSTCNT");
        temp2.type = PROG_INTEGER;
        temp2.data.number = proc->fr->instcnt;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("DESCR");
        temp2.type = PROG_INTEGER;
        temp2.data.number = proc->fr->descr;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("CPU");
        temp2.type = PROG_FLOAT;
        temp2.data.fnumber = pcnt;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("NEXTRUN");
        temp2.type = PROG_INTEGER;
        temp2.data.number = -1;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("STARTED");
        temp2.type = PROG_INTEGER;
        temp2.data.number = (int) proc->fr->started;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("TYPE");
        temp2.type = PROG_STRING;
        temp2.data.string = alloc_prog_string("MUFEVENT");
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("SUBTYPE");
        temp2.type = PROG_STRING;
        temp2.data.string = alloc_prog_string("");
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("FILTERS");
        arr = new_array_packed(0);
        for (i = 0; i < proc->filtercount; i++) {
            array_set_intkey_strval(&arr, i, proc->filters[i]);
        }
        temp2.type = PROG_ARRAY;
        temp2.data.array = arr;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
    }
    return nw;
}

extern timequeue tqhead;        /* timequeue.c */

/* int muf_interrupt_process():                                   */
/*  This function is in charge of queueing an interrupt onto the  */
/*   specified program's frame, and storing any of the program's  */
/*   current queue items into fr->qitem for later requeueing.     */
/*  If we didn't move the timequeue items like this, SLEEP and    */
/*   EVENT_WAITFOR and READ would all break.  -Hinoserm           */
static int
muf_interrupt_process(struct frame *fr, struct muf_interrupt *interrupt,
                      const char *event, struct inst *val)
{
    register struct muf_ainterrupt *a;

    a = (struct muf_ainterrupt *) malloc(sizeof(struct muf_ainterrupt));
    a->interrupt = interrupt;
    a->ret = NULL;
    a->next = NULL;

    //log_status("muf_interrupt_process(): %p\n", a); /* for debugging */

    if (!fr->ainttop)
        fr->ainttop = a;

    if (fr->aintbot)
        fr->aintbot->next = a;
    fr->aintbot = a;

    if (fr->ainttop == a) {
        register struct mufevent_process *p;
        struct muf_qitem *q =
            (struct muf_qitem *) malloc(sizeof(struct muf_qitem));
        q->type = 0;
        fr->qitem = q;

        /* this allows interrupt handler functions to use EVENT_WAITFOR */
        for (p = mufevent_processes; p; p = p->next) {
            if (p->fr == fr) {
                if (p->next)
                    p->next->prev = p->prev;
                if (p->prev)
                    p->prev->next = p->next;
                else
                    mufevent_processes = p->next;

                p->prev = NULL;
                p->next = NULL;

                q->t.eq = p;
                q->type = 1;
                break;
            }
        }

        if (!q->type) {
            register timequeue tmp, ptr;

            tmp = ptr = tqhead;
            while (ptr) {
                if (fr == ptr->fr && ptr->typ == TQ_MUF_TYP
                    && ptr->subtyp != TQ_MUF_TIMER) {
                    q->type = 2;
                    q->t.tq = ptr;
                    if (tmp == ptr) {
                        tqhead = tmp = tmp->next;
                        ptr = tmp;
                    } else {
                        tmp->next = ptr->next;
                        ptr = tmp->next;
                    }
                } else {
                    tmp = ptr;
                    ptr = ptr->next;
                }
            }
        }

        if (q->type) {
            add_muf_delay_event(0, fr->descr, fr->player, NOTHING, NOTHING,
                                fr->prog, fr, "INTERRUPT");
        } else {
            free((void *) q);
            fr->qitem = NULL;
        }
    }

    a->data = (struct inst *) malloc(sizeof(struct inst));
    copyinst(val, a->data);
    a->eventid = string_dup(event);
    //interp_loop(fr->player, fr->prog, fr, 0); /* was for debugging */

    return !(interrupt->keep);
}

static int
muf_interrupt_requeue(struct muf_qitem *q)
{
    switch (q->type) {
        case 1:
            /* for putting back mufevent queue items */
            if ((q->t.eq->next = mufevent_processes))
                q->t.eq->next->prev = q->t.eq;
            q->t.eq->prev = NULL;
            mufevent_processes = q->t.eq;
            break;
        case 2:
            /* for putting back standard timequeue items */
            q->t.tq->next = tqhead;
            tqhead = q->t.tq;
            break;
    }

    return (int) q->type;
}

/* int muf_interrupt_exit():                                       */
/*  Called from inside interp_loop() inside the EXIT prim, this    */
/*   function is in charge of requeuing any previous timequeue     */
/*   items, and also free()ing up the topmost active interrupt.    */
/*  Returns 0 if the program can continue, otherwise q->type.      */
int
muf_interrupt_exit(struct frame *fr)
{
    struct muf_ainterrupt *a = fr->ainttop;
    register int qtype = 0;

    if (!a)
        return 0;

    //log_status("muf_interrupt_pop_act(): %p\n", a); /* for debugging */

    if (!a->next) {
        fr->interrupted = 0;
        if (fr->qitem) {
            qtype = muf_interrupt_requeue(fr->qitem);
            free((void *) fr->qitem);
            fr->qitem = NULL;
        }
    }

    if (fr->aintbot == a)
        fr->aintbot = a->next;
    fr->ainttop = a->next;

    CLEAR(a->data);
    free((void *) a->eventid);
    free((void *) a->data);
    free((void *) a);

    return qtype;
}

/* muf_interrupt muf_interrupt_find():                            */
/*  This function searches through a program's interrupt list and */
/*   returns it if found, NULL otherwise.  -Hinoserm              */
struct muf_interrupt *
muf_interrupt_find(register struct frame *fr, register const char *id)
{
    register struct muf_interrupt *e = fr->interrupts;

    for (; e; e = e->next)
        if (!strcmp(e->id, id))
            return e;

    return NULL;
}

/* int muf_interrupt_check_byevent():                             */
/*  This function searches the frame's interrupt list and calls   */
/*   muf_interrupt_process() if a match is found.  Returns the    */
/*   number of interrupts thrown.  -Hinoserm */
int
muf_interrupt_check_byevent(struct frame *fr, register const char *event,
                            struct inst *val)
{
    register struct muf_interrupt *i;
    register int cnt = 0;

    if (!fr->use_interrupts)
        return 0;

    for (i = fr->interrupts; i; i = i->next)
        if (equalstr(i->event, (char *) event))
            cnt += muf_interrupt_process(fr, i, event, val);

    return cnt;
}

/* void muf_interrupt_clean():                                    */
/*  Called by prog_clean() to delete the frame's interrupt lists. */
void
muf_interrupt_clean(struct frame *fr)
{
    struct muf_interrupt *i = fr->interrupts, *tmp;
    struct muf_ainterrupt *a = fr->ainttop, *atmp;
    struct muf_qitem *q = fr->qitem;

    while (i) {
        tmp = i->next;
        free((void *) i->id);
        free((void *) i->event);
        free((void *) i);
        i = tmp;
    }

    while (a) {
        atmp = a->next;
        CLEAR(a->data);
        free((void *) a->eventid);
        free((void *) a->data);
        free((void *) a);
        a = atmp;
    }

    if (q) {
        switch (q->type) {
            case 1:
                q->t.eq->fr = NULL;
                muf_event_queue_pfree(q->t.eq);
                break;
            case 2:
                q->t.tq->fr = NULL;
                free_timenode(q->t.tq);
                break;
        }
        free((void *) q);
    }
}
