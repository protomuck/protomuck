extern void prim_socksend(PRIM_PROTOTYPE);
extern void prim_nbsockrecv(PRIM_PROTOTYPE);
extern void prim_sockclose(PRIM_PROTOTYPE);
extern void prim_nbsockopen(PRIM_PROTOTYPE);
extern void prim_socksecure(PRIM_PROTOTYPE);
extern void prim_sockunsecure(PRIM_PROTOTYPE);
extern void prim_sslerror(PRIM_PROTOTYPE);
extern void prim_sockcheck(PRIM_PROTOTYPE);
extern void prim_sockdescr(PRIM_PROTOTYPE);
extern void prim_lsockopen(PRIM_PROTOTYPE);
extern void prim_sockaccept(PRIM_PROTOTYPE);
extern void prim_ssl_sockaccept(PRIM_PROTOTYPE);
extern void prim_nbsockrecv_char(PRIM_PROTOTYPE);
extern void prim_get_sockinfo(PRIM_PROTOTYPE);
extern void prim_socket_setuser(PRIM_PROTOTYPE);
extern void prim_set_sockopt(PRIM_PROTOTYPE);
extern void prim_socktodescr(PRIM_PROTOTYPE);
extern void prim_sockshutdown(PRIM_PROTOTYPE);
extern void prim_udpsend(PRIM_PROTOTYPE);
extern void prim_udpopen(PRIM_PROTOTYPE);
extern void prim_udpclose(PRIM_PROTOTYPE);
extern void prim_dns(PRIM_PROTOTYPE);


#define PRIMS_SOCKET_FUNCS prim_socksend, prim_nbsockrecv, prim_sockclose, \
    prim_nbsockopen, prim_sockcheck, prim_sockdescr, prim_lsockopen,       \
    prim_sockaccept, prim_nbsockrecv_char, prim_get_sockinfo,              \
    prim_socket_setuser, prim_set_sockopt, prim_socktodescr,               \
    prim_sockshutdown, prim_ssl_sockaccept, prim_socksecure,               \
    prim_sockunsecure, prim_dns, prim_udpsend, prim_udpopen, prim_udpclose \


#define PRIMS_SOCKET_NAMES "SOCKSEND", "NBSOCKRECV", "SOCKCLOSE",       \
    "NBSOCKOPEN", "SOCKCHECK", "SOCKDESCR", "LSOCKOPEN", "SOCKACCEPT",  \
    "NBSOCKRECV_CHAR", "GET_SOCKINFO", "SOCKET_SETUSER", "SET_SOCKOPT", \
    "SOCKTODESCR", "SOCKSHUTDOWN", "SSL_SOCKACCEPT", "SOCKSECURE",      \
    "SOCKUNSECURE", "DNS", "UDPSEND", "UDPOPEN", "UDPCLOSE"

#ifdef MUF_SOCKETS
# ifdef UDP_SOCKETS
# define PRIMS_SOCKET_CNT 21
# else
# define PRIMS_SOCKET_CNT 18
#endif
#else
#define PRIMS_SOCKET_CNT 0
#endif
