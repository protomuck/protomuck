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
	struct mufevent_process *next;
	dbref player;
	dbref prog;
	struct frame *fr;
} *mufevent_processes;


/* void muf_event_register(dbref player, dbref prog, struct frame* fr)
 * Called when a MUF program enters EVENT_WAIT, to register that
 * the program is ready to process MUF events.
 */
void
muf_event_register(dbref player, dbref prog, struct frame *fr)
{
	struct mufevent_process *newproc;
	struct mufevent_process *ptr;

	newproc = (struct mufevent_process *) malloc(sizeof(struct mufevent_process));

	newproc->player = player;
	newproc->prog = prog;
	newproc->fr = fr;
	newproc->next = NULL;

	ptr = mufevent_processes;
	while (ptr && ptr->next) {
		ptr = ptr->next;
	}
	if (!ptr) {
		mufevent_processes = newproc;
	} else {
		ptr->next = newproc;
	}
}


/* int muf_event_dequeue_pid(int pid)
 * removes the MUF program with the given PID from the EVENT_WAIT queue.
 */
int
muf_event_dequeue_pid(int pid)
{
	struct mufevent_process **prev;
	struct mufevent_process *next;
	int count = 0;

	prev = &mufevent_processes;
	while (*prev) {
		if ((*prev)->fr->pid == pid) {
			next = (*prev)->next;
			muf_event_purge((*prev)->fr);
			prog_clean((*prev)->fr);
			count++;
			free(*prev);
			*prev = next;
		} else {
			prev = &((*prev)->next);
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
	struct mufevent_process **prev;
	struct mufevent_process *tmp;
	int count = 0;

	prev = &mufevent_processes;
	while (*prev) {
		if (prog == NOTHING || (*prev)->player == prog || (*prev)->prog == prog ||
			event_has_refs(prog, (*prev)->fr)) {
			tmp = *prev;
			*prev = tmp->next;
			muf_event_purge(tmp->fr);
			prog_clean(tmp->fr);
			count++;
			free(tmp);
		} else {
			prev = &((*prev)->next);
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
	struct mufevent_process *tmp;
	struct mufevent_process *ptr = mufevent_processes;

	tmp = ptr;
	while ((ptr) && (pid != ptr->fr->pid)) {
		tmp = ptr;
		ptr = ptr->next;
	}

	if (!ptr) {
		return 0;
	}

	if (!controls(player, ptr->prog) && player != ptr->player) {
		return 0;
	}
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
				(proc->fr->instcnt / 1000), pcnt, proc->prog, NAME(proc->player), "EVENT_WAIT");
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

	if (exclusive && !strcmp(event, ptr->event)) {
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



/* static struct mufevent* muf_event_pop(struct frame* fr)
 * This pops the top muf event off of the given program instance's
 * event queue, and returns it to the caller.
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
	struct mufevent_process *proc;
	struct mufevent_process *next;
	struct mufevent_process **prev;
	struct mufevent_process **nextprev;
	struct mufevent *ev;
	dbref tmpcp;
	int tmpbl;
	int tmpfg;

	proc = mufevent_processes;
	prev = &mufevent_processes;
	while (proc && limit > 0) {
		nextprev = &((*prev)->next);
		next = proc->next;
		if (proc->fr) {
			ev = muf_event_pop(proc->fr);
			if (ev) {
				limit--;

				nextprev = prev;
				*prev = proc->next;

				if (proc->fr->argument.top + 1 >= STACK_SIZE) {
					/*
					 * Uh oh! That MUF program's stack is full!
					 * Print an error, free the frame, and exit.
					 */
					notify_nolisten(proc->player, "Program stack overflow.", 1);
					prog_clean(proc->fr);
				} else {
                              tmpcp = DBFETCH(proc->player)->sp.player.curr_prog;
                              tmpbl = DBFETCH(proc->player)->sp.player.block;
					tmpfg = (proc->fr->multitask != BACKGROUND);

					copyinst(&ev->data, &(proc->fr->argument.st[proc->fr->argument.top]));
					proc->fr->argument.top++;
					push(proc->fr->argument.st, &(proc->fr->argument.top),
						 PROG_STRING, MIPSCAST alloc_prog_string(ev->event));

					interp_loop(proc->player, proc->prog, proc->fr, 0);

					if (!tmpfg) {
                                    DBFETCH(proc->player)->sp.player.block = tmpbl;
                                    DBFETCH(proc->player)->sp.player.curr_prog = tmpcp;
					}
				}
				muf_event_free(ev);

				proc->fr = NULL;
				proc->next = NULL;
				free(proc);
			}
		}
		prev = nextprev;
		proc = next;
	}
}


