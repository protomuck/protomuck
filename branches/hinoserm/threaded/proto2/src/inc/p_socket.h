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

extern void prim_nbsock6open(PRIM_PROTOTYPE);
extern void prim_lsock6open(PRIM_PROTOTYPE);
extern void prim_udp6send(PRIM_PROTOTYPE);

#define PRIMLIST_SOCKET	{ "SOCKSEND",			LARCH, 2, prim_socksend },			\
						{ "NBSOCKRECV",			LARCH, 1, prim_nbsockrecv },		\
						{ "SOCKCLOSE",			LARCH, 1, prim_sockclose },			\
						{ "NBSOCKOPEN",			LARCH, 2, prim_nbsockopen },		\
						{ "SOCKCHECK",			LARCH, 1, prim_sockcheck },			\
						{ "SOCKDESCR",			LARCH, 1, prim_sockdescr },			\
						{ "LSOCKOPEN",			LBOY, 2, prim_lsockopen },			\
						{ "SOCKACCEPT",			LBOY, 1, prim_sockaccept },			\
						{ "NBSOCKRECV_CHAR",	LARCH, 1, prim_nbsockrecv_char },	\
						{ "GET_SOCKINFO",		LARCH, 1, prim_get_sockinfo },		\
						{ "SOCKET_SETUSER",		LARCH, 3, prim_socket_setuser },	\
						{ "SET_SOCKOPT",		LARCH, 2, prim_set_sockopt },		\
						{ "SOCKTODESCR",		LARCH, 1, prim_socktodescr },		\
						{ "SOCKSHUTDOWN",		LARCH, 2, prim_sockshutdown },		\
						{ "SSL_SOCKACCEPT",		LBOY, 1, prim_ssl_sockaccept },		\
						{ "SOCKSECURE",			LBOY, 1, prim_socksecure },			\
						{ "SOCKUNSECURE",		LBOY, 1, prim_sockunsecure },		\
						{ "DNS",				LARCH, 1, prim_dns },				\
						{ "UDPSEND",			LARCH, 3, prim_udpsend },			\
						{ "UDPOPEN",			LARCH, 1, prim_udpopen },			\
						{ "UDPCLOSE",			LARCH, 1, prim_udpclose },			\
						{ "NBSOCK6OPEN",		LARCH, 2, prim_nbsock6open },		\
						{ "LSOCK6OPEN",			LBOY, 2, prim_lsock6open },			\
						{ "UDP6SEND",			LARCH, 3, prim_udp6send }

#ifdef MUF_SOCKETS
#define PRIMS_SOCKET_CNT 24
#else
#define PRIMS_SOCKET_CNT 0
#endif
