#ifndef __NEWRESOLV_H
#define __NEWRESOLV_H
#include <arpa/inet.h>

struct hostinfo { /* linked-list struct for the new host cacher */
    time_t           wupd;      /* time of last update */
    int              a;         /* ip */
#ifdef IPV6
    struct in6_addr  a6;        /* Version 6 IP */
#endif
    char             *name;     /* hostname */
    unsigned short   links;     /* number of links to this hostname */
    unsigned short   uses;      /* times used */
    struct hostinfo *next;
    struct hostinfo *prev;
};

struct husrinfo { /* linked-list struct for usernames (userports) */
    int              a;         /* ip address */
#ifdef IPV6
    struct in6_addr  a6;        /* Version 6 IP */
#endif
    unsigned short   uport;     /* the userport number */
    char            *user;      /* username */
    struct husrinfo *next;
    struct husrinfo *prev;
};

struct huinfo { /* host/user information */
    struct hostinfo *h; /* hostname */
    struct husrinfo *u; /* username */
};

extern struct hostinfo *hostdb;
extern struct husrinfo *userdb;
extern unsigned long hostdb_count;

#ifdef USE_RESLVD
extern int reslvd_sock;
extern bool reslvd_connected;
extern int reslvd_open(void);
extern void reslvd_input(void);
extern void reslvd_disc(void);
extern void reslvd_close(void);
#endif

#ifdef SPAWN_HOST_RESOLVER
extern void kill_resolver(void);
extern void spawn_resolver(void);
extern void resolve_hostnames(void);
extern int resolverpid;
extern int resolver_sock[2];
#endif

#ifdef IPV6
extern struct huinfo *host_getinfo6(struct in6_addr a6, unsigned short lport, unsigned short prt);
extern char *host_get_ipv6attr(int a, unsigned short lport, unsigned short prt);
#endif

extern char *ip_address_prototype(void* x, int xsize);
extern char *hostToIPex(struct hostinfo * h);
#define ip_address(x) ip_address_prototype(&x, sizeof(x))

extern struct huinfo *host_getinfo(int a, unsigned short lport, unsigned short prt);
extern void host_delete(struct huinfo *hu);
extern void host_init(void);
extern void host_load(void);
extern void host_save(void);
extern void host_shutdown(void);
extern void host_free(struct hostinfo *h);
extern void host_check_cache(void);
extern void do_hostcache(dbref player, const char *args);
extern unsigned long host_hostdb_size(void);
extern unsigned long host_userdb_size(void);
#endif /* __NEWRESOLV_H */
