#ifdef NEWHTTPD

extern void prim_descr_safeboot(PRIM_PROTOTYPE);
extern void prim_body_getchar(PRIM_PROTOTYPE);
extern void prim_body_nextchar(PRIM_PROTOTYPE);
extern void prim_body_prevchar(PRIM_PROTOTYPE);
extern void prim_httpdata(PRIM_PROTOTYPE);

#define PRIMS_HTTP_CNT 5

#define PRIMLIST_HTTP     { "DESCR_SAFEBOOT", LARCH, 1, prim_descr_safeboot }, \
                          { "BODY_GETCHAR",   LM2,   2, prim_body_getchar   }, \
                          { "BODY_NEXTCHAR",  LM2,   1, prim_body_nextchar  }, \
                          { "BODY_PREVCHAR",  LM2,   1, prim_body_prevchar  }, \
                          { "HTTPDATA",       LMAGE, 1, prim_httpdata       }


#else /* !NEWHTTPD */
#define PRIMS_HTTP_CNT 0
#endif /* NEWHTTPD */
