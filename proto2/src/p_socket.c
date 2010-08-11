/*
 * Revision 1.7  2003/11/5 Alynna
 * Added MUF SSL sockets.
 *
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
#include "mufevent.h"
#include "netresolve.h"

extern struct inst *oper1, *oper2, *oper3, *oper4, *oper5, *oper6;
extern struct inst temp1, temp2, temp3;
extern int tmp, result;
extern dbref ref;
extern char buf[BUFFER_LEN];

struct muf_socket_queue *socket_list = NULL; /* 1 way linked-list */

/* add_socket_to_queue will add a new MUF socket to the queue 
 * that gets checked for sending MUF socket events.
 * Should be called whenever a new MUF socket is made
 */
void
add_socket_to_queue(struct muf_socket *newSock, struct frame *fr)
{
    struct muf_socket_queue *nw;

    /* make new node */
    nw = (struct muf_socket_queue *) malloc(sizeof(struct muf_socket_queue));
    nw->theSock = newSock;
    nw->fr = fr;
    nw->pid = fr->pid;
    nw->next = socket_list;
    socket_list = nw;
}

/* remove_socket_from_queue() will remove a MUF socket from
 * the queue of sockets to check. Called from RCLEAR() in interp.c
 * and SOCKCLOSE/SOCKSHUTDOWN.
 */
void
remove_socket_from_queue(struct muf_socket *oldSock)
{
    struct muf_socket_queue *curr, *temp = NULL;

    curr = socket_list;
    if (!curr)
        return;                 /* socket list is empty */

    if (curr->theSock == oldSock) { /* need to remove first */
        temp = curr->next;
        curr->fr = NULL;
        curr->next = NULL;
        curr->theSock = NULL;
        free((void *) curr);
        socket_list = temp;
    } else {                    /* need to search through the list */
        while (curr && curr->theSock != oldSock) {
            temp = curr;
            curr = curr->next;
        }
        if (!curr)
            return;             /* nothing to free */
        temp->next = curr->next;
        curr->fr = NULL;
        curr->next = NULL;
        curr->theSock = NULL;
        free((void *) curr);
    }
}

/* muf_socket_clean():                                              */
/*  Used in prog_clean() to remove all sockets pointing to that     */
/*  frame.                                                          */
void
muf_socket_clean(struct frame *fr)
{
    struct muf_socket_queue *curr = socket_list, *tmp;

    while (curr) {
        tmp = curr->next;
        if (curr->fr == fr)
            remove_socket_from_queue(curr->theSock);
        curr = tmp;
    }
}

/* muf_socket_sendevent():                                          */
/*  Used in shovechars() by the socket event stuff. -Hinoserm       */
void
muf_socket_sendevent(struct muf_socket_queue *curr)
{
    char littleBuf[50];
    struct inst temp1;

    if (!curr->theSock->readWaiting && curr->pid == curr->fr->pid) {
        curr->theSock->readWaiting = 1;
        temp1.type = PROG_SOCKET;
        temp1.data.sock = curr->theSock;
        if (curr->theSock->listening) {
            sprintf(littleBuf, "SOCKET.LISTEN.%d", curr->theSock->socknum);
        } else if (!curr->theSock->connected) {
#ifdef WIN_VC
            char errval = 0;
#else
			int errval = 0;
#endif
            socklen_t slen = sizeof(errval);

            sprintf(littleBuf, "SOCKET.CONNECT.%d", curr->theSock->socknum);
            curr->theSock->readWaiting = 0;
            curr->theSock->last_time = time(NULL);

            getsockopt(curr->theSock->socknum, SOL_SOCKET, SO_ERROR, &errval,
                       &slen);
            curr->theSock->connected = (!errval ? 1 : -1);
        } else {
            sprintf(littleBuf, "SOCKET.READ.%d", curr->theSock->socknum);
        }

        muf_event_add(curr->fr, littleBuf, &temp1, 1);
    }
}

/* Reassigns the frame that events on that socket are to go to. */
void
update_socket_frame(struct muf_socket *mufSock, struct frame *newfr)
{
    struct muf_socket_queue *curr = socket_list;

    while (curr && curr->theSock != mufSock)
        curr = curr->next;

    if (curr) {                 /* socket found in queue, so update frame */
        curr->fr = newfr;
        curr->pid = newfr->pid;
        if (curr->theSock->readWaiting) { /* need to send an event to new fr */
            curr->theSock->readWaiting = 0; /* HACK! HACK! -Hinoserm */
            muf_socket_sendevent(curr);
        }
    }
}

void
prim_socksend(PRIM_PROTOTYPE)
{
    /* socket string<message> */
    CHECKOP(2);
    oper2 = POP();              /* string */
    oper1 = POP();              /* socket */

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

    if (oper1->data.sock->connected == 1) { /* Only send if the socket is connected. */
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

        sprintf(buf, "%s\n", oper2->data.string->data);

#if defined(SSL_SOCKETS) && defined(USE_SSL)
        if (oper1->data.sock->ssl_session)
            result = SSL_write(oper1->data.sock->ssl_session, buf, strlen(buf));
        else
#endif
            result = writesocket(oper1->data.sock->socknum, buf, strlen(buf));

        if (tp_log_sockets)
            log2filetime("logs/sockets", "#%d by %s SOCKSEND:  %d\n", program,
                         unparse_object(PSafe, PSafe),
                         oper1->data.sock->socknum);

        if (result < 1)
            result = 0;
    } else
        result = -1;

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void
prim_nbsockrecv(PRIM_PROTOTYPE)
{
    char *bigbuf, *bufpoint;
    char *mystring;
    int loop, gotmessage = 0;
    int readme = 0;
    int conRead = 1;            /* bool to detect dropped connections */
    int sockval = 0;
    fd_set reads;
    struct timeval t_val;
    int charCount = 0;

    CHECKOP(1);
    /* socket -- */
    oper1 = POP();              /* socket */

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
    make_nonblocking(sockval);

    FD_ZERO(&reads);
    FD_SET(oper1->data.sock->socknum, &reads);

    select(oper1->data.sock->socknum + 1, &reads, NULL, NULL, &t_val);
    if (FD_ISSET(oper1->data.sock->socknum, &reads)) {
#if defined(SSL_SOCKETS) && defined(USE_SSL)
        if (oper1->data.sock->ssl_session)
            while (1) {
                readme = SSL_read(oper1->data.sock->ssl_session, mystring, 1);
                if (SSL_get_error(oper1->data.sock->ssl_session, readme) !=
                    SSL_ERROR_WANT_READ)
                    break;
        } else
#endif
            readme = readsocket(oper1->data.sock->socknum, mystring, 1);

        conRead = readme;
        while (readme > 0 && charCount < BUFFER_LEN) {
            if ((*mystring == '\0') || (((*mystring == '\n') ||
                                         (*mystring == '\r'))))
                break;
            gotmessage = 1;     /* indicates the socket sent -something- */
            ++charCount;
            /* if (isascii(*mystring)) -- Commented this out to test 8-bit support on the stack -Hinoserm */
            *bufpoint++ = *mystring;
#if defined(SSL_SOCKETS) && defined(USE_SSL)
            if (oper1->data.sock->ssl_session)
                while (1) {
                    readme =
                        SSL_read(oper1->data.sock->ssl_session, mystring, 1);
                    if (SSL_get_error(oper1->data.sock->ssl_session, readme) !=
                        SSL_ERROR_WANT_READ)
                        break;
            } else
#endif
                readme = readsocket(oper1->data.sock->socknum, mystring, 1);
        }
        if (*mystring == '\n' && oper1->data.sock->usequeue) {
            gotmessage = 1;     /* needed to catch enter presses for sockqueue */
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
            gotmessage = 0;     /* now it determines if we send or not. */
            for (q = bigbuf, qend = bigbuf + charCount; q < qend; ++q) {
                if (*q == '\n' || *q == '\r' || p == pend) { /* EOL, or full */
                    *p = '\0';
                    if (p > theSock->raw_input) /* have string */
                        strcpy(bigbuf, theSock->raw_input);
                    else        /* empty string */
                        strcpy(bigbuf, "");
                    free(theSock->raw_input);
                    theSock->raw_input = NULL;
                    theSock->raw_input_at = NULL;
                    p = NULL;
                    gotmessage = 1; /* to prevent clearing bigbuf later */
                    break;
                } else if (p < pend && theSock->usesmartqueue) { /* && isascii(*q) used to be in here -Hinoserm */
                    if (*q == '\t') /* tab */
                        *p++ = ' ';
                    else if (*q == 8 || *q == 127) /* delete */
                        if (p > theSock->raw_input)
                            --p;
                        else
                            *p = '\0';
                    else if (*q != 13)
                        *p++ = *q; /* Changed this a bit to test -Hinoserm */
                } else if (p < pend && theSock->usequeue) /* && isascii(*q) used to be in here -Hinoserm */
                    *p++ = *q;
            }                   /* for */
            if (p > theSock->raw_input)
                theSock->raw_input_at = p;
            if (!gotmessage)
                strcpy(bigbuf, "");
        }                       /* if usequeues */
    }                           /* if FD_ISSET */
    if (gotmessage) {           /* update */
        oper1->data.sock->last_time = time(NULL);
        oper1->data.sock->commands += 1;
    }
    oper1->data.sock->readWaiting = 0;
    CLEAR(oper1);
    if (tp_log_sockets)
        if (gotmessage)
            log2filetime("logs/sockets", "#%d by %s SOCKRECV:  %d\n", program,
                         unparse_object(PSafe, PSafe), sockval);

    CHECKOFLOW(2);
    PushInt(conRead);
    PushString(bigbuf);
    free((void *) mystring);
    free((void *) bigbuf);
}

void
prim_nbsockrecv_char(PRIM_PROTOTYPE)
{
    char *mystring;
    int readme, gotmessage = 0;
    int sockval = 0;
    fd_set reads;
    struct timeval t_val;
    unsigned int theChar = 0;
    unsigned char aChar = '\0';

    CHECKOP(1);
    /* socket -- i i */
    oper1 = POP();              /* socket */

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

    if (FD_ISSET(oper1->data.sock->socknum, &reads)) {
#if defined(SSL_SOCKETS) && defined(USE_SSL)
        if (oper1->data.sock->ssl_session)
            while (1) {
                readme = SSL_read(oper1->data.sock->ssl_session, mystring, 1);
                if (SSL_get_error(oper1->data.sock->ssl_session, readme) !=
                    SSL_ERROR_WANT_READ)
                    break;
        } else
#endif
            readme = readsocket(oper1->data.sock->socknum, mystring, 1);

        if (readme > 0) {
            gotmessage = 1;
            aChar = mystring[0];
            theChar = (int) aChar;
        }
    }
    oper1->data.sock->last_time = time(NULL); /* time of last command */
    oper1->data.sock->readWaiting = 0;
    CLEAR(oper1);
    if (tp_log_sockets)
        if (gotmessage)
            log2filetime("logs/sockets", "#%d by %s SOCKRECV:  %d\n", program,
                         unparse_object(PSafe, PSafe), sockval);
    if (readme < 1)
        readme = 0;
    CHECKOFLOW(2);
    PushInt(readme);
    PushInt(theChar);
}

void
prim_sockclose(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    /* socket */
    oper1 = POP();
    if (mlev < LARCH)
        abort_interp("Socket calls are ArchWiz-only primitives.");
    if (oper1->type != PROG_SOCKET)
        abort_interp("Socket argument expected!");

    if (oper1->data.sock->is_player) { /* don't close descrs */
        result = 0;
    } else {
#if defined(SSL_SOCKETS) && defined(USE_SSL)
        /* Alynna - Must be done twice to enact a bidirectional shutdown,
           so do it again if still open, meaning remote end allows socket reuse */
        if (oper1->data.sock->ssl_session) {
            if (SSL_shutdown(oper1->data.sock->ssl_session) < 1)
                SSL_shutdown(oper1->data.sock->ssl_session);
        }
#endif
        if (shutdown(oper1->data.sock->socknum, 2) == -1)
#if defined(BRAINDEAD_OS)
            result = -1;
#else
            result = errnosocket;
#endif
        else
            result = 0;
        if (tp_log_sockets)
            log2filetime("logs/sockets", "#%d by %s SOCKCLOSE:  %d\n", program,
                         unparse_object(PSafe, PSafe),
                         oper1->data.sock->socknum);
        oper1->data.sock->connected = 0;
        remove_socket_from_queue(oper1->data.sock);
    }

    CLEAR(oper1);
    PushInt(result);
}

void
prim_sockshutdown(PRIM_PROTOTYPE)
{
    CHECKOP(2);                 /* socket int */
    oper2 = POP();
    oper1 = POP();
    if (mlev < LARCH)
        abort_interp("Socket calls are ArchWiz-only primitives.");
    if (oper1->type != PROG_SOCKET)
        abort_interp("Socket argument expected. (1)");
    if (oper2->type != PROG_INTEGER)
        abort_interp("Integer of 0 - 2 expected.");
    tmp = oper2->data.number;
    if (tmp < 0 || tmp > 2)
        abort_interp("Method can only be 0, 1, or 2");
    if (oper1->data.sock->is_player && tmp == 2) { /* don't close descrs */
        result = 0;
    } else {
        if (shutdown(oper1->data.sock->socknum, tmp) == -1)
#if defined(BRAINDEAD_OS)
            result = -1;
#else
            result = errnosocket;
#endif
        else
            result = 0;
        if (tp_log_sockets)
            log2filetime("logs/sockets", "#%d by %s SOCKSHUTDOWN:  %d\n",
                         program, unparse_object(PSafe, PSafe),
                         oper1->data.sock->socknum);
        if (tmp == 2) {
#if defined(SSL_SOCKETS) && defined(USE_SSL)
            /* Alynna - Must be done twice to enact a bidirectional shutdown,
               so do it again if still open, meaning remote end allows socket reuse */
            if (oper1->data.sock->ssl_session) {
                if (SSL_shutdown(oper1->data.sock->ssl_session) < 1)
                    SSL_shutdown(oper1->data.sock->ssl_session);
            }
#endif
            oper1->data.sock->connected = 0; /* only say not-connected if */
            remove_socket_from_queue(oper1->data.sock); /* complete shutdown.  */
        }
    }

    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void
prim_nbsockopen(PRIM_PROTOTYPE)
{
    struct inst *result = NULL;
    register int mysock = 0;
    struct sockaddr_in name;
    struct hostent *myhost;
    char myresult[255];

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
    if (!oper1->data.string)
        abort_interp("Host cannot be an empty string.");
    if (!(myhost = gethostbyname(oper1->data.string->data))) {
        strcpy(myresult, "Invalid host.");
        result = (struct inst *) malloc(sizeof(struct inst));
        result->type = PROG_INTEGER;
        result->data.number = 0;
    } else {
        name.sin_port = (int) htons(oper2->data.number);
        name.sin_family = AF_INET;
        bcopy((char *) myhost->h_addr, (char *) &name.sin_addr,
              myhost->h_length);
        mysock = socket(AF_INET, SOCK_STREAM, 6); /* Open a TCP socket */

        make_nonblocking(mysock);
        if (connect(mysock, (struct sockaddr *) &name, sizeof(name)) == -1)
#if defined(BRAINDEAD_OS) || defined(WIN32)
			if (errnosocket == 10035)
				strcpy(myresult, "Operation now in progress");
			else
				sprintf(myresult, "ERROR: %d", errnosocket);
#else
            strcpy(myresult, strerror(errnosocket));
#endif
        else
            strcpy(myresult, "noerr");

        check_maxd(mysock);
        /* Socket was made, now initialize the muf_socket struct */
        result = (struct inst *) malloc(sizeof(struct inst));
        result->type = PROG_SOCKET;
        result->data.sock =
            (struct muf_socket *) malloc(sizeof(struct muf_socket));
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
        result->data.sock->username =
            alloc_string(unparse_object(PSafe, PSafe));
        result->data.sock->connected_at = time(NULL);
        result->data.sock->last_time = time(NULL);
        result->data.sock->usequeue = 0;
        result->data.sock->usesmartqueue = 0;
        result->data.sock->readWaiting = 0;
#ifdef IPV6
	result->data.sock->ipv6 = 0;
	result->data.sock->host6 = NULL;
#endif
#if defined(SSL_SOCKETS) && defined(USE_SSL)
        result->data.sock->ssl_session = NULL;
#endif
        add_socket_to_queue(result->data.sock, fr);
        if (tp_log_sockets)
            log2filetime("logs/sockets",
                         "#%d by %s SOCKOPEN:  %s:%d -> %d\n", program,
                         unparse_object(PSafe, PSafe),
                         oper1->data.string->data, oper2->data.number,
                         result->data.sock->socknum);
    }


    CLEAR(oper1);
    CLEAR(oper2);
    copyinst(result, &arg[(*top)++]);
    CLEAR(result);
    PushString(myresult);
    if (result)
        free((void *) result);
}

void
prim_nbsock6open(PRIM_PROTOTYPE)
{
#ifndef IPV6
    abort_interp("IPv6 not enabled.  Recompile, reformat, reinstall.");
#else
    struct inst *result = NULL;
    register int mysock = 0;
    struct sockaddr_in6 name;
    struct hostent *myhost;
    char myresult[255];

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
    if (!oper1->data.string)
        abort_interp("Host cannot be an empty string.");
    if (!(myhost = gethostbyname2(oper1->data.string->data, AF_INET6))) {
        strcpy(myresult, "Invalid host.");
        result = (struct inst *) malloc(sizeof(struct inst));
        result->type = PROG_INTEGER;
        result->data.number = 0;
    } else {
        name.sin6_port = (int) htons(oper2->data.number);
        name.sin6_family = AF_INET6;
        bcopy((char *) myhost->h_addr, (char *) &name.sin6_addr,
              myhost->h_length);
        mysock = socket(AF_INET6, SOCK_STREAM, 6); /* Open a TCP socket */

        make_nonblocking(mysock);
        if (connect(mysock, (void*)&name, sizeof(struct sockaddr_in6)) == -1)
#if defined(BRAINDEAD_OS) || defined(WIN32)
            sprintf(myresult, "ERROR: %d", errnosocket);
#else
            strcpy(myresult, strerror(errnosocket));
#endif
        else
            strcpy(myresult, "noerr");

        check_maxd(mysock);
        /* Socket was made, now initialize the muf_socket struct */
        result = (struct inst *) malloc(sizeof(struct inst));
        result->type = PROG_SOCKET;
        result->data.sock =
            (struct muf_socket *) malloc(sizeof(struct muf_socket));
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
        result->data.sock->host = -1;
        result->data.sock->host6 = (void*)myhost->h_addr;
	result->data.sock->ipv6 = 1;
        result->data.sock->username =
            alloc_string(unparse_object(PSafe, PSafe));
        result->data.sock->connected_at = time(NULL);
        result->data.sock->last_time = time(NULL);
        result->data.sock->usequeue = 0;
        result->data.sock->usesmartqueue = 0;
        result->data.sock->readWaiting = 0;
#if defined(SSL_SOCKETS) && defined(USE_SSL)
        result->data.sock->ssl_session = NULL;
#endif
        add_socket_to_queue(result->data.sock, fr);
        if (tp_log_sockets)
            log2filetime("logs/sockets",
                         "#%d by %s SOCKOPEN:  %s:%d -> %d\n", program,
                         unparse_object(PSafe, PSafe),
                         oper1->data.string->data, oper2->data.number,
                         result->data.sock->socknum);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    copyinst(result, &arg[(*top)++]);
    CLEAR(result);
    PushString(myresult);
    if (result)
        free((void *) result);
#endif
}


void
prim_socksecure(PRIM_PROTOTYPE)
{
#if !defined(SSL_SOCKETS) || !defined(USE_SSL)
    abort_interp("MUF SSL sockets not supported.");
#else
    int ssl_error;

    CHECKOP(1);
    oper1 = POP();

    if (mlev < LARCH)
        abort_interp("Socket calls are ArchWiz-only primitives.");
    if (oper1->type != PROG_SOCKET)
        abort_interp("SOCKET arguement expected.");
    if (oper1->data.sock->listening)
        abort_interp("SOCKET must not be a listening socket.");
    if (oper1->data.sock->connected) {
        if (oper1->data.sock->ssl_session) {
            ssl_error = SSL_renegotiate(oper1->data.sock->ssl_session);
            if (ssl_error <= 0) {
                ssl_error =
                    SSL_get_error(oper1->data.sock->ssl_session, ssl_error);
                result = (ssl_error != SSL_ERROR_WANT_READ
                          && ssl_error != SSL_ERROR_WANT_WRITE)
                    ? ssl_error : 0;
            } else
                result = 0;
        } else {
            oper1->data.sock->ssl_session = SSL_new(ssl_ctx_client);
            SSL_set_fd(oper1->data.sock->ssl_session,
                       oper1->data.sock->socknum);
            ssl_error = SSL_connect(oper1->data.sock->ssl_session);
            if (ssl_error <= 0) {
                ssl_error =
                    SSL_get_error(oper1->data.sock->ssl_session, ssl_error);
                result = (ssl_error != SSL_ERROR_WANT_READ
                          && ssl_error != SSL_ERROR_WANT_WRITE)
                    ? ssl_error : 0;
            } else
                result = 0;
        }
    } else
        result = -1;

    CLEAR(oper1);
    PushInt(result);
#endif /* SSL_SOCKETS && USE_SSL */
}

void
prim_sockunsecure(PRIM_PROTOTYPE)
{
#if !defined(SSL_SOCKETS) || !defined(USE_SSL)
    abort_interp("MUF SSL sockets not supported.");
#else

    CHECKOP(1);
    oper1 = POP();

    if (mlev < LARCH)
        abort_interp("Socket calls are ArchWiz-only primitives.");
    if (oper1->type != PROG_SOCKET)
        abort_interp("SOCKET arguement expected.");
    if (oper1->data.sock->listening)
        abort_interp("SOCKET must not be a listening socket.");

    if (oper1->data.sock->connected) {
        if (oper1->data.sock->ssl_session) {
            if (SSL_shutdown(oper1->data.sock->ssl_session) < 1)
                SSL_shutdown(oper1->data.sock->ssl_session);
            SSL_free(oper1->data.sock->ssl_session);
            oper1->data.sock->ssl_session = NULL;
        }
    }
    CLEAR(oper1);
#endif /* SSL_SOCKETS && USE_SSL */
}

void
prim_sockcheck(PRIM_PROTOTYPE)
{
    /* socket -- i */
    CHECKOP(1);
    oper1 = POP();              /* socket */
    if (mlev < LARCH)
        abort_interp("Socket calls are ArchWiz-only primitives.");
    if (oper1->type != PROG_SOCKET)
        abort_interp("Socket argument expected!");
    if (oper1->data.sock->listening)
        abort_interp("SOCKCHECK does not work with listening SOCKETS.");
    oper1->data.sock->last_time = time(NULL);
    result = oper1->data.sock->connected;
    CLEAR(oper1);
    PushInt(result);
}

void
prim_sockdescr(PRIM_PROTOTYPE)
{
    int sockdescr = 0;

    CHECKOP(1);
    oper1 = POP();              /* SOCKET */
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
    struct sockaddr_in my_addr;
    char myresult[255];
    int errors = 0;
    int yes = 1;

    CHECKOP(2);
    /* int<queue size> int<port#> */
    oper1 = POP();              /* port */
    oper2 = POP();              /* queue size */
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
    my_addr.sin_addr.s_addr = bind_to; /* what am I bound to? */
    memset(&(my_addr.sin_zero), '\0', 8); /* zero rest of struct */
    /* Make sure is able to reuse the port */
    setsockopt(sockdescr, SOL_SOCKET, SO_REUSEADDR, (char *) &yes, sizeof(int));
    /* Bind to the port */
    errors = bind(sockdescr, (struct sockaddr *) &my_addr, sizeof(struct sockaddr));
    if (errors == -1) {
        /* Error binding to port. */
#if defined(BRAINDEAD_OS) || defined(WIN32)
        sprintf(myresult, "ERROR %d", errnosocket);
#else
        strcpy(myresult, strerror(errnosocket));
#endif
        errors = 0;
        PushInt(errors);
        PushString(myresult);
        return;
    }

    /* activate listen port */
    errors = listen(sockdescr, oper2->data.number);
    if (errors == -1) {
        /* Error setting listen mode. */
#if defined(BRAINDEAD_OS) || defined(WIN32)
        sprintf(myresult, "ERROR: %d", errnosocket);
#else
        strcpy(myresult, strerror(errnosocket));
#endif
        errors = 0;
        PushInt(errors);
        PushString(myresult);
        return;
    }
    /* set non-blocking */
    make_nonblocking(sockdescr);
    /* No errors, make our listening socket */

    check_maxd(sockdescr);
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
    result->data.sock->host = bind_to;
#ifdef IPV6
    result->data.sock->host6 = NULL;
    result->data.sock->ipv6 = 0;
#endif
    result->data.sock->usequeue = 0;
    result->data.sock->readWaiting = 0;
#if defined(SSL_SOCKETS) && defined(USE_SSL)
    result->data.sock->ssl_session = NULL;
#endif
    add_socket_to_queue(result->data.sock, fr);
    if (tp_log_sockets)
        log2filetime("logs/sockets",
                     "#%d by %s LSOCKOPEN: Port:%d -> %d\n", program,
                     unparse_object(PSafe, PSafe), oper1->data.number,
                     result->data.sock->socknum);
    CLEAR(oper1);
    CLEAR(oper2);
    copyinst(result, &arg[(*top)++]);
    PushString(myresult);
    CLEAR(result);
    if (result)
        free((void *) result);
}

void
prim_lsock6open(PRIM_PROTOTYPE)
{
#ifndef IPV6
    abort_interp("IPv6 not enabled.  Recompile, reformat, reinstall.");
#else
    int sockdescr = 0;
    struct inst *result;
    struct sockaddr_in6 my_addr;
    char myresult[255];
    int errors = 0;
    int yes = 1;

    CHECKOP(2);
    /* int<queue size> int<port#> */
    oper1 = POP();              /* port */
    oper2 = POP();              /* queue size */
    if (mlev < LBOY)
        abort_interp("lsock6open is W4 or above.");
    if (oper1->type != PROG_INTEGER || oper2->type != PROG_INTEGER)
        abort_interp("LSOCK6OPEN requires two integers.");
    if ((oper1->data.number < 1) || (oper1->data.number > 65535))
        abort_interp("Invalid port number for LSOCKOPEN.");
    if (oper2->data.number < 1 || oper2->data.number > 20)
        abort_interp("Invalid queue size (between 5 and 20).");
    sockdescr = socket(AF_INET6, SOCK_STREAM, 0); /* get the socket descr */
    my_addr.sin6_family = AF_INET6;
    my_addr.sin6_port = (int)htons(oper1->data.number); /* set bind port # */
    //my_addr.sin6_port = oper1->data.number; /* set bind port # */
    my_addr.sin6_addr = bind6; /* what am I bound to? */
    // memmove(&my_addr.sin6_addr, &bind6, sizeof(struct in6_addr));
    /* Make sure is able to reuse the port */
    setsockopt(sockdescr, SOL_SOCKET, SO_REUSEADDR, (char *) &yes, sizeof(int));
    setsockopt(sockdescr, IPPROTO_IPV6, IPV6_V6ONLY, (char *) &yes, sizeof(int));
    /* Bind to the port */
    errors = bind(sockdescr, (void*)&my_addr, sizeof(struct sockaddr_in6));
    if (errors == -1) {
        /* Error binding to port. */
#if defined(BRAINDEAD_OS) || defined(WIN32)
        sprintf(myresult, "ERROR: %d", errnosocket);
#else
        strcpy(myresult, strerror(errnosocket));
#endif
        errors = 0;
        PushInt(errors);
        PushString(myresult);
        return;
    }

    /* activate listen port */
    errors = listen(sockdescr, oper2->data.number);
    if (errors == -1) {
        /* Error setting listen mode. */
#if defined(BRAINDEAD_OS) || defined(WIN32)
        sprintf(myresult, "ERROR: %d", errnosocket);
#else
        strcpy(myresult, strerror(errnosocket));
#endif
        errors = 0;
        PushInt(errors);
        PushString(myresult);
        return;
    }
    /* set non-blocking */
    make_nonblocking(sockdescr);
    /* No errors, make our listening socket */

    check_maxd(sockdescr);
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
    result->data.sock->host = -1;
    result->data.sock->host6 = &bind6;
    result->data.sock->ipv6 = 1;
    result->data.sock->usequeue = 0;
    result->data.sock->readWaiting = 0;
#if defined(SSL_SOCKETS) && defined(USE_SSL)
    result->data.sock->ssl_session = NULL;
#endif
    add_socket_to_queue(result->data.sock, fr);
    if (tp_log_sockets)
        log2filetime("logs/sockets",
                     "#%d by %s LSOCKOPEN: Port:%d -> %d\n", program,
                     unparse_object(PSafe, PSafe), oper1->data.number,
                     result->data.sock->socknum);
    CLEAR(oper1);
    CLEAR(oper2);
    copyinst(result, &arg[(*top)++]);
    PushString(myresult);
    CLEAR(result);
    if (result)
        free((void *) result);
#endif
}

void
prim_sockaccept(PRIM_PROTOTYPE)
{
    int newsock = 0;
    int sockdescr = 0;
    struct inst *result;
    char username[10];
    char myresult[255];
    struct sockaddr_in remoteaddr; /* client's address */
#ifdef IPV6
    struct sockaddr_in6 remoteaddr6; /* client's address */
#endif
    int addr_len;
    fd_set reads;
    struct timeval t_val;
    struct huinfo *hu;

    CHECKOP(1);
    /* LSOCKET */
    oper1 = POP();              /* LSOCKET */
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
    if (!(FD_ISSET(sockdescr, &reads))) { /* No connection waiting */
        CLEAR(oper1);
        PushInt(newsock);
        return;
    }

#ifdef IPV6
if (oper1->data.sock->ipv6) {
    /* connection is waiting - ipv6 */
    addr_len = sizeof(remoteaddr6);
    sockdescr = oper1->data.sock->socknum;
    newsock =
        accept(sockdescr, (struct sockaddr *) &remoteaddr6, (socklen_t *) &addr_len);
    if (newsock == -1) {        /* some kind of error */
#if defined(BRAINDEAD_OS) || defined(WIN32) || defined(__CYGWIN__)
        sprintf(myresult, "ERROR: ERRORNOSOCKET");
#else
        strcpy(myresult, strerror(errnosocket));
#endif
        CLEAR(oper1);
        PushString(myresult);
        return;
    }

    make_nonblocking(newsock);
    check_maxd(newsock);

    /* We have the new socket, now initialize muf_socket struct */
    oper1->data.sock->commands += 1;
    result = (struct inst *) malloc(sizeof(struct inst));
    result->type = PROG_SOCKET;
    result->data.sock = (struct muf_socket *) malloc(sizeof(struct muf_socket));
    hu = host_getinfo6(remoteaddr6.sin6_addr, oper1->data.sock->port,
                       remoteaddr6.sin6_port);
    result->data.sock->host = -1;
    result->data.sock->host6 = &remoteaddr6.sin6_addr;
    result->data.sock->ipv6 = 1;
    sprintf(username, "%d", ntohs(remoteaddr6.sin6_port));
} else {
#endif
    /* connection is waiting - ipv4 */
    addr_len = sizeof(remoteaddr);
    sockdescr = oper1->data.sock->socknum;
    newsock =
        accept(sockdescr, (struct sockaddr *) &remoteaddr, (socklen_t *) &addr_len);
    if (newsock == -1) {        /* some kind of error */
#if defined(BRAINDEAD_OS) || defined(WIN32) || defined(__CYGWIN__)
        sprintf(myresult, "ERROR: ERRORNOSOCKET");
#else
        strcpy(myresult, strerror(errnosocket));
#endif
        CLEAR(oper1);
        PushString(myresult);
        return;
    }

    make_nonblocking(newsock);
    check_maxd(newsock);

    /* We have the new socket, now initialize muf_socket struct */
    oper1->data.sock->commands += 1;
    result = (struct inst *) malloc(sizeof(struct inst));
    result->type = PROG_SOCKET;
    result->data.sock = (struct muf_socket *) malloc(sizeof(struct muf_socket));
    hu =  host_getinfo(remoteaddr.sin_addr.s_addr, oper1->data.sock->port,
                       remoteaddr.sin_port);
    result->data.sock->host = ntohl(remoteaddr.sin_addr.s_addr);
    sprintf(username, "%d", ntohs(remoteaddr.sin_port));
#ifdef IPV6
    result->data.sock->host6 = (void*)&in6addr_any;
    result->data.sock->ipv6 = 0;
}
#endif
    /* END ipv6/ipv4 segregated block */
    result->data.sock->hostname = alloc_string(hu->h->name);
    host_delete(hu);
    result->data.sock->socknum = newsock;
    result->data.sock->connected = 1;
    result->data.sock->links = 1;
    result->data.sock->listening = 0;
    result->data.sock->raw_input = NULL;
    result->data.sock->raw_input_at = NULL;
    result->data.sock->inIAC = 0;
    result->data.sock->connected_at = time(NULL);
    result->data.sock->last_time = time(NULL);
    result->data.sock->port = oper1->data.sock->port; /* port it connected to */
    result->data.sock->usequeue = 0;
    result->data.sock->usesmartqueue = 0;
    result->data.sock->commands = 0;
    result->data.sock->is_player = 0;
    result->data.sock->readWaiting = 0;
    result->data.sock->username = alloc_string(username); /* not done */
#if defined(SSL_SOCKETS) && defined(USE_SSL)
    result->data.sock->ssl_session = NULL;
#endif
    add_socket_to_queue(result->data.sock, fr);
    if (tp_log_sockets)
        log2filetime("logs/sockets",
                     "#%d by %s SOCKACCEPT: Port:%d -> %d\n", program,
                     unparse_object(PSafe, PSafe), oper1->data.number,
                     result->data.sock->socknum);
    oper1->data.sock->readWaiting = 0;
    CLEAR(oper1);
    copyinst(result, &arg[(*top)++]);
    CLEAR(result);
    if (result)
        free((void *) result);
}


void
prim_ssl_sockaccept(PRIM_PROTOTYPE)
{
#if !defined(SSL_SOCKETS) || !defined(USE_SSL)
    abort_interp("MUF SSL sockets not supported.");
#else
    int newsock = 0;
    int sockdescr = 0;
    struct inst *result;
    char username[10];
    char myresult[255];
    struct sockaddr_in remoteaddr; /* client's address */
#ifdef IPV6
    struct sockaddr_in6 remoteaddr6; /* client's address */
#endif
    int addr_len;
    fd_set reads;
    struct timeval t_val;
    struct huinfo *hu;
    int ssl_error;
    SSL *ssl_session;

    CHECKOP(1);
    /* LSOCKET */
    oper1 = POP();              /* LSOCKET */
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
    if (!(FD_ISSET(sockdescr, &reads))) { /* No connection waiting */
        CLEAR(oper1);
        PushInt(newsock);
        return;
    }

#ifdef IPV6
if (oper1->data.sock->ipv6) {
    /* connection is waiting - ipv6 */
    addr_len = sizeof(remoteaddr6);
    sockdescr = oper1->data.sock->socknum;
    newsock =
        accept(sockdescr, (struct sockaddr *) &remoteaddr6, (socklen_t *) &addr_len);
    if (newsock == -1) {        /* some kind of error */
#if defined(BRAINDEAD_OS) || defined(WIN32) || defined(__CYGWIN__)
        sprintf(myresult, "ERROR: ERRORNOSOCKET");
#else
        strcpy(myresult, strerror(errnosocket));
#endif
        CLEAR(oper1);
        PushString(myresult);
        return;
    }

    make_nonblocking(newsock);
    check_maxd(newsock);

    ssl_session = SSL_new(ssl_ctx);
    SSL_set_fd(ssl_session, newsock);

    if ((ssl_error = SSL_accept(ssl_session)) <= 0) {
        ssl_error = SSL_get_error(ssl_session, ssl_error);
        if (ssl_error != SSL_ERROR_WANT_READ
            && ssl_error != SSL_ERROR_WANT_WRITE) {
            sprintf(myresult, "SSLerr: %d", ssl_error);
            SSL_free(ssl_session);
            CLEAR(oper1);
            PushString(myresult);
            return;
        }
    }

    /* We have the new socket, now initialize muf_socket struct */
    oper1->data.sock->commands += 1;
    result = (struct inst *) malloc(sizeof(struct inst));
    result->type = PROG_SOCKET;
    result->data.sock = (struct muf_socket *) malloc(sizeof(struct muf_socket));
    hu = host_getinfo6(remoteaddr6.sin6_addr, oper1->data.sock->port,
                       remoteaddr6.sin6_port);
    result->data.sock->host = -1;
    result->data.sock->host6 = &remoteaddr6.sin6_addr;
    result->data.sock->ipv6 = 1;
    sprintf(username, "%d", ntohs(remoteaddr6.sin6_port));
} else {
#endif
    /* connection is waiting - ipv4 */
    addr_len = sizeof(remoteaddr);
    sockdescr = oper1->data.sock->socknum;
    newsock =
        accept(sockdescr, (struct sockaddr *) &remoteaddr, (socklen_t *) &addr_len);
    if (newsock == -1) {        /* some kind of error */
#if defined(BRAINDEAD_OS) || defined(WIN32) || defined(__CYGWIN__)
        sprintf(myresult, "ERROR: ERRORNOSOCKET");
#else
        strcpy(myresult, strerror(errnosocket));
#endif
        CLEAR(oper1);
        PushString(myresult);
        return;
    }

    make_nonblocking(newsock);
    check_maxd(newsock);
    ssl_session = SSL_new(ssl_ctx);
    SSL_set_fd(ssl_session, newsock);

    if ((ssl_error = SSL_accept(ssl_session)) <= 0) {
        ssl_error = SSL_get_error(ssl_session, ssl_error);
        if (ssl_error != SSL_ERROR_WANT_READ
            && ssl_error != SSL_ERROR_WANT_WRITE) {
            sprintf(myresult, "SSLerr: %d", ssl_error);
            SSL_free(ssl_session);
            CLEAR(oper1);
            PushString(myresult);
            return;
        }
    }

    /* We have the new socket, now initialize muf_socket struct */
    oper1->data.sock->commands += 1;
    result = (struct inst *) malloc(sizeof(struct inst));
    result->type = PROG_SOCKET;
    result->data.sock = (struct muf_socket *) malloc(sizeof(struct muf_socket));
    hu =  host_getinfo(remoteaddr.sin_addr.s_addr, oper1->data.sock->port,
                       remoteaddr.sin_port);
    result->data.sock->host = ntohl(remoteaddr.sin_addr.s_addr);
    sprintf(username, "%d", ntohs(remoteaddr.sin_port));
#ifdef IPV6
    result->data.sock->host6 = (void*)&in6addr_any;
    result->data.sock->ipv6 = 0;
}
#endif
    /* END ipv6/ipv4 segregated block */
    result->data.sock->hostname = alloc_string(hu->h->name);
    host_delete(hu);
    result->data.sock->socknum = newsock;
    result->data.sock->connected = 1;
    result->data.sock->links = 1;
    result->data.sock->listening = 0;
    result->data.sock->raw_input = NULL;
    result->data.sock->raw_input_at = NULL;
    result->data.sock->inIAC = 0;
    result->data.sock->connected_at = time(NULL);
    result->data.sock->last_time = time(NULL);
    result->data.sock->port = oper1->data.sock->port; /* port it connected to */
    result->data.sock->usequeue = 0;
    result->data.sock->usesmartqueue = 0;
    result->data.sock->commands = 0;
    result->data.sock->is_player = 0;
    result->data.sock->readWaiting = 0;
    result->data.sock->username = alloc_string(username); /* not done */
#if defined(SSL_SOCKETS) && defined(USE_SSL)
    result->data.sock->ssl_session = NULL;
#endif
    add_socket_to_queue(result->data.sock, fr);
    if (tp_log_sockets)
        log2filetime("logs/sockets",
                     "#%d by %s SOCKACCEPT: Port:%d -> %d\n", program,
                     unparse_object(PSafe, PSafe), oper1->data.number,
                     result->data.sock->socknum);
    oper1->data.sock->readWaiting = 0;
    CLEAR(oper1);
    copyinst(result, &arg[(*top)++]);
    CLEAR(result);
    if (result)
        free((void *) result);
#endif
}

void
prim_get_sockinfo(PRIM_PROTOTYPE)
{
    stk_array *nw;
    struct muf_socket *theSock;

    /* (SOCKET) -- dict */
    CHECKOP(1);
    oper1 = POP();              /* socket */
    if (mlev < LARCH)
        abort_interp("Permission denied.");
    if (oper1->type != PROG_SOCKET)
        abort_interp("MUF Socket expected. (1)");
    theSock = oper1->data.sock;
    nw = new_array_dictionary();
    array_set_strkey_intval(&nw, "DESCR", theSock->socknum);
#if defined(SSL_SOCKETS) && defined(USE_SSL)
    array_set_strkey_intval(&nw, "SSL", theSock->ssl_session ? 1 : 0);
#endif
    array_set_strkey_intval(&nw, "CONNECTED", theSock->connected);
    array_set_strkey_intval(&nw, "LISTENING", theSock->listening);
#ifdef IPV6
    array_set_strkey_intval(&nw, "IPV6", theSock->ipv6);
    if (theSock->ipv6)
        array_set_strkey_strval(&nw, "HOST", ip_address_prototype((void*)theSock->host6->s6_addr,16));
    else
		array_set_strkey_strval(&nw, "HOST", ip_address_prototype(&theSock->host,4));
#else
	array_set_strkey_strval(&nw, "HOST", host_as_hex(theSock->host));
#endif
        
    array_set_strkey_intval(&nw, "CONNECTED_AT", theSock->connected_at);
    array_set_strkey_intval(&nw, "LAST_TIME", theSock->last_time);
    array_set_strkey_intval(&nw, "COMMANDS", theSock->commands);
    array_set_strkey_intval(&nw, "PORT", theSock->port);
    array_set_strkey_intval(&nw, "SIMPLEQUEUE", theSock->usequeue);
    array_set_strkey_intval(&nw, "TELNETQUEUE", theSock->usesmartqueue);
    array_set_strkey_intval(&nw, "READWAITING", theSock->readWaiting);
    array_set_strkey_strval(&nw, "HOSTNAME",
                            theSock->hostname ? theSock->hostname : "");
    array_set_strkey_strval(&nw, "USERNAME",
                            theSock->username ? theSock->username : "");
    CLEAR(oper1);
    PushArrayRaw(nw);
}

void
prim_socket_setuser(PRIM_PROTOTYPE)
{
    char *ptr;
    char pad_char[] = "";
    struct muf_socket *theSock;
    struct descriptor_data *d;

    /* (SOCKET) ref pass -- bool */
    CHECKOP(3);
    oper3 = POP();              /* password */
    oper2 = POP();              /* player dbref */
    oper1 = POP();              /* non-listening socket */
    if (mlev < LARCH)
        abort_interp("Socket prims are W3.");
    if (oper1->type != PROG_SOCKET)
        abort_interp("Expected MUF socket. (1)");
    if (oper1->data.sock->listening)
        abort_interp("Does not work with listening sockets. (1)");
    if (!oper1->data.sock->connected)
        abort_interp("This socket is not connected.");
    if (oper1->data.sock->is_player)
        abort_interp("This socket already connected to a player or descr.");
    theSock = oper1->data.sock;
    if (oper2->type != PROG_OBJECT || !valid_player(oper2))
        abort_interp("Expected player dbref. (2)");
    ref = oper2->data.objref;
    if (oper3->type != PROG_STRING)
        abort_interp("Expected string for password. (3)");
    ptr = oper3->data.string ? oper3->data.string->data : pad_char;
    if (!check_password(ref, ptr)) { /* incorrect password */
        CLEAR(oper1);
        CLEAR(oper2);
        CLEAR(oper3);
        result = 0;
        PushInt(result);
        return;
    }
    /* passed password check. Now to connect. */
    /* first make sure that the socket is non-blocking */
    make_nonblocking(theSock->socknum);
    /* Now establish a normal telnet connection to the MUCK */
#ifdef IPV6
    if (theSock->ipv6) {
#if defined(SSL_SOCKETS) && defined (USE_SSL)
    if (theSock->ssl_session)
        d = initializesock(theSock->socknum,
                           host_getinfo6(*theSock->host6, theSock->port,
                                        htons(atoi(theSock->username))), CT_SSL,
                           theSock->port, 0);
    else
#endif
        d = initializesock(theSock->socknum,
                           host_getinfo6(*theSock->host6, theSock->port,
                                        htons(atoi(theSock->username))),
                           CT_MUCK, theSock->port, 0);
    } else {
#endif
#if defined(SSL_SOCKETS) && defined (USE_SSL)
    if (theSock->ssl_session)
        d = initializesock(theSock->socknum,
                           host_getinfo(htonl(theSock->host), theSock->port,
                                        htons(atoi(theSock->username))), CT_SSL,
                           theSock->port, 0);
    else
#endif
        d = initializesock(theSock->socknum,
                           host_getinfo(htonl(theSock->host), theSock->port,
                                        htons(atoi(theSock->username))),
                           CT_MUCK, theSock->port, 0);

#ifdef IPV6
    }
#endif
    /* d is now in the descriptor list and properly initialized. 
     * Now connect it to a player. */
    result = plogin_user(d, ref);
    if (tp_log_connects && result)
        log2filetime(CONNECT_LOG,
                     "SOCKET_SETUSER: %2d %s %s(%s) %s P#%d\n",
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
prim_socktodescr(PRIM_PROTOTYPE)
{
    struct muf_socket *theSock;
    struct descriptor_data *d;

    /* (SOCKET) -- int:descr */
    CHECKOP(1);
    oper1 = POP();              /* socket */
    if (mlev < LARCH)
        abort_interp("Socket prims are W3.");
    if (oper1->type != PROG_SOCKET)
        abort_interp("Expected MUF socket. (1)");
    theSock = oper1->data.sock;
    if (theSock->listening)
        abort_interp("Does not work with listening sockets. (1)");
    if (!theSock->connected)
        abort_interp("This socket is not connected. (1)");
    if (theSock->is_player)
        abort_interp("This socket already connected to player or descr.");
    /* make sure socket is non-blocking */
    make_nonblocking(theSock->socknum);
    /* Now add the descriptor to the MUCK's descriptor list */
#ifdef IPV6
    if (theSock->ipv6) 
    d = initializesock(theSock->socknum,
                       host_getinfo6(*theSock->host6, theSock->port,
                                     htons(atoi(theSock->username))), CT_INBOUND,
                       theSock->port, 0);
    else
#endif
    d = initializesock(theSock->socknum,
                       host_getinfo(htonl(theSock->host), theSock->port,
                                    htons(atoi(theSock->username))), CT_INBOUND,
                       theSock->port, 0);

   
    /* now the descriptor is queued with the rest of the MUCK's d's */
    if (tp_log_sockets)
        log2filetime("logs/sockets",
                     "#%d by %s SOCKTODESCR: %d", program,
                     unparse_object(PSafe, PSafe), theSock->socknum);
    theSock->is_player = -1;
    CLEAR(oper1);
    PushInt(d->descriptor);
}

void
prim_set_sockopt(PRIM_PROTOTYPE)
{
    int flag = 0;
    struct muf_socket *theSock;

    /* socket flag -- i */
    CHECKOP(2);
    oper2 = POP();              /* int */
    oper1 = POP();              /* socket */
    if (mlev < LARCH)
        abort_interp("Permission denied.");
    if (oper1->type != PROG_SOCKET)
        abort_interp("Expected MUF socket. (1)");
    theSock = oper1->data.sock;
    if (oper2->type != PROG_INTEGER)
        abort_interp("Expected integer. (2)");
    flag = oper2->data.number;
    if (flag == 0) {            /* remove any queue options */
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
    } else if (flag == 5) {
        update_socket_frame(theSock, fr);
        result = 1;
    } else
        result = 0;
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void muf_udp_clean_byport(int portnum)
{
 int i, j, x;
 i=0; x=udp_count+1;
 while (i<=x) 
  if (udp_sockets[i].portnum == portnum) {
   if (tp_log_sockets) log2filetime("logs/sockets", "UDPCLOSE: entry %d, port %d, descr %d\n", i, udp_sockets[i].portnum, udp_sockets[i].socket);
   close(udp_sockets[i].socket);
#ifdef IPV6
   close(udp_sockets[i].socket6);
#endif
   udp_count--;
   j=i; while (j<=x) {
    udp_sockets[j] = udp_sockets[j+1];
    j++;
   } 
  } 
 else i++;
}

void udp_socket_clean(struct frame *fr)
{
 int i, j, x;
 i=0; x=udp_count+1;
 while (i<=x) 
  if (udp_sockets[i].fr == fr) {
   if (tp_log_sockets) log2filetime("logs/sockets", "UDP.socket_clean: entry %d, port %d, descr %d\n", i, udp_sockets[i].portnum, udp_sockets[i].socket);
   udp_count--;
   close(udp_sockets[i].socket);
#ifdef IPV6
   close(udp_sockets[i].socket6);
#endif
   j=i; 
   while (j<=x) {
    udp_sockets[j] = udp_sockets[j+1];
    j++;
   } 
  } 
 else i++;
}


int muf_udp_portinuse(int portnum)
{
 int i;
 for(i=0; i<=udp_count+1; ++i) 
  if (udp_sockets[i].portnum == portnum) return 1;
 return 0;
}

void
prim_udpopen(PRIM_PROTOTYPE)
{
 struct sockaddr_in sa;
#ifdef IPV6
 struct sockaddr_in6 sa6;
 int yes = 1;
#endif
 unsigned int tmp; 


 CHECKOP(1);
 oper1 = POP(); /* What port to listen for UDP events on? */

 /* Some standard checks */
 if (mlev < LARCH) 
  abort_interp("Socket prims are W3.");
 if (oper1->type != PROG_INTEGER)
  abort_interp("Integer expected (1)");
 if ((oper1->data.number < 1024) || (oper1->data.number > 65535))
  abort_interp("Port number must be between 1024 and 65535.");
 if (udp_count>31)
  abort_interp("Out of UDP ports! (32)");

 /* leave with FALSE if port in use */
 if (muf_udp_portinuse(oper1->data.number)) {
  CLEAR(oper1);
  result=0; PushInt(result);
  return;
 } else {

 /* Ready to open */
 tmp = socket(AF_INET, SOCK_DGRAM, 0);
 make_nonblocking(tmp);
 sa.sin_family = AF_INET;
 sa.sin_port = htons(oper1->data.number);
 sa.sin_addr.s_addr = bind_to;
 if (bind(tmp, (struct sockaddr *)&sa, sizeof(sa)) == -1) {
  /* The only real possible error here is 'port in use', so return zero if there was an error */
  CLEAR(oper1);
  result=0; PushInt(result); 
  return;
 }
 /* add it to the recvto mainloop */
 check_maxd(tmp);
 udp_sockets[udp_count].socket = tmp;
 udp_sockets[udp_count].fr = fr;
 udp_sockets[udp_count].portnum = oper1->data.number;
 if (tp_log_sockets) log2filetime("logs/sockets", "UDP4OPEN: entry %d, port %d, descr %d\n", udp_count, udp_sockets[udp_count].portnum, udp_sockets[udp_count].socket);

#ifdef IPV6
 /* Open an ipv6 one too! */
 tmp = socket(AF_INET6, SOCK_DGRAM, 0);
 setsockopt(tmp, IPPROTO_IPV6, IPV6_V6ONLY,(char *)&yes, sizeof(yes));
 make_nonblocking(tmp);
 sa6.sin6_family = AF_INET6;
 sa6.sin6_port = htons(oper1->data.number);
 sa6.sin6_addr = bind6;
 if (bind(tmp, (struct sockaddr *)&sa6, sizeof(sa6)) == -1) {
  /* The only real possible error here is 'port in use', so return zero if there was an error */
  CLEAR(oper1);
  result=0; PushInt(result); 
  return;
 }
 /* add it to the recvto mainloop */
 check_maxd(tmp);
 udp_sockets[udp_count].socket6 = tmp;
 if (tp_log_sockets) log2filetime("logs/sockets", "UDP6OPEN: entry %d, port %d, descr %d\n", udp_count, udp_sockets[udp_count].portnum, udp_sockets[udp_count].socket6);
#endif

 udp_count++;
 CLEAR(oper1);
 result=1; PushInt(result);
 }
}

void
prim_udpclose(PRIM_PROTOTYPE)
{

 CHECKOP(1);
 oper1 = POP(); /* What port to unlisten for UDP events on? */

 /* Some standard checks */
 if (mlev < LARCH) 
  abort_interp("Socket prims are W3.");
 if (oper1->type != PROG_INTEGER)
  abort_interp("Integer expected (1)");
 if ((oper1->data.number < 1024) || (oper1->data.number > 65535))
  abort_interp("Port number must be between 1024 and 65535.");
 if (udp_count<0)
  abort_interp("Insanity!  UDP socket count is negative!");

 /* leave with FALSE if port not in use */
 if (!muf_udp_portinuse(oper1->data.number)) {
  CLEAR(oper1);
  result=0; PushInt(result);
 } else {
 /* Ready to close */
 muf_udp_clean_byport(oper1->data.number); 
 CLEAR(oper1);
 result=1; PushInt(result);
 }
}

void
prim_udpsend(PRIM_PROTOTYPE)
{

 struct hostent *myhost;
 struct sockaddr_in sa;
 unsigned int tmp; 
 
 CHECKOP(3);
 oper3 = POP(); /* port number */
 oper2 = POP(); /* IP address */
 oper1 = POP(); /* UDP data packet */
 
 /* A bunch of sanity checks */
 if (mlev < LARCH)
    abort_interp("Socket prims are W3");
 if (oper3->type != PROG_INTEGER)
    abort_interp("Integer argument expected.");
 if ((oper3->data.number < 1) || (oper3->data.number > 65535))
    abort_interp("Invalid port number.");
 if (oper2->type != PROG_STRING)
    abort_interp("String argument expected. (1)");
 if (!oper2->data.string)
    abort_interp("Host cannot be an empty string.");
 if (oper1->type != PROG_STRING)
    abort_interp("UDP data to be sent must be a string. (3)");

 /* make some sense of the hostname, or return FALSE */
 myhost = gethostbyname(oper2->data.string->data);
 if (!myhost) {
    CLEAR(oper3);
    CLEAR(oper2);
    CLEAR(oper1);
    result=0; PushInt(result);
    return;
 }
 /* Make the socket */
 tmp = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
 sa.sin_family = AF_INET;
 sa.sin_port = htons(oper3->data.number);
 sa.sin_addr.s_addr = *myhost->h_addr;
 bcopy((char *) myhost->h_addr, (char *) &sa.sin_addr, myhost->h_length);
 
 /* ship it out, shut it down. */
 sendto(tmp, oper1->data.string->data, oper1->data.string->length, 0, (struct sockaddr *)&sa, sizeof(sa)); 
 if (tp_log_sockets) log2filetime("logs/sockets", "UDPSEND: host %s, port %d\n", myhost->h_name, htons(sa.sin_port));
 close(tmp); //HinoSpoon
 
 /* Yay! Return TRUE */    
 CLEAR(oper3);
 CLEAR(oper2);
 CLEAR(oper1);
 result = 1; PushInt(result); 

}

void
prim_udp6send(PRIM_PROTOTYPE)
{
#ifndef IPV6
 abort_interp("IPv6 not supported.  Recompile, reformat, reinstall.");
#else
 struct hostent *myhost;
 struct sockaddr_in6 sa6;
 unsigned int tmp; 
 
 CHECKOP(3);
 oper3 = POP(); /* port number */
 oper2 = POP(); /* IP address */
 oper1 = POP(); /* UDP data packet */
 
 /* A bunch of sanity checks */
 if (mlev < LARCH)
    abort_interp("Socket prims are W3");
 if (oper3->type != PROG_INTEGER)
    abort_interp("Integer argument expected.");
 if ((oper3->data.number < 1) || (oper3->data.number > 65535))
    abort_interp("Invalid port number.");
 if (oper2->type != PROG_STRING)
    abort_interp("String argument expected. (1)");
 if (!oper2->data.string)
    abort_interp("Host cannot be an empty string.");
 if (oper1->type != PROG_STRING)
    abort_interp("UDP data to be sent must be a string. (3)");

 /* make some sense of the hostname, or return FALSE */
 myhost = gethostbyname2(oper2->data.string->data, AF_INET6);
 if (!myhost) {
    CLEAR(oper3);
    CLEAR(oper2);
    CLEAR(oper1);
    result=0; PushInt(result);
    return;
 }
 /* Make the socket */
 tmp = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
 sa6.sin6_family = AF_INET6;
 sa6.sin6_port = htons(oper3->data.number);
 // sa6.sin6_addr = *myhost->h_addr;
 bcopy((char *) myhost->h_addr, (char *) &sa6.sin6_addr, myhost->h_length);
 
 /* ship it out, shut it down. */
 sendto(tmp, oper1->data.string->data, oper1->data.string->length, 0, (struct sockaddr *)&sa6, sizeof(sa6)); 
 if (tp_log_sockets) log2filetime("logs/sockets", "UDPSEND: host %s, port %d\n", myhost->h_name, htons(sa6.sin6_port));
 close(tmp);
 
 /* Yay! Return TRUE */    
 CLEAR(oper3);
 CLEAR(oper2);
 CLEAR(oper1);
 result = 1; PushInt(result); 
#endif
}

void
prim_dns(PRIM_PROTOTYPE)
{

 struct hostent *myhost;
 char bufip[16], bufname[256];
 struct in_addr sa;
  
 CHECKOP(1);
 oper1 = POP(); /* hostname */
 
 /* A bunch of sanity checks */
 if (mlev < LARCH)
    abort_interp("Socket prims are W3");
 if (oper1->type != PROG_STRING)
    abort_interp("String argument expected. (1)");
 if (!oper1->data.string)
    abort_interp("Host cannot be an empty string.");
 memset(bufip,0,16);
 memset(bufname,0,256);
 memset(buf,0,16);

 /* make some sense of the hostname, or return FALSE */
 myhost = gethostbyname(oper1->data.string->data);
 if (!myhost) {
    CLEAR(oper1);
    bufip[0] = '\0';
    bufname[0] = '\0';
    PushString(bufname);
    PushString(bufip);
    return;
 } else {
    bcopy((char *)myhost->h_name,bufname,strlen(myhost->h_name));
    bcopy((char *)myhost->h_addr,(char *)&sa,myhost->h_length);
    bcopy(inet_ntoa(sa),bufip,16);
    CLEAR(oper1);
    PushString(bufname); 
    PushString(bufip); 
 }
}

#endif /* MUF_SOCKETS */
