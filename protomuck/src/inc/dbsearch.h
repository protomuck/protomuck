/* Took the flgchkdat out of look.c and inserted into this header
 * file for the purpose of working with the findnext prop.
 */

struct flgchkdat {
    int     fortype;            /* check FOR a type? */
    int     istype;             /* If check FOR a type, which one? */
    int     isnotroom;          /* not a room. */
    int     isnotexit;          /* not an exit. */
    int     isnotthing;         /* not type thing */
    int     isnotplayer;        /* not a player */
    int     isnotprog;          /* not a program */
    int     forlevel;           /* check for a mucker level? */
    int     islevel;            /* if check FOR a mucker level, which level? */
    int     isnotzero;          /* not ML0 */
    int     isnotone;           /* not ML1 */
    int     isnottwo;           /* not ML2 */
    int     isnotthree;         /* not ML3 */
    int     isnotfour;          /* not ML4 */
    int     isnotfive;          /* not ML5 */
    int     isnotsix;           /* not ML6 */
    int     isnotseven;         /* not ML7 */
    int     isnoteight;         /* not ML8 */
    int     isnotnine;          /* not ML9 */
    int     isnotwiz;           /* not ML5+ */
    int     iswiz;              /* is  ML5+ */
    int     setflags;           /* flags that are set to check for */
    int     clearflags;         /* flags to check are cleared. */
    int     setflag2;           /* flags that are set to check for */
    int     clearflag2;         /* flags to check are cleared. */
    int     setpowers;          /* powers that are set to check for */
    int     clearpowers;        /* powers to check are cleared. */
    int     anypower;           /* check for any power? */
    int     forlink;            /* check linking? */
    int     islinked;           /* if yes, check if not unlinked */
    int     forold;             /* check for old object? */
    int     isold;              /* if yes, check if old */
    int     loadedsize;         /* check for propval-loaded size? */
    int     issize;             /* list objs larger than size? */
    int     size;               /* what size to check against. No check if 0 */
};

