#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#ifndef WIN32
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
# include <sys/time.h>
#else
# include <winsock.h>
#endif
#include <string.h>
#include <sys/types.h>

#include "maillib.h"
#include "config.h"
#include "version.h"

extern const char *tp_mailserver;
extern const char *tp_servername;

#ifndef WIN32
# define do_write(s,x,y)	\
	if (!writef(s,x,y)) return ESEND_COMM_ERROR; usleep(600)
#else
# define do_write(s,x,y)	\
	if (!writef(s,x,y)) return ESEND_COMM_ERROR; Sleep(80)
#endif

int
writef(int sock, char *format, ...)
{
   va_list vargs;
   int i;
   int result;
   char mybuf[BIG_BUF];

   va_start(vargs, format);
   i = vsprintf(mybuf, format, vargs);
   result = send(sock, &mybuf[0], strlen(mybuf), 0);
   va_end(vargs);
  
   return(result);
}

int
email_start_send(const char *to, const char *from, const char *subject, int *sockptr)
{
   struct hostent *mailhost;
   struct sockaddr_in name;
   int addr_len;
   int mysock;

   mailhost = gethostbyname(tp_mailserver);
   if (mailhost == 0)
      return ESEND_CANT_FIND_MAILER;

   name.sin_port = htons(25);
   name.sin_family = AF_INET;
   bcopy((char *)mailhost->h_addr, (char *)&name.sin_addr,
     mailhost->h_length);
   mysock = socket(AF_INET, SOCK_STREAM, 6);
   addr_len = sizeof(name);
   
   if (connect(mysock, (struct sockaddr *)&name, addr_len) == -1) {
	   close(mysock);
	   return ESEND_CANT_CONNECT;
   }

   *sockptr = mysock;

   do_write(mysock, "HELO %s\n", tp_servername);
   do_write(mysock, "MAIL FROM: %s RET=FULL\n", from);
   do_write(mysock, "RCPT TO: %s NOTIFY=FAILURE,DELAY\n", to);
   do_write(mysock, "DATA%s\n","");
   do_write(mysock, "From: %s\n", from);
   do_write(mysock, "To: %s\n", to);
   do_write(mysock, "X-Mailer: %s\n", NEONVER);
   do_write(mysock, "Subject: %s\n", subject);
   do_write(mysock, "%s\n", "");
   return 0;
}

int
email_send_line(char *line, int mysock)
{
   do_write(mysock, "%s\n", line);
   return 0;
}

int
email_end_send(int mysock)
{
   do_write(mysock, "\n%s", "\n.\nQUIT\n");
   close(mysock);
   return 0;
}
