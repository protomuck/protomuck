
/*
	Copyright (C) 1991, Marcus J. Ranum. All rights reserved.
*/

/*
code to interface client MUDs with rwho server

this is a standalone library.
*/

#include 	"config.h"

#ifdef RWHO

#include	<fcntl.h>
#include	<sys/file.h>
#include	<sys/time.h>
#include	<sys/types.h>
#include	<sys/socket.h>
#include	<netinet/in.h>
#include	<netdb.h>

#define	DGRAMPORT		6888

#ifndef	NO_HUGE_RESOLVER_CODE
extern	struct	hostent	*gethostbyname();
#endif

static	int	dgramfd = -1;
static	char	*password;
static	char	*localnam;
static	char	*lcomment;
static	struct	sockaddr_in	addr;
time_t	senttime;
typedef struct  sockaddr	*SOCKADDRPTR;

/* enable RWHO and send the server a "we are up" message */
int
rwhocli_setup(const char *server, const char *serverpw, const char *myname, const char *comment)
{
#ifndef	NO_HUGE_RESOLVER_CODE
	struct	hostent		*hp;
#endif
	char			pbuf[512];
	const char			*p;

	if(dgramfd != -1)
		return(1);

	password = (char *)malloc(strlen(serverpw) + 1);
	localnam = (char *)malloc(strlen(myname) + 1);
	lcomment = (char *)malloc(strlen(comment) + 1);
	if(password == (char *)0 || localnam == (char *)0 || lcomment == (char *)0)
		return(1);
	strcpy(password,serverpw);
	strcpy(localnam,myname);
	strcpy(lcomment,comment);

	p = server;
	while(*p != '\0' && (*p == '.' || isdigit(*p)))
		p++;

	if(*p != '\0') {
#ifndef	NO_HUGE_RESOLVER_CODE
		if((hp = gethostbyname((char *)server)) == (struct hostent *)0)
			return(1);
		(void)bcopy(hp->h_addr,(char *)&addr.sin_addr,hp->h_length);
#else
		return(1);
#endif
	} else {
		int	f;

		if((f = inet_addr(server)) == -1L)
			return(1);
		(void)bcopy((char *)&f,(char *)&addr.sin_addr,sizeof(f));
	}

	addr.sin_port = htons(DGRAMPORT);
	addr.sin_family = AF_INET;

	if((dgramfd = socket(AF_INET,SOCK_DGRAM,0)) < 0)
		return(1);

	time(&senttime);

	sprintf(pbuf,"U\t%.20s\t%.20s\t%.20s\t%.10d\t0\t%.25s",
		localnam,password,localnam,senttime,comment);
	sendto(dgramfd,pbuf,strlen(pbuf),0,(SOCKADDRPTR)&addr,sizeof(addr));
	return(0);
}

/* disable RWHO */
int
rwhocli_shutdown()
{
	char	pbuf[512];

	if(dgramfd != -1) {
		sprintf(pbuf,"D\t%.20s\t%.20s\t%.20s",localnam,password,localnam);
		sendto(dgramfd,pbuf,strlen(pbuf),0,(SOCKADDRPTR)&addr,sizeof(addr));
		close(dgramfd);
		dgramfd = -1;
		free(password);
		free(localnam);
	}
	return(0);
}

/* send an update ping that we're alive */
int
rwhocli_pingalive()
{
	char	pbuf[512];

	if(dgramfd != -1) {
		sprintf(pbuf,"M\t%.20s\t%.20s\t%.20s\t%.10d\t0\t%.25s",
			localnam,password,localnam,senttime,lcomment);
		sendto(dgramfd,pbuf,strlen(pbuf),0,(SOCKADDRPTR)&addr,sizeof(addr));
	}
	return(0);
}

/* send a "so-and-so-logged in" message */
int
rwhocli_userlogin(const char *uid, const char *name, time_t tim)
{
	char	pbuf[512];

	if(dgramfd != -1) {
		sprintf(pbuf,"A\t%.20s\t%.20s\t%.20s\t%.20s\t%.10d\t0\t%.20s",
			localnam,password,localnam,uid,tim,name);
		sendto(dgramfd,pbuf,strlen(pbuf),0,(SOCKADDRPTR)&addr,sizeof(addr));
	}
	return(0);
}

/* send a "so-and-so-logged out" message */
int
rwhocli_userlogout(const char *uid)
{
	char	pbuf[512];

	if(dgramfd != -1) {
		sprintf(pbuf,"Z\t%.20s\t%.20s\t%.20s\t%.20s",
			localnam,password,localnam,uid);
		sendto(dgramfd,pbuf,strlen(pbuf),0,(SOCKADDRPTR)&addr,sizeof(addr));
	}
	return(0);
}

#endif /* RWHO */
