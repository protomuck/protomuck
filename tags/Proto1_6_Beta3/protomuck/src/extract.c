#include "copyright.h"
#include "config.h"

#include "db.h"
#include "params.h"
#include "tune.h"
#include "interface.h"
#include "match.h"
#include "props.h"

dbref   me;

void 
check_properties(const char *dir, dbref obj)
{
    int     val;
    char   *buf, *prop;
    PropPtr pptr;
    PropPtr pref;
    char    name[BUFFER_LEN];

    pref = first_prop(obj, dir, &pptr, name);
    while (pref > 0) {
	buf = (char *) malloc(strlen(dir) + strlen(name) + 2);
	(void) strcat(strcpy(buf, dir), name);
	if (prop = (char *) get_property_class(obj, buf))
	    printf("%s%c%s\n", buf + 1, PROP_DELIMITER, uncompress(prop));
	else if (val = get_property_value(obj, buf))
	    printf("%s%c^%d\n", buf + 1, PROP_DELIMITER, val);
	if (is_propdir(obj, buf))
	    check_properties((char *) strcat(buf, "/"), obj);
	free(buf);
	pref = next_prop(pptr, pref, name);
    }
}

void 
check_common(dbref obj)
{
    printf("\nObject %s\n", unparse_object(me, obj));
    printf("Name: %s\n", uncompress(DBFETCH(obj)->name));
    if (GETDESC(obj))
	printf("Desc: %s\n", uncompress(GETDESC(obj)));
    printf("Loc: #%s\n", unparse_object(me, DBFETCH(obj)->location));
    printf("Owner: #%d\n", unparse_object(me, DBFETCH(obj)->owner));
    printf("First contents: #%d\n", unparse_object(me, DBFETCH(obj)->contents));
    printf("Next item: #%d\n", unparse_object(me, DBFETCH(obj)->next));
    printf("Key: %s\n", unparse_boolexp(me, GETLOCK(obj)), 1);
    if (GETFAIL(obj))
	printf("Fail: %s\n", uncompress(GETFAIL(obj)));
    if (GETSUCC(obj))
	printf("Succ: %s\n", uncompress(GETSUCC(obj)));
    if (GETDROP(obj))
	printf("Drop: %s\n", uncompress(GETDROP(obj)));
    if (GETOFAIL(obj))
	printf("Ofail: %s\n", uncompress(GETOFAIL(obj)));
    if (GETOSUCC(obj))
	printf("Osucc: %s\n", uncompress(GETOSUCC(obj)));
    if (GETODROP(obj))
	printf("Odrop: %s\n", uncompress(GETODROP(obj)));
    printf("Properties:\n");
    check_properties("/", obj);
    printf("End of properties.\n");
}

void 
check_room(dbref obj)
{
    printf("Dropto: %s\n", unparse_object(me, DBFETCH(obj)->sp.room.dropto));
    printf("First exit: %s\n", unparse_object(me, DBFETCH(obj)->exits));
}

void 
check_thing(dbref obj)
{
    printf("Home: %s\n", unparse_object(me, DBFETCH(obj)->sp.thing.home));
    printf("First action: %s\n", unparse_object(me, DBFETCH(obj)->exits));
    printf("Value: %d\n", DBFETCH(obj)->sp.thing.value);
}

void 
check_exit(dbref obj)
{
    int     i, j;

    printf("Dest#: %d\n", j = DBFETCH(obj)->sp.exit.ndest);
    for (i = 0; i < j; i++) {
	printf("Link #%d: %s\n", i + 1, unparse_object(me, DBFETCH(obj)->sp.exit.dest[i]));
    }
}

void 
check_player(dbref obj)
{
    printf("Home: %s\n", unparse_object(me, DBFETCH(obj)->sp.player.home));
    printf("First action: %s\n", unparse_object(me, DBFETCH(obj)->exits));
    printf("%s: %d\n", tp_cpennies, DBFETCH(obj)->sp.player.pennies);
    printf("Password: %s\n", DBFETCH(obj)->sp.player.password);
}

void 
check_program(dbref obj)
{
    char    buf[BUFFER_LEN];
    FILE   *f;
/*
    if (IsMUP(obj))
     sprintf(buf, "mup/%d.p", (int) obj); else
*/
     sprintf(buf, "muf/%d.m", (int) obj);
    f = fopen(buf, "r");
    if (!f) {
	printf("No program source file found.\n");
	return;
    }
    printf("***Program source file:\n");
    while (fgets(buf, BUFFER_LEN, f)) {
	printf("%s", buf);	/* newlines automatically included */
    }
    fclose(f);
    printf("***End of program source.\n");
}

/*
 * extract must be called in the form "extract <filename> <ID>"
 */

FILE   *input_file;

void 
main(int argc, char **argv)
{
    int     i;

    if (argc != 3) {
	fprintf(stderr, "Must be called in form 'extract <filename> <dbref>'\n");
	exit(1);
    }
    if ((input_file = fopen(argv[1], "r")) == NULL) {
	fprintf(stderr, "Could not open file '%d'\n", argv[1]);
	exit(1);
    }
    me = atoi(argv[2]);

    db_free();
    db_read(input_file);

    fprintf(stderr, "dbtop = %d\n", db_top);

    printf("MUCK 2.2fb extract for dbref #%d\n", me);

    for (i = 0; i < db_top; i++) {
	if (OWNER(i) != me)
	    continue;

	if (!(i % 256))
            fprintf(stderr, "Checking object %d..\n", i);
	check_common(i);
	switch (db[i].flags & TYPE_MASK) {
	    case TYPE_ROOM:
		check_room(i);
		break;
	    case TYPE_THING:
		check_thing(i);
		break;
	    case TYPE_EXIT:
		check_exit(i);
		break;
	    case TYPE_PLAYER:
		check_player(i);
		break;
	    case TYPE_PROGRAM:
		check_program(i);
		break;
	    case TYPE_GARBAGE:
		break;
	    default:
		break;
	}
    }
    fclose(input_file);
    fprintf(stderr, "Completed extract normally.\n");
}

/* dummy compiler */
void
do_compile(dbref p, dbref pr)
{
}
struct macrotable
       *
new_macro(const char *name, const char *definition, dbref player)
{
    return (NULL);
}

void 
exec_or_notify(dbref player, dbref thing, const char *message)
{
}

int 
notify(dbref player, const char *msg)
{
    return 0;
}

void 
notify_except(dbref first, dbref exception, const char *msg)
{
}

dbref 
lookup_player(const char *name)
{
    return 0;
}

void 
clear_players(void)
{
}

void 
clear_primitives(void)
{
}

void 
add_event(int dtime, dbref program, char *strdata)
{
}

void 
init_primitives(void)
{
}

void 
add_player(dbref who)
{
}

int 
interp(dbref player, dbref program, dbref source)
{
    return 0;
}
