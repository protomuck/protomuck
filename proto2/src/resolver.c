
#define NO_MALLOC_PROFILING
#include "copyright.h"
#include "config.h"

#ifdef SOLARIS
#  ifndef _POSIX_SOURCE
#    define _POSIX_SOURCE       /* Solaris needs this */
#  endif
#endif

#include <sys/socket.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>

/*
 * SunOS can't include signal.h and sys/signal.h, stupid broken OS.
 */
#if defined(HAVE_SYS_SIGNAL_H) && !defined(SUN_OS)
# include <sys/signal.h>
#endif



/* number of hostnames cached in an LRU queue */
#define HOST_CACHE_SIZE 8192

/* Time before retrying to resolve a previously unresolved IP address. */
/* 1800 seconds == 30 minutes */
#define EXPIRE_TIME 3200

/* Time before the resolver gives up on a identd lookup.  Prevents hangs. */
#define IDENTD_TIMEOUT 60


int
notify(int player, const char *msg)
{
    return printf("%s\n", msg);
}

/* extern int errno; */

const char *addrout(int, int, int);

#define MALLOC(result, type, number) do {   \
                                       if (!((result) = (type *) malloc ((number) * sizeof (type)))) \
                                       abort();                             \
                                     } while (0)

#define FREE(x) (free((void *) x))


struct hostcache {
    int ipnum;
    char name[128];
    time_t time;
    struct hostcache *next;
    struct hostcache **prev;
} *hostcache_list = 0;

void
hostdel(int ip)
{
    struct hostcache *ptr;

    for (ptr = hostcache_list; ptr; ptr = ptr->next) {
        if (ptr->ipnum == ip) {
            if (ptr->next) {
                ptr->next->prev = ptr->prev;
            }
            *ptr->prev = ptr->next;
            FREE(ptr);
            return;
        }
    }
}

const char *
hostfetch(int ip)
{
    struct hostcache *ptr;

    for (ptr = hostcache_list; ptr; ptr = ptr->next) {
        if (ptr->ipnum == ip) {
            if (time(NULL) - ptr->time > EXPIRE_TIME) {
                hostdel(ip);
                return NULL;
            }
            if (ptr != hostcache_list) {
                *ptr->prev = ptr->next;
                if (ptr->next) {
                    ptr->next->prev = ptr->prev;
                }
                ptr->next = hostcache_list;
                if (ptr->next) {
                    ptr->next->prev = &ptr->next;
                }
                ptr->prev = &hostcache_list;
                hostcache_list = ptr;
            }
            return (ptr->name);
        }
    }
    return NULL;
}

void
hostprune(void)
{
    struct hostcache *ptr;
    struct hostcache *tmp;
    int i = HOST_CACHE_SIZE;

    ptr = hostcache_list;
    while (i-- && ptr) {
        ptr = ptr->next;
    }
    if (i < 0 && ptr) {
        *ptr->prev = NULL;
        while (ptr) {
            tmp = ptr->next;
            FREE(ptr);
            ptr = tmp;
        }
    }
}

void
hostadd(int ip, const char *name)
{
    struct hostcache *ptr;

    MALLOC(ptr, struct hostcache, 1);

    ptr->next = hostcache_list;
    if (ptr->next) {
        ptr->next->prev = &ptr->next;
    }
    ptr->prev = &hostcache_list;
    hostcache_list = ptr;
    ptr->ipnum = ip;
    ptr->time = 0;
    strcpy(ptr->name, name);
    hostprune();
}


void
hostadd_timestamp(int ip, const char *name)
{
    hostadd(ip, name);
    hostcache_list->time = time(NULL);
}








void set_signals(void);

#ifdef _POSIX_VERSION
void our_signal(int signo, void (*sighandler) (int));
#else
# define our_signal(s,f) signal((s),(f))
#endif

/*
 * our_signal(signo, sighandler)
 *
 * signo      - Signal #, see defines in signal.h
 * sighandler - The handler function desired.
 *
 * Calls sigaction() to set a signal, if we are posix.
 */
#ifdef _POSIX_VERSION
void
our_signal(int signo, void (*sighandler) (int))
{
    struct sigaction act, oact;

    act.sa_handler = sighandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    /* Restart long system calls if a signal is caught. */
#ifdef SA_RESTART
    act.sa_flags |= SA_RESTART;
#endif

    /* Make it so */
    sigaction(signo, &act, &oact);
}
#endif /* _POSIX_VERSION */

/*
 * set_signals()
 * set_sigs_intern(bail)
 *
 * Traps a bunch of signals and reroutes them to various
 * handlers. Mostly bailout.
 *
 * If called from bailout, then reset all to default.
 *
 * Called from main() and bailout()
 */

void
set_signals(void)
{
    /* we don't care about SIGPIPE, we notice it in select() and write() */
    our_signal(SIGPIPE, SIG_IGN);

    /* didn't manage to lose that control tty, did we? Ignore it anyway. */
    our_signal(SIGHUP, SIG_IGN);
}






void
make_nonblocking(int s)
{
#if !defined(O_NONBLOCK) || defined(ULTRIX) /* POSIX ME HARDER */
# ifdef FNDELAY                 /* SUN OS */
#  define O_NONBLOCK FNDELAY
# else
#  ifdef O_NDELAY               /* SyseVil */
#   define O_NONBLOCK O_NDELAY
#  endif /* O_NDELAY */
# endif /* FNDELAY */
#endif

    if (fcntl(s, F_SETFL, O_NONBLOCK) == -1) {
        perror("make_nonblocking: fcntl");
        abort();
    }
}


const char *
get_username(int a, int prt, int myprt)
{
    int fd, len, result;
    struct sockaddr_in addr;
    char *ptr, *ptr2;
    static char buf[1024];
    int lasterr;
    int timeout = IDENTD_TIMEOUT;

    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("resolver ident socket");
        return (0);
    }

    make_nonblocking(fd);

    len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = a;
    addr.sin_port = htons((short) 113);

    do {
        result = connect(fd, (struct sockaddr *) &addr, len);
        lasterr = errno;
        if (result < 0) {
            if (!timeout--)
                break;
            sleep(1);
        }
    } while (result < 0 && lasterr == EINPROGRESS);
    if (result < 0 && lasterr != EISCONN) {
        goto bad;
    }

    sprintf(buf, "%d,%d\n", prt, myprt);
    do {
        result = write(fd, buf, strlen(buf));
        lasterr = errno;
        if (result < 0) {
            if (!timeout--)
                break;
            sleep(1);
        }
    } while (result < 0 && lasterr == EAGAIN);
    if (result < 0)
        goto bad2;

    do {
        result = read(fd, buf, sizeof(buf));
        lasterr = errno;
        if (result < 0) {
            if (!timeout--)
                break;
            sleep(1);
        }
    } while (result < 0 && lasterr == EAGAIN);
    if (result < 0)
        goto bad2;

    ptr = index(buf, ':');
    if (!ptr)
        goto bad2;
    ptr++;
    if (*ptr)
        ptr++;
    if (strncmp(ptr, "USERID", 6))
        goto bad2;

    ptr = index(ptr, ':');
    if (!ptr)
        goto bad2;
    ptr = index(ptr + 1, ':');
    if (!ptr)
        goto bad2;
    ptr++;
    shutdown(fd, 2);
    close(fd);
    if ((ptr2 = index(ptr, '\r')))
        *ptr2 = '\0';
    if (!*ptr)
        return (0);
    return ptr;

  bad2:
    shutdown(fd, 2);

  bad:
    close(fd);
    return (0);
}


/*  addrout -- Translate address 'a' from int to text.          */
const char *
addrout(int a, int prt, int myprt)
{
    static char buf[128];
    char tmpbuf[128];
    const char *ptr, *ptr2;
    struct in_addr addr;
    struct hostent *he;

    addr.s_addr = a;

    ptr = hostfetch(ntohl(a));
    if (ptr) {
        ptr2 = get_username(a, prt, myprt);
        if (ptr2) {
            sprintf(buf, "%s(%s)", ptr, ptr2);
        } else {
            sprintf(buf, "%s(%d)", ptr, prt);
        }
        return buf;
    }

    he = gethostbyaddr(((char *) &addr), sizeof(addr), AF_INET);
    if (he) {
        strcpy(tmpbuf, he->h_name);
        hostadd(ntohl(a), tmpbuf);
        ptr = get_username(a, prt, myprt);
        if (ptr) {
            sprintf(buf, "%s(%s)", tmpbuf, ptr);
        } else {
            sprintf(buf, "%s(%d)", tmpbuf, prt);
        }
        return buf;
    }

    a = ntohl(a);
    sprintf(tmpbuf, "%d.%d.%d.%d",
            (a >> 24) & 0xff, (a >> 16) & 0xff, (a >> 8) & 0xff, a & 0xff);
    hostadd_timestamp(a, tmpbuf);
    ptr = get_username(htonl(a), prt, myprt);
    if (ptr) {
        sprintf(buf, "%s(%s)", tmpbuf, ptr);
    } else {
        sprintf(buf, "%s(%d)", tmpbuf, prt);
    }
    return buf;
}


#define erreturn { \
                     return 0; \
		 }
int
do_resolve(void)
{
    int ip1, ip2, ip3, ip4;
    int prt, myprt;
    int doagain;
    char *result;
    const char *ptr;
    char buf[1024];
    int fullip;

    ip1 = ip2 = ip3 = ip4 = prt = myprt = -1;
    do {
        doagain = 0;
        *buf = '\0';
        result = fgets(buf, sizeof(buf), stdin);
        if (!result) {
            if (errno == EAGAIN) {
                doagain = 1;
                sleep(1);
            } else {
                if (feof(stdin))
                    erreturn;
                perror("fgets");
                erreturn;
            }
        }
    } while (doagain || !strcmp(buf, "\n"));

    if (!strcmp(buf, "QUIT"))
        exit(0);

    sscanf(buf, "%d.%d.%d.%d(%d)%d", &ip1, &ip2, &ip3, &ip4, &prt, &myprt);
    if (ip1 < 0 || ip2 < 0 || ip3 < 0 || ip4 < 0 || prt < 0)
        erreturn;
    if (ip1 > 255 || ip2 > 255 || ip3 > 255 || ip4 > 255 || prt > 65535)
        erreturn;
    if (myprt > 65535 || myprt < 0)
        erreturn;

    fullip = (ip1 << 24) | (ip2 << 16) | (ip3 << 8) | ip4;
    fullip = htonl(fullip);
    ptr = addrout(fullip, prt, myprt);
    printf("%d.%d.%d.%d(%d):%s\n", ip1, ip2, ip3, ip4, prt, ptr);
    fflush(stdout);
    return 1;
}




int
main(int argc, char **argv)
{
    if (argc > 1) {
        fprintf(stderr, "Usage: %s\n", *argv);
        exit(1);
    }

    /* remember to ignore certain signals */
    set_signals();

    /* go do it */
    while (do_resolve()) ;
    fprintf(stderr, "Resolver exited.\n");

    exit(0);
}
