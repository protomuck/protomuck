#include "copyright.h"
#include "config.h"
#include "version.h"
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <netdb.h>

#if defined(BRAINDEAD_OS) || defined(WIN32) || defined(__APPLE__)
typedef int socklen_t;
#endif

#define MAX_COMMAND_LEN 1024
#define IDENTD_TIMEOUT 20
#define RESOLVER_VERSION "1.0"

static short daemon_mode = 0;
static short verbose_mode = 0;
static short sockcnt = 0;
static int socks[16];
static int maxd = 0;

struct descriptor_data {
    struct {
        //char *out;
        char *in;
        char *in_at;
        //size_t out_len;
    } buffer;
    unsigned int ip;
    unsigned short port;
    int descriptor;
    int booted;
    struct descriptor_data *prev;
    struct descriptor_data *next;
};

typedef struct descriptor_data t_descr;
t_descr *descr_list = NULL;

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
#endif /* SA_RESTART */

    /* Make it so */
    sigaction(signo, &act, &oact);
}
#endif /* _POSIX_VERSION */

/*
 * set_signals()
 *
 * Traps a bunch of signals and reroutes them to various
 * handlers. Mostly bailout.
 *
 * Called from main()
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
verb(char *format, ...)
{
    va_list args;

    if (verbose_mode < 1)
        return;

    va_start(args, format);
    fprintf(stderr, "reslvd: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
}

void
verb_2(char *format, ...)
{
    va_list args;

    if (verbose_mode < 2)
        return;

    va_start(args, format);
    fprintf(stderr, "reslvd: ");
    vfprintf(stderr, format, args);
    fprintf(stderr, "\n");
    va_end(args);
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
#endif /* 
        */
    if (fcntl(s, F_SETFL, O_NONBLOCK) == -1) {
        perror("make_nonblocking: fcntl");
        abort();
    }
}

const char *
host_as_hex(unsigned int addr)
{
    static char buf[32];

    sprintf(buf,
            "%d.%d.%d.%d",
            (addr >> 24) & 0xff,
            (addr >> 16) & 0xff, (addr >> 8) & 0xff, addr & 0xff);

    return buf;
}

void
new_connection(int sock)
{
    int newsock;
    struct sockaddr_in addr;
    socklen_t addr_len;

    t_descr *d;

    addr_len = sizeof(addr);
    newsock = accept(sock, (struct sockaddr *) &addr, &addr_len);
    if (newsock >= maxd)
        maxd = newsock + 1;

    verb("ACPT: %0.2d %s(%d)", newsock,
         host_as_hex(ntohl(addr.sin_addr.s_addr)), ntohs(addr.sin_port));

    d = (t_descr *) malloc(sizeof(t_descr));
    d->descriptor = newsock;
    d->booted = 0;
    d->buffer.in = NULL;
    d->buffer.in_at = NULL;
//    d->buffer.out = NULL;
//    d->buffer.out_len = 0;
    d->ip = ntohl(addr.sin_addr.s_addr);
    d->port = ntohs(addr.sin_port);

    d->next = descr_list;
    if (descr_list)
        descr_list->prev = d;
    d->prev = NULL;
    descr_list = d;
}

void
disconnect_descr(t_descr * d)
{
    verb("DISC: %0.2d %s(%d)", d->descriptor, host_as_hex(d->ip), d->port);
    shutdown(d->descriptor, 2);
    closesocket(d->descriptor);

    if (d->next)
        d->next->prev = d->prev;
    if (d->prev)
        d->prev->next = d->next;
    if (d == descr_list)
        descr_list = d->next;

    if (d->buffer.in)
        free((void *) d->buffer.in);
    free((void *) d);
}

void
queue_write(t_descr * d, char *format, ...)
{
    char buf[MAX_COMMAND_LEN];

    va_list args;

    va_start(args, format);
    vsprintf(buf, format, args);
    va_end(args);

    if (writesocket(d->descriptor, buf, strlen(buf)) < 0) {
        if (errno != EWOULDBLOCK)
            d->booted = 1;
        return;
    }

    writesocket(d->descriptor, "\r\n", 2);
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
    addr.sin_addr.s_addr = htonl(a);
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

const char *
get_hostname(int a)
{
    static char buf[128];
    struct in_addr addr;
    struct hostent *he;

    addr.s_addr = htonl(a);

    he = gethostbyaddr(((char *) &addr), sizeof(addr), AF_INET);
    if (he)
        return strcpy(buf, he->h_name);

    else
        return NULL;
}

void
do_command(t_descr * d, const char *cmd)
{
    const char *q;
    int a;

    if (!strncmp(cmd, "USER", 4)) {
        int myprt, port;

        sscanf(cmd, "USER %x,%d,%d", &a, &myprt, &port);
        if ((q = get_username(a, port, myprt)))
            queue_write(d, "USER %X,%d,%d,%s", a, myprt, port, q);
    } else if (!strncmp(cmd, "HOST", 4)) {
        sscanf(cmd, "HOST %x", &a);
        if ((q = get_hostname(a)))
            queue_write(d, "HOST %X,%s", a, q);
    } else if (!strncmp(cmd, "QUIT", 4)) {
        d->booted = 1;
    }
}

void
process_input(t_descr * d)
{
    char buf[MAX_COMMAND_LEN];
    int got;
    char *p, *pend, *q, *qend;

    if ((got = readsocket(d->descriptor, buf, sizeof buf)) <= 0) {
        d->booted = 1;
        return;
    }

    if (!d->buffer.in) {
        d->buffer.in = (char *) malloc(sizeof(char) * MAX_COMMAND_LEN);
        d->buffer.in_at = d->buffer.in;
    }

    p = d->buffer.in_at;
    pend = d->buffer.in + MAX_COMMAND_LEN - 1;
    for (q = buf, qend = buf + got; q < qend; q++) {
        if (*q == '\n') {
            *p = '\0';
            if (p > d->buffer.in)
                do_command(d, d->buffer.in);
            verb_2("INPT: %0.2d: %s", d->descriptor, d->buffer.in);
            p = d->buffer.in;
        } else if (p < pend) {
            if ((*q == 8 || *q == 127)) {
                if (p > d->buffer.in)
                    p--;
            } else if (*q != 13) {
                *p++ = *q;
            }
        }
    }
    if (p > d->buffer.in) {
        d->buffer.in_at = p;
    } else {
        free((void *) d->buffer.in);
        d->buffer.in = 0;
        d->buffer.in_at = 0;
    }
    return;
}

//void
//process_output(t_descr * d)
//{
//    return;
//}

int
make_socket(int port, int a)
{
    int s;
    struct sockaddr_in server;
    int opt;

    s = socket(AF_INET, SOCK_STREAM, 0);
    verb("Bound socket %d to %s:%d", s, host_as_hex(ntohl(a)), port);

    if (s < 0) {
        perror("creating stream socket");
        exit(3);
    }

    opt = 1;
    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = a;
    server.sin_port = (int) htons(port);
    if (bind(s, (struct sockaddr *) &server, sizeof(server))) {
        perror("binding stream socket");
        closesocket(s);
        exit(4);
    }

    listen(s, 5);
    if (s >= maxd)
        maxd = s + 1;
    return s;
}

void
bind_to(const char *addr)
{
    char buf[MAX_COMMAND_LEN];
    struct sockaddr_in name;
    struct hostent *he;
    int port;
    char *p;

    strcpy(buf, addr);

    for (p = buf; *p && (*p != ':'); p++) ;
    if (*p)
        *p++ = '\0';

    if (*buf && (he = gethostbyname(buf)))
        bcopy((char *) he->h_addr, (char *) &name.sin_addr, he->h_length);
    else
        name.sin_addr.s_addr = INADDR_ANY;

    if (!(port = atoi(p)))
        port = RESOLVER_PORT;

    socks[sockcnt++] = make_socket(port, name.sin_addr.s_addr);
}

void
show_program_usage(char *prog)
{
    exit(1);
}

/* main():                                  */
/*  The beginning of it all.                */
int
main(int argc, char **argv, char **envt)
{
    int i;

    fd_set input_set, output_set;
    t_descr *d, *dnext;
    int running = 1;

    for (i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (!strcmp(argv[i], "-b") || !strcmp(argv[i], "--bind")) {
                if (i + 1 >= argc)
                    show_program_usage(*argv);
                bind_to(argv[++i]);
            } else if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--daemon")) {
                daemon_mode++;
            } else if (!strcmp(argv[i], "-v")) {
                if (!verbose_mode++)
                    verb("Version " RESOLVER_VERSION " (" PROTOVER ") starting.");
                verbose_mode++;
            } else if (!strcmp(argv[i], "-V")) {
                printf("reslvd version: " RESOLVER_VERSION " (" PROTOVER ")\n");
                exit(0);
            }
        }
    }

    verb_2("Setting signals.");
    set_signals();

    if (daemon_mode) {
        verb("Forking into background.");
        fclose(stdin);
        fclose(stdout);
        if (!verbose_mode)
            fclose(stderr);
        if (fork() != 0)
            exit(0);
    }

    if (!sockcnt)
        socks[sockcnt++] = make_socket(RESOLVER_PORT, INADDR_ANY);

    verb("Entering main loop.");

    while (running) {
        FD_ZERO(&output_set);
        FD_ZERO(&input_set);

        for (i = 0; i < sockcnt; i++)
            FD_SET(socks[i], &input_set);

        for (d = descr_list; d; d = dnext) {
            dnext = d->next;

            if (d->booted) {
                disconnect_descr(d);
            } else {
                FD_SET(d->descriptor, &input_set);
                //if (d->buffer.out) { /* Prepare write pool */
                //    FD_SET(d->descriptor, &output_set);
                //}
            }
        }

        select(maxd, &input_set, &output_set, NULL, NULL);

        for (i = 0; i < sockcnt; i++)
            if (FD_ISSET(socks[i], &input_set))
                new_connection(socks[i]);

        for (d = descr_list; d; d = d->next) {
            if (FD_ISSET(d->descriptor, &input_set))
                process_input(d);
            //if (FD_ISSET(d->descriptor, &output_set))
            //    process_output(d);
        }
    }

    return 0;
}
