/*
  Timequeue event code by Foxen
*/

#include "copyright.h"
#include "config.h"
#include "params.h"
#include "match.h"

#include "interp.h"
#include "db.h"
#include "tune.h"
#include "mpi.h"
#include "props.h"
#include "interface.h"
#include "externs.h"
#include "mufevent.h"

/*
 * Events types and data:
 *  What, typ, sub, when, user, where, trig, prog, frame, str1, cmdstr, str3
 *  qmpi   1    0   1     user  loc    trig  --    --     mpi   cmd     arg
 *  dmpi   1    1   when  user  loc    trig  --    --     mpi   cmd     arg
 *  lmpi   1    8   1     spkr  loc    lstnr --    --     mpi   cmd     heard
 *  oqmpi  1   16   1     user  loc    trig  --    --     mpi   cmd     arg
 *  odmpi  1   17   when  user  loc    trig  --    --     mpi   cmd     arg
 *  olmpi  1   24   1     spkr  loc    lstnr --    --     mpi   cmd     heard
 *  qmuf   0    0   0     user  loc    trig  prog  --     stk_s cmd@    --
 *  lmuf   0    1   0     spkr  loc    lstnr prog  --     heard cmd@    --
 *  dmuf   0    2   when  user  loc    trig  prog  frame  mode  --      --
 *  rmuf   0    3   -1    user  loc    trig  prog  frame  mode  --      --
 *  trmuf  0    4   when  user  loc    trig  prog  frame  mode  --      --
 *  tevmuf 0    5   when  user  loc    trig  prog  frame  mode  event   --
 */

/* definition for struct timenode moved to externs.h */

timequeue tqhead;
timequeue tqtail;
mutex     tq_mutex;

void prog_clean(struct frame *fr);
int has_refs(dbref program, timequeue ptr);

int
valid_objref(dbref obj)
{
    return (!((obj >= db_top)
              || (obj >= 0 && Typeof(obj) == TYPE_GARBAGE)
              || (obj < 0)));
}


extern int top_pid;
int process_count = 0;

timequeue
alloc_timenode(int typ, int subtyp, time_t mytime, int descr, dbref player,
               dbref loc, dbref trig, dbref program, struct frame *fr,
               const char *strdata, const char *strcmd, const char *str3)
{
    timequeue ptr;

	mutex_lock(tq_mutex);

    ptr = (timequeue) malloc(sizeof(struct timenode));
    
    ptr->typ = typ;
    ptr->subtyp = subtyp;
    ptr->when = mytime;
    ptr->uid = player;
    ptr->loc = loc;
    ptr->trig = trig;
    ptr->descr = descr;
    ptr->fr = fr;
    ptr->called_prog = program;
    ptr->called_data = (char *) string_dup((char *) strdata);
    ptr->command = alloc_string(strcmd);
    ptr->str3 = alloc_string(str3);
    ptr->eventnum = (fr) ? fr->pid : top_pid++;
	ptr->running = 0;
	ptr->thread = NULL;
	ptr->next = NULL;
	ptr->prev = NULL;

	mutex_unlock(tq_mutex);
    return (ptr);
}
void
remove_timenode(timequeue ptr)
{
	mutex_lock(tq_mutex);

	if (ptr->prev)
		ptr->prev->next = ptr->next;
	if (ptr->next)
		ptr->next->prev = ptr->prev;
	if (ptr == tqhead)
		tqhead = ptr->next;
	if (ptr == tqtail)
		tqtail = ptr->prev;

	process_count--;

	mutex_unlock(tq_mutex);
}

void
rotate_timenode_to_end(timequeue ptr)
{
	mutex_lock(tq_mutex);

	if (tqtail != ptr) {
		remove_timenode(ptr);

		if (tqtail) {
			tqtail->next = ptr;
			ptr->prev = tqtail;
		}
		tqtail = ptr;

		if (!tqhead)
			tqhead = ptr;

		ptr->next = NULL;
	}

	mutex_unlock(tq_mutex);
}	


void
free_timenode(timequeue ptr)
{
    struct descriptor_data *curdescr;

	mutex_lock(tq_mutex);

    if (ptr->command)
        free(ptr->command);
    if (ptr->called_data)
        free(ptr->called_data);
    if (ptr->str3)
        free(ptr->str3);
    if (ptr->fr) {
        if (ptr->typ != TQ_MUF_TYP || ptr->subtyp != TQ_MUF_TIMER) {
            if (ptr->fr->multitask != BACKGROUND) {
                if (OkObj(ptr->uid)) {
                    DBFETCH(ptr->uid)->sp.player.block = 0;
                } else {
                    if ((curdescr = get_descr(ptr->fr->descr, NOTHING)))
                        curdescr->block = 0;
                }
            }
			if (ptr->running) {
				ptr->fr->err = -1;
				mutex_lock(ptr->fr->mutx);
				mutex_unlock(ptr->fr->mutx);
			} else
				prog_clean(ptr->fr);

        }
        if (ptr->typ == TQ_MUF_TYP && (ptr->subtyp == TQ_MUF_READ ||
                                       ptr->subtyp == TQ_MUF_TREAD)) {
            if (OkObj(ptr->uid)) {
                FLAGS(ptr->uid) &= ~INTERACTIVE;
                FLAGS(ptr->uid) &= ~READMODE;
                anotify_nolisten(ptr->uid,
                                 CINFO
                                 "Data input aborted.  The command you were using was killed.",
                                 1);
            } else {
                if ((curdescr = get_descr(ptr->fr->descr, NOTHING))) {
                    DR_RAW_REM_FLAGS(curdescr, DF_INTERACTIVE);
                    curdescr->interactive = 0;
                    curdescr->block = 0;
                }
            }
        }
    }

    free(ptr);

	mutex_unlock(tq_mutex);
}

void
purge_timenode_free_pool()
{

}

int
control_process(dbref player, int pid)
{
    timequeue ptr = tqhead;

    while ((ptr) && (pid != ptr->eventnum))
        ptr = ptr->next;

    if (!ptr)
        return muf_event_controls(player, pid);

    if (!controls(player, ptr->called_prog) && !controls(player, ptr->trig))
        return 0;

    return 1;
}

int
add_event(int event_typ, int subtyp, int dtime, int descr, dbref player,
          dbref loc, dbref trig, dbref program, struct frame *fr,
          const char *strdata, const char *strcmd, const char *str3)
{
    timequeue event;
    struct descriptor_data *curdescr = NULL;
    time_t rtime = time((time_t *) NULL) + (time_t) dtime;
    int mypids = 0;

	mutex_lock(tq_mutex);

    process_count++;
    event = alloc_timenode(event_typ, subtyp, rtime, descr, player, loc, trig, program, fr, strdata, strcmd, str3);

	if (tqtail) {
		tqtail->next = event;
		event->prev = tqtail;
	}
	tqtail = event;

	if (!tqhead)
		tqhead = event;

	mutex_unlock(tq_mutex);
    return (event->eventnum);
}

int
alter_event(timequeue event, int subtyp, int dtime, int descr, dbref player,
          dbref loc, dbref trig, dbref program, struct frame *fr,
          const char *strdata, const char *strcmd, const char *str3)
{

	if (!event) {
		return add_event(TQ_MUF_TYP, subtyp, dtime, descr, player, loc, trig, program, fr, strdata, strcmd, str3);
	} else {
		time_t rtime = time((time_t *) NULL) + (time_t) dtime;

		event->subtyp = subtyp;
		event->when = rtime;
		event->uid = player;
		event->loc = loc;
		event->trig = trig;
		event->descr = descr;
		event->fr = fr;
		event->called_prog = program;

		if (event->called_data)
			free((void *)event->called_data);
		event->called_data = (char *) string_dup((char *) strdata);

		if (event->command)
			free((void *)event->command);
		event->command = alloc_string(strcmd);

		if (event->str3)
			free((void *)event->str3);
		event->str3 = alloc_string(str3);
	}

    return (event->eventnum);
}


int
add_mpi_event(int delay, int descr, dbref player, dbref loc, dbref trig,
              const char *mpi, const char *cmdstr, const char *argstr,
              int listen_p, int omesg_p)
{
    int subtyp = TQ_MPI_QUEUE;

    if (delay >= 1) {
        subtyp = TQ_MPI_DELAY;
    }
    if (listen_p) {
        subtyp |= TQ_MPI_LISTEN;
    }
    if (omesg_p) {
        subtyp |= TQ_MPI_OMESG;
    }
    return add_event(TQ_MPI_TYP, subtyp, delay, descr, player, loc, trig,
                     NOTHING, NULL, mpi, cmdstr, argstr);
}


int
add_muf_queue_event(int descr, dbref player, dbref loc, dbref trig, dbref prog,
                    const char *argstr, const char *cmdstr, int listen_p)
{
    return add_event(TQ_MUF_TYP, (listen_p ? TQ_MUF_LISTEN : TQ_MUF_QUEUE), 0,
                     descr, player, loc, trig, prog, NULL, argstr, cmdstr,
                     NULL);
}


int
add_muf_delayq_event(int delay, int descr, dbref player, dbref loc, dbref trig,
                     dbref prog, const char *argstr, const char *cmdstr,
                     int listen_p)
{
    return add_event(TQ_MUF_TYP, (listen_p ? TQ_MUF_LISTEN : TQ_MUF_QUEUE),
                     delay, descr, player, loc, trig, prog, NULL, argstr,
                     cmdstr, NULL);
}


int
add_muf_read_event(int descr, dbref player, dbref prog, struct frame *fr)
{
    if (OkObj(player))
        FLAGS(player) |= (INTERACTIVE | READMODE);

    return add_event(TQ_MUF_TYP, TQ_MUF_READ, -1, descr, player, -1, fr->trig,
                     prog, fr, "READ", NULL, NULL);
}

int
add_muf_tread_event(int descr, dbref player, dbref prog, struct frame *fr,
                    int delay)
{
    if (OkObj(player))
        FLAGS(player) |= (INTERACTIVE | READMODE);

    return add_event(TQ_MUF_TYP, TQ_MUF_TREAD, delay, descr, player, -1,
                     fr->trig, prog, fr, "READ", NULL, NULL);
}

int
add_muf_timer_event(int descr, dbref player, dbref prog, struct frame *fr,
                    int delay, char *id)
{
    char buf[40];

    sprintf(buf, "TIMER.%.32s", id);
    fr->timercount++;
    return add_event(TQ_MUF_TYP, TQ_MUF_TIMER, delay, descr, player, -1,
                     fr->trig, prog, fr, buf, NULL, NULL);
}

int
add_muf_delay_event(int delay, int descr, dbref player, dbref loc, dbref trig,
                    dbref prog, struct frame *fr, const char *mode)
{
    return add_event(TQ_MUF_TYP, TQ_MUF_DELAY, delay, descr, player, loc, trig,
                     prog, fr, mode, NULL, NULL);
}



/* Changed the return to INT and added the cmd parameter. This
 * will help with keeping events from breaking when using in-server
 * globals like WHO and QUIT. -Akari
 */
int
read_event_notify(int descr, dbref player, const char *cmd)
{
    timequeue ptr;


    if (muf_event_read_notify(descr, player, cmd)) {
        return 1;
    }

	mutex_lock(tq_mutex);

    ptr = tqhead;
    while (ptr) {
        if (ptr->uid != NOTHING ? ptr->uid == player : ptr->descr == descr) {
            if (ptr->fr && ptr->fr->multitask != BACKGROUND) {
                /* Don't queue the event unless there's actually 
                 * a command. this is to accomidate changes in
                 * interface.c to avoid problems with events and WHO -Akari
                 */
                if (*cmd || ptr->fr->wantsblanks) {
                    struct inst temp;

                    temp.type = PROG_INTEGER;
                    temp.data.number = descr;
                    muf_event_add(ptr->fr, "READ", &temp, 1);
					mutex_unlock(tq_mutex);
                    return 1;
                }
            }
        }
        ptr = ptr->next;
    }

	mutex_unlock(tq_mutex);
    return 0;
}

/* I had to add the 'timequeue event' parameter to handle_read_event() in order
 * to fix support for TREAD. Since handle_read_event() is called -after- the
 * TREAD event is removed from the timequeue, the event itself has to be 
 * passed in order to work with it at all. When this function is called from
 * anywhere else, 'event' should always be passed as NULL. 
 */

void
handle_read_event(int descr, dbref player, const char *command,
                  struct timenode *event, int len, int wclen)
{
    struct frame *fr;
    timequeue ptr;
    int typ, oldflags = 0;
    short nothing_flag;
    dbref prog;
    struct descriptor_data *curdescr = NULL;

    nothing_flag = 0;
    if (command == NULL)
        nothing_flag = 1;

    if (OkObj(player)) {
        oldflags = FLAGS(player);
        FLAGS(player) &= ~(INTERACTIVE | READMODE);
    } else {
        if ((curdescr = get_descr(descr, NOTHING))) {
            curdescr->interactive = 0;
            DR_RAW_REM_FLAGS(curdescr, DF_INTERACTIVE);
        }
    }

	if (!event) { /* If event is NULL, we don't want to do this part. */
		mutex_lock(tq_mutex);
		for (ptr = tqhead; ptr; ptr = ptr->next) {
			if (ptr->running)
				continue;
			if (ptr->typ == TQ_MUF_TYP && (ptr->subtyp == TQ_MUF_READ ||
										   ptr->subtyp == TQ_MUF_TREAD) &&
				(OkObj(player) ? ptr->uid == player : ptr->descr == descr)) {
				break;
			}
		}
		mutex_unlock(tq_mutex);
	} else
		ptr = event; /* pointer now points to the passed TREAD event. */
     
    /* When execution gets to here, either ptr will point to the
     * READ event for the player, or else ptr will be NULL.
     */

    if (ptr) {
        /* remember our program, and our execution frame. */

        fr = ptr->fr;
		mutex_lock(fr->mutx);
        /* To allow read to catch blank lines */
        if (!fr->brkpt.debugging || fr->brkpt.isread)
            if (!fr->wantsblanks && command && !*command && !event) {
                /* put flags back on player and return */
                if (OkObj(player)) {
                    FLAGS(player) = oldflags;
                } else {
                    if ((curdescr = get_descr(descr, NOTHING))) {
                        curdescr->interactive = 2;
                        DR_RAW_ADD_FLAGS(curdescr, DF_INTERACTIVE);
                    }
                }

				mutex_unlock(fr->mutx);
                return;
            }
        typ = ptr->subtyp;
        prog = ptr->called_prog;

        if (fr->brkpt.debugging && !fr->brkpt.isread) {
            /* We're in the MUF debugger!  Call it with the input line. */
            if (command) {
                if (muf_debugger(descr, player, prog, command, fr)) {
                    /* MUF Debugger exited.  Free up the program frame & exit */
					mutex_unlock(fr->mutx);
                    dequeue_frame(fr);
                    return;
                }
            } else {
                if (muf_debugger(descr, player, prog, "", fr)) {
                    /* MUF Debugger exited.  Free up the program frame & exit */
					mutex_unlock(fr->mutx);
                    dequeue_frame(fr);
                    return;
                }
            }
        } else if (command || event) {
            /* This is a MUF READ event. */
            if (command && !string_compare(command, BREAK_COMMAND) &&
                (OkObj(player)
                 && ((MLevel(player) >= tp_min_progbreak_lev)
                     || (Wiz(player))))) {
                /* Whoops!  The user typed @Q.  Free the frame and exit. */
				mutex_unlock(fr->mutx);
                dequeue_frame(fr);
                return;
            }

            if ((fr->argument.top >= STACK_SIZE) ||  (nothing_flag && fr->argument.top >= STACK_SIZE - 1)) {
                /* Uh oh! That MUF program's stack is full!
                 * Print an error, free the frame, and exit.
                 */
                if (OkObj(player))
                    notify_nolisten(player, "Program stack overflow.", 1);

				mutex_unlock(fr->mutx);
                dequeue_frame(fr);
                return;
            }

            /* Everything looks okay.  Lets stuff the input line
             * on the program's argument stack as a string item.
             *
             * len and wclen are calculated inside of process_input.
             * process_input -> save_command -> add_to_queue -> make_text_block.
             * process_commands -> do_command -> process_command ->
             *                     interactive -> handle_read_event.
             * 
             * -brevantes
             */
            fr->argument.st[fr->argument.top].type = PROG_STRING;
            fr->argument.st[fr->argument.top++].data.string = alloc_prog_string_exact(command ? command : "", len, wclen);
                //alloc_prog_string(command ? command : "");
            if (typ == TQ_MUF_TREAD) {
                if (nothing_flag) {
                    fr->argument.st[fr->argument.top].type = PROG_INTEGER;
                    fr->argument.st[fr->argument.top++].data.number = 0;
                } else {
                    fr->argument.st[fr->argument.top].type = PROG_INTEGER;
                    fr->argument.st[fr->argument.top++].data.number = 1;
                }
            }
        }
		mutex_unlock(fr->mutx);

		mutex_lock(tq_mutex);
        /*
         * Check for any other READ events for this player.
         * If there are any, set the READ related flags.
         */
        ptr = tqhead;
        while (ptr) {
            if (ptr->typ == TQ_MUF_TYP && (ptr->subtyp == TQ_MUF_READ ||
                                           ptr->subtyp == TQ_MUF_TREAD)) {
                if (OkObj(player) && ptr->uid == player) {
                    FLAGS(player) |= (INTERACTIVE | READMODE);
                }
            }
            ptr = ptr->next;
        }
		mutex_unlock(tq_mutex);
    }
}


void
next_timequeue_event(struct thread_data *thread)
{
    struct frame *tmpfr;
    int tmpfg;
    timequeue event, event2;
    int maxruns = 0;
    int forced_pid = 0;
    time_t rtime = current_systime;
    struct descriptor_data *curdescr = NULL;

	mutex_lock(tq_mutex);

    for (event = tqhead; event; event = event->next) {
        if (event->typ == TQ_MUF_TYP && event->subtyp == TQ_MUF_READ)
			continue;
       	if (event->running)
			continue;
		if (rtime >= event->when)
			break;
	}

	if (event) {
		char buf[BUFFER_LEN];
		//printf("Thread %2d running %5d - %s\r\n", (thread ? thread->id : 0), event->eventnum, unparse_object(MAN, event->called_prog, buf));
		event->running = 1;
		event->thread = thread;
		//remove_timenode(event);

        forced_pid = event->eventnum;
        //event->eventnum = 0;
        if (event->typ == TQ_MPI_TYP) {
            char cbuf[BUFFER_LEN];
            int ival;

            ival =
                (event->subtyp & TQ_MPI_OMESG) ? MPI_ISPUBLIC : MPI_ISPRIVATE;
            if (event->subtyp & TQ_MPI_LISTEN) {
                ival |= MPI_ISLISTENER;
                do_parse_mesg(event->descr, event->uid, event->trig,
                              event->called_data, "(MPIlisten)", cbuf, ival, event->str3 ? event->str3 : "", event->command ? event->command : "");
            } else if ((event->subtyp & TQ_MPI_SUBMASK) == TQ_MPI_DELAY) {
                do_parse_mesg(event->descr, event->uid, event->trig,
                              event->called_data, "(MPIdelay)", cbuf, ival, event->str3 ? event->str3 : "", event->command ? event->command : "");
            } else {
                do_parse_mesg(event->descr, event->uid, event->trig,
                              event->called_data, "(MPIqueue)", cbuf, ival, event->str3 ? event->str3 : "", event->command ? event->command : "");
            }
            if (*cbuf) {
                if (!(event->subtyp & TQ_MPI_OMESG)) {
                    notify_nolisten(event->uid, cbuf, 1);
                } else {
                    char bbuf[BUFFER_LEN];
                    dbref plyr;

                    sprintf(bbuf, ">> %.4000s %.*s",
                            NAME(event->uid),
                            (int) (4000 - strlen(NAME(event->uid))),
                            pronoun_substitute(event->descr, event->uid, cbuf));
                    plyr = DBFETCH(event->loc)->contents;
                    for (; plyr != NOTHING; plyr = DBFETCH(plyr)->next) {
                        if (Typeof(plyr) == TYPE_PLAYER && plyr != event->uid)
                            notify_nolisten(plyr, bbuf, 0);
                    }
                }
            }
			event->fr = NULL;
			remove_timenode(event);
			free_timenode(event);
        } else if (event->typ == TQ_MUF_TYP) {
            if (Typeof(event->called_prog) == TYPE_PROGRAM) {
                if (event->subtyp == TQ_MUF_DELAY) {
					int freed = 0;
                    short tmpbl = 0;
					int err = 0;

					mutex_lock(event->fr->mutx);
					mutex_unlock(tq_mutex);

                    if (OkObj(event->uid)) {
                        /* tmpcp = DBFETCH(event->uid)->sp.player.curr_prog; */
                        //tmpbl = DBFETCH(event->uid)->sp.player.block;
                    } else {
                        if ((curdescr = get_descr(event->descr, NOTHING)))
                            tmpbl = curdescr->block;
                    }
                    tmpfg = (event->fr->multitask != BACKGROUND);
                    interp_loop(event->uid, event->called_prog, event->fr, 0, thread, &err, event);
					mutex_lock(tq_mutex);
					if (err == -1)
						mutex_unlock(event->fr->mutx);
					if (err) {                   //If err == 1, prog_free() has been called and the frame can no longer be accessed.
						for (event2 = tqhead; event2; event2 = event2->next) {
							//Since it's possible the program got dequeue_prog'd, we need to double-check here.
							if (event == event2) {
								freed = 1;
								event->fr = NULL;
								remove_timenode(event);
								free_timenode(event);
								break;%
							}
						}
						if (!freed)
							event->running = 0;
					} else {
						event->called_prog = event->fr->prog;
						event->uid = event->fr->player;
						event->running = 0;
						rotate_timenode_to_end(event);
						mutex_unlock(event->fr->mutx);
					}
                    //if (!tmpfg) {
                    //    if (OkObj(event->uid)) {
                    //        //DBFETCH(event->uid)->sp.player.block = tmpbl;
                    //    } else {
                    //        if ((curdescr = get_descr(event->descr, NOTHING)))
                    //            curdescr->block = tmpbl;
                    //    }
                    //}
                } else if (event->subtyp == TQ_MUF_TIMER) {
                    struct inst temp;

                    temp.type = PROG_INTEGER;
                    temp.data.number = (int)event->when;
                    event->fr->timercount--;
                    muf_event_add(event->fr, event->called_data, &temp, 0);
					event->fr = NULL;
					remove_timenode(event);
					free_timenode(event);
                } else if (event->subtyp == TQ_MUF_TREAD) {
                    handle_read_event(event->descr, event->uid, NULL, event, 0, 0);
					event->fr = NULL;
					remove_timenode(event);
					free_timenode(event);
                } else {
                    event->fr = interp(event->descr, event->uid, event->loc,
                                   event->called_prog, event->trig, BACKGROUND,
                                   STD_HARDUID, forced_pid, event->called_data ? event->called_data : "", event->command ? event->command : "", 1);
					event->fr = NULL;
					remove_timenode(event);
					free_timenode(event);
					//event->subtyp = TQ_MUF_DELAY;
					//event->when = 0;
					//event->running = 0;
                }
            }
			//printf("Done running.\r\n");
        }
    }

	mutex_unlock(tq_mutex);
}


int
in_timequeue(int pid)
{
    timequeue ptr = tqhead;

    if (!pid)
        return 0;
    if (muf_event_pid_frame(pid))
        return 1;
    if (!tqhead)
        return 0;
    while ((ptr) && (ptr->eventnum != pid))
        ptr = ptr->next;
    if (ptr)
        return 1;
    return 0;
}

int
in_timequeue_only(int pid)
{
    timequeue ptr = tqhead;

    if (!pid)
        return 0;
    if (!tqhead)
        return 0;
    while ((ptr) && (ptr->eventnum != pid))
        ptr = ptr->next;
    if (ptr)
        return 1;
    return 0;
}

struct frame *
timequeue_pid_frame(int pid)
{
    struct frame *out = NULL;
    timequeue ptr;

    if (!pid)
        return NULL;
    out = muf_event_pid_frame(pid);
    if (out != NULL)
        return out;

    ptr = tqhead;
    if (!ptr)
        return NULL;
    while ((ptr) && (ptr->eventnum != pid))
        ptr = ptr->next;
    if (ptr)
        return ptr->fr;
    return NULL;
}


time_t
next_event_time(void)
{
    time_t rtime = current_systime;

    if (tqhead) {
        if (tqhead->when == -1) {
            return (-1L);
        } else if (rtime >= tqhead->when) {
            return (0L);
        } else {
            return ((time_t) (tqhead->when - rtime));
        }
    }
    return (-1L);
}

void
list_events(dbref player)
{
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    int count = 0;
    timequeue ptr = tqhead;
    time_t rtime = current_systime;
    time_t etime = 0;
    double pcnt = 0.0;
	char buft[128];

	mutex_lock(tq_mutex);

    anotify_nolisten(player,
                     CINFO "Thrd     PID Next  Run KInst %CPU Prog#   Player", 1);

    while (ptr) {
		if (ptr->running)
			strcpy(buf2, "Run");
		else
			strcpy(buf2, ((ptr->when - rtime) > 0) ? time_format_2((time_t) (ptr->when - rtime), buft) : "Due");
		
        if (ptr->fr) {
            etime = rtime - ptr->fr->started;
            if (etime > 0) {
                pcnt = ptr->fr->totaltime.tv_sec;
                pcnt += ptr->fr->totaltime.tv_usec / 1000000;
                pcnt = pcnt * 100 / etime;
                if (pcnt > 100.0) {
                    pcnt = 100.0;
                }
            } else {
                pcnt = 0.0;
            }
        }
        if (ptr->typ == TQ_MUF_TYP && ptr->subtyp == TQ_MUF_DELAY) {
            (void) sprintf(buf, "%4d %8d %4s %4s %5lld %4.1f #%-6d %-16s %.512s",
                           (ptr->fr->thread ? ptr->fr->thread->id : 0), ptr->eventnum, buf2,
                           time_format_2((long) etime, buft),
                           (ptr->fr->instcnt / 1000), pcnt,
                           ptr->called_prog,
                           (OkObj(ptr->uid)) ? NAME(ptr->uid) : "(Login)",
                           ptr->called_data);
        } else if (ptr->typ == TQ_MUF_TYP && ptr->subtyp == TQ_MUF_READ) {
            (void) sprintf(buf, "%4d %8d %4s %4s %5lld %4.1f #%-6d %-16s %.512s",
                           (ptr->fr->thread ? ptr->fr->thread->id : 0), ptr->eventnum, "--",
                           time_format_2((long) etime, buft),
                           (ptr->fr->instcnt / 1000), pcnt,
                           ptr->called_prog,
                           (OkObj(ptr->uid)) ? NAME(ptr->uid) : "(Login)",
                           ptr->called_data);
        } else if (ptr->typ == TQ_MUF_TYP && ptr->subtyp == TQ_MUF_TIMER) {
            (void) sprintf(buf, "%4d (%6d) %4s %4s %5lld %4.1f #%-6d %-16s %.512s",
                           (ptr->fr->thread ? ptr->fr->thread->id : 0), ptr->eventnum, buf2,
                           time_format_2((long) etime, buft),
                           (ptr->fr->instcnt / 1000), pcnt,
                           ptr->called_prog,
                           (OkObj(ptr->uid)) ? NAME(ptr->uid) : "(Login)",
                           ptr->called_data);

        } else if (ptr->typ == TQ_MUF_TYP && ptr->subtyp == TQ_MUF_TREAD) {
            (void) sprintf(buf, "%4d %8d %4s %4s %5lld %4.1f #%-6d %-16s %.512s",
						   (ptr->fr->thread ? ptr->fr->thread->id : 0),
                           ptr->eventnum, buf2,
                           time_format_2((long) etime, buft),
                           (ptr->fr->instcnt / 1000), pcnt,
                           ptr->called_prog,
                           (OkObj(ptr->uid)) ? NAME(ptr->uid) : "(Login)",
                           ptr->called_data);
        } else if (ptr->typ == TQ_MPI_TYP) {
            (void) sprintf(buf,
                           "    %8d %4s   --   MPI   -- #%-6d %-16s \"%.512s\"",
                           ptr->eventnum, buf2, ptr->trig, NAME(ptr->uid),
                           ptr->called_data);
        } else {
            (void) sprintf(buf,
                           "    %8d %4s   0s     0   -- #%-6d %-16s \"%.512s\"",
                           ptr->eventnum, buf2, ptr->called_prog,
                           (OkObj(ptr->uid)) ? NAME(ptr->uid) : "(Login)",
                           ptr->called_data);
        }
        if (Mage(OWNER(player)) || ((ptr->called_prog != NOTHING) &&
                                    (OWNER(ptr->called_prog) == OWNER(player)))
            || (ptr->uid == player))
            notify_nolisten(player, buf, 1);
        else if (ptr->called_prog == NOTHING) {
            fprintf(stderr, "Strangeness alert!  @ps produces %s\n", buf);
        } else if (ptr->called_prog != NOTHING
                   && OWNER(ptr->called_prog) == OWNER(player)) {
            notify_nolisten(player, buf, 1);
        }
        ptr = ptr->next;
        count++;
    }
    count += muf_event_list(player, "%4d %8d %4s %4s %5lld %4.1f #%-6d %-16s %.512s");
    sprintf(buf, CINFO "%d events.  %d threads.", count, thread_count);
    anotify_nolisten(player, buf, 1);

	mutex_unlock(tq_mutex);
}

int
descr_running_queue(int descr)
{
    int icount = 0;
    timequeue ptr = tqhead;

    while (ptr) {
        if (ptr->descr == descr) {
            icount++;
        }
        ptr = ptr->next;
    }
    return icount;
}

/* called by the getpids prim in p_misc.c */
stk_array *
get_pids(dbref ref)
{
    struct inst temp1;
    stk_array *nw;

    timequeue ptr = tqhead;

    nw = new_array_packed(0);
    while (ptr) {
        if (((ptr->typ != TQ_MPI_TYP) ? (ptr->called_prog == ref)
             : (ptr->trig == ref)) || (ptr->uid == ref) || (ref < NOTHING)) {
            temp1.type = PROG_INTEGER;
            temp1.data.number = ptr->eventnum;
            array_appenditem(&nw, &temp1);
            CLEAR(&temp1);
        }
        ptr = ptr->next;
    }
    nw = get_mufevent_pids(nw, ref);
    return nw;
}

/* Used with the GETPIDINFO prim, this function packs a 
 * dictionary array with information about the PID in
 * question and returns it to the stack. The GETPIDINFO
 * prim is in p_misc.c
 */
stk_array *
get_pidinfo(int pid)
{
    struct inst temp1, temp2;
    stk_array *nw;
    time_t rtime = current_systime;
    time_t etime = 0;
    double pcnt = 0.0;

    timequeue ptr = tqhead;

    nw = new_array_dictionary();
    while (ptr) {
        if (ptr->eventnum == pid) {
            if (ptr->typ != TQ_MUF_TYP || ptr->subtyp != TQ_MUF_TIMER) {
                break;
            }
        }
        ptr = ptr->next;
    }
    if (ptr && (ptr->eventnum == pid) &&
        (ptr->typ != TQ_MUF_TYP || ptr->subtyp != TQ_MUF_TIMER)) {
        if (ptr->fr) {
            etime = rtime - ptr->fr->started;
            if (etime > 0) {
                pcnt = ptr->fr->totaltime.tv_sec;
                pcnt += ptr->fr->totaltime.tv_usec / 1000000;
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
        temp2.data.number = ptr->eventnum;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("CALLED_PROG");
        temp2.type = PROG_OBJECT;
        temp2.data.objref = ptr->called_prog;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("TRIG");
        temp2.type = PROG_OBJECT;
        temp2.data.objref = ptr->trig;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("PLAYER");
        temp2.type = PROG_OBJECT;
        temp2.data.objref = ptr->uid;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("CALLED_DATA");
        temp2.type = PROG_STRING;
        temp2.data.string = alloc_prog_string(ptr->called_data);
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("INSTCNT");
        temp2.type = PROG_INTEGER;
        temp2.data.number = ptr->fr ? ptr->fr->instcnt : 0;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("DESCR");
        temp2.type = PROG_INTEGER;
        temp2.data.number = ptr->descr;
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
        temp2.data.number = (int) ptr->when;
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("STARTED");
        temp2.type = PROG_INTEGER;
        temp2.data.number = (int) (ptr->fr ? ptr->fr->started : 0);
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("TYPE");
        temp2.type = PROG_STRING;
        temp2.data.string =
            (ptr->typ == TQ_MUF_TYP) ? alloc_prog_string("MUF") : (ptr->typ ==
                                                                   TQ_MPI_TYP) ?
            alloc_prog_string("MPI") : alloc_prog_string("UNK");
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
        temp1.type = PROG_STRING;
        temp1.data.string = alloc_prog_string("SUBTYPE");
        temp2.type = PROG_STRING;
        temp2.data.string = (ptr->typ == TQ_MUF_READ) ?
            alloc_prog_string("READ") :
            (ptr->typ == TQ_MUF_TREAD) ? alloc_prog_string("TREAD") :
            (ptr->typ == TQ_MUF_TIMER) ? alloc_prog_string("TIMER") :
            (ptr->typ == TQ_MUF_DELAY) ? alloc_prog_string("DELAY") :
            alloc_prog_string("");
        array_setitem(&nw, &temp1, &temp2);
        CLEAR(&temp1);
        CLEAR(&temp2);
    } else {
        nw = get_mufevent_pidinfo(nw, pid);
    }
    return nw;
}
int
dequeue_frame(struct frame *fr)
{

    timequeue event;
	int count = 0;

	mutex_lock(tq_mutex);

    for (event = tqhead; event; event = event->next) {
        if (event->fr == fr) {
			fr->err = -1;
			while (event->running) Sleep(10);
			event->fr = NULL;
			timequeue tmp = event;
			event = event->next;
			remove_timenode(tmp);
			free_timenode(tmp);
			count++;
		}
	}

	mutex_unlock(tq_mutex);

	return count;
}

/*
 * Sleeponly values:
 *     0: kill all matching processes
 *     1: kill only matching sleeping processes
 *     2: kill only matching foreground processes
 */
int
dequeue_prog(dbref program, int sleeponly, struct frame *exclude)
{
    int count = 0;
    timequeue event;

	mutex_lock(tq_mutex);
	DBLOCK(program);

    for (event = tqhead; event; event = event->next) {
		if (exclude && event->fr == exclude)
			continue;
		if (sleeponly == 1 && event->typ == TQ_MUF_TYP && (event->subtyp == TQ_MUF_LISTEN || event->subtyp == TQ_MUF_QUEUE))
			continue;
		if (sleeponly == 2 && event->fr->multitask != FOREGROUND)
			continue;
        if (event && ((!sleeponly && has_refs(program, event)) || event->called_prog == program || event->uid == program)) {
			timequeue tmp = event;
			remove_timenode(event);
			
			if (event->fr) {
				event->fr->err = -1;
				mutex_unlock(tq_mutex);
				if (event->running) { //no point waiting on an event that cannot run.
					while (event->running) Sleep(10);
				} else if (event->typ == TQ_MUF_TYP && event->subtyp == TQ_MUF_DELAY) {
					prog_clean(event->fr);
				}
				mutex_lock(tq_mutex);
			}
			event->fr = NULL;

			free_timenode(event);
			count++;
			//We have to start over since another thread could have altered
			// the queue while we were waiting for the program to die.
			if (!(event = tqhead))
				break; 
		}
	}


    /* We used to treat MUF events as only background processes.
     * Now we pass the actual sleeponly parameter so that we can treat
     * them as foreground processes when needed when handling READ events.
     * -Akari
     */
    count += muf_event_dequeue(program, sleeponly);

    /* Make sure to re-set any READ/INTERACTIVE flags needed */
    for (event = tqhead; event; event = event->next) {
        if (OkObj(event->uid) && event->typ == TQ_MUF_TYP
            && (event->subtyp == TQ_MUF_READ || event->subtyp == TQ_MUF_TREAD)) {
            FLAGS(event->uid) |= (INTERACTIVE | READMODE);
        }
    }

	DBUNLOCK(program)
	mutex_unlock(tq_mutex);

    return (count);
}

/* Needed to add this second dequeue function since there is
 * a chance that the descr would match a player or prog dbref, which
 * would've been a big problem. This function is intended to dequeue
 * all the functions belonging to a descriptor and is only called
 * if the descriptor never connected to a player, since there are 
 * already routines that handle it when a player disconnects.
 */
int
dequeue_prog_descr(int descr, int sleeponly)
{
	return 0;

    int count = 0;
    timequeue tmp, ptr;
    struct descriptor_data *curdescr = NULL;

    while (tqhead && tqhead->descr == descr
           && ((tqhead->fr) ? (!((tqhead->fr->multitask == BACKGROUND) &&
                                 (sleeponly == 2))) : (!sleeponly))) {
        ptr = tqhead;
        tqhead = tqhead->next;
        free_timenode(ptr);
        process_count--;
        count++;
    }
    if (tqhead) {
        tmp = tqhead;
        ptr = tqhead->next;
        while (ptr) {
            if ((ptr->descr == descr) &&
                ((ptr->fr) ? (!((ptr->fr->multitask == BACKGROUND) &&
                                (sleeponly == 2))) : (!sleeponly))) {
                tmp->next = ptr->next;
                free_timenode(ptr);
                process_count--;
                count++;
                ptr = tmp;
            } else {
                tmp = ptr;
                ptr = ptr->next;
            }
        }
    }
    /* We used to treat event processes as background only
     * but with READ events, we really need to acknowledge that
     * they have forground possibilities as well. So now we pass
     * the sleeponly parameter to the muf_event_dequeue functions
     */
    count += muf_event_dequeue_descr(descr, sleeponly);

    for (ptr = tqhead; ptr; ptr = ptr->next) {
        if (ptr->typ == TQ_MUF_TYP && (ptr->subtyp == TQ_MUF_READ ||
                                       ptr->subtyp == TQ_MUF_TREAD)) {
            if ((curdescr = get_descr(descr, NOTHING)))
                curdescr->interactive = 0;
        }
    }
    return (count);
}

int
dequeue_process(int pid)
{
    timequeue tmp, ptr;
    short deqflag = 0;

	return 0;

    if (!pid)
        return 0;

    if (muf_event_dequeue_pid(pid)) {
        process_count--;
        deqflag = 1;
    }

    tmp = ptr = tqhead;
    while (ptr) {
        if (pid == ptr->eventnum) {
            if (tmp == ptr) {
                tqhead = tmp = tmp->next;
                free_timenode(ptr);
                ptr = tmp;
            } else {
                tmp->next = ptr->next;
                free_timenode(ptr);
                ptr = tmp->next;
            }
            process_count--;
            deqflag = 1;
        } else {
            tmp = ptr;
            ptr = ptr->next;
        }
    }

    if (!deqflag)
        return 0;

    for (ptr = tqhead; ptr; ptr = ptr->next) {
        if (OkObj(ptr->uid) && ptr->typ == TQ_MUF_TYP
            && (ptr->subtyp == TQ_MUF_READ || ptr->subtyp == TQ_MUF_TREAD)) {
            FLAGS(ptr->uid) |= (INTERACTIVE | READMODE);
        }
    }

    return 1;
}

int
dequeue_timers(int pid, char *id)
{
    char buf[40];
    timequeue tmp, ptr;
    int deqflag = 0;


	mutex_lock(tq_mutex);

    if (!pid)
        return 0;

    if (id)
        sprintf(buf, "TIMER.%.30s", id);

    ptr = tqhead;
    while (ptr) {
		tmp = ptr->next;
        if (pid == ptr->eventnum &&
            ptr->typ == TQ_MUF_TYP && ptr->subtyp == TQ_MUF_TIMER &&
            (!id || !strcmp(ptr->called_data, buf)))
		{
            ptr->fr->timercount--;
            ptr->fr = NULL;
			remove_timenode(ptr);
            free_timenode(ptr);
            process_count--;
            deqflag = 1;
        }
		ptr = tmp;
    }

	mutex_unlock(tq_mutex);

    return deqflag;
}


void
do_dequeue(int descr, dbref player, const char *arg1)
{
    char buf[BUFFER_LEN];
    int count;
    dbref match;
    struct match_data md;
    timequeue tmp, ptr = tqhead;

    if (Guest(player)) {
        anotify_fmt(player, CFAIL "%s", tp_noguest_mesg);
        return;
    }

	return;

    if (*arg1 == '\0') {
        anotify_nolisten(player, CINFO "Dequeue which event?", 1);
    } else {
        if (!string_compare(arg1, "all")) {
            if (!Mage(OWNER(player))) {
                anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
                return;
            }
            while (ptr) {
                tmp = ptr;
                tqhead = ptr = ptr->next;
                free_timenode(tmp);
                process_count--;
            }
            tqhead = NULL;
            muf_event_dequeue(NOTHING, 0);
            anotify_nolisten(player, CSUCC "Time queue cleared.", 1);
        } else {
            if (!number(arg1)) {
                init_match(descr, player, arg1, NOTYPE, &md);
                match_absolute(&md);
                match_everything(&md);

                match = noisy_match_result(&md);
                if (match == NOTHING) {
                    anotify_nolisten(player,
                                     CINFO
                                     "I don't know what you want to dequeue!",
                                     1);
                    return;
                }
                if (!valid_objref(match)) {
                    anotify_nolisten(player, CINFO "Invalid object.", 1);
                    return;
                }
                if (!controls(player, match)) {
                    anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
                    return;
                }
                count = dequeue_prog(match, 0, NULL);
                if (!count) {
                    anotify_nolisten(player,
                                     CINFO "That program isn't running.", 1);
                    return;
                }
                if (count > 1) {
                    sprintf(buf, CSUCC "%d processes dequeued.", count);
                } else {
                    sprintf(buf, CSUCC "Process dequeued.");
                }
                anotify_nolisten(player, buf, 1);
            } else {
                if ((count = atoi(arg1))) {
                    if (!(control_process(player, count))) {
                        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
                        return;
                    }
                    if (!(dequeue_process(count))) {
                        anotify_nolisten(player, CINFO "No such process.", 1);
                        return;
                    }
                    process_count--;
                    anotify_nolisten(player, CSUCC "Process dequeued.", 1);
                } else {
                    anotify_nolisten(player,
                                     CINFO
                                     "What process do you want to dequeue?", 1);
                }
            }
        }
    }
    return;
}

/* Checks the MUF timequeue for address references on the stack or */
/* dbref references on the callstack */
int
has_refs(dbref program, timequeue ptr)
{
    int loop;

    if (ptr->typ != TQ_MUF_TYP || !(ptr->fr) ||
        Typeof(program) != TYPE_PROGRAM ||
        !(DBFETCH(program)->sp.program.instances)) {
        /*
           fprintf(stderr, "Program %s no references, terminating.\n",
           NAME(program));
         */
        return 0;
    }

    for (loop = 1; loop < ptr->fr->caller.top; loop++) {
        if (ptr->fr->caller.st[loop] == program)
            return 1;
    }

    for (loop = 0; loop < ptr->fr->argument.top; loop++) {
        if (ptr->fr->argument.st[loop].type == PROG_ADD &&
            ptr->fr->argument.st[loop].data.addr->progref == program)
            return 1;
    }

    return 0;
}


int
scan_instances(dbref program)
{
    timequeue tq = tqhead;
    int i = 0, loop;

    while (tq) {
        if (tq->typ == TQ_MUF_TYP && tq->fr) {
            if (tq->called_prog == program) {
                i++;
            }
            for (loop = 1; loop < tq->fr->caller.top; loop++) {
                if (tq->fr->caller.st[loop] == program)
                    i++;
            }
            for (loop = 0; loop < tq->fr->argument.top; loop++) {
                if (tq->fr->argument.st[loop].type == PROG_ADD &&
                    tq->fr->argument.st[loop].data.addr->progref == program)
                    i++;
            }
        }
        tq = tq->next;
    }
    return i;
}


static int propq_level = 0;

void
propqueue(int descr, dbref player, dbref where, dbref trigger, dbref what,
          dbref xclude, const char *propname, const char *toparg, int mlev,
          int mt)
{
    const char *tmpchar;
    const char *pname;
    dbref the_prog;
    char buf[BUFFER_LEN];
    char exbuf[BUFFER_LEN];

    the_prog = NOTHING;
    tmpchar = NULL;

    /* queue up program referred to by the given property */
    if (((the_prog = get_property_dbref(what, propname)) != NOTHING) ||
        (tmpchar = get_property_class(what, propname))) {
        if ((tmpchar && *tmpchar) || the_prog != NOTHING) {
            if (tmpchar) {
                if (*tmpchar == '&') {
                    the_prog = AMBIGUOUS;
                } else if (*tmpchar == NUMBER_TOKEN && number(tmpchar + 1)) {
                    the_prog = (dbref) atoi(++tmpchar);
                } else if (*tmpchar == REGISTERED_TOKEN) {
                    the_prog = find_registered_obj(what, tmpchar);
                } else if (number(tmpchar)) {
                    the_prog = (dbref) atoi(tmpchar);
                } else {
                    the_prog = NOTHING;
                }
            } else {
                if (the_prog == AMBIGUOUS)
                    the_prog = NOTHING;
            }
            if (the_prog != AMBIGUOUS) {
                if (the_prog < 0 || the_prog >= db_top) {
                    the_prog = NOTHING;
                } else if (Typeof(the_prog) != TYPE_PROGRAM) {
                    the_prog = NOTHING;
                } else if (OkObj(player) && (OWNER(the_prog) != OWNER(player))
                           && !(FLAGS(the_prog) & LINK_OK)) {
                    the_prog = NOTHING;
                } else if (MLevel(the_prog) < mlev) {
                    the_prog = NOTHING;
                } else if (MLevel(OWNER(the_prog)) < mlev) {
                    the_prog = NOTHING;
                } else if (the_prog == xclude) {
                    the_prog = NOTHING;
                }
            }
            if (propq_level < 8) {
                propq_level++;
                if (the_prog == AMBIGUOUS) {
                    char cbuf[BUFFER_LEN];
                    int ival;

                    ival = (mt == 0) ? MPI_ISPUBLIC : MPI_ISPRIVATE;
                    do_parse_mesg(descr, player, what, tmpchar + 1,
                                  "(MPIqueue)", cbuf, ival, "", toparg);
                    if (*cbuf) {
                        if (mt) {
                            notify_nolisten(player, cbuf, 1);
                        } else {
                            char bbuf[BUFFER_LEN];
                            dbref plyr;

                            sprintf(bbuf, ">> %.4000s",
                                    pronoun_substitute(descr, player, cbuf));
                            plyr = DBFETCH(where)->contents;
                            while (plyr != NOTHING) {
                                if (Typeof(plyr) == TYPE_PLAYER
                                    && plyr != player)
                                    notify_nolisten(plyr, bbuf, 0);
                                plyr = DBFETCH(plyr)->next;
                            }
                        }
                    }
                } else if (the_prog != NOTHING) {
                    struct frame *tmpfr;

                    tmpfr = interp(descr, player, where, the_prog, trigger,
                                   BACKGROUND, STD_HARDUID, 0, toparg ? toparg : "", "Queued event.", 1);
                }
                propq_level--;
            } else {
                anotify_nolisten(player,
                                 CINFO
                                 "Propqueue stopped to prevent infinite loop.",
                                 1);
            }
        }
    }
    strcpy(buf, propname);
    if (is_propdir(what, buf)) {
        strcat(buf, "/");
        while ((pname = next_prop_name(what, exbuf, buf))) {
            strcpy(buf, pname);
            propqueue(descr, player, where, trigger, what, xclude, buf, toparg,
                      mlev, mt);
        }
    }
}


void
envpropqueue(int descr, dbref player, dbref where, dbref trigger, dbref what,
             dbref xclude, const char *propname, const char *toparg, int mlev,
             int mt)
{
    while (what != NOTHING) {
        propqueue(descr, player, where, trigger, what, xclude, propname, toparg,
                  mlev, mt);
        what = getparent(what);
    }
}


void
listenqueue(int descr, dbref player, dbref where, dbref trigger, dbref what,
            dbref xclude, const char *propname, const char *toparg, int mlev,
            int mt, int mpi_p)
{
    const char *tmpchar;
    const char *pname, *sep, *ptr;
    dbref the_prog = NOTHING;
    char buf[BUFFER_LEN];
    char exbuf[BUFFER_LEN];
    char *ptr2;

    if (!(FLAGS(what) & LISTENER) && !(FLAGS(OWNER(what)) & ZOMBIE))
        return;

    the_prog = NOTHING;
    tmpchar = NULL;

    /* queue up program referred to by the given property */
    if (((the_prog = get_property_dbref(what, propname)) != NOTHING) ||
        (tmpchar = get_property_class(what, propname))) {

        if (tmpchar) {
            sep = tmpchar;
            while (*sep) {
                if (*sep == '\\') {
                    sep++;
                } else if (*sep == '=') {
                    break;
                }
                if (*sep)
                    sep++;
            }
            if (*sep == '=') {
                for (ptr = tmpchar, ptr2 = buf; ptr < sep; *ptr2++ = *ptr++) ;
                *ptr2 = '\0';
                strcpy(exbuf, toparg);
                if (!equalstr(buf, exbuf)) {
                    tmpchar = NULL;
                } else {
                    tmpchar = ++sep;
                }
            }
        }

        if ((tmpchar && *tmpchar) || the_prog != NOTHING) {
            if (tmpchar) {
                if (*tmpchar == '&') {
                    the_prog = AMBIGUOUS;
                } else if (*tmpchar == NUMBER_TOKEN && number(tmpchar + 1)) {
                    the_prog = (dbref) atoi(++tmpchar);
                } else if (*tmpchar == REGISTERED_TOKEN) {
                    the_prog = find_registered_obj(what, tmpchar);
                } else if (number(tmpchar)) {
                    the_prog = (dbref) atoi(tmpchar);
                } else {
                    the_prog = NOTHING;
                }
            } else {
                if (the_prog == AMBIGUOUS)
                    the_prog = NOTHING;
            }
            if (the_prog != AMBIGUOUS) {
                if (the_prog < 0 || the_prog >= db_top) {
                    the_prog = NOTHING;
                } else if (Typeof(the_prog) != TYPE_PROGRAM) {
                    the_prog = NOTHING;
                } else if (OkObj(player) && OWNER(the_prog) != OWNER(player) &&
                           !(FLAGS(the_prog) & LINK_OK)) {
                    the_prog = NOTHING;
                } else if (MLevel(the_prog) < mlev) {
                    the_prog = NOTHING;
                } else if (MLevel(OWNER(the_prog)) < mlev) {
                    the_prog = NOTHING;
                } else if (the_prog == xclude) {
                    the_prog = NOTHING;
                }
            }
            if (the_prog == AMBIGUOUS) {
                if (mpi_p) {
                    add_mpi_event(1, descr, player, where, trigger, tmpchar + 1,
                                  (mt ? "Listen" : "Olisten"), toparg, 1,
                                  (mt == 0));
                }
            } else if (the_prog != NOTHING) {
                add_muf_queue_event(descr, player, where, trigger, the_prog,
                                    toparg, "(_Listen)", 1);
            }
        }
    }
    strcpy(buf, propname);
    if (is_propdir(what, buf)) {
        strcat(buf, "/");
        while ((pname = next_prop_name(what, exbuf, buf))) {
            strcpy(buf, pname);
            listenqueue(descr, player, where, trigger, what, xclude, buf,
                        toparg, mlev, mt, mpi_p);
        }
    }
}

void
timequeue_init(void)
{
	tqhead = NULL;
    tqtail = NULL;
	mutex_init(tq_mutex);
}