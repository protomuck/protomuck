/* This file contains the helper functions for the MySQL support */
/* As well as the actual prims themselves */


#include "copyright.h"
#include "config.h"
#include "params.h"
#ifdef SQL_SUPPORT
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <mysql/mysql.h>
#include <mysql/mysql_version.h>
#include "db.h"
#include "tune.h"
#include "props.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "strings.h"
#include "interp.h"
#include "p_mysql.h"

extern struct inst *oper1, *oper2, *oper3, *oper4, *oper5, *oper6;

/* Prim for connecting to a SQL database */
void
prim_sqlconnect(PRIM_PROTOTYPE)
{
    char password[BUFFER_LEN];
    char hostname[BUFFER_LEN];
    char username[BUFFER_LEN];
    char database[BUFFER_LEN];
    struct inst *newsql;
    unsigned int timeout, notConnected, *timeoutPtr;
    MYSQL *result, *tempsql;

    CHECKOP(5);
    oper5 = POP();              /* timeout INT */
    oper4 = POP();              /* database STR */
    oper3 = POP();              /* password STR */
    oper2 = POP();              /* username STR */
    oper1 = POP();              /* hostname STR */

    if (mlev < LARCH)
        abort_interp("MYSQL connections are ArchWiz level prims.");
    if (oper5->type != PROG_INTEGER)
        abort_interp("Timeout must be an integer.");
    if (oper5->data.number < 1)
        oper5->data.number = 1; /* avoiding 0 or negative timeouts */
    if (oper1->type != PROG_STRING || oper2->type != PROG_STRING ||
        oper3->type != PROG_STRING || oper4->type != PROG_STRING)
        abort_interp("Login arguments must be strings.");
    if (!oper2->data.string)
        abort_interp("Username cannot be an empty string.");
    /* copy data over */
    if (oper1->data.string)     /* null host is assumed to be localhost */
        strcpy(hostname, oper1->data.string->data);
    else
        strcpy(hostname, "");
    strcpy(username, oper2->data.string->data);
    if (oper3->data.string)     /* null passwords are possible. */
        strcpy(password, oper3->data.string->data);
    else
        strcpy(password, "");
    if (oper4->data.string)     /* null databases are possible. */
        strcpy(database, oper4->data.string->data);
    else
        strcpy(database, "");
    timeout = oper5->data.number;
    timeoutPtr = &timeout;      /* mysql_options wants a pointer to an int */

    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    CLEAR(oper4);
    CLEAR(oper5);

    tempsql = malloc(sizeof(MYSQL));
    /* Making this error abort because it's a rare critical error 
     * that the user can't do anything about most of the time. */
    if (mysql_init(tempsql) == NULL) {
        free(tempsql);
        abort_interp("Unable to initialize MySQL connection.");
    }
    /* set the timeout and try to connect */
    mysql_options(tempsql, MYSQL_OPT_CONNECT_TIMEOUT, (char *) timeoutPtr);
    result =
        mysql_real_connect(tempsql, hostname, username, password, database, 0,
                           NULL, 0);

    if (result) {               /* connection successful */
        notConnected = 0;
        newsql = (struct inst *) malloc(sizeof(struct inst));
        newsql->type = PROG_MYSQL;
        newsql->data.mysql = (struct muf_sql *) malloc(sizeof(struct muf_sql));
        newsql->data.mysql->mysql_conn = tempsql;
        newsql->data.mysql->connected = 1;
        newsql->data.mysql->timeout = oper5->data.number;
        newsql->data.mysql->links = 1; /* 1 instance so far */
        copyinst(newsql, &arg[(*top)++]);
        CLEAR(newsql);
        PushInt(notConnected);  /* Pushing a 0 for success, unlike normal */
    } else {                    /* connection failed. Push error int and mesg */
        int errNum;
        char errbuf[BUFFER_LEN];

        strcpy(errbuf, mysql_error(tempsql));
        errNum = mysql_errno(tempsql);
        PushString(errbuf);
        PushInt(errNum);
        free(tempsql);
    }
}

/* Prim for sending queries to a SQL connection */
void
prim_sqlquery(PRIM_PROTOTYPE)
{
    MYSQL *tempsql = NULL;
    MYSQL_RES *res = NULL;
    MYSQL_ROW row = NULL;
    MYSQL_FIELD *fields = NULL;
    int num_rows, num_fields, counter, all_rows;
    char query[BUFFER_LEN];
    unsigned long i = 0;
    stk_array *nw;
    stk_array *fieldsList;
    char errbuf[BUFFER_LEN];

    nargs = 1;

    CHECKOP(2);                 /* SQL string */
    oper1 = POP();              /* the query */
    oper2 = POP();              /* the SQL connectin */

    if (mlev < LARCH)
        abort_interp("SQL prims are ArchWiz level.");
    if (oper1->type != PROG_STRING)
        abort_interp("String argument expected for query. (2)");
    if (!oper1->data.string)
        abort_interp("The query cannot be an empty string. (2)");
    if (oper2->type != PROG_MYSQL)
        abort_interp("MySQL connection expected. (1)");
    if (!oper2->data.mysql->connected)
        abort_interp("This MySQL connection is closed.");

    strcpy(query, oper1->data.string->data);
    CLEAR(oper1);
    tempsql = oper2->data.mysql->mysql_conn;
    if (mysql_query(tempsql, query)) { /* Query failed */
        int errno = -1;

        strcpy(errbuf, mysql_error(tempsql));
        CLEAR(oper2);
        PushString(errbuf);
        PushInt(errno);
        return;                 /* Push error string, and -1, and return */
    }
    res = mysql_store_result(tempsql);
    CLEAR(oper2);
    num_rows = 0;
    if (res) {                  /* there IS a result */
        all_rows = mysql_num_rows(res);
        num_rows = all_rows > tp_mysql_result_limit ?
            tp_mysql_result_limit : all_rows;
        CHECKOFLOW(num_rows + 2);
        num_fields = mysql_num_fields(res);
        fields = mysql_fetch_fields(res);
        counter = 0;
        fieldsList = new_array_packed(num_fields);
        for (i = 0; i < num_fields; ++i)
            array_set_intkey_strval(&fieldsList, i, fields[i].name);
        while ((row = mysql_fetch_row(res))) { //fetch all rows, push limit
            nw = new_array_dictionary();
            if (counter++ < num_rows) {
                for (i = 0; i < num_fields; ++i) {
                    if (row[i])
                        array_set_strkey_strval(&nw, fields[i].name, row[i]);

                }
                PushArrayRaw(nw);
            } else
                break;          /* The limit has been reached, exit the while loop */
        }
        mysql_free_result(res);
    } else {                    /* no result */
        num_rows = 0;
        fieldsList = new_array_packed(0);
    }
    PushInt(num_rows);
    PushArrayRaw(fieldsList);
    return;
}

/* Simply  close the connection. This is also done automatically 
 * if the MYSQL gets cleared without already having been closed.
 */
void
prim_sqlclose(PRIM_PROTOTYPE)
{
    CHECKOP(1);
    /* MYSQL */
    oper1 = POP();

    if (mlev < LARCH)
        abort_interp("MySQL prims are ArchWiz.");
    if (oper1->type != PROG_MYSQL)
        abort_interp("MySQL connection expected.");
    if (oper1->data.mysql->connected) {
        mysql_close(oper1->data.mysql->mysql_conn);
        oper1->data.mysql->connected = 0;
    }
    CLEAR(oper1);
}

void
prim_sqlping(PRIM_PROTOTYPE)
{
    int result = 0;

    CHECKOP(1);
    /* MYSQL */
    oper1 = POP();

    if (mlev < LARCH)
        abort_interp("MySQL prims are ArchWiz level.");
    if (oper1->type != PROG_MYSQL)
        abort_interp("MySQL connection expected.");

    if (oper1->data.mysql->connected) {
        result = mysql_ping(oper1->data.mysql->mysql_conn);
        if (!result)
            result = 1;
        else
            oper1->data.mysql->connected = 0;
    }
    CLEAR(oper1);
    PushInt(result);
}

#endif
