#ifdef NEWHTTPD

extern void prim_descr_safeboot(PRIM_PROTOTYPE);
extern void prim_descr_sendfile(PRIM_PROTOTYPE);
extern void prim_body_getchar(PRIM_PROTOTYPE);
extern void prim_body_nextchar(PRIM_PROTOTYPE);
extern void prim_body_prevchar(PRIM_PROTOTYPE);
extern void prim_base64encode(PRIM_PROTOTYPE);
extern void prim_base64decode(PRIM_PROTOTYPE);

#define PRIMS_HTTP_FUNCS prim_descr_safeboot, prim_descr_sendfile,    \
    prim_body_getchar, prim_body_nextchar, prim_body_prevchar,        \
    prim_base64encode, prim_base64decode

#define PRIMS_HTTP_NAMES "DESCR_SAFEBOOT", "DESCR_SENDFILE",          \
    "BODY_GETCHAR", "BODY_NEXTCHAR", "BODY_PREVCHAR", "BASE64ENCODE", \
    "BASE64DECODE"

#define PRIMS_HTTP_CNT 7

#else /* !NEWHTTPD */
#define PRIMS_HTTP_CNT 0
#endif /* NEWHTTPD */
