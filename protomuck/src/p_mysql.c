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

MYSQL *mysql_conn   = NULL;
MYSQL_RES *res      = NULL;
MYSQL_ROW row       = NULL;
MYSQL_FIELD *fields = NULL;

extern struct inst *oper1, *oper2, *oper3, *oper4;

void prim_sqlquery(PRIM_PROTOTYPE)
{
    char buf[BUFFER_LEN];
    struct inst temp1, temp2;
    int num_rows, num_fields, counter, all_rows;
    char query[BUFFER_LEN];
    unsigned long *lengths = NULL;
    unsigned long i = 0;
    stk_array *nw;
    stk_array *fieldsList;
    int fieldListMade = 0;
    char errbuf[BUFFER_LEN];
    nargs = 1;
    CHECKOP(1);
    oper1 = POP();

    if (oper1->type != PROG_STRING)
        abort_interp("Non-string argument. (1)");
    if (!oper1->data.string)
        abort_interp("Empty strings not accepted. (1)");
    if (!mysql_conn)
       abort_interp("MySQL connection is closed.");

    strcpy(query, oper1->data.string->data);
    if (mysql_query(mysql_conn, query)) {
       strcpy(errbuf, mysql_error(mysql_conn));
       abort_interp(errbuf);
       // abort_interp("MySQL: Query failed.");
    }
    CLEAR(oper1);
    res = mysql_store_result(mysql_conn);
    num_rows = 0;
    if (res) { /* there IS a result */
        all_rows = mysql_num_rows(res);
        num_rows = all_rows > tp_mysql_result_limit ? 
                   tp_mysql_result_limit : all_rows;
        CHECKOFLOW(num_rows + 2);
        num_fields = mysql_num_fields(res);
        fields = mysql_fetch_fields(res);
        counter = 0;
        fieldsList = new_array_packed(0);
        while ((row = mysql_fetch_row(res))) { //fetch all rows, push limit
            nw = new_array_dictionary();
            if (counter++ < num_rows) {
                for (i = 0; i < num_fields; ++i ) {
                    if (row[i]) {
                        temp1.type = PROG_STRING;
                        temp1.data.string = alloc_prog_string(fields[i].name);
                        temp2.type = PROG_STRING;
                        temp2.data.string = alloc_prog_string(row[i]);
                        array_setitem(&nw, &temp1, &temp2);
                        if (!fieldListMade)
                            array_appenditem(&fieldsList, &temp1);
                        CLEAR(&temp1);
                        CLEAR(&temp2);
                    }
                }
                fieldListMade = 1;
                PushArrayRaw(nw);
            }
        }
        mysql_free_result(res);
    }
    PushArrayRaw(fieldsList);
    PushInt(num_rows);
    return;
}

/* All of the command line functions for SQL support.
 * TODO: If SQL becomes it's own MUF data type, this function
 * should be removed, along with the @tune SQL login info.
 */
void do_sql(int descr, dbref player, const char *arg)
{
    if (!Arch(player)) {
        anotify_nolisten2(player, CFAIL "Permission denied.");
        return;
    }
    
    if (*arg == '\0') {
        if (mysql_conn) 
            anotify_nolisten2(player, CSUCC "MySQL: Connecton is open.");
        else
            anotify_nolisten2(player, CINFO "MySQL: Connection is closed.");
        return;
    }

    if (!string_compare(arg, "on")) {
        if (mysql_conn) {
            anotify_nolisten2(player, CINFO "MySQL: Resetting connection.");
            sql_off();
        }
        if (!sql_on()) 
            anotify_nolisten2(player, CSUCC "MySQL: Connection opened.");
        else
            anotify_nolisten2(player, CFAIL "MySQL: Connection failure."); 
        return;
    }

    if (!string_compare(arg, "off")) {
        if (!mysql_conn) 
            anotify_nolisten2(player, 
                                CINFO "MySQL: Connection already closed.");
        else {
            sql_off();
            anotify_nolisten2(player, CSUCC "MySQL: Connection closed.");
        }
        return;
    }
    anotify(player, CINFO "Arguments are 'on', 'off', or nothing.");
} 

void do_sqlquery(int descr, dbref player, const char *arg) {
    // Absolutely NO problems with this code. Stress tested with 80+
    // queries in one shot.
    unsigned int num_rows, num_fields, counter;

    if (!Arch(player)) {
        anotify_nolisten2(player, CFAIL "Permission denied.");
        return;
    }
    if (!mysql_conn) {
        anotify_nolisten2( player, CFAIL "MySql: Connection is closed." );
        return;
    }
    if(mysql_query(mysql_conn,arg)) {
        anotify_nolisten2( player, CFAIL "MySql: Query Failed.");

        //Don't know if theres any clean up required so I'm closing
        // the connection on a failed query. --SOMETHING TO LOOK AT--.
        anotify_nolisten2( player, CFAIL "MySql: Closing Connection.");
        sql_off();
        return;
    } else { // query worked
        res=mysql_store_result(mysql_conn);
        if (!res) {
            anotify_nolisten2( player, CSUCC "MySql: No results returned.");
            return;
        } else {
            anotify_nolisten2( player, CSUCC "MySql: Results returned.");
            num_rows = mysql_num_rows(res);
            num_fields = mysql_num_fields(res);
            anotify_fmt(player, CSUCC "MySql: records: %ld  fields: %ld\n", num_rows, num_fields);
            fields = mysql_fetch_fields(res);
            if (num_rows > tp_mysql_result_limit) {
                anotify_fmt(player,    CFAIL "MySql: return exceeded tunable paramater MYSQL_RESULT_LIMIT, showing %ld records\n",    tp_mysql_result_limit);
            };

            counter = 0;
            while (row = mysql_fetch_row(res)) {
                if (counter++ < tp_mysql_result_limit) {
                    unsigned char *tmpstr;
                    unsigned long *lengths;
                    unsigned long i = 0;
                    strcpy (tmpstr, "");
                    for (i = 0; i < num_fields; i++) {
                        strcat (tmpstr, row[i] ? row[i] : "NULL");
                        strcat (tmpstr, " ");
                    }
                    anotify_fmt(player, CSUCC "%s", tmpstr);
                }
            }
        }
        mysql_free_result(res);
    }
    return;
}

/* Attempts to establish the connection. 
 * Uses the @tune parameters for the connection data.
 * Returns 0 if was successful and points mysql_conn to the
 * connection data. Otherwise returns 1 if there is an error.
 */
int sql_on()
{
    MYSQL *mysql, *result;
    char passbuf[BUFFER_LEN];
    mysql = malloc(sizeof(MYSQL));
    if (!strcmp(tp_mysql_password, "NULL"))
        strcpy(passbuf, "");
    else
        strcpy(passbuf, tp_mysql_password);
    if (mysql_init(mysql) == NULL)
        return 1;
    result = mysql_real_connect (
        mysql, tp_mysql_hostname, tp_mysql_username, passbuf, 
        tp_mysql_database, 0, NULL, 0 );
    
    if (!result) {
notify(3166, mysql_error(mysql));
        free(mysql);
        return 1;
    }
    mysql_conn = mysql;
    
    return 0;
}

/* Shuts down the SQL connection and NULLs the pointer. Always returns 0 */
int sql_off() 
{
    if (mysql_conn) {
        mysql_close(mysql_conn);
        free(mysql_conn);
        mysql_conn = NULL;
    }
    return 0;
}

#endif
