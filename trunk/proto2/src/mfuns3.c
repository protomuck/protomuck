#include "config.h"

#include <math.h>
#include <ctype.h>
#include "params.h"

#include "db.h"
#include "tune.h"
#include "mpi.h"
#include "externs.h"
#include "props.h"
#include "match.h"
#include "interp.h"
#include "interface.h"
#include "msgparse.h"
#include "cgi.h"

/* This file contains MPI macros to handle Pueblo/Netscape differences. */

extern struct descriptor_data *descriptor_list;

char *
commandtext(dbref player, char *command, char *text)
{
    static char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];

    strcpy(buf2, "");

    strcpy(buf, text);
    if (FLAG2(player) & F2PUEBLO) {
        sprintf(buf, "<a xch_cmd=\"%s\">%s</a>", command, text);
    }

    return (buf);
}

char *
playmidi(dbref player, char *musicurl, char *volume)
{
    static char buf[BUFFER_LEN];

    strcpy(buf, "");

    if (FLAG2(player) & F2PUEBLO) {
        sprintf(buf, "<img xch_sound=play href=\"%s\" xch_volume=%s>", musicurl,
                volume);
    }

    return (buf);
}

char *
stopmidi(dbref player)
{
    static char buf[BUFFER_LEN];

    if (FLAG2(player) & F2PUEBLO) {
        sprintf(buf, "<img xch_sound=stop device=midi>");
    }
    return (buf);
}

/* Begin MPI stuff here. */

const char *
mfn_command(MFUNARGS)
{
    return (commandtext(player, argv[0], argv[1]));
}

const char *
mfn_playmidi(MFUNARGS)
{
    if (argc == 1)
        return (playmidi(player, argv[0], "60"));
    else
        return (playmidi(player, argv[0], argv[1]));
}

const char *
mfn_stopmidi(MFUNARGS)
{
    return (stopmidi(player));
}
