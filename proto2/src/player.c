#include "copyright.h"
#include "config.h"

#include "db.h"
#include "params.h"
#include "props.h"
#include "tune.h"
#include "interface.h"
#include "externs.h"

static hash_tab player_list[PLAYER_HASH_SIZE];

dbref
lookup_player(const char *name)
{
    hash_data *hd;

    if ((hd = find_hash(name, player_list, PLAYER_HASH_SIZE)) == NULL)
        return NOTHING;
    else
        return (hd->dbval);
}

dbref
connect_player(const char *name, const char *password)
{
    dbref player;
    
    if (*name == NUMBER_TOKEN && number(name + 1) && atoi(name + 1)) {
        player = (dbref) atoi(name + 1);
        if ((player < 0) || (player >= db_top)
            || (Typeof(player) != TYPE_PLAYER))
            player = NOTHING;
    } else {
        player = lookup_player(name);
    }
    if (player == NOTHING)
        return NOTHING;
    if (!check_password(player,password)) 
        return NOTHING;

    return player;
}

dbref
create_player(const char *name, const char *password)
{
    char buf[80];
    dbref player;

    if (!ok_player_name(name) || !ok_password(password))
        return NOTHING;
    if (!tp_building || tp_db_readonly)
        return NOTHING;

    /* else he doesn't already exist, create him */
    player = new_object();
/*    newp = DBFETCH(player); */

    /* initialize everything */
    NAME(player) = alloc_string(name);
    DBFETCH(player)->location = tp_player_start; /* home */
    FLAGS(player) = TYPE_PLAYER;
    if (valid_obj(tp_player_prototype)
        && (Typeof(tp_player_prototype) == TYPE_PLAYER)) {
        struct object *newp = DBFETCH(player);
        struct plist *daprops = DBFETCH(player)->properties;

        FLAGS(player) = FLAGS(tp_player_prototype);
        FLAG2(player) = FLAG2(tp_player_prototype);

        if (tp_pcreate_copy_props) {
            daprops = (struct plist *) copy_prop(tp_player_prototype);
            newp->properties = daprops;
            newp->exits = NOTHING;
            newp->contents = NOTHING;
            newp->next = NOTHING;
#ifdef DISKBASE
            newp->propsfpos = 0;
            newp->propsmode = PROPS_UNLOADED;
            newp->propstime = 0;
            newp->nextold = NOTHING;
            newp->prevold = NOTHING;
            dirtyprops(player);
#endif
        }
        newp->location = tp_player_start;
        DBDIRTY(player);
    } else {
        struct object *newp = DBFETCH(player);

        newp->location = tp_player_start;
        DBDIRTY(player);
    }
    OWNER(player) = player;
    DBFETCH(player)->sp.player.home = tp_player_start;
    DBFETCH(player)->exits = NOTHING;
    DBFETCH(player)->sp.player.pennies = tp_start_pennies;
    DBFETCH(player)->sp.player.password = alloc_string(password);
    DBFETCH(player)->sp.player.curr_prog = NOTHING;
    DBFETCH(player)->sp.player.insert_mode = 0;

    /* link him to tp_player_start */
    PUSH(player, DBFETCH(tp_player_start)->contents);
    add_player(player);
    DBDIRTY(player);
    DBDIRTY(tp_player_start);
    if (MLevel(player) > LM3)
        SetMLevel(player, LM3);

    sprintf(buf, CNOTE "%s is born!", PNAME(player));
    anotify_except(DBFETCH(tp_player_start)->contents, NOTHING, buf, player);

    return player;
}

void
do_password(dbref player, const char *old, const char *newobj)
{

    if (Guest(player)) {
        anotify_fmt(player, CFAIL "%s", tp_noguest_mesg);
        return;
    }

    if (!check_password(player,old)) {
        anotify_nolisten2(player,
                          CFAIL "Syntax: @password <oldpass>=<newpass>");
    } else if (!ok_password(newobj)) {
        anotify_nolisten2(player, CFAIL "Bad new password.");
    } else {
        free((void *) DBFETCH(player)->sp.player.password);
        DBSTORE(player, sp.player.password, alloc_string(newobj));
        anotify_nolisten2(player, CFAIL "Password changed.");
    }
}

void
clear_players(void)
{
    kill_hash(player_list, PLAYER_HASH_SIZE, 0);
    return;
}

void
add_player(dbref who)
{
    hash_data hd;

    hd.dbval = who;
    if (add_hash(NAME(who), hd, player_list, PLAYER_HASH_SIZE) == NULL)
        panic("Out of memory");
    else
        return;
}


void
delete_player(dbref who)
{
    int result;

    result = free_hash(NAME(who), player_list, PLAYER_HASH_SIZE);

    if (result) {
        int i;

        wall_wizards(MARK
                     "WARNING: Playername hashtable is inconsistent.  Rebuilding it.");
        clear_players();
        for (i = db_top; i-- > 0;) {
            if (Typeof(i) == TYPE_PLAYER) {
                add_player(i);
            }
        }
        result = free_hash(NAME(who), player_list, PLAYER_HASH_SIZE);
        if (result) {
            wall_wizards(MARK
                         "WARNING: Playername hashtable still inconsistent after rebuild.");
        }
    }

    return;
}
