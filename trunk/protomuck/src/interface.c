/*
 * $Header: /export/home/davin/tmp/protocvs/protomuck/src/interface.c,v 1.7 2000-09-18 03:15:28 akari Exp $
 *
 * $Log: not supported by cvs2svn $
 *
 * Revision 1.2   2000/06/27 22:12:34  moose
 * A lot of updates for ProtoMUCK
 *
 * Revision 1.11  1996/10/24 20:14:50  loki
 * Fixed annoying web-bug
 *
 * Revision 1.10  1996/10/14 06:53:04  loki
 * Fixed stupid typo.
 *
 * Revision 1.9  1996/10/14 06:50:06  loki
 * Fixed web server bug.
 *
 * Revision 1.8  1996/10/08 23:47:14  jtraub
 * Quickpatch to remove a serious security hole.
 * ./
 *
 * Revision 1.7  1996/10/03 19:51:31  loki
 * Took out some debugging info...
 * One more attempt at fixing NT timeslicing/socket bug.
 *
 * Revision 1.5  1996/10/02 21:35:16  loki
 * Preliminary fix to timeslicing via really cheeseball hack.
 * This fix is for the NT port of Neon.
 *
 */

#include <sys/types.h>
#ifdef WIN32
# include <fcntl.h>
# include <ctype.h>
# include <string.h>
#else
# include <sys/file.h>
# include <sys/ioctl.h>
# include <sys/wait.h>
# include <fcntl.h>
# include <errno.h>
# include <ctype.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
#endif

#include "cgi.h"
#include "copyright.h"
#include "config.h"
#include "defaults.h"
#include "db.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "props.h"
#include "match.h"
#include "mcp.h"
#include "mpi.h"
#include "reg.h"
#include "externs.h"
#include "mufevent.h"

#ifdef HTTPD
#include "interp.h"
#endif

/* Cynbe stuff added to help decode corefiles: */
#define CRT_LAST_COMMAND_MAX 65535
char crt_last_command[ CRT_LAST_COMMAND_MAX ];
int  crt_last_len;
int  crt_last_player;
int  crt_connect_count = 0;


//extern int errno;
int     shutdown_flag = 0;
int     restart_flag = 0;
int total_loggedin_connects = 0;

static const char *connect_fail = "Incorrect login.\r\n";

static const char *flushed_message = "<Flushed>\r\n";
static const char *shutdown_message = "\r\nBe back in a few!\r\n";

int resolver_sock[2];

struct descriptor_data  *descriptor_list = 0;

static int sock, sock_html, sock_pueblo;
static int ndescriptors = 0;
extern void fork_and_dump();

#ifdef RWHO
extern int rwhocli_setup(const char *server, const char *serverpw, const char *myname, const char *comment);
extern int rwhocli_shutdown();
extern int rwhocli_pingalive();
extern int rwhocli_userlogin(const char *uid, const char *name,time_t tim);
extern int rwhocli_userlogout(const char *uid);
#endif

#ifdef COMPRESS
extern const char *uncompress(const char *);
#endif

void    process_commands(void);
void    shovechars(int port);
void    shutdownsock(struct descriptor_data * d);
struct descriptor_data *initializesock(int s, const char *hostname, int port, int hostaddr, int ctype);
void    make_nonblocking(int s);
void    freeqs(struct descriptor_data * d);
void    welcome_user(struct descriptor_data * d);
void    help_user(struct descriptor_data * d);
void    check_connect(struct descriptor_data * d, const char *msg);
void    close_sockets(const char *msg);
int     boot_off(dbref player);
void    boot_player_off(dbref player);
void    boot_player_off_too(dbref player);
const char *addrout(int, unsigned short);
void    dump_users(struct descriptor_data * d, char *user);
struct descriptor_data *new_connection(int sock, int ctype);
void    parse_connect(const char *msg, char *command, char *user, char *pass);
void    set_userstring(char **userstring, const char *command);
int     do_command(struct descriptor_data * d, char *command);
char   *strsave(const char *s);
int     make_socket(int);
int     queue_string(struct descriptor_data *, const char *);
int     queue_write(struct descriptor_data *, const char *, int);
int     process_output(struct descriptor_data * d);
int     process_input(struct descriptor_data * d);
void    announce_connect(int descr, dbref);
void    announce_disconnect(struct descriptor_data *);
char   *time_format_1(time_t);
char   *time_format_2(time_t);
void    init_descriptor_lookup();
void    init_descr_count_lookup();
void    remember_descriptor(struct descriptor_data *);
void    remember_player_descr(dbref player, int);
void    update_desc_count_table();
int*    get_player_descrs(dbref player, int* count);
void    forget_player_descr(dbref player, int);
void    forget_descriptor(struct descriptor_data *);
struct descriptor_data* descrdata_by_descr(int i);
struct  descriptor_data* lookup_descriptor(int);
int     online(dbref player);
int     online_init();
dbref   online_next(int *ptr);
long    max_open_files(void);

#ifdef SPAWN_HOST_RESOLVER
void kill_resolver();
void spawn_resolver();
void resolve_hostnames();
#endif


#define MALLOC(result, type, number) do {   \
				       if (!((result) = (type *) malloc ((number) * sizeof (type)))) \
				       panic("Out of memory");                             \
				     } while (0)

#define FREE(x) (free((void *) x))

#ifndef BOOLEXP_DEBUGGING

#ifdef DISKBASE
extern FILE *input_file;
#endif
#ifdef DELTADUMPS
extern FILE *delta_infile;
extern FILE *delta_outfile;
#endif

static unsigned short resolver_myport;
short db_conversion_flag = 0;
short db_decompression_flag = 0;
short wizonly_mode = 0;

time_t sel_prof_start_time;
long sel_prof_idle_sec;
long sel_prof_idle_usec;
unsigned long sel_prof_idle_use;

void
show_program_usage(char *prog)
{
    fprintf(stderr, "Usage: %s [<options>] [infile [dumpfile [portnum]]]\n", prog);
    fprintf(stderr, "   Arguments:\n");
    fprintf(stderr, "       infile            db loaded at startup. optional with -dbin.\n");
    fprintf(stderr, "       outfile           output db to save to. optional with -dbout.\n");
    fprintf(stderr, "       portnum           port number to listen for connections on.\n");
    fprintf(stderr, "   Options:\n");
    fprintf(stderr, "       -dbin INFILE      uses INFILE as the database to load at startup.\n");
    fprintf(stderr, "       -dbout OUTFILE    uses OUTFILE as the output database to save to.\n");
    fprintf(stderr, "       -port NUMBER      sets the port number to listen for connections on.\n");
    fprintf(stderr, "       -gamedir PATH     changes directory to PATH before starting up.\n");
    fprintf(stderr, "       -convert          load db, save in current format, and quit.\n");
    fprintf(stderr, "       -decompress       when saving db, save in uncompressed format.\n");
    fprintf(stderr, "       -nosanity         don't do db sanity checks at startup time.\n");
    fprintf(stderr, "       -insanity         load db, then enter interactive sanity editor.\n");
    fprintf(stderr, "       -sanfix           attempt to auto-fix a corrupt db after loading.\n");
    fprintf(stderr, "       -wizonly          only allow wizards to login.\n");
    fprintf(stderr, "       -version          display this server's version.\n");
    fprintf(stderr, "       -help             display this message.\n");
    exit(1);
}


extern int sanity_violated;

int 
main(int argc, char **argv)
{
    int whatport;
    FILE *ffd;
    char *infile_name = NULL;
    char *outfile_name = NULL;
    int i, plain_argnum, nomore_options;
    int sanity_skip;
    int sanity_interactive;
    int sanity_autofix;

#ifdef DETACH
    int   fd;
#endif

#ifdef WIN32
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(1, 1), &wsaData) != 0) {
		perror("WSAStartup failed: unable to initialize Windows socket service.");
		exit(1);
	}
#endif

    if (argc < 3) {
	show_program_usage(*argv);
    }

    time(&current_systime);
    init_descriptor_lookup();
    init_descr_count_lookup();

    plain_argnum = 0;
    nomore_options = 0;
    sanity_skip = 0;
    sanity_interactive = 0;
    sanity_autofix = 0;
    for (i = 1; i < argc; i++) {
	if (!nomore_options && argv[i][0] == '-') {
	    if (!strcmp(argv[i], "-convert")) {
		db_conversion_flag = 1;
	    } else if (!strcmp(argv[i], "-decompress")) {
		db_decompression_flag = 1;
	    } else if (!strcmp(argv[i], "-nosanity")) {
		sanity_skip = 1;
	    } else if (!strcmp(argv[i], "-insanity")) {
		sanity_interactive = 1;
	    } else if (!strcmp(argv[i], "-wizonly")) {
		wizonly_mode = 1;
	    } else if (!strcmp(argv[i], "-sanfix")) {
		sanity_autofix = 1;
          } else if (!strcmp(argv[i], "-version")) {
            printf("ProtoMUCK %s (%s -- %s)\n", PROTOBASE, VERSION, NEONVER);
            exit(0);
          } else if (!strcmp(argv[i], "-dbin")) {
            if (i + 1 >= argc) {
               show_program_usage(*argv);
            }
            infile_name = argv[++i];

          } else if (!strcmp(argv[i], "-dbout")) {
            if (i + 1 >= argc) {
               show_program_usage(*argv);
		}
            outfile_name = argv[++i];

          } else if (!strcmp(argv[i], "-port")) {
            if (i + 1 >= argc) {
               show_program_usage(*argv);
            }
            resolver_myport = whatport = atoi(argv[++i]);
          } else if (!strcmp(argv[i], "-gamedir")) {
            if (i + 1 >= argc) {
               show_program_usage(*argv);
            }
            if (chdir(argv[++i])) {
               perror("cd to gamedir");
               exit(4);
            }
	    } else if (!strcmp(argv[i], "--")) {
		nomore_options = 1;
	    } else {
		show_program_usage(*argv);
	    }
	} else {
	    plain_argnum++;
	    switch (plain_argnum) {
	      case 1:
		infile_name = argv[i];
		break;
	      case 2:
		outfile_name = argv[i];
		break;
	      case 3:
		resolver_myport = whatport = atoi(argv[i]);
		break;
	      default:
		show_program_usage(*argv);
		break;
	    }
	}
    }
    if (plain_argnum < 2) {
	show_program_usage(*argv);
    }

#ifdef DISKBASE
    if (!strcmp(infile_name, outfile_name)) {
	fprintf(stderr, "Output file must be different from the input file.");
	exit(3);
    }
#endif


    if (!sanity_interactive) {

#ifdef DETACH
	/* Go into the background */
	fclose(stdin); fclose(stdout); fclose(stderr);
	if (fork() != 0) exit(0);
#endif

	/* save the PID for future use */
	if ((ffd = fopen(PID_FILE,"w")) != NULL)
	{
	    fprintf(ffd, "%d\n", getpid());
	    fclose(ffd);
	}

	log_status("INIT: %s starting.\n", VERSION);
	#ifdef WIN32
	   fprintf(stderr,"ProtoMUCK %s(%s-compat) - Windows Port\n", PROTOBASE, VERSION);
	   fprintf(stderr,"----------------------------------------------\n");
	   fprintf(stderr,"NOTE: The Windows port of Proto is considered\n");
         fprintf(stderr," to be EXPERIMENTAL.  I will try to provide as\n");
         fprintf(stderr," much support as I can for it, but recognize that\n");
         fprintf(stderr," ProtoMuck as a whole is a spare-time project.\n");
         fprintf(stderr,"   --Moose/Van (ashitaka@home.com)\n");
         fprintf(stderr,"   --Akari     (Nakoruru08@hotmail.com)\n");
	   fprintf(stderr,"----------------------------------------------\n");
	#endif

#ifdef DETACH
	/* Detach from the TTY, log whatever output we have... */
	freopen(LOG_ERR_FILE,"a",stderr);
	setbuf(stderr, NULL);
	freopen(LOG_FILE,"a",stdout);
	setbuf(stdout, NULL);

	/* Disassociate from Process Group */
# ifdef SYS_POSIX
	setsid();
# else
#  ifdef  SYSV
	setpgrp(); /* System V's way */
#  else
	setpgrp(0, getpid()); /* BSDism */
#  endif  /* SYSV */

#  ifdef  TIOCNOTTY  /* we can force this, POSIX / BSD */
	if ( (fd = open("/dev/tty", O_RDWR)) >= 0)
	{
	    ioctl(fd, TIOCNOTTY, (char *)0); /* lose controll TTY */
	    close(fd);
	}
#  endif /* TIOCNOTTY */
# endif /* !SYS_POSIX */
#endif /* DETACH */

    }

	/* Initialize MCP and some packages. */
	mcp_initialize();
	gui_initialize();

    sel_prof_start_time = time(NULL); /* Set useful starting time */
    sel_prof_idle_sec = 0;
    sel_prof_idle_usec = 0;
    sel_prof_idle_use = 0;

    if (init_game(infile_name, outfile_name) < 0) {
	fprintf(stderr, "Couldn't load %s!\n", infile_name);
	exit(2);
    }

       resolver_myport = whatport = tp_textport;

#ifdef SPAWN_HOST_RESOLVER
	if (!db_conversion_flag) {
            fprintf(stderr, "INIT: external host resolver started\n");
	    spawn_resolver();
	    host_init();
	}
#endif

    if (!sanity_interactive && !db_conversion_flag) {
#ifndef WIN32
	set_signals();
#endif

#ifdef MUD_ID
	do_setuid(MUD_ID);
#endif                          /* MUD_ID */

	if (!sanity_skip) {
	    sanity(AMBIGUOUS);
	    if (sanity_violated) {
		wizonly_mode = 1;
		if (sanity_autofix) {
		    sanfix(AMBIGUOUS);
		}
	    }
	}

	/* go do it */
	shovechars(whatport);

	if (restart_flag) {
	    close_sockets(
		"\r\nServer restarting, be back in a few!\r\n.");
	} else {
	    close_sockets(shutdown_message);
	}

	do_dequeue(-1, (dbref) 1, "all");

#ifdef RWHO
	if (tp_rwho)
	    rwhocli_shutdown();
#endif
	host_shutdown();
      }



    if (sanity_interactive) {
	san_main();
    } else {
	dump_database();
	tune_save_parmsfile();

#ifdef SPAWN_HOST_RESOLVER
	kill_resolver();
#endif

#ifdef WIN32
	WSACleanup();
#endif

#ifdef MALLOC_PROFILING
	db_free();
	free_old_macros();
	purge_all_free_frames();
	purge_mfns();
#endif

#ifdef DISKBASE
	fclose(input_file);
#endif
#ifdef DELTADUMPS
	fclose(delta_infile);
	fclose(delta_outfile);
	(void) unlink(DELTAFILE_NAME);
#endif

#ifdef MALLOC_PROFILING
	CrT_summarize_to_file("malloc_log", "Shutdown");
#endif

	if (restart_flag) {
	    char numbuf[16];
	    sprintf(numbuf, "%d", whatport);
	    execl("restart", "restart", numbuf, (char *)0);
	}
    }

    exit(0);

#ifdef OLD
    dump_database();
    tune_save_parmsfile();

#ifdef SPAWN_HOST_RESOLVER
    kill_resolver();
#endif

#ifdef MALLOC_PROFILING
    db_free();
    free_old_macros();
    purge_all_free_frames();
    purge_mfns();
#endif

#ifdef DISKBASE
    fclose(input_file);
#endif DISKBASE
#ifdef DELTADUMPS
    fclose(delta_infile);
    fclose(delta_outfile);
    (void) unlink(DELTAFILE_NAME);
#endif

#ifdef MALLOC_PROFILING
    CrT_summarize_to_file("malloc_log", "Shutdown");
#endif

    if (restart_flag) {
	char numbuf[16];
	sprintf(numbuf, "%d", whatport);
	execl("restart", "restart", numbuf, (char *)0);
    }

    exit(0);
#endif /* OLD */
    return(0);
}

#endif                          /* BOOLEXP_DEBUGGING */

/* CGI decoder */

void
notify_descriptor(int c, const char *msg)
{
   char *ptr1;
   const char *ptr2;
   char buf[BUFFER_LEN + 2];
   struct descriptor_data *d;

   d = descrdata_by_descr(c);

    ptr2 = msg;
    while (ptr2 && *ptr2) {
	ptr1 = buf;
	while (ptr2 && *ptr2 && *ptr2 != '\r')
	    *(ptr1++) = *(ptr2++);
	*(ptr1++) = '\r';
	*(ptr1++) = '\n';
	*(ptr1++) = '\0';
	if (*ptr2 == '\r')
	    ptr2++;
   }
   queue_ansi(d, buf);
   process_output(d);   
}

char *
html_escape (const char *str)
{
     static char buf[BUFFER_LEN * 2];
     char  orig[BUFFER_LEN];
     char  *result;

     strcpy(orig, str);

     str = orig;

     result = buf;

     while(*str)
     {
	 if(*str == '<')
	 {
	     *result++ = '&';
	     *result++ = 'l';
	     *result++ = 't';
	     *result++ = ';';
	     str++;
	 } else 
	 if(*str == '>')
	 {
	     *result++ = '&';
	     *result++ = 'g';
	     *result++ = 't';
	     *result++ = ';';
	     str++;
	 } else
	 if(*str == '&')
	 {
	     *result++ = '&';
	     *result++ = 'a';
	     *result++ = 'm';
	     *result++ = 'p';
	     *result++ = ';';
	     str++;
	 } else
	 if(*str == '\"')
	 {
	     *result++ = '&';
	     *result++ = 'q';
	     *result++ = 'u';
	     *result++ = 'o';
	     *result++ = 't';
	     *result++ = ';';
	     str++;
	 } else *result++ = *str++;
     }
     *result = '\0';
     return buf;
}

int
queue_ansi(struct descriptor_data *d, const char *msg)
{
	char buf[BUFFER_LEN + 8];

	if (d->connected) {
		if (FLAGS(d->player) & CHOWN_OK) {
			strip_bad_ansi(buf, msg);
		} else {
			strip_ansi(buf, msg);
		}
	} else {
		strip_ansi(buf, msg);
	}
      mcp_frame_output_inband(&d->mcpframe, buf);
	return strlen(buf);
}

int 
notify_nolisten(dbref player, const char *msg, int isprivate)
{
    struct descriptor_data *d;
    int     retval = 0;
    char    buf[BUFFER_LEN + 2];
    char    buf2[BUFFER_LEN + 2];
    int firstpass = 1;
    char *ptr1;
    char* temp;
    const char *ptr2;
    dbref ref;
    char *lwp;
    int  *darr;
    int len, di;
    int dcount;

#ifdef COMPRESS
    extern const char *uncompress(const char *);

    msg = uncompress(msg);
#endif                          /* COMPRESS */

    ptr2 = msg;
    while (ptr2 && *ptr2) {
	ptr1 = buf;
	while (ptr2 && *ptr2 && *ptr2 != '\r')
	    *(ptr1++) = *(ptr2++);
	*(ptr1++) = '\r';
	*(ptr1++) = '\n';
	*(ptr1++) = '\0';
	if (*ptr2 == '\r')
	    ptr2++;

	darr = get_player_descrs(player, &dcount);

        for (di = 0; di < dcount; di++) {
          d = descrdata_by_descr(darr[di]);
	    if (d->connected && d->player == player) {
		if (Html(player))
		{
		    temp = html_escape(buf);
		    strcpy(buf, "<code>");
		    strcat(buf, temp);
		    strcat(buf, "</code>");
		    if(!Pueblo(player)) strcat(buf, "<BR>");
		}

		if((d->linelen > 0) && !(FLAGS(player)&CHOWN_OK)) {
		    lwp = buf;
		    len = strlen(buf);
		    queue_ansi(descrdata_by_descr(darr[di]), buf);
		} else
		    queue_ansi(descrdata_by_descr(darr[di]), buf);

		if (firstpass) retval++;
	    }
	}
	if (tp_zombies) {
	    if ((Typeof(player) == TYPE_THING) && (FLAGS(player) & ZOMBIE) &&
		    !(FLAGS(OWNER(player)) & ZOMBIE)) {
		ref = getloc(player);
		if (Mage(OWNER(player)) || ref == NOTHING ||
		    Typeof(ref) != TYPE_ROOM || !(FLAGS(ref) & ZOMBIE)) {
		    if (isprivate || getloc(player) != getloc(OWNER(player))) {
			char pbuf[BUFFER_LEN];
			const char *prefix;
			prefix = GETPECHO(player);
#ifdef COMPRESS
			prefix = uncompress(prefix);
#endif

			if (prefix && *prefix) {
			    char ch = *match_args;

			    *match_args = '\0';
			    prefix = do_parse_mesg(-1, player, player, prefix,
					"(@Pecho)", pbuf, MPI_ISPRIVATE
				     );
			    *match_args = ch;
			}
			if (!prefix || !*prefix) {
			    prefix = NAME(player);
			    sprintf(buf2, "%s> %.*s", prefix,
				(int)(BUFFER_LEN - (strlen(prefix) + 3)), buf);
			} else {
			    sprintf(buf2, "%s %.*s", prefix,
				(int)(BUFFER_LEN - (strlen(prefix) + 2)), buf);
			}
				darr = get_player_descrs(OWNER(player), &dcount);

                        for (di = 0; di < dcount; di++) {
				    if (Html(OWNER(player)))
                                       queue_ansi(descrdata_by_descr(darr[di]), html_escape(buf2)); else
                            queue_ansi(descrdata_by_descr(darr[di]), buf2);
                            if (firstpass) retval++;
                        }
		    }
		}
	    }
	}
	firstpass = 0;
    }
    return retval;
}


int 
notify_html_nolisten(dbref player, const char *msg, int isprivate)
{
    struct descriptor_data *d;
    int     retval = 0;
    char    buf[BUFFER_LEN + 2];
    char    buf2[BUFFER_LEN + 2];
    int firstpass = 1;
    char *ptr1;
    const char *ptr2;
    dbref ref;
    char *lwp;
    int *darr;
    int len, di;
    int dcount;

#ifdef COMPRESS
    extern const char *uncompress(const char *);

    msg = uncompress(msg);
#endif                          /* COMPRESS */

    ptr2 = msg;
    while (ptr2 && *ptr2) {
	ptr1 = buf;
	while (ptr2 && *ptr2 && *ptr2 != '\r')
	    *(ptr1++) = *(ptr2++);
	if (*ptr2 == '\r')
	{
	    ptr2++;
	    *(ptr1++) = '\r';
	    *(ptr1++) = '\n';
	}
	*(ptr1++) = '\0';

	darr = get_player_descrs(player, &dcount);

        for (di = 0; di < dcount; di++) {
            d = descrdata_by_descr(darr[di]);
		if((d->linelen > 0) && (d->player == player)
		      && !(FLAGS(d->player)&CHOWN_OK) && 
			  (Html(d->player))) 
		{
		    lwp = buf;
		    len = strlen(buf);
		    queue_ansi(descrdata_by_descr(darr[di]), buf);
		} else if ((Html(d->player)) && (d->player == player))
		{
		    queue_ansi(descrdata_by_descr(darr[di]), buf);
		    if (NHtml(d->player)) queue_ansi(d, "<BR>");
		}
		if (firstpass) retval++;
	}
	if (tp_zombies) {
	    if ((Typeof(player) == TYPE_THING) && (FLAGS(player) & ZOMBIE) &&
		    !(FLAGS(OWNER(player)) & ZOMBIE)) {
		ref = getloc(player);
		if (Mage(OWNER(player)) || ref == NOTHING ||
		    Typeof(ref) != TYPE_ROOM || !(FLAGS(ref) & ZOMBIE)) {
		    if (isprivate || getloc(player) != getloc(OWNER(player))) {
			char pbuf[BUFFER_LEN];
			const char *prefix;

			prefix = GETPECHO(player);

#ifdef COMPRESS
			prefix = uncompress(prefix);
#endif

			if (prefix && *prefix) {
			    char ch = *match_args;

			    *match_args = '\0';
			    prefix = do_parse_mesg(-1, player, player, prefix,
					"(@Pecho)", pbuf, MPI_ISPRIVATE
				     );
			    *match_args = ch;
			}
			if (!prefix || !*prefix) {
			    prefix = NAME(player);
			    sprintf(buf2, "%s> %.*s", prefix,
				(int)(BUFFER_LEN - (strlen(prefix) + 3)), buf);
			} else {
			    sprintf(buf2, "%s %.*s", prefix,
				(int)(BUFFER_LEN - (strlen(prefix) + 2)), buf);
			}
				darr = get_player_descrs(OWNER(player), &dcount);

                        for (di = 0; di < dcount; di++) {
                            queue_ansi(descrdata_by_descr(darr[di]), buf2);
                            if (firstpass) retval++;
                        }
		    }
		}
	    }
	}
	firstpass = 0;
    }
    return retval;
}


int 
notify_from_echo(dbref from, dbref player, const char *msg, int isprivate)
{
    const char *ptr;

#ifdef COMPRESS
    extern const char *uncompress(const char *);
    ptr = uncompress(msg);
#else
    ptr = msg;
#endif                          /* COMPRESS */

    if (tp_listeners) {
	if (tp_listeners_obj || Typeof(player) == TYPE_ROOM) {
	    listenqueue(-1, from, getloc(from), player, player, NOTHING,
			"@listen", ptr, tp_listen_mlev, 1, 0);
	    listenqueue(-1, from, getloc(from), player, player, NOTHING,
			"~listen", ptr, tp_listen_mlev, 1, 1);
	    listenqueue(-1, from, getloc(from), player, player, NOTHING,
			"~olisten", ptr, tp_listen_mlev, 0, 1);
#ifdef ALLOW_OLD_TRIGGERS
	    listenqueue(-1, from, getloc(from), player, player, NOTHING,
			"_listen", ptr, tp_listen_mlev, 1, 1);
	    listenqueue(-1, from, getloc(from), player, player, NOTHING,
			"_olisten", ptr, tp_listen_mlev, 0, 1);
#endif
	}
    }

    if (Typeof(player) == TYPE_THING && (FLAGS(player) & VEHICLE) &&
	    ( /* !(FLAGS(player) & DARK) || */ Mage(OWNER(player)))
    ) {
	dbref ref;
	ref = getloc(player);
	if (Mage(OWNER(player)) || ref == NOTHING ||
		Typeof(ref) != TYPE_ROOM || !(FLAGS(ref) & VEHICLE)
	) {
	    if (!isprivate && getloc(from) == getloc(player)) {
		char buf[BUFFER_LEN];
		char pbuf[BUFFER_LEN];
		const char *prefix;

		prefix = GETOECHO(player);
		if (prefix && *prefix) {
		    char ch = *match_args;

		    *match_args = '\0';
		    prefix = do_parse_mesg(-1, from, player, prefix,
				    "(@Oecho)", pbuf, MPI_ISPRIVATE
			    );
		    *match_args = ch;
		}
		if (!prefix || !*prefix)
		    prefix = "Outside>";
		sprintf(buf, "%s %.*s",
			prefix,
			(int)(BUFFER_LEN - (strlen(prefix) + 2)),
			msg
		);
		ref = DBFETCH(player)->contents;
		while (ref != NOTHING) {
		    notify_nolisten(ref, buf, isprivate);
		    ref = DBFETCH(ref)->next;
		}
	    }
	}
    }

    return notify_nolisten(player, msg, isprivate);
}
int 
notify_html_from_echo(dbref from, dbref player, const char *msg, int 
isprivate) {
    const char *ptr;

#ifdef COMPRESS
    extern const char *uncompress(const char *);
    ptr = uncompress(msg);
#else
    ptr = msg;
#endif                          /* COMPRESS */

    if (tp_listeners) {
	if (tp_listeners_obj || Typeof(player) == TYPE_ROOM) {
	    listenqueue(-1, from, getloc(from), player, player, NOTHING,
			"@listen", ptr, tp_listen_mlev, 1, 0);
	    listenqueue(-1, from, getloc(from), player, player, NOTHING,
			"~listen", ptr, tp_listen_mlev, 1, 1);
	    listenqueue(-1, from, getloc(from), player, player, NOTHING,
			"~olisten", ptr, tp_listen_mlev, 0, 1);
#ifdef ALLOW_OLD_TRIGGERS
	    listenqueue(-1, from, getloc(from), player, player, NOTHING,
			"_listen", ptr, tp_listen_mlev, 1, 1);
	    listenqueue(-1, from, getloc(from), player, player, NOTHING,
			"_olisten", ptr, tp_listen_mlev, 0, 1);
#endif
	}
    }

    if (Typeof(player) == TYPE_THING && (FLAGS(player) & VEHICLE) &&
	    ( /* !(FLAGS(player) & DARK) || */ Mage(OWNER(player)))
    ) {
	dbref ref;
	ref = getloc(player);
	if (Mage(OWNER(player)) || ref == NOTHING ||
		Typeof(ref) != TYPE_ROOM || !(FLAGS(ref) & VEHICLE)
	) {
	    if (!isprivate && getloc(from) == getloc(player)) {
		char buf[BUFFER_LEN];
		char pbuf[BUFFER_LEN];
		const char *prefix;

		prefix = GETOECHO(player);
		if (prefix && *prefix) {
		    char ch = *match_args;

		    *match_args = '\0';
		    prefix = do_parse_mesg(-1, from, player, prefix,
				    "(@Oecho)", pbuf, MPI_ISPRIVATE
			    );
		    *match_args = ch;
		}
		if (!prefix || !*prefix)
		    prefix = "Outside>";
		sprintf(buf, "%s %.*s",
			prefix,
			(int)(BUFFER_LEN - (strlen(prefix) + 2)),
			msg
		);
		ref = DBFETCH(player)->contents;
		while (ref != NOTHING) {
		    notify_html_nolisten(ref, buf, isprivate);
		    ref = DBFETCH(ref)->next;
		}
	    }
	}
    }

    return notify_html_nolisten(player, msg, isprivate);
}

int 
notify_from(dbref from, dbref player, const char *msg)
{
    return notify_from_echo(from, player, msg, 1);
}

int 
notify_html_from(dbref from, dbref player, const char *msg)
{
    return notify_html_from_echo(from, player, msg, 1);
}


int 
notify(dbref player, const char *msg)
{
    return notify_from_echo(player, player, msg, 1);
}

int 
notify_html(dbref player, const char *msg)
{
    return notify_html_from_echo(player, player, msg, 1);
}

int 
anotify_nolisten2(dbref player, const char *msg)
{
    char    buf[BUFFER_LEN + 2];

    if((Typeof(player) == TYPE_PLAYER || (Typeof(player) == TYPE_THING && FLAGS(player) & ZOMBIE)) && (FLAGS(OWNER(player)) & CHOWN_OK)
	&& !(FLAG2(OWNER(player)) & F2HTML)) {
	parse_ansi(player, buf, msg);
    } else {
	unparse_ansi(buf, msg);
    }

    return notify_nolisten(player, buf, 1);
}

int 
anotify_nolisten(dbref player, const char *msg, int isprivate)
{
    char    buf[BUFFER_LEN + 2];

    if((Typeof(player) == TYPE_PLAYER || (Typeof(player) == TYPE_THING && FLAGS(player) & ZOMBIE)) && (FLAGS(OWNER(player)) & CHOWN_OK)
	&& !(FLAG2(OWNER(player)) & F2HTML)) {
	parse_ansi(player, buf, msg);
    } else {
	unparse_ansi(buf, msg);
    }

    return notify_nolisten(player, buf, isprivate);
}


int 
anotify_from_echo(dbref from, dbref player, const char *msg, int isprivate)
{
    char    buf[BUFFER_LEN + 500];

    if((Typeof(player) == TYPE_PLAYER || (Typeof(player) == TYPE_THING && FLAGS(player) & ZOMBIE)) && (FLAGS(OWNER(player)) & CHOWN_OK)
	&& !(FLAG2(OWNER(player)) & F2HTML)) {
	parse_ansi(player, buf, msg);
    } else {
	unparse_ansi(buf, msg);
    }

    return notify_from_echo(from, player, buf, isprivate);
}


int 
anotify_from(dbref from, dbref player, const char *msg)
{
    return anotify_from_echo(from, player, msg, 1);
}


int 
anotify(dbref player, const char *msg)
{
    char    buf[BUFFER_LEN + 500];

    if((Typeof(player) == TYPE_PLAYER || (Typeof(player) == TYPE_THING && FLAGS(player) & ZOMBIE)) && (FLAGS(OWNER(player)) & CHOWN_OK)
	&& !(FLAG2(OWNER(player)) & F2HTML)) {
	parse_ansi(player, buf, msg);
    } else {
	unparse_ansi(buf, msg);
    }

    return notify(player, buf);
}

struct timeval 
timeval_sub(struct timeval now, struct timeval then)
{
    now.tv_sec -= then.tv_sec;
    now.tv_usec -= then.tv_usec;
    if (now.tv_usec < 0) {
	now.tv_usec += 1000000;
	now.tv_sec--;
    }
    return now;
}

int 
msec_diff(struct timeval now, struct timeval then)
{
    return ((now.tv_sec - then.tv_sec) * 1000
	    + (now.tv_usec - then.tv_usec) / 1000);
}

struct timeval 
msec_add(struct timeval t, int x)
{
    t.tv_sec += x / 1000;
    t.tv_usec += (x % 1000) * 1000;
    if (t.tv_usec >= 1000000) {
	t.tv_sec += t.tv_usec / 1000000;
	t.tv_usec = t.tv_usec % 1000000;
    }
    return t;
}

struct timeval 
update_quotas(struct timeval last, struct timeval current)
{
    int     nslices;
    int     cmds_per_time;
    struct descriptor_data *d;

    nslices = msec_diff(current, last) / tp_command_time_msec;

    if (nslices > 0) {
	for (d = descriptor_list; d; d = d->next) {
	    if (d->connected) {
		cmds_per_time = ((FLAGS(d->player) & INTERACTIVE)
			      ? (tp_commands_per_time * 8) : tp_commands_per_time);
	    } else {
		cmds_per_time = tp_commands_per_time;
	    }
	    d->quota += cmds_per_time * nslices;
	    if (d->quota > tp_command_burst_size)
		d->quota = tp_command_burst_size;
	}
    }
    return msec_add(last, nslices * tp_command_time_msec);
}

/*
 * long max_open_files()
 *
 * This returns the max number of files you may have open
 * as a long, and if it can use setrlimit() to increase it,
 * it will do so.
 *
 * Becuse there is no way to just "know" if get/setrlimit is
 * around, since its defs are in <sys/resource.h>, you need to
 * define USE_RLIMIT in config.h to attempt it.
 *
 * Otherwise it trys to use sysconf() (POSIX.1) or getdtablesize()
 * to get what is avalible to you.
 */
#ifdef HAVE_RESOURCE_H
# include <sys/resource.h>
#endif

#if defined(RLIMIT_NOFILE) || defined(RLIMIT_OFILE)
# define USE_RLIMIT
#endif

long     max_open_files(void)
{
#if defined(_SC_OPEN_MAX) && !defined(USE_RLIMIT) /* Use POSIX.1 method, sysconf() */
/*
 * POSIX.1 code.
 */
    return sysconf(_SC_OPEN_MAX);
#else /* !POSIX */
# if defined(USE_RLIMIT) && (defined(RLIMIT_NOFILE) || defined(RLIMIT_OFILE))
#  ifndef RLIMIT_NOFILE
#   define RLIMIT_NOFILE RLIMIT_OFILE /* We Be BSD! */
#  endif /* !RLIMIT_NOFILE */
/*
 * get/setrlimit() code.
 */
    struct rlimit file_limit;
    
    getrlimit(RLIMIT_NOFILE, &file_limit);              /* Whats the limit? */

    if (file_limit.rlim_cur < file_limit.rlim_max)      /* if not at max... */
    {
	file_limit.rlim_cur = file_limit.rlim_max;      /* ...set to max. */
	setrlimit(RLIMIT_NOFILE, &file_limit);
	
	getrlimit(RLIMIT_NOFILE, &file_limit);          /* See what we got. */
    }   

    return (long) file_limit.rlim_cur;

# else /* !RLIMIT */
/*
 * Don't know what else to do, try getdtablesize().
 * email other bright ideas to me. :) (whitefire)
 */
    return (long) getdtablesize();
# endif /* !RLIMIT */
#endif /* !POSIX */
}

void 
goodbye_user(struct descriptor_data * d)
{
    writesocket(d->descriptor, "\r\n", 2);
    writesocket(d->descriptor, tp_leave_message, strlen(tp_leave_message));
    writesocket(d->descriptor, "\r\n\r\n", 4);
}

void
idleboot_user(struct descriptor_data * d)
{
    writesocket(d->descriptor, "\r\n", 2);
    writesocket(d->descriptor, tp_idleboot_msg, strlen(tp_idleboot_msg));
    writesocket(d->descriptor, "\r\n\r\n", 4);
    d->booted=1;
}

static int con_players_max = 0;  /* one of Cynbe's good ideas. */
static int con_players_curr = 0;  /* for playermax checks. */
extern void purge_free_frames(void);

time_t current_systime = 0;

void 
shovechars(int port)
{
    fd_set  input_set, output_set;
    long   now, tmptq;
    struct timeval last_slice, current_time;
    struct timeval next_slice;
    struct timeval timeout, slice_timeout;
    int     maxd, cnt;
    struct descriptor_data *d, *dnext;
    struct descriptor_data *newd;
    int     avail_descriptors;
    struct timeval sel_in, sel_out;
    int     wwwport, puebloport;
    int socknum;
    int openfiles_max;

    puebloport  = tp_puebloport;
    wwwport     = tp_wwwport;
    sock_pueblo = make_socket(puebloport);
    sock_html   = make_socket(wwwport);
    sock        = make_socket(port);
    maxd = sock + 1;
    gettimeofday(&last_slice, (struct timezone *) 0);

    openfiles_max =  max_open_files();
    printf("Max FDs = %d\n", openfiles_max);

    avail_descriptors = max_open_files() - 5;

    while (shutdown_flag == 0) {
    gettimeofday(&current_time, (struct timezone *) 0);
	last_slice = update_quotas(last_slice, current_time);

	next_muckevent();
	process_commands();
	muf_event_process();

	for (d = descriptor_list; d; d = dnext) {
	    dnext = d->next;
	    if (d->booted) {
#ifdef HTTPDELAY
		if(d->httpdata) {
		    queue_ansi(d, d->httpdata);
		    free((void *)d->httpdata);
		    d->httpdata = NULL;
		}
#endif
		process_output(d);
		if (d->booted == 2) {
		    goodbye_user(d);
		}
		d->booted = 0;
		process_output(d);
		shutdownsock(d);
	    }
	}
	purge_free_frames();
	untouchprops_incremental(1);

	if (shutdown_flag)
	    break;
	timeout.tv_sec = 10;
	timeout.tv_usec = 0;
	next_slice = msec_add(last_slice, tp_command_time_msec);
	slice_timeout = timeval_sub(next_slice, current_time);

	FD_ZERO(&input_set);
	FD_ZERO(&output_set);
	if (ndescriptors < avail_descriptors) {
	    FD_SET(sock, &input_set);
	    FD_SET(sock_html, &input_set);
	    FD_SET(sock_pueblo, &input_set);
	}
	for (d = descriptor_list; d; d = d->next) {
	    if (d->input.lines > 100)
		timeout = slice_timeout;
	    else
		FD_SET(d->descriptor, &input_set);
	    if (d->output.head)
		FD_SET(d->descriptor, &output_set);
	}
#ifdef SPAWN_HOST_RESOLVER
	FD_SET(resolver_sock[1], &input_set);
#endif

	tmptq = next_muckevent_time();
	if ((tmptq >= 0L) && (timeout.tv_sec > tmptq)) {
	    timeout.tv_sec = tmptq + (tp_pause_min / 1000);
	    timeout.tv_usec = (tp_pause_min % 1000) * 1000L;
	}
	gettimeofday(&sel_in,NULL);
	if (select(maxd, &input_set, &output_set,
		   (fd_set *) 0, &timeout) < 0) {
	    if (errnosocket != EINTR) {
		perror("select");
		return;
	    }
	} else {
	    int csock = -1;
	    int ctype = CT_MUCK;

	    time(&current_systime);
	    gettimeofday(&sel_out,NULL);
	    if (sel_out.tv_usec < sel_in.tv_usec) {
	      sel_out.tv_usec += 1000000;
	      sel_out.tv_sec -= 1;
	    }
	    sel_out.tv_usec -= sel_in.tv_usec;
	    sel_out.tv_sec -= sel_in.tv_sec;
	    sel_prof_idle_sec += sel_out.tv_sec;
	    sel_prof_idle_usec += sel_out.tv_usec;
	    if (sel_prof_idle_usec >= 1000000) {
	      sel_prof_idle_usec -= 1000000;
	      sel_prof_idle_sec += 1;
	    }
	    sel_prof_idle_use++;
	    time(&now);
	    if (FD_ISSET(sock, &input_set)) {
		csock = sock;
		ctype = CT_MUCK;
	    }
	    if (FD_ISSET(sock_html, &input_set)) {
		csock = sock_html;
		ctype = CT_HTML;
	    }
	    if (FD_ISSET(sock_pueblo, &input_set)) {
		csock = sock_pueblo;
		ctype = CT_PUEBLO;
	    }
	    if (csock >= 0) {
		if ((newd = new_connection(csock, ctype)) <= 0) {
		    if ((newd < 0) && errnosocket
#ifndef WIN32
			    && errno != EINTR
			    && errno != EMFILE
			    && errno != ENFILE
#endif
		    ) {
#ifdef WIN32
			log_status("new_connection: error #%d\n", errnosocket);
#else
			log_status("new_connection: %s\n", sys_errlist[errnosocket]);
#endif
		    }
		} else {
		    if (newd->descriptor >= maxd)
			maxd = newd->descriptor + 1;
		}
	    }
#ifdef SPAWN_HOST_RESOLVER
	    if (FD_ISSET(resolver_sock[1], &input_set)) {
	       resolve_hostnames();
	    }
#endif
	    for (cnt = 0, d = descriptor_list; d; d = dnext) {
		dnext = d->next;
		if (FD_ISSET(d->descriptor, &input_set)) {
		    d->last_time = now;
		    if (!process_input(d)) {
			d->booted = 1;
		    }
		}
		if (FD_ISSET(d->descriptor, &output_set)) {
		    if (!process_output(d)) {
			d->booted = 1;
		    }
		}
		if (d->connected) {
		    cnt++;
		    if (tp_idleboot&&((now - d->last_time) > tp_maxidle)&&
			    !TMage(d->player) && !(POWERS(d->player) & POW_IDLE)
		    ) {
			idleboot_user(d);
		    }
		} else {
		    if ((now - d->connected_at) > 300) {
			d->booted = 1;
		    }
		}
	    }
	    if (cnt > con_players_max) {
		add_property((dbref)0, "~sys/max_connects", NULL, cnt);
#ifdef ALLOW_OLD_TRIGGERS
		add_property((dbref)0, "_sys/max_connects", NULL, cnt);
#endif
		con_players_max = cnt;
	    }
	    con_players_curr = cnt;
	}
    }
    (void) time(&now);

    add_property((dbref)0, "~sys/lastdumptime", NULL, (int)now);
    add_property((dbref)0, "~sys/shutdowntime", NULL, (int)now);
#ifdef ALLOW_OLD_TRIGGERS
    add_property((dbref)0, "_sys/lastdumptime", NULL, (int)now);
    add_property((dbref)0, "_sys/shutdowntime", NULL, (int)now);
#endif
}


void 
wall_and_flush(const char *msg)
{
    struct descriptor_data *d, *dnext;
    char    buf[BUFFER_LEN + 2];

#ifdef COMPRESS
    extern const char *uncompress(const char *);

    msg = uncompress(msg);
#endif                          /* COMPRESS */

    if (!msg || !*msg) return;
    strcpy(buf, msg);
    strcat(buf, "\r\n");

    for (d = descriptor_list; d; d = dnext) {
	dnext = d->next;
	queue_ansi(d, buf);
	/* queue_write(d, "\r\n", 2); */
	if (!process_output(d)) {
	    d->booted = 1;
	}
    }
}

void 
wall_logwizards(const char *msg)
{
    struct descriptor_data *d, *dnext;
    char    buf[BUFFER_LEN + 2];
    int pos=0;

#ifdef COMPRESS
    extern const char *uncompress(const char *);

    msg = uncompress(msg);
#endif                          /* COMPRESS */

    sprintf(buf, "LOG> %s", msg);

    while(buf[pos]) {
	if((buf[pos] == '\n') || !isprint(buf[pos])) {
	    buf[pos] = '\0';
	    break;
	}
	pos++;
    }
    strcat(buf, "\r\n");

    for (d = descriptor_list; d; d = dnext) {
	dnext = d->next;
	if ( d->connected && Arch(d->player) && (FLAG2(d->player)&F2LOGWALL)) {
/*
	    if (d->type == CT_PUEBLO)
	    {
		char *temp = html_escape(buf);
		strcpy(buf,"<code>"); strcat (buf, temp); strcat(buf, "</code>");
	    }
 */
	    queue_ansi(d, buf);
	    if (!process_output(d))
		d->booted = 1;
	}
    }
}

void 
wall_arches(const char *msg)
{
    struct descriptor_data *d, *dnext;
    char    buf[BUFFER_LEN + 2];

#ifdef COMPRESS
    extern const char *uncompress(const char *);

    msg = uncompress(msg);
#endif                          /* COMPRESS */

    strcpy(buf, msg);
    strcat(buf, "\r\n");

    for (d = descriptor_list; d; d = dnext) {
	dnext = d->next;
	if ( d->connected && TArch(d->player)) {
	    queue_ansi(d, buf);
	    if (!process_output(d))
		d->booted = 1;
	}
    }
}


void 
wall_wizards(const char *msg)
{
    struct descriptor_data *d, *dnext;
    char    buf[BUFFER_LEN + 2];

#ifdef COMPRESS
    extern const char *uncompress(const char *);

    msg = uncompress(msg);
#endif                          /* COMPRESS */

    strcpy(buf, msg);
    strcat(buf, "\r\n");

    for (d = descriptor_list; d; d = dnext) {
	dnext = d->next;
	if ( d->connected && Mage(d->player)) {
	    queue_ansi(d, buf);
	    if (!process_output(d))
		d->booted = 1;
	}
    }
}


void 
ansi_wall_wizards(const char *msg)
{
    struct descriptor_data *d, *dnext;
    char    buf[BUFFER_LEN + 2], buf2[BUFFER_LEN + 2];

#ifdef COMPRESS
    extern const char *uncompress(const char *);

    msg = uncompress(msg);
#endif                          /* COMPRESS */

    strcpy(buf, msg);
    strcat(buf, "\r\n");

    for (d = descriptor_list; d; d = dnext) {
	dnext = d->next;
	if ( d->connected && Mage(d->player)) {
        if (FLAGS(d->player) & CHOWN_OK) {
            parse_ansi(d->player, buf2, buf);
        } else {
            unparse_ansi(buf2, buf);
        }
	    queue_ansi(d, buf2);
	    if (!process_output(d))
		d->booted = 1;
	}
    }
}


void 
show_wizards(dbref player)
{
    struct descriptor_data *d, *dnext;

    anotify(player, CINFO "WizChatters:");
    for (d = descriptor_list; d; d = dnext) {
	dnext = d->next;
	if ( d->connected && Mage(d->player))
	    notify(player, NAME(d->player));
    }
}



void
flush_user_output(dbref player)
{
    int di;
    int* darr;
    int dcount;
    struct descriptor_data *d;

    darr = get_player_descrs(OWNER(player), &dcount);
    for (di = 0; di < dcount; di++) {
        d = descrdata_by_descr(darr[di]);
        if (d && !process_output(d)) {
            d->booted = 1;
        }
    }
}   


void 
wall_all(const char *msg)
{
    struct descriptor_data *d, *dnext;
    char    buf[BUFFER_LEN + 2];

#ifdef COMPRESS
    extern const char *uncompress(const char *);

    msg = uncompress(msg);
#endif                          /* COMPRESS */

    strcpy(buf, msg);
    strcat(buf, "\r\n");

    for (d = descriptor_list; d; d = dnext) {
	dnext = d->next;
	queue_ansi(d, buf);
	if (!process_output(d))
	    d->booted = 1;
    }
}

const char* host_as_hex( /* CrT */
    unsigned addr
) {
    static char buf[32];

    sprintf(buf,
	"%d.%d.%d.%d",
	(addr >> 24) & 0xff,
	(addr >> 16) & 0xff,
	(addr >>  8) & 0xff,
	 addr        & 0xff
    );

    return buf;
}

int
str2ip( const char *ipstr )
{
    int ip1, ip2, ip3, ip4;

    if(sscanf(ipstr, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4) != 4)
	return -1;

    if (ip1 < 0 || ip2 < 0 || ip3 < 0 || ip4 < 0) return -1;
    if (ip1>255 || ip2>255 || ip3>255 || ip4>255) return -1;

    return( (ip1 << 24) | (ip2 << 16) | (ip3 << 8) | ip4 );
    /* return( htonl((ip1 << 24) | (ip2 << 16) | (ip3 << 8) | ip4) ); */
}

struct descriptor_data *
new_connection(int sock, int ctype)
{
    int     newsock;
    struct sockaddr_in addr;
    int     addr_len;
    char    hostname[128];

    addr_len = sizeof(addr);
    newsock = accept(sock, (struct sockaddr *) & addr, &addr_len);
    
    if (newsock < 0) {
	return 0;
    } else {
	strcpy(hostname, addrout(addr.sin_addr.s_addr, addr.sin_port));
	if (reg_site_is_blocked(ntohl(addr.sin_addr.s_addr)) == TRUE) {
	    log_status( "*BLK: %2d %s(%d) %s C#%d\n", newsock,
		hostname, ntohs(addr.sin_port),
		host_as_hex(ntohl(addr.sin_addr.s_addr)),
		++crt_connect_count
	    );
	    shutdown(newsock, 2);
	    closesocket(newsock);
	    return 0;
	}
	if (ctype != 1)
	log_status( "ACPT: %2d %s(%d) %s C#%d %s\n", newsock,
	    hostname, ntohs(addr.sin_port),
	    host_as_hex(ntohl(addr.sin_addr.s_addr)),
	    ++crt_connect_count,
	    ctype==0? "muck"    : (
	    ctype==1? "html"    : (
	    ctype==2? "Pueblo"  : (
	    "unknown" )))
	);
	return initializesock( newsock,
	    hostname, ntohs(addr.sin_port),
	    ntohl(addr.sin_addr.s_addr), ctype
	);
    }
}


#ifdef SPAWN_HOST_RESOLVER

void
kill_resolver()
{
    int i;
    pid_t p;

    write(resolver_sock[1], "QUIT\n", 5);
    p = wait(&i);
}



void
spawn_resolver()
{
    socketpair(AF_UNIX, SOCK_STREAM, 0, resolver_sock);
    make_nonblocking(resolver_sock[1]);
    if (!fork()) {
	close(0);
	close(1);
	dup(resolver_sock[0]);
	dup(resolver_sock[0]);
	execl("./resolver", "resolver", NULL);
	execl("./bin/resolver", "resolver", NULL);
	execl("../src/resolver", "resolver", NULL);
	perror("resolver execlp");
	_exit(1);
    }
}


void
resolve_hostnames()
{
    char buf[BUFFER_LEN], *oldname;
    char *ptr, *ptr2, *ptr3, *hostip, *port, *hostname, *username, *tempptr;
    struct descriptor_data *d;
    int got, iport, dc;
    int ipnum;

    got = read(resolver_sock[1], buf, sizeof(buf));
    if (got < 0) return;
    if (got == sizeof(buf)) {
      got--;
	while(got>0 && buf[got] != '\n') buf[got--] = '\0';
    }
    ptr = buf;
    dc = 0;
    do {
	for(ptr2 = ptr; *ptr && *ptr != '\n' && dc < got; ptr++,dc++);
	if (*ptr) { *ptr++ = '\0'; dc++; }
	if (*ptr2) {
	    ptr3 = index(ptr2, ':');
	    if (!ptr3) return;
	   hostip = ptr2;
	   port = index(ptr2, '(');
	   if (!port) return;
	   tempptr = index(port, ')');
	   if (!tempptr) return;
	   *tempptr = '\0';
	   hostname = ptr3;
	   username = index(ptr3, '(');
	   if (!username) return;
	   tempptr = index(username, ')');
	   if (!tempptr) return;
	   *tempptr = '\0';
	    if (*port&&*hostname&&*username) {
		*port++ = '\0';
		*hostname++ = '\0';
		*username++ = '\0';
		if( (ipnum = str2ip(hostip)) != -1 ) {
		    /* we got a new name, update? */
		    if(!( oldname = host_fetch(ipnum) ))
			host_add(ipnum, hostname);
		    else {
			if(strcmp(oldname, hostname)) {
			    log_status( "*RES: %s to %s\n", oldname, hostname );
			    host_del(ipnum);
			    host_add(ipnum, hostname);
			}
		    }
		} else {
		    log_status( "*BUG: resolve_hostnames bad ipstr %s\n", hostip );
		}
		iport = atoi(port);
		for (d = descriptor_list; d; d = d->next) {
		    if((d->hostaddr == ipnum) && (iport == d->port)) {
			FREE(d->hostname);
			FREE(d->username);
			d->hostname = strsave(hostname);
			d->username = strsave(username);
		    }
		}
	    }
	}
    } while (dc < got && *ptr);
}

#endif


/*  addrout -- Translate address 'a' from int to text.          */

const char *
addrout(int a, unsigned short prt)
{
    static char buf[128], *host;
    struct in_addr addr;
    
    addr.s_addr = a;
    prt = ntohs(prt);
	
#ifndef SPAWN_HOST_RESOLVER
    if (tp_hostnames) {
	/* One day the nameserver Qwest uses decided to start */
	/* doing halfminute lags, locking up the entire muck  */
	/* that long on every connect.  This is intended to   */
	/* prevent that, reduces average lag due to nameserver*/
	/* to 1 sec/call, simply by not calling nameserver if */
	/* it's in a slow mood *grin*. If the nameserver lags */
	/* consistently, a hostname cache ala OJ's tinymuck2.3*/
	/* would make more sense:                             */
	static secs_lost = 0;
	if (secs_lost) {
	    secs_lost--;
	} else {
	    time_t gethost_start = current_systime;
	    struct hostent *he   = gethostbyaddr(((char *)&addr), 
						 sizeof(addr), AF_INET);
	    time_t gethost_stop  = time(&current_systime);
	    time_t lag           = gethost_stop - gethost_start;
	    if (lag > 10) {
		secs_lost = lag;
#if         MIN_SECS_TO_LOG
		if (lag >= CFG_MIN_SECS_TO_LOG) {
		    log_status( "GETHOSTBYNAME-RAN: secs %3d\n", lag );
		}
#endif

	    }
	    if (he) {
		sprintf(buf, "%s(%d)", he->h_name, prt);
		return buf;
	    }
	}
    }
#endif /* no SPAWN_HOST_RESOLVER */

    a = ntohl(a);

#ifdef SPAWN_HOST_RESOLVER
    sprintf(buf, "%d.%d.%d.%d(%u)%u\n",
	(a >> 24) & 0xff,
	(a >> 16) & 0xff,
	(a >> 8)  & 0xff,
	 a        & 0xff,
	prt, resolver_myport
    );
    if (tp_hostnames) {
	write(resolver_sock[1], buf, strlen(buf));
    }
#endif
    if( (host = host_fetch(a)) )
	sprintf(buf, "%s", host);
    else
	sprintf(buf, "%s", host_as_hex(a));

    return buf;
}


void 
clearstrings(struct descriptor_data * d)
{
    if (d->output_prefix) {
	FREE(d->output_prefix);
	d->output_prefix = 0;
    }
    if (d->output_suffix) {
	FREE(d->output_suffix);
	d->output_suffix = 0;
    }
}

void 
shutdownsock(struct descriptor_data * d)
{
    if (d->connected) {
	log_status("DISC: %2d %s %s(%s) %s\n",
		d->descriptor, unparse_object(d->player, d->player),
		d->hostname, d->username,
		host_as_hex(d->hostaddr));
	announce_disconnect(d);
    } else {
	if(d->type != CT_HTML)
	log_status("DISC: %2d %s(%s) %s (never connected)\n",
		d->descriptor, d->hostname, d->username,
		host_as_hex(d->hostaddr));
    }
    if (d->identify)
      FREE(d->identify);
    clearstrings(d);
    shutdown(d->descriptor, 2);
    closesocket(d->descriptor);
    forget_descriptor(d);
    freeqs(d);
    *d->prev = d->next;
    if (d->next)
	d->next->prev = d->prev;
    if (d->hostname)
	free((void *)d->hostname);
    if (d->username)
	free((void *)d->username);
    mcp_frame_clear(&d->mcpframe);
#ifdef HTTPDELAY
    if (d->httpdata)
	free((void *)d->httpdata);
#endif
    FREE(d);
    ndescriptors--;
}

void
SendText(McpFrame * mfr, const char *text)
{
	queue_string((struct descriptor_data *) mfr->descriptor, text);
}

int
mcpframe_to_descr(McpFrame * ptr)
{
	return ((struct descriptor_data *) ptr->descriptor)->descriptor;
}

int
mcpframe_to_user(McpFrame * ptr)
{
	return ((struct descriptor_data *) ptr->descriptor)->player;
}

struct descriptor_data *
initializesock(int s, const char *hostname, int port, int hostaddr, int ctype)
{
    struct descriptor_data *d;
    char buf[128];

    

    ndescriptors++;
    MALLOC(d, struct descriptor_data, 1);
    d->descriptor = s;
    d->connected = 0;
    d->player = NOTHING;
    d->booted = 0;
    d->fails = 0;
    d->con_number = 0;
    d->connected_at = time((time_t *)NULL);
    make_nonblocking(s);
    d->output_prefix = 0;
    d->output_suffix = 0;
    d->output_size = 0;
    d->output.lines = 0;
    d->output.head = 0;
    d->output.tail = &d->output.head;
    d->hostaddr = hostaddr;
    d->input.lines = 0;
    d->input.head = 0;
    d->input.tail = &d->input.head;
    d->raw_input = 0;
    d->raw_input_at = 0;
    d->quota = tp_command_burst_size;
    d->commands = 0;
    d->last_time = d->connected_at;
    d->port = port;
    d->type = ctype;
    mcp_frame_init(&d->mcpframe, d);
    d->http_login = 0;
    d->identify = strdup("nohttpdlogin");
    d->lastmidi = strdup("(none)");
    d->hostname = alloc_string(hostname);
    sprintf(buf, "%d", port);
    d->username = alloc_string(buf);
    d->linelen = 0;
#ifdef HTTPDELAY
    d->httpdata = NULL;
#endif
    if (descriptor_list)
	descriptor_list->prev = &d->next;
    d->next = descriptor_list;
    d->prev = &descriptor_list;
    descriptor_list = d;
    remember_descriptor(d);

    

    welcome_user(d);
    return d;
}

int 
make_socket(int port)
{
    int     s;
    struct sockaddr_in server;
    int     opt;

    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
	perror("creating stream socket");
	exit(3);
    }
    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR,
		   (char *) &opt, sizeof(opt)) < 0) {
	perror("setsockopt");
	exit(1);
    }
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
    if (bind(s, (struct sockaddr *) & server, sizeof(server))) {
	perror("binding stream socket");
	closesocket(s);
	exit(4);
    }
    listen(s, 5);
    return s;
}

struct text_block *
make_text_block(const char *s, int n)
{
    struct text_block *p;

    MALLOC(p, struct text_block, 1);
    MALLOC(p->buf, char, n);
    bcopy(s, p->buf, n);
    p->nchars = n;
    p->start = p->buf;
    p->nxt = 0;
    return p;
}

void 
free_text_block(struct text_block * t)
{
    FREE(t->buf);
    FREE((char *) t);
}

void 
add_to_queue(struct text_queue * q, const char *b, int n)
{
    struct text_block *p;

    if (n == 0)
	return;

    p = make_text_block(b, n);
    p->nxt = 0;
    *q->tail = p;
    q->tail = &p->nxt;
    q->lines++;
}

int 
flush_queue(struct text_queue * q, int n)
{
    struct text_block *p;
    int     really_flushed = 0;

    n += strlen(flushed_message);

    while (n > 0 && (p = q->head)) {
	n -= p->nchars;
	really_flushed += p->nchars;
	q->head = p->nxt;
	q->lines--;
	free_text_block(p);
    }
    p = make_text_block(flushed_message, strlen(flushed_message));
    p->nxt = q->head;
    q->head = p;
    q->lines++;
    if (!p->nxt)
	q->tail = &p->nxt;
    really_flushed -= p->nchars;
    return really_flushed;
}

int 
queue_write(struct descriptor_data * d, const char *b, int n)
{
    int     space;

    space = tp_max_output - d->output_size - n;
    if (space < 0)
	d->output_size -= flush_queue(&d->output, -space);
    add_to_queue(&d->output, b, n);
    d->output_size += n;
    return n;
}

int 
queue_string(struct descriptor_data * d, const char *s)
{
    return queue_write(d, s, strlen(s));
}

int 
process_output(struct descriptor_data * d)
{
    struct text_block **qp, *cur;
    int     cnt;

    /* drastic, but this may give us crash test data */
    if (!d || !d->descriptor) {
	fprintf(stderr, "process_output: bad descriptor or connect struct!\n");
	abort();
    }

    if (d->output.lines == 0) {
	return 1;
    }
    
    for ( qp = &d->output.head; (cur = *qp); ) {
	cnt = writesocket(d->descriptor, cur->start, cur->nchars);
	if (cnt < 0) {
#ifdef DEBUGPROCESS
	fprintf(stderr, "process_output: write failed errno %d %s\n", errno,
		sys_errlist[errno]);
#endif
	    if (errno == EWOULDBLOCK)
		return 1;
	    return 0;
	}
	d->output_size -= cnt;
	if (cnt == cur->nchars) {
	    d->output.lines--;
	    if (!cur->nxt) {
		d->output.tail = qp;
		d->output.lines = 0;
	    }
	    *qp = cur->nxt;
	    free_text_block(cur);
	    continue;           /* do not adv ptr */
	}
	cur->nchars -= cnt;
	cur->start += cnt;
	break;
    }
    return 1;
}

void 
make_nonblocking(int s)
{
#if !defined(O_NONBLOCK) || defined(ULTRIX)     /* POSIX ME HARDER */
# ifdef FNDELAY         /* SUN OS */
#  define O_NONBLOCK FNDELAY 
# else
#  ifdef O_NDELAY       /* SyseVil */
#   define O_NONBLOCK O_NDELAY
#  endif /* O_NDELAY */
# endif /* FNDELAY */
#endif

#ifdef WIN32
	if (ioctl(s, FIONBIO, NULL) != 0) {
#else
    if (fcntl(s, F_SETFL, O_NONBLOCK) == -1) {
#endif
	perror("make_nonblocking: fcntl");
	panic("O_NONBLOCK fcntl failed");
    }
}

void 
freeqs(struct descriptor_data * d)
{
    struct text_block *cur, *next;

    cur = d->output.head;
    while (cur) {
	next = cur->nxt;
	free_text_block(cur);
	cur = next;
    }
    d->output.lines = 0;
    d->output.head = 0;
    d->output.tail = &d->output.head;

    cur = d->input.head;
    while (cur) {
	next = cur->nxt;
	free_text_block(cur);
	cur = next;
    }
    d->input.lines = 0;
    d->input.head = 0;
    d->input.tail = &d->input.head;

    if (d->raw_input)
	FREE(d->raw_input);
    d->raw_input = 0;
    d->raw_input_at = 0;
}

char   *
strsave(const char *s)
{
    char   *p;

    MALLOC(p, char, strlen(s) + 1);

    if (p)
	strcpy(p, s);
    return p;
}

void 
save_command(struct descriptor_data * d, const char *command)
{
    if (d->connected && !string_compare((char *) command, BREAK_COMMAND)) {
	if (dequeue_prog(d->player, 2))
	    anotify(d->player, CINFO "Foreground program aborted.");
	DBFETCH(d->player)->sp.player.block = 0;
	if (!(FLAGS(d->player) & INTERACTIVE))
	    return;
    }
    add_to_queue(&d->input, command, strlen(command) + 1);
}

int 
process_input(struct descriptor_data * d)
{
    char    buf[MAX_COMMAND_LEN * 2];
    int     got;
    char   *p, *pend, *q, *qend;

    got = readsocket(d->descriptor, buf, sizeof buf);
    if (got <= 0) {
#ifdef DEBUGPROCESS
	fprintf(stderr, "process_input: read failed errno %d %s\n", errno,
		sys_errlist[errno]);
#endif
	return 0;
    }
    if (!d->raw_input) {
	MALLOC(d->raw_input, char, MAX_COMMAND_LEN);
	d->raw_input_at = d->raw_input;
    }
    p = d->raw_input_at;
    pend = d->raw_input + MAX_COMMAND_LEN - 1;
    for (q = buf, qend = buf + got; q < qend; q++) {
	if (*q == '\n') {
	    *p = '\0';
	    if (p > d->raw_input) {
		save_command(d, d->raw_input);
	    } else {
		/* BARE NEWLINE! */
		if((d->type == CT_HTML) && (d->http_login == 0)) {
#ifdef HTTPDELAY
		    if(d->httpdata) {
			queue_ansi(d, d->httpdata);
			free((void *)d->httpdata);
			d->httpdata = NULL;
		    }
#endif
		    d->booted = 1;
		}
	    }
	    p = d->raw_input;
	} else if (p < pend && isascii(*q)) {
	    if (isprint(*q)) {
		*p++ = *q;
	    } else if (*q == '\t') {
		*p++ = ' ';
	    } else if (*q == 8 || *q == 127) {
		/* if BS or DEL, delete last character */
		if (p > d->raw_input) p--;
	    }
	}
    }
    if (p > d->raw_input) {
	d->raw_input_at = p;
    } else {
	FREE(d->raw_input);
	d->raw_input = 0;
	d->raw_input_at = 0;
    }
    return 1;
}

void 
set_userstring(char **userstring, const char *command)
{
    if (*userstring) {
	FREE(*userstring);
	*userstring = 0;
    }
    while (*command && isascii(*command) && isspace(*command))
	command++;
    if (*command)
	*userstring = strsave(command);
}

void 
process_commands(void)
{
    int     nprocessed;
    struct descriptor_data *d, *dnext;
    struct text_block *t;

    do {
	nprocessed = 0;
	for (d = descriptor_list; d; d = dnext) {
	    dnext = d->next;
	    if (d->quota > 0 && (t = d->input.head)
	       && (!(d->connected && DBFETCH(d->player)->sp.player.block))) {
		d->quota--;
		nprocessed++;
		if (!do_command(d, t->start)) {
               if (Typeof(tp_quit_prog) == TYPE_PROGRAM) {
                  char *full_command, xbuf[BUFFER_LEN], buf[BUFFER_LEN], *msg, *command;
                  struct frame *tmpfr;

                  command = QUIT_COMMAND;
                  strcpy(match_cmdname, QUIT_COMMAND);
                  strcpy(buf, command + sizeof(QUIT_COMMAND) -1);
                  msg = buf;
                  full_command = strcpy(xbuf, msg);
                  for (; *full_command && !isspace(*full_command); full_command++);
                  if (*full_command)
                     full_command++;
                  strcpy(match_args, full_command);
                  tmpfr = interp(d->descriptor, d->player, DBFETCH(d->player)->location,
                                 tp_quit_prog, (dbref) -5, FOREGROUND, STD_REGUID);
                  if (tmpfr) {
                     interp_loop(d->player, tp_quit_prog, tmpfr, 0);
                  }
               } else {
                  d->booted = 2;
               }
            }
		/* start former else block */
		d->input.head = t->nxt;
		d->input.lines--;
		if (!d->input.head) {
		    d->input.tail = &d->input.head;
		    d->input.lines = 0;
		}
		free_text_block(t);
		/* end former else block */
	    }
	}
    } while (nprocessed > 0);
}

int 
do_command(struct descriptor_data * d, char *command)
{
    struct frame *tmpfr;
    char cmdbuf[BUFFER_LEN];

    if (d->connected)
	ts_lastuseobject(d->player);
	if (!mcp_frame_process_input(&d->mcpframe, command, cmdbuf, sizeof(cmdbuf)))
		return 1;
    command = cmdbuf;
    if (!strcmp(command, QUIT_COMMAND)) {
	return 0;
    } else if( !strncmp(command, PUEBLO_COMMAND, sizeof(PUEBLO_COMMAND) - 1)) {
	queue_ansi(d, "</xch_mudtext><img xch_mode=html>");
	d->type = CT_PUEBLO;
    } else if( !strncmp(command, "!WHO", sizeof("!WHO") - 1)) {
	if (!d->connected && (reg_site_is_barred(d->hostaddr) == TRUE)) {
		queue_ansi(d, "Connect and find out!\r\n");
	} else if (tp_secure_who && (!d->connected || !TMage(d->player))) {
		queue_ansi(d, "Connect and find out!\r\n");
	} else {
		dump_users(d, command + sizeof("!WHO") - 1);
	}
    } else if (!strncmp(command, WHO_COMMAND, sizeof(WHO_COMMAND) - 1)) {
	char buf[BUFFER_LEN];
	if (d->output_prefix) {
	    queue_ansi(d, d->output_prefix);
	    queue_write(d, "\r\n", 2);
	}
	strcpy(buf, "@");
	strcat(buf, WHO_COMMAND);
	strcat(buf, " ");
	strcat(buf, command + sizeof(WHO_COMMAND) - 1);

	if (!d->connected) {
	    if (tp_secure_who || (reg_site_is_barred(d->hostaddr) == TRUE)) {
		queue_ansi(d,"Login and find out!\r\n");
		if (d->type == CT_HTML) queue_ansi(d, "<BR>");
	    } else {
            if (Typeof(tp_login_who_prog) == TYPE_PROGRAM) {
               char *full_command, xbuf[BUFFER_LEN], *msg;

               strcpy(match_cmdname, WHO_COMMAND);
               strcpy(buf, command + sizeof(WHO_COMMAND) -1);
               msg = buf;
               full_command = strcpy(xbuf, msg);
               for (; *full_command && !isspace(*full_command); full_command++);
               if (*full_command)
                  full_command++;
               strcpy(match_args, full_command);
               tmpfr = interp(d->descriptor, -1, -1, tp_login_who_prog, (dbref) -5, FOREGROUND, STD_REGUID);
		   if (tmpfr) {
			interp_loop(-1, tp_login_who_prog, tmpfr, 0);
		   }
            } else {
   		   dump_users(d, command + sizeof(WHO_COMMAND) - 1);
            }
	    }
	} else {
            if (can_move(d->descriptor, d->player, buf, 5)) {
               do_move(d->descriptor, d->player, buf, 5);
            } else {
    	       dump_users(d, command + sizeof(WHO_COMMAND) - 1);
            }
	}
	if (d->output_suffix) {
	    queue_ansi(d, d->output_suffix);
	    queue_write(d, "\r\n", 2);
	}
    } else if (!strncmp(command, PREFIX_COMMAND, sizeof(PREFIX_COMMAND) - 1)) {
	set_userstring(&d->output_prefix, command + sizeof(PREFIX_COMMAND) - 1);
    } else if (!strncmp(command, SUFFIX_COMMAND, sizeof(SUFFIX_COMMAND) - 1)) {
	set_userstring(&d->output_suffix, command + sizeof(SUFFIX_COMMAND) - 1);
    } else {
	if (d->connected) {
	    if (d->output_prefix) {
		queue_ansi(d, d->output_prefix);
		queue_write(d, "\r\n", 2);
	    }
	    process_command(d->descriptor, d->player, command);
	    if (d->output_suffix) {
		queue_ansi(d, d->output_suffix);
		queue_write(d, "\r\n", 2);
	    }
	} else {
	    check_connect(d, command);
	}
    }
    (d->commands)++;
    return 1;
}

void 
interact_warn(dbref player)
{
    if (FLAGS(player) & INTERACTIVE) {
	char    buf[BUFFER_LEN];

	sprintf(buf, CINFO "***  %s  ***",
		(FLAGS(player) & READMODE) ?
		"You are currently using a program.  Use \"@Q\" to return to a more reasonable state of control." :
		(DBFETCH(player)->sp.player.insert_mode ?
		 "You are currently inserting MUF program text.  Use \".\" to return to the editor, then \"quit\" if you wish to return to your regularly scheduled Muck universe." :
		 "You are currently using the MUF program editor."));
	anotify(player, buf);
    }
}

#ifdef HTTPD
void
httpd_unknown(struct descriptor_data *d, const char *name) {
    if(httpd_get_lsedit(d, tp_www_root, "http/404") <= 0) {
	queue_ansi(d,
	    "<HTML>\r\n<HEAD><TITLE>404 Not Found</TITLE></HEAD><BODY>\r\n"
	    "<H1>404 Not Found</H1>\r\n"
	    "The requested URL "
	);
	queue_ansi(d, name);
	queue_ansi(d,
	    " was not found on this server.<P>\r\n</BODY></HTML>\r\n"
	);
    }
    d->booted = 1;
}

int
httpd_get_lsedit(struct descriptor_data *d, dbref what, const char *prop) {
    const char *m = NULL;
    int   lines = 0;
    char  buf[BUFFER_LEN];

    if(!OkObj(what))
	return 0;
    
    if (*prop && (Prop_Hidden(prop) || Prop_Private(prop)))
	return 0;

    while((*prop == '/') || (*prop == ' '))
	prop++;

    while ( (lines < 256) && (!lines || (m && *m)) ) {
	if(*prop)
	    sprintf(buf, WWWDIR "/%.512s#/%d", prop, ++lines);
	else
	    sprintf(buf, WWWDIR "#/%d", ++lines);
	m = get_property_class(what, buf);
#ifdef COMPRESS
	if(m && *m) m = uncompress(m);
#endif
	if( m && *m ) {
	    sprintf(buf, "%.512s\r\n", m);
	    queue_ansi(d, buf);
	}
    }

    return lines - 1;
}

void
httpd_get(struct descriptor_data *d, char *name, const char *http) {
    const char *prop = NULL, *m = NULL, **dit = infotext;
    dbref   what = 0;
    PropPtr prpt;
    int lines = 0, cold = CT_MUCK;
    int foundit = 0;
    char *params = NULL;
    char    buf[BUFFER_LEN];

#ifdef HTTPDELAY
    if( d->httpdata ) {
	free((void *)d->httpdata);
	d->httpdata = NULL;
    }
#endif

    params = name;

    while ((*params != '\0') && (*params != '?'))
	params++;

    if (*params == '?') 
    {
       *params = '\0';
       params++;
    }

   if (strcmp(name, ""))
   {
    if (string_prefix("login", name))
     {
	char buf[BUFFER_LEN];
	char *identify; 
	
	sprintf(buf, "<!-- ProtoMUCK %s WebLogin Interface (Written by Loki of Neon fame) -->\n", PROTOBASE);
	
	queue_ansi(d, buf);
	queue_ansi(d, "<html>");
	
	sprintf(buf, "<title>%s Web Login</title>\n", tp_muckname);
	
	queue_ansi(d, buf);
	
        sprintf(buf, "%s.%d", d->hostname, (int)current_systime);
	
	identify = strdup(buf);
	
	queue_ansi(d, "<FRAMESET ROWS=\"80%,*\">\n");
	
	sprintf(buf, "<FRAME SRC=\"webinterface?id=%s\" NAME=\"muckwindow\"  SCROLLING=\"auto\" NORESIZE>\n", identify);
	queue_ansi(d, buf);
	queue_ansi(d, "<FRAMESET COLS=\"70%,*\">\n");
	sprintf(buf, "<FRAME SRC=\"webinput?id=%s&muckinput=emp:ty\" NAME=\"input\" SCROLLING=\"no\" NORESIZE>\n", identify); 
	queue_ansi(d, buf);
	queue_ansi(d, "<FRAME SRC=\"midicontrol\">");
	queue_ansi(d, "</FRAMESET>\n"); 
	queue_ansi(d, "</FRAMESET>\n");
	queue_ansi(d, "<NOFRAMES>\n");
	queue_ansi(d, "<h1>No frames support.</h1>");
	queue_ansi(d, "We're sorry.  Your browser does not support frames.<BR>");
	queue_ansi(d, "ProtoMuck's weblogin interface requires Frames support.<br>");
	queue_ansi(d, "<BR>Try a browser such as <a href=\"http://www.netscape.com\">Netscape</a>.");
	queue_ansi(d, "</NOFRAMES>");  
	
	FREE(identify);
	d->booted = 1;
        foundit = 1;
     } else
     if (string_prefix("webinput", name))
     {
	char buf[MAX_COMMAND_LEN * 2];
	char *identify;
	char *temp;
	char *p, *pend, *q, *qend;
	struct descriptor_data *d2 = NULL;
	int found = 0;
        time_t now;

        time(&now);
        foundit = 1;
	identify = strdup(getcgivar(params, "id"));

	temp = getcgivar(params, "muckinput");
	strcat(temp,"\n");
	strcpy(buf, temp);

	if (strcmp(temp, "emp:ty"))
	 {

	for (d2 = descriptor_list; d2 && (strcmp(d2->identify,identify)); d2 = d2 -> next);

	if (d2)
	  if (!strcmp(d2->identify,identify)) {
	found = 1;

	if (!d2->raw_input)
	{
	   MALLOC(d2->raw_input, char, MAX_COMMAND_LEN);
	   d2->raw_input_at = d2->raw_input;
	}
	
	p = d2->raw_input_at;
        d2->last_time = now;
	pend = d2->raw_input + MAX_COMMAND_LEN - 1;
	for (q = buf, qend = buf + strlen(temp); q < qend; q++)
	{
	   if (*q == '\n')
	     {
		*p = '\0';
		if (p > d2->raw_input)
		  {
		     save_command(d2, d2->raw_input);
		  }
		p = d2->raw_input;
	     } else if (p < pend && isascii(*q)) {
		if (isprint(*q)) {
		   *p++ = *q;
		}
	     }
	}
	     if (p > d2->raw_input)
	      {
		 d2->raw_input_at = p;
	      } else {
		 FREE(d2->raw_input);
		 d2->raw_input = 0;
		 d2->raw_input_at = 0;
	      }
	 }
	 }
/*        if (found) { */
	sprintf(buf, "<!-- ProtoMUCK %s Input Window -->\n", PROTOBASE); 
	queue_ansi(d, buf);

	queue_ansi(d, "<FORM METHOD=GET ACTION=\"webinput\">\n");
	queue_ansi(d, "<INPUT TYPE=\"TEXT\" NAME=\"muckinput\" SIZE=50 maxlength=256>\n");
	
	sprintf(buf, "<INPUT TYPE=\"HIDDEN\" NAME=\"id\" VALUE=\"%s\">\n", identify);
	queue_ansi(d, buf);
	queue_ansi(d, "</FORM>");
	if (d2) if (d2->connected)
	{
	    sprintf(buf, "<b>Input for session:</b> %s@%s", NAME(d2->player), tp_muckname);
	    queue_ansi(d, buf);
	}       
        foundit = 1;         
	d->booted = 1;
/*      } else
	{
	   queue_ansi(d, "<b>Server has disconnected.</b><BR>");
	   queue_ansi(d, "Please reconnect.");
	} */
    } else
    if (string_prefix("midicontrol", name)) {
	char buf[BUFFER_LEN];
        sprintf(buf, "<embed src=\"%s\" name=\"muckmidi\" width=144 height=60>\n", tp_dummy_midi);
	queue_ansi(d, buf);
        foundit = 1;
    } else
    if (string_prefix("webinterface", name)) {
	FREE(d->identify);
	d->booted = 0;
	d->identify = strdup(getcgivar(params,"id"));
	d->http_login = 1;
        foundit = 1;       
	welcome_user(d);
    }
   }
   if (!foundit)
   {
    if (*name == '@') {
	name++;
	if( string_prefix("credits", name) ) {
	    queue_ansi(d, "<HTML>\r\n<HEAD><TITLE>ProtoMuck Credits</TITLE></HEAD>\r\n");
	    queue_ansi(d, "<H1>ProtoMuck Credits</H1>\r\n");
	    queue_ansi(d, "<BODY><PRE>\r\n");
	    while(*dit) {
		queue_ansi(d, *dit++);
		queue_ansi(d, "\r\n");
	    }
	    queue_ansi(d, "</PRE></BODY></HTML>\r\n");
	    d->booted = 1;
	} else if ( string_prefix("who", name) ) {
	    queue_ansi(d, "<HTML>\r\n<HEAD><TITLE>Who's on now?</TITLE></HEAD>\r\n");
	    queue_ansi(d, "<H1>Who's on now?</H1>\r\n");
	    queue_ansi(d, "<BODY><PRE>\r\n");
	    if (tp_secure_who)
		queue_ansi(d, "Connect and find out!\r\n");
	    else
		dump_users(d, "");
	    queue_ansi(d, "</PRE></BODY></HTML>\r\n");
	    d->booted = 1;
	} else if ( string_prefix("welcome", name) ) {
	    queue_ansi(d, "<HTML>\r\n<HEAD><TITLE>Welcome!</TITLE></HEAD>\r\n");
	    queue_ansi(d, "<H1>Welcome!</H1>\r\n");
	    queue_ansi(d, "<BODY><PRE>\r\n");
		cold = d->type;
		d->type = CT_MUCK;
		welcome_user(d);
		d->type = cold;
	    queue_ansi(d, "</PRE></BODY></HTML>\r\n");
	    d->booted = 1;
	} else {
	    httpd_unknown(d, name);
	}
	return;
    } else if (*name == '~') { 
	prop = ++name;
	while (*prop && (*prop != '/')) buf[prop-name] = (*(prop++));
	buf[prop-name] = '\0';
	if (*buf)
	    what = lookup_player(buf);
	else
	    what = NOTHING;
	while (*prop == '/') prop++;
    } else {
	prop = name;
	what = tp_www_root;
    }
    if ( what < 0 || what >= db_top ) {
	httpd_unknown(d, prop);
	return;
    }
    /* First check if there is an lsedit style list here */
    
    if (*prop && (Prop_Hidden(prop) || Prop_Private(prop))) {
	httpd_unknown(d, prop);
	return;
    }

    while ( (lines < 256) && (!lines || (m && *m)) ) {
	if(*prop)
	    sprintf(buf, WWWDIR "/%s#/%d", prop, ++lines);
	else
	    sprintf(buf, WWWDIR "#/%d", ++lines);
	m = get_property_class(what, buf);
#ifdef COMPRESS
	if(m && *m) m = uncompress(m);
#endif
	if( m && *m ) {
	    sprintf(buf, "%s\r\n", m);
	    queue_ansi(d, buf);
	}
    }
    if ( lines > 1 ) { 
	sprintf(buf, "<HR><font size=-1>ProtoMUCK %s - Moose, Akari (Web support by Loki)", PROTOBASE);
	queue_ansi(d, buf);
    }
    if ( lines <= 1 ) {
	/* There was no lsedit list, try a relocation */
	if(*prop) {
	    sprintf(buf, WWWDIR "/%s", prop);
	    m = get_property_class(what, buf);
	} else
	    m = get_property_class(what, WWWDIR);

	  if (*prop)
	  {
	      prpt = get_property(what, buf);
	  } else
	      prpt = get_property(what, WWWDIR);
#ifdef DISKBASE
	  if (prpt)
	      propfetch(what, prpt);
#endif 

#ifdef COMPRESS
	if(m && *m) m = uncompress(m);
#endif
	if((prpt) || (m && *m)) 
	{
		struct frame *tmpfr;

	      if (PropType(prpt) == PROP_REFTYP) /* Program reference? */
	      {
		sprintf(buf, "%d|%s|%s|%s",
		   d->descriptor, d->hostname, d->username, params);
		strcpy(match_args, buf);
		strcpy(match_cmdname, "(WWW)");
		tmpfr = interp(d->descriptor, tp_www_surfer, what, PropDataRef(prpt),
		       tp_www_root, PREEMPT, STD_HARDUID);
		if (tmpfr) {
			interp_loop(tp_www_surfer, PropDataRef(prpt), tmpfr, 1);
		}
		strcpy(match_args,"");
		strcpy(match_cmdname,"");
		sprintf(buf, "<META NAME=\"server\" VALUE=\"ProtoMUCK %s -- Moose, Akari (Web support by Loki)\">", PROTOBASE);
            d->httpdata = string_dup(buf);
	      } else
	      {

#ifdef HTTPDELAY
	    sprintf(buf, "HTTP/1.0 302 Found\r\n"
		"Status: 302 Found\r\n"
		"Location: %s\r\n"
		"Content-type: text/html\r\n\r\n"
		"<HTML>\r\n<HEAD><TITLE>302 Found</TITLE></HEAD>\r\n"
		"<H1>302 Found</H1>\r\n"
		"Your browser doesn't seem to support redirection.<P>\r\n"
		"Try clicking <A HREF=\"%s\">HERE</A>.\r\n</HTML>\r\n",
		m, m);
		d->httpdata = string_dup(buf);
#else
	    queue_ansi(d, "HTTP/1.0 302 Found\r\n"
		"Status: 302 Found\r\n"
		"Location: "
	    ); queue_ansi(d, m);
	    queue_ansi(d, "\r\nContent-type: text/html\r\n\r\n"
		"<HTML>\r\n<HEAD><TITLE>302 Found</TITLE></HEAD>\r\n"
		"<H1>302 Found</H1>\r\n"
		"Your browser doesn't seem to support redirection.<P>\r\n"
		"Try clicking <A HREF=\""
	    ); queue_ansi(d, m);
	    queue_ansi(d, "\">HERE</A>.\r\n</HTML>\r\n");
#endif
	    if (!*http)
		d->booted = 1;
	   }
	} else {
	    httpd_unknown(d, prop);
	    return;
	}
    } else {
	d->booted = 1;
    }
     }
}
#endif /* HTTPD */

void 
check_connect(struct descriptor_data * d, const char *msg)
{
    struct frame *tmpfr;
    char    command[80];
    char    user[BUFFER_LEN];
    char    password[80];
    const   char *why;
#ifdef HTTPD
    char    *name;
#endif
    dbref   player;
    time_t  now;
    int     result;

    if( tp_log_connects )
	log2filetime(CONNECT_LOG, "%2d: %s\r\n", d->descriptor, msg );

    parse_connect(msg, command, user, password);

#ifdef HTTPD
    if (d->type == CT_HTML && (d->http_login == 0)) {
	if (!strcmp(command, "GET")) {
            if (!string_prefix(user, "/webinput")) 
	    log_status(" GET: '%s' '%s' %s(%s) %s\n",
		user, "<hidden>", d->hostname, d->username,
		host_as_hex(d->hostaddr));
	    name = user;
	    while( *name == '/' ) name++;
	    httpd_get(d, name, password);
	} else if (index(msg, ':')) {
		/* Ignore http request fields */
	} else if (d->http_login == 0) {
	    log_status("XWWW: '%s' '%s' %s(%s) %s\n",
		user, "<HIDDEN>", d->hostname, d->username,
		host_as_hex(d->hostaddr));
	    queue_ansi(d,
		"<HTML>\r\n<HEAD><TITLE>400 Bad Request</TITLE></HEAD><BODY>\r\n"
		"<H1>400 Bad Request</H1>\r\n"
		"You sent a query this server doesn't understand.<P>\r\n"
		"Reason: Unknown method.<P>\r\n"
		"</BODY></HTML>\r\n"
	    );
	    d->booted = 1;
	}
      return;
   } else if (d->type == CT_HTML && (index(msg,':'))) { /* Ignore */ }
#endif
	   if (( string_prefix("connect", command) && !string_prefix("c", command) ) || !string_compare(command, "ch")) {
	player = connect_player(user, password);
	if (player == NOTHING) {
	    d->fails++;
	    queue_ansi(d, connect_fail);
          player = lookup_player(user);
          if (player > 0) {
             time(&now);
             result = get_property_value(player, "@/Failed/Count") + 1;
             SETMESG(player,"@/Failed/Host",d->hostname);
             add_property(player, "@/Failed/Time", NULL, now);
             add_property(player, "@/Failed/Count", NULL, result);
          }
	    if (d->type == CT_HTML) queue_ansi(d, "<BR>");
	    if( d->fails >= 3 ) d->booted = 1;
	    log_status("FAIL: %2d %s pw '%s' %s(%s) %s\n",
		d->descriptor, user, "<hidden>",
		d->hostname, d->username,
		host_as_hex(d->hostaddr));

	} else if ( (why=reg_user_is_suspended( player )) ) {
	    queue_ansi(d,"\r\n" );
	    queue_ansi(d,"You are temporarily suspended: " );
	    queue_ansi(d,why );
	    queue_ansi(d,"\r\n" );
	    queue_ansi(d,"Please contact " );
	    queue_ansi(d, tp_reg_email );
	    queue_ansi(d," for assistance if needed.\r\n" );
	    log_status("*LOK: %2d %s %s(%s) %s %s\n",
		d->descriptor, unparse_object(player, player),
		d->hostname, d->username,
		host_as_hex(d->hostaddr), why);
	    d->booted = 1;
	} else if (reg_user_is_barred( d->hostaddr, player ) == TRUE) {
	    char buf[ 1024 ];
	    sprintf( buf, CFG_REG_MSG2, tp_reg_email );
	    queue_ansi( d, buf );
	    log_status("*BAN: %2d %s %s(%s) %s\n",
		d->descriptor, unparse_object(player, player),
		d->hostname, d->username,
		host_as_hex(d->hostaddr));
	    d->booted = 1;
	} else if ((wizonly_mode ||
		 (tp_playermax && con_players_curr >= tp_playermax_limit)) &&
		!TMage(player)
	    ) {
		if (wizonly_mode) {
		    queue_ansi(d, "Sorry, but the game is in maintenance mode currently, and only wizards are allowed to connect.  Try again later.");
		} else {
		    queue_ansi(d, BOOT_MESG);
		}
		queue_ansi(d, "\r\n");
		d->booted = 1;
	} else {
          if (!string_compare(command, "ch") && !Arch(player))
          {
                queue_ansi(d, "Only wizards can connect hidden.") ;
                d->fails++;
	          log_status("FAIL[CH]: %2d %s pw '%s' %s(%s) %s\n",
		      d->descriptor, user, "<hidden>",
		      d->hostname, d->username,
     		      host_as_hex(d->hostaddr));
        } else {
	    log_status("CONN: %2d %s %s(%s) %s\n",
		d->descriptor, unparse_object(player, player),
		d->hostname, d->username,
		host_as_hex(d->hostaddr));
	    d->connected = 1;
	    d->connected_at = current_systime;
	    d->player = player;
		update_desc_count_table();
                remember_player_descr(player, d->descriptor);
          if (!string_compare(command, "ch"))
          {
             FLAG2(player) |= F2HIDDEN;
          } else {
             FLAG2(player) &= ~F2HIDDEN;
          }
	    if (d->type == CT_PUEBLO) 
	    {
	       FLAG2(player) |= F2PUEBLO;
	    }
	    else
	    {
	       FLAG2(player) &= ~F2PUEBLO;
	    }

	    if (d->type == CT_HTML)
	    {
	       FLAG2(player) |= F2HTML;
	    }
	    else
	    {
	       FLAG2(player) &= ~F2HTML;
	    }
	   
	    /* cks: someone has to initialize this somewhere. */
	    DBFETCH(d->player)->sp.player.block = 0;
	    spit_file(player, MOTD_FILE);
	    if ( TArch(player) ) {
		char buf[ 128 ];
		int count = hop_count();
		sprintf( buf, CNOTE
		    "There %s %d registration%s in the hopper.",
		    (count==1)?"is":"are", count, (count==1)?"":"s" );
		anotify( player, buf );
	    }
	    interact_warn(player);
	    if (sanity_violated && TMage(player)) {
		notify(player, MARK "WARNING!  The DB appears to be corrupt!");
	    }
	    announce_connect(d->descriptor, player);
	}
     }
    } else if (string_prefix("help", command) ) { /* Connection Help */
	log_status("HELP: %2d %s(%s) %s %d cmds\n",
	    d->descriptor, d->hostname, d->username,
	    host_as_hex(d->hostaddr), d->commands);
	help_user(d);
    } else if (string_prefix("request", command) ) {
	/* Guests online should already be checked out ok */
	if( reg_site_is_barred(d->hostaddr) == TRUE ) {
	    queue_ansi(d,"Sorry, but we are not accepting requests from your site.\r\n");
	    d->booted = 1;
	} else {
  	    log_status("RQST: %2d %s(%s) %s %d cmds\n",
	      d->descriptor, d->hostname, d->username,
	      host_as_hex(d->hostaddr), d->commands);
          if( request(-1, d, msg) )
             d->booted = 1;
      }
    } else {
      if (Typeof(tp_login_huh_command) == TYPE_PROGRAM) {
         char   *full_command, xbuf[BUFFER_LEN];

         full_command = strcpy(xbuf, msg);
         for (; *full_command && !isspace(*full_command); full_command++);
         if (*full_command)
            full_command++;
         strcpy(match_cmdname, command);
         strcpy(match_args, full_command);
         tmpfr = interp(d->descriptor, -1, -1, tp_login_huh_command, (dbref) -5, FOREGROUND, STD_REGUID);
	   if (tmpfr) {
		interp_loop(-1, tp_login_huh_command, tmpfr, 0);
	   }
      } else {
           log_status("TYPO: %2d %s(%s) %s '%s' %d cmds\n",
             d->descriptor, d->hostname, d->username,
             host_as_hex(d->hostaddr), command, d->commands);
           welcome_user(d);
      }
    }
}
void 
parse_connect(const char *msg, char *command, char *user, char *pass)
{
    int cnt;
    char   *p;

    while (*msg && isascii(*msg) && isspace(*msg)) /* space */
	msg++;
    p = command;
    cnt = 0;
    while (*msg && isascii(*msg) && !isspace(*msg) && (++cnt < 80))
	*p++ = *msg++; 
    *p = '\0'; 
    cnt = 0;
    while (*msg && isascii(*msg) && isspace(*msg)) /* space */
	msg++;
    p = user;
    cnt = 0;
    while (*msg && isascii(*msg) && !isspace(*msg) && (++cnt < BUFFER_LEN))
	*p++ = *msg++;
    *p = '\0';
    while (*msg && isascii(*msg) && isspace(*msg)) /* space */
	msg++;
    p = pass;
    cnt = 0;
    while (*msg && isascii(*msg) /* && !isspace(*msg) */ && (++cnt < 80))
	*p++ = *msg++;
    *p = '\0';
}


int 
boot_off(dbref player)
{
    int* darr;
    int dcount;
    struct descriptor_data *d;
    struct descriptor_data *last = NULL;

	darr = get_player_descrs(player, &dcount);
	if (darr) {
        last = descrdata_by_descr(darr[0]);
	}


    if (last) {
	process_output(last);
	last->booted = 1;
	/* shutdownsock(last); */
	return 1;
    }
    return 0;
}

void 
boot_player_off(dbref player)
{
    int di;
    int* darr;
    int dcount;
    struct descriptor_data *d;
 
	darr = get_player_descrs(player, &dcount);

    for (di = 0; di < dcount; di++) {
        d = descrdata_by_descr(darr[di]);
        if (d) {
            d->booted = 1;
        }
    }
}

void 
boot_player_off_too(dbref player)
{
    int di;
    int* darr;
    int dcount;
    struct descriptor_data *d;
 
	darr = get_player_descrs(player, &dcount);

    for (di = 0; di < dcount; di++) {
        d = descrdata_by_descr(darr[di]);
        if (d) {
#ifdef HTTPDELAY
		if(d->httpdata) {
		    queue_ansi(d, d->httpdata);
		    free((void *)d->httpdata);
		    d->httpdata = NULL;
		}
#endif
		process_output(d);
            shutdownsock(d);
        }
    }
}

void
close_sockets(const char *msg)
{
    struct descriptor_data *d, *dnext;

    for (d = descriptor_list; d; d = dnext) {
	dnext = d->next;
	writesocket(d->descriptor, msg, strlen(msg));
	clearstrings(d);                        /** added to clean up **/
	if (shutdown(d->descriptor, 2) < 0)
	    perror("shutdown");
	closesocket(d->descriptor);
	freeqs(d);                              /****/
	*d->prev = d->next;                     /****/
	if (d->next)                            /****/
	    d->next->prev = d->prev;            /****/
	if (d->hostname)                        /****/
	    free((void *)d->hostname);    /****/
	if (d->username)                        /****/
	    free((void *)d->username);    /****/
	FREE(d);                                /****/
	ndescriptors--;                         /****/
    }
    closesocket(sock);
    closesocket(sock_html);
    closesocket(sock_pueblo);
}

struct descriptor_data *
get_descr(int descr, int player)
{
    struct descriptor_data *next=descriptor_list;

    while(next) {
	if ( ((player > 0) && (next->player == player)) ||
	     ((descr  > 0) && (next->descriptor == descr)) ) return next;
	next = next->next;
    }
    return NULL;
}

void
do_dinfo(dbref player, const char *arg)
{
    struct descriptor_data *d;
    int who, descr;
    char *ctype = NULL;
    time_t now;

    if (!Wiz(player)) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }
    if(!*arg) {
	anotify(player, CINFO "Usage: dinfo descriptor  or  dinfo name");
	return;
    }
    (void) time(&now);
    if(strcmp(arg, "me"))
	who = lookup_player(arg);
    else who = player;
    descr = atoi(arg);
    if (!( d = get_descr(descr, who))) {
	anotify(player, CINFO "Invalid descriptor or user not online.");
	return;
    }

    switch(d->type) {
	case CT_MUCK:
		ctype = "muck"; break;
	case CT_HTML:
		ctype = "html"; break;
	case CT_PUEBLO:
		ctype = "pueblo"; break;
	default:
		ctype = "unknown";
    }

    anotify_fmt(player, "%s" AQUA " descr " YELLOW "%d" BLUE " (%s)",
	d->connected ? ansi_unparse_object(player, d->player) : GREEN
	"[Connecting]", d->descriptor, ctype
    );

    if(Arch(player))
	anotify_fmt(player, AQUA "Host: " CYAN "%s" BLUE "@" CYAN "%s",
		d->username, d->hostname);
    else
	anotify_fmt(player, AQUA "Host: " CYAN "%s", d->hostname);

    anotify_fmt(player, AQUA "IP: " CYAN "%s" YELLOW "(%d) " NAVY "%X",
	host_as_hex(d->hostaddr), d->port, d->hostaddr);

    anotify_fmt(player, VIOLET "Online: " PURPLE "%s  " BROWN "Idle: "
	YELLOW "%s  " CRIMSON "Commands: " RED "%d",
	time_format_1(now - d->connected_at),
	time_format_2(now - d->last_time), d->commands
    );

    if (d->connected)
	anotify_fmt(player, AQUA "Location: %s",
	    ansi_unparse_object(player, DBFETCH(d->player)->location));
}

void
do_dwall(dbref player, const char *name, const char *msg)
{
    struct descriptor_data *d;
    int who, descr;
    char buf[BUFFER_LEN];

    if (!Wiz(player) || (POWERS(player) & POW_ANNOUNCE)) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }
    if(!*name || !*msg) {
	anotify(player, CINFO "Usage: dwall descriptor=msg  or  dwall name=msg");
    }
    if(strcmp(name, "me"))
	who = lookup_player(name);
    else who = player;
    descr = atoi(name);
    if (!( d = get_descr(descr, who))) {
	anotify(player, CINFO "Invalid descriptor or user not online.");
	return;
    }

    switch(msg[0]) {
	case ':':
	case ';':
		sprintf(buf, MARK "%s %s\r\n", NAME(player), msg+1);
		break;
	case '#':
		sprintf(buf, MARK "%s\r\n", msg+1);
		break;
	case '!':
		sprintf(buf, "%s\r\n", msg+1);
		break;
	default:
		sprintf(buf, MARK "%s tells you, \"%s\"\r\n", NAME(player), msg);
		break;
    }

    queue_ansi(d, buf);
    if(!process_output(d)) d->booted = 1;
    anotify_fmt(player, CSUCC "Message sent to descriptor %d.", d->descriptor);
}

void
do_dboot(dbref player, const char *name)
{
    struct descriptor_data *d;
    int who, descr;

    if (!Wiz(player) || (POWERS(player) & POW_BOOT)) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	return;
    }
    if(!*name) {
	anotify(player, CINFO "Usage: dboot descriptor  or  dboot name");
    }
    if(strcmp(name, "me"))
	who = lookup_player(name);
    else who = player;
    descr = atoi(name);
    if (!( d = get_descr(descr, who))) {
	anotify(player, CINFO "Invalid descriptor or user not online.");
	return;
    }

    d->booted = 1;
    anotify(player, CSUCC "Booted.");
}

void
do_armageddon(dbref player, const char *msg)
{
    char buf[BUFFER_LEN];
    if (!Arch(player)) {
	anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
	log_status("SHAM: Armageddon by %s\n", unparse_object(player, player));
	return;
    }
    if( *msg == '\0' || strcmp(msg, tp_muckname))
    {
	anotify(player, CINFO "Usage: @armageddon muckname" );
	notify(player, "***WARNING*** All data since last save will be lost!" );
	return;
    }
    sprintf(buf, "\r\nImmediate shutdown by %s.\r\n", NAME(player));
    log_status("DDAY: %s(%d): %s\n",
	    NAME(player), player, msg);
    fprintf(stderr, "DDAY: %s(%d)\n",
	    NAME(player), player);
    close_sockets(buf);

#ifdef SPAWN_HOST_RESOLVER
    kill_resolver();
#endif

    exit(1);
}

int
request( dbref player, struct descriptor_data *d, const char *msg )
{
    char    command[80];
    char    user[80];
    char    password[80];
    char    email[80];
    char    firstname[80];
    char    lastname[80];
    char    fullname[161];
    char    *jerk;

    if ((player > 0) && !Guest(player) && !Mage(player)) {
	anotify(player, CFAIL "Only guests can request new characters.");
	return 0;
    }

    if( !d )
	if(!( d = get_descr(0, player) )) return 0;

    if( tp_online_registration ) {
	parse_connect(msg, command, user, password);
	parse_connect(password, email, firstname, lastname);
	if(!*user) {
	    queue_ansi(d, "To request a character type:\r\n\r\n"
		"   request <char name> <e-mail> <your name>   (Don't type the <> signs)\r\n\r\n"
		"<char name> is the name you'd like for your character\r\n"
		"   <e-mail> is your email address, ie: user@host.com\r\n"
		"<your name> is your first and last name in real life\r\n"
	    );
	    return 0;
	}
	if (!ok_player_name(user)) {
	    queue_ansi(d,
		"Sorry, that name is invalid or in use.  Try another?\r\n"
	    );
	} else if (!strchr(email,'@') || !strchr(email,'.')) {
	    queue_ansi(d,
		"That doesn't look like an email address.  Type just 'request' for help.\r\n"
	    );
	} else if ( *firstname == '\0' ) {
	    queue_ansi(d,
		"You forgot your name.  Type just 'request' for help.\r\n"
	    );
	} else if ( *lastname == '\0' ) {
	    queue_ansi(d,
		"You forgot your last name.\r\n"
	    );
	} else if ( strchr(firstname,'\'')
		 || strchr(lastname,'\'')
		 || strchr(email,'\'')
		 || strchr(user,'\'')  ) {
	    queue_ansi(d,
		"Please don't use single quotes in names.\r\n"
	    );
	} else if ( strchr(firstname,'`')
		 || strchr(lastname,'`')
		 || strchr(email,'`')
		 || strchr(user,'`')  ) {
	    queue_ansi(d,
		"Please don't use backquotes in names.\r\n"
	    );
	} else if ( strchr(firstname,'\"')
		 || strchr(lastname,'\"')
		 || strchr(email,'\"')
		 || strchr(user,'\"')  ) {
	    queue_ansi(d,
		"Please don't use quotes in names.\r\n"
	    );
	} else if ( strchr(firstname,'\\')
		 || strchr(lastname,'\\')
		 || strchr(email,'\\')
		 || strchr(user,'\\')  ) {
	    queue_ansi(d,
		"Please don't use backslashes in names.\r\n"
	    );
	} else if ( strchr(email,'<')
		 || strchr(email,'>')
		 || strchr(email,'/')
		 || strchr(email,';')
		 || strchr(email,'!')
		 || strchr(email,'|')
		 || strchr(email,'%')
		 || strchr(email,'&')
		 || strchr(email,':')  ) {
	    queue_ansi(d,
		"There are unacceptable characters in your email address.\r\n"
	    );
	} else {
	    jerk = reg_email_is_a_jerk(email) ? "##JERK## " : "";

#ifdef SPAWN_HOST_RESOLVER
	    resolve_hostnames(); /* See if we can get a real site name */
#endif

	    log2filetime( LOG_HOPPER, "'%s' %s'%s' '%s %s' %s(%s) %s\r\n",
		user, jerk, email, firstname, lastname,
		d->hostname, d->username, host_as_hex(d->hostaddr)
	    );

	    log_status(
		"*REG: %2d '%s' %s'%s' '%s %s' %s(%s) %s\n",
		d->descriptor, user, jerk, email, firstname, lastname,
		d->hostname, d->username, host_as_hex(d->hostaddr)
	    );

	    if(tp_fast_registration && !(*jerk) ) {
		sprintf(fullname, "%s %s", firstname, lastname);
		email_newbie(user, email, fullname);
		queue_ansi(d, "Your request has been processed.\r\n"
		    "Your player's password has been e-mailed to you.\r\n"
		);
		wall_arches( MARK
		    "New request filed in newbie hopper and processed." );
	    } else {
		queue_ansi(d, "Your request has been filed.\r\n"
		    "Turnaround is generally less than 24 hours.\r\n"
		    "Your player's password will be sent to you via e-mail.\r\n"
		);
		wall_arches( MARK "New request filed in newbie hopper." );
	    }
	    return 1;
	}
    } else queue_ansi(d, "Online registration is not open now.\r\n" );
    return 0;
}

void 
emergency_shutdown(void)
{
    close_sockets("\r\nEep!  Emergency shutdown.  Be back soon!");

#ifdef SPAWN_HOST_RESOLVER
    kill_resolver();
#endif

}


void 
dump_users(struct descriptor_data * e, char *user)
{
    struct descriptor_data *d;
    int     wizard, arch, players, ansi;
    time_t  now;
    char    buf[2048], tbuf[BUFFER_LEN];
    char    pbuf[64];
    const char    *p;
    static  players_max = 0;
#ifdef COMPRESS
    extern const char *uncompress(const char *);
#endif

    wizard = e->connected && (Mage(e->player) || (POWERS(e->player) & POW_EXPANDED_WHO));

    if( e->connected ) {
	ansi = ( FLAGS(e->player) & CHOWN_OK ) ? 1 : 0 ;
    } else ansi = 0;

    if ((e->type == CT_PUEBLO)) queue_ansi(e, "<code>");
    if ((e->type == CT_HTML)) queue_ansi(e, "<pre>");

    while (*user && isspace(*user)) user++;

#ifdef MORTWHO
    if( *user == '!' ) { user++; } else { wizard = 0; }
#else
    if( *user == '!' ) { user++; wizard = 0; }
#endif

    arch = wizard && e->connected && Arch(e->player);

    while (*user && isspace(*user)) user++;

    if (!*user) user = NULL;

    (void) time(&now);
    if (wizard) {
      if(ansi)
	queue_ansi(e,
		ANSIRED    "DS "
		ANSIGREEN "Player Name            "
		ANSICYAN "Room    " 
		ANSIPURPLE "On For " 
		ANSIYELLOW "Idle "
		ANSIBLUE "Host"
		ANSINORMAL "\r\n");
      else
	queue_ansi(e, "DS Player Name            Room    On For Idle Host\r\n");
    } else {
	if (tp_who_doing) {
	    if( (p = get_property_class((dbref)0, "_poll")) )
	    {
#ifdef COMPRESS
		p = uncompress(p);
#endif
		if(ansi)
		  sprintf( buf,
		    ANSIGREEN "Player Name           "
		    ANSIPURPLE "On For "
		    ANSIYELLOW "Idle  "
		    ANSICYAN "%-.43s"
		    ANSINORMAL "\r\n",p);
		else
		  sprintf( buf,
		    "Player Name           On For Idle  %-.43s\r\n",p);
		queue_ansi(e, buf);
	    } else
		if(ansi)
		  queue_ansi(e,
		    ANSIGREEN "Player Name           "
		    ANSIPURPLE "On For "
		    ANSIYELLOW "Idle  "
		    ANSICYAN "Doing..."
		    ANSINORMAL "\r\n");
		else
		  queue_ansi(e,
		    "Player Name           On For Idle  Doing...\r\n" );
	} else
	    if(ansi)
		queue_ansi(e,
		  ANSIGREEN "Player Name           "
		  ANSIPURPLE "On For "
		  ANSIYELLOW "Idle"
		  ANSINORMAL "\r\n");
	    else
		queue_ansi(e, "Player Name           On For Idle\r\n");
    }
    if (e->type == CT_PUEBLO) queue_ansi(e, "<code>");

    d = descriptor_list;
    players = 0;
    while (d) {
	if ((!user || (d->connected && string_prefix(NAME(d->player), user)))
        && (!tp_who_hides_dark || wizard || (!(FLAGS(d->player) & DARK) && !(FLAG2(d->player) & F2HIDDEN)))) {
	    if (d->connected && (d->player < 0 || d->player >= db_top)) {
		players++;
		sprintf( buf, "<???> desc %d #%d", d->descriptor, d->player );
	    } else if (wizard) {
		players++;
		if(d->connected)
		    sprintf(pbuf, "%s(%s)", NAME(d->player), unparse_flags(d->player, tbuf));
		else
		  if ((d->type == CT_HTML) && (d->http_login == 0))
		    sprintf(pbuf, "[WWW]"); else
		    sprintf(pbuf, "[Connecting]");

		sprintf(pbuf, "%.*s", PLAYER_NAME_LIMIT + 5, pbuf);

		sprintf(buf, "%s%-3d%s%-*s %s%5d %s%9s%s%s%4s %s%s%s%s%s\r\n",
			ansi ? ANSIRED : "",
			d->descriptor,  
			ansi ? ANSIGREEN : "",
			PLAYER_NAME_LIMIT + 5, pbuf,
			ansi ? ANSICYAN : "",
			d->connected ? DBFETCH(d->player)->location : -1,
			ansi ? ANSIPURPLE : "",
			time_format_1(now - d->connected_at),
			d->connected ? ((FLAGS(d->player) & INTERACTIVE) ? "*" : " ")
				: " ",
			ansi ? ANSIYELLOW : "",
			time_format_2(now - d->last_time),
			ansi ? ANSIBLUE : "",
			arch ? d->username : "",
			arch ? "@" : "",
			d->hostname,
			ansi ? ANSINORMAL : "" );
	    } else if (d->connected) {
		players++;

		if (tp_who_doing) {
		    sprintf(buf, "%s%-*s %s%10s %s%4s%s %s%-.45s%s\r\n",
			ansi ? ANSIGREEN : "",
			PLAYER_NAME_LIMIT + 1,
			NAME(d->player),
			ansi ? ANSIPURPLE : "",
			time_format_1(now - d->connected_at),
			ansi ? ANSIYELLOW : "",
			time_format_2(now - d->last_time),
			((FLAGS(d->player) & INTERACTIVE) ? "*" : " "),
			ansi ? ANSICYAN : "",
			GETDOING(d->player) ?
#ifdef COMPRESS
			    uncompress(GETDOING(d->player))
#else
			    GETDOING(d->player)
#endif
			    : "",
			ansi ? ANSINORMAL : "" );
		} else {
		    sprintf(buf, "%s%-*s %s%10s %s%4s%s%s\r\n",
			ansi ? ANSIGREEN : "",
			PLAYER_NAME_LIMIT + 1,
			NAME(d->player),
			ansi ? ANSIPURPLE : "",
			time_format_1(now - d->connected_at),
			ansi ? ANSIYELLOW : "",
			time_format_2(now - d->last_time),
			((FLAGS(d->player) & INTERACTIVE) ? "*" : " "),
			ansi ? ANSINORMAL : "" );
		}
	    }
	    if( e->type == CT_PUEBLO) strcat(buf, "<code>");
	    if( d->connected || wizard )
		queue_ansi(e, buf);
	}
	d = d->next;
    }
    if (players > players_max)  players_max = players;
    sprintf(buf, "%s%d player%s %s connected.  %s(Max was %d)%s\r\n",
	ansi ? ANSIBLUE : "",
	players,
	(players == 1) ? "" : "s",
	(players == 1) ? "is" : "are",
	ansi ? ANSIYELLOW : "",
	players_max,
	ansi ? ANSINORMAL : ""
    );
    queue_ansi(e, buf);
     if ((e->type == CT_PUEBLO)) queue_ansi(e, 
       "</code>"); 
     if ((e->type == CT_HTML)) queue_ansi(e, "</pre>");
}

void 
pdump_who_users(int c, char *user)
{
   struct descriptor_data *e;

   e = descriptor_list;
   while(e && e->descriptor != c) {
      e = e->next;
   }
   if(e->descriptor == c)
      dump_users(e, user);
}

char   *
time_format_1(time_t dt)
{
    register struct tm *delta;
    static char buf[64];

    delta = gmtime(&dt);

    if (delta->tm_yday > 0)
	sprintf(buf, "%dd %02d:%02d",
		delta->tm_yday, delta->tm_hour, delta->tm_min);
    else
	sprintf(buf, "%02d:%02d",
		delta->tm_hour, delta->tm_min);
    return buf;
}

char   *
time_format_2(time_t dt)
{
    register struct tm *delta;
    static char buf[64];

    delta = gmtime(&dt);

    if (delta->tm_yday > 0)
	sprintf(buf, "%dd", delta->tm_yday);
    else if (delta->tm_hour > 0)
	sprintf(buf, "%dh", delta->tm_hour);
    else if (delta->tm_min > 0)
	sprintf(buf, "%dm", delta->tm_min);
    else
	sprintf(buf, "%ds", delta->tm_sec);
    return buf;
}


void
announce_puppets(dbref player, const char *msg, const char *prop)
{
    dbref what, where;
    const char *ptr, *msg2;
    char buf[BUFFER_LEN];

    for (what = 0; what < db_top; what++) {
	if (Typeof(what) == TYPE_THING && (FLAGS(what) & ZOMBIE)) {
	    if (OWNER(what) == player) {
		where = getloc(what);
		if ((!Dark(where)) &&(!Dark(player))&&(!Dark(what))) {
		    msg2 = msg;
		    if ((ptr = (char *)get_property_class(what, prop)) && *ptr)
			msg2 = ptr;
		    sprintf(buf, CMOVE "%.512s %.3000s", PNAME(what), msg2);
		    anotify_except(DBFETCH(where)->contents, what, buf, what);
		}
	    }
	}
    }
}

void 
announce_connect(int descr, dbref player)
{
    dbref   loc;
    char    buf[BUFFER_LEN];
    struct match_data md;
    dbref   exit;

    if ((loc = getloc(player)) == NOTHING)
	return;

	total_loggedin_connects++;

#ifdef RWHO
    if (tp_rwho) {
	time(&tt);
	sprintf(buf, "%d@%s", player, tp_muckname);
	rwhocli_userlogin(buf, NAME(player), tt);
    }
#endif

    if(Guest(player)) {
	FLAGS(player) &= ~CHOWN_OK;
	if (tp_online_registration)
	{
	  notify(player, MARK "You can request your own character while online.");
	  notify(player, "    Type 'request' to see what to do.");
	}
    }

    /*
     * See if there's a connect action.  If so, and the player is the first to
     * connect, send the player through it.  If the connect action is set
     * sticky, then suppress the normal look-around.
     */

    /* queue up all _connect programs referred to by properties */
    envpropqueue(descr, player, getloc(player), NOTHING, player, NOTHING,
		 "@connect", "Connect", 1, 1);
    envpropqueue(descr, player, getloc(player), NOTHING, player, NOTHING,
		 "@oconnect", "Oconnect", 1, 0);
    envpropqueue(descr, player, getloc(player), NOTHING, player, NOTHING,
		 "~connect", "Connect", 1, 1);
    envpropqueue(descr, player, getloc(player), NOTHING, player, NOTHING,
		 "~oconnect", "Oconnect", 1, 0);
#ifdef ALLOW_OLD_TRIGGERS
    envpropqueue(descr, player, getloc(player), NOTHING, player, NOTHING,
		 "_connect", "Connect", 1, 1);
    envpropqueue(descr, player, getloc(player), NOTHING, player, NOTHING,
		 "_oconnect", "Oconnect", 1, 0);
#endif

    exit = NOTHING;
    if (online(player) == 1) {
	init_match(descr, player, "connect", TYPE_EXIT, &md);  /* match for connect */
	md.match_level = 1;
	match_all_exits(&md);
	exit = match_result(&md);
	if (exit == AMBIGUOUS) exit = NOTHING;
    }

    if (exit != NOTHING) do_move(descr, player, "connect", 1);

    if (exit == NOTHING || !( (Typeof(exit) == TYPE_EXIT) && (FLAGS(exit)&STICKY) ) )   {
	if (can_move(descr, player, "look", 1)) {
	    do_move(descr, player, "look", 1);
	} else {
	    do_look_around (descr, player);
	}
    }

    if ((loc = getloc(player)) == NOTHING)
	return;

    if (!tp_quiet_connects) {
       if ((!Dark(player)) && (!Dark(loc)) && (!Hidden(player))) {
         if (online(player) == 1)
	      sprintf(buf, CMOVE "%s has connected.", PNAME(player));
         else
	      sprintf(buf, CMOVE "%s has reconnected.", PNAME(player));
	   anotify_except(DBFETCH(loc)->contents, player, buf, player);
       }

       if (online(player) == 1 && (!Hidden(player))) {
	   announce_puppets(player, "wakes up.", "_/pcon");
       }
    }

    ts_useobject(player);
    return;
}

void 
announce_disconnect(struct descriptor_data *d)
{
    dbref   player = d->player;
    dbref   loc;
    char    buf[BUFFER_LEN];
    object_flag_type f=0;

    if ((loc = getloc(player)) == NOTHING)
	return;

	total_loggedin_connects--;

#ifdef RWHO
    if (tp_rwho) {
	sprintf(buf, "%d@%s", player, tp_muckname);
	rwhocli_userlogout(buf);
    }
#endif

    if (dequeue_prog(player, 2))
	anotify(player, CINFO "Foreground program aborted.");

    if ((!Dark(player)) && (!Dark(loc)) && (!Hidden(player))) {
      if (online(player) == 1)
 	   sprintf(buf, CMOVE "%s has disconnected.", PNAME(player));
      else
         sprintf(buf, CMOVE "%s has dropped a connection.", PNAME(player));
	anotify_except(DBFETCH(loc)->contents, player, buf, player);
    }

    d->connected = 0;
    d->player = NOTHING;
    forget_player_descr(player, d->descriptor);
    update_desc_count_table();

    /* trigger local disconnect action */
    if (!online(player)) {
	/* queue up all _connect programs referred to by properties */
	envpropqueue(d->descriptor, player, getloc(player), NOTHING, player, NOTHING,
		"@disconnect", "Disconnect", 1, 1);
	envpropqueue(d->descriptor, player, getloc(player), NOTHING, player, NOTHING,
		"@odisconnect", "Odisconnect", 1, 0);
	envpropqueue(d->descriptor, player, getloc(player), NOTHING, player, NOTHING,
		"~disconnect", "Disconnect", 1, 1);
	envpropqueue(d->descriptor, player, getloc(player), NOTHING, player, NOTHING,
		"~odisconnect", "Odisconnect", 1, 0);
#ifdef ALLOW_OLD_TRIGGERS
	envpropqueue(d->descriptor, player, getloc(player), NOTHING, player, NOTHING,
		"_disconnect", "Disconnect", 1, 1);
	envpropqueue(d->descriptor, player, getloc(player), NOTHING, player, NOTHING,
		"_odisconnect", "Odisconnect", 1, 0);
#endif
	if (can_move(d->descriptor, player, "disconnect", 1)) {
	    do_move(d->descriptor, player, "disconnect", 1);
	}
	gui_dlog_freeall_descr(d->descriptor);

      if (!Hidden(player)) {
 	   announce_puppets(player, "falls asleep.", "_/pdcon");
      }
    }
}

#ifdef MUD_ID
void
do_setuid(char *name)
{
#include <pwd.h>
    struct passwd *pw;

    if ((pw = getpwnam(name)) == NULL) {
	log_status("can't get pwent for %s\n", name);
	exit(1);
    }
    if (setuid(pw->pw_uid) == -1) {
	log_status("can't setuid(%d): ", pw->pw_uid);
	perror("setuid");
	exit(1);
    }
}

#endif				/* MUD_ID */

struct descriptor_data *descr_count_table[FD_SETSIZE];
int current_descr_count = 0;

void
init_descr_count_lookup()
{
    int i;
    for (i = 0; i < FD_SETSIZE; i++) {
	descr_count_table[i] = NULL;
    }
}

void
update_desc_count_table()
{
    int c;
    struct descriptor_data *d;

    current_descr_count = 0;
    for (c = 0, d = descriptor_list; d; d = d->next)
    {
        if (d->connected)
	{
	    d->con_number = c + 1;
	    descr_count_table[c++] = d;
	    current_descr_count++;
	}
    }
}

struct descriptor_data *
descrdata_by_count(int c)
{
    c--;
    if (c >= current_descr_count || c < 0) {
        return NULL;
    }
    return descr_count_table[c];
}

struct descriptor_data *descr_lookup_table[FD_SETSIZE];

void
init_descriptor_lookup()
{
    int i;
    for (i = 0; i < FD_SETSIZE; i++) {
	descr_lookup_table[i] = NULL;
    }
}

int*
get_player_descrs(dbref player, int* count)
{
	int* darr;

	if (Typeof(player) == TYPE_PLAYER) {
		*count = DBFETCH(player)->sp.player.descr_count;
	    darr = DBFETCH(player)->sp.player.descrs;
		if (!darr) {
			*count = 0;
		}
		return darr;
	} else {
		*count = 0;
		return NULL;
	}
}

void
remember_player_descr(dbref player, int descr)
{
    int  count = DBFETCH(player)->sp.player.descr_count;
    int* arr   = DBFETCH(player)->sp.player.descrs;

    if (Typeof(player) != TYPE_PLAYER)
       return;

    if (!arr) {
        arr = (int*)malloc(sizeof(int));
        arr[0] = descr;
        count = 1;
    } else {
        arr = (int*)realloc(arr,sizeof(int) * (count+1));
        arr[count] = descr;
        count++;
    }
    DBFETCH(player)->sp.player.descr_count = count;
    DBFETCH(player)->sp.player.descrs = arr;
}

void
forget_player_descr(dbref player, int descr)
{
    int  count = DBFETCH(player)->sp.player.descr_count;
    int* arr   = DBFETCH(player)->sp.player.descrs;

    if (Typeof(player) != TYPE_PLAYER)
       return;

    if (!arr) {
        count = 0;
    } else if (count > 1) {
	int src, dest;
        for (src = dest = 0; src < count; src++) {
	    if (arr[src] != descr) {
		if (src != dest) {
		    arr[dest] = arr[src];
		}
		dest++;
	    }
	}
	if (dest != count) {
	    count = dest;
	    arr = (int*)realloc(arr,sizeof(int) * count);
        }
    } else {
        free((void*)arr);
        arr = NULL;
        count = 0;
    }
    DBFETCH(player)->sp.player.descr_count = count;
    DBFETCH(player)->sp.player.descrs = arr;
}

void
remember_descriptor(struct descriptor_data *d)
{
    if (d) {
	descr_lookup_table[d->descriptor] = d;
    }
}

void
forget_descriptor(struct descriptor_data *d)
{
    if (d) {
	descr_lookup_table[d->descriptor] = NULL;
    }
}

struct descriptor_data *
lookup_descriptor(int c)
{
    if (c >= FD_SETSIZE || c < 0) {
        return NULL;
    }
    return descr_lookup_table[c];
}

struct descriptor_data *
descrdata_by_descr(int i)
{
    return lookup_descriptor(i);
}


/*** JME ***/
int 
online(dbref player)
{
    return DBFETCH(player)->sp.player.descr_count;
}

int 
pcount()
{
    return total_loggedin_connects;
}

int 
pidle(int c)
{
    struct descriptor_data *d;
    long    now;

    d = descrdata_by_count(c);

    (void) time(&now);
    if (d) {
	return (now - d->last_time);
    }

    return -1;
}

dbref 
pdbref(int c)
{
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) {
	return (d->player);
    }

    return NOTHING;
}

int 
pontime(int c)
{
    struct descriptor_data *d;
    long    now;

    d = descrdata_by_count(c);

    (void) time(&now);
    if (d) {
	return (now - d->connected_at);
    }

    return -1;
}

char   *
phost(int c)
{
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) {
	return ((char *) d->hostname);
    }

    return (char *) NULL;
}

char   *
puser(int c)
{
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) {
	return ((char *) d->username);
    }

    return (char *) NULL;
}

/*** Foxen ***/
int
least_idle_player_descr(dbref who)
{
	struct descriptor_data *d;
	struct descriptor_data *best_d = NULL;
	int dcount, di;
	int* darr;
	long best_time = 0;

	darr = get_player_descrs(who, &dcount);
	for (di = 0; di < dcount; di++) {
		d = descrdata_by_descr(darr[di]);
		if (d && (!best_time || d->last_time > best_time)) {
			best_d = d;
			best_time = d->last_time;
		}
	}
	if (best_d) {
		return best_d->con_number;
	}
	return 0;
}


char   *
pipnum(int c)
{
    static char ipnum[40];
    const char *p;
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) {
	p = host_as_hex(d->hostaddr);
	strcpy(ipnum, p);
	return ((char *) ipnum);
    }

    return (char *) NULL;
}

char   *
pport(int c)
{
    static char port[40];
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) {
	sprintf(port, "%d", d->port);
	return ((char *) port);
    }

    return (char *) NULL;
}

/*** Foxen ***/
int
pfirstconn(dbref who)
{
    struct descriptor_data *d;
    int count = 1;
    for (d = descriptor_list; d; d = d->next) {
	if (d->connected) {
	    if (d->player == who) return count;
	    count++;
	}
    }
    return 0;
}


void 
pboot(int c)
{
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) {
	process_output(d);
	d->booted = 1;
	/* shutdownsock(d); */
    }
}


void 
pnotify(int c, char *outstr)
{
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d) {
	queue_ansi(d, outstr);
	queue_write(d, "\r\n", 2);
    }
}


int 
pdescr(int c)
{
    struct descriptor_data *d;

    d = descrdata_by_count(c);

    if (d)
	return (d->descriptor);

    return -1;
}


int 
pnextdescr(int c)
{
    struct descriptor_data *d;

    d = descrdata_by_descr(c);
    if (d) d = d->next;
    while (d && (!d->connected)) d = d->next;
    if (d) return (d->descriptor);
    return (0);
}


int 
pdescrcon(int c)
{
    struct descriptor_data *d;
    int     i = 1;

    d = descrdata_by_descr(c);
	if (d) {
		return d->con_number;
	} else {
		return 0;
	}
}


int 
pset_user(int c, dbref who)
{
    struct descriptor_data *d;

    d = descrdata_by_descr(c);
    if (d && d->connected) {
	announce_disconnect(d);
	if (who != NOTHING) {
	    d->player = who;
	    d->connected = 1;
	    update_desc_count_table();
            remember_player_descr(who, d->descriptor);
            announce_connect(d->descriptor, who);
	}
        if (d->type == CT_PUEBLO) {
           FLAG2(d->player) |= F2PUEBLO;
        } else {
           FLAG2(d->player) &= ~F2PUEBLO;
        }
        if (d->type == CT_HTML) {
           FLAG2(d->player) |= F2HTML;
        } else {
           FLAG2(d->player) &= ~F2HTML;
        }
	return 1;
    }
    return 0;
}

int
plogin_user(int c, dbref who) {
   struct descriptor_data *d;

   d = descrdata_by_descr(c);
   if(!(d && d->connected))
      return 0;

   d->connected = 1;
   d->connected_at = current_systime;
   d->player = who;
   if (d->type == CT_PUEBLO) 
   {
      FLAG2(d->player) |= F2PUEBLO;
   }
   else
   {
      FLAG2(d->player) &= ~F2PUEBLO;
   }
   if (d->type == CT_HTML)
   {
      FLAG2(d->player) |= F2HTML;
   }
   else
   {
      FLAG2(d->player) &= ~F2HTML;
   }
   spit_file(d->player, MOTD_FILE);
   if ( TArch(d->player) ) {
      char buf[ 128 ];
      int count = hop_count();
      sprintf( buf, CNOTE
         "There %s %d registration%s in the hopper.",
         (count==1)?"is":"are", count, (count==1)?"":"s" );
      anotify( d->player, buf );
    }
    interact_warn(d->player);
    if (sanity_violated && TMage(d->player)) {
       notify(d->player, MARK "WARNING!  The DB appears to be corrupt!");
    }
    announce_connect(d->descriptor, d->player);
    update_desc_count_table();
    remember_player_descr(who, d->descriptor);
    DBFETCH(d->player)->sp.player.block = 0;
    return 1;
}

int 
pset_user2(int c, dbref who)
{
   struct descriptor_data *d;
   int result;

   d = descrdata_by_descr(c);
   if(!(d && d->connected))
      return 0;

   if(d->connected)
      result = pset_user(c, who);
   else
      result = plogin_user(c, who);

   return result;
}

int
dbref_first_descr(dbref c)
{
	int dcount;
	int* darr;
	struct descriptor_data *d;

	darr = get_player_descrs(c, &dcount);
	if (dcount > 0) {
		return darr[dcount - 1];
	} else {
		return NOTHING;
	}
}

McpFrame *
descr_mcpframe(int c)
{
	struct descriptor_data *d;

	d = descriptor_list;
	while (d && d->connected) {
		if (d->descriptor == c) {
			return &d->mcpframe;
		}
		d = d->next;
	}
	return NULL;
}

int
pdescrflush(int c)
{
	struct descriptor_data *d;
	int i = 0;

	if (c != -1) {
		d = descrdata_by_descr(c);
		if (d) {
			if (!process_output(d)) {
				d->booted = 1;
			}
			i++;
		}
	} else {
		for (d = descriptor_list; d; d = d->next) {
			if (!process_output(d)) {
				d->booted = 1;
			}
			i++;
		}
	}
	return i;
}

int
pdescrtype(int c)
{
    struct descriptor_data *d, *dnext;

    d = descrdata_by_descr(c);
    if (d && d->descriptor == c)
       return d->type;
    else
       return -1;
}

int
pdescrp(int c)
{
    struct descriptor_data *d, *dnext;

    d = descrdata_by_descr(c);
    if (d && d->descriptor == c)
       return 1;
    else
       return 0;
}

void
pdescr_welcome_user(int c)
{
    struct descriptor_data *d, *dnext;

    d = descrdata_by_descr(c);
    if (d && d->descriptor == c)
       welcome_user(d);
    return;
}

void
pdescr_logout(int c)
{
    struct descriptor_data *d, *dnext;

    d = descrdata_by_descr(c);
    if (d && d->descriptor == c) {
       log_status("LOGOUT: %2d %s %s(%s) %s\n",
		d->descriptor, unparse_object(d->player, d->player),
		d->hostname, d->username,
		host_as_hex(d->hostaddr));
       announce_disconnect(d);
       d->connected = 0;
       d->player = (dbref) -1;
    }
    return;
}

dbref 
partial_pmatch(const char *name)
{
    struct descriptor_data *d;
    dbref last = NOTHING;

    d = descriptor_list;
    while (d) {
	if (d->connected && (last != d->player) &&
		string_prefix(NAME(d->player), name)) {
	    if (last != NOTHING) {
		last = AMBIGUOUS;
		break;
	    }
	    last = d->player;
	}
	d = d->next;
    }
    return (last);
}


#ifdef RWHO
void
update_rwho()
{
    struct descriptor_data *d;
    char buf[BUFFER_LEN];

    rwhocli_pingalive();
    d = descriptor_list;
    while (d) {
	if (d->connected) {
	    sprintf(buf, "%d@%s", d->player, tp_muckname);
	    rwhocli_userlogin(buf, NAME(d->player), d->connected_at);
	}
	d = d->next;
    }
}
#endif

void 
welcome_user(struct descriptor_data * d)
{
    FILE   *f;
    char   *ptr;
    char    buf[BUFFER_LEN];
    const char *fname;

    if ((d->type == CT_PUEBLO) || ((d->type == CT_HTML) && (d->http_login))) {
       if (d->type == CT_PUEBLO) {
	  queue_ansi(d, "\r\nThis world is Pueblo 1.0 Enhanced.\r\n\r\n");
	  queue_ansi(d, "</xch_mudtext><img xch_mode=html>");
       }
	strcpy(buf, WELC_HTML);

	if ((f = fopen(buf, "r")) == NULL) {
	    queue_ansi(d, DEFAULT_WELCOME_MESSAGE);
	    perror("spit_file: welcome.html");
	} else {
	  while (fgets(buf, sizeof buf, f)) {
	    ptr = index(buf, '\n');
	    if (ptr && ptr > buf && *(ptr-1) != '\r') {
		*ptr++ = '\r';
		*ptr++ = '\n';
		*ptr++ = '\0';
	    }
	    queue_ansi(d, buf);
	  }
	  fclose(f);
	}
    }
    if (wizonly_mode) {
	queue_ansi(d, MARK "<b>Due to maintenance, only wizards can connect now.</b>\r\n");
    } else if (tp_playermax && con_players_curr >= tp_playermax_limit) {
	queue_ansi(d, WARN_MESG);
	queue_ansi(d, "\r\n");
    }
    
    if (d->type == CT_MUCK)
    {
	fname = reg_site_welcome(d->hostaddr);
	if (fname && (*fname == '.')) {
	    strcpy(buf, WELC_FILE);
	} else if (fname && (*fname == '#')) {
	    if (tp_rand_screens > 0)
		sprintf(buf, "data/welcome%d.txt",(rand()%tp_rand_screens)+1);
	    else
		strcpy(buf, WELC_FILE);
	} else if (fname) {
	    strcpy(buf, "data/welcome/");
	    ptr = buf + strlen(buf);
	    while(*fname && (*fname != ' ')) (*(ptr++)) = (*(fname++));
	    *ptr = '\0';
	    strcat(buf, ".txt");
	} else if (reg_site_is_barred(d->hostaddr) == TRUE) {
	    strcpy(buf, BARD_FILE);
	} else if (tp_rand_screens > 0) {
	    sprintf(buf, "data/welcome%d.txt", (rand()%tp_rand_screens) + 1);
	} else {
	    strcpy(buf, WELC_FILE);
	}

	if ((f = fopen(buf, "r")) == NULL) {
	    queue_ansi(d, DEFAULT_WELCOME_MESSAGE);
	    perror("spit_file: welcome.txt");
	} else {
	  while (fgets(buf, sizeof buf, f)) {
	    ptr = index(buf, '\n');
	    if (ptr && ptr > buf && *(ptr-1) != '\r') {
		*ptr++ = '\r';
		*ptr++ = '\n';
		*ptr++ = '\0';
	    }
	    queue_ansi(d, buf);
	  }
	  fclose(f);
	}
    }
    if (wizonly_mode) {
	queue_ansi(d, MARK "Due to maintenance, only wizards can connect now.\r\n");
    } else if (tp_playermax && con_players_curr >= tp_playermax_limit) {
	queue_ansi(d, WARN_MESG);
	queue_ansi(d, "\r\n");
    }
}

void 
help_user(struct descriptor_data * d)
{

    FILE   *f;
    char    buf[BUFFER_LEN];

    if ((f = fopen("data/connect.txt", "r")) == NULL) {
	queue_ansi(d, "The help file is missing, the management has been notified.\r\n");
	perror("spit_file: connect.txt");
    } else {
	while (fgets(buf, sizeof buf, f)) {
	    queue_ansi(d, buf);
	}
	fclose(f);
    }
}






