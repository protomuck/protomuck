/* Muf Interpreter and dispatcher. */

#include "copyright.h"
#include "config.h"

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#ifndef WIN_VC
# include <sys/socket.h>
#endif
#include "db.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "props.h"
#include "strings.h"
#include "interp.h"
#include "mcpgui.h"
#include "mufevent.h"

/* This package performs the interpretation of mud forth programs.
   It is a basically push pop kinda thing, but I'm making some stuff
   inline for maximum efficiency.

   Oh yeah, because it is an interpreted language, please do type
   checking during this time.  While you're at it, any objects you
   are referencing should be checked against db_top.
   */

/* in cases of boolean expressions, we do return a value, the stuff
   that's on top of a stack when a mud-forth program finishes executing.
   In cases where they don't, leave a value, we just return a 0.  Note:
   this stuff does not return string or whatnot.  It at most can be
   relied on to return a boolean value.

   interp sets up a player's frames and so on and prepares it for
   execution.
   */

void 
p_null(PRIM_PROTOTYPE)
{
    return;
}

/* void    (*prim_func[]) (PRIM_PROTOTYPE) = */
void    (*prim_func[]) (PRIM_PROTOTYPE) =
{
    p_null, p_null, p_null, p_null, p_null, p_null, p_null, p_null, p_null,
    /* JMP, READ, TREAD, SLEEP,  CALL,   EXECUTE, RETURN, EVENT_WAITFOR, CATCH */
    PRIMS_CONNECTS_FUNCS,
    PRIMS_DB_FUNCS,
    PRIMS_MATH_FUNCS,
    PRIMS_MISC_FUNCS,
    PRIMS_PROPS_FUNCS,
    PRIMS_STACK_FUNCS,
    PRIMS_STRINGS_FUNCS,
    PRIMS_FLOAT_FUNCS,
    PRIMS_ERROR_FUNCS,
#ifdef FILE_PRIMS
    PRIMS_FILE_FUNCS,
#endif
    PRIMS_ARRAY_FUNCS,
    PRIMS_MCP_FUNCS,
    PRIMS_SOCKET_FUNCS,
    PRIMS_SYSTEM_FUNCS, 
#ifdef SQL_SUPPORT
    PRIMS_MYSQL_FUNCS,
#endif
    PRIMS_INTERNAL_FUNCS,
    NULL
};

struct localvars*
localvars_get(struct frame *fr, dbref prog)
{
	struct localvars *tmp = fr->lvars;

	while (tmp && tmp->prog != prog) tmp = tmp->next;
	if (tmp) {
		/* Pull this out of the middle of the stack. */
		*tmp->prev = tmp->next;
		if (tmp->next)
			tmp->next->prev = tmp->prev;

	} else {
		/* Create a new var frame. */
		int count = MAX_VAR;
		tmp = (struct localvars *) malloc(sizeof(struct localvars));
		tmp->prog = prog;
		while (count-- > 0) {
			tmp->lvars[count].type = PROG_INTEGER;
			tmp->lvars[count].data.number = 0;
		}
	}

	/* Add this to the head of the stack. */
	tmp->next = fr->lvars;
	tmp->prev = &fr->lvars;
	fr->lvars = tmp;
	if (tmp->next)
		tmp->next->prev = &tmp->next;

	return tmp;
}

void
localvar_dupall(struct frame *fr, struct frame *oldfr)
{
	struct localvars *orig;
	struct localvars **targ;

	orig = oldfr->lvars;
	targ = &fr->lvars;

	while (orig) {
		int count = MAX_VAR;
		*targ = (struct localvars*)malloc(sizeof(struct localvars));
		while (count-- > 0)
			copyinst(&orig->lvars[count], &(*targ)->lvars[count]);
		(*targ)->prog = orig->prog;
		(*targ)->next = NULL;
		(*targ)->prev = targ;
		targ = &((*targ)->next);
		orig = orig->next;
	}
}

void
localvar_freeall(struct frame *fr)
{
	struct localvars *ptr;
	struct localvars *nxt;

	ptr = fr->lvars;

	while (ptr) {
		int count = MAX_VAR;
		nxt = ptr->next;
		while (count-- > 0)
			CLEAR(&ptr->lvars[count]);
		ptr->next = NULL;
		ptr->prev = NULL;
		ptr->prog = NOTHING;
		free((void*)ptr);
		ptr = nxt;
	}
	fr->lvars = NULL;
}

void
scopedvar_addlevel(struct frame *fr, struct inst *pc, int count)
{
	struct scopedvar_t *tmp;
	int siz;
	siz = sizeof(struct scopedvar_t) + (sizeof(struct inst) * (count - 1));

	tmp = (struct scopedvar_t *) malloc(siz);
	tmp->count = count;
	tmp->varnames = pc->data.mufproc->varnames;
	tmp->next = fr->svars;
	fr->svars = tmp;
	while (count-- > 0) {
		tmp->vars[count].type = PROG_INTEGER;
		tmp->vars[count].data.number = 0;
	}
}

void
scopedvar_dupall(struct frame *fr, struct frame *oldfr)
{
	struct scopedvar_t *cur;
	struct scopedvar_t *newsv;
	struct scopedvar_t **prev;
	int siz, count;

	prev = &fr->svars;
	*prev = NULL;
	for (cur = oldfr->svars; cur; cur = cur->next) {
		count = cur->count;
		siz = sizeof(struct scopedvar_t) + (sizeof(struct inst) * (count - 1));

		newsv = (struct scopedvar_t *) malloc(siz);
		newsv->count = count;
		newsv->varnames = cur->varnames;
		newsv->next = NULL;
		while (count-- > 0) {
			copyinst(&cur->vars[count], &newsv->vars[count]);
		}
		*prev = newsv;
	}
}

int
scopedvar_poplevel(struct frame *fr)
{
	struct scopedvar_t *tmp;

	if (!fr->svars) {
		return 0;
	}

	tmp = fr->svars;
	fr->svars = fr->svars->next;
	while (tmp->count-- > 0) {
		CLEAR(&tmp->vars[tmp->count]);
	}
	free(tmp);
	return 1;
}

void
scopedvar_freeall(struct frame *fr)
{
	while (scopedvar_poplevel(fr)) ;
}


struct inst *
scopedvar_get(struct frame *fr, int level, int varnum)
{
    struct scopedvar_t *svinfo = fr->svars;
    while (svinfo && level-- > 0)
        svinfo = svinfo->next;
    if (!svinfo) {
        return NULL;
    }
    if (varnum < 0 || varnum >= svinfo->count) {
        return NULL;
    }
    return (&svinfo->vars[varnum]);
}

const char*
scopedvar_getname_byinst(struct inst *pc, int varnum)
{
    while (pc->type != PROG_FUNCTION)
        pc--;
    if (!pc->data.mufproc) {
       return NULL;
    }
    if (varnum < 0 || varnum >= pc->data.mufproc->vars) {
        return NULL;
    }
    if (!pc->data.mufproc->varnames) {
        return NULL;
    }
    return pc->data.mufproc->varnames[varnum];
}

const char*
scopedvar_getname( struct frame *fr, int level, int varnum)
{
    struct scopedvar_t *svinfo = fr->svars;
    
    while (svinfo && level-- > 0 )
        svinfo = svinfo->next;

    if (!svinfo) {
        return NULL;
    }
    if (varnum < 0 || varnum >= svinfo->count) {
        return NULL;
    }
    if (!svinfo->varnames) {
        return NULL;
    }
    return svinfo->varnames[varnum];
}

int
scopedvar_getnum(struct frame *fr, int level, const char* varname)
{
    struct scopedvar_t *svinfo = fr->svars;
    int varnum;

    while (svinfo && level-- > 0)
        svinfo = svinfo->next;
    if (!svinfo) {
        return -1;
    }
    if (!svinfo->varnames) {
        return -1;
    }
    for (varnum = 0; varnum < svinfo->count; ++varnum) {
        if (!string_compare(svinfo->varnames[varnum], varname)) {
            return varnum;
        }
    }
    return -1;
}

/* RCLEAR() is the function that is used to free memory 
 * resources used for various MUF data types. -Every- 
 * MUF string, socket, function, address, etc., should
 * be cleared through this function, as it also maintains
 * the link counters for such things that keep them from
 * being freed early. It also handles auto-closing of
 * MUF sockets if they are not closed elsewhere. 
 */
void 
RCLEAR(struct inst * oper, char *file, int line)
{
    int varcnt, j;
    if (oper == NULL)
        return;

    switch (oper->type) {
        case PROG_CLEARED: {
            time_t lt;
            char buf[40];

            lt = time(NULL);
            format_time(buf, 32, "%c", localtime(&lt));
            fprintf(stderr, "%.32s:", buf);
            fprintf(stderr, "Attempt to re-CLEAR() instruction from %s:%hd "
                "previously CLEAR()ed at %s:%d\n", file, line, 
                (char *) oper->data.addr, oper->line);
            return;
        }
        case PROG_ADD:
            DBFETCH(oper->data.addr->progref)->sp.program.instances--;
            oper->data.addr->links--;
            break;
        case PROG_STRING:
            if (oper->data.string && --oper->data.string->links == 0) 
                free((void *) oper->data.string);
            break;
        case PROG_FUNCTION:
            if (oper->data.mufproc) {
                free((void *) oper->data.mufproc->procname);
                varcnt = oper->data.mufproc->vars;
                for (j = 0; j < varcnt; ++j) {
                    free((void*)oper->data.mufproc->varnames[j]);
                }
                free((void*) oper->data.mufproc->varnames); 
                free((void *) oper->data.mufproc);
            }
            break;
        case PROG_ARRAY:
            array_free(oper->data.array);
            break;
        case PROG_LOCK:
            if (oper->data.lock != TRUE_BOOLEXP)
                free_boolexp(oper->data.lock);
            break;
        case PROG_SOCKET:
            oper->data.sock->links = oper->data.sock->links - 1;
            if (oper->data.sock && oper->data.sock->links == 0) {
                if (oper->data.sock->host) {
                    shutdown(oper->data.sock->socknum,2);
                    close(oper->data.sock->socknum);
                }
                free((void *) oper->data.sock);
            }
            break;
#ifdef SQL_SUPPORT
        case PROG_MYSQL:
            oper->data.mysql->links = oper->data.mysql->links - 1;
            if (oper->data.mysql && oper->data.mysql->links == 0) {
                if (oper->data.mysql->connected) /* close if still open */
                    mysql_close(oper->data.mysql->mysql_conn);
                free((void *) oper->data.mysql->mysql_conn);
                free((void *) oper->data.mysql);
            }
            break; 
#endif
    } 
    oper->line = line;
    oper->data.addr = (struct prog_addr *) file;
    oper->type = PROG_CLEARED;
}

void    push(struct inst * stack, int *top, int type, voidptr res);

int     valid_object(struct inst * oper);

int     top_pid = 1;
int     nargs = 0;

static struct frame *free_frames_list = NULL;

struct forvars *for_pool = NULL;
struct forvars **last_for = &for_pool;
struct tryvars *try_pool = NULL;
struct tryvars **last_try = &try_pool;

void
purge_free_frames(void)
{
    struct frame *ptr, *ptr2;
    int count = tp_free_frames_pool;

    for(ptr = free_frames_list; ptr && --count>0; ptr = ptr->next);
    while (ptr && ptr->next) {
	ptr2 = ptr->next;
	ptr->next = ptr->next->next;
	free(ptr2);
    }
}

void
purge_all_free_frames(void)
{
    struct frame *ptr;

    while (free_frames_list) {
	ptr = free_frames_list;
	free_frames_list = ptr->next;
	free(ptr);
    }
}

void
purge_for_pool(void)
{
	struct forvars *cur, *next;

	cur = *last_for;
	*last_for = NULL;
	last_for = &for_pool;

	while (cur) {
		next = cur->next;
		free(cur);
		cur = next;
	}
}


void
purge_try_pool(void)
{
	struct tryvars *cur, *next;

	cur = *last_try;
	*last_try = NULL;
	last_try = &try_pool;

	while (cur) {
		next = cur->next;
		free(cur);
		cur = next;
	}
}



struct frame*
interp(int descr, dbref player, dbref location, dbref program,
       dbref source, int nosleeps, int whichperms)
{
    struct frame *fr;
    int     i;
    PropPtr tptr;

    if (!MLevel(program) || !MLevel(OWNER(program)) ||
	    ((source != NOTHING) && !TMage(OWNER(source)) &&
	     !can_link_to(OWNER(source), TYPE_EXIT, program))) {
	anotify_nolisten(player,  CFAIL "Program call: Permission denied.", 1);
	return 0;
    }
    if (free_frames_list) {
	fr = free_frames_list;
	free_frames_list = fr->next;
    } else {
	fr = (struct frame *) malloc(sizeof(struct frame));
    }
    fr->next = NULL;
    fr->pid = top_pid++;
    fr->descr = descr;
    fr->multitask = nosleeps;
    fr->perms = whichperms;
    fr->already_created = 0;
    fr->been_background = (nosleeps == 2);
    fr->trig = source;
    fr->events = NULL;
    fr->timercount = 0;
    fr->started = current_systime;
    fr->prog = program;
    fr->player = player;
    fr->instcnt = 0;
    fr->skip_declare = 0;
    fr->wantsblanks = 0;
    fr->caller.top = 1;
    fr->caller.st[0] = source;
    fr->caller.st[1] = program;


    fr->system.top = 1;
    fr->system.st[0].progref = 0;
    fr->system.st[0].offset = 0;

    fr->fors.top = 0;
    fr->fors.st = NULL;
    fr->trys.top = 0;
    fr->trys.st = NULL;
    fr->errorstr = NULL;

    fr->waitees = NULL;
    fr->waiters = NULL;

    fr->rndbuf = NULL;
    fr->dlogids = NULL;

    fr->argument.top = 0;
    fr->pc = DBFETCH(program)->sp.program.start;//TYPEROOM taken out for command props.
    fr->writeonly = ((source == -1) || /* (Typeof(source) == TYPE_ROOM)  || */
		     ((Typeof(source) == TYPE_PLAYER) && (!online(source))) ||
		     ((FLAGS(player) & READMODE) && player != NOTHING));
    fr->level = 0;
    fr->error.is_flags = 0;
   
    /* set inst count limit for preempt programs. Can be used to limit
     * w-bitted preempt programs from going forever. 
     */ 
    fr->preemptlimit = 0;
    tptr = get_property(program, "_/instlimit");
    if (tptr) {
#ifdef DISKBASE
        propfetch(program, tptr);
#endif
        if (PropType(tptr) == PROP_STRTYP) 
            fr->preemptlimit = atoi(get_uncompress(PropDataStr(tptr)));
    }
    /* set basic local variables */

    fr->svars = NULL;
    fr->lvars = NULL;
    for (i = 0; i < MAX_VAR; i++) {
	fr->variables[i].type = PROG_INTEGER;
	fr->variables[i].data.number = 0;
    }

    fr->brkpt.debugging  = 0;
    fr->brkpt.bypass     = 0;
    fr->brkpt.isread     = 0;
    fr->brkpt.showstack  = 0;
    fr->brkpt.dosyspop   = 0;
    fr->brkpt.lastline   = 0;
    fr->brkpt.lastpc     = 0;
    fr->brkpt.lastlisted = 0;
    fr->brkpt.lastcmd    = NULL;
    fr->brkpt.breaknum   = -1;

    fr->brkpt.lastproglisted = NOTHING;
    fr->brkpt.proglines = NULL;

    fr->brkpt.count     = 1;
    fr->brkpt.temp[0]      = 1;
    fr->brkpt.level[0]     = -1;
    fr->brkpt.line[0]      = -1;
    fr->brkpt.linecount[0] = -2;
    fr->brkpt.pc[0]        = NULL;
    fr->brkpt.pccount[0]   = -2;
    fr->brkpt.prog[0]      = program;

    fr->proftime.tv_sec = 0;
    fr->proftime.tv_usec = 0;
    fr->totaltime.tv_sec = 0;
    fr->totaltime.tv_usec = 0;

    fr->variables[0].type = PROG_OBJECT;
    fr->variables[0].data.objref = player;
    fr->variables[1].type = PROG_OBJECT;
    fr->variables[1].data.objref = location;
    fr->variables[2].type = PROG_OBJECT;
    fr->variables[2].data.objref = source;
    fr->variables[3].type = PROG_STRING;
    fr->variables[3].data.string = (!*match_cmdname) ? 0 : alloc_prog_string(match_cmdname);

    if (DBFETCH(program)->sp.program.code) {
       DBFETCH(program)->sp.program.profuses++;
    }
    DBFETCH(program)->sp.program.instances++;
    push(fr->argument.st, &(fr->argument.top), PROG_STRING, match_args ?
	 MIPSCAST alloc_prog_string(match_args) : 0);
    return fr;
}

static int err;
int     already_created;

struct forvars * 
copy_fors(struct forvars *forstack) 
{ 
    struct forvars *in; 
    struct forvars *out = NULL; 
    struct forvars *nu; 
    struct forvars *last = NULL; 
    
    for (in = forstack; in; in = in->next) { 
        if (!for_pool) { 
            nu = malloc(sizeof(struct forvars)); 
        } else { 
            nu = for_pool; 
            if (*last_for == for_pool->next) { 
                last_for = &for_pool; 
            } 
            for_pool = nu->next; 
        } 
    
        nu->didfirst = in->didfirst; 
        copyinst(&in->cur, &nu->cur); 
        copyinst(&in->end, &nu->end); 
        nu->step = in->step; 
        nu->next = NULL; 
    
        if (!out) { 
            last = out = nu; 
        } else { 
            last->next = nu; 
            last = nu; 
        } 
   } 
   return out; 
}   


struct forvars *
push_for(struct forvars *forstack)
{
	struct forvars *nw;

	if (!for_pool) {
		nw = (struct forvars *) malloc(sizeof(struct forvars));
	} else {
		nw = for_pool;
		if (*last_for == for_pool->next) {
			last_for = &for_pool;
		}
		for_pool = nw->next;
	}
	nw->next = forstack;
	return nw;
}

struct forvars *
pop_for(struct forvars *forstack)
{
	struct forvars *newstack;

	newstack = forstack->next;
	forstack->next = for_pool;
	for_pool = forstack;
	if (last_for == &for_pool) {
		last_for = &(for_pool->next);
	}
	return newstack;
}

struct tryvars * 
copy_trys(struct tryvars *trystack) 
{ 
    struct tryvars *in; 
    struct tryvars *out = NULL; 
    struct tryvars *nu; 
    struct tryvars *last = NULL; 
    
    for (in = trystack; in; in = in->next) { 
        if (!try_pool) { 
            nu = malloc(sizeof(struct tryvars)); 
        } else { 
            nu = try_pool; 
            if (*last_try == try_pool->next) { 
                last_try = &try_pool; 
            } 
            try_pool = nu->next; 
        } 
    
        nu->depth      = in->depth; 
        nu->call_level = in->call_level; 
        nu->for_count  = in->for_count; 
        nu->addr       = in->addr; 
        nu->next = NULL; 
    
        if (!out) { 
            last = out = nu; 
        } else { 
            last->next = nu; 
            last = nu; 
        } 
    } 
    return out; 
} 



struct tryvars *
push_try(struct tryvars *trystack)
{
	struct tryvars *nu;

	if (!try_pool) {
                nu = (struct tryvars *) malloc(sizeof(struct tryvars));
	} else {
		nu = try_pool;
		if (*last_try == try_pool->next) {
			last_try = &try_pool;
		}
		try_pool = nu->next;
	}
	nu->next = trystack;
	return nu;
}

struct tryvars *
pop_try(struct tryvars *trystack)
{
	struct tryvars *newstack;

	newstack = trystack->next;
	trystack->next = try_pool;
	try_pool = trystack;
	if (last_try == &try_pool) {
		last_try = &(try_pool->next);
	}
	return newstack;
}


/* clean up lists from watchpid and sends event */
void
watchpid_process(struct frame *fr)
{
	struct frame *frame;
	struct mufwatchpidlist *cur;
	struct mufwatchpidlist **curptr;
	struct inst temp1;
	temp1.type = PROG_INTEGER;
	temp1.data.number = fr->pid;

	while (fr->waitees) {
		cur = fr->waitees;
		fr->waitees = cur->next;

		frame = timequeue_pid_frame (cur->pid);
		free (cur);
		if (frame) {
			for (curptr = &frame->waiters; *curptr; curptr = &(*curptr)->next) {
				if ((*curptr)->pid == fr->pid) {
					cur = *curptr;
					*curptr = (*curptr)->next;
					free (cur);
					break;
				}
			}
		}
	}

	while (fr->waiters) {
		char buf[64];

		sprintf (buf, "PROC.EXIT.%d", fr->pid);

		cur = fr->waiters;
		fr->waiters = cur->next;

		frame = timequeue_pid_frame (cur->pid);
		free (cur);
		if (frame) {
			muf_event_add(frame, buf, &temp1, 0);
			for (curptr = &frame->waitees; *curptr; curptr = &(*curptr)->next) {
				if ((*curptr)->pid == fr->pid) {
					cur = *curptr;
					*curptr = (*curptr)->next;
					free (cur);
					break;
				}
			}
		}
	}
}


/* clean up the stack. */
void 
prog_clean(struct frame * fr)
{
    int     i;
    struct frame *ptr;
    time_t  now;
    now = current_systime;

    if ( (FLAG2(fr->player)&F2MUFCOUNT) && (controls(fr->player, fr->prog)
	|| (Mage(fr->player) && (OWNER(fr->prog) != MAN)))
    ) {
	notify_fmt(fr->player, "MUF> %d %s %ds", fr->instcnt,
		unparse_object(fr->player, fr->prog), (now - fr->started)
	);
    }

    if (FLAG2(fr->prog)&F2MUFCOUNT) {
	add_property(fr->prog, "~muf/count", NULL, fr->instcnt);
	add_property(fr->prog, "~muf/start", NULL, fr->started);
	add_property(fr->prog, "~muf/end", NULL, now);
	add_property(fr->prog, "~muf/who", NULL, fr->player);
	add_property(fr->prog, "~muf/trig", NULL, fr->trig);
    }

    for(ptr = free_frames_list; ptr; ptr = ptr->next) {
	if (ptr == fr) {
            time_t lt;
            char buf[40];
            
            lt = time(NULL);
            format_time(buf, 32, "%c", localtime(&lt));
            fprintf(stderr, "%.32s:", buf);
	    fprintf(stderr, "prog_clean(): Tried to free an already freed program frame!\n");
	    abort(); /* abort, since this is a critical error */
	}
    }

    watchpid_process(fr);

    fr->system.top = 0;
    for (i = 0; i < fr->argument.top; i++)
	CLEAR(&fr->argument.st[i]);

    for (i = 1; i <= fr->caller.top; i++)
	DBFETCH(fr->caller.st[i])->sp.program.instances--;

    for (i = 0; i < MAX_VAR; i++)
	CLEAR(&fr->variables[i]);

	localvar_freeall(fr);
	scopedvar_freeall(fr);

    if (fr->fors.st) {
	struct forvars **loop = &(fr->fors.st);

	while (*loop) {
            CLEAR(&((*loop)->cur));
            CLEAR(&((*loop)->end));
	    loop = &((*loop)->next);
	}
	*loop = for_pool;
	if (last_for == &for_pool) {
	  last_for = loop;
	}
	for_pool = fr->fors.st;
	fr->fors.st = NULL;
	fr->fors.top = 0;
    }

    if (fr->trys.st) {
        struct tryvars **loop = &(fr->trys.st);

        while (*loop) {
            loop = &((*loop)->next);
        }
        *loop = try_pool;
        if (last_try == &try_pool) {
            last_try = loop;
        }
        try_pool = fr->trys.st;
        fr->trys.st = NULL;
        fr->trys.top = 0;
    }

    fr->argument.top = 0;
    fr->pc = 0;
    if (fr->brkpt.lastcmd)
	free(fr->brkpt.lastcmd);
    if (fr->brkpt.proglines) {
	free_prog_text(fr->brkpt.proglines);
	fr->brkpt.proglines = NULL;
    }

    if (fr->rndbuf)
      delete_seed(fr->rndbuf);

    muf_dlog_purge(fr);

    dequeue_timers(fr->pid, NULL);

    muf_event_purge(fr);
    fr->next = free_frames_list;
    free_frames_list = fr;
    err = 0;
}


void 
reload(struct frame * fr, int atop, int stop)
{
    fr->argument.top = atop;
    fr->system.top = stop;
}


int 
logical_false(struct inst * p)
{
    return ((p->type == PROG_STRING && (p->data.string == 0 || !(*p->data.string->data)))
	    || (p->type == PROG_MARK)
  	    || (p->type == PROG_ARRAY && (!p->data.array || !p->data.array->items))
	    || (p->type == PROG_LOCK && p->data.lock == TRUE_BOOLEXP)
	    || (p->type == PROG_INTEGER && p->data.number == 0)
          || (p->type == PROG_FLOAT &&
             (p->data.fnumber < SMALL_NUM && p->data.fnumber > NSMALL_NUM))
	    || (p->type == PROG_OBJECT && p->data.objref == NOTHING));
}


void 
copyinst(struct inst * from, struct inst * to)
{
    int j, varcnt;
	*to = *from;
	switch(from->type) {
	case PROG_FUNCTION:
	    if (from->data.mufproc) {
			to->data.mufproc = (struct muf_proc_data*)malloc(
			            sizeof(struct muf_proc_data));
			to->data.mufproc->procname = string_dup(
                              from->data.mufproc->procname);
			to->data.mufproc->vars = varcnt 
                                               = from->data.mufproc->vars;
			to->data.mufproc->args = from->data.mufproc->args;
                        to->data.mufproc->varnames = (const char**)calloc(
                            varcnt, sizeof(const char*));
                        for (j = 0; j < varcnt; ++j )
                            to->data.mufproc->varnames[j] = 
                                string_dup(from->data.mufproc->varnames[j]);
		}
		break;
	case PROG_STRING:
	    if (from->data.string) {
			from->data.string->links++;
		}
		break;
	case PROG_ARRAY:
	    if (from->data.array) {
			from->data.array->links++;
		}
		break;
	case PROG_ADD:
		from->data.addr->links++;
		DBFETCH(from->data.addr->progref)->sp.program.instances++;
		break;
	case PROG_LOCK:
	    if (from->data.lock != TRUE_BOOLEXP) {
			to->data.lock = copy_bool(from->data.lock);
		}
		break;
      case PROG_SOCKET:
	    if (from->data.sock) {
			from->data.sock->links++;
	    }
            break;
#ifdef SQL_SUPPORT
      case PROG_MYSQL:
            if (from->data.mysql) {
                from->data.mysql->links++;
            }
            break;
#endif
      }
}


void
copyvars(vars *from, vars *to)
{
    int i;

    for (i = 0; i < MAX_VAR; i++) {
        copyinst(&(*from)[i], &(*to)[i]);
    }
}



void
calc_profile_timing(dbref prog, struct frame *fr)
{
	struct timeval tv;
	struct timeval tv2;

	gettimeofday(&tv, NULL);
	if (tv.tv_usec < fr->proftime.tv_usec) {
		tv.tv_usec += 1000000;
		tv.tv_sec -= 1;
	}
	tv.tv_usec -= fr->proftime.tv_usec;
	tv.tv_sec -= fr->proftime.tv_sec;
	tv2 = DBFETCH(prog)->sp.program.proftime;
	tv2.tv_sec += tv.tv_sec;
	tv2.tv_usec += tv.tv_usec;
	if (tv2.tv_usec >= 1000000) {
		tv2.tv_usec -= 1000000;
		tv2.tv_sec += 1;
	}
      DBFETCH(prog)->sp.program.proftime.tv_sec = tv2.tv_sec;
      DBFETCH(prog)->sp.program.proftime.tv_usec = tv2.tv_usec;
	fr->totaltime.tv_sec += tv.tv_sec;
	fr->totaltime.tv_usec += tv.tv_usec;
	if (fr->totaltime.tv_usec > 1000000) {
		fr->totaltime.tv_usec -= 1000000;
		fr->totaltime.tv_sec += 1;
	}
}


static int interp_depth = 0;

void 
do_abort_loop(dbref player, dbref program, const char *msg,
	      struct frame * fr, struct inst * pc, int atop, int stop,
	      struct inst * clinst1, struct inst * clinst2)
{
    char buffer[128];
    struct descriptor_data *curdescr;
      
	if (fr->trys.top) {
		fr->errorstr = string_dup(msg);
		err++;
	} else {
		if (pc) {
			calc_profile_timing(program, fr);
		}
	}
    if (clinst1)
	CLEAR(clinst1);
    if (clinst2)
	CLEAR(clinst2);
    reload(fr, atop, stop);
    fr->pc = pc;
    if (!fr->trys.top) {
	    if (pc) {
		interp_err(OWNER(player), program, pc, fr->argument.st, fr->argument.top,
			   fr->caller.st[1], insttotext(fr, 0, pc, buffer, sizeof(buffer), 30, program), msg);
		if (controls(OWNER(player), program) || (FLAG2(OWNER(player)) & F2PARENT))
		    muf_backtrace(player, program, STACK_SIZE, fr);
	      else
	         if (FLAG2(program) & F2PARENT && player != OWNER(program))
	            muf_backtrace(OWNER(program), program, STACK_SIZE, fr);
	    } else {
		notify_nolisten(player, msg, 1);
	    }
	    fr->level--;
	    prog_clean(fr);
	    DBSTORE(player, sp.player.block, 0);
            if (player == NOTHING ) {
                curdescr = get_descr(fr->descr, NOTHING);
                if ( curdescr ) { 
                    curdescr->block = 0;
                    curdescr->interactive = 0;
                    DR_RAW_REM_FLAGS(curdescr, DF_INTERACTIVE);
                }
            } 
    }
}

extern struct line *read_program(dbref prog);
extern char *show_line_prims(struct frame *fr, dbref program, struct inst *pc, int maxprims, int markpc);

void
interp_set_depth(struct frame * fr)
{
   interp_depth = fr->level;
}

struct inst * 
interp_loop(dbref player, dbref program, struct frame * fr, int rettyp)
{
    register struct inst *pc;
    register int atop;
    register struct inst *arg;
    register struct inst *temp1;
    register struct inst *temp2;
    register struct stack_addr *sys;
    register int instr_count;
    register int stop;
    int     i, tmp, writeonly, mlev;
    static struct inst retval;
    char dbuf[BUFFER_LEN];
    struct descriptor_data *curdescr = NULL;

    if(interp_depth) {
       fr->level = interp_depth++;
       interp_depth = 0;
    } else {
       fr->level++;
    }

    /* load everything into local stuff */
    pc = fr->pc;
    atop = fr->argument.top;
    stop = fr->system.top;
    arg = fr->argument.st;
    sys = fr->system.st;
    writeonly = fr->writeonly;
    already_created = 0;
    fr->brkpt.isread = 0;

    if (!pc) {
	struct line *tmpline;

	tmpline = DBFETCH(program)->sp.program.first;
	DBFETCH(program)->sp.program.first = read_program(program);
	do_compile(-1, OWNER(program), program, 0);
	free_prog_text(DBFETCH(program)->sp.program.first);
	DBSTORE(program, sp.program.first, tmpline);
	pc = fr->pc = DBFETCH(program)->sp.program.start;
	if (!pc) {
	    abort_loop_hard("Program not compilable. Cannot run.", NULL, NULL);
	}
	DBFETCH(program)->sp.program.profuses++;
        DBFETCH(program)->sp.program.instances++;
    }
    ts_useobject(program);
    err = 0;

    instr_count = 0;
    mlev = ProgMLevel(program);
    gettimeofday(&fr->proftime, NULL);

    /* This is the 'natural' way to exit a function */
    while (stop) {
	fr->instcnt++;
	instr_count++;
        if (fr->preemptlimit)
            if (fr->instcnt > fr->preemptlimit)
                    abort_loop("Program specific instruction limit exceeded.", NULL, NULL); 
	if ((fr->multitask == PREEMPT) || (FLAGS(program) & BUILDER)) {
	    if (mlev >= LMAGE) {
		instr_count = 0;/* if program is wizbit, then clear count */
	    } else {
		/* else make sure that the program doesn't run too long */
		if (instr_count >= tp_max_instr_count)
		    abort_loop("Maximum preempt instruction count exceeded", NULL, NULL);
	    }
	} else {
	    /* if in FOREGROUND or BACKGROUND mode, '0 sleep' every so often. */
	    if ((fr->instcnt > tp_instr_slice * 4) && (instr_count >= tp_instr_slice)) {
		fr->pc = pc;
		reload(fr, atop, stop);
		DBSTORE(player, sp.player.block, (!fr->been_background));
                if (player == NOTHING ) {
                    curdescr = get_descr(fr->descr, NOTHING);
                    if (curdescr) 
                        curdescr->block = !(fr->been_background);
                }
		add_muf_delay_event(0, fr->descr, player, NOTHING, NOTHING, program, fr,
		    (fr->multitask==FOREGROUND) ? "FOREGROUND" : "BACKGROUND");
		fr->level--;
		calc_profile_timing(program,fr);
		return NULL;
	    }
	}
	if ((FLAGS(program) & ZOMBIE) && !fr->been_background && controls(player, program)) {
	    fr->brkpt.debugging = 1;
	} else {
	    fr->brkpt.debugging = 0;
	}
	if ( (	(FLAGS(program) & DARK) ||
         ( fr->brkpt.debugging && fr->brkpt.showstack && !fr->brkpt.bypass) ) &&
         (controls(OWNER(player), program) || (FLAG2(OWNER(player)) & F2PARENT))
	) {
		/* Small fix so only program owner can see debug traces */
	    char   *m = debug_inst(fr, 0, pc, fr->pid, arg, dbuf, sizeof(dbuf), 
                                   atop, program);
	    notify_nolisten(player, m, 1);
	}
      if ( FLAGS(program) & DARK && FLAG2(program) & F2PARENT && (OWNER(program) != player || player == -1 )) {
	    char   *m = debug_inst(fr, 0, pc, fr->pid, arg, dbuf, sizeof(dbuf),
                                   atop, program);
	    notify_nolisten(OWNER(program), m, 1);
      }
	if (fr->brkpt.debugging) {
	    if (fr->brkpt.count) {
		for (i = 0; i < fr->brkpt.count; i++) {
		    if (
			    (!fr->brkpt.pc[i] || pc==fr->brkpt.pc[i]) &&
			      /* pc matches */
			    (fr->brkpt.line[i] == -1 ||
			     (fr->brkpt.lastline != pc->line &&
			      fr->brkpt.line[i] == pc->line)) &&
			      /* line matches */
			    (fr->brkpt.level[i] == -1 ||
			     stop <= fr->brkpt.level[i]) &&
			      /* level matches */
			    (fr->brkpt.prog[i] == NOTHING ||
			     fr->brkpt.prog[i] == program) &&
			      /* program matches */
			    (fr->brkpt.linecount[i] == -2 ||
			     (fr->brkpt.lastline != pc->line &&
			      fr->brkpt.linecount[i]-- <= 0)) &&
			      /* line count matches */
			    (fr->brkpt.pccount[i] == -2 ||
			     (fr->brkpt.lastpc != pc &&
			      fr->brkpt.pccount[i]-- <= 0))
			      /* pc count matches */
			    ) {
			if (fr->brkpt.bypass) {
			    if (fr->brkpt.pccount[i] == -1)
				fr->brkpt.pccount[i] = 0;
			    if (fr->brkpt.linecount[i] == -1)
				fr->brkpt.linecount[i] = 0;
			} else {
			    char *m;
			    char buf[BUFFER_LEN];
                            if (fr->brkpt.dosyspop) {
                                program = sys[--stop].progref;
                                pc = sys[stop].offset;
                            }
			    add_muf_read_event(fr->descr, player, program, fr);
			    reload(fr, atop, stop);
			    fr->pc = pc;
			    fr->brkpt.isread = 0;
			    fr->brkpt.breaknum = i;
			    fr->brkpt.lastlisted = 0;
			    fr->brkpt.bypass = 0;
                            fr->brkpt.dosyspop = 0;
			    DBSTORE(player, sp.player.curr_prog, program);
			    DBSTORE(player, sp.player.block, 0);
                            if ( player == NOTHING ) {
                                curdescr = get_descr(fr->descr, NOTHING);
                                if (curdescr) { 
                                    curdescr->block = 0;
                                    DR_RAW_REM_FLAGS(curdescr, DF_INTERACTIVE);
                                }
                            }
			    fr->level--;
			    if (!fr->brkpt.showstack) {
				m = debug_inst(fr, 0, pc, fr->pid, arg, dbuf, 
                                               sizeof(dbuf), atop, program);
				notify_nolisten(player, m, 1);
			    }
			    if (pc <= DBFETCH(program)->sp.program.code ||
				    (pc-1)->line != pc->line) {
				list_proglines(player,program,fr,pc->line,0);
			    } else {
				m = show_line_prims(fr, program,pc,15,1);
				sprintf(buf, "     %s", m);
				notify_nolisten(player, buf, 1);
			    }
			    calc_profile_timing(program,fr);
			    return NULL;
			}
		    }
		}
	    }
	    fr->brkpt.lastline = pc->line;
	    fr->brkpt.lastpc = pc;
	    fr->brkpt.bypass = 0;
	}
	if (mlev < LMAGE) {
	    if (fr->instcnt > (tp_max_instr_count *
	      ((mlev == LM3)? 16 : ((mlev == LM2) ? 4 : 1))))
		abort_loop("Maximum total instruction count exceeded.", NULL, NULL);
	}
	switch (pc->type) {
	    case PROG_INTEGER:
          case PROG_FLOAT:
	    case PROG_ADD:
	    case PROG_OBJECT:
	    case PROG_VAR:
          case PROG_SVAR:
	    case PROG_LVAR:
	    case PROG_STRING:
	    case PROG_LOCK:
          case PROG_MARK:
	 case PROG_ARRAY:
		if (atop >= STACK_SIZE)
		    abort_loop("Stack overflow.", NULL, NULL);
		copyinst(pc, arg + atop);
		pc++;
		atop++;
		break;
            case PROG_LVAR_AT:
            case PROG_LVAR_AT_CLEAR: {
                struct inst *tmp;
                struct localvars *lv;
                    
                if (atop >= STACK_SIZE)
                    abort_loop("Stack overflow.", NULL, NULL);
                if (pc->data.number >= MAX_VAR || pc->data.number < 0)
                    abort_loop("Local variable number out of range.",
                                NULL, NULL);
                lv = localvars_get(fr, program);
                tmp = &(lv->lvars[pc->data.number]);
                copyinst(tmp, arg + atop);
                if (pc->type == PROG_LVAR_AT_CLEAR) {
                    CLEAR(tmp);
                    tmp->type = PROG_INTEGER;
                    tmp->data.number = 0;
                }
                ++pc;
                ++atop;
            }
                break;
            case PROG_LVAR_BANG: {
                struct inst *the_var;
                struct localvars *lv;
            
                if (atop < 1)
                    abort_loop("Stack Underflow.", NULL, NULL);
                if (fr->trys.top && atop - fr->trys.st->depth < 1)
                    abort_loop("Stack protection fault.", NULL, NULL);
                if (pc->data.number >= MAX_VAR || pc->data.number < 0)
                    abort_loop("Local variable out of range.", NULL, NULL);

                lv = localvars_get(fr, program);
                the_var = &(lv->lvars[pc->data.number]);

                CLEAR(the_var);
                temp1 = arg + --atop;
                *the_var = *temp1;
                ++pc;
            }
                break;
            case PROG_SVAR_AT:
            case PROG_SVAR_AT_CLEAR:
			{
				struct inst *tmp;
				if (atop >= STACK_SIZE)
					abort_loop("Stack overflow.", NULL, NULL);

				tmp = scopedvar_get(fr, 0,  pc->data.number);
				if (!tmp)
					abort_loop("Scoped variable number out of range.", NULL, NULL);

				copyinst(tmp, arg + atop);
                                if (pc->type == PROG_SVAR_AT_CLEAR) {
                                    CLEAR(tmp);
                                    tmp->type = PROG_INTEGER;
                                    tmp->data.number = 0;
                                }

				pc++;
				atop++;
			}
			break;

		case PROG_SVAR_BANG:
			{
				struct inst *the_var;
				if (atop < 1)
					abort_loop("Stack Underflow.", NULL, NULL);
                                if (fr->trys.top && atop - fr->trys.st->depth < 1)
                                        abort_loop("Stack protection fault.", NULL, NULL);

				the_var = scopedvar_get(fr, 0, pc->data.number);
				if (!the_var)
					abort_loop("Scoped variable number out of range.", NULL, NULL);

				CLEAR(the_var);
                                temp1 = arg + --atop;
                                *the_var = *temp1;
				pc++;
			}
			break;

	    case PROG_FUNCTION:
			{
				int i = pc->data.mufproc->args;
				if (atop < i)
					abort_loop("Stack Underflow.", NULL, NULL);
                                if (fr->trys.top && atop - fr->trys.st->depth < i) 
                                        abort_loop("Stack protection fault.", NULL, NULL);
				if (fr->skip_declare)
					fr->skip_declare = 0;
				else
					scopedvar_addlevel(fr, pc, pc->data.mufproc->vars);
				while (i-->0)
				{
					struct inst *tmp;
					temp1 = arg + --atop;
					tmp = scopedvar_get(fr, 0,  i);
					if (!tmp)
						abort_loop_hard("Internal error: Scoped variable number out of range in FUNCTION init.", temp1, NULL);
					CLEAR(tmp);
					copyinst(temp1, tmp);
					CLEAR(temp1);
				}
				pc++;
			}
			break;

	    case PROG_IF:
		if (atop < 1)
		    abort_loop("Stack Underflow.", NULL, NULL);
                if (fr->trys.top && atop - fr->trys.st->depth < 1) 
                    abort_loop("Stack protection fault.", NULL, NULL);		temp1 = arg + --atop;
		if (logical_false(temp1))
		    pc = pc->data.call;
		else
		    pc++;
		CLEAR(temp1);
		break;

	    case PROG_EXEC:
		if (stop >= STACK_SIZE)
		    abort_loop("System Stack Overflow", NULL, NULL);
		sys[stop].progref = program;
		sys[stop++].offset = pc + 1;
		pc = pc->data.call;
		fr->skip_declare = 0;  /* Make sure we DON'T skip var decls */
		break;

	    case PROG_JMP:
		/* Don't need to worry about skipping scoped var decls here. */
		/* JMP to a function header can only happen in IN_JMP */
		pc = pc->data.call;
		break;

		case PROG_TRY:
			if (atop < 1)
				abort_loop("Stack Underflow.", NULL, NULL);
                        if (fr->trys.top && atop - fr->trys.st->depth < 1) 
                                abort_loop("Stack protection fault.", NULL, NULL);
			temp1 = arg + --atop;
			if (temp1->type != PROG_INTEGER)
				abort_loop("Argument is not an integer.", temp1, NULL);
			if (temp1->data.number > atop)
				abort_loop("Attempted to lock more stack items than exist.", temp1, NULL);

			fr->trys.top++;
			fr->trys.st = push_try(fr->trys.st);
			fr->trys.st->depth = atop - temp1->data.number;
			fr->trys.st->call_level = stop;
			fr->trys.st->for_count = 0;
			fr->trys.st->addr = pc->data.call;

			pc++;
			CLEAR(temp1);
			break;

	    case PROG_PRIMITIVE:
		/*
		 * All pc modifiers and stuff like that should stay here,
		 * everything else call with an independent dispatcher.
		 */
		switch (pc->data.number) {
		    case IN_JMP:
			if (atop < 1)
			    abort_loop("Stack underflow.  Missing address.", NULL, NULL);
                        if (fr->trys.top && atop - fr->trys.st->depth < 1) 
                            abort_loop("Stack protection fault.", NULL, NULL);
			temp1 = arg + --atop;
			if (temp1->type != PROG_ADD)
			    abort_loop("Argument is not an address.", temp1, NULL);
			if (temp1->data.addr->progref > db_top ||
				temp1->data.addr->progref < 0 ||
				(Typeof(temp1->data.addr->progref) != TYPE_PROGRAM))
			    abort_loop_hard("Internal error.  Invalid address.", temp1, NULL);
			if (program != temp1->data.addr->progref) {
			    abort_loop("Destination outside current program.", temp1, NULL);
			}
			if (temp1->data.addr->data->type == PROG_FUNCTION) {
				fr->skip_declare = 1;
			}
			pc = temp1->data.addr->data;
			CLEAR(temp1);
			break;

		    case IN_EXECUTE:
			if (atop < 1)
			    abort_loop("Stack Underflow. Missing address.", NULL, NULL);
                        if (fr->trys.top && atop - fr->trys.st->depth < 1) 
                            abort_loop("Stack protection fault.", NULL, NULL);
			temp1 = arg + --atop;
			if (temp1->type != PROG_ADD)
			    abort_loop("Argument is not an address.", temp1, NULL);
			if (temp1->data.addr->progref > db_top ||
				temp1->data.addr->progref < 0 ||
				(Typeof(temp1->data.addr->progref) != TYPE_PROGRAM))
			    abort_loop_hard("Internal error.  Invalid address.", temp1, NULL);
			if (stop >= STACK_SIZE)
			    abort_loop("System Stack Overflow", temp1, NULL);
			sys[stop].progref = program;
			sys[stop++].offset = pc + 1;
			if (program != temp1->data.addr->progref) {
			    program = temp1->data.addr->progref;
			    fr->caller.st[++fr->caller.top] = program;
			    mlev = ProgMLevel(program);
			    DBFETCH(program)->sp.program.instances++;
			}
			pc = temp1->data.addr->data;
			CLEAR(temp1);
			break;

		    case IN_CALL:
			if (atop < 1)
			    abort_loop("Stack Underflow. Missing dbref argument.", NULL, NULL);
                        if (fr->trys.top && atop - fr->trys.st->depth < 1) 
                            abort_loop("Stack protection fault.", NULL, NULL);
			temp1 = arg + --atop;
			temp2 = 0;
			if (temp1->type != PROG_OBJECT) {
			    temp2 = temp1;
			    if (atop < 1)
				abort_loop("Stack Underflow. Missing dbref of func.", temp1, NULL);
                            if (fr->trys.top && atop - fr->trys.st->depth < 1) 
                                abort_loop("Stack protection fault.", NULL, NULL);
			    temp1 = arg + --atop;
			    if (temp2->type != PROG_STRING)
				abort_loop("Public Func. name string required. (2)", temp1, temp2);
			    if (!temp2->data.string)
				abort_loop("Null string not allowed. (2)", temp1, temp2);
			}
			if (temp1->type != PROG_OBJECT)
			    abort_loop("Dbref required. (1)", temp1, temp2);
			if (!valid_object(temp1)
				|| Typeof(temp1->data.objref) != TYPE_PROGRAM)
			    abort_loop("invalid object.", temp1, temp2);
			if (!(DBFETCH(temp1->data.objref)->sp.program.code)) {
			    struct line *tmpline;

			    tmpline = DBFETCH(temp1->data.objref)->sp.program.first;
			    DBFETCH(temp1->data.objref)->sp.program.first =
				read_program(temp1->data.objref);
			    do_compile(-1, OWNER(temp1->data.objref), temp1->data.objref, 0);
			    free_prog_text(DBFETCH(temp1->data.objref)->sp.program.first);
			    DBSTORE(temp1->data.objref, sp.program.first, tmpline);
			    if (!(DBFETCH(temp1->data.objref)->sp.program.code))
				abort_loop("Program not compilable.", temp1, temp2);
			}
			if (ProgMLevel(temp1->data.objref) == 0)
			    abort_loop(tp_noperm_mesg, temp1, temp2);
			if (mlev < LMAGE && OWNER(temp1->data.objref) != ProgUID
				&& !Linkable(temp1->data.objref))
			    abort_loop(tp_noperm_mesg, temp1, temp2);
			if (stop >= STACK_SIZE)
			    abort_loop("System Stack Overflow", temp1, temp2);
			sys[stop].progref = program;
			sys[stop++].offset = pc + 1;
			if (!temp2) {
			    pc = DBFETCH(temp1->data.objref)->sp.program.start;
			} else {
			    struct publics *pbs;
			    int     tmpint;

			    pbs = DBFETCH(temp1->data.objref)->sp.program.pubs;
			    while (pbs) {
				tmpint = string_compare(temp2->data.string->data, pbs->subname);
				if (!tmpint)
				    break;
				pbs = pbs->next;
			    }
			    if (!pbs)
				abort_loop("PUBLIC or WIZCALL-type Function not found. (2)", temp2, temp2);
			    if (mlev < pbs->mlev)
				abort_loop("Insufficient permissions to call WIZCALL-type function. (2)", temp2, temp2);
			    pc = pbs->addr.ptr;
			}
			if (temp1->data.objref != program) {
			    calc_profile_timing(program,fr);
			    gettimeofday(&fr->proftime, NULL);
			    program = temp1->data.objref;
			    fr->caller.st[++fr->caller.top] = program;
			    DBFETCH(program)->sp.program.instances++;
			    mlev = ProgMLevel(program);
			}
			DBFETCH(program)->sp.program.profuses++;
			ts_useobject(program);
			CLEAR(temp1);
			if (temp2)
			    CLEAR(temp2);
			break;

		    case IN_RET:
			if (stop > 1 && program != sys[stop - 1].progref) {
			    if (sys[stop - 1].progref > db_top ||
				    sys[stop - 1].progref < 0 ||
				    (Typeof(sys[stop - 1].progref) != TYPE_PROGRAM))
				abort_loop_hard("Internal error.  Invalid address.", NULL, NULL);
			    calc_profile_timing(program,fr);
			    gettimeofday(&fr->proftime, NULL);
			    DBFETCH(program)->sp.program.instances--;
			    program = sys[stop - 1].progref;
			    mlev = ProgMLevel(program);
			    fr->caller.top--;
			}
			scopedvar_poplevel(fr);
			pc = sys[--stop].offset;
			break;

			case IN_CATCH:
				{
					int depth;

					if (!(fr->trys.top))
						abort_loop_hard("Internal error.  TRY stack underflow.", NULL, NULL);

					depth = fr->trys.st->depth;
					while (atop > depth) {
						temp1 = arg + --atop;
						CLEAR(temp1);
					}

					while (fr->trys.st->for_count-->0) {
						CLEAR(&fr->fors.st->cur);
						CLEAR(&fr->fors.st->end);
						fr->fors.top--;
						fr->fors.st = pop_for(fr->fors.st);
					}

					fr->trys.top--;
					fr->trys.st = pop_try(fr->trys.st);

					if (fr->errorstr) {
						arg[atop].type = PROG_STRING;
						arg[atop++].data.string = alloc_prog_string(fr->errorstr);
						free(fr->errorstr);
						fr->errorstr = NULL;
					} else {
						arg[atop].type = PROG_STRING;
						arg[atop++].data.string = NULL;
					}
					reload(fr, atop, stop);
				}
				pc++;
				break;

			case IN_EVENT_WAITFOR:
				if (atop < 1)
					abort_loop("Stack Underflow. Missing eventID list array argument.", NULL, NULL);
                                if (fr->trys.top && atop - fr->trys.st->depth < 1) 
                                        abort_loop("Stack protection fault.", NULL, NULL);
				temp1 = arg + --atop;
				if (temp1->type != PROG_ARRAY)
					abort_loop("EventID string list array expected.", temp1, NULL);
				if (temp1->data.array && temp1->data.array->type != ARRAY_PACKED)
					abort_loop("Argument must be a list array of eventid strings.", temp1, NULL);
				if (!array_is_homogenous(temp1->data.array, PROG_STRING))
					abort_loop("Argument must be a list array of eventid strings.", temp1, NULL);
				fr->pc = pc + 1;
				reload(fr, atop, stop);

				{
					int i, outcount;
					int count = array_count(temp1->data.array);
					char** events = (char**)malloc(count * sizeof(char**));
					for (outcount = i = 0; i < count; i++) {
						char *val = array_get_intkey_strval(temp1->data.array, i);
						if (val != NULL) {
							int found = 0;
							int j;
							for (j = 0; j < outcount; j++) {
								if (!strcmp(events[j], val)) {
									found = 1;
									break;
								}
							}
							if (!found) {
								events[outcount++] = val;
							}
						}
					}
					muf_event_register_specific(player, program, fr, outcount, events);
					free(events);
				}

                        DBSTORE(player, sp.player.block, (!fr->been_background));
                        if ( player == NOTHING ) {
                            curdescr = get_descr(fr->descr, NOTHING);
                            if (curdescr) { 
                                curdescr->block = 1;
                                DR_RAW_REM_FLAGS(curdescr, DF_INTERACTIVE);
                            }
                        }
				CLEAR(temp1);
				fr->level--;
				calc_profile_timing(program,fr);
				return NULL;
				/* NOTREACHED */
				break;

		    case IN_READ:
			if (writeonly)
			    abort_loop("Program is write-only.", NULL, NULL);
			if (fr->multitask == BACKGROUND)
			    abort_loop("BACKGROUND programs are write only.", NULL, NULL);
			reload(fr, atop, stop);
			fr->brkpt.isread = 1;
			fr->pc = pc + 1;
                        if (fr->player != NOTHING ) {         
   			    DBSTORE(player, sp.player.curr_prog, program);
			    DBSTORE(player, sp.player.block, 0);
                        } else { 
                            curdescr = get_descr(fr->descr, NOTHING);
                            if (curdescr) {
                                DR_RAW_ADD_FLAGS(curdescr, DF_INTERACTIVE);
                                curdescr->interactive = 2;
                                curdescr->block = 0;
                            }
                        } 
			add_muf_read_event(fr->descr, player, program, fr);
			fr->level--;
			calc_profile_timing(program,fr);
			return NULL;
			/* NOTREACHED */
			break;

		    case IN_TREAD:
			if (atop < 1)
				abort_loop("Stack Underflow.", NULL, NULL);
			temp1 = arg + --atop;
			if (temp1->type != PROG_INTEGER)
				abort_loop("Invalid argument type.", temp1, NULL);
			fr->pc = pc + 1;
			reload(fr, atop, stop);
			if (temp1->data.number < 0)
				abort_loop("Timetravel beyond scope of muf.", temp1, NULL);
			if (writeonly)
				abort_loop("Program is write-only.", temp1, NULL);
			if (fr->multitask == BACKGROUND)
				abort_loop("BACKGROUND programs are write only.", temp1, NULL);
			reload(fr, atop, stop);
			fr->brkpt.isread = 1;
			fr->pc = pc + 1;
                        if (fr->player != NOTHING) {
			    DBSTORE(player, sp.player.curr_prog, program);
		    	    DBSTORE(player, sp.player.block, 0);
                        } else {
                            curdescr = get_descr(fr->descr, NOTHING);
                            if (curdescr) { 
                                curdescr->block = 0;
                                curdescr->interactive = 2;
                                DR_RAW_ADD_FLAGS(curdescr, DF_INTERACTIVE);
                            }
                        } 
			add_muf_tread_event(fr->descr, player, program, fr, temp1->data.number);
			fr->level--;
			calc_profile_timing(program,fr);
			return NULL;
			/* NOTREACHED */
			break;

		    case IN_SLEEP:
			if (atop < 1)
			    abort_loop("Stack Underflow.", NULL, NULL);
                        if (fr->trys.top && atop - fr->trys.st->depth < 1) 
                            abort_loop("Stack protection fault.", NULL, NULL);
			temp1 = arg + --atop;
			if (temp1->type != PROG_INTEGER)
			    abort_loop("Invalid argument type.", temp1, NULL);
			fr->pc = pc + 1;
			reload(fr, atop, stop);
			if (temp1->data.number < 0)
			    abort_loop("Timetravel beyond scope of muf.", temp1, NULL);
			add_muf_delay_event(temp1->data.number, fr->descr, player,
				NOTHING, NOTHING, program, fr, "SLEEPING");
			DBSTORE(player, sp.player.block, (!fr->been_background));
                        if (player == NOTHING) {
                            curdescr = get_descr(fr->descr, NOTHING);
                            if (curdescr) 
                                curdescr->block = !(fr->been_background);
                        }
			fr->level--;
			calc_profile_timing(program,fr);
			return NULL;
			/* NOTREACHED */
			break;

		    default:
			nargs = 0;
			reload(fr, atop, stop);
			tmp = atop;
			prim_func[pc->data.number - 1] (player, program, mlev, pc, arg, &tmp, fr);
			atop = tmp;
			pc++;
			break;
		}               /* switch */
		break;
	    case PROG_CLEARED:
		fprintf(stderr, "Attempt to execute instruction cleared by %s:%hd in program %d\n",
		       (char *)pc->data.addr, pc->line, program);
		pc = NULL;
		abort_loop_hard("Program internal error. Program erroneously freed from memory.", NULL, NULL);

	    default:
		pc = NULL;
		abort_loop_hard("Program internal error. Unknown instruction type.", NULL, NULL);
	}                       /* switch */
	if (err) {
			if (fr->trys.top) {
				while (fr->trys.st->call_level < stop) {
					if (stop > 1 && program != sys[stop - 1].progref) {
						if (sys[stop - 1].progref > db_top ||
							sys[stop - 1].progref < 0 ||
							(Typeof(sys[stop - 1].progref) != TYPE_PROGRAM))
									abort_loop_hard("Internal error.  Invalid address.", NULL, NULL);
						calc_profile_timing(program,fr);
						gettimeofday(&fr->proftime, NULL);
						DBFETCH(program)->sp.program.instances--;
						program = sys[stop - 1].progref;
						mlev = ProgMLevel(program);
						fr->caller.top--;
					}
					scopedvar_poplevel(fr);
					stop--;
				}

				pc = fr->trys.st->addr;
				err = 0;
			} else {
				reload(fr, atop, stop);
				prog_clean(fr);
				DBSTORE(player, sp.player.block, 0);
                                if ( player == NOTHING ) {
                                    curdescr = get_descr(fr->descr, NOTHING);
                                    if (curdescr)
                                        curdescr->block = 0;
                                }
				fr->level--;
				calc_profile_timing(program,fr);
				return NULL;
			}
	}
    }                           /* while */

    DBSTORE(player, sp.player.block, 0);
    if ( player == NOTHING ) {
        curdescr = get_descr(fr->descr, NOTHING);
        if (curdescr)
            curdescr->block = 0;
    }
    if (atop) {
	struct inst *rv;

	if (rettyp) {
	    copyinst(arg + atop - 1, &retval); 
	    rv = &retval;
	} else {
	    if (!logical_false(arg + atop - 1)) {
		rv = (struct inst *)1;
	    } else {
		rv = NULL;
	    }
	}
	reload(fr, atop, stop);
	prog_clean(fr);
	fr->level--;
	calc_profile_timing(program,fr);
	return rv;
    }
    reload(fr, atop, stop);
    prog_clean(fr);
    fr->level--;
    calc_profile_timing(program,fr);
    return NULL;
}


void 
interp_err(dbref player, dbref program, struct inst *pc,
	   struct inst *arg, int atop, dbref origprog, const char *msg1, const char *msg2)
{
    char    buf[BUFFER_LEN];
    int     errcount;
    time_t lt;
    char tbuf[40];

    err++;
    if (OWNER(origprog) == player && player != -1) {
	strcpy(buf, "Program Error.  Your program just got the following error.");
    } else {
	sprintf(buf, "Programmer Error.  Please tell %s what you typed, and the following message.",
		NAME(OWNER(origprog)));
    }
    notify_nolisten(player, buf, 1);
    if(FLAG2(origprog) & F2PARENT && player != OWNER(origprog))
       notify_nolisten(OWNER(origprog), "Program Error.  Your program just got the following error.", 1);

    sprintf(buf, "%s(#%d), line %d; %s: %s", NAME(program), program, pc->line,
	    msg1, msg2);
    notify_nolisten(player, buf, 1);
    if(FLAG2(origprog) & F2PARENT && OWNER(origprog) != player)
       notify_nolisten(OWNER(origprog), buf, 1);

    log_status("MUF: %s\n", buf);
    lt = time(NULL);
    format_time(tbuf, 32, "%c", localtime(&lt));
    errcount = get_property_value(origprog, ".debug/errcount");
    errcount++;
    add_property(origprog, ".debug/errcount", NULL, errcount);
    add_property(origprog, ".debug/lasterr", buf, 0);
    add_property(origprog, ".debug/lastcrash", NULL, (int)current_systime);
    add_property(origprog, ".debug/lastplayer", NULL, (int)player);
    add_property(origprog, ".debug/lastcrashtime", tbuf, 0);
    if(*match_cmdname)	add_property(origprog, ".debug/lastcmd", match_cmdname, 0);
    if(*match_args)	add_property(origprog, ".debug/lastarg", match_args, 0);

    if (origprog != program) {
        errcount = get_property_value(program, ".debug/errcount");
        errcount++;
        add_property(program, ".debug/errcount", NULL, errcount);
        add_property(program, ".debug/lasterr", buf, 0);
        add_property(program, ".debug/lastcrash", NULL, (int)current_systime);
        add_property(program, ".debug/lastcrashtime", tbuf, 0);
        add_property(program, ".debug/lastplayer", NULL, (int)player);
        add_property(program, ".debug/origprog", NULL, (int)origprog);
    }
}



void 
push(struct inst *stack, int *top, int type, voidptr res)
{
    stack[*top].type = type;
    if (type < PROG_STRING)
	stack[*top].data.number = *(int *) res;
    else
	stack[*top].data.string = (struct shared_string *) res;
    (*top)++;
}


int 
valid_player(struct inst * oper)
{
    return (!(oper->type != PROG_OBJECT || oper->data.objref >= db_top
	      || oper->data.objref < 0 || (Typeof(oper->data.objref) != TYPE_PLAYER)));
}



int 
valid_object(struct inst * oper)
{
    return (!(oper->type != PROG_OBJECT || oper->data.objref >= db_top
	      || (oper->data.objref < 0) || Typeof(oper->data.objref) == TYPE_GARBAGE));
}


int 
is_home(struct inst * oper)
{
    return (oper->type == PROG_OBJECT && oper->data.objref == HOME);
}


int 
permissions(int mlev, dbref player, dbref thing)
{
    if ( mlev < 0 || mlev > LMAN ) return 0;

    if (
        ( (Protect(thing) && !(mlev >= LBOY) && (MLevel(OWNER(thing)) >= LBOY)) ||
          (Protect(thing) && !(mlev > MLevel(OWNER(thing))))
        ) &&
          !( (FLAG2(OWNER(thing)) & F2ANTIPROTECT) || (FLAG2(thing) & F2ANTIPROTECT) )
        ) return 0;

    if ((mlev >= LWIZ) && (mlev >= MLevel(OWNER(thing)))) return 1;

    if (thing == player)
	return 1;

    switch (Typeof(thing)) {
	case TYPE_PLAYER:
	    return 0;
	case TYPE_EXIT:
	    return (OWNER(thing) == OWNER(player) || OWNER(thing) == NOTHING);
	case TYPE_ROOM:
	case TYPE_THING:
	case TYPE_PROGRAM:
	    return (OWNER(thing) == OWNER(player));
    }

    return 0;
}


int
find_mlev(dbref prog, struct frame * fr, int st)
{
	int maxmlev = (POWERS(OWNER(prog)) & POW_ALL_MUF_PRIMS) ? LBOY : MLevel(OWNER(prog));
	int mlev;
	if ((FLAGS(prog) & STICKY) && (FLAGS(prog) & HAVEN) && ((st > 1) && (TMage(OWNER(prog))))) {
		mlev = find_mlev(fr->caller.st[st - 1], fr, st - 1);
	} else {
		if ((FLAGS(prog) & HAVEN) && ((st > 1) && (TMage(OWNER(prog))))) {
			mlev = MLevel(OWNER(fr->caller.st[st - 1]));
		} else {
			if ((FLAGS(prog) & QUELL) && (FLAGS(prog) & HAVEN)) {
				mlev = MLevel(prog);
			} else {
				if ((FLAGS(prog) & QUELL) && (((fr->player > 0) && (fr->player < db_top)) && (TMage(OWNER(prog))))) {
					mlev = QLevel(OWNER(fr->player));
				} else {
					mlev = (POWERS(OWNER(prog)) & POW_ALL_MUF_PRIMS) ? LBOY : MLevel(OWNER(prog));
				}
			}
		}
	}
	if (maxmlev < mlev) {
		mlev = maxmlev;
	}
	return mlev;
}


dbref 
find_uid(dbref player, struct frame * fr, int st, dbref program)
{
    if ((FLAGS(program) & STICKY) || (fr->perms == STD_SETUID)) {
	if (FLAGS(program) & HAVEN) {
	    if ((st > 1) && (TMage(OWNER(program))))
		return (find_uid(player, fr, st - 1, fr->caller.st[st - 1]));
	    return (OWNER(program));
	}
	return (OWNER(program));
    }
    if (ProgMLevel(program) < 2)
	return (OWNER(program));
    if ((FLAGS(program) & HAVEN) || (fr->perms == STD_HARDUID)) {
	if (fr->trig == NOTHING)
	    return (OWNER(program));
	return (OWNER(fr->trig));
    }
    return (OWNER(player));
}


void 
do_abort_interp(dbref player, const char *msg, struct inst * pc,
		struct inst * arg, int atop, struct frame *fr,
		struct inst * oper1, struct inst * oper2,
		struct inst * oper3, struct inst * oper4, int nargs,
		dbref program, char *file, int line)
{
    char buffer[128];

	if (fr->trys.top) {
		fr->errorstr = string_dup(msg);
		err++;
	} else {
	    fr->pc = pc;
	    calc_profile_timing(program,fr);
	    interp_err(player, program, pc, arg, atop, fr->caller.st[1],
		       insttotext(fr, 0, pc, buffer, sizeof(buffer), 30, program), msg);
	    if (controls(player, program) && player != -1)
		muf_backtrace(player, program, STACK_SIZE, fr);
	/*    else */
	       if (FLAG2(program) & F2PARENT && player != OWNER(program))
	          muf_backtrace(OWNER(program), program, STACK_SIZE, fr);
	}
    switch (nargs) {
	case 4:
	    if (oper4)  RCLEAR(oper4, file, line);
	case 3:
	    if (oper3)  RCLEAR(oper3, file, line);
	case 2:
	    if (oper2)  RCLEAR(oper2, file, line);
	case 1:
	    if (oper1)  RCLEAR(oper1, file, line);
    }
    return;
}


void
do_abort_silent(void)
{
	/* WORK:  killing this program's pid may not exit, but instead will
	 * be caught in a TRY-CATCH block.  This may be undesirable. */
    err++;
}









