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

#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h> 
#ifndef WIN32
# include <netdb.h>
# include <sys/socket.h>
# include <sys/errno.h>
# include <errno.h>
# include <netinet/in.h>
#else
# include <winsock.h>
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

extern struct inst *oper1, *oper2, *oper3, *oper4;
extern struct inst temp1, temp2, temp3;
extern int tmp, result;
extern dbref ref;
extern char buf[BUFFER_LEN];
/* struct tm *time_tm; */


void
prim_socksend(PRIM_PROTOTYPE)
{
   char *outputbuf;
   char bigbuffer[BUFFER_LEN];
   int myresult;

   CHECKOP(2);
   oper2 = POP();
   oper1 = POP();
   if (mlev < LARCH)
       abort_interp("Socket calls are ArchWiz-only primitives.");
   if (oper2->type != PROG_STRING) abort_interp("String argument expected!");  
   if (oper1->type != PROG_SOCKET) abort_interp("Socket argument expected!");
   if (!oper2->data.string) abort_interp("Cannot send empty strings.");
   outputbuf = bigbuffer;
   sprintf(bigbuffer, "%s\n",oper2->data.string->data);

   myresult = send(oper1->data.sock->socknum, outputbuf,
                   (int) strlen(outputbuf), (unsigned int) 0);
   if(tp_log_sockets)
      log2filetime( "logs/sockets", "#%d by %s SOCKSEND:  %d\n", program, 
                     unparse_object(player, player), oper1->data.sock->socknum);

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

   CHECKOP(1);
   oper1 = POP();

   if (mlev < LARCH)
       abort_interp("Socket calls are ArchWiz-only primitives.");
   if (oper1->type != PROG_SOCKET) abort_interp("Socket argument expected!");

   mystring = (char *) malloc(3);
   bigbuf = (char *) malloc(1024);
   bufpoint = bigbuf;

   t_val.tv_sec = 0;
   t_val.tv_usec = 0;

   loop = 0;
   sockval = oper1->data.sock->socknum;
   *mystring = '\0';
   FD_ZERO(&reads);                                                    
   FD_SET(oper1->data.sock->socknum, &reads);

   select(oper1->data.sock->socknum + 1, &reads, NULL, NULL, &t_val);

   if (FD_ISSET(oper1->data.sock->socknum, &reads))
   {
      readme = recv(oper1->data.sock->socknum,mystring,1,0);
      while (readme > 0)
      {
        if ((*mystring == '\0') || (((*mystring == '\n') ||
            (*mystring == '\r'))))
            break;
        gotmessage = 1;
        *bufpoint++=*mystring;
        readme = recv(oper1->data.sock->socknum,mystring,1,0);
      }
      oper1->data.sock->lastchar = *mystring;
   }
   CLEAR(oper1);
   if(tp_log_sockets)
     if(gotmessage)
       log2filetime( "logs/sockets", "#%d by %s SOCKRECV:  %d\n", program, 
                      unparse_object(player, player), sockval);
   *bufpoint = '\0';
   if (readme < 1)
      readme = 0;
   PushInt(readme);
   PushString(bigbuf);
   free((void *)mystring);
   free((void *)bigbuf);
}

void
prim_sockclose(PRIM_PROTOTYPE)
{
   int myresult;

   CHECKOP(1);

   oper1 = POP();
   if (mlev < LARCH)
       abort_interp("Socket calls are ArchWiz-only primitives.");
   if (oper1->type != PROG_SOCKET) abort_interp("Socket argument expected!");

   if (shutdown(oper1->data.sock->socknum,2) == -1)
   #if defined(BRAINDEAD_OS)
      myresult = -1;                                              
   #else
      myresult = errnosocket;
   #endif
      else myresult = 0;
   if(tp_log_sockets)
      log2filetime( "logs/sockets", "#%d by %s SOCKCLOSE:  %d\n", program, 
                     unparse_object(player, player), oper1->data.sock->socknum);
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
    int addr_len;

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

    name.sin_port = htons(oper2->data.number);
    name.sin_family = AF_INET;                                         
    bcopy((char *)myhost->h_addr, (char *)&name.sin_addr,
           myhost->h_length);
    mysock = socket(AF_INET, SOCK_STREAM, 6); /* Open a TCP socket */
    addr_len = sizeof(name);
    fcntl(mysock, F_SETFL, O_NONBLOCK);
    if (connect(mysock, (struct sockaddr *)&name, addr_len) == -1)
#if defined(BRAINDEAD_OS) || defined(WIN32)
                sprintf(myresult, "ERROR %d", errnosocket);
#else
        strcpy(myresult, sys_errlist[errnosocket]);
#endif
        else strcpy(myresult, "noerr");
    }
    result = (struct inst *) malloc(sizeof(struct inst));
    result->type = PROG_SOCKET;
    result->data.sock = (struct muf_socket *) malloc(sizeof(struct muf_socket));
    result->data.sock->socknum = mysock;
    result->data.sock->connected = 0;
    result->data.sock->links = 1;
    result->data.sock->lastchar = '0';


/*    mufsock = (struct muf_socket *) malloc(sizeof(struct muf_socket));

    result->type = PROG_SOCKET;
    mufsock->socknum = mysock;
    result->data.sock = mufsock;
    result->data.sock->connected = 0;
    result->data.sock->links = 0;
    result->data.sock->lastchar = '0';
*/
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

   CHECKOP(1);
   oper1 = POP();

   if (mlev < LARCH)
       abort_interp("Socket calls are ArchWiz-only primitives.");
   if (oper1->type != PROG_SOCKET) abort_interp("Socket argument expected!");

   t_val.tv_sec = 0;
   t_val.tv_usec = 0;

   sockval = oper1->data.sock->socknum;
   FD_ZERO(&writes);
   FD_SET(oper1->data.sock->socknum, &writes);

   select(oper1->data.sock->socknum + 1, NULL, &writes, NULL, &t_val);

   if (FD_ISSET(oper1->data.sock->socknum, &writes)) { 
      connected = 1;
      fcntl(oper1->data.sock->socknum, F_SETFL, 0);
   }
   else {
      connected = 0;
   }
   if (connected == 1) {
      getsockopt(oper1->data.sock->socknum, SOL_SOCKET, SO_ERROR, &optval, (socklen_t *) &len);
      if ( optval != 0 )
         connected = -1;
      else oper1->data.sock->connected = 1;
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
    if (oper1->type != PROG_SOCKET) abort_interp("Socket arguement expected.");
    
    sockdescr = oper1->data.sock->socknum;
    PushInt(sockdescr);
    CLEAR(oper1);
}
  

