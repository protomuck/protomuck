#include "copyright.h"
#include "config.h"

#include "db.h"
#include "params.h"
#include "props.h"
#include "tune.h"
#include "interface.h"
#include "externs.h"

static hash_tab player_list[PLAYER_HASH_SIZE];

bool
check_password(dbref player, const char *check_pw)
{
    if (player == NOTHING)
        return 0;

    const char *password = DBFETCH(player)->sp.player.password;

    /* We now do this smartly based on the DB_NEWPASSES */
    /*  database flag. -Hinoserm                        */

    if (!password || !*password)
        return 1;

    if (!db_hash_passwords)
        return !strcmp(check_pw, password);

    if (db_hash_compare(password, check_pw)) {
        switch (db_hash_tagtoval(password)) {
            case HTYPE_CURRENT:
                break;          /* Our current best, preserve */
            case HTYPE_NONE:
                break;          /* If there's no password, preserve */
            case HTYPE_DISABLED:
                break;          /* If there's no password, preserve */
            case HTYPE_INVALID:
                break;          /* Don't recognize the type, preserve */
            default:           /* Not our current best, upgrade */
                set_password(player, check_pw);
                break;
        }
        return 1;
    }
    return 0;
}

bool
set_password(dbref player, const char *password)
{
    if (player == NOTHING)
        return 0;

    if (!password || !*password) {
        if (DBFETCH(player)->sp.player.password)
            free((void *) DBFETCH(player)->sp.player.password);

        char hashbuf[BUFFER_LEN];

        int res = db_hash_password(HTYPE_NONE, hashbuf, NULL, NULL);

        if (!res)
            return 0;
        DBSTORE(player, sp.player.password, alloc_string(hashbuf));
        return 1;
    }

    if (!ok_password(password))
        return 0;

    if (DBFETCH(player)->sp.player.password)
        free((void *) DBFETCH(player)->sp.player.password);

    if (db_hash_passwords) {
        char hashbuf[BUFFER_LEN];

        int res = db_hash_password(HTYPE_CURRENT, hashbuf, password, NULL);

        if (!res)
            return 0;
        DBSTORE(player, sp.player.password, alloc_string(hashbuf));
        return 1;
    }

    DBSTORE(player, sp.player.password, alloc_string(password));
    return 1;
}

dbref
lookup_player(const char *name)
{
    hash_data *hd;

    if (*name == '#') {
        name++;
        if (!OkObj(atoi(name)) || Typeof(atoi(name)) != TYPE_PLAYER)
            return NOTHING;
        else
            return atoi(name);
    } else {
        if (*name == '*')
            name++;

        if ((hd = find_hash(name, player_list, PLAYER_HASH_SIZE)) == NULL)
            return NOTHING;
        else
            return (hd->dbval);
    }
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
    if (!check_password(player, password))
        return NOTHING;

    return player;
}

dbref
create_player(dbref creator, const char *name, const char *password)
{
    char buf[BUFFER_LEN];

    struct object *newp;

    dbref player;

    if (!ok_player_name(name) || !ok_password(password) || tp_db_readonly)
        return NOTHING;

    /* else he doesn't already exist, create him */
    player = new_object(creator);
    newp = DBFETCH(player);

    /* initialize everything */
    NAME(player) = alloc_string(name);
    FLAGS(player) = TYPE_PLAYER;

    if (OkObj(tp_player_prototype)
        && (Typeof(tp_player_prototype) == TYPE_PLAYER)) {
        FLAGS(player) = FLAGS(tp_player_prototype);
        FLAG2(player) = FLAG2(tp_player_prototype);

        if (tp_pcreate_copy_props) {
            newp->properties = copy_prop(tp_player_prototype);
#ifdef DISKBASE
            newp->propsfpos = 0;
            newp->propsmode = PROPS_UNLOADED;
            newp->propstime = 0;
            newp->nextold = NOTHING;
            newp->prevold = NOTHING;
            dirtyprops(player);
#endif
        }
    }

    if (OkObj(tp_player_start)) {
        DBFETCH(player)->location = tp_player_start;
        DBFETCH(player)->sp.player.home = tp_player_start;
    } else {
        DBFETCH(player)->location = GLOBAL_ENVIRONMENT;
        DBFETCH(player)->sp.player.home = GLOBAL_ENVIRONMENT;
    }

    OWNER(player) = player;
    newp->exits = NOTHING;
    newp->sp.player.pennies = tp_start_pennies;
    newp->sp.player.password = NULL; /* this has to stay here. -hinoserm */
    newp->sp.player.curr_prog = NOTHING;
    newp->sp.player.insert_mode = 0;
#ifdef IGNORE_SUPPORT
    newp->sp.player.ignoretime = 0;
#endif /* IGNORE_SUPPORT */

    /* set password */
    set_password(player, password);

    /* link him to tp_player_start */
    PUSH(player, DBFETCH(tp_player_start)->contents);
    add_player(player);
    DBDIRTY(player);
    DBDIRTY(tp_player_start);

    sprintf(buf, CNOTE "%s is born!", NAME(player));
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

    if (!check_password(player, old)) {
        anotify_nolisten2(player,
                          CFAIL "Syntax: @password <oldpass>=<newpass>");
    } else if (!ok_password(newobj)) {
        anotify_nolisten2(player, CFAIL "Bad new password.");
    } else {
        set_password(player, newobj);
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
