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

struct muf_socket_queue *socket_list = NULL; /* 1 way link list */

/* add_socket_to_queue will add a new MUF socket to the queue 
 * that gets checked for sending MUF socket events.
 * Should be called whenever a new MUF socket is made
 */
void 
add_socket_to_queue(struct muf_socket *newSock, struct frame *fr)
{
    struct muf_socket_queue *curr, *nw;
    curr = socket_list;

    /* make new node */
    nw = (struct muf_socket_queue *) malloc(sizeof(struct muf_socket_queue *));
    nw->theSock = newSock;
    nw->fr = fr;
    nw->next = NULL;

    if (!curr) { /* list is empty, point list to new */
        socket_list = nw;
    } else { /* list not empty, find end */
        while(curr->next)
            curr->next;
        curr->next = nw;
    }
}

/* remove_socket_from_queue() will remove a MUF socket from
 * the queue of sockets to check. Called from RCLEAR() in interp.c
 */
void 
remove_socket_from_queue(struct muf_socket *oldSock)
{
    struct muf_socket_queue *curr, *next, *temp;
    
    curr = socket_list;
    if (!curr)
        return; /* socket list is empty */
    
    if (curr->theSock == oldSock) { /* need to remove first */
        temp = curr->next;
        curr->fr = NULL;
        curr->next = NULL;
        curr->theSock = NULL;
        free((void*) curr);
        socket_list = temp;
    } else { /* need to search through the list */
        while(curr && curr->theSock != oldSock) {
            temp = curr;
            curr = curr->next;
        } 
        if (!curr)
            return; /* nothing to free */
        temp->next = curr->next;
        curr->fr = NULL;
        curr->next = NULL;
        curr->theSock = NULL;
        free((void *) curr);
    }
}

/* checks all of the sockets in the queue to see if they
 * are in the 'readable' set. If so, passes an event
 * to that program frame.
 */
void 
muf_socket_events()
{
    fd_set reads;
    char littleBuf[50]; 
    int maxDescr = 0;
    struct timeval t_val;
    struct muf_socket_queue *curr = socket_list;
    struct inst temp1;
    int eventAdded = 0;

    if (!curr)
        return; /* no sockets to check */

    FD_ZERO(&reads);
    t_val.tv_sec  = 0;
    t_val.tv_usec = 0;

    while(curr) { /* add sockets to check to set */
        if (!curr->theSock->readWaiting) { /* add it */
            FD_SET(curr->theSock->socknum, &reads);
            if (curr->theSock->socknum >= maxDescr)
                maxDescr = curr->theSock->socknum;
            eventAdded = 1;
        }
        curr = curr->next;
    } /* while(curr) */
    if (!eventAdded)
        return; /* none need to be checked */
    
    /* now our &reads set is ready */
    select(maxDescr + 1, &reads, NULL, NULL, &t_val);
    
    /* now check through the reads set */
    curr = socket_list;
    while (curr) {
        if (!(curr->theSock->readWaiting) && 
            FD_ISSET(curr->theSock->socknum, &reads)) { /* event time */
            curr->theSock->readWaiting = 1;
            temp1.type = PROG_SOCKET;
            temp1.data.sock = curr->theSock;
            curr->theSock->links += 1; /* manual pointer copy */
            if (curr->theSock->listening)
                strcpy(littleBuf, "SOCKET.LISTEN");
            else
                strcpy(littleBuf, "SOCKET.READ");
            muf_event_add(curr->fr, littleBuf, &temp1, 1); /* 1 = exclusive */ 
            CLEAR(&temp1);
        } /* if */
        curr = curr->next;
    } /* while(curr) */ 
}

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
    int loop, gotmessage = 0;
    int readme = 0;
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
    memset(bigbuf, '\0', BUFFER_LEN); /* clear buffer out */
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
           gotmessage = 1; /* indicates the socket sent -something- */
	   ++charCount;
	   if (isascii(*mystring))
               *bufpoint++=*mystring;
           readme = recv(oper1->data.sock->socknum,mystring,1,0);
       }
       if (*mystring == '\n' && oper1->data.sock->usequeue) {
           gotmessage = 1; /* needed to catch enter presses for sockqueue */
           if (charCount == 0)
               charCount = 1;
           bigbuf[0] = '\n';
       }
       if (gotmessage && oper1->data.sock->usequeue) { /* special handling */
           char *p, *pend, *q, *qend;
           struct muf_socket *theSock = oper1->data.sock;       
           if (!theSock->raw_input) { /* No Queue. Allocate one. */
               theSock->raw_input = (char *) malloc(sizeof(char) * BUFFER_LEN);
               theSock->raw_input_at = theSock->raw_input;
           }
           p = theSock->raw_input_at;
           pend = theSock->raw_input + BUFFER_LEN - 1;
           gotmessage = 0; /* now it determines if we send or not. */
           for (q = bigbuf, qend = bigbuf + charCount; q < qend; ++q) {
               if (*q == '\n' || *q == '\r' || p == pend) { /* EOL, or full */ 
                   *p = '\0';
                   if (p > theSock->raw_input) /* have string */
                       strcpy(bigbuf, theSock->raw_input);
                   else /* empty string */
                       strcpy(bigbuf, "");
                   free(theSock->raw_input);
                   theSock->raw_input = NULL;
                   theSock->raw_input_at = NULL; 
                   p = NULL;
                   gotmessage = 1; /* to prevent clearing bigbuf later */
                   break;   
               } else if (p < pend && isascii(*q) && theSock->usesmartqueue) {
                   if (isprint(*q))
                       *p++ = *q;
		   else if (*q == '\t') /* tab */
                       *p++ = ' ';
                   else if (*q == 8 || *q == 127)  /* delete */
                       if (p > theSock->raw_input)
                           --p;
                       else
                           *p = '\0';
               } else if (p < pend && isascii(*q) && theSock->usequeue)
                   *p++ = *q;
           } /* for */
           if (p > theSock->raw_input)
               theSock->raw_input_at = p;
           if (!gotmessage)
               strcpy(bigbuf, "");
       } /* if usequeues */
    } /* if FD_ISSET */
    if (gotmessage) { /* update */
        oper1->data.sock->last_time = time(NULL);
        oper1->data.sock->commands += 1;
    }
    oper1->data.sock->readWaiting = 0;
    CLEAR(oper1);
    if(tp_log_sockets)
        if(gotmessage)
            log2filetime( "logs/sockets", "#%d by %s SOCKRECV:  %d\n", program, 
                           unparse_object(player, player), sockval);
    
    if (readme < 1 && readme != -1)
        readme = 0;
    CHECKOFLOW(2);
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
    }
    oper1->data.sock->last_time = time(NULL); /* time of last command */
    oper1->data.sock->readWaiting = 0;
    CLEAR(oper1);
    if(tp_log_sockets)
        if(gotmessage)
            log2filetime( "logs/sockets", "#%d by %s SOCKRECV:  %d\n", program,
                           unparse_object(player, player), sockval);
    if (readme < 1)
        readme = 0;
    CHECKOFLOW(2);
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
    oper1->data.sock->connected = 0;
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
    result->data.sock->listening = 0;
    result->data.sock->raw_input = NULL;
    result->data.sock->raw_input_at = NULL;
    result->data.sock->inIAC = 0;
    result->data.sock->commands = 0;
    result->data.sock->is_player = 0;
    result->data.sock->port = oper2->data.number; /* remote port # */
    result->data.sock->hostname = alloc_string(oper1->data.string->data);
    result->data.sock->host = ntohl(name.sin_addr.s_addr);
    result->data.sock->username = alloc_string(unparse_object(player, player));
    result->data.sock->connected_at = time(NULL);
    result->data.sock->last_time = time(NULL);
    result->data.sock->usequeue = 0; 
    result->data.sock->usesmartqueue = 0;
    result->data.sock->host = validHost; 
    result->data.sock->readWaiting = 0;
    if (tp_socket_events)
        add_socket_to_queue(result->data.sock, fr);

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
    oper1->data.sock->last_time = time(NULL);
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
    CLEAR(oper1);
    PushInt(sockdescr);
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
    result->data.sock->listening = 1;
    result->data.sock->raw_input = NULL;
    result->data.sock->raw_input_at = NULL;
    result->data.sock->inIAC = 0;
    result->data.sock->connected_at = time(NULL);
    result->data.sock->last_time = time(NULL);
    result->data.sock->commands = 0;
    result->data.sock->is_player = 0;
    result->data.sock->port = oper1->data.number; /* listening port */
    result->data.sock->hostname = alloc_string("localhost");
    result->data.sock->username = alloc_string(tp_muckname);
    result->data.sock->host = 1;
    result->data.sock->usequeue = 0;
    result->data.sock->readWaiting = 0;
    if (tp_socket_events)
        add_socket_to_queue(result->data.sock, fr);

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
    char hostname[128];
    char *hptr = hostname;
    char username[10]; 
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
    oper1->data.sock->commands += 1; 
    result = (struct inst *) malloc(sizeof(struct inst));
    result->type = PROG_SOCKET;
    result->data.sock = (struct muf_socket *) malloc(sizeof(struct muf_socket));
    result->data.sock->socknum = newsock;
    result->data.sock->connected = 1;
    result->data.sock->links = 1;
    result->data.sock->listening = 0;
    result->data.sock->raw_input = NULL;
    result->data.sock->raw_input_at = NULL;
    result->data.sock->inIAC = 0;
    result->data.sock->connected_at = time(NULL);
    result->data.sock->last_time = time(NULL);
    strcpy(hostname, addrout(oper1->data.sock->port, remoteaddr.sin_addr.s_addr,
                     remoteaddr.sin_port));
    result->data.sock->hostname = alloc_string(hostname); 
    sprintf(username, "%d", ntohs(remoteaddr.sin_port));
    result->data.sock->username = alloc_string(username); /* not done */
    result->data.sock->host = ntohl(remoteaddr.sin_addr.s_addr);
    result->data.sock->port = oper1->data.sock->port; /* port it connected to*/
    result->data.sock->usequeue = 0;
    result->data.sock->usesmartqueue = 0;
    result->data.sock->commands = 0;
    result->data.sock->is_player = 0;
    result->data.sock->readWaiting = 0;
    if (tp_socket_events)
        add_socket_to_queue(result->data.sock, fr);

    if (tp_log_sockets)
        log2filetime( "logs/sockets", "#%d by %s SOCKACCEPT: Port:%d -> %d\n",
                      program, unparse_object(player, (dbref) 1),
                      oper1->data.number, result->data.sock->socknum);

    CLEAR(oper1);

    copyinst(result, &arg[(*top)++]);
    CLEAR(result);
}    

void
prim_get_sockinfo(PRIM_PROTOTYPE)
{
    stk_array *nw;
    struct muf_socket *theSock;
    /* (SOCKET) -- dict */
    CHECKOP(1);
    oper1 = POP(); /* socket */

    if (mlev < LARCH)
        abort_interp("Permission denied.");
    if (oper1->type != PROG_SOCKET)
        abort_interp("MUF Socket expected. (1)");

    theSock = oper1->data.sock;
    nw = new_array_dictionary();
    array_set_strkey_intval(&nw, "DESCR", theSock->socknum);
    array_set_strkey_intval(&nw, "CONNECTED", theSock->connected);
    array_set_strkey_intval(&nw, "LISTENING", theSock->listening);
    array_set_strkey_strval(&nw, "HOST", host_as_hex(theSock->host));
    array_set_strkey_intval(&nw, "CONNECTED_AT", theSock->connected_at);
    array_set_strkey_intval(&nw, "LAST_TIME", theSock->last_time);
    array_set_strkey_intval(&nw, "COMMANDS", theSock->commands);
    array_set_strkey_intval(&nw, "PORT", theSock->port);
    array_set_strkey_intval(&nw, "SOCKQUEUE", theSock->usequeue);
    array_set_strkey_intval(&nw, "SMARTQUEUE", theSock->usesmartqueue);
    array_set_strkey_intval(&nw, "READWAITING", theSock->readWaiting);
    array_set_strkey_strval(&nw, "HOSTNAME", 
                                theSock->hostname ? theSock->hostname : "" ); 
    array_set_strkey_strval(&nw, "USERNAME", 
                                theSock->username ? theSock->username : "" );

    CLEAR(oper1);
    PushArrayRaw(nw);

} 

void
prim_socket_setuser(PRIM_PROTOTYPE)
{
    char *ptr;
    char pad_char[] = "";
    struct inst *oper1, *oper2, *oper3;
    struct muf_socket *theSock;
    const char *password;
    struct descriptor_data *d;
    
    /* (SOCKET) ref pass -- bool */
    CHECKOP(3);
    oper3 = POP(); /* password */
    oper2 = POP(); /* player dbref */
    oper1 = POP(); /* non-listening socket */

    if (mlev < LARCH)
        abort_interp("Socket prims are W3.");
    if (oper1->type != PROG_SOCKET)
        abort_interp("Expected MUF socket. (1)");
    if (oper1->data.sock->listening)
        abort_interp("Does not work with listening sockets. (1)");
    if (!oper1->data.sock->connected)
        abort_interp("This socket is not connected.");
    theSock = oper1->data.sock;
    if (oper2->type != PROG_OBJECT || !valid_player(oper2))
        abort_interp("Expected player dbref. (2)");
    ref = oper2->data.objref;
    if (oper3->type != PROG_STRING)
        abort_interp("Expected string for password. (3)");
    ptr = oper3->data.string ? oper3->data.string->data : pad_char; 

    password = DBFETCH(ref)->sp.player.password;
    if (password) { /* check pass since character has one */
        if (strcmp(ptr, password)) { /* incorrect password */
            CLEAR(oper1);
            CLEAR(oper2);
            CLEAR(oper3);
            result = 0;
            PushInt(result);
            return;
        }
    }
    /* passed password check. Now to connect. */
    /* first make sure that the socket is non-blocking */
#ifdef WIN_VC
    ioctl(theSock->socknum, FIONBIO, &turnon);
#else
   fcntl(theSock->socknum, F_SETFL, O_NONBLOCK);
#endif
    /* Now establish a normal telnet connection to the MUCK */
    d = initializesock(theSock->socknum, theSock->hostname, 
                       atoi(theSock->username), theSock->host, CT_MUCK, 
                       theSock->port, 1);   

    /* d is now in the descriptor list and properly initialized. 
     * Now connect it to a player. */
    result = plogin_user(d, ref);
    if (result) /* update global max descr */
        check_maxd(d);

    if( tp_log_connects && result)
        log2filetime(CONNECT_LOG, "SOCKET_SETUSER: %2d %s %s(%s) %s P#%d\n",
             theSock->socknum, unparse_object(ref, ref),
             theSock->hostname, theSock->username,
             host_as_hex(theSock->host), theSock->port);
    if (result)
        theSock->is_player = 1;
        
    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushInt(result);
}

void
prim_set_sockopt(PRIM_PROTOTYPE)
{
    int flag = 0;
    struct muf_socket *theSock;
    /* socket flag -- i */
    CHECKOP(2);
    oper2 = POP(); /* int */
    oper1 = POP(); /* socket */

    if (mlev < LARCH)
        abort_interp("Permission denied.");
    if (oper1->type != PROG_SOCKET)
        abort_interp("Expected MUF socket. (1)");
    theSock = oper1->data.sock;
    if (oper2->type != PROG_INTEGER)
        abort_interp("Expected integer. (2)");
    flag = oper2->data.number;

    if (flag == 0) { /* remove any queue options */
        theSock->usequeue = 0;
        theSock->usesmartqueue = 0;
        if (theSock->raw_input) {
            free(theSock->raw_input);
            theSock->raw_input = NULL;
            theSock->raw_input_at = NULL;
        }
        result = 1;
    } else if (flag == 1) {
        theSock->usequeue = 1;
        theSock->usesmartqueue = 0;
        result = 1;
    } else if (flag == 2) {
        theSock->usequeue = 1;
        theSock->usesmartqueue = 1;
        result = 1;
    } else
        result = 0;

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

#endif /* MUF_SOCKETS */
