extern void prim_sqlquery(PRIM_PROTOTYPE);

#define PRIMS_MYSQL_FUNCS prim_sqlquery

#define PRIMS_MYSQL_NAMES "SQLQUERY"

#ifdef SQL_SUPPORT
#define PRIMS_MYSQL_CNT 1 
#else
#define PRIMS_MYSQL_CNT 0
#endif
