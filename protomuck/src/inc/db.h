#include "copyright.h"

#ifndef __DB_H
#define __DB_H

#include "config.h"
#include <stdio.h>
#ifndef WIN_VC
# include <time.h>
#endif
#include <sys/time.h>
#ifdef HAVE_TIMEBITS_H
#  define __need_timeval 1
#  include <timebits.h>
#endif

#ifdef SQL_SUPPORT
#include <mysql/mysql.h>
#include <mysql/mysql_version.h>
#endif

/* max length of command argument to process_command */
#define MAX_COMMAND_LEN 3072
#define BUFFER_LEN ((MAX_COMMAND_LEN)*4)
#define FILE_BUFSIZ ((BUFSIZ)*8)

/* smallest possible numbers to use before a float is considered to be '0' or
   'false'. */
#define SMALL_NUM 1.0E-37
#define NSMALL_NUM -1.0E-37

extern time_t current_systime;
extern char match_args[BUFFER_LEN];
extern char match_cmdname[BUFFER_LEN];

typedef int dbref;		/* offset into db */

#define TIME_INFINITE ((sizeof(time_t) == 4)? 0xefffffff : 0xefffffffffffffff)

#define DB_READLOCK(x)
#define DB_WRITELOCK(x)
#define DB_RELEASE(x)

#ifdef GDBM_DATABASE
#  define DBFETCH(x)  dbfetch(x)
#  define DBDIRTY(x)  dbdirty(x)
#else				/* !GDBM_DATABASE */
#  define DBFETCH(x)  (db + (x))
#  ifdef DEBUGDBDIRTY
#    define DBDIRTY(x)  {if (!(db[x].flags & OBJECT_CHANGED))  \
			   log2file("dirty.out", "#%d: %s %d\n", (int)x, \
			   __FILE__, __LINE__); \
		       db[x].flags |= OBJECT_CHANGED;}
#  else
#    define DBDIRTY(x)  {db[x].flags |= OBJECT_CHANGED;}
#  endif
#endif

#define DBSTORE(x, y, z)    {DBFETCH(x)->y = z; DBDIRTY(x);}
#define NAME(x)     (db[x].name)
#define PNAME(x)    (db[x].name)
#define RNAME(x)    (db[x].name)
#define OWNER(x)    (db[x].owner)
#define FLAGS(x)    (db[x].flags)
#define FLAG2(x)    (db[x].flag2)
#define FLAG3(x)    (db[x].flag3)
#define FLAG4(x)    (db[x].flag4)
#define POWERS(x)   (db[OWNER(x)].powers)
#define POWERSDB(x) (db[x].powers)
#define POWER2(x)   (db[OWNER(x)].power2)
#define POWER2DB(x) (db[x].power2)

/* defines for possible data access mods. */
#define GETMESG(x,y)   (get_property_class(x, y))
#define GETDESC(x)  GETMESG(x, "_/de")
#define GETANSIDESC(x)  GETMESG(x, "_/anside")
#define GETHTMLDESC(x)  GETMESG(x, "_/htmlde")
#define GETIDESC(x) GETMESG(x, "_/ide")
#define GETIANSIDESC(x) GETMESG(x, "_/ianside")
#define GETIHTMLDESC(x) GETMESG(x, "_/ihtmlde")
#define GETSUCC(x)  GETMESG(x, "_/sc")
#define GETOSUCC(x) GETMESG(x, "_/osc")
#define GETFAIL(x)  GETMESG(x, "_/fl")
#define GETOFAIL(x) GETMESG(x, "_/ofl")
#define GETDROP(x)  GETMESG(x, "_/dr")
#define GETODROP(x) GETMESG(x, "_/odr")
#define GETDOING(x) GETMESG(x, "_/do")
#define GETOECHO(x) GETMESG(x, "_/oecho")
#define GETPECHO(x) GETMESG(x, "_/pecho")

#define SETMESG(x,y,z)    {add_property(x, y, z, 0);}
#define SETDESC(x,y)   SETMESG(x,"_/de",y)
#define SETANSIDESC(x,y)   SETMESG(x,"_/anside",y)
#define SETHTMLDESC(x,y)   SETMESG(x,"_/htmlde",y)
#define SETIDESC(x,y)   SETMESG(x,"_/ide",y)
#define SETIANSIDESC(x,y)  SETMESG(x,"_/ianside",y)
#define SETIHTMLDESC(x,y)  SETMESG(x,"_/ihtmlde",y)
#define SETSUCC(x,y)   SETMESG(x,"_/sc",y)
#define SETFAIL(x,y)   SETMESG(x,"_/fl",y)
#define SETDROP(x,y)   SETMESG(x,"_/dr",y)
#define SETOSUCC(x,y)  SETMESG(x,"_/osc",y)
#define SETOFAIL(x,y)  SETMESG(x,"_/ofl",y)
#define SETODROP(x,y)  SETMESG(x,"_/odr",y)
#define SETDOING(x,y)  SETMESG(x,"_/do",y)
#define SETOECHO(x,y)  SETMESG(x,"_/oecho",y)
#define SETPECHO(x,y)  SETMESG(x,"_/pecho",y)

#define LOADMESG(x,y,z)    {add_prop_nofetch(x,y,z,0); DBDIRTY(x);}
#define LOADDESC(x,y)   LOADMESG(x,"_/de",y)
#define LOADANSIDESC(x,y)   LOADMESG(x,"_/anside",y)
#define LOADHTMLDESC(x,y)   LOADMESG(x,"_/htmlde",y)
#define LOADIDESC(x,y)  LOADMESG(x,"_/ide",y)
#define LOADIANSIDESC(x,y)  LOADMESG(x,"_/ianside",y)
#define LOADIHTMLDESC(x,y)  LOADMESG(x,"_/ihtmlde",y)
#define LOADSUCC(x,y)   LOADMESG(x,"_/sc",y)
#define LOADFAIL(x,y)   LOADMESG(x,"_/fl",y)
#define LOADDROP(x,y)   LOADMESG(x,"_/dr",y)
#define LOADOSUCC(x,y)  LOADMESG(x,"_/osc",y)
#define LOADOFAIL(x,y)  LOADMESG(x,"_/ofl",y)
#define LOADODROP(x,y)  LOADMESG(x,"_/odr",y)

#define SETLOCK(x,y) {set_property(x, "_/lok", PROP_LOKTYP, (PTYPE)y);}
#define LOADLOCK(x,y) {set_property_nofetch(x, "_/lok", PROP_LOKTYP, (PTYPE)y); DBDIRTY(x);}
#define CLEARLOCK(x) {set_property(x, "_/lok", PROP_LOKTYP, (PTYPE)TRUE_BOOLEXP);}
#define GETLOCK(x) (get_property_lock(x, "_/lok"))

#define DB_PARMSINFO    0x0001
#define DB_COMPRESSED   0x0002

#define TYPE_ROOM	   	    0x0
#define TYPE_THING	    0x1
#define TYPE_EXIT	   	    0x2
#define TYPE_PLAYER	    0x3
#define TYPE_PROGRAM	    0x4
#define TYPE_GARBAGE	    0x6
#define NOTYPE		    0x7	/* no particular type */
#define TYPE_MASK	   	    0x7	/* room for expansion */
#define ANTILOCK	   	    0x8	/* negates key (*OBSOLETE*) */
#define W3		  	   0x10	/* gets automatic control */
#define LINK_OK		   0x20	/* anybody can link to this room */
#define DARK		   0x40	/* contents of room are not printed */
#define INTERNAL	  	   0x80	/* internal-use-only flag */
#define STICKY		  0x100	/* this object goes home when dropped */
#define BUILDER		  0x200	/* this player can use construction commands */
#define CHOWN_OK	 	  0x400	/* this player can be @chowned to */
#define JUMP_OK		  0x800	/* A room which can be jumped from, or */
                                    /* a player who can be jumped to */
#define GENDER_MASK	 0x3000	/* 2 bits of gender */
#define GENDER_SHIFT	     12	/* 0x1000 is 12 bits over (for shifting) */
#define GENDER_UNASSIGNED   0x0	/* unassigned - the default */
#define GENDER_NEUTER	    0x1	/* neuter */
#define GENDER_FEMALE	    0x2	/* for women */
#define GENDER_MALE 	    0x3     /* for men */

#define UNDEF_FLAG_1	 0x4000	/* This used to be the KILL_OK flag. */
#define W4               0x8000     /* W4 flag */

#define HAVEN		0x10000	/* can't kill here */
#define ABODE		0x20000	/* can set home here */

#define W1			0x40000	/* W1 flag */

#define QUELL		0x80000	/* When set, a wizard is considered to not be
		            		 * a wizard. */
#define W2	           0x100000	/* W2 flag */

#define INTERACTIVE    0x200000     /* when this is set, player is either editing
				             * a program or in a READ. */
#define OBJECT_CHANGED 0x400000     /* when an object is dbdirty()ed, set this */
#define SAVED_DELTA    0x800000     /* object last saved to delta file */

#define VEHICLE       0x1000000     /* Vehicle flag */
#define ZOMBIE        0x2000000     /* Zombie flag */

#define LISTENER      0x4000000     /* listener flag */
#define XFORCIBLE     0x8000000     /* externally forcible flag */
#define SANEBIT      0x10000000     /* used to check db sanity */

#define READMODE     0x10000000     /* when set, player is in a READ */


/* F2 flags */

#define F2GUEST		    0x1     /* Guest character */
#define F2LOGWALL	          0x2     /* Wizard sees logs walled */
#define F2MUFCOUNT	    0x4     /* Program notes instruction counts */ 
#define F2HIDDEN            0x8     /* The new HIDDEN flag */
#define F2MOBILE	         0x10     /* Mobile object */ 
#define F2PUEBLO           0x20     /* Player has Pueblo multimedia support */
#define F2HTML             0x40     /* Player has at least BASIC HTML, maybe Pueblo */
#define F2MCP              0x80     /* Legacy. Was for Loki's MCP programs */ 
#define F2PROTECT         0x100     /* The new PROTECT flag */
#define F2PARENT          0x200     /* The new PARENT flag */
#define F2COMMAND         0x400     /* For command props - INTERNAL FLAG ONLY */
#define F2EXAMINE_OK      0x800     /* The new EXAMINE_OK flag */
#define F2ANTIPROTECT    0x1000     /* Anti-protect flag to allow a wizard to combat against PROTECT. */
#define F2IDLE           0x2000     /* To watch if someone is idle or not. */
#define F2NO_COMMAND     0x4000     /* Set on an object to prevent command props from being ran */
#define F2LIGHT          0x8000     /* The LIGHT flag to counteract against the DARK flag */
#define F2TRUEIDLE      0x10000     /* To watch if someone is idle past the @tune'd idletime */

/* Proto @Powers */

#define POW_ANNOUNCE          0x1   /* [a] Can use @wall and dwall commands */
#define POW_BOOT              0x2   /* [b] Can use @boot and dboot commands */
#define POW_CHOWN_ANYTHING    0x4   /* [c] Can @chown anything, unless it is PROTECTed */
#define POW_EXPANDED_WHO      0x8   /* [x] Gets the wizard version of WHO */
#define POW_HIDE             0x10   /* [h] Can set themselves DARK or login HIDDEN */
#define POW_IDLE             0x20   /* [i] Not effected by the idle limit */
#define POW_LINK_ANYWHERE    0x40   /* [l] Can @link an exit to anywhere */
#define POW_LONG_FINGERS     0x80   /* [g] Can do anything from a long distance */
#define POW_NO_PAY          0x100   /* [n] Infinite money */
#define POW_OPEN_ANYWHERE   0x200   /* [o] @open an exit from any location */
#define POW_PLAYER_CREATE   0x400   /* [p] Can use @pcreate, @frob, and @toad */
#define POW_SEARCH          0x800   /* [s] Can use @find, @entrances, and @contents */
#define POW_SEE_ALL        0x1000   /* [e] Can examine any object, and @list any program */
#define POW_TELEPORT       0x2000   /* [t] Fee use of @teleport */
#define POW_SHUTDOWN       0x4000   /* [d] Ability to @shutdown and @restart*/
#define POW_CONTROL_MUF    0x8000   /* [f] Ability to control all MUFs */
#define POW_CONTROL_ALL   0x10000   /* [r] Ability to control all objects */
#define POW_ALL_MUF_PRIMS 0x20000   /* [m] Gives full access to MUF prims */
#define POW_STAFF         0x40000   /* [w] Special support for 'staff' bits in MUF */

/* FREE POWER LETTERS: jkquvyz */

/* what flags to NOT dump to disk. */
#define DUMP_MASK	(INTERACTIVE | SAVED_DELTA | OBJECT_CHANGED | LISTENER | READMODE | SANEBIT)
#define DUM2_MASK	(F2IDLE | F2COMMAND | F2TRUEIDLE)
#define DUM3_MASK (0)
#define DUM4_MASK (0)

/* what powers to NOT dump to disk. */
#define POWERS_DUMP_MASK (0)
#define POWER2_DUMP_MASK (0)

typedef int object_flag_type;
typedef int object_power_type;

#define DoNull(s) ((s) ? (s) : "")
#define Typeof(x) ((x == HOME) ? TYPE_ROOM : (FLAGS(x) & TYPE_MASK))

#define MAN	(1)
#define Man(x)   ((x) == MAN)
#define GOD (1)
#define God(x)   ((x) == GOD)

#define LMAN	(9)
#define LBOY      (8)
#define LARCH	(7)
#define LWIZ	(6)
#define LMAGE	(5)
#define LM3	      (4)
#define LM2	      (3)
#define LM1	      (2)
#define LMUF	(2)
#define LMPI	(1)

#define CheckMWLevel(x)       ( ( ((FLAGS(x) & W4)?1:0)<<3 ) + \
                                ( ((FLAGS(x) & W3)?1:0)<<2 ) + \
                                ( ((FLAGS(x) & W2)?1:0)<<1 ) + \
                                ( ((FLAGS(x) & W1)?1:0)    )    )

extern int RawMWLevel(dbref thing);

#define RawMLevel(x)    ( Man(x) ? LMAN : RawMWLevel(x) )

#define MLevel(x)       ( Man(x) ? LMAN : RawMWLevel(x) )

#define QLevel(x)	( (FLAGS(x) & QUELL) ? 0 : MLevel(x) )

#define SetMLevel(x,y)	( FLAGS(x) =  ( FLAGS(x) & ~(W1|W2|W3|W4) ) | \
					  ( ((y) &  8) ? W4 : 0 ) | \
					  ( ((y) &  4) ? W3 : 0 ) | \
					  ( ((y) &  2) ? W2 : 0 ) | \
					  ( ((y) &  1) ? W1 : 0 )   )

#define TBoy(x)         (MLevel(x) >= LBOY)
#define TArch(x)		(MLevel(x) >= LARCH)
#define TWiz(x)		(MLevel(x) >= LWIZ)
#define TWizard(x)      (MLevel(x) >= LWIZ)
#define TMage(x)		(MLevel(x) >= LMAGE)

#define Boy(x)          (QLevel(x) >= LBOY)
#define Arch(x)		(QLevel(x) >= LARCH)
#define Wiz(x)		(QLevel(x) >= LWIZ)
#define Wizard(x)       (QLevel(x) >= LWIZ)
#define Mage(x)		(QLevel(x) >= LMAGE)

#define Mucker3(x) (MLevel(x) >= LM3)
#define Mucker2(x) (MLevel(x) >= LM2)
#define Mucker1(x) (MLevel(x) >= LM1)
#define Mucker(x)	 (MLevel(x) >= LMUF)
#define Meeper(x)	 (MLevel(x) >= LMPI)


#define PREEMPT 0
#define FOREGROUND 1
#define BACKGROUND 2

#define Dark(x) ((FLAGS(x) & DARK) != 0)

#define Builder(x) ( !tp_restricted_building || (FLAGS(x) & BUILDER) || TMage(x))

#define Guest(x) ( (FLAG2(x) & F2GUEST) && !TMage(x) )

#define Html(x) ((( FLAG2(x) & F2PUEBLO ) || (FLAG2(x) & F2HTML)) != 0)

#define Pueblo(x) (( FLAG2(x) & F2PUEBLO ) != 0)

#define NHtml(x) (( FLAG2(x) & F2HTML ) != 0)

#define MCP(x) (( FLAG2(x) & F2MCP ) != 0)

#define Viewable(x) ( ( (FLAGS(x) & VEHICLE) && (Typeof(x) == TYPE_PROGRAM) ) )

#define ExamineOk(x) (FLAG2(x) & F2EXAMINE_OK)

#define Linkable(x) ((x) == HOME || \
                     (((Typeof(x) == TYPE_ROOM || Typeof(x) == TYPE_THING) ? \
                      (FLAGS(x) & ABODE) : (FLAGS(x) & LINK_OK)) != 0))
#define Light(x)    (FLAG2(x) & F2LIGHT)
#define Protect(x)  (FLAG2(x) & F2PROTECT)
#define Hidden(x)   (FLAG2(x) & F2HIDDEN)

#define OkObj(x)  ( ((x) >= 0) && ((x) < db_top) )
#define OkRoom(x) ( OkObj(x) && (Typeof(x)==TYPE_ROOM) )
#define EnvRoom   ( OkRoom(GLOBAL_ENVIRONMENT) ? GLOBAL_ENVIRONMENT : 0 )
#define RootRoom  ( (const) (OkRoom(tp_player_start) ? tp_player_start : EnvRoom) )
#define OkType(x) (	(Typeof(x)==TYPE_THING) ||	\
			(Typeof(x)==TYPE_ROOM) ||	\
			(Typeof(x)==TYPE_EXIT) ||	\
			(Typeof(x)==TYPE_PLAYER) ||	\
			(Typeof(x)==TYPE_PROGRAM)	)

/* Boolean expressions, for locks */
typedef char boolexp_type;

#define BOOLEXP_AND 0
#define BOOLEXP_OR 1
#define BOOLEXP_NOT 2
#define BOOLEXP_CONST 3
#define BOOLEXP_PROP 4

struct boolexp {
    boolexp_type type;
    struct boolexp *sub1;
    struct boolexp *sub2;
    dbref   thing;
    struct plist *prop_check;
};

#define TRUE_BOOLEXP ((struct boolexp *) 0)

/* special dbref's */
#define NOTHING ((dbref) -1)	/* null dbref */
#define AMBIGUOUS ((dbref) -2)	/* multiple possibilities, for matchers */
#define HOME ((dbref) -3)	/* virtual room, represents mover's home */

/* editor data structures */

/* Line data structure */
struct line {
    const char *this_line;	/* the line itself */
    struct line *next, *prev;	/* the next line and the previous line */
};

/* constants and defines for MUV data types */
#define MUV_ARRAY_OFFSET		16
#define MUV_ARRAY_MASK			(0xff << MUV_ARRAY_OFFSET)
#define MUV_ARRAYOF(x)			(x + (1 << MUV_ARRAY_OFFSET))
#define MUV_TYPEOF(x)			(x & ~MUV_ARRAY_MASK)
#define MUV_ARRAYSETLEVEL(x,l)	((l << MUV_ARRAY_OFFSET) | MUF_TYPEOF(x))
#define MUV_ARRAYGETLEVEL(x)	((x & MUV_ARRAY_MASK) >> MUV_ARRAY_OFFSET)

/* stack and object declarations */
/* Integer types go here */
#define PROG_VARIES      255    /* MUV flag denoting variable number of args */
#define PROG_VOID        254    /* MUV void return type */
#define PROG_UNTYPED     253    /* MUV unknown var type */

#define PROG_CLEARED       0
#define PROG_PRIMITIVE     1	/* forth prims and hard-coded C routines */
#define PROG_INTEGER       2	/* integer types */
#define PROG_FLOAT         3    /* Floating point numbers */
#define PROG_OBJECT        4	/* database objects */
#define PROG_VAR           5	/* variables */
#define PROG_LVAR          6	/* variables */
#define PROG_SVAR          7    /* variables */
/* Pointer types go here, numbered *AFTER* PROG_STRING */
#define PROG_STRING        9	/* string types */
#define PROG_FUNCTION      10   /* function names for debugging. */
#define PROG_LOCK          11	/* boolean expression */
#define PROG_ADD           12	/* program address - used in calls&jmps */
#define PROG_IF            13	/* A low level IF statement */
#define PROG_EXEC          14	/* EXECUTE shortcut */
#define PROG_JMP           15	/* JMP shortcut */
#define PROG_ARRAY         16   /* Array @ = (r1)..(ri) (i) */
#define PROG_DICTIONARY    17   /* Dictionary array @ = (k1) (r1)..(ki) (ri) (i) */
#define PROG_SOCKET        18   /* ProtoMUCK socket type */
#define PROG_MARK          19   /* Stack markers -- not in yet */
#define PROG_SVAR_AT       20   /* @ shortcut for scoped vars */
#define PROG_SVAR_AT_CLEAR 21   /* @ for scoped vars with optimization on */
#define PROG_SVAR_BANG     22   /* ! shortcut for scoped vars */
#define PROG_TRY           23   /* TRY shortcut */
#define PROG_LVAR_AT       24   /* @ shortcut for lvars */
#define PROG_LVAR_AT_CLEAR 25   /* @ for local vars with var clear optim */
#define PROG_LVAR_BANG     26   /* ! shortcut for local vars */
#define PROG_MYSQL         27   /* A MySQL database connection */

#define MAX_VAR        104	/* maximum number of variables including the
				 * basic ME and LOC                */
#define RES_VAR          4	/* no of reserved variables */

#define STACK_SIZE      1024	/* maximum size of stack */

struct shared_string {		/* for sharing strings in programs */
    int     links;		/* number of pointers to this struct */
    int     length;		/* length of string data */
    char    data[1];		/* shared string data */
};

struct prog_addr {              /* for 'addres references */
    int     links;              /* number of pointers */
    dbref   progref;            /* program dbref */
    struct inst *data;          /* pointer to the code */
};

struct stack_addr {             /* for the system calstack */
    dbref   progref;            /* program call was made from */
    struct inst *offset;        /* the address of the call */
};

struct stk_array_t;

struct muf_socket {             /* struct for MUF socket data */
   int socknum;                 /* The descriptor number for the socket */
   int connected;               /* Set to 1 if ever connected */
   int listening;               /* Set to 1 if successfully opened listening */
   int links;                   /* Number of instances of the socket. */
   int host;                    /* the host address integer */
   char *raw_input;             /* recieve queue for telnet connections. */
   char *raw_input_at;          /* for use in handling the recieve queue */
   int  inIAC;                  /* For correct telnet negotiations. */
   int connected_at;            /* Systime connection was made. */
   int last_time;               /* last time command recieved. */
   const char *hostname;        /* string host name for incoming cons */
   const char *username;        /* string user name for incoming cons */
   int commands;                /* number of commands entered. */
   int port;                    /* port number that LSOCKET is listening on */
   int usequeue;                /* toggles recieve buffer behavior */
   int usesmartqueue;          /* makes the socket completely telnet savy */
   int is_player;               /* means to not close the socket when clearing*/
   int readWaiting;             /* to support socket events */
};

struct muf_socket_queue {
    struct muf_socket *theSock;     /* points to the MUF socket item */
    struct frame *fr;               /* the frame the socket belongs to */
    struct muf_socket_queue *next;
};

#ifdef SQL_SUPPORT
struct muf_sql { /* struct for MUF SQL connections */
    MYSQL *mysql_conn; /* The connection descriptor struct */
    int connected;     /* Bool indicating if connected or not */
    int timeout;       /* Timeout for this connection */
    int links;         /* number of instances of this connection */
};
#endif

struct muf_proc_data {
    char *procname;
	int vars;
	int args;
	const char **varnames;
};

struct inst {			/* instruction */
    short   type;
    short   line;
    union {
        struct shared_string *string;  /* strings */
        struct boolexp *lock;     /* boolean lock expression */
        int     number;		  /* used for both primitives and integers */
        float   fnumber;          /* used for float storage */
        dbref   objref;		  /* object reference */
        struct stk_array_t *array;/* FB6 style array */
        struct inst *call;	  /* use in IF and JMPs */
        struct prog_addr *addr;   /* the result of 'funcname */
        struct muf_socket *sock;  /* a ProtoMUCK socket */
#ifdef SQL_SUPPORT
	struct muf_sql *mysql;    /* A MySQL connection */
#endif
        struct muf_proc_data *mufproc; /* Data specific to each procedure */
    }       data;
};

#include "array.h"

typedef struct inst vars[MAX_VAR];

struct forvars {
	int didfirst;
	struct inst cur;
	struct inst end;
	int step;
	struct forvars *next;
};

struct tryvars {
	int depth;
	int call_level;
	int for_count;
	struct inst *addr;
	struct tryvars *next;
};

struct stack {
    int     top;
    struct inst st[STACK_SIZE];
};

struct sysstack {
    int     top;
    struct stack_addr st[STACK_SIZE];
};

struct callstack {
    int     top;
    dbref   st[STACK_SIZE];
};

struct localvars {
	struct localvars *next;
	struct localvars **prev;
	dbref prog;
	vars lvars;
};

struct forstack {
	int top;
	struct forvars *st;
};

struct trystack {
	int top;
	struct tryvars *st;
};

#define MAX_BREAKS 16
struct debuggerdata {
    unsigned debugging:1;   /* if set, this frame is being debugged */
    unsigned force_debugging:1; /* if set, debugger active even without Z */
    unsigned bypass:1;      /* if set, bypass breakpoint on starting instr */
    unsigned isread:1;      /* if set, the prog is trying to do a read */
    unsigned showstack:1;   /* if set, show stack debug line, each inst. */
    unsigned dosyspop:1;    /* if set, fix system stack before returning */
    int lastlisted;         /* last listed line */
    char *lastcmd;          /* last executed debugger command */
    short breaknum;         /* the breakpoint that was just caught on */

    dbref lastproglisted;   /* What program's text was last loaded to list? */
    struct line *proglines; /* The actual program text last loaded to list. */

    short count;            /* how many breakpoints are currently set */
    short temp[MAX_BREAKS];         /* is this a temp breakpoint? */
    short level[MAX_BREAKS];        /* level breakpnts.  If -1, no check. */
    struct inst *lastpc;            /* Last inst interped.  For inst changes. */
    struct inst *pc[MAX_BREAKS];    /* pc breakpoint.  If null, no check. */
    int pccount[MAX_BREAKS];        /* how many insts to interp.  -2 for inf. */
    int lastline;                   /* Last line interped.  For line changes. */
    int line[MAX_BREAKS];           /* line breakpts.  -1 no check. */
    int linecount[MAX_BREAKS];      /* how many lines to interp.  -2 for inf. */
    dbref prog[MAX_BREAKS];         /* program that breakpoint is in. */
};

struct scopedvar_t {
      int count;
      const char** varnames;
      struct scopedvar_t *next;
      struct inst vars[1];
};

struct dlogidlist {
      struct dlogidlist *next;
      char dlogid[32];
};

struct mufwatchpidlist {
	struct mufwatchpidlist *next;
	int pid;
};

#define STD_REGUID  0
#define STD_SETUID  1
#define STD_HARDUID 2

/* frame data structure necessary for executing programs */
struct frame {
    struct frame *next;
    struct sysstack system;         /* system stack */
    struct stack argument;          /* argument stack */
    struct callstack caller;        /* caller prog stack */
    struct forstack fors;           /* for loop stack */
    struct trystack trys;           /* try block stack */
    struct localvars* lvars;        /* local variables */
    vars    variables;              /* global variables */
    struct inst *pc;                /* next executing instruction */
    int     writeonly;              /* This program should not do reads */
    int     multitask;              /* This program's multitasking mode */
    int     perms;                  /* permissions restrictions on program */
    int     level;                  /* prevent interp call loops */
    int     preemptlimit;           /* cap preempt insts in _/instlimit prop */
    short   already_created;        /* this prog already created an object */
    short   been_background;        /* this prog has run in the background */
    short   skip_declare;           /* tells interp to skip next scoped var decl */
    short   wantsblanks;            /* tells interps to accept blank reads */
    dbref   trig;                   /* triggering object */
    dbref   prog;                   /* program dbref */
    dbref   player;                 /* person who ran the program */
    time_t  started;                /* When this program started. */
    int     instcnt;                /* How many instructions have run. */
    int     timercount;             /* How many timers currently exist. */
    int     pid;                    /* what is the process id? */
    char   *errorstr;               /* The error string thrown */
    int     descr;                  /* Descriptor of running player */
    void *rndbuf;                   /* buffer for seedable random */
    struct scopedvar_t *svars;      /* Variables with function scoping. */
    struct mufevent *events;        /* MUF event list. */
    struct debuggerdata brkpt;      /* info the debugger needs */
    struct timeval proftime;        /* profiling timing code */
    struct timeval totaltime;       /* profiling timing code */
    struct dlogidlist *dlogids;     /* List of dlogids this frame uses. */
    struct mufwatchpidlist *waiters;
    struct mufwatchpidlist *waitees;
	union {
		struct {
			unsigned int div_zero:1;	/* Divide by zero */
			unsigned int nan:1;	/* Result would not be a number */
			unsigned int imaginary:1;	/* Result would be imaginary */
			unsigned int f_bounds:1;	/* Float boundary error */
			unsigned int i_bounds:1;	/* Integer boundary error */
		} error_flags;
		int is_flags;
	} error;
};


struct publics {
    char   *subname;
    int mlev;
    union {
	struct inst *ptr;
	int     no;
    }       addr;
    struct publics *next;
};


struct mcp_binding {
	struct mcp_binding *next;

	char *pkgname;
	char *msgname;
	struct inst *addr;
};

/* union of type-specific fields */

union specific {      /* I've been railroaded! */
    struct {			/* ROOM-specific fields */
	dbref   dropto;
    }       room;
    struct {			/* THING-specific fields */
	dbref   home;
	int     value;
    }       thing;
    struct {			/* EXIT-specific fields */
	int     ndest;
	dbref  *dest;
    }       exit;
    struct {			/* PLAYER-specific fields */
	dbref   home;
	int     pennies;
	dbref   curr_prog;	/* program I'm currently editing */
	short   insert_mode;	/* in insert mode? */
	short   block;
	const char *password;
        int*    descrs;
        short   descr_count;
        int     last_descr;
    }       player;
    struct {			      /* PROGRAM-specific fields */
	short   curr_line;	      /* current-line */
	unsigned short instances;     /* #instances of this prog running */
	int     siz;		      /* size of code */
	struct inst *code;	      /* byte-compiled code */
	struct inst *start;	      /* place to start executing */
	struct line *first;	      /* first line */
	struct publics *pubs;	      /* public subroutine addresses */
	struct mcp_binding *mcpbinds;	/* MCP message bindings. */
      struct timeval proftime;      /* Profiling time spent in this program */
      time_t profstart;             /* Time when profiling started for this prog */
      unsigned int profuses;        /* # calls to this program while profiling */
    }       program;
};

/* timestamps record */

struct timestamps {
    time_t created;
    time_t modified;
    time_t lastused;
    int    usecount;
};


struct object {

    const char *name;
    dbref   location;		/* pointer to container */
    dbref   owner;
    dbref   contents;
    dbref   exits;
    dbref   next;		/* pointer to next in contents/exits chain */
    struct plist *properties;

#ifdef DISKBASE
    int	    propsfpos;
    time_t  propstime;
    dbref   nextold;
    dbref   prevold;
    short   propsmode;
    short   spacer;
#endif

    object_flag_type flags, flag2, flag3, flag4;
    object_power_type powers, power2;
    struct timestamps ts;
    union specific sp;
    unsigned int     mpi_prof_use;
    struct timeval   mpi_proftime;
};

struct macrotable {
    char   *name;
    char   *definition;
    dbref   implementor;
    struct macrotable *left;
    struct macrotable *right;
};

/* Possible data types that may be stored in a hash table */
union u_hash_data {
    int     ival;		/* Store compiler tokens here */
    dbref   dbval;		/* Player hashing will want this */
    void   *pval;		/* compiler $define strings use this */
};

/* The actual hash entry for each item */
struct t_hash_entry {
    struct t_hash_entry *next;	/* Pointer for conflict resolution */
    const char *name;		/* The name of the item */
    union u_hash_data dat;	/* Data value for item */
};

typedef union u_hash_data hash_data;
typedef struct t_hash_entry hash_entry;
typedef hash_entry *hash_tab;

#define PLAYER_HASH_SIZE   (1024)	/* Table for player lookups */
#define COMP_HASH_SIZE     (256)/* Table for compiler keywords */
#define DEFHASHSIZE        (256)/* Table for compiler $defines */

extern struct object *db;
extern struct macrotable *macrotop;
extern dbref db_top;

#ifndef MALLOC_PROFILING
extern char *alloc_string(const char *);
#endif

extern struct line * read_program(dbref);
extern struct line * get_new_line(void);
extern int fetch_propvals(dbref obj, const char *dir);

extern dbref getparent(dbref obj);

extern dbref db_write_deltas(FILE *f);

extern void free_prog_text(struct line * l);

extern void write_program(struct line * first, dbref i);

extern void log_program_text(struct line * first, dbref player, dbref i);

#ifndef MALLOC_PROFILING
extern struct shared_string *alloc_prog_string(const char *);
#endif

extern dbref new_object(void);	/* return a new object */

extern dbref getref(FILE *);	/* Read a database reference from a file. */

extern void putref(FILE *, dbref);	/* Write one ref to the file */

extern struct boolexp *getboolexp(FILE *);	/* get a boolexp */
extern void putboolexp(FILE *, struct boolexp *);	/* put a boolexp */

extern int db_write_object(FILE *, dbref);	/* write one object to file */

extern dbref db_write(FILE * f);/* write db to file, return # of objects */

extern dbref db_read(FILE * f);	/* read db from file, return # of objects */

 /* Warning: destroys existing db contents! */

extern void db_free(void);

extern dbref parse_dbref(const char *);	/* parse a dbref */

extern int ifloat(const char *s);

extern int  number(const char *s);

extern void putproperties(FILE *f, int obj);

extern void getproperties(FILE *f, int obj);

extern void free_line(struct line *l);

extern void db_free_object(dbref i);

extern void db_clear_object(dbref i);

extern void macrodump(struct macrotable *node, FILE *f);

extern void macroload(FILE *f);

extern int WLevel(dbref player);

#define DOLIST(var, first) \
  for((var) = (first); (var) != NOTHING; (var) = DBFETCH(var)->next)
#define PUSH(thing, locative) \
    {DBSTORE((thing), next, (locative)); (locative) = (thing);}
#define getloc(thing) (DBFETCH(thing)->location)

/*
  Usage guidelines:

  To obtain an object pointer use DBFETCH(i).  Pointers returned by DBFETCH
  may become invalid after a call to new_object().

  To update an object, use DBSTORE(i, f, v), where i is the object number,
  f is the field (after ->), and v is the new value.

  If you have updated an object without using DBSTORE, use DBDIRTY(i) before
  leaving the routine that did the update.

  When using PUSH, be sure to call DBDIRTY on the object which contains
  the locative (see PUSH definition above).

  Some fields are now handled in a unique way, since they are always memory
  resident, even in the GDBM_DATABASE disk-based muck.  These are: name,
  flags and owner.  Refer to these by NAME(i), FLAGS(i) and OWNER(i).
  Always call DBDIRTY(i) after updating one of these fields.

  The programmer is responsible for managing storage for string
  components of entries; db_read will produce malloc'd strings.  The
  alloc_string routine is provided for generating malloc'd strings
  duplicates of other strings.  Note that db_free and db_read will
  attempt to free any non-NULL string that exists in db when they are
  invoked.
*/
#endif				/* __DB_H */











