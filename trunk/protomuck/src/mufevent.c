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


struct mufevent_process {
	struct mufevent_process *next, *prev;
	dbref player;
	dbref prog;
        int descr;//Added in proto for login screen support
	int filtercount;
	char** filters;
	struct frame *fr;
} *mufevent_processes;


/* static void muf_event_process_free(struct mufevent* ptr)
 * Frees up a mufevent_process once you are done with it.
 * This shouldn't be used outside this module.
 */
static void
muf_event_process_free(struct mufevent_process *ptr)
{
    int i;

    if (ptr->next) {
        ptr->next->prev = ptr->prev;
    }
    if (ptr->prev) {
        ptr->prev->next = ptr->next;
    } else {
        mufevent_processes = ptr->next;
    }

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


/* void muf_event_register_specific(dbref player, dbref prog, struct frame* fr, int eventcount, char** eventids)
 * Called when a MUF program enters EVENT_WAITFOR, to register that
 * the program is ready to process MUF events of the given ID type.
 * This duplicates the eventids list for itself, so the caller is
 * responsible for freeing the original eventids list passed.
 */
void
muf_event_register_specific(dbref player, dbref prog, struct frame *fr, int eventcount, char** eventids)
{
	struct mufevent_process *newproc;
	struct mufevent_process *ptr;
        struct descriptor_data *curdescr = NULL;
	int i;

	newproc = (struct mufevent_process *) malloc(sizeof(struct mufevent_process));

        newproc->prev = NULL;
	newproc->next = NULL;
	newproc->player = player;
        newproc->descr = fr->descr;
	newproc->prog = prog;
	newproc->fr = fr;
	newproc->filtercount = eventcount;
        if (player == NOTHING ) {
            curdescr = get_descr(fr->descr, NOTHING);
            if (curdescr) {
                curdescr->interactive = 2;
                DR_RAW_ADD_FLAGS(curdescr, DF_INTERACTIVE);
            }
        }
	if (eventcount > 0) {
		newproc->filters = (char**) malloc(eventcount * sizeof(char**));
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
muf_event_read_notify(int descr, dbref player)
{
	struct mufevent_process *ptr;

	ptr = mufevent_processes;
	while (ptr) {
		if (ptr->player == player) {
			if (ptr->fr && ptr->fr->multitask != BACKGROUND) {
				struct inst temp;

				temp.type = PROG_INTEGER;
				temp.data.number = descr;
				muf_event_add(ptr->fr, "READ", &temp, 1);
				return 1;
			}
		}
		ptr = ptr->next;
	}
	return 0;
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


/* int muf_event_dequeue(dbref prog)
 * Deregisters a program from any instances of it in the EVENT_WAIT queue.
 */
int
muf_event_dequeue(dbref prog)
{
    struct mufevent_process *proc, *tmp;
    int count = 0;

    proc = mufevent_processes;
    while (proc) {
        tmp = proc;
        proc = proc->next;
        if (prog == NOTHING || tmp->player == prog || tmp->prog == prog ||
            event_has_refs(prog, tmp->fr)) {
            muf_event_purge(tmp->fr);
            muf_event_process_free(tmp);
            ++count;
        }
    }
    return count;
}


/* int muf_event_dequeue(int descr)
 * Just like muf_event_dequeue except that it only checks for
 * matching descriptors instead of the other criteria that the 
 * other function checks for. Since descr's could be the same 
 * value as a program or player dbref, this needed to be handled
 * seperately. In order to get this to work, a descr field was added
 * to struct mufevent_process.
 */
int
muf_event_dequeue_descr(int descr)
{
    struct mufevent_process *proc, *tmp;
    int count = 0;

    proc = mufevent_processes;
    while (proc) {
        tmp = proc;
        proc = proc->next;
        if (tmp->descr == descr ) { 
            muf_event_purge(tmp->fr);
            muf_event_process_free(tmp);
            ++count;
        }
    }
    return count;
}

struct frame*
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
    if (!proc) /* process not found */
        return 0;

    if (!controls(player, proc->prog) && player != proc->player)
        return 0; /*player doesn't own prog, nor is running it */

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
	double pcnt;
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
				(proc->fr->instcnt / 1000), pcnt, proc->prog, NAME(proc->player),
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
muf_event_count(struct frame* fr)
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
muf_event_exists(struct frame* fr, const char* eventid)
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

	if (exclusive && ptr && !strcmp(event, ptr->event)) {
		return;
	}

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
struct mufevent*
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



/* static struct mufevent* muf_event_peek(struct frame* fr)
 * This returns a pointer to the top muf event of the given
 * program instance's event queue.  The event is not removed
 * from the queue.
 */
static struct mufevent *
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
    dbref current_program;
    struct descriptor_data *curdescr = NULL;
    int block, is_fg;

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
                    notify_nolisten(proc->player, "Program stack overflow.", 1);
                    prog_clean(proc->fr);
                } else {
                    current_program = DBFETCH(
                                         proc->player)->sp.player.curr_prog;
                    block = DBFETCH(proc->player)->sp.player.block;
                    is_fg = (proc->fr->multitask != BACKGROUND);

                    copyinst(&ev->data, 
                            &(proc->fr->argument.st[proc->fr->argument.top]));
                    proc->fr->argument.top++;
                    push(proc->fr->argument.st, &(proc->fr->argument.top),
                         PROG_STRING, MIPSCAST alloc_prog_string(ev->event));

                    interp_loop(proc->player, proc->prog, proc->fr, 0);

                    if (!is_fg) {
                        DBFETCH(proc->player)->sp.player.block = block;
                        if (proc->player == NOTHING) {
                            curdescr = get_descr(proc->descr, NOTHING);
                            if (curdescr)
                                curdescr->block = block;  
                        }
                        DBFETCH(proc->player)->sp.player.curr_prog = block;
                    }
                }
                muf_event_free(ev);

                tmp = proc->next; /* proc->next might have changed */
                proc->fr = NULL;  /* don't want to free the program itself */
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
            || ref < 0 ) {
            temp1.type = PROG_INTEGER;
            temp1.data.number = proc->fr->pid;
            array_appenditem(&nw, &temp1);
            CLEAR(&temp1);
        }
        proc = proc->next;
    }

    return nw;
}
