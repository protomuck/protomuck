#include "copyright.h"
#include "config.h"
#include "db.h"
#include "interp.h"
#include "inst.h"
#include "version.h"

/* Basically, this thing takes the list of all the MUF primitives in Proto */
/*  and checks the man.txt file to see if they have docs.  It's mostly     */
/*  quickhackish and not coded all that well.   -Hinoserm                  */

/* these arrays MUST agree with what's in inst.h */
const char *base_inst[] = {
    "JMP", "READ", "TREAD", "SLEEP", "CALL", "EXECUTE", "EXIT", "EVENT_WAITFOR",
    "CATCH", "CATCH_DETAILED",
    PRIMS_CONNECTS_NAMES,
    PRIMS_DB_NAMES,
    PRIMS_MATH_NAMES,
    PRIMS_MISC_NAMES,
    PRIMS_PROPS_NAMES,
    PRIMS_STACK_NAMES,
    PRIMS_STRINGS_NAMES,
    PRIMS_FLOAT_NAMES,
    PRIMS_ERROR_NAMES,
#ifdef PCRE_SUPPORT
    PRIMS_REGEX_NAMES,
#endif
#ifdef FILE_PRIMS
    PRIMS_FILE_NAMES,
#endif
    PRIMS_ARRAY_NAMES,
#ifdef MCP_SUPPORT
    PRIMS_MCP_NAMES,
#endif
#ifdef MUF_SOCKETS
    PRIMS_SOCKET_NAMES,
#endif
    PRIMS_SYSTEM_NAMES,
#ifdef SQL_SUPPORT
    PRIMS_MYSQL_NAMES,
#endif
#ifdef MUF_EDIT_PRIMS
    PRIMS_MUFEDIT_NAMES,
#endif
#ifdef NEWHTTPD
    PRIMS_HTTP_NAMES,           /* hinoserm */
#endif
    PRIMS_INTERNAL_NAMES
};

int
check_file(FILE *file, const char *onwhat)
{
    char buf[BUFFER_LEN];
    char topic[BUFFER_LEN];
    char *p;
    register int arglen, found;

    fseek(file, 0, SEEK_SET); /* start at the beginning */

    *topic = '\0';
    strcpy(topic, onwhat);
    if (*onwhat)
        strcat(topic, "|");

    arglen = strlen(topic);
    if (*topic && (arglen > 1)) {
        do {
            do {
                if (!(fgets(buf, sizeof buf, file)))
                    return 0;
            } while (*buf != '~');
            do {
                if (!(fgets(buf, sizeof buf, file)))
                    return 0;
            } while (*buf == '~');
            p = buf;
            found = 0;
            buf[strlen(buf) - 1] = '|';
            while (*p && !found) {
                if (strncasecmp(p, topic, arglen)) {
                    while (*p && (*p != '|'))
                        p++;
                    if (*p)
                        p++;
                } else
                    found = 1;
            }
        } while (!found);
    }

    return 1;
}

int
main(int argc, char *argv[])
{
    FILE *f;
    register int i, count = 0;

    if ((f = fopen(MAN_FILE, "r")) == NULL) {
        fprintf(stderr, "No file %s!\n", MAN_FILE);
        return 1;
    } else {
        printf(".--------------------------------------------------------------.\n");
        printf("| ProtoMUCK MUF primitive doc checker  -   Written by Hinoserm |\n");
        printf("|--------------------------------------------------------------|\n");
        printf("| Primitives list is from ProtoMUCK v%-25s |\n", PROTOBASE);
        printf("|--------------------------------------------------------------|\n");

        for (i = 0; i < BASE_MAX; i++)
            if (base_inst[i][0] != ' ')
            if (!check_file(f, base_inst[i]))
                printf("| %3d: %-55s |\n", ++count, base_inst[i]);

        if (!count)
            printf("| No undocumented primitives! Yay!                             |\n");

        printf("`--------------------------------------------------------------'\n");
        fclose(f);
    }

	return 0;
}
