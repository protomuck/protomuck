#include "copyright.h"
#include "version.h"

/* penny related stuff */
/* amount of object endowment, based on cost */
#define OBJECT_ENDOWMENT(cost) (((cost)-5)/5)
#define OBJECT_DEPOSIT(endow) ((endow)*5+4)


/* timing stuff */
#define TIME_MINUTE(x)  (60 * (x))                /* 60 seconds */
#define TIME_HOUR(x)    ((x) * (TIME_MINUTE(60))) /* 60 minutes */
#define TIME_DAY(x)     ((x) * (TIME_HOUR(24)))   /* 24 hours   */


#define MAX_OUTPUT 131071		/* maximum amount of queued output in bytes */

#define DB_INITIAL_SIZE 100  /* initial malloc() size for the db */


/* User interface low level commands */
#define QUIT_COMMAND      "QUIT"
#define WHO_COMMAND       "WHO"
#define LOGIN_WHO_COMMAND "LWHO" /* This only effects the action, not the command itself */
#define PUEBLO_COMMAND    "PUEBLOCLIENT"
#define PREFIX_COMMAND    "OUTPUTPREFIX"
#define SUFFIX_COMMAND    "OUTPUTSUFFIX"

/* Turn this back on when you want MUD to set from root to some user_id */
/* #define MUD_ID "MUCK" */

/* Used for breaking out of muf READs or for stopping foreground programs. */
#define BREAK_COMMAND "@Q"

#define EXIT_DELIMITER ';'      /* delimiter for lists of exit aliases  */
#define MAX_LINKS 20    /* maximum number of destinations for an exit */



#define PCREATE_FLAGS (0)   /* default flag bits for created players */

#define GLOBAL_ENVIRONMENT ((dbref) 0)  /* parent of all rooms.  Always #0 */

/* magic cookies (not chocolate chip) :) */

#define ESCAPE_CHAR 27
#define NOT_TOKEN '!'
#define AND_TOKEN '&'
#define OR_TOKEN '|'
#define LOOKUP_TOKEN '*'
#define REGISTERED_TOKEN '$'
#define NUMBER_TOKEN '#'
#define ARG_DELIMITER '='
#define PROP_DELIMITER ':'
#define PROPDIR_DELIMITER '/'
#define PROP_RDONLY '_'
#define PROP_RDONLY2 '%'
#define PROP_PRIVATE '.'
#define PROP_HIDDEN '@'
#define PROP_SEEONLY '~'

/* magic command cookies (oh me, oh my!) */

#define SAY_TOKEN '"'
#define POSE_TOKEN ':'
#define OVERIDE_TOKEN '!'


/* @edit'or stuff */

#define EXIT_INSERT "."         /* character to exit from insert mode    */
#define INSERT_COMMAND 'i'
#define DELETE_COMMAND 'd'
#define QUIT_EDIT_COMMAND   'q'
#define CANCEL_EDIT_COMMAND 'x'
#define COMPILE_COMMAND 'c'
#define LIST_COMMAND   'l'
#define EDITOR_HELP_COMMAND 'h'
#define KILL_COMMAND 'k'
#define SHOW_COMMAND 's'
#define SHORTSHOW_COMMAND 'a'
#define VIEW_COMMAND 'v'
#define UNASSEMBLE_COMMAND 'u'
#define NUMBER_COMMAND 'n'
#define PUBLICS_COMMAND 'p'

/* @alias stuff */
#define ALIASDIR_CUR  "/@alias/names/"
#define ALIASDIR_LAST "/@alias/last/"

/* maximum number of arguments */
#define MAX_ARG  2

/* Usage comments:
   Line numbers start from 1, so when an argument variable is equal to 0, it
   means that it is non existent.

   I've chosen to put the parameters before the command, because this should
   more or less make the players get used to the idea of forth coding..     */


