extern void prim_sqlquery(PRIM_PROTOTYPE);
extern void prim_sqlconnect(PRIM_PROTOTYPE);
extern void prim_sqlclose(PRIM_PROTOTYPE);
extern void prim_sqlping(PRIM_PROTOTYPE);

#define PRIMS_MYSQL_FUNCS prim_sqlquery, prim_sqlconnect, prim_sqlclose, \
    prim_sqlping

#define PRIMS_MYSQL_NAMES "SQLQUERY", "SQLCONNECT", "SQLCLOSE", "SQLPING"

#ifdef SQL_SUPPORT
#define PRIMS_MYSQL_CNT 4
#else
#define PRIMS_MYSQL_CNT 0
#endif
