/*
 * $Header: /export/home/davin/tmp/protocvs/proto1.0/src/inc/config.h,v 1.1.1.1 2000-09-20 18:26:34 akari Exp $
 * $Log: not supported by cvs2svn $
 *
 * Revision 1.3  2000/06/27 22:32:17  moose
 * Edited for ProtoMUCK
 *
 * Revision 1.2  1996/09/19 05:45:35  jtraub
 * removed define for in-server HackAndSlash code
 *
 *
 * config.h
 *
 * Tunable parameters -- Edit to you heart's content 
 *
 */
#include "copyright.h"

/************************************************************************
   Administrative Options 

   Various things that affect how the muck operates and what privs
 are compiled in.
 ************************************************************************/

/* Detaches the process as a daemon so that it don't cause problems
 * keeping a terminal line open and such. Logs normal output to a file
 * and writes out a protomuck.pid file 
 */
#undef DETACH

/* Makes God (#1) immune to @force, @newpassword, and being set !Wizard.  
 */
#define GOD_PRIV

/* Use to compress string data (recomended)
 */
#define COMPRESS

/* To use a simple disk basing scheme where properties aren't loaded
 * from the input file until they are needed, define this. 
 */
#undef DISKBASE

/* To make the server save using fast delta dumps that only write out the
 * changed objects, except when @dump or @shutdown are used, or when too
 * many deltas have already been saved to disk, #define this. 
 */
#undef DELTADUMPS

/*
 * Ports where tinymuck lives -- Note: If you use a port lower than
 * 1024, then the program *must* run suid to root!
 * Port 4201 is a port with historical significance, as the original
 * TinyMUD Classic ran on that port.  It was the office number of James
 * Aspnes, who wrote TinyMUD from which TinyMUCK eventually was derived.
 *
 * Please note: ProtoMUCK will use up to 4 ports:
 *
 * The internal port (don't worry about this, just make sure it's a free 
 *  port.)
 *
 * The main port (set it here or in config or at startup)
 *
 * The HTTPD port (set it here or in config or at startup)
 *
 * The Pueblo port (set it here or in config or at startup)
 *
 */

#define INTERNAL_PORT 3006
#define TINYPORT 3005
#define WWWPORT 3004
#define PUEBLOPORT 3007
/*
 * Some systems can hang for up to 30 seconds while trying to resolve
 * hostnames.  Define this to use a non-blocking second process to resolve
 * hostnames for you.  NOTE:  You need to compile the 'resolver' program
 * (make resolver) and put it in the directory that the protomuck program is
 * run from.
 */
#undef SPAWN_HOST_RESOLVER

/*
 * Set this to be the file that you wish the ProtoMUCK file prims to
 * be set to save to by default. "files/" is the normal directory
 * chosen, though "../public_html" might be another favorite. Setting
 * this to a directory that does not exist will disable file prims 
 * except for use with shortcuts.
 */
#define FILE_PRIMS_DIRECTORY "files/"

/* Debugging info for database loading */
#undef VERBOSELOAD

/* A little extra debugging info for read()/write() on process input/output */
/* I put this in when I couldn't figure out why sockets were failing from */
/* a bad net connection for the server. */
#undef DEBUGPROCESS

/* Define MORTWHO to make it so wizards need to type WHO! to see hosts */
/* When undefined, WHO! will show the mortal WHO+@doing (without going Q) */
#define MORTWHO

/* Define to compile in RWHO support */
#undef RWHO

/* Define to compile in MPI support */
#define MPI

/* Define to require that a room a Guest can go in have the Guest flag */
/* If undefined, Guests will be banned from rooms with a Guest flag */
#define G_NEEDFLAG

/* Define to compile in HTTPD server WWW page support */
#define HTTPD

/* Define this to do proper delayed link references for HTTPD */
/* The _/www:http://... links won't work on some clients without it. */
#define HTTPDELAY

/* Define this to compile the old FB-style PARSEPROP in.
 * If undefined, PARSEPROP is an internal alias to Proto's PARSEMPI
 */
#define OLDPARSE

/* Define this for old, FB-style control checks (e.g. W2 and higher
 * control everything, instead of you-control-everything-with-a
 * Lower-level-than-you.
 */
#define FB_CONTROLS

/* Define this to compile in support for the older, less secure _connect
 * _disconnect, _arrive, _depart, and such triggers as well as Proto's
 * ~arrive, ~depart, etc.
 */
#define ALLOW_OLD_TRIGGERS

/* Define this to keep backups of past dumps instead of deleting them.
 *
 */
#define KEEPDUMPS

/* Define this to set which server to use for e-mail registration.
 */
#define MAILSERVER "mail.eskimo.com"

/* Needed for a dummy load at startup.  Define to a small MIDI
 * somewhere which has the correct MIME type.  Preferably on
 * your local server.
 */
#define DUMMYMIDI "http://adobe.chaco.com/~jeremy/ami/smoon.mid"

/* This is a shortcut.  Define this to add ALL compatiblity options.
 * To ensure TinyMUCKfb compatiblity.
 */
#undef TINYMUCKFB_COMPAT


/* All compiler directives to mod for FB support go in here. */
#ifdef TINYMUCKFB_COMPAT
   #define OLDPARSE
   #define ALLOW_OLD_TRIGGERS
   #define FB_CONTROLS
#endif

/************************************************************************
   Game Options

   These are the ones players will notice. 
 ************************************************************************/

/* Set this to your mark for shouts, dumps, etc.  Also change @tunes */

#define MARK "[!] "

/* Make the `examine' command display full names for types and flags */
#define VERBOSE_EXAMINE

/* limit on player name length */
#define PLAYER_NAME_LIMIT 16

/************************************************************************
   Various Messages 
 
   Printed from the server at times, esp. during login.
 ************************************************************************/

#define NOBUILD_MESG	"Building is currently disabled."
#define NOGUEST_MESG	"This command is unavailable to guests."

#define NOBBIT_MESG	"You're not a builder."
#define NOMBIT_MESG	"You're not a programmer."
#define NOEDIT_MESG	"That is already being edited."

#define NOPERM_MESG	"Permission denied."
#define HUH_MESG	"Need help?  Just type \"help\"."

#define LEAVE_MESG	LEAVE_MESSAGE
#define IDLE_MESG	IDLEBOOT_MESSAGE
#define WARN_MESG	PLAYERMAX_WARNMESG
#define BOOT_MESG	PLAYERMAX_BOOTMESG
#define DBRO_MESG	"The muck is currently read-only."

/*
 * Welcome message if you don't have a welcome.txt
 */
#define DEFAULT_WELCOME_MESSAGE "\r\nWelcome!\r\n"

/*
 * Message if someone trys the create command and your system is
 * setup for registration.
 */
#define CFG_REG_MSG2 "Your character isn't allowed to connect from this site.\r\nSend email to %s to be added to this site.\r\n"

/*
 * Goodbye message.
 */
#define LEAVE_MESSAGE "May your heart always guide your actions."


/*
 * Error messeges spewed by the help system.
 */
#define NO_NEWS_MSG "That topic does not exist.  Type 'news topics' to list the news topics available."
#define NO_HELP_MSG "That topic does not exist.  Type 'help index' to list the help topics available."
#define NO_MAN_MSG "That topic does not exist.  Type 'man' to list the MUF topics available."
#define NO_INFO_MSG "That file does not exist.  Type 'info' to get a list of the info files available."

/************************************************************************
   File locations
 
   Where the system looks for its datafiles.
 ************************************************************************/
#define WELC_FILE "data/welcome.txt"     /* For the opening screen	*/
#define WELC_HTML "data/welcome.html"    /* For Pueblo users            */
#define BARD_FILE "data/welcome/fbi.txt" /* For naughty sites		*/
#define MOTD_FILE "data/motd.txt"        /* For the message of the day	*/

#define HELP_FILE "data/help.txt"    /* For the 'help' command      */
#define HELP_DIR  "data/help"        /* For 'help' subtopic files   */
#define NEWS_FILE "data/news.txt"    /* For the 'news' command      */
#define NEWS_DIR  "data/news"        /* For 'news' subtopic files   */
#define MAN_FILE  "data/man.txt"     /* For the 'man' command       */
#define MAN_DIR   "data/man"         /* For 'man' subtopic files    */
#define MPI_FILE  "data/mpihelp.txt" /* For the 'mpi' command       */
#define MPI_DIR   "data/mpihelp"     /* For 'mpi' subtopic files    */
#define INFO_DIR  "data/info/"
#define EDITOR_HELP_FILE "data/edit-help.txt" /* editor help file   */
#define SYSPARM_HELP_FILE "data/sysparms.txt" /* @tune parms help file */

#define DELTAFILE_NAME "data/deltas-file"  /* The file for deltas */
#define PARMFILE_NAME "data/parmfile.cfg"  /* The file for config parms */
#define WORDLIST_FILE "data/wordlist.txt"  /* File for compression dict. */

#define LOG_GRIPE   "logs/gripes"       /* Gripes Log */
#define LOG_HOPPER  "logs/hopper"	/* Registration hopper */
#define LOG_STATUS  "logs/status"       /* System errors and stats */
#define LOG_CONC    "logs/concentrator" /* Concentrator errors and stats */
#define LOG_MUF     "logs/muf-errors"   /* Muf compiler errors and warnings. */
#define COMMAND_LOG "logs/commands"     /* Player commands */
#define PROGRAM_LOG "logs/programs"     /* text of changed programs */
#define CONNECT_LOG "logs/connects"	/* text of connection info */

#define MACRO_FILE  "muf/macros"
#define PID_FILE    "protomuck.pid"      /* Write the server pid to ... */

#define RESOLVER_PID_FILE "hostfind.pid"   /* Write the resolver pid to ... */

#ifdef LOCKOUT
# define LOCKOUT_FILE "data/lockout.txt"
#endif /* LOCKOUT */

#ifdef DETACH
# define LOG_FILE "logs/protomuck"          /* Log stdout to ... */      
# define LOG_ERR_FILE "logs/protomuck.err"  /* Log stderr to ... */      
#endif /* DETACH */

/************************************************************************
  System Dependency Defines. 

  You probably will not have to monkey with this unless the muck fails
 to compile for some reason.
 ************************************************************************/

/* If you get problems compiling strftime.c, define this. */
#define USE_STRFTIME

/* Use this only if your realloc does not allocate in powers of 2
 * (if your realloc is clever, this option will cause you to waste space).
 * SunOS requires DB_DOUBLING.  ULTRIX doesn't.  */
#define  DB_DOUBLING

/* Prevent Many Fine Cores. */
#undef NOCOREDUMP

/* if do_usage() in wiz.c gives you problems compiling, define this */
#undef NO_USAGE_COMMAND

/* if do_memory() in wiz.c gives you problems compiling, define this */
#define NO_MEMORY_COMMAND

/* This gives some debug malloc profiling, but also eats some overhead,
   so only define if your using it. */
#undef MALLOC_PROFILING
#undef CRT_DEBUG_ALSO

/************************************************************************/
/************************************************************************/
/*    FOR INTERNAL USE ONLY.  DON'T CHANGE ANYTHING PAST THIS POINT.    */
/************************************************************************/
/************************************************************************/

#ifdef SANITY
#undef MALLOC_PROFILING
#endif

/*
 * Very general defines 
 */
#define TRUE  1
#define FALSE 0

/*
 * Memory/malloc stuff.
 */
#undef LOG_PROPS
#undef LOG_DISKBASE
#undef DEBUGDBDIRTY
#define FLUSHCHANGED /* outdated, needs to be removed from the source. */

/*
 * Include all the good standard headers here.
 */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

#ifdef WIN32
# include "winconf.h"
# include "process.h"
# include <winsock.h>
# define  strcasecmp(x,y) stricmp((x),(y))
# define  strncasecmp(x,y,z) strnicmp((x),(y),(z))
# define  waitpid(x,y,z) cwait((y),(x),_WAIT_CHILD)
# define  ioctl(x,y,z) ioctlsocket((x),(y),(z))
# define  EWOULDBLOCK WSAEWOULDBLOCK
# define  EINTR WSAEWOULDBLOCK
# define  getdtablesize() (FD_SETSIZE)
# define  readsocket(x,y,z) recv((x),(y),(z),0)
# define  writesocket(x,y,z) send((x),(y),(z),0)
# define  gettimeofday(x,y) (((x)->tv_sec = time(NULL)) + ((x)->tv_usec = 0))
# define  errnosocket WSAGetLastError()
#else
# include "autoconf.h"
# define  readsocket(x,y,z) read((x),(y),(z))
# define  writesocket(x,y,z) write((x),(y),(z))
# define  closesocket(x) close(x)
# define  errnosocket    errno
#endif

#ifdef STDC_HEADERS
# include <stdlib.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

/*
 * Which set of memory commands do we have here...
 */
#if defined(STDC_HEADERS) || defined(HAVE_STRING_H)
# include <string.h>
/* An ANSI string.h and pre-ANSI memory.h might conflict.  */
# if !defined(STDC_HEADERS) && defined(HAVE_MEMORY_H)
#  include <memory.h>
# endif /* not STDC_HEADERS and HAVE_MEMORY_H */
/* Map BSD funcs to ANSI ones. */
# define index		strchr
# define rindex		strrchr
# define bcopy(s, d, n) memcpy ((d), (s), (n))
# define bcmp(s1, s2, n) memcmp ((s1), (s2), (n))
# define bzero(s, n) memset ((s), 0, (n))
#else /* not STDC_HEADERS and not HAVE_STRING_H */
# include <strings.h>
/* Map ANSI funcs to BSD ones. */
# define strchr		index
# define strrchr	rindex
# define memcpy(d, s, n) bcopy((s), (d), (n))
# define memcmp(s1, s2, n) bcmp((s1), (s2), (n))
/* no real way to map memset to bzero, unfortunatly. */
#endif /* not STDC_HEADERS and not HAVE_STRING_H */

#ifdef HAVE_RANDOM
# define SRANDOM(seed)	srandom((seed))
# define RANDOM()	random()
#else
# define SRANDOM(seed)	srand((seed))
# define RANDOM()	rand()
#endif

/*
 * Time stuff.
 */
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

/*
 * Include some of the useful local headers here.
 */
#ifdef MALLOC_PROFILING
#include "crt_malloc.h"
#endif

/******************************************************************/
/* System configuration stuff... Figure out who and what we are.  */
/******************************************************************/

/*
 * Try and figure out what we are.
 * 
 * Not realy used for anything any more, probably can be scrapped,
 * will see in a version or so.
 */
#if defined(linux) || defined(__linux__) || defined(LINUX)
# define SYS_TYPE "Linux"
# define LINUX
#endif

#ifdef sgi
# define SYS_TYPE "SGI"
#endif

#ifdef sun
# define SYS_TYPE "SUN"
# define SUN_OS
# define BSD43
#endif

#ifdef ultrix
# define SYS_TYPE "ULTRIX"
# define ULTRIX
#endif

#ifdef bds4_3
# ifndef SYS_TYPE
#  define SYS_TYPE "BSD 4.3"
# endif
# define BSD43
#endif

#ifdef bds4_2
# ifndef SYS_TYPE
#  define SYS_TYPE "BSD 4.2"
# endif
#endif

#if defined(SVR3) 
# ifndef SYS_TYPE
#  define SYS_TYPE "SVR3"
# endif
#endif

#if defined(SYSTYPE_SYSV) || defined(_SYSTYPE_SYSV)
# ifndef SYS_TYPE
#  define SYS_TYPE "SVSV"
# endif
#endif

#if defined(__WINDOWS__)
# define SYS_TYPE "WIN32"
#endif

#ifndef SYS_TYPE
# define SYS_TYPE "UNKNOWN"
#endif

/******************************************************************/
/* Final line of defense for self configuration, systems we know  */
/* need special treatment.                                        */ 
/******************************************************************/



