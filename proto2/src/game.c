#include "copyright.h"
#include "config.h"

#include "db.h"
#include "mpi.h"
#include "props.h"
#include "params.h"
#include "tune.h"
#include "interp.h"
#include "interface.h"
#include "match.h"
#include "msgparse.h"
#include "externs.h"
#include "strings.h"
#include "netresolve.h"

#define anotify_nolisten2(x, y) anotify_nolisten(x, y, 1);

#ifdef MCCP_ENABLED
extern void do_compress(dbref player, int descr, const char *msg);
#endif /* MCCP_ENABLED */

extern struct inst temp1;
/* declarations */
static const char *dumpfile = 0;

static int epoch = 0;

FILE *input_file;

FILE *delta_infile;

FILE *delta_outfile;

char *in_filename = NULL;

int dumper_pid;

char dump_done;

/* This is the command, @autoarchive. */
void
do_autoarchive(int descr, dbref player)
{
    char timebuf[BUFFER_LEN];

    if (!(Boy(player))) {
        log_status("SHAM: by %s\n", unparse_object(player, player));
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
        return;
    }
    if (auto_archive_now()) {
        sprintf(timebuf, "%d minutes", ARCHIVE_DELAY / 60);
        anotify_fmt(player,
                    CINFO "Need to wait at least %s between @autoarchives.",
                    timebuf);
        return;
    }
    log_status("ARCHIVE: by %s\n", unparse_object(player, player));
    anotify(player, CSUCC "The site archiving script has been called.");
}

/* This does the actual script call. Changes to how site archiving 
 * works and operates shoudl probably be made here.
 */
void
archive_site()
{
    system("./archive &");
}

void
do_dump(dbref player, const char *newfile)
{
    char buf[BUFFER_LEN];

    char *yerf = NULL;

    if (Wiz(player)) {
        if (*newfile && Boy(player)) {
            if (*dumpfile) {
                yerf = (char *) dumpfile;
                dumpfile = newfile;
            } else {
                dumpfile = alloc_string(newfile);
                yerf = (char *) dumpfile;
            }
            sprintf(buf, CINFO "Dumping to file %s...", dumpfile);
        } else {
            sprintf(buf, CINFO "Dumping to file %s...", dumpfile);
        }
        anotify_nolisten2(player, buf);
        dump_db_now();
        anotify_nolisten2(player, CINFO "Done.");
        if (*newfile)
            dumpfile = (char *) yerf;
    } else {
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
    }
}

#ifdef DELTADUMPS
void
do_delta(dbref player)
{
    if (Mage(player)) {
        anotify_nolisten2(player, CINFO "Dumping deltas...");
        delta_dump_now();
        anotify_nolisten2(player, CINFO "Done.");
    } else
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
}
#endif

/* Toggle wizonly and start a shutdown cycle. The final shutdown_flag gets set
 * inside of shovechars when the delay has elapsed.
 *
 * While delayed_shutdown is positive, all interpreter loops will be sent
 * sent a SHUTDOWN event that is exclusive to this shutdown cycle if they
 * haven't seen one already. If this shutdown cycle is aborted and another
 * is started later, they will get another event for the new cycle (unless the
 * old event was never handled).
 * 
 * -brevantes  */
void
do_delayed_shutdown(dbref player)
{
    if (tp_shutdown_delay < 1) {
        /* This should only happen if someone is messing around with the caller
         * functions that use us. */
        notify(player, SYSRED
               "LOGIC ERROR: do_delayed_shutdown run with negative delay.");
        return;
    }
    char buf[BUFFER_LEN];
    delayed_shutdown = current_systime + tp_shutdown_delay;
    wizonly_mode = 1;
    
    temp1.type = PROG_INTEGER;
    temp1.data.number = delayed_shutdown;

    /* We need to wake up any WAIT events and let them know about the shutdown.
     * muf_event_process will make sure any woken up events don't get a second
     * SHUTDOWN event when they're passed to interp_loop. -brevantes */
    broadcast_muf_event("SHUTDOWN", &temp1, 1, 1);
    propqueue(0, 1, 0, -1, 0, -1, "@shutdown", "Shutdown", 1, 1);

    notify(player, SYSRED "The countdown starts...");


    sprintf(buf, "%s%s%sSystem shutdown in %li seconds.\r\n",
            SYSWHITE, MARK, SYSNORMAL, (long int)tp_shutdown_delay);
    wall_and_flush(buf);
}

/* Cancel a delayed shutdown. This does not clean up any of the SHUTDOWN events
 * that were broadcasted; individual processes are responsible for examining
 * the timestamp that was packaged with the SHUTDOWN event and making a
 * decision based on whether systime has exceeded it.
 *
 * This does not untoggle wizonly, and informs the user of such. -brevantes */
void
cancel_delayed_shutdown(dbref player)
{
    char buf[BUFFER_LEN];

    if (delayed_shutdown) {
        restart_flag = 0;
        delayed_shutdown = 0;

        log_status("Countdown aborted by %s\n", unparse_object(player, player));

        sprintf(buf, "%s%s%sShutdown cancelled.\r\n", SYSWHITE, MARK, SYSNORMAL);
        wall_and_flush(buf);

        notify(player, SYSYELLOW
            "Countdown aborted. Use \"@restrict off\" to enable logins.");

        /* Restore default shutdown messages. */
        strcpy(restart_message, "\r\nServer restarting, be back in a few!\r\n");
        strcpy(shutdown_message, "\r\nServer shutting down, be back in a few!\r\n");
        return;
    } else {
        notify(player,
               SYSYELLOW "No shutdown in progress.");
        return;
    }
}

void
do_shutdown(dbref player, const char *muckname, const char *msg)
{
    int nodelay = !strcmp(msg, "now");
    int cancel = !strcmp(msg, "abort");

    if ((Arch(player)) || (POWERS(player) & POW_SHUTDOWN)) {
        if (*muckname == '\0' || strcmp(muckname, tp_muckname)) {
            notify(player, SYSCYAN
                "Usage: " SYSAQUA "@shutdown muckname[=now|abort|message]");
            return;
        }
        if (cancel) {
            cancel_delayed_shutdown(player);
            return;
        }
        if (!nodelay && delayed_shutdown) {
            notify(player, SYSRED
                   "Delayed shutdown already in progress.");
            notify(player, SYSCYAN
                   "    @shutdown muckname=abort (to cancel shutdown)");
            notify(player, SYSCYAN
                   "    @shutdown muckname=now   (immediate shutdown)");
            return;
        }
        if (!nodelay && tp_shutdown_delay > 0) {
            log_status("SHUT: by %s (%lis delay)\n",
                unparse_object(player, player), (long int)tp_shutdown_delay);
            do_delayed_shutdown(player);
        } else {
            log_status("SHUT: by %s\n", unparse_object(player, player));
            shutdown_flag = 1;
        }
        restart_flag = 0;
        if (!nodelay && *msg != '\0') {
            strcat(shutdown_message, SYSWHITE MARK SYSNORMAL);
            strcat(shutdown_message, msg);
            strcat(shutdown_message, "\r\n");
        }
    } else {
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
        log_status("SHAM: Shutdown by %s\n", unparse_object(player, player));
    }
}

void
do_restart(dbref player, const char *muckname, const char *msg)
{
    int nodelay = !strcmp(msg, "now");
    int cancel = !strcmp(msg, "abort");


    if ((Arch(player)) || (POWERS(player) & POW_SHUTDOWN)) {
        if (*muckname == '\0' || strcmp(muckname, tp_muckname)) {
            notify(player, SYSCYAN
                   "Syntax: " SYSAQUA "@restart muckname[=now|abort|message]");
            return;
        }
        if (cancel) {
            cancel_delayed_shutdown(player);
            return;
        }
        if (!nodelay && delayed_shutdown) {
            notify(player, SYSRED
                   "Delayed shutdown already in progress.");
            notify(player, SYSCYAN
                   "    @restart muckname=abort (to cancel shutdown)");
            notify(player, SYSCYAN
                   "    @restart muckname=now   (immediate restart)");
            return;
        }
        if (!nodelay && tp_shutdown_delay > 0) {
            log_status("REST: by %s (%lis delay)\n",
                unparse_object(player, player), (long int)tp_shutdown_delay);
            do_delayed_shutdown(player);
        } else {
            log_status("REST: by %s\n", unparse_object(player, player));
            shutdown_flag = 1;
        }
        if (!nodelay && *msg != '\0') {
            strcat(restart_message, SYSWHITE MARK SYSNORMAL);
            strcat(restart_message, msg);
            strcat(restart_message, "\r\n");
        }
        restart_flag = 1;
    } else {
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
        log_status("SHAM: Restart by %s\n", unparse_object(player, player));
    }
}


#ifdef DISKBASE
extern int propcache_hits;

extern int propcache_misses;
#endif

static void
dump_database_internal(void)
{
    char tmpfile[2048];

    FILE *f;

    int copies;

    char buf[BUFFER_LEN];

    tune_save_parmsfile();

    sprintf(tmpfile, "%s.#%d#", dumpfile, epoch);

    if ((f = fopen(tmpfile, "w")) != NULL) {
        char fromfile[512], destfile[512];

        db_write(f);
        fclose(f);
        if (tp_dump_copies > 0) {
            copies = (tp_dump_copies < 25) ? tp_dump_copies : 25;
            for (; copies > 1; copies--) {
                sprintf(fromfile, "backup/%s.#%d#", dumpfile, copies - 1);
                sprintf(destfile, "backup/%s.#%d#", dumpfile, copies);
#ifdef WIN_VC
                unlink(destfile);
#endif
                rename(fromfile, destfile);
            }
            sprintf(destfile, "backup/%s.#1#", dumpfile);
#ifdef WIN_VC
            unlink(destfile);   /* proto.new.#1# */
#endif
            rename(dumpfile, destfile); /* proto.new -> proto.new.1 */

        }
#ifdef WIN_VC
        /* Windows rename() differs from Unix rename(). Won't overwrite. */
        if (unlink(dumpfile))
            perror(dumpfile);
#endif
        if (rename(tmpfile, dumpfile) < 0) {
            perror(tmpfile);
            sprintf(buf,
                    SYSRED
                    "[WARNING] Error renaming the DB from %s to %s.  The DB got saved to %s.",
                    tmpfile, dumpfile, tmpfile);
            ansi_wall_wizards(buf);
        }
#ifdef DISKBASE

        fclose(input_file);
        free((void *) in_filename);
        in_filename = string_dup(dumpfile);
        if ((input_file = fopen(in_filename, "r")) == NULL)
            perror(dumpfile);


#ifdef DELTADUMPS
        fclose(delta_outfile);
        if ((delta_outfile = fopen(DELTAFILE_NAME, "w")) == NULL)
            perror(DELTAFILE_NAME);

        fclose(delta_infile);
        if ((delta_infile = fopen(DELTAFILE_NAME, "r")) == NULL)
            perror(DELTAFILE_NAME);
#endif
#endif

    } else {
        perror(tmpfile);
        sprintf(buf,
                SYSRED
                "[DANGER] Error opening the db file %s for writing!  The DB did not save! :(",
                tmpfile);
        ansi_wall_wizards(buf);
    }

    /* Write out the macros */

    sprintf(tmpfile, "%s.#%d#", MACRO_FILE, epoch);

    if ((f = fopen(tmpfile, "w")) != NULL) {
        macrodump(macrotop, f);
        fclose(f);
#ifdef WIN_VC
        if (unlink(MACRO_FILE))
            perror(MACRO_FILE);
#endif
        if (rename(tmpfile, MACRO_FILE) < 0)
            perror(tmpfile);
    } else {
        perror(tmpfile);
        sprintf(buf,
                SYSRED
                "[WARN] Error opening the MUF macros file %s for writing.",
                tmpfile);
        ansi_wall_wizards(buf);
    }

#ifdef DISKBASE
    if (tp_dbdump_warning)
        wall_and_flush(tp_dumpdone_mesg);

    propcache_hits = 0L;
    propcache_misses = 1L;
#endif

    if (tp_periodic_program_purge)
        free_unused_programs();

#ifdef DISKBASE
    dispose_all_oldprops();
#endif
}

void
panic(const char *message)
{
    char panicfile[2048];

    FILE *f;

    log_status("PANIC: %s\n", message);
    fprintf(stderr, "PANIC: %s\n", message);

    /* shut down interface */
    emergency_shutdown();

    /* dump panic file */
    sprintf(panicfile, "%s.PANIC", dumpfile);
    if ((f = fopen(panicfile, "w")) == NULL) {
        perror("CANNOT OPEN PANIC FILE, YOU LOSE");

#if defined(NOCOREDUMP)
        _exit(135);
#else /* !NOCOREDUMP */
# ifdef SIGIOT
        signal(SIGIOT, SIG_DFL);
# endif
        abort();
#endif /* NOCOREDUMP */
    } else {
        log_status("DUMP: %s\n", panicfile);
        fprintf(stderr, "DUMP: %s\n", panicfile);
        db_write(f);
        fclose(f);
        log_status("DUMP: %s (done)\n", panicfile);
        fprintf(stderr, "DUMP: %s (done)\n", panicfile);
        (void) unlink(DELTAFILE_NAME);
    }

    /* Write out the macros */
    sprintf(panicfile, "%s.PANIC", MACRO_FILE);
    if ((f = fopen(panicfile, "w")) != NULL) {
        macrodump(macrotop, f);
        fclose(f);
    } else {
        perror("CANNOT OPEN MACRO PANIC FILE, YOU LOSE");
#if defined(NOCOREDUMP)
        _exit(135);
#else /* !NOCOREDUMP */
# ifdef SIGIOT
        signal(SIGIOT, SIG_DFL);
# endif
        abort();
#endif /* NOCOREDUMP */
    }

#if defined(NOCOREDUMP)
    _exit(136);
#else /* !NOCOREDUMP */
# ifdef SIGIOT
    signal(SIGIOT, SIG_DFL);
# endif
    abort();
#endif /* NOCOREDUMP */
}

void
dump_database(bool dofork)
{
    epoch++;

    log_status("DUMP: %s.#%d#\n", dumpfile, epoch);

    if (tp_db_events) {
        temp1.type = PROG_INTEGER;
        temp1.data.number = current_systime;

        broadcast_muf_event("DUMP", &temp1, 0, 1);
    }
    if (tp_dump_propqueues)
        propqueue(0, 1, 0, -1, 0, -1, "@dump", "Dump", 1, 1);
    if (tp_dbdump_warning)
        wall_and_flush(tp_dumping_mesg);

#ifdef DISKBASE
    dump_database_internal();
#else
    /* Alynna - saving the PID of the dumper so I can get around an SSL issue */
#ifndef WIN_VC
    if (dofork) {
        if (!(dumper_pid = fork())) {
            dump_database_internal();
            _exit(0);
        }
    } else
#endif
        dump_database_internal();
#endif
    log_status("DUMP: %s.#%d# (done)\n", dumpfile, epoch);
}

time_t last_monolithic_time = 0;

/*
 * Named "fork_and_dump()" mostly for historical reasons...
 */
void
fork_and_dump(bool dofork)
{
    epoch++;

    last_monolithic_time = current_systime;
    log_status("DUMP: %s.#%d#\n", dumpfile, epoch);

    if (tp_db_events) {
        temp1.type = PROG_INTEGER;
        temp1.data.number = current_systime;

        broadcast_muf_event("DUMP", &temp1, 0, 1);
    }
    if (tp_dump_propqueues)
        propqueue(0, 1, 0, -1, 0, -1, "@dump", "Dump", 1, 1);
    if (tp_dbdump_warning)
        wall_and_flush(tp_dumping_mesg);

#ifdef DISKBASE
    dump_database_internal();
#else
    /* Alynna - saving the PID of the dumper so I can get around an SSL issue */
#ifndef WIN_VC
    if (dofork) {
        if (!(dumper_pid = fork())) {
            dump_database_internal();
            _exit(0);
        }
    } else
#endif
        dump_database_internal();
#endif
    time(&current_systime);
    host_check_cache();
    host_save();
}

#ifdef DELTADUMPS
extern deltas_count;

int
time_for_monolithic(void)
{
    dbref i;

    int count = 0;

    int a, b;

    if (!last_monolithic_time)
        last_monolithic_time = current_systime;
    if (current_systime - last_monolithic_time >=
        (tp_monolithic_interval - tp_dump_warntime)
        ) {
        return 1;
    }

    for (i = 0; i < db_top; i++)
        if (FLAGS(i) & (SAVED_DELTA | OBJECT_CHANGED))
            count++;
    if (((count * 100) / db_top) > tp_max_delta_objs) {
        return 1;
    }
#ifdef DISKBASE
    fseek(delta_infile, 0L, 2);
    a = ftell(delta_infile);
    fseek(input_file, 0L, 2);
    b = ftell(input_file);
    if (a >= b) {
        return 1;
    }
#endif
    return 0;
}
#endif

void
dump_warning(void)
{
    if (tp_dbdump_warning) {
#ifdef DELTADUMPS
        if (time_for_monolithic()) {
            wall_and_flush(tp_dumpwarn_mesg);
        } else {
            if (tp_deltadump_warning) {
                wall_and_flush(tp_deltawarn_mesg);
            }
        }
#else
        wall_and_flush(tp_dumpwarn_mesg);
#endif
    }
    if (tp_db_events) {
        temp1.type = PROG_INTEGER;
        temp1.data.number = current_systime + tp_dump_warntime;

        broadcast_muf_event("DUMPWARN", &temp1, 0, 1);
    }
    if (tp_dump_propqueues)
        propqueue(0, 1, 0, -1, 0, -1, "@dumpwarn", "Dumpwarn", 1, 1);
}

#ifdef DELTADUMPS
void
dump_deltas(void)
{
    if (time_for_monolithic()) {
        fork_and_dump(1);
        deltas_count = 0;
        return;
    }

    epoch++;
    log_status("DELT: %s.#%d#\n", dumpfile, epoch);

    if (tp_deltadump_warning)
        wall_and_flush(tp_dumpdeltas_mesg);

    db_write_deltas(delta_outfile);

    if (tp_deltadump_warning)
        wall_and_flush(tp_dumpdone_mesg);
#ifdef DISKBASE
    propcache_hits = 0L;
    propcache_misses = 1L;
#endif
    host_save();
}
#endif

int
init_game(const char *infile, const char *outfile)
{
    FILE *f;

    log_status_nowall("init_game\n");

    if ((f = fopen(MACRO_FILE, "r")) == NULL)
        log_status_nowall("INIT: Macro storage file %s is tweaked.\n",
                          MACRO_FILE);
    else {
        macroload(f);
        fclose(f);
    }
    log_status_nowall("init_game: macro file loaded\n");
    in_filename = (char *) string_dup(infile);
    if ((input_file = fopen(infile, "r")) == NULL) {
        log_status_nowall("DIE: input file not readable\n");
        return -1;
    }
#ifdef DELTADUMPS
    if ((delta_outfile = fopen(DELTAFILE_NAME, "w")) == NULL) {
        log_status_nowall("DIE: delta outfile not writable\n");
        return -1;
    }
    if ((delta_infile = fopen(DELTAFILE_NAME, "r")) == NULL) {
        log_status_nowall("DIE: delta infile not readable\n");
        return -1;
    }
#endif

    db_free();
    log_status_nowall("init_game/db_free\n");
    init_primitives();
    log_status_nowall("init_game/init_primitives: MUF initialized\n"); /* init muf compiler */
    mesg_init();
    log_status_nowall("init_game/mesg_init: MPI initialized\n");
    /* init mpi interpreter */
    SRANDOM(getpid());
    log_status_nowall("init_game: random number generator initialized\n"); /* init random number generator */
    tune_load_parmsfile(NOTHING); /* load @tune parms from file */
    log_status_nowall("init_game: loaded parmfile.cfg\n");
    /* ok, read the db in */
    log_status_nowall("LOAD: %s\n", infile);
    fprintf(stderr, "LOAD: %s\n", infile);
    if (db_read(input_file) < 0) {
        log_status_nowall("DIE: database load error.  Damnit.\n");
        return -1;
    }
    log_status_nowall("LOAD: %s (done)\n", infile);
    fprintf(stderr, "LOAD: %s (done)\n", infile);

#ifndef DISKBASE
    /* everything ok */
    fclose(input_file);
#endif

    tune_load_parmsfile(NOTHING); /* load @tune parms from file */
    /* set up dumper */
    if (dumpfile)
        free((void *) dumpfile);
    dumpfile = alloc_string(outfile);

    if (!db_conversion_flag) {
        /* initialize the ~sys/startuptime property */
        add_property((dbref) 0, "~sys/startuptime", NULL,
                     (int) time((time_t *) NULL));
        add_property((dbref) 0, "~sys/maxpennies", NULL, tp_max_pennies);
        add_property((dbref) 0, "~sys/dumpinterval", NULL, (int)tp_dump_interval);
        add_property((dbref) 0, "~sys/concount", NULL, 0);
        add_property((dbref) 0, "~sys/max_connects", NULL, 0);
        if (tp_allow_old_trigs) {
            add_property((dbref) 0, "_sys/startuptime", NULL,
                         (int) time((time_t *) NULL));
            add_property((dbref) 0, "_sys/maxpennies", NULL, tp_max_pennies);
            add_property((dbref) 0, "_sys/concount", NULL, 0);
            add_property((dbref) 0, "_sys/dumpinterval", NULL,
                         (int)tp_dump_interval);
            add_property((dbref) 0, "_sys/max_connects", NULL, 0);
        }
    }
    /* Load the local flags into memory. */
    lflags_update();
    return 0;
}

void
cleanup_game(void)
{
    if (dumpfile)
        free((void *) dumpfile);
    free((void *) in_filename);

}

void
do_restrict(dbref player, const char *arg)
{
    if (!Wiz(player)) {
        notify(player, tp_noperm_mesg);
        return;
    }

    if (!strcmp(arg, "on")) {
        wizonly_mode = 1;
        anotify(player,
                CSUCC "Login access is now restricted to wizards only.");
    } else if (!strcmp(arg, "off")) {
        wizonly_mode = 0;
        anotify(player, CSUCC "Login access is now unrestricted.");
    } else {
        anotify_fmt(player, CINFO "Restricted mode is: %s",
                    wizonly_mode ? "on" : "off");
    }
}


/* use this only in process_command */
#define Matched(string) { if(!string_prefix((string), command)) goto bad; }

int force_level = 0;

struct frame *aForceFrameStack[9];

const char version_line1[] =
    SYSCRIMSON "ProtoMUCK " PROTOBASE SYSPURPLE " (" SYSRED VERSION SYSPURPLE
    ")" SYSNORMAL " on " SYSCYAN 
#if defined(WIN32) || defined(WIN_VC)
    "Windows (native)" 
#else  /* 
 */
# ifdef CYGWIN
    "Windows (cygwin)" 
# else
#  if defined(__APPLE__)
    "Mac OS X" 
#  else
#   if defined(__linux__)
    "GNU/Linux" 
#   else
    "Unix" 
#   endif
#  endif
# endif
#endif  /* 
 */
    SYSNORMAL ": " SYSGREEN UNAME_VALUE SYSNORMAL;



const char version_line2[] = SYSGREEN "Compile-time Options: " SYSNORMAL 
#ifdef SQL_SUPPORT
    "MySQL " 
#endif

#ifdef USE_SSL
    "SSL " 
#endif

#ifdef USE_RESLVD
    "ReslvD " 
#endif

#ifdef IPV6
    "IPv6 " 
#endif

#ifdef CONTROLS_SUPPORT
    "ControlsACLs " 
#endif

#ifdef DESCRFILE_SUPPORT
    "DescrFile " 
#endif

#ifdef IGNORE_SUPPORT
    "Ignores " 
#endif 

#ifndef NO_SYSCOLOR
    "Color " 
#endif

#ifdef __cplusplus
	"C++ "
#endif

#ifdef MCP_SUPPORT
    "MCP " 
#endif 

#ifdef ARCHAIC_DATABASES
    "ArchaicDB " 
#endif

#ifdef DETACH
    "Detach " 
#endif

#ifdef COMPRESS
    "Compression " 
#endif

#ifdef USE_PS
    "ProcessAPI " 
#endif

#ifdef DISKBASE
    "DiskBased " 
#else
    "MemBased " 
#endif

#ifdef DELTADUMPS
    "Deltas " 
#endif

    "Userflags " 
#ifdef MUF_SOCKETS
    "MUF:Sockets " 
#endif

#ifdef USE_SSL
# ifdef SSL_SOCKETS
    "MUF:SSL " 
# endif
#endif

#ifdef MCCP_ENABLED
    "MCCP "
#endif

#ifdef UTF8_SUPPORT
    "UTF8 "
#endif

#ifdef UDP_SOCKETS
    "MUF:UDP " 
#endif

#ifdef IPV6
    "MUF:IPv6 " 
#endif

#ifdef MUF_EDIT_PRIMS
    "MUF:Edit " 
#endif

#ifdef PCRE_SUPPORT
    "MUF:PCRE "
#endif

#ifdef DEBUGPROCESS
# ifdef DBDEBUG
    "Debug:3 " 
# else
    "Debug:2 " 
# endif
#else
# ifdef DBDEBUG
    "Debug:1 " 
# else
    "Debug:0 " 
# endif
#endif

#if defined(WIN_VC) && defined(_DEBUG)
	"WIN_DEBUG "
#endif
    ;


extern void do_muf_funcprofs(dbref player, char *arg1);
extern void do_modload(dbref player, char *arg);
extern void do_modunload(dbref player, char *arg);
extern void do_modinfo(dbref player, char *arg);

void
process_command(int descr, dbref player, char *command, int len, int wclen)
{
    /* TODO: Now that initial string length is being passed in, most instances
     *       of strcpy could be converted to a faster memcpy. -brevantes */

    char *arg1;

    char *arg2;

    char *full_command, /* *commandline=command, */ *commandstuff;

    char *p;                    /* utility */

    char pbuf[BUFFER_LEN];

    char xbuf[BUFFER_LEN];

    char ybuf[BUFFER_LEN];

    char zbuf[BUFFER_LEN];

    /* char cbuf[BUFFER_LEN]; char *c; int csharp; */
    int isOverride = 0;

    if (command == 0) {
        fprintf(stderr, "PANIC: Null command passed to process_command.\n");
        abort();
    }

    if (force_level >= 32) {
        anotify_fmt(player,
                    CFAIL
                    "I'm sorry, %s, I'm afraid I cannot do that.  Maximum force recursion depth exceeded.",
                    NAME(player));
        return;
    }

    /* robustify player */
    if (player < 0 || player >= db_top ||
        (Typeof(player) != TYPE_PLAYER && Typeof(player) != TYPE_THING)) {
        log_status("process_command: bad player %d\n", player);
        return;
    }

    if (((tp_log_commands || (tp_log_guests && Guest(OWNER(player)))) ||
         (tp_log_suspects && (FLAG2(OWNER(player)) & F2SUSPECT)) ||
         (tp_log_wizards && (MLevel(OWNER(player)) >= LMAGE))
        ) &&
        (tp_log_interactive || !(FLAGS(player) & (INTERACTIVE | READMODE)))) {
        if (*command)           /* To prevent logging of NULL commands? FB6 change */
            log_command("%s%s%s%s(%d) in %s(%d):%s %s\n",
                        (MLevel(OWNER(player)) >=
                         LMAGE) ? "WIZ: " : (FLAG2(OWNER(player)) & F2SUSPECT) ?
                        "SUSPECT: " : "",
                        (Typeof(player) != TYPE_PLAYER) ? NAME(player) : "",
                        (Typeof(player) != TYPE_PLAYER) ? " by " : "",
                        NAME(OWNER(player)), (int) player,
                        NAME(DBFETCH(player)->location),
                        (int) DBFETCH(player)->location,
                        (FLAGS(player) & INTERACTIVE) ? " [interactive]" : " ",
                        command);
    }

    if (FLAGS(player) & INTERACTIVE) {
        interactive(descr, player, command, len, wclen);
        return;
    }
    /* eat leading whitespace */
    while (*command && isspace(*command))
        (void) command++;
    /* disable null command once past READ stuff */
    if (!*command)
        return;

    commandstuff = command;
    /* check for single-character commands */
    if (*command == SAY_TOKEN || *command == '\'') {
        sprintf(pbuf, "say %s", command + 1);
        command = &pbuf[0];
    } else if (*command == POSE_TOKEN || *command == ';') {
        sprintf(pbuf, "pose %s", command + 1);
        command = &pbuf[0];
    } else
        if ((*command == '|'
             || (*commandstuff++ == '\\' && *commandstuff == '\\'))
            && can_move(descr, player, "spoof", 0)) {
        if (*command == '\\')
            sprintf(pbuf, "spoof %s", command + 2);
        else
            sprintf(pbuf, "spoof %s", command + 1);
        command = &pbuf[0];
    }

    /* if player is a wizard, and uses overide token to start line... */
    /* ... then do NOT run actions, but run the command they specify. */
    if (!strncmp(command, WHO_COMMAND, sizeof(WHO_COMMAND) - 1)) {
        char xxbuf[BUFFER_LEN];

        strcpy(xxbuf, "@");
        strcat(xxbuf, WHO_COMMAND);
        strcat(xxbuf, " ");
        strcat(xxbuf, command + sizeof(WHO_COMMAND) - 1);
        if (can_move(descr, player, xxbuf, 5)) {
            do_move(descr, player, xxbuf, 5);
        } else {
            pdump_who_users(descr, command + sizeof(WHO_COMMAND) - 1);
        }
        return;
    }

    /* Alynna: fix to process truly only argc from the command line .. 
       memset(cbuf, 0, sizeof(cbuf)); c = command;
       for(csharp = 0; !isspace(*(c+csharp)); csharp++) {
       cbuf[csharp] = *(c+csharp);
       }
       cbuf[csharp+1] = '\0';
     */
    if (!(*command == OVERIDE_TOKEN && TMage(player))) {
        switch (can_move2(descr, player, command, 0)) {
            case 1:
                do_move(descr, player, command, 0); /* command is exact match for exit */
                *match_args = 0;
                *match_cmdname = 0;
                return;
            case 2:
                *match_args = 0;
                *match_cmdname = 0;
                return;
        }
    }

    if (*command == OVERIDE_TOKEN && TMage(player)) {
        command++;
        isOverride = 1;
    }

    full_command = strcpy(xbuf, command);
    for (; *full_command && !isspace(*full_command); full_command++) ;
    if (*full_command)
        full_command++;

    /* find arg1 -- move over command word */
    command = strcpy(ybuf, command);
    for (arg1 = command; *arg1 && !isspace(*arg1); arg1++) ;
    /* truncate command */
    if (*arg1)
        *arg1++ = '\0';

    /* remember command for programs */
    strcpy(match_cmdname, command);

    /* move over spaces */
    while (*arg1 && isspace(*arg1))
        arg1++;

    /* find end of arg1, start of arg2 */
    for (arg2 = arg1; *arg2 && *arg2 != ARG_DELIMITER; arg2++) ;

    /* truncate arg1 */
    for (p = arg2 - 1; p >= arg1 && isspace(*p); p--)
        *p = '\0';

    /* go past delimiter if present */
    if (*arg2)
        *arg2++ = '\0';
    while (*arg2 && isspace(*arg2))
        arg2++;

    strcpy(match_cmdname, command);
    strcpy(match_args, full_command);
    if (!(isOverride && TMage(player)) && tp_enable_commandprops) {
        if (prop_command(descr, player, command, full_command, "@command", 1))
            return;
        if (prop_command(descr, player, command, full_command, "@ocommand", 0))
            return;
        if (prop_command(descr, player, command, full_command, "~command", 1))
            return;
        if (prop_command(descr, player, command, full_command, "~ocommand", 0))
            return;
        if (prop_command(descr, player, command, full_command, "_command", 1))
            return;
        if (prop_command(descr, player, command, full_command, "_ocommand", 0))
            return;
    }
    switch (command[0]) {
        case '@':
            switch (command[1]) {
                case 'a':
                case 'A':
                    /* @action, @attach */
                    switch (command[2]) {
                        case 'c':
                        case 'C':
                            Matched("@action");
                            do_action(descr, player, arg1, arg2);
                            break;
                        case 'r':
                        case 'R':
                            if (string_compare(command, "@armageddon"))
                                goto bad;
                            do_armageddon(player, full_command);
                            break;
                        case 't':
                        case 'T':
                            Matched("@attach");
                            do_attach(descr, player, arg1, arg2);
                            break;
                        case 'n':
                        case 'N':
                            Matched("@ansidescribe");
                            do_ansidescribe(descr, player, arg1, arg2);
                            break;
                        case 'u':
                        case 'U':
                            if (string_compare(command, "@autoarchive"))
                                goto bad;
                            if (!tp_auto_archive)
                                goto bad;
                            do_autoarchive(descr, player);
                            break;
                        default:
                            goto bad;
                    }
                    break;
                case 'b':
                case 'B':
                    Matched("@boot");
                    do_boot(player, arg1);
                    break;
                case 'c':
                case 'C':
                    /* chown, contents, create */
                    switch (command[2]) {
                        case 'h':
                        case 'H':
                            switch (command[3]) {
                                case 'a':
                                case 'A':
                                    Matched("@charset");
                                    do_encoding(descr, player, arg1);
                                    break;
                                case 'l':
                                case 'L':
                                    Matched("@chlock");
                                    do_chlock(descr, player, arg1, arg2);
                                    break;
                                case 'o':
                                case 'O':
                                    if (strlen(command) < 7) {
                                        Matched("@chown")
                                            do_chown(descr, player, arg1, arg2);
                                    } else {
                                        Matched("@chown_lock");
                                        do_chlock(descr, player, arg1, arg2);
                                    }
                                    break;
                                default:
                                    goto bad;
                            }
                            break;
                        case 'o':
                        case 'O':
                            switch (command[4]) {
                                case 'l':
                                case 'L':
                                    Matched("@conlock");
                                    do_conlock(descr, player, arg1, arg2);
                                    break;
                                case 't':
                                case 'T':
                                    Matched("@contents");
                                    do_contents(descr, player, arg1, arg2);
                                    break;
                                default:
                                    goto bad;
                            }
                            break;
                        case 'r':
                        case 'R':
                            if (string_compare(command, "@credits")) {
                                Matched("@create");
                                do_create(player, arg1, arg2);
                            } else {
                                do_credits(player);
                            }
                            break;
                        default:
                            goto bad;
                    }
                    break;
                case 'd':
                case 'D':
                    /* describe, dequeue, dig, or dump */
                    switch (command[2]) {
                        case 'b':
                        case 'B':
                            Matched("@dbginfo");
                            do_serverdebug(descr, player, arg1, arg2);
                            break;
                        case 'e':
                        case 'E':
#ifdef DELTADUMPS
                            if (command[3] == 'l' || command[3] == 'L') {
                                Matched("@delta");
                                do_delta(player);
                            } else
#endif
                            {
                                if ((command[3] == 's' || command[3] == 'S') &&
                                    (command[4] == 't' || command[4] == 'T')) {
                                    Matched("@destroy");
                                    do_recycle(descr, player, arg1);
                                } else {
                                    Matched("@describe");
                                    do_describe(descr, player, arg1, arg2);
                                }
                            }
                            break;
                        case 'i':
                        case 'I':
                            Matched("@dig");
                            do_dig(descr, player, arg1, arg2);
                            break;
#ifdef DELTADUMPS
                        case 'l':
                        case 'L':
                            Matched("@dlt");
                            do_delta(player);
                            break;
#endif
                        case 'o':
                        case 'O':
                            Matched("@doing");
                            if (!tp_who_doing)
                                goto bad;
                            do_doing(descr, player, arg1, arg2);
                            break;
                        case 'r':
                        case 'R':
                            Matched("@drop");
                            do_drop_message(descr, player, arg1, arg2);
                            break;
                        case 'u':
                        case 'U':
                            Matched("@dump");
                            do_dump(player, full_command);
                            break;
                        default:
                            goto bad;
                    }
                    break;
                case 'e':
                case 'E':
                    switch (command[2]) {
                        case 'd':
                        case 'D':
                            Matched("@edit");
                            do_edit(descr, player, arg1);
                            break;
                        case 'n':
                        case 'N':
                            switch (command[3]) {
                                case 'c':
                                case 'C':
                                    Matched("@encoding");
                                    do_encoding(descr, player, arg1);
                                    break;
                                case 't':
                                case 'T':
                                    Matched("@entrances");
                                    do_entrances(descr, player, arg1, arg2);
                                    break;
                                default:
                                    goto bad;
                            }
                            break;
                        case 'x':
                        case 'X':
                            Matched("@examine");
                            sane_dump_object(player, arg1);
                            break;
                        default:
                            goto bad;
                    }
                    break;
                case 'f':
                case 'F':
                    /* fail, find, force, or frob */
                    switch (command[2]) {
                        case 'a':
                        case 'A':
                            Matched("@fail");
                            do_fail(descr, player, arg1, arg2);
                            break;
                        case 'i':
                        case 'I':
                            if (command[3] == 'x' || command[3] == 'X') {
                                Matched("@fixwizbits");
                                do_fixw(player, full_command);
                            } else {
                                Matched("@find");
                                do_find(player, arg1, arg2);
                            }
                            break;
                        case 'l':
                        case 'L':
                            if (command[3] == 'a' || command[3] == 'A') {
                                Matched("@flags");
                                do_flags(descr, player, full_command);
                            } else if (command[3] == 'o' || command[3] == 'O') {
                                Matched("@flock");
                                do_flock(descr, player, arg1, arg2);
                            }
                            break;
                        case 'o':
                        case 'O':
                            if (strlen(command) < 7) {
                                Matched("@force");
                                do_force(descr, player, arg1, arg2);
                            } else {
                                Matched("@force_lock");
                                do_flock(descr, player, arg1, arg2);
                            }
                            break;
                        case 'r':
                        case 'R':
                            if (string_compare(command, "@frob"))
                                goto bad;
                            do_frob(descr, player, arg1, arg2);
                            break;
                        default:
                            goto bad;
                    }
                    break;
                case 'h':
                case 'H':
                    switch (command[2]) {
                        case 'o':
                        case 'O':
                            Matched("@hostcache");
                            do_hostcache(player, arg1);
                            break;
                        default:
                            Matched("@htmldescribe");
                            do_htmldescribe(descr, player, arg1, arg2);
                            break;
                    }
                    break;
                case 'i':
                case 'I':
                    switch (command[2]) {
                        case 'h':
                        case 'H':
                            Matched("@ihtmldescribe");
                            do_ihtmldescribe(descr, player, arg1, arg2);
                            break;
                        case 'a':
                        case 'A':
                            Matched("@iansidescribe");
                            do_iansidescribe(descr, player, arg1, arg2);
                            break;
                        default:
                            Matched("@idescribe");
                            do_idescribe(descr, player, arg1, arg2);
                            break;
                    }
                    break;
                case 'k':
                case 'K':
                    Matched("@kill");
                    do_dequeue(descr, player, arg1);
                    break;
                case 'l':
                case 'L':
                    /* lock or link */
                    switch (command[2]) {
                        case 'i':
                        case 'I':
                            switch (command[3]) {
                                case 'n':
                                case 'N':
                                    Matched("@link");
                                    do_link(descr, player, arg1, arg2);
                                    break;
                                case 's':
                                case 'S':
                                    Matched("@list");
                                    match_and_list(descr, player, arg1, arg2,
                                                   1);
                                    break;
                                default:
                                    goto bad;
                            }
                            break;
                        case 'o':
                        case 'O':
                            Matched("@lock");
                            do_lock(descr, player, arg1, arg2);
                            break;
                        default:
                            goto bad;
                    }
                    break;
                case 'm':
                case 'M':
                    switch (command[2]) {
#ifdef MCP_SUPPORT
                        case 'c':
                        case 'C':
                            switch (command[4]) {
                                case 'e':
                                case 'E':
                                    Matched("@mcpedit");
                                    (void) do_mcpedit(descr, player, arg1);
                                    break;
                                case 'p':
                                case 'P':
                                    Matched("@mcpprogram");
                                    (void) do_mcpprogram(descr, player, arg1);
                                    break;
                                default:
                                    goto bad;
                            }
							break;
#endif
                        case 'e':
                        case 'E':
                            Matched("@memory");
                            do_memory(player);
                            break;
                        case 'p':
                        case 'P':
                            Matched("@mpitops");
                            do_mpi_topprofs(player, arg1);
                            break;
                        case 'u':
                        case 'U':
                            Matched("@muftops");
                            do_muf_topprofs(player, arg1);
                            break;
						case 'f':
						case 'F':
							Matched("@mfuncs");
							do_muf_funcprofs(player, arg1);
							break;
#ifdef MODULAR_SUPPORT
						case 'o':
						case 'O':
							switch (command[4]) {
								case 'l':
								case 'L':
									Matched("@modload");
									do_modload(player, full_command);
									break;
								case 's':
								case 'S':
									Matched("@modinfo");
									do_modinfo(player, full_command);
								    break;
								case 'u':
								case 'U':
									Matched("@modunload");
									do_modunload(player, full_command);
									break;
								default:
                                    goto bad;
						   }
						   break;
#endif
                        default:
                            goto bad;
                    }
                    break;
                case 'n':
                case 'N':
                    /* @name or @newpassword */
                    switch (command[2]) {
                        case 'a':
                        case 'A':
                            Matched("@name");
                            do_name(descr, player, arg1, arg2);
                            break;
                        case 'e':
                        case 'E':
                            if (string_compare(command, "@newpassword"))
                                goto bad;
                            do_newpassword(player, arg1, arg2);
                            break;
                        default:
                            goto bad;
                    }
                    break;
                case 'o':
                case 'O':
                    switch (command[2]) {
                        case 'd':
                        case 'D':
                            Matched("@odrop");
                            do_odrop(descr, player, arg1, arg2);
                            break;
                        case 'e':
                        case 'E':
                            Matched("@oecho");
                            do_oecho(descr, player, arg1, arg2);
                            break;
                        case 'f':
                        case 'F':
                            Matched("@ofail");
                            do_ofail(descr, player, arg1, arg2);
                            break;
                        case 'p':
                        case 'P':
                            Matched("@open");
                            do_open(descr, player, arg1, arg2);
                            break;
                        case 's':
                        case 'S':
                            Matched("@osuccess");
                            do_osuccess(descr, player, arg1, arg2);
                            break;
                        case 'w':
                        case 'W':
                            Matched("@owned");
                            do_owned(player, arg1, arg2);
                            break;
                        default:
                            goto bad;
                    }
                    break;
                case 'p':
                case 'P':
                    switch (command[2]) {
                        case 'a':
                        case 'A':
                            Matched("@password");
                            do_password(player, arg1, arg2);
                            break;
                        case 'c':
                        case 'C':
                            Matched("@pcreate");
                            do_pcreate(player, arg1, arg2);
                            break;
                        case 'e':
                        case 'E':
                            Matched("@pecho");
                            do_pecho(descr, player, arg1, arg2);
                            break;
                        case 'o':
                        case 'O':
                            Matched("@powers");
                            do_powers(descr, player, arg1, arg2);
                            break;
                        case 'r':
                        case 'R':
                            if (string_prefix("@program", command)) {
                                do_prog(descr, player, arg1);
                                break;
                            } else if (string_prefix("@proginfo", command)) {
                                do_proginfo(player, arg1);
                                break;
                            } else {
                                Matched("@propset");
                                do_propset(descr, player, arg1, arg2);
                                break;
                            }
                        case 's':
                        case 'S':
                            Matched("@ps");
                            list_events(player);
                            break;
                        case 'u':
                        case 'U':
                            Matched("@purge");
                            do_purge(descr, player, arg1, arg2);
                            break;
                        default:
                            goto bad;
                    }
                    break;
                case 'r':
                case 'R':
                    switch (command[3]) {
                        case 'c':
                        case 'C':
                            Matched("@recycle");
                            do_recycle(descr, player, arg1);
                            break;
#ifdef PCRE_SUPPORT
                        case 'g':
                        case 'G':
                            Matched("@regexcache");
                            show_re_cache(player);
                            break;
#endif /* PCRE_SUPPORT */
                        case 'l':
                        case 'L':
                            Matched("@relink");
                            do_relink(descr, player, arg1, arg2);
                            break;
                        case 's':
                        case 'S':
                            if (!string_compare(command, "@restart")) {
                                do_restart(player, arg1, arg2);
                            } else if (!string_compare(command, "@restrict")) {
                                do_restrict(player, arg1);
                            } else {
                                goto bad;
                            }
                            break;
                        default:
                            goto bad;
                    }
                    break;
                case 's':
                case 'S':
                    /* set, shutdown, success */
                    switch (command[2]) {
                        case 'a':
                        case 'A':
                            if (!string_compare(command, "@sanity")) {
                                sanity(player);
                            } else if (!string_compare(command, "@sanchange")) {
                                sanechange(player, full_command);
                            } else if (!string_compare(command, "@sanfix")) {
                                sanfix(player);
                            } else {
                                goto bad;
                            }
                            break;
                        case 'e':
                        case 'E':
                            Matched("@set");
                            do_set(descr, player, arg1, arg2);
                            break;
                        case 'h':
                        case 'H':
                            if (!string_compare(command, "@shout")) {
                                do_wall(player, full_command);
                                break;
                            }
                            if (string_compare(command, "@shutdown"))
                                goto bad;
                            do_shutdown(player, arg1, arg2);
                            break;
                        case 'q':
                        case 'Q':
                            goto bad;
                            break;
                        case 't':
                        case 'T':
                            Matched("@stats");
                            do_stats(player, arg1);
                            break;
                        case 'u':
                        case 'U':
                            Matched("@success");
                            do_success(descr, player, arg1, arg2);
                            break;
                        case 'w':
                        case 'W':
                            Matched("@sweep");
                            do_sweep(descr, player, arg1);
                            break;
                        default:
                            goto bad;
                    }
                    break;
                case 't':
                case 'T':
                    switch (command[2]) {
                        case 'e':
                        case 'E':
                            Matched("@teleport");
                            do_teleport(descr, player, arg1, arg2);
                            break;
                        case 'o':
                        case 'O':
                            if (!string_compare(command, "@toad"))
                                do_frob(descr, player, arg1, arg2);
                            else if (!string_compare(command, "@tops"))
                                do_all_topprofs(player, arg1);
                            else
                                goto bad;
                            break;
                        case 'r':
                        case 'R':
                            Matched("@trace");
                            do_trace(descr, player, arg1, atoi(arg2));
                            break;
                        case 'u':
                        case 'U':
                            Matched("@tune");
                            do_tune(player, arg1, arg2);
                            break;
                        default:
                            goto bad;
                    }
                    break;
                case 'u':
                case 'U':
                    switch (command[2]) {
                        case 'N':
                        case 'n':
                            if (string_prefix(command, "@unlink")) {
                                do_unlink(descr, player, arg1);
                            } else if (string_prefix(command, "@unlock")) {
                                do_unlock(descr, player, arg1);
                            } else if (string_prefix(command, "@uncompile")) {
                                do_uncompile(player);
                            } else {
                                goto bad;
                            }
                            break;

#ifndef NO_USAGE_COMMAND
                        case 'S':
                        case 's':
                            Matched("@usage");
                            do_usage(player);
                            break;
#endif

                        default:
                            goto bad;
                            break;
                    }
                    break;
                case 'v':
                case 'V':
                    Matched("@version");
                    anotify_nolisten2(player, version_line1);
                    anotify_nolisten2(player, version_line2);
                    break;
                case 'w':
                case 'W':
                    if (string_compare(command, "@wall"))
                        goto bad;
                    do_wall(player, full_command);
                    break;
                default:
                    goto bad;
            }
            break;
        case '&':
            do_mush_set(descr, player, arg1, arg2, command);
            break;
#ifdef MCCP_ENABLED
        case 'c':
        case 'C':
            Matched("compress");
            do_compress(player, descr, arg1);
            break;
#endif /* MCCP_ENABLED */
        case 'd':
        case 'D':
            switch (command[1]) {
                case 'b':
                case 'B':
                    Matched("dboot");
                    do_dboot(player, arg1);
                    break;
                case 'i':
                case 'I':
                    Matched("dinfo");
                    do_dinfo(player, arg1);
                    break;
                case 'r':
                case 'R':
                    Matched("drop");
                    do_drop(descr, player, arg1, arg2);
                    break;
                case 'w':
                case 'W':
                    Matched("dwall");
                    do_dwall(player, arg1, arg2);
                    break;
                default:
                    goto bad;
            }
            break;
        case 'e':
        case 'E':
            switch (command[1]) {
                case 'm':
                case 'M':
                    Matched("emote");
                    do_pose(descr, player, full_command);
                    break;
                case 'x':
                case 'X':
                case '\0':
                    Matched("examine");
                    do_examine(descr, player, arg1, arg2);
                    break;
                default:
                    goto bad;
            }
            break;
        case 'g':
        case 'G':
            /* get, give, go, or gripe */
            switch (command[1]) {
                case 'e':
                case 'E':
                    Matched("get");
                    do_get(descr, player, arg1, arg2);
                    break;
                case 'i':
                case 'I':
                    Matched("give");
                    do_give(descr, player, arg1, atoi(arg2));
                    break;
                case 'o':
                case 'O':
                    Matched("goto");
                    do_move(descr, player, arg1, 0);
                    break;
                case 'r':
                case 'R':
                    if (command[2] == 'i' || command[2] == 'I') {
                        if (string_compare(command, "gripe"))
                            goto bad;
                        do_gripe(player, full_command);
                        break;
                    }
                default:
                    goto bad;
            }
            break;
        case 'h':
        case 'H':
            switch (command[1]) {
                case 'a':
                case 'A':
                    Matched("hand");
                    do_drop(descr, player, arg1, arg2);
                    break;
                case 'e':
                case 'E':
                    Matched("help");
                    do_help(player, arg1, arg2);
                    break;
                default:
                    goto bad;
            }
            break;
        case 'i':
        case 'I':
            if (string_compare(command, "info")) {
                Matched("inventory");
                do_inventory(player);
            } else {
                do_info(player, arg1, arg2);
            }
            break;
        case 'l':
        case 'L':
            if (string_prefix("look", command)) {
                do_look_at(descr, player, arg1, arg2);
                break;
            } else {
                Matched("leave");
                do_leave(descr, player);
                break;
            }
        case 'm':
        case 'M':
            if (string_prefix(command, "move")) {
                do_move(descr, player, arg1, 0);
                break;
            } else if (!string_compare(command, "motd")) {
                do_motd(player, full_command);
                break;
            } else if (!string_compare(command, "mpi")) {
                do_mpihelp(player, arg1, arg2);
                break;
            } else {
                if (string_compare(command, "man"))
                    goto bad;
                /* Sure, using full_command here means that players won't be   */
                /*  able to specifiy segments by line number, but I doubt many */
                /*  do that anyhow.  Besides, it only works if the manual is   */
                /*  broken into seperate files, and doing it this way lets =   */
                /*  work.  -Hinoserm */
                do_man(player, full_command, arg2);
            }
            break;
        case 'n':
        case 'N':
            /* news */
            Matched("news");
            do_news(player, arg1, arg2);
            break;
        case 'p':
        case 'P':
            switch (command[1]) {
                case 'a':
                case 'A':
                case '\0':
                    Matched("page");
                    do_page(descr, player, arg1, arg2);
                    break;
                case 'o':
                case 'O':
                    Matched("pose");
                    do_pose(descr, player, full_command);
                    break;
                case 'u':
                case 'U':
                    Matched("put");
                    do_drop(descr, player, arg1, arg2);
                    break;
                default:
                    goto bad;
            }
            break;
        case 'r':
        case 'R':
            switch (command[1]) {
                case 'e':
                case 'E':
                    Matched("read"); /* undocumented alias for look */
                    do_look_at(descr, player, arg1, arg2);
                    break;
                default:
                    goto bad;
            }
            break;
        case 's':
        case 'S':
            /* say, "score" */
            switch (command[1]) {
                case 'a':
                case 'A':
                    Matched("say");
                    do_say(descr, player, full_command);
                    break;
                case 'c':
                case 'C':
                case '\0':
                    if (command[1] && (command[2] == 'a' || command[2] == 'A')) {
                        Matched("scan");
                        do_sweep(descr, player, arg1);
                    } else {
                        Matched("score");
                        do_score(player);
                    }
                    break;
                default:
                    goto bad;
            }
            break;
        case 't':
        case 'T':
            switch (command[1]) {
                case 'a':
                case 'A':
                    Matched("take");
                    do_get(descr, player, arg1, arg2);
                    break;
                case 'h':
                case 'H':
                    Matched("throw");
                    do_drop(descr, player, arg1, arg2);
                    break;
                default:
                    goto bad;
            }
            break;
        case 'w':
        case 'W':
            switch (command[1]) {
                case 'c':
                case 'C':
                    Matched("wc");
                    do_wizchat(player, full_command);
                    break;
                case 'i':
                case 'I':
                    Matched("wizchat");
                    do_wizchat(player, arg1);
                    break;
                case 'h':
                case 'H':
                case '\0':
                    Matched("whisper");
                    do_whisper(descr, player, arg1, arg2);
                    break;
                default:
                    goto bad;
            }
            break;
        default:
          bad:
        {
            /* Alynna's HUH propqueue support! 
               This crazy crap written 20031115 */
            /* FIX: Only do output when huh_mesg is not null */
            if (*tp_huh_mesg)
                anotify_fmt(player, CINFO "%s", tp_huh_mesg);

            /* Do the propqueue, with the HUH args on the stack */
            sprintf(zbuf, "HUH:%s %s", command, full_command);
            propqueue(descr, player, DBFETCH(player)->location, player, 0, -1,
                      "@huh", zbuf, 1, 1);

            /* Regular logging */
            if (tp_log_failed_commands
                && !controls(player, DBFETCH(player)->location)) {
                log_status("HUH from %s(%d) in %s(%d)[%s]: %s %s\n",
                           NAME(player), player,
                           NAME(DBFETCH(player)->location),
                           DBFETCH(player)->location,
                           NAME(OWNER(DBFETCH(player)->location)), command,
                           full_command);
            }
        }
            break;
    }

}

/* This is the command prop support in ProtoMUCK. It allows certain
 * commands to be done without having to make literal exits to do
 * them. There's support for command/ propdirs and logincommand/ 
 * propdirs, the latter only being on #0.
 *
 * Alynna - this has been expanded to support rooms and exits. 
 */

int
prop_command(int descr, dbref player, const char *command, const char *arg,
             const char *type, int mt)
{
    PropPtr ptr;

    dbref progRef = AMBIGUOUS;

    dbref where = player;

    char propName[BUFFER_LEN];

    const char *workBuf = NULL;

    if (player == NOTHING)      /* For handling logincommand props. */
        where = (dbref) 0;

    sprintf(propName, "%s%c%s", type, PROPDIR_DELIMITER, command);
    strcpy(match_cmdname, command);
    strcpy(match_args, arg);
    ptr = envprop_cmds(&where, propName, Prop_Hidden(propName));
    if (!ptr)
        return 0;
#ifdef DISKBASE
    propfetch(where, ptr);
#endif

    switch (PropType(ptr)) {
        case PROP_STRTYP:
            workBuf = PropDataUNCStr(ptr);
            break;
        case PROP_REFTYP:
            progRef = PropDataRef(ptr);
            break;
        default:
            return 0;
            break;
    }
    if (workBuf && *workBuf) {
        if (*workBuf == '&')
            progRef = AMBIGUOUS;
        else if (*workBuf == '#' && number(workBuf + 1))
            progRef = (dbref) atoi(++workBuf);
        else if (*workBuf == '$')
            progRef = find_registered_obj(where, workBuf);
        else {
            if (player < 1)
                notify_descriptor(descr, workBuf);
            else {
                notify(player, workBuf);
            }
            return 1;
        }
    } else if (progRef == AMBIGUOUS || progRef == NOTHING) {
        if (player < 1)
            notify_descriptor(descr,
                              "Invalid program call from a command prop.");
        else
            anotify_nolisten2(player, CINFO
                              "Invalid program call from a command prop.");
        return 1;
    }
    if (progRef == AMBIGUOUS) {
        char cbuf[BUFFER_LEN];

        int ival;

        ival = (mt == 0) ? MPI_ISPUBLIC : MPI_ISPRIVATE;
        if (player < 1)
            do_parse_mesg(descr, (dbref) -1, (dbref) 0, workBuf + 1,
                          match_cmdname, cbuf, ival);
        else
            do_parse_mesg(descr, player, where, workBuf + 1, match_cmdname,
                          cbuf, ival);

        if (*cbuf) {
            if (player < 1) {
                notify_descriptor(descr, cbuf);
            } else {
                if (mt) {
                    notify_nolisten(player, cbuf, 1);
                } else {
                    char bbuf[BUFFER_LEN];

                    dbref plyr;

                    sprintf(bbuf, ">> %.4000s",
                            pronoun_substitute(descr, player, cbuf));
                    plyr = DBFETCH(where)->contents;
                    while (plyr != NOTHING) {
                        if (Typeof(plyr) == TYPE_PLAYER && plyr != player)
                            notify_nolisten(plyr, bbuf, 0);
                        plyr = DBFETCH(plyr)->next;
                    }
                }
            }
        }
        return 1;
    } else {
        if (progRef < 0 || progRef >= db_top) {
            if (player < 1)
                notify_descriptor(descr,
                                  "Invalid program call from a command prop.");
            else
                anotify_nolisten2(player,
                                  CINFO
                                  "Invalid program call from a command prop.");
            return 1;

        } else if (Typeof(progRef) == TYPE_ROOM) {
            if (!OkObj(player) || !OkObj(progRef)) {
                notify_descriptor(descr,
                                  "Invalid program call from a command prop.");
                return 1;
            } else {
                if (Wizard(player) || Mage(where) || controls(player, progRef)
                    || (FLAGS(progRef) && JUMP_OK))
                    enter_room(descr, player, progRef, where);
                else
                    notify_descriptor(descr, RED "Permission denied.");

            }
            return 1;
        } else if (Typeof(progRef) == TYPE_EXIT) {
            if (!OkObj(player) || !OkObj(progRef)) {
                notify_descriptor(descr,
                                  "Invalid program call from a command prop.");
                return 1;
            } else {
                if (OkObj((DBFETCH(progRef)->sp.exit.dest)[0])
                    && Typeof((DBFETCH(progRef)->sp.exit.dest)[0]) == TYPE_ROOM) {
                    if (Wizard(player) || Mage(where)
                        || controls(player, progRef) || (FLAGS(progRef)
                                                         && JUMP_OK))
                        enter_room(descr, player,
                                   (int) (DBFETCH(progRef)->sp.exit.dest)[0],
                                   progRef);
                    else
                        notify_descriptor(descr, RED "Permission denied.");
                } else {
                    notify_descriptor(descr,
                                      "Exits in command props can only enter rooms.");
                    return 1;
                }
                return 1;
            }
/* Alynna - Jump to player support -- not refined enough to enable 
        } else if (Typeof(progRef) == TYPE_PLAYER) {
            if (!OkObj(player) || !OkObj(progRef)) {
                notify_descriptor(descr,
                                  "Invalid program call from a command prop.");
		return 1;
            } else {
            if (OkObj(DBFETCH(progRef)->location) && 
		(Typeof(DBFETCH(progRef)->location == TYPE_ROOM) || Typeof(DBFETCH(progRef)->location) == TYPE_THING)) {
    	        if (Wizard(player) || Mage(where) || controls(player,progRef) || 
                   ((FLAGS(progRef) && JUMP_OK) || (FLAGS(DBFETCH(progRef)->location) && JUMP_OK)) )
		    enter_room(descr, player, (int) (DBFETCH(progRef)->location), progRef);
        	else
                    notify_descriptor(descr, RED "Permission denied.");
                } else {
                notify_descriptor(descr,
                                   "Exits in command props can only enter rooms.");
		return 1;
    	        }
	    return 1;
            }
*/
        } else if (Typeof(progRef) == TYPE_THING) {
            if (!OkObj(player) || !OkObj(progRef)) {
                notify_descriptor(descr,
                                  "Invalid program call from a command prop.");
                return 1;
            } else {
                if (OkObj(DBFETCH(progRef)->location) &&
                    ((Typeof(DBFETCH(progRef)->location) == TYPE_ROOM)
                     || Typeof(DBFETCH(progRef)->location) == TYPE_THING)) {
                    if (Wizard(player) || Mage(where)
                        || (controls(player, progRef)
                            && controls(player, DBFETCH(progRef)->location))
                        || (((FLAGS(progRef) & JUMP_OK)
                             || (FLAGS(progRef) & VEHICLE))
                            && ((FLAGS(DBFETCH(progRef)->location) & JUMP_OK)
                                && !(FLAGS(DBFETCH(progRef)->location) &
                                     VEHICLE))))
                        enter_room(descr, player,
                                   (int) (DBFETCH(progRef)->location), progRef);
                    else
                        notify_descriptor(descr, RED "Permission denied.");
                } else {
                    notify_descriptor(descr,
                                      "Cowardly refusing to move you into that.");
                    return 1;
                }
                return 1;
            }
        } else if (Typeof(progRef) != TYPE_PROGRAM) {
            if (player < 1)
                notify_descriptor(descr,
                                  "Invalid program call from a command prop.");
            else
                anotify_nolisten2(player,
                                  CINFO
                                  "Invalid program call from a command prop.");
            return 1;
        } else {
            struct frame *tmpfr;

            tmpfr =
                interp(descr, player,
                       (OkObj(player)) ? DBFETCH(player)->location : -1,
                       progRef, where, FOREGROUND, STD_HARDUID, 0);
            if (tmpfr) {
                interp_loop(player, progRef, tmpfr, 0);
            }
            return 1;
        }
    }
}

#undef Matched
