/*
 * Revision 1.6  2000/5/26 Akari
 * Changed sockets to W3 level.
 * Added empty string type checking to prevent crashers.
 * Fixed 'missing first character' bug in sockrecv
 *
 * Revision 1.5  1996/10/02 19:30:35  loki
 * Added support for WinNT/Braindead OS's like SunOS
 *
 * Revision 1.4  1996/09/20 00:30:14  loki
 * Fixed really stupid bug in prim_socksend
 * It never actually sent newlines, which kinda killed most things.
 *
 * Revision 1.3  1996/09/19 23:34:19  loki
 * Fixed evil pointer bug.  Oops.
 *
 */

/* Primitives package */

#include "copyright.h"
#include "config.h"

#ifdef MUF_SOCKETS
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#ifndef WIN_VC
# include <sys/socket.h>
# include <sys/errno.h>
# include <sys/errno.h>
# include <netinet/in.h>
# include <netdb.h>
# include <unistd.h>
#endif

#include "db.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "strings.h"
#include "interp.h"
#include "cgi.h"

#if defined(BRAINDEAD_OS) || defined(WIN32)
 typedef int socklen_t;
#endif

extern struct inst *oper1, *oper2, *oper3, *oper4;
extern struct inst temp1, temp2, temp3;
extern int tmp, result;
extern dbref ref;
extern char buf[BUFFER_LEN];

void
prim_socksend(PRIM_PROTOTYPE)
{
    char *outputbuf;
    char bigbuffer[BUFFER_LEN];
    int myresult;

    /* socket string<message> */
    CHECKOP(2);
    oper2 = POP(); /* string */
    oper1 = POP(); /* socket */

    if (mlev < LARCH)
        abort_interp("Socket calls are ArchWiz-only primitives.");
    if (oper2->type != PROG_STRING) 
        abort_interp("String argument expected!");  
    if (oper1->type != PROG_SOCKET) 
        abort_interp("Socket argument expected!");
    if (oper1->data.sock->listening)
        abort_interp("SOCKSEND does not accept listening sockets.");
    if (!oper2->data.string) 
        abort_interp("Cannot send empty strings.");

    outputbuf = bigbuffer;
    sprintf(bigbuffer, "%s\n",oper2->data.string->data);

/* In order to keep the socket's sendqueue from overloading, we change
 * it to blocking (fixes an old old bug). In nbsockrecv, we change 
 * the socket to non-blocking to prevent hangups from certain programs
 * (MS Telnet). 
 */
#ifdef WIN_VC
    ioctl(oper1->data.sock->socknum, FIONBIO, 0);
#else
    fcntl(oper1->data.sock->socknum, F_SETFL, 0);
#endif

    myresult = send(oper1->data.sock->socknum, outputbuf,
                    (int) strlen(outputbuf), (unsigned int) 0);
    if(tp_log_sockets)
       log2filetime( "logs/sockets", "#%d by %s SOCKSEND:  %d\n", program, 
                     unparse_object(player, player), 
                     oper1->data.sock->socknum);

    CLEAR(oper1);
    CLEAR(oper2);
    if (myresult < 1 )
        myresult = 0;
    PushInt(myresult);
}

void
prim_nbsockrecv(PRIM_PROTOTYPE)
{
    char *bigbuf, *bufpoint;
    char *mystring;
    int loop, readme, gotmessage = 0;
    int sockval = 0;
    fd_set reads;
    struct timeval t_val;
    int charCount = 0;
    int turnon = 1;

    CHECKOP(1);
    /* socket -- */
    oper1 = POP(); /* socket */

    if (mlev < LARCH)
        abort_interp("Socket calls are ArchWiz-only primitives.");
    if (oper1->type != PROG_SOCKET) 
        abort_interp("Socket argument expected!");
    if (oper1->data.sock->listening)
        abort_interp("NBSOCKRECV does not work with Listening SOCKETS.");

    mystring = (char *) malloc(3);
    bigbuf = (char *) malloc(BUFFER_LEN);
    bufpoint = bigbuf;

    t_val.tv_sec = 0;
    t_val.tv_usec = 0;

    loop = 0;
    sockval = oper1->data.sock->socknum;
    *mystring = '\0';

/* In order to keep this from hanging on certain packets,
 * we need to change the socket to non-blocking
 */
#ifdef WIN_VC
    ioctl(sockval, FIONBIO, &turnon);
#else
    fcntl(sockval, F_SETFL, O_NONBLOCK);
#endif

    FD_ZERO(&reads);                                                    
    FD_SET(oper1->data.sock->socknum, &reads);

    select(oper1->data.sock->socknum + 1, &reads, NULL, NULL, &t_val);
    if (FD_ISSET(oper1->data.sock->socknum, &reads))
    {
       readme = recv(oper1->data.sock->socknum,mystring,1,0);
       while (readme > 0 && charCount < BUFFER_LEN)
       {
           if ((*mystring == '\0') || (((*mystring == '\n') ||
               (*mystring == '\r') )))
               break;
           gotmessage = 1;
	   ++charCount;
	   if (isascii(*mystring))
               *bufpoint++=*mystring;
           readme = recv(oper1->data.sock->socknum,mystring,1,0);
       }
       if(isascii(*mystring))
           oper1->data.sock->lastchar = *mystring;
    }
    CLEAR(oper1);
    if(tp_log_sockets)
        if(gotmessage)
            log2filetime( "logs/sockets", "#%d by %s SOCKRECV:  %d\n", program, 
                           unparse_object(player, player), sockval);
    *bufpoint = '\0';
/* If readme is 0, it got dropped. If it's less than -1, there was
 * an error and it should be dropped anyway. 
 * If it's -1, then it's just a empty non-blocking call.
 */
    if (readme < 1 && readme != -1)
        readme = 0;

    PushInt(readme);
    PushString(bigbuf);
    free((void *)mystring);
    free((void *)bigbuf);
}

void
prim_nbsockrecv_char(PRIM_PROTOTYPE)
{
    char *mystring;
    int loop, readme, gotmessage = 0;
    int sockval = 0;
    fd_set reads;
    struct timeval t_val;
    unsigned int theChar = 0;
    unsigned char aChar = '\0';

    CHECKOP(1);
    /* socket -- i i */
    oper1 = POP(); /* socket */

    if (mlev < LARCH)
        abort_interp("Socket calls are ArchWiz-only primitives.");
    if (oper1->type != PROG_SOCKET)
        abort_interp("Socket argument expected!");
    if (oper1->data.sock->listening)
        abort_interp("NBSOCKRECV_CHAR does not work with Listening SOCKETS.");
 
    CHECKOFLOW(2);

    mystring = (char *) malloc(3);

    t_val.tv_sec = 0;
    t_val.tv_usec = 0;

    sockval = oper1->data.sock->socknum;
    *mystring = '\0';
    FD_ZERO(&reads);
    FD_SET(oper1->data.sock->socknum, &reads);

    select(oper1->data.sock->socknum + 1, &reads, NULL, NULL, &t_val);

    if (FD_ISSET(oper1->data.sock->socknum, &reads))
    {
       readme = recv(oper1->data.sock->socknum,mystring,1,0);
       if (readme > 0)
       {
           gotmessage = 1;
           aChar = mystring[0];
           theChar = (int) aChar;
       }
       oper1->data.sock->lastchar = *mystring;
    }
    CLEAR(oper1);
    if(tp_log_sockets)
        if(gotmessage)
            log2filetime( "logs/sockets", "#%d by %s SOCKRECV:  %d\n", program,
                           unparse_object(player, player), sockval);
    if (readme < 1)
        readme = 0;
    PushInt(readme);
    PushInt(theChar);
}

void
prim_sockclose(PRIM_PROTOTYPE)
{
    int myresult;

    CHECKOP(1);
    /* socket */
    oper1 = POP();
    if (mlev < LARCH)
        abort_interp("Socket calls are ArchWiz-only primitives.");
    if (oper1->type != PROG_SOCKET) 
        abort_interp("Socket argument expected!");

    if (shutdown(oper1->data.sock->socknum,2) == -1)
    #if defined(BRAINDEAD_OS)
       myresult = -1;                                              
    #else
       myresult = errnosocket;
    #endif
       else myresult = 0;
    if(tp_log_sockets)
        log2filetime( "logs/sockets", "#%d by %s SOCKCLOSE:  %d\n", program, 
                       unparse_object(player, player), 
                       oper1->data.sock->socknum);
    CLEAR(oper1);
    PushInt(myresult);
}   
        
void
prim_nbsockopen(PRIM_PROTOTYPE)
{
    int mysock = 0;
    struct inst *result;
    struct muf_socket *mufsock;
    struct hostent *myhost;
    char myresult[255];
    struct sockaddr_in name;
    int addr_len = 0;
    int turnon = 1;
    int validHost = 0;

    CHECKOP(2);
    oper2 = POP();
    oper1 = POP();
    if (mlev < LARCH)
       abort_interp("Socket calls are ArchWiz-only primitives.");
    if (oper2->type != PROG_INTEGER)
       abort_interp("Integer argument expected.");
    if ((oper2->data.number < 1) || (oper2->data.number > 65535))
       abort_interp("Invalid port number.");
    if (oper1->type != PROG_STRING)
       abort_interp("String argument expected.");
    if ( !oper1->data.string )
       abort_interp("Host cannot be an empty string.");

    strcpy(buf, oper1->data.string->data);
    myhost = gethostbyname(buf);
    if (myhost == 0) {
        strcpy(myresult, "Invalid host."); 
    } else {
        validHost = 1;
        name.sin_port = (int)htons(oper2->data.number);
        name.sin_family = AF_INET;                                         
        bcopy((char *)myhost->h_addr, (char *)&name.sin_addr,
               myhost->h_length);
        mysock = socket(AF_INET, SOCK_STREAM, 6); /* Open a TCP socket */
        addr_len = sizeof(name);

#if !defined(O_NONBLOCK) || defined(ULTRIX)     /* POSIX ME HARDER */
# ifdef FNDELAY         /* SUN OS */
#  define O_NONBLOCK FNDELAY
# else
#  ifdef O_NDELAY       /* SyseVil */
#   define O_NONBLOCK O_NDELAY
#  endif /* O_NDELAY */
# endif /* FNDELAY */
#endif

#ifdef WIN_VC
        ioctl(mysock, FIONBIO, &turnon);
#else
        fcntl(mysock, F_SETFL, O_NONBLOCK);
#endif
        if (connect(mysock, (struct sockaddr *)&name, addr_len) == -1)
#if defined(BRAINDEAD_OS) || defined(WIN32)
            sprintf(myresult, "ERROR: %d", errnosocket);
#else
            strcpy(myresult, sys_errlist[errnosocket]);
#endif
        else strcpy(myresult, "noerr");
    }

    /* Socket was made, now initialize the muf_socket struc */
    result = (struct inst *) malloc(sizeof(struct inst));
    result->type = PROG_SOCKET;
    result->data.sock = (struct muf_socket *) malloc(sizeof(struct muf_socket));
    result->data.sock->socknum = mysock;
    result->data.sock->connected = 0;
    result->data.sock->links = 1;
    result->data.sock->lastchar = '0';
    result->data.sock->listening = 0;
    /* Right now, this is just a temp fix to prevent a crasher with
     * invalid hosts. Eventually it will actually store the long int
     * value of the host. There will be no need to change other references
     * in the code once this change is made, since any host int will
     * be 'true', which corresponds with the '1' being set manually 
     * for right now.
     */
    result->data.sock->host = validHost; 
    if(tp_log_sockets)
      log2filetime( "logs/sockets", "#%d by %s SOCKOPEN:  %s:%d -> %d\n", 
                    program, unparse_object(player, player), 
                    oper1->data.string->data, oper2->data.number,
                    result->data.sock->socknum );
    CLEAR(oper1);
    CLEAR(oper2);
    copyinst(result, &arg[(*top)++]);
    CLEAR(result);
    PushString(myresult);
}


void
prim_sockcheck(PRIM_PROTOTYPE)
{
    int connected = 0;
    int sockval = 0;
    int len = 10;
    int optval = 0;
    fd_set writes;
    struct timeval t_val;

    /* socket -- i */
    CHECKOP(1);
    oper1 = POP(); /* socket */

    if (mlev < LARCH)
        abort_interp("Socket calls are ArchWiz-only primitives.");
    if (oper1->type != PROG_SOCKET) 
        abort_interp("Socket argument expected!");
    if (oper1->data.sock->listening)
        abort_interp("SOCKCHECK does not work with listening SOCKETS.");

    t_val.tv_sec = 0;
    t_val.tv_usec = 0;

    sockval = oper1->data.sock->socknum;
    FD_ZERO(&writes);
    FD_SET(oper1->data.sock->socknum, &writes);

    select(oper1->data.sock->socknum + 1, NULL, &writes, NULL, &t_val);

    if (FD_ISSET(oper1->data.sock->socknum, &writes)) { 
       connected = 1;
#if !defined(O_NONBLOCK) || defined(ULTRIX)     /* POSIX ME HARDER */
# ifdef FNDELAY         /* SUN OS */
#  define O_NONBLOCK FNDELAY
# else
#  ifdef O_NDELAY       /* SyseVil */
#   define O_NONBLOCK O_NDELAY
#  endif /* O_NDELAY */
# endif /* FNDELAY */
#endif
#ifdef WIN_VC
       ioctl(oper1->data.sock->socknum, FIONBIO, 0);
#else
       fcntl(oper1->data.sock->socknum, F_SETFL, 0);
#endif
    }
    else {
       connected = 0;
    }
    if (connected == 1) {
       getsockopt(oper1->data.sock->socknum, SOL_SOCKET, SO_ERROR,
                  (char*) &optval, (socklen_t *) &len);
       if ( optval != 0 )
           connected = -1;
       else 
           oper1->data.sock->connected = 1;
    }
    CLEAR(oper1);
    PushInt(connected);
}

void
prim_sockdescr(PRIM_PROTOTYPE)
{
    int sockdescr = 0;
    CHECKOP(1); 
    oper1 = POP(); /* SOCKET */

    if (mlev < LARCH)
        abort_interp("Socket prims are ArchWiz-only.");
    if (oper1->type != PROG_SOCKET) 
        abort_interp("Socket arguement expected.");
    
    sockdescr = oper1->data.sock->socknum;
    PushInt(sockdescr);
    CLEAR(oper1);
}
  
void 
prim_lsockopen(PRIM_PROTOTYPE)
{
    int sockdescr = 0;
    struct inst *result;
    struct muf_socket *mufsock;
    struct sockaddr_in my_addr;
    char myresult[255];
    int addr_len;
    int errors = 0;
    int yes = 1;

    CHECKOP(2);
    /* int<queue size> int<port#> */
    oper1 = POP(); /* port */
    oper2 = POP(); /* queue size */

    if (mlev < LBOY)
        abort_interp("lsockopen is W4 or above.");
    if (oper1->type != PROG_INTEGER || oper2->type != PROG_INTEGER)
        abort_interp("LSOCKOPEN requires two integers.");
    if ((oper1->data.number < 1) || (oper1->data.number > 65535))
        abort_interp("Invalid port number for LSOCKOPEN.");
    if (oper2->data.number < 1 || oper2->data.number > 20)
        abort_interp("Invalid queue size (between 5 and 20).");

    sockdescr = socket(AF_INET, SOCK_STREAM, 0); /* get the socket descr */
    
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = (int) htons(oper1->data.number); /* set bind port # */
    my_addr.sin_addr.s_addr = INADDR_ANY; /* get my own IP address */
    memset(&(my_addr.sin_zero), '\0', 8); /* zero rest of struct */
 
    /* Make sure is able to reuse the port */
    setsockopt(sockdescr, SOL_SOCKET, SO_REUSEADDR, (char *)&yes, sizeof(int));
    
    /* Bind to the port */
    errors = bind(sockdescr, (struct sockaddr *)&my_addr, 
                              sizeof(struct sockaddr));
    
    if (errors == -1 ) {
        /* Error binding to port. */
#if defined(BRAINDEAD_OS) || defined(WIN32)
        sprintf(myresult, "ERROR %d", errnosocket);
#else
        strcpy(myresult, sys_errlist[errnosocket]);
#endif
        errors = 0;
        PushInt(errors);
        PushString(myresult);
        return;
    }

    /* activate listen port */
    errors = listen(sockdescr, oper2->data.number);

    if (errors == -1 ) {
       /* Error setting listen mode. */
#if defined(BRAINDEAD_OS) || defined(WIN32)
        sprintf(myresult, "ERROR: %d", errnosocket);
#else
        strcpy(myresult, sys_errlist[errnosocket]);
#endif
        errors = 0;
        PushInt(errors);
        PushString(myresult);
        return;
    }   
    /* No errors, make our listening socket */
    strcpy(myresult, "noerr"); 
    result = (struct inst *) malloc(sizeof(struct inst));
    result->type = PROG_SOCKET;
    result->data.sock = (struct muf_socket *) malloc(sizeof(struct muf_socket));
    result->data.sock->socknum = sockdescr;
    result->data.sock->connected = 1;
    result->data.sock->links = 1;
    result->data.sock->lastchar = '0';
    result->data.sock->listening = 1;

    if (tp_log_sockets)
        log2filetime( "logs/sockets", "#%d by %s LSOCKOPEN: Port:%d -> %d\n",
                      program, unparse_object(player, (dbref) 1),
                      oper1->data.number, result->data.sock->socknum);
    CLEAR(oper1);
    CLEAR(oper2);
    copyinst(result, &arg[(*top)++]);
    PushString(myresult);
    CLEAR(result);
}

void 
prim_sockaccept(PRIM_PROTOTYPE)
{
    int newsock = 0;
    int sockdescr = 0;
    struct inst *result;
    struct muf_socket *mufsock;
    struct hostent *myhost;
    char myresult[255];
    struct sockaddr_in myaddr;
    struct sockaddr_in remoteaddr; // client's address
    int addr_len;
    fd_set reads;
    struct timeval t_val; 
  
    CHECKOP(1);
    /* LSOCKET */
    oper1 = POP(); /* LSOCKET */
    
    if (mlev < LBOY)
        abort_interp("Listening sockets are for W4 or above.");
    if (oper1->type != PROG_SOCKET)
        abort_interp("Listening SOCKET arguement expected.");
    if (!(oper1->data.sock->listening))
        abort_interp("Must pass listening SOCKET to SOCKACCEPT.");

    t_val.tv_sec = 0;
    t_val.tv_usec = 0; 

    sockdescr = oper1->data.sock->socknum;
    FD_ZERO(&reads);
    FD_SET(sockdescr, &reads);
    
    select(sockdescr + 1, &reads, NULL, NULL, &t_val);

    if (!(FD_ISSET(sockdescr, &reads))) { //No connection waiting
        CLEAR(oper1);
        PushInt(newsock);
        return;
    }
    /* connection is waiting */
    addr_len = sizeof(remoteaddr);
    sockdescr = oper1->data.sock->socknum;
    newsock = accept(sockdescr, (struct sockaddr *) &remoteaddr, 
		     (socklen_t *) &addr_len);
    if (newsock == -1) {//some kind of error
#if defined(BRAINDEAD_OS) || defined(WIN32) || defined(CYGWIN)
        sprintf(myresult, "ERROR: ERRORNOSOCKET");
#else
        strcpy(myresult, sys_errlist[errnosocket]);
#endif
        newsock = 0;
        PushString(myresult);
        return;
    } 

    /* We have the new socket, now initialize muf_socket struct */
    CLEAR(oper1);
    result = (struct inst *) malloc(sizeof(struct inst));
    result->type = PROG_SOCKET;
    result->data.sock = (struct muf_socket *) malloc(sizeof(struct muf_socket));
    result->data.sock->socknum = newsock;
    result->data.sock->connected = 1;
    result->data.sock->links = 1;
    result->data.sock->lastchar = '0';
    result->data.sock->listening = 0;

    //add in socket logging here
    copyinst(result, &arg[(*top)++]);
    CLEAR(result);
}    

#endif /* MUF_SOCKETS */
