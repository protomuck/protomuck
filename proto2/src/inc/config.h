/*
 * Revision 1.4  2001/02/17 12:22:25  alynna 
 * Make this thing compile flawlessly under CYGWIN  
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
#ifndef __CONFIG_H
#define __CONFIG_H

#include "copyright.h"

/************************************************************************
   Administrative Options 

   Various things that affect how the muck operates and what privs
 are compiled in.
 ************************************************************************/

/* Alynna - Lets make something so that later on it will see the CYGWIN 
 * edits.  If this is defined, it will use CYGWIN edits.  Usually CYGWIN
 * will be detected.  If not, define me.
 */
#undef CYGWIN 

/* If compiling for OS-X, #define this and follow these instructions:
 * After running ./configure, edit Makefile and change the line that
 * reads: CC=gcc to
 *        CC=cc -no-cpp-precomp
 * Edit the restart script line that
 * reads: #limit stacksize 16384 to
 *         limit stacksize 16384
 */
#undef APPLE
  
/* Alynna - If you defined CYGWIN, set these to your timezone offset in 
 * hours.  -8 = PST, -5 = EST, 0 = GMT 
 */
#define CYGWIN_TZ -8 
#define CYGWIN_TZX "PST" 

/* Alynna - Define this for SSL support */
/* Note: Use ./configure --with-ssl */
/* Todo: Rename this to SSL_SUPPORT */
#undef USE_SSL

/* Alynna - Define this to have support for the @/ignore prop, a reflist
   on a player which will drop notifies from the players in the prop to
   the player specified.
   Note: MUF_MAX_IGNORES and MAX_IGNORES should be the same, just the MUF
   version should be in text, for the MAX_IGNORES MUF define.
 */
/* Be aware, this is slightly resource intensive. -Hinoserm */
#undef IGNORE_SUPPORT
#define MAX_IGNORES 16
#define MUF_MAX_IGNORES "16"

/* Define this to disable the colors used in many of the in-server
 * commands. Leave it undefined for traditional in-server ANSI 
 * in the MUCK's commands.
 * System colors are defined at the end of interface.h as SYS<color>
 * tags. If you would like to make it so that only certain colors
 * are used in-server, you could define this, and then go define
 * specifically which colors you want used. Otherwise, pretty much
 * all the color used in in-server commands will be disabled by
 * #defining this. 
 */
#undef NO_SYSCOLOR

/* Define this to compile with MySQL support. MySQL must be installed
 * somewhere on your system. When compiling with this option, you
 * MUST run ./configure --with-mysql in the proto/src directory
 * in order to include the necessary MySQL libraries. 
 * If the mysql headers are in the normal /user/include/mysql/
 * location, then it will find it fine. Otherwise, type:
 * ./configure --with-mysql=<path to the mysql headers>
 */
#undef SQL_SUPPORT

/* Define FILE_PRIMS to compile in a set of W4 level file-access
 * and control prims. The file prims are often a problem when
 * compiling on non-linux servers, so if you are having problems
 * compiling p_file.c, and don't intend to use file prims, you may
 * want to undef this.
 */

#define FILE_PRIMS

/* Alynna -- ACHTUNG!  PELIGRO!  DANGER!
 * DO NOT DEFINE UNDER PENALTY OF LAW.  This define is DANGEROUS.
 * Define this if you want to run Proto as root, and still have 
 * access to things like file primitives and other potential
 * compromises.  You might want to do this if you decided that
 * ProtoMUCK is a better webserver than Apache. 
 */
#undef PROTO_AS_ROOT

/* Define MUF_SOCKETS to include the MUF socket prims.
 * MUF socket support is necessary for any MUF programs
 * that involve opening connections to other servers,
 * or opening listening ports in MUF. However, this support
 * will often prevent a clean compile on some systems, so
 * if you are encountering compile time errors in p_socket.c
 * you may want to undef this.
 */

#define MUF_SOCKETS

/* Define MUF_EDIT_PRIMS to compile in a set of prims that have
 * the ability to edit MUF code or the MUF macros. This can be
 * potentially risky, as it grants the MUF code the ability to edit
 * other MUF code, so if you have doubts about its use, #undef it.
 */

#define MUF_EDIT_PRIMS

/* Define this to enable support for MCP (MUD Client Protocol). */

#define MCP_SUPPORT

/* Detaches the process as a daemon so that it don't cause problems
 * keeping a terminal line open and such. Logs normal output to a file
 * and writes out a protomuck.pid file
 */

#define DETACH

/* This is the section to turn on and off cutting edge features.
 */

#define CONTROLS_SUPPORT	/* Alynna 20030907 */

// If you have problems compiling with DETACH defined, uncomment one
// of these:

// For Linux and most POSIX
// #define USE_SID

// For SysV stuff
// #define USE_SYSVPGRP

// For BSDs
// #define USE_BSDPGRP

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
 * Please note: ProtoMUCK will use up to 3 ports:
 *
 * The main port (set it here or in restart or in parmfile.cfg  or at startup)
 *
 * The HTTPD port (Defaults to 1 below main port. Can be changed in
 *                 parmfile.cfg) 
 *
 * The Pueblo port (Defaults to 2 below main port. Can be changed in 
 *                  parmfile.cfg) 
 *
 */

#define TINYPORT      4567    /* Port that tinymuck uses for playing */

/*
 * Some systems can hang for up to 30 seconds while trying to resolve
 * hostnames.  Define this to use a non-blocking second process to resolve
 * hostnames for you.  NOTE:  You need to compile the 'resolver' program
 * (make resolver) and put it in the directory that the protomuck program is
 * run from.
 */
#define SPAWN_HOST_RESOLVER

/* If this is defined, the resolver will try to restart itself if killed. */
#define RESTART_RESOLVER 

/* Debugging info for database loading */
/* Alynna - Obsoleted, use -verboseload command line option */
/* #undef VERBOSELOAD */

/* A little extra debugging info for read()/write() on process input/output */
/* I put this in when I couldn't figure out why sockets were failing from */
/* a bad net connection for the server. */
#undef DEBUGPROCESS

/* Define this to make various system related @tune options changable */
/* by W4+ admin only. Leaving it undefined will allow W3 admin to change */
/* all @tune options as before. */
#define W4_TUNEABLES

/* Define this to compile in the new EXPERIMENTAL webserver. It will share all */
/* it's other settings with the old webserver. They may both be compiled in.   */
#define NEWHTTPD

/* Define this to turn off the 'exiting insert mode' message in the
 * MUF editor for the picky programmer in you.
 */
#undef NO_EXITMSG

/* Define the following to allow dynamic linked exits. This allows
 * an action's destination to be changed by the message calls, 
 * either by using MUF or MPI within the calls.  
 * I.e., @succ, @odrop, ect. Only requirement is that the new
 * destination must be of the same type as the old destination.
 * If the exit points to a room, the reassigned destination must
 * point to a room as well, and this does not work with actions
 * that are pointing to programs.   
 */
#define DYNAMIC_LINKS

/* Define this to set which server to use for e-mail registration.
 */
#define MAILSERVER "mail.somewhere.com"

/************************************************************************
   Game Options

   These are the ones players will notice. 
 ************************************************************************/

/* Set this to your mark for shouts, dumps, etc.  Also change @tunes */

#define MARK "[!]"

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
#define HELP_LOG    "logs/help"         /* text of failed help attemtps */

#define MACRO_FILE  "muf/macros"
#define PID_FILE    "protomuck.pid"      /* Write the server pid to ... */

#define RESOLVER_PID_FILE "hostfind.pid"   /* Write the resolver pid to ... */

#ifdef LOCKOUT
# define LOCKOUT_FILE "data/lockout.txt"
#endif /* LOCKOUT */

/*
 * Set this to be the file that you wish the ProtoMUCK file prims to
 * be set to save to by default. "files/" is the normal directory
 * chosen, though "../public_html" might be another favorite. Setting
 * this to a directory that does not exist will disable file prims 
 * except for use with shortcuts.
 */
#define FILE_PRIMS_DIRECTORY "files/"

#ifdef DETACH
# define LOG_FILE "logs/protomuck"          /* Log stdout to ... */      
# define LOG_ERR_FILE "logs/protomuck.err"  /* Log stderr to ... */      
#endif /* DETACH */

/************************************************************************
  System Dependency Defines. 

  You probably will not have to monkey with this unless the muck fails
 to compile for some reason.
 ************************************************************************/
#ifdef APPLE
#define __FreeBSD__
#endif

/* If you get problems compiling strftime.c, define this. */
#undef USE_STRFTIME

/* Use this only if your realloc does not allocate in powers of 2
 * (if your realloc is clever, this option will cause you to waste space).
 * SunOS requires DB_DOUBLING.  ULTRIX doesn't.  */
#define DB_DOUBLING

/* Prevent Many Fine Cores. */
#undef NOCOREDUMP

/* if do_usage() in wiz.c gives you problems compiling, define this */
#undef NO_USAGE_COMMAND

/* if do_memory() in wiz.c gives you problems compiling, define this */
#define NO_MEMORY_COMMAND

/* This gives some debug Bmalloc profiling, but also eats some overhead,
   so only define if your using it. */
#undef MALLOC_PROFILING
/* #define CRT_DEBUG_ALSO TRUE */

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

#ifdef _MSC_VER 
/* 
 * If you are using Visual C++, please use 6.0 or later.
 */
#define WIN_VC
#endif


/* Alynna: I am just going to assume this works.  
 * Not touching VC++ with 10 foot pole.  :) */
#ifdef WIN_VC
# include "winconf.h"
# include "process.h"
# include <winsock.h>
# define  errnosocket WSAGetLastError()
# ifdef  SPAWN_HOST_RESOLVER
#  undef SPAWN_HOST_RESOLVER
# endif
  /* Define WINNT >= 4.0 to get better WinSock compile */
# ifdef _WIN32_WINNT
#  undef _WIN32_WINNT
# endif
# define _WIN32_WINNT 0x0400
# include <windows.h>
# define  strcasecmp(x,y) stricmp((x),(y))
# define  strncasecmp(x,y,z) strnicmp((x),(y),(z))
# define  waitpid(x,y,z) cwait((y),(x),_WAIT_CHILD)
# define  ioctl(x,y,z) ioctlsocket((x),(y),(z))
# define  EWOULDBLOCK WSAEWOULDBLOCK
# define  EINTR WSAEWOULDBLOCK
# define  getdtablesize() (FD_SETSIZE)
# define  readsocket(x,y,z) recv((x),(y),(z),0)
# define  writesocket(x,y,z) send((x),(y),(z),0)
# define  random() rand()
# define  srandom() srand()
  extern void gettimeofday(struct timeval *tval, void *tzone);
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

/* Alynna: Modified to define CYGWIN, just in case this really is CYGWIN
 * And the user didnt define it, and DONT define WIN32 here, as CYGWIN
 * fills in for WIN32.  The UNIX stuff will usually work.
 */
#if defined(linux) || defined(__linux__) || defined(LINUX)
# define SYS_TYPE "Linux"
# define LINUX
#endif

#if defined(__CYGWIN__)
# define SYS_TYPE "Cygwin"
# define CYGWIN
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

#ifdef bsd4_3
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
#  define SYS_TYPE "SYSV"
# endif
#endif

#if defined(__WINDOWS__) 
# define SYS_TYPE "WINDOWS"
# if !defined(WIN32) && !defined(__VISUAL_C__)
#  define WIN32
# endif
#endif

#if defined(WIN_VC)
# define SYS_TYPE "WINDOWS"
#endif

#ifndef SYS_TYPE
# define SYS_TYPE "UNKNOWN"
#endif

/******************************************************************/
/* Final line of defense for self configuration, systems we know  */
/* need special treatment.                                        */ 
/******************************************************************/





#endif /* _CONFIG_H_ */
