extern void prim_socksend(PRIM_PROTOTYPE);
extern void prim_nbsockrecv(PRIM_PROTOTYPE);
extern void prim_sockclose(PRIM_PROTOTYPE);
extern void prim_nbsockopen(PRIM_PROTOTYPE);
extern void prim_sockcheck(PRIM_PROTOTYPE);
extern void prim_sockdescr(PRIM_PROTOTYPE);
extern void prim_lsockopen(PRIM_PROTOTYPE);
extern void prim_sockaccept(PRIM_PROTOTYPE);
extern void prim_nbsockrecv_char(PRIM_PROTOTYPE);
extern void prim_get_sockinfo(PRIM_PROTOTYPE);

#define PRIMS_SOCKET_FUNCS prim_socksend, prim_nbsockrecv, prim_sockclose, \
    prim_nbsockopen, prim_sockcheck, prim_sockdescr, prim_lsockopen,       \
    prim_sockaccept, prim_nbsockrecv_char, prim_get_sockinfo

#define PRIMS_SOCKET_NAMES "SOCKSEND", "NBSOCKRECV", "SOCKCLOSE",      \
    "NBSOCKOPEN", "SOCKCHECK", "SOCKDESCR", "LSOCKOPEN", "SOCKACCEPT", \
    "NBSOCKRECV_CHAR", "GET_SOCKINFO"

#ifdef MUF_SOCKETS
#define PRIMS_SOCKET_CNT 10
#else
#define PRIMS_SOCKET_CNT 0
#endif
