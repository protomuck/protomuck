/*
 * Copyright (c) 1990 Chelsea Dyerman
 * University of California, Berkeley (XCF)
 *
 * Some parts of this code -- in particular the dictionary based compression
 * code is Copyright 1995 by Dragon's Eye Productions
 *
 */

/*
 * This file is generated by mkversion.sh. Any changes made will go away.
 */

#include "patchlevel.h"
#include "params.h"
#include "interface.h"
#include "externs.h"

const char *generation = "59";
const char *creation = "Mon Feb 19 2001 at 02:35:30 EST";
const char *version = PATCHLEVEL;

const char *infotext[] =
{
    " ",
    RED "ProtoMUCK " PROTOBASE " " WHITE "-- " CRIMSON VERSION,
    " ",
    CYAN "Based on the original code written by these programmers:",
    "  " AQUA "David Applegate    James Aspnes    Timothy Freeman    Bennet Yee",
    " ",
    CYAN "Others who have done major coding work along the way:",
    "  " AQUA "Lachesis, ChupChups, FireFoot, Russ 'Random' Smith, Dr. Cat,",
    "  " AQUA "Revar, Points, Loki, PakRat, Nodaitsu",
    " ",
    BROWN "ProtoMuck is derived from TinyMUCK, which was itself an extension",
    BROWN "of TinyMUD.  Proto is an updated version of the NeonMUCK code base,",
    BROWN "which originaly came from the FuzzBall distribution, by Foxen/Revar,",
    BROWN "of TinyMUCK.",
    " ",
    GREEN "And, here is the list of the programmers for ProtoMUCK:",
    FOREST "  Moose and Akari: protomuck@bigfoot.com",
    " ",
    BROWN "Thanks are also due towards the multiple people who had also",
    BROWN "contributed ideas for the MUCK as we worked on it, and even tried to",
    BROWN "help us out along the way.",
    CYAN "Feel free to check ProtoMUCK's webpage at the following address:",
    AQUA "http://protomuck.sourceforge.net/",
    " ",
    BLUE "The Pueblo multimedia protocol is (C)Chaco Communications.",
    BLUE "http://www.chaco.com/pueblo for more information.",
    BLUE "The recommended client by us for any mu* server is BeipMU*.",
    BLUE "http://beipmu.twu.net/ for more information.",
    CINFO "Done.",
    0,
};


void
do_credits(dbref player)
{
    int i;

    for (i = 0; infotext[i]; i++) {
        anotify_nolisten2(player, infotext[i]);
    }
}
