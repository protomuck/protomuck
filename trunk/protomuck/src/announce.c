/*
 *      announce - sits listening on a port, and whenever anyone connects
 *                 announces a message and disconnects them
 *
 *      Usage:  announce [port] < message_file
 *
 *      Author: Lawrence Brown <lpb@cs.adfa.oz.au>      Aug 90
 *
 *      Bits of code are adapted from the Berkeley telnetd sources
 */

/* Note that this program has been traditionally provided as a tool for
 * MUCK admins to use as they see fit (usually when moving a MUCK to
 * a new address), but that it isn't compiled as part of the standard
 * build. -Akari
 */
#define PORT    4201

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>

extern char **environ;
#ifdef HAVE_ERRNO_H
#include <errno.h>
#else
#ifdef HAVE_SYS_ERRNO_H
#include <sys/errno.h>
#else
extern int errno;
#endif
#endif
char   *Name;			/* name of this program for error messages */
char    msg[32768];

notify(int player, const char *msg) 
{ 
        return printf("%s\n", msg); 
} 
    


int
main(int argc, char *argv[])
{
    int     s, ns, foo;
    static struct sockaddr_in sin = {AF_INET};
    char   *host, *inet_ntoa();
    char    tmp[32768];
    int     ct;

    Name = argv[0];		/* save name of program for error messages  */
    sin.sin_port = htons((u_short) PORT);	/* Assume PORT */
    argc--, argv++;
    if (argc > 0) {		/* unless specified on command-line       */
	sin.sin_port = atoi(*argv);
	sin.sin_port = htons((u_short) sin.sin_port);
    }
    strcpy(msg, "");
    strcpy(tmp, "");
    while (1) {
        if (fgets(tmp,32766,stdin) == NULL)
	//if ((gets(tmp)) == NULL)
	    break;
	strcat(tmp, "\r\n");
	strcat(msg, tmp);
    }
    msg[4095] = '\0';
    signal(SIGHUP, SIG_IGN);	/* get socket, bind port to it      */
    s = socket(AF_INET, SOCK_STREAM, 0);
    if (s < 0) {
	perror("announce: socket");
	exit(1);
    }
    if (bind(s, &sin, sizeof sin) < 0) {
	perror("bind");
	exit(1);
    }
    if ((foo = fork()) != 0) {
	fprintf(stderr, "announce: pid %d running on port %d\n", foo,
		ntohs((u_short) sin.sin_port));
	_exit(0);
    } else {
#ifdef PRIO_PROCESS
	setpriority(PRIO_PROCESS, getpid(), 10);
#endif
    }
    if (listen(s, 1) < 0) {	/* start listening on port */
	perror("announce: listen");
	_exit(1);
    }
    foo = sizeof sin;
    for (;;) {			/* loop forever, accepting requests & printing
				 * msg */
	ns = accept(s, &sin, &foo);
	if (ns < 0) {
	    perror("announce: accept");
	    _exit(1);
	}
	host = inet_ntoa(sin.sin_addr);
	ct = time(0L);
	fprintf(stderr, "CONNECTION made from %s at %s",
		host, ctime(&ct));
	write(ns, msg, strlen(msg));
	sleep(5);
	close(ns);
    }
}				/* main */


