#include "copyright.h"
#include "config.h"
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>
#ifdef WIN_VC
# include <string.h>
#else
# include <sys/ioctl.h>
# include <sys/wait.h>
# include <sys/socket.h>
# include <sys/errno.h>
# include <netinet/in.h>
# include <netdb.h>
#endif
#include "defaults.h"
#include "db.h"
#include "interface.h"
#include "params.h"
#include "tune.h"
#include "externs.h"
#include "netresolve.h"         /* hinoserm */

//#ifndef HOSTCACHE_DEBUG
//#define HOSTCACHE_DEBUG
//#endif

struct hostinfo *hostdb = NULL; /* the main host cache */
struct husrinfo *userdb = NULL; /* the username list */
unsigned int hostdb_count = 0; /* number of entries in the host cache */

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
                    log_status("HOST: GOT: %X (%s):%s\n", ipnum, host_as_hex(ipnum), hostname);
#endif /* HOSTCACHE_DEBUG */

                    for (h = hostdb; h; h = h->next) {
                        if (h->a == ipnum) {
                            if (h->wupd && strcmp(h->name, hostname))
                                log_status("*RES: %s to %s\n", h->name,
                                           hostname);
                            strcpy(h->name, hostname);
                            h->wupd = current_systime;
                        }
                    }

                    if ((iport = atoi(port))) {
                        struct husrinfo *u;

                        while (isspace(*username))
                            ++username; /* strip occasional leading spaces */

                        for (u = userdb; u; u = u->next) {
                            if (u->uport == iport && u->a == ipnum)
                                strcpy(u->user, username);
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
    static int secs_lost = 0;

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
            strcpy(h->name, he->h_name);
            h->wupd = current_systime;
            return 1;
        }
    }

    return 0;
}

void
host_request(struct hostinfo *h, unsigned short lport, unsigned short prt)
{
    if (!tp_hostnames)
        return;

#ifdef HOSTCACHE_DEBUG
    log_status("HOST: Request: %X (%s)\n", h->a, host_as_hex(h->a));
#endif /* HOSTCACHE_DEBUG */

#ifdef SPAWN_HOST_RESOLVER
    if (!host_get_oldres(h, lport, prt))
#endif
        host_get_oldstyle(h);
}

struct huinfo *
host_getinfo(int a, unsigned short lport, unsigned short prt)
{
    register struct hostinfo *h;
    register struct husrinfo *u;
    register struct huinfo *hu = (struct huinfo *) malloc(sizeof(struct huinfo));

    prt = ntohs(prt);
    a = ntohl(a);

    if (prt) {                  /* only if username is requested */
        u = (struct husrinfo *) malloc(sizeof(struct husrinfo));
        sprintf(u->user, "%d", prt);
        u->next = userdb;
        u->a = a;
        u->uport = prt;
        u->prev = NULL;
        if (userdb)
            userdb->prev = u;
        userdb = u;
        hu->u = u;
    } else
        hu->u = NULL;

    for (h = hostdb; h; h = h->next) {
        if (h->a == a) {
            h->links++;
            h->uses++;
#ifdef HOSTCACHE_DEBUG
            log_status("HOST: In cache: %X (%s), %s\n", h->a, host_as_hex(h->a), h->name);
#endif /* HOSTCACHE_DEBUG */
            if (current_systime - h->wupd > 80)
                host_request(h, lport, prt);
            hu->h = h;
            return hu;
        }
    }
#ifdef HOSTCACHE_DEBUG
    log_status("HOST: New (not in cache): %X (%s)\n", a, host_as_hex(a));
#endif /* HOSTCACHE_DEBUG */

    h = (struct hostinfo *) malloc(sizeof(struct hostinfo));
    h->links = 1;
    h->uses = 1;
    h->a = a;
    h->wupd = 0;
    strcpy(h->name, host_as_hex(a));
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

void
host_delete(struct huinfo *hu)
{
    hu->h->links--;

/*    if (hostdb_count > 200 && current_systime - hu->h->wupd > 7200) */ /* || tp_host_cache_cleantime < 0 */
/*        host_free(hu->h); */

    if (hu->u) {
        if (hu->u->next)
            hu->u->next->prev = hu->u->prev;
        if (hu->u->prev)
            hu->u->prev->next = hu->u->next;
        if (hu->u == userdb)
            userdb = hu->u->next;
        free((void *) hu->u);
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
        h->a = (int)ip;
        strcpy(h->name, name);
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
        if (!strcmp(h->name, host_as_hex(h->a)))
            fprintf(f, "%X %s\n", h->a, h->name);
    }

    fclose(f);
}

void
host_check_cache(void)
{
    struct hostinfo *h;
    
    for (h = hostdb; h; h = h->next) {
        if (!h->wupd && h->links)
            host_request(h, 0, 0);
    }
}

void
host_init(void)
{
    host_load();
}

void
host_shutdown(void)
{
    host_save();
    host_flush(1);
}

char *
time_format_3(time_t dt)
{
    register struct tm *delta;
    static char buf[64];

    if (!dt)
        return strcpy(buf, "---");

    dt = current_systime - dt;

    delta = gmtime(&dt);

    if (delta->tm_yday > 0)
        sprintf(buf, "%dd %02d:%02d ago",
                delta->tm_yday, delta->tm_hour, delta->tm_min);
    else
        sprintf(buf, "%02d:%02d:%02ds ago", delta->tm_hour, delta->tm_min, delta->tm_sec);
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
    char arg1[BUFFER_LEN];
    char *arg2;

    strcpy(arg1, args);

    for (arg2 = arg1; *arg2 && !isspace(*arg2); arg2++) ;
    if (*arg2)
        *arg2++ = '\0';
    while(isspace(*arg2)) arg2++;

    if (string_prefix(arg1, "#sh")) {
        struct hostinfo **harr;
        register int i = 0;
        register int count = 0;

        if (strcasecmp(arg2, "all")) {
            count = atoi(arg2);
            if (count < 1)
                count = 10;
            if (count > hostdb_count)
                count = hostdb_count;
        } else
            count = hostdb_count;

        anotify_fmt(player, CINFO "IP Number         Use  Last Updated   Hostname");

        harr = (struct hostinfo **)malloc(hostdb_count * sizeof(struct hostinfo *));
        for (h = hostdb; h; h = h->next)
            harr[i++] = h;
        qsort(harr, hostdb_count, sizeof(struct hostinfo *), (*hostsort_mostused));
        
        for (i = 0; i < count; i++)
            anotify_fmt(player, " %-15s  %0.3d  %-14s %s", host_as_hex(harr[i]->a), harr[i]->uses, time_format_3(harr[i]->wupd), harr[i]->name);
        anotify_fmt(player, CINFO "Top %d host%s displayed (%d total).", count, count == 1 ? "" : "s", hostdb_count);
    } else {
        anotify_fmt(player, "Bytes used by cache: %d", sizeof(struct hostinfo) * hostdb_count);
        anotify_fmt(player, "Hostnames in cache:  %d", hostdb_count);
        anotify_fmt(player, "Last cache flush:    %s", "...");
    }
}









