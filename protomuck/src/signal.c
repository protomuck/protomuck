/*
 * signal.c -- Curently included into interface.c
 *
 * Seperates the signal handlers and such into a seperate file
 * for maintainability.
 *
 * Broken off from interface.c, and restructured for POSIX
 * compatible systems by Peter A. Torkelson, aka WhiteFire.
 */

/* #define _POSIX_SOURCE */             /* Solaris needs this */
#ifdef SOLARIS
#  ifndef _POSIX_SOURCE
#    define _POSIX_SOURCE               /* Solaris needs this */
#  endif
#endif

#include "config.h"
#include "interface.h"

#include <signal.h>

/*
 * SunOS can't include signal.h and sys/signal.h, stupid broken OS.
 */
#if defined(HAVE_SYS_SIGNAL_H) && !defined(SUN_OS)
# include <sys/signal.h>
#endif

#if defined(ULTRIX) || defined(_POSIX_VERSION)
#undef RETSIGTYPE
#define RETSIGTYPE void
#endif

/*
 * Function prototypes
 */
void    set_signals(void);
RETSIGTYPE sig_shutdown(int);
RETSIGTYPE bailout(int);
RETSIGTYPE sig_reap_resolver(int);

#ifdef _POSIX_VERSION
void our_signal(int signo, void (*sighandler)());
#else
# define our_signal(s,f) signal((s),(f))
#endif

/*
 * our_signal(signo, sighandler)
 *
 * signo      - Signal #, see defines in signal.h
 * sighandler - The handler function desired.
 *
 * Calls sigaction() to set a signal, if we are posix.
 */
#ifdef _POSIX_VERSION
void our_signal(int signo, void (*sighandler)())
{
    struct sigaction    act, oact;
    
    act.sa_handler = sighandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    /* Restart long system calls if a signal is caught. */
#ifdef SA_RESTART
    act.sa_flags |= SA_RESTART;
#endif

    /* Make it so */
    sigaction(signo, &act, &oact);
}
#endif /* _POSIX_VERSION */

/*
 * set_signals()
 * set_sigs_intern(bail)
 *
 * Traps a bunch of signals and reroutes them to various
 * handlers. Mostly bailout.
 *
 * If called from bailout, then reset all to default.
 *
 * Called from main() and bailout()
 */
#define SET_BAIL (bail ? SIG_DFL : bailout)
#define SET_SHUT (bail ? SIG_DFL : sig_shutdown)
#define SET_IGN  (bail ? SIG_DFL : SIG_IGN)

static void set_sigs_intern(int bail)
{
    /* we don't care about SIGPIPE, we notice it in select() and write() */
    our_signal(SIGPIPE, SET_IGN);

    /* didn't manage to lose that control tty, did we? Ignore it anyway. */
    our_signal(SIGHUP, SET_IGN);

    /* resolver's exited. Better clean up the mess our child leaves */
    our_signal(SIGCHLD, bail ? SIG_DFL : sig_reap_resolver);

#ifdef SIGTRAP
    our_signal(SIGTRAP, SET_BAIL);
#endif
#ifdef SIGIOT
    /* our_signal(SIGIOT, SET_BAIL); */ /* This is SIGABRT, want cores from it. CrT */
#endif
#ifdef SIGEMT
    our_signal(SIGEMT, SET_BAIL);
#endif
#ifdef SIGBUS
    our_signal(SIGBUS, SET_BAIL);
#endif
#ifdef SIGSYS
    our_signal(SIGSYS, SET_BAIL);
#endif
    our_signal(SIGFPE, SET_BAIL);
    our_signal(SIGSEGV, SET_BAIL);
    our_signal(SIGTERM, SET_BAIL);
#ifdef SIGXCPU
    our_signal(SIGXCPU, SET_BAIL);
#endif
#ifdef SIGXFSZ
    our_signal(SIGXFSZ, SET_BAIL);
#endif
#ifdef SIGVTALRM
    our_signal(SIGVTALRM, SET_BAIL);
#endif
    our_signal(SIGUSR1, SET_SHUT);
    our_signal(SIGUSR2, SET_SHUT);

    /* standard termination signals */
    our_signal(SIGINT, SET_SHUT);
    our_signal(SIGTERM, SET_SHUT);

    /* catch these because we might as well */
    our_signal(SIGQUIT, SET_SHUT);
}

void set_signals(void)
{
    set_sigs_intern(FALSE);
}

/*
 * Signal handlers
 */

/* Shutdown the muck */

extern int shutdown_flag;
extern int restart_flag;

RETSIGTYPE sig_shutdown(int i)
{
	shutdown_flag = 1;
	restart_flag = 0;
}

/*
 * BAIL!
 */
RETSIGTYPE bailout(int sig)
{
    char    message[128];

    /* turn off signals */
    set_sigs_intern(TRUE);
    
    sprintf(message, "BAILOUT: caught signal %d", sig);

    panic(message);
    _exit(7);

#if !defined(SYSV) && !defined(_POSIX_VERSION) && !defined(ULTRIX) && !defined(WIN32)
    return 0;
#endif
}

/*
 * Clean out Zombie Resolver Process.
 */
RETSIGTYPE sig_reap_resolver(int i)
{
#if defined(SPAWN_HOST_RESOLVER)
        extern void kill_resolver(void);
	kill_resolver();
#endif

#if !defined(SYSV) && !defined(_POSIX_VERSION) && !defined(ULTRIX) && !defined(WIN32)
    return 0;
#endif
}

