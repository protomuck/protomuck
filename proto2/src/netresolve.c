#include "copyright.h"
#include "config.h"

#include "defaults.h"
#include "db.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "externs.h"
#include "netresolve.h"         /* hinoserm */

struct hostinfo *hostdb = NULL; /* the main host cache */
struct husrinfo *userdb = NULL; /* the username list */
unsigned long userdb_count = 0; /* number of username entries in user cache */
unsigned long hostdb_count = 0; /* number of entries in the host cache */
unsigned long hostdb_flushed = 0; /* number of entries purged during last flush */
time_t hostdb_flushtime = 0;    /* time of last hostcache flush */
dbref hostdb_flushplyr = NOTHING; /* ref of player if last flush was from #flush */

#ifdef IPV6

const char *
ip_address_prototype(void* x, int xsize)
{
    static char buf[64];
    int y;

    if (xsize <= 4) {
	y = ntohl(*(unsigned int*)x);
	inet_ntop(AF_INET, &y, (char *)buf, 64);
    } else {
	inet_ntop(AF_INET6, x, (char *)buf, 64);
    }
    return buf;
}

const char *hostToIPex(struct hostinfo * h)
{
    /* if the address is filled with either 0.0.0.0 or 255.255.255.255
       assume its ipv6 */
    if (h->a == -1)
#ifdef IPV6
	return ip_address_prototype(&h->a6, 16);
#else
	return "255.255.255.255";
#endif
    else
	return ip_address_prototype(&h->a, 4);
}

#else

const char *hostToIPex(struct hostinfo * h)
{
	return host_as_hex(h->a);
}

#endif

#if defined(HAVE_PTHREAD_H) || defined(WIN_VC)

/* Beginnings of very basic threaded resolver. */

struct tres_data {
    struct hostinfo * h;
    /* struct husrinfo * u; */
    unsigned short lport;
    unsigned short prt;
};

const char *
get_username(int a, int prt, int myprt)
{
    int fd, len, result;
    struct sockaddr_in addr;
    char *ptr, *ptr2;
    static char buf[1024];
    int lasterr;
    int timeout = 30;

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
        }
    } while (result < 0 && lasterr == EINPROGRESS);
    if (result < 0 && lasterr != EISCONN) {
        goto bad;
    }

    sprintf(buf, "%d,%d\n", prt, myprt);
    do {
        result = writesocket(fd, buf, strlen(buf));
        lasterr = errno;
        if (result < 0) {
            if (!timeout--)
                break;
        }
    } while (result < 0 && lasterr == EAGAIN);
    if (result < 0)
        goto bad2;

    do {
        result = readsocket(fd, buf, sizeof(buf));
        lasterr = errno;
        if (result < 0) {
            if (!timeout--)
                break;
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
    closesocket(fd);
    if ((ptr2 = index(ptr, '\r')))
        *ptr2 = '\0';
    if (!*ptr)
        return (0);
    return ptr;

  bad2:
    shutdown(fd, 2);

  bad:
    closesocket(fd);
    return (0);
}

#ifdef WIN_VC
DWORD WINAPI threaded_resolver_go(void *ptr)
#else
void *threaded_resolver_go(void *ptr)
#endif
{
    struct in_addr addr;
    struct tres_data *tr;
    struct hostent *he;
    char *old_ptr;

    tr = (struct tres_data *)ptr;

    addr.s_addr = htonl(tr->h->a);

    he = gethostbyaddr(((char *) &addr), sizeof(addr), AF_INET);

    if (he) {
        char buf[MAX_COMMAND_LEN];
        char *j;
        const char *c;
        struct husrinfo *u;


        /*-------------------------------------------------------------*/
        /* I just realized how bad it is to call log_status from a     */
        /*  thread, especially with MCCP (zlib) compression. -Hinoserm */
        /*-------------------------------------------------------------*/
        /* if (tr->h->wupd && strcmp(tr->h->name, he->h_name))         */
        /*    log_status("*RES: %s to %s\n", tr->h->name, he->h_name); */
        /*-------------------------------------------------------------*/

        
        old_ptr = tr->h->name;
        tr->h->name = alloc_string(he->h_name);
        free((void *) old_ptr);
        tr->h->wupd = current_systime;

        if ((c = get_username(htonl(tr->h->a), tr->prt, tr->lport))) {
            strcpy(buf, c);
            for (j = buf; isspace(*j); ++j) ; /* strip occasional leading spaces */
            for (u = userdb; u; u = u->next) {
                if (u->uport == tr->prt && u->a == tr->h->a) {
                    old_ptr = u->user;
                    u->user = alloc_string(j);
                    free((void *) old_ptr);
                    /* break; */
                }
            }
        }
    }

    free((void *) tr);
#ifdef WIN_VC
# ifndef __cplusplus
	/* Supposedly, when using C++ and WINAPI, you just return from the thread function. */
	ExitThread(0);
# endif
#else
    pthread_exit(NULL);
#endif
	return NULL;
}


bool
host_get_resthrd(struct hostinfo *h, unsigned short lport, unsigned short prt)
{
	struct tres_data *tr;
#ifndef WIN_VC
	int result;
	pthread_t thread1;
    pthread_attr_t attr;
#endif

    tr = (struct tres_data *) malloc(sizeof(struct tres_data));

    tr->h = h;
    /* tr->u = u; */
    tr->lport = lport;
    tr->prt = prt;
   
#ifdef WIN_VC
	CreateThread(NULL, 0, threaded_resolver_go, (void*)tr, 0, NULL);
#else
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    
    result = pthread_create(&thread1, &attr, threaded_resolver_go, (void*) tr);
 
    pthread_attr_destroy(&attr);

	if (result)
        return 0;
    else
#endif
        return 1;
}

#endif


#ifdef USE_RESLVD

char *reslvd_buf = NULL;
char *reslvd_buf_at = NULL;
int reslvd_sock = 0;
bool reslvd_connected = 0;

int
reslvd_open(void)
{
    char buf[BUFFER_LEN];
    struct sockaddr_in addr;
    struct hostent *he;
    char *p;

    if (reslvd_connected || !*tp_reslvd_address)
        return 0;

    strcpy(buf, tp_reslvd_address);

    for (p = buf; *p && (*p != ':'); p++) ;
    if (*p)
        *p++ = '\0';

    if (*buf && (he = gethostbyname(buf))) {
        bcopy((char *) he->h_addr, (char *) &addr.sin_addr, he->h_length);
    } else if ((he = gethostbyname(RESOLVER_HOST))) {
        bcopy((char *) he->h_addr, (char *) &addr.sin_addr, he->h_length);
    } else {
        log_status("RLVD: Failed to find host.\n");
        return -1;
    }

    addr.sin_family = AF_INET;
    if (!(addr.sin_port = htons(atoi(p))))
        addr.sin_port = htons(RESOLVER_PORT);

    if ((reslvd_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;

    if (connect(reslvd_sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
        closesocket(reslvd_sock);
        reslvd_sock = 0;
        return -1;
    }

    make_nonblocking(reslvd_sock);
    check_maxd(reslvd_sock);

    reslvd_connected = 1;
    return 1;
}

void
reslvd_close(void)
{
    if (!reslvd_connected)
        return;

    shutdown(reslvd_sock, 2);
    closesocket(reslvd_sock);

    if (reslvd_buf)
        free((void *) reslvd_buf);

    reslvd_buf = NULL;
    reslvd_buf_at = NULL;
    reslvd_sock = 0;
    reslvd_connected = 0;
}

void
reslvd_disc(void)
{
    log_status("RLVD: Connection to daemon broken.\n");
    reslvd_close();
#ifdef SPAWN_HOST_RESOLVER
    if (reslvd_open() != 1)
        spawn_resolver();
#else /* !SPAWN_HOST_RESOLVER */
    reslvd_open();
#endif /* SPAWN_HOST_RESOLVER */
}

void
reslvd_input(void)
{
    char buf[MAX_COMMAND_LEN];
    char buf2[MAX_COMMAND_LEN];
    char *p, *pend, *q, *qend;
    int got, a;

    if (!reslvd_connected)
        return;

    if ((got = readsocket(reslvd_sock, buf, sizeof buf)) <= 0) {
        reslvd_disc();
        return;
    }

    if (!reslvd_buf) {
        reslvd_buf = (char *) malloc(sizeof(char) * MAX_COMMAND_LEN);
        reslvd_buf_at = reslvd_buf;
    }

    p = reslvd_buf_at;
    pend = reslvd_buf + MAX_COMMAND_LEN - 1;
    for (q = buf, qend = buf + got; q < qend; q++) {
        if (*q == '\n') {
            *p = '\0';
            if (p > reslvd_buf) {
                if (!strncmp(reslvd_buf, "USER", 4)) {
                    struct husrinfo *u;
                    int myprt, port;
                    char *j;

                    sscanf(reslvd_buf, "USER %x,%d,%d,%s", &a, &myprt, &port,
                           (char *) &buf2);
                    for (j = buf2; isspace(*j); ++j) ; /* strip occasional leading spaces */
                    for (u = userdb; u; u = u->next) {
                        if (u->uport == port && u->a == a) {
                            free((void *) u->user);
                            u->user = alloc_string(j);
                        }
                    }
                } else if (!strncmp(reslvd_buf, "HOST", 4)) {
                    struct hostinfo *h;

                    sscanf(reslvd_buf, "HOST %x,%s", &a, (char *) &buf2);
#ifdef HOSTCACHE_DEBUG
                    log_status("RLVD: GOT: %X (%s):%s\n", a, host_as_hex(a),
                               buf2);
#endif /* HOSTCACHE_DEBUG */
                    for (h = hostdb; h; h = h->next) {
                        if (h->a == a) {
                            if (h->wupd && strcmp(h->name, buf2))
                                log_status("*RES: %s to %s\n", h->name, buf2);
                            free((void *) h->name);
                            h->name = alloc_string(buf2);
                            h->wupd = current_systime;
                        }
                    }
                }
            }
            p = reslvd_buf;
        } else if (p < pend) {
            if ((*q == 8 || *q == 127)) {
                if (p > reslvd_buf)
                    p--;
            } else if (*q != 13) {
                *p++ = *q;
            }
        }
    }

    if (p > reslvd_buf) {
        reslvd_buf_at = p;
    } else {
        free((void *) reslvd_buf);
        reslvd_buf = NULL;
        reslvd_buf_at = NULL;
    }

    return;
}

/* host_get_reslvd():                                               */
/*  This is the send request function for the shared resolver.      */
bool
host_get_reslvd(struct hostinfo * h, unsigned short lport, unsigned short prt)
{
    char buf[MAX_COMMAND_LEN];

    if (!reslvd_connected)
        return 0;

    sprintf(buf, "HOST %X\n", h->a);
    if (writesocket(reslvd_sock, buf, strlen(buf)) < 1) {
        reslvd_disc();
        return 0;
    }

    if (lport && prt) {
        sprintf(buf, "USER %X,%d,%d\n", h->a, lport, prt);
        if (writesocket(reslvd_sock, buf, strlen(buf)) < 1) {
            reslvd_disc();
            return 0;
        }
    }

    return 1;
}

#endif /* USE_RESLVD */

#ifdef SPAWN_HOST_RESOLVER

int resolverpid = 0;
int resolver_sock[2];

void
kill_resolver(void)
{
    int i;
    pid_t p;

    resolverpid = 0;
    write(resolver_sock[1], "QUIT\n", 5);
    p = wait(&i);
}

void
spawn_resolver(void)
{
    socketpair(AF_UNIX, SOCK_STREAM, 0, resolver_sock);
    make_nonblocking(resolver_sock[1]);
    if (!(resolverpid = fork())) {
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

    check_maxd(resolver_sock[0]);
    check_maxd(resolver_sock[1]);

    fprintf(stderr, "INIT: external host resolver started\n");
}

void
resolve_hostnames(void)
{
    char buf[BUFFER_LEN];
    char *ptr, *ptr2, *ptr3, *hostip, *port, *hostname, *username, *tempptr;
    int got, iport, dc;
    int ipnum;

    if ((got = read(resolver_sock[1], buf, sizeof(buf))) < 0)
        return;

    if (got == sizeof(buf)) {
        got--;
        while (got > 0 && buf[got] != '\n')
            buf[got--] = '\0';
    }

    ptr = buf;
    dc = 0;
    do {
        for (ptr2 = ptr; *ptr && *ptr != '\n' && dc < got; ptr++, dc++) ;
        if (*ptr) {
            *ptr++ = '\0';
            dc++;
        }
        if (*ptr2) {
            ptr3 = index(ptr2, ':');
            if (!ptr3)
                return;
            hostip = ptr2;
            port = index(ptr2, '(');
            if (!port)
                return;
            tempptr = index(port, ')');
            if (!tempptr)
                return;
            *tempptr = '\0';
            hostname = ptr3;
            username = index(ptr3, '(');
            if (!username)
                return;
            tempptr = index(username, ')');
            if (!tempptr)
                return;
            *tempptr = '\0';
            if (*port && *hostname && *username) {
                *port++ = '\0';
                *hostname++ = '\0';
                *username++ = '\0';

                if ((ipnum = str2ip(hostip)) != -1) {
                    struct hostinfo *h;

#ifdef HOSTCACHE_DEBUG
                    log_status("HOST: GOT: %X (%s):%s\n", ipnum,
                               host_as_hex(ipnum), hostname);
#endif /* HOSTCACHE_DEBUG */

                    for (h = hostdb; h; h = h->next) {
                        if (h->a == ipnum) {
                            if (h->wupd && strcmp(h->name, hostname))
                                log_status("*RES: %s to %s\n", h->name,
                                           hostname);
                            free((void *) h->name);
                            h->name = alloc_string(hostname);
                            h->wupd = current_systime;
                        }
                    }

                    if ((iport = atoi(port))) {
                        struct husrinfo *u;

                        while (isspace(*username))
                            ++username; /* strip occasional leading spaces */

                        for (u = userdb; u; u = u->next) {
                            if (u->uport == iport && u->a == ipnum) {
                                free((void *) u->user);
                                u->user = alloc_string(username);
                            }
                        }
                    }
                } else {
                    log_status("*BUG: resolve_hostnames bad ipstr %s\n",
                               hostip);
                }
            }
        }
    } while (dc < got && *ptr);
}

/* host_get_oldres():                                               */
/*  This is the send request function for the old resolver process. */
bool
host_get_oldres(struct hostinfo *h, unsigned short lport, unsigned short prt)
{
    char buf[200];

    if (!resolverpid)
        return 0;

    sprintf(buf, "%d.%d.%d.%d(%u)%u\n",
            (h->a >> 24) & 0xff,
            (h->a >> 16) & 0xff, (h->a >> 8) & 0xff, h->a & 0xff, prt, lport);

    if (write(resolver_sock[1], buf, strlen(buf)) < 1)
        return 0;

    return 1;
}
#endif


/* host_get_oldstyle():                                             */
/*  This is the old, original lookup method, with no added resolver */
/*  process at all.  This is only ever used as a fallback.          */
bool
host_get_oldstyle(struct hostinfo * h)
{
    struct in_addr addr;
    static time_t secs_lost = 0;

    addr.s_addr = htonl(h->a);

    /* One day the nameserver Qwest uses decided to start */
    /* doing halfminute lags, locking up the entire muck  */
    /* that long on every connect.  This is intended to   */
    /* prevent that, reduces average lag due to nameserver */
    /* to 1 sec/call, simply by not calling nameserver if */
    /* it's in a slow mood *grin*. If the nameserver lags */
    /* consistently, a hostname cache ala OJ's tinymuck2.3 */
    /* would make more sense:                             */

    if (secs_lost) {
        secs_lost--;
    } else {
        time_t gethost_start = current_systime;
        struct hostent *he = gethostbyaddr(((char *) &addr),
                                           sizeof(addr), AF_INET);
        time_t gethost_stop = time(&current_systime);
        time_t lag = gethost_stop - gethost_start;

        if (lag > 10) {
            secs_lost = lag;
#if MIN_SECS_TO_LOG
            if (lag >= CFG_MIN_SECS_TO_LOG) {
                log_status("GETHOSTBYNAME-RAN: secs %3d\n", lag);
            }
#endif
        }
        if (he) {
            if (h->wupd && strcmp(h->name, he->h_name))
                log_status("*RES: %s to %s\n", h->name, he->h_name);
            free((void *) h->name);
            h->name = alloc_string(he->h_name);
            h->wupd = current_systime;
            return 1;
        }
    }

    return 0;
}

#ifdef IPV6
char *
host_get_ipv6addr(struct hostinfo * h)
{
    struct in6_addr addr;
    char buf[INET_ADDRSTRLEN];

    addr = h->a6;
    inet_ntop(AF_INET6, ((char *) &addr.s6_addr), buf, INET6_ADDRSTRLEN);
    if (*buf) {
        if (h->wupd && strcmp(h->name, buf)) 
	    log_status("*RES: %s to %s\n", h->name, buf);
        free((void *) h->name);
        h->name = alloc_string(buf);
        h->wupd = current_systime;
    }
    return h->name;
}
#endif

void
host_request(struct hostinfo *h, unsigned short lport, unsigned short prt)
{
    if (!tp_hostnames)
        return;

#ifdef HOSTCACHE_DEBUG
    log_status("HOST: Request: %X (%s)\n", h->a, host_as_hex(h->a));
#endif /* HOSTCACHE_DEBUG */


#ifdef USE_RESLVD
    if (host_get_reslvd(h, lport, prt))
#endif
#ifdef SPAWN_HOST_RESOLVER
        if (!host_get_oldres(h, lport, prt))
#endif
#if defined(HAVE_PTHREAD_H)
            if (!host_get_resthrd(h, lport, prt))
#endif
            host_get_oldstyle(h);

    
}

struct huinfo *
host_getinfo(int a, unsigned short lport, unsigned short prt)
{
    struct hostinfo *h;
    struct husrinfo *u;
    struct huinfo *hu = (struct huinfo *) malloc(sizeof(struct huinfo));

    prt = ntohs(prt);
    a = ntohl(a);

    if (prt) {                  /* only if username is requested */
        char smbuf[32];
        u = (struct husrinfo *) malloc(sizeof(struct husrinfo));
        u->user = alloc_string(intostr(smbuf, prt));
        u->next = userdb;
        u->a = a;
#ifdef IPV6
	u->a6 = in6addr_any; /* Same as NULL! */
#endif
        u->uport = prt;
        u->prev = NULL;
        if (userdb)
            userdb->prev = u;
        userdb = u;
        hu->u = u;
        userdb_count++;
    } else                      /* This case is mostly only for eventual MUF-based lookups. */
        hu->u = NULL;

    for (h = hostdb; h; h = h->next) {
        if (h->a == a) {
            /* Found it in the cache. */
            h->links++;
            h->uses++;
#ifdef HOSTCACHE_DEBUG
            log_status("HOST: In cache: %X (%s), %s\n", h->a, host_as_hex(h->a),
                       h->name);
#endif /* HOSTCACHE_DEBUG */
            /* Only re-request if the time is right. Might eventually */
            /*  want to move this to a seperate @tune.                */
            if (current_systime - h->wupd > tp_clean_interval)
                host_request(h, lport, prt);
            hu->h = h;
            return hu;
        }
    }

#ifdef HOSTCACHE_DEBUG
    log_status("HOST: New (not in cache): %X (%s)\n", a, host_as_hex(a));
#endif /* HOSTCACHE_DEBUG */

    /* Not in the cache, make a new entry and request it. */
    h = (struct hostinfo *) malloc(sizeof(struct hostinfo));
    h->links = 1;
    h->uses = 1;
    h->a = a;
#ifdef IPV6
    h->a6 = in6addr_any;
#endif
    h->wupd = 0;
    h->name = alloc_string(host_as_hex(a));
    h->prev = NULL;
    h->next = hostdb;
    if (hostdb)
        hostdb->prev = h;
    hostdb = h;

    hostdb_count++;
    host_request(h, lport, prt);
    hu->h = h;
    return hu;
}

#ifdef IPV6
struct huinfo *
host_getinfo6(struct in6_addr a6, unsigned short lport, unsigned short prt)
{
    struct hostinfo *h;
    struct husrinfo *u;
    struct huinfo *hu =
        (struct huinfo *) malloc(sizeof(struct huinfo));

    prt = ntohs(prt);

    if (prt) {                  /* only if username is requested */
        char smbuf[32];
        u = (struct husrinfo *) malloc(sizeof(struct husrinfo));
        u->user = alloc_string(intostr(smbuf, prt));
        u->next = userdb;
        u->a = -1;
	u->a6 = a6;
        u->uport = prt;
        u->prev = NULL;
        if (userdb)
            userdb->prev = u;
        userdb = u;
        hu->u = u;
        userdb_count++;
    } else                      /* This case is mostly only for eventual MUF-based lookups. */
        hu->u = NULL;

    for (h = hostdb; h; h = h->next) {
        if (h->a6.s6_addr == a6.s6_addr) {
            /* Found it in the cache. */
            h->links++;
            h->uses++;
#ifdef HOSTCACHE_DEBUG
            log_status("HOST: In cache: %X (%s), %s\n", h->a, host_as_hex(h->a),
                       h->name);
#endif /* HOSTCACHE_DEBUG */
            /* Only re-request if the time is right. Might eventually */
            /*  want to move this to a seperate @tune.                */
            if (current_systime - h->wupd > tp_clean_interval)
                host_get_ipv6addr(h);
            hu->h = h;
            return hu;
        }
    }

#ifdef HOSTCACHE_DEBUG
    log_status("HOST: New (not in cache): %X (%s)\n", a, host_as_hex(a));
#endif /* HOSTCACHE_DEBUG */

    /* Not in the cache, make a new entry and request it. */
    h = (struct hostinfo *) malloc(sizeof(struct hostinfo));
    h->links = 1;
    h->uses = 1;
    h->a = -1;
    h->a6 = a6;
    h->wupd = 0;
    h->name = alloc_string("::");
    h->prev = NULL;
    h->next = hostdb;
    if (hostdb)
        hostdb->prev = h;
    hostdb = h;

    hostdb_count++;
    host_get_ipv6addr(h);
    hu->h = h;
    return hu;
}
#endif

void
host_delete(struct huinfo *hu)
{
    hu->h->links--;

    if (hu->u) {
        if (hu->u->next)
            hu->u->next->prev = hu->u->prev;
        if (hu->u->prev)
            hu->u->prev->next = hu->u->next;
        if (hu->u == userdb)
            userdb = hu->u->next;
        free((void *) hu->u->user);
        free((void *) hu->u);
        userdb_count--;
    }

    free((void *) hu);
}

void
host_free(struct hostinfo *h)
{
    if (h->next)
        h->next->prev = h->prev;
    if (h->prev)
        h->prev->next = h->next;
    if (h == hostdb)
        hostdb = h->next;
    hostdb_count--;
    free((void *) h->name);
    free((void *) h);
}

void
host_flush(bool all)
{
    struct hostinfo *h = hostdb;
    struct hostinfo *h2;

    while (h) {
        h2 = h->next;
        if (all || !h->links)
            host_free(h);
        h = h2;
    }
}

void
host_load(void)
{
    FILE *f;
    unsigned int ip;
    char name[129];
    char *p = name;
    struct hostinfo *h;

    if (!(f = fopen("nethost.cache", "r")))
        return;

    host_flush(0);

    while (fscanf(f, "%x %s\n", &ip, p) == 2) {
        h = (struct hostinfo *) malloc(sizeof(struct hostinfo));
        h->links = 0;
        h->uses = 0;
        h->wupd = 0;
        h->a = (int) ip;
        h->name = alloc_string(name);
        h->prev = NULL;
        h->next = hostdb;
        if (hostdb)
            hostdb->prev = h;
        hostdb = h;
        hostdb_count++;
    }

    fclose(f);
}

void
host_save(void)
{
    FILE *f;
    struct hostinfo *h;

    if (!(f = fopen("nethost.cache", "w")))
        return;

    for (h = hostdb; h; h = h->next) {
        /* We only want to store updated hostnames.  Since h->name will be */
        /*  the same as the IP address if a hostname wasn't found, we use  */
        /*  strcmp to make sure it never stores anything except hostnames  */
        /*  into the file.  -Hinoserm                                      */
        if (h->wupd && strcmp(h->name, host_as_hex(h->a)))
            fprintf(f, "%X %s\n", h->a, h->name);
    }

    fclose(f);
}

void
host_check_cache(void)
{
    struct hostinfo *h;

    if (hostdb_count > 512) {
        hostdb_flushed = 0;
        hostdb_flushtime = current_systime;
        hostdb_flushplyr = NOTHING;
    }

    for (h = hostdb; h; h = h->next) {
        if (!h->wupd && h->links) {
            /* If the host entry is in use, yet never got */
            /*  a result, we try again to look it up.     */
            host_request(h, 0, 0);
        } else if (!h->links && hostdb_count > 512 && (current_systime - h->wupd) > (tp_clean_interval * h->uses)) { /* tp_max_cached_hosts */
            /* If the host entry is not in use, and the total entry  */
            /*  count is above tp_max_cached_hosts, and the entry is */
            /*  older than (tp_clean_interval * h->uses), free it.   */
            host_free(h);
            hostdb_flushed++;
        }
    }
}

void
host_init(void)
{
    /* This function is redundant, someone needs to take */
    /* the time to rename host_load() to host_init().    */
    host_load();
}

void
host_shutdown(void)
{
    host_save();                /* Save active cache to file. */
    host_flush(1);              /* And then wipe the cache from memory. */
}

unsigned long
host_hostdb_size(void)
{
    struct hostinfo *h;
    unsigned long hsize =
        ((sizeof(struct hostinfo) + 1) * hostdb_count) + sizeof(hostdb_count) +
        sizeof(hostdb_flushed) + sizeof(hostdb_flushtime) +
        sizeof(hostdb_flushplyr);
    /* The +1 above is because each h->name string has a \0 byte, */
    /*  which strlen won't see.  Keeps the add instruction out of */
    /*  the loop.                                                 */

    for (h = hostdb; h; h = h->next)
        hsize += strlen(h->name);

    return hsize;
}

unsigned long
host_userdb_size(void)
{
    struct husrinfo *u;
    unsigned long usize =
        ((sizeof(struct husrinfo) + 1) * userdb_count) + sizeof(userdb_count);
    /* The +1 above is because each u->user string has a \0 byte, */
    /*  which strlen won't see.  Keeps the add instruction out of */
    /*  the loop.                                                 */

    for (u = userdb; u; u = u->next)
        usize += strlen(u->user);

    return usize;
}

/*-- Everything past here is pretty much --*/
/*--  for the @hostcache command.        --*/

char *
time_format_3(time_t dt)
{
    struct tm *delta;
    static char buf[64];

    if (!dt)
        return strcpy(buf, "---");

    dt = current_systime - dt;

    delta = gmtime(&dt);

    if (delta->tm_yday > 0)
        sprintf(buf, "%dd %02d:%02d ago",
                delta->tm_yday, delta->tm_hour, delta->tm_min);
    else
        sprintf(buf, "%02d:%02d:%02ds ago", delta->tm_hour, delta->tm_min,
                delta->tm_sec);
    return buf;
}

int
hostsort_mostused(const void *x, const void *y)
{
    struct hostinfo *a = *(struct hostinfo **) y;
    struct hostinfo *b = *(struct hostinfo **) x;

    if (a->uses > b->uses)
        return 1;
    else if (a->uses < b->uses)
        return -1;
    else if (a->links > b->links)
        return 1;
    else if (a->links < b->links)
        return -1;
    else if (a->wupd > b->wupd)
        return 1;
    else if (a->wupd < b->wupd)
        return -1;
    else
        return string_compare(b->name, a->name);
}

void
do_hostcache(dbref player, const char *args)
{
    struct hostinfo *h;
    struct hostinfo *hnext;
    char arg1[BUFFER_LEN];
    char *arg2;

    strcpy(arg1, args);

    for (arg2 = arg1; *arg2 && !isspace(*arg2); arg2++) ;
    if (*arg2)
        *arg2++ = '\0';
    while (isspace(*arg2))
        arg2++;

    if (string_prefix(arg1, "#re")) {
        /* This code here is a notch messy, but it works. */
#if defined(USE_RESLVD) && defined(SPAWN_HOST_RESOLVER)
        kill_resolver();
        reslvd_close();
        if (reslvd_open() != 1)
            spawn_resolver();
#elif defined(USE_RESLVD)
        reslvd_close();
        reslvd_open();
#elif defined(SPAWN_HOST_RESOLVER)
        kill_resolver();
        spawn_resolver();
#endif
        /* Sure is non-informative.  Hope to eventually change that. */
        anotify_fmt(player, CSUCC "Done.");
    } else if (string_prefix(arg1, "#fl")) {
        bool doall = (!strcasecmp(arg2, "all"));

        hostdb_flushed = 0;
        hostdb_flushplyr = player;

        anotify_fmt(player, CINFO "Flushing...");

        for (h = hostdb; h; h = hnext) {
            hnext = h->next;
            if (!h->links
                && (doall
                    || (current_systime - h->wupd) >
                    (tp_clean_interval * h->uses))) {
                host_free(h);
                hostdb_flushed++;
            }
        }

        hostdb_flushtime = current_systime;
        anotify_fmt(player, CSUCC "Done. " CINFO "%ld %s flushed.",
                    hostdb_flushed,
                    (hostdb_flushed == 1) ? "entry" : "entries");
    } else if (string_prefix(arg1, "#sh")) {
        struct hostinfo **harr;
        unsigned long i = 0;
        unsigned long count = 0;

        if (strcasecmp(arg2, "all")) {
            count = atoi(arg2);
            if (count < 1)
                count = 10;
            if (count > hostdb_count)
                count = hostdb_count;
        } else
            count = hostdb_count;

        anotify_fmt(player,
                    CINFO "IP Number         Use  Last Updated   Hostname");

        harr =
            (struct hostinfo **) malloc(hostdb_count *
                                        sizeof(struct hostinfo *));
        for (h = hostdb; h; h = h->next)
            harr[i++] = h;
        qsort(harr, hostdb_count, sizeof(struct hostinfo *),
              (*hostsort_mostused));

        for (i = 0; i < count; i++)
            anotify_fmt(player, " %-15s  %0.3d  %-14s %s",
                        host_as_hex(harr[i]->a), harr[i]->uses,
                        time_format_3(harr[i]->wupd), harr[i]->name);
        anotify_fmt(player, CINFO "Top %d host%s displayed (%d total).", count,
                    count == 1 ? "" : "s", hostdb_count);
    } else {
        anotify_fmt(player, "Bytes used by cache:  %ld", host_hostdb_size());
        anotify_fmt(player, "Hostnames in cache:   %ld", hostdb_count);
        anotify_fmt(player, "Username entries:     %ld", userdb_count);
        anotify_fmt(player, "Username bytes used:  %ld", host_userdb_size());
        if (hostdb_flushtime)
            anotify_fmt(player, "Last flush was:       %s",
                        time_format_3(hostdb_flushtime));
        if (hostdb_flushplyr != NOTHING)
            anotify_fmt(player, "  Initiated by:       %s",
                        unparse_object(player, hostdb_flushplyr));
        if (hostdb_flushtime)
            anotify_fmt(player, "  Entries flushed:    %ld", hostdb_flushed);
    }
}
