extern void prim_addpennies(PRIM_PROTOTYPE);
extern void prim_moveto(PRIM_PROTOTYPE);
extern void prim_pennies(PRIM_PROTOTYPE);
extern void prim_dbcomp(PRIM_PROTOTYPE);
extern void prim_dbref(PRIM_PROTOTYPE);
extern void prim_contents(PRIM_PROTOTYPE);
extern void prim_exits(PRIM_PROTOTYPE);
extern void prim_next(PRIM_PROTOTYPE);
extern void prim_name(PRIM_PROTOTYPE);
extern void prim_setname(PRIM_PROTOTYPE);
extern void prim_match(PRIM_PROTOTYPE);
extern void prim_rmatch(PRIM_PROTOTYPE);
extern void prim_copyobj(PRIM_PROTOTYPE);
extern void prim_set(PRIM_PROTOTYPE);
extern void prim_mlevel(PRIM_PROTOTYPE);
extern void prim_flagp(PRIM_PROTOTYPE);
extern void prim_playerp(PRIM_PROTOTYPE);
extern void prim_thingp(PRIM_PROTOTYPE);
extern void prim_roomp(PRIM_PROTOTYPE);
extern void prim_programp(PRIM_PROTOTYPE);
extern void prim_exitp(PRIM_PROTOTYPE);
extern void prim_okp(PRIM_PROTOTYPE);
extern void prim_location(PRIM_PROTOTYPE);
extern void prim_owner(PRIM_PROTOTYPE);
extern void prim_controls(PRIM_PROTOTYPE);
extern void prim_getlink(PRIM_PROTOTYPE);
extern void prim_getlinks(PRIM_PROTOTYPE);
extern void prim_setlink(PRIM_PROTOTYPE);
extern void prim_setown(PRIM_PROTOTYPE);
extern void prim_newobject(PRIM_PROTOTYPE);
extern void prim_newroom(PRIM_PROTOTYPE);
extern void prim_newexit(PRIM_PROTOTYPE);
extern void prim_recycle(PRIM_PROTOTYPE);
extern void prim_setlockstr(PRIM_PROTOTYPE);
extern void prim_getlockstr(PRIM_PROTOTYPE);
extern void prim_part_pmatch(PRIM_PROTOTYPE);
extern void prim_truename(PRIM_PROTOTYPE);
extern void prim_checkpassword(PRIM_PROTOTYPE);
extern void prim_pmatch(PRIM_PROTOTYPE);
extern void prim_nextowned(PRIM_PROTOTYPE);
extern void prim_nextplayer(PRIM_PROTOTYPE);
extern void prim_newplayer(PRIM_PROTOTYPE);
extern void prim_copyplayer(PRIM_PROTOTYPE);
extern void prim_toadplayer(PRIM_PROTOTYPE);
extern void prim_objmem(PRIM_PROTOTYPE);
extern void prim_movepennies(PRIM_PROTOTYPE);
extern void prim_instances(PRIM_PROTOTYPE);
extern void prim_compiledp(PRIM_PROTOTYPE);
extern void prim_setpassword(PRIM_PROTOTYPE);
extern void prim_newpassword(PRIM_PROTOTYPE);
extern void prim_nextentrance(PRIM_PROTOTYPE);
extern void prim_ispowerp(PRIM_PROTOTYPE);
extern void prim_powerp(PRIM_PROTOTYPE);
extern void prim_newprogram(PRIM_PROTOTYPE);
extern void prim_compile(PRIM_PROTOTYPE);
extern void prim_uncompile(PRIM_PROTOTYPE);
extern void prim_contents_array(PRIM_PROTOTYPE);
extern void prim_exits_array(PRIM_PROTOTYPE);
extern void prim_getlinks_array(PRIM_PROTOTYPE);
extern void prim_getobjinfo(PRIM_PROTOTYPE);
extern void prim_findnext(PRIM_PROTOTYPE);
extern void prim_find_array(PRIM_PROTOTYPE);

#define PRIMS_DB_FUNCS prim_addpennies, prim_moveto, prim_pennies,       \
    prim_dbcomp, prim_dbref, prim_contents, prim_exits, prim_next,       \
    prim_name, prim_setname, prim_match, prim_rmatch, prim_copyobj,      \
    prim_set, prim_mlevel, prim_flagp, prim_playerp, prim_thingp,        \
    prim_roomp, prim_programp, prim_exitp, prim_okp, prim_location,      \
    prim_owner, prim_getlink, prim_setlink, prim_setown, prim_newobject, \
    prim_newroom, prim_newexit,  prim_recycle,              \
    prim_setlockstr, prim_getlockstr, prim_part_pmatch, prim_controls,   \
    prim_truename, prim_checkpassword, prim_pmatch, prim_newplayer,      \
    prim_copyplayer, prim_toadplayer, prim_objmem, prim_getlinks,        \
    prim_movepennies, prim_instances, prim_compiledp, prim_setpassword,  \
    prim_newpassword, prim_nextentrance, prim_ispowerp, prim_powerp,     \
    prim_newprogram, prim_compile, prim_uncompile, prim_contents_array,  \
    prim_exits_array, prim_getlinks_array, prim_getobjinfo, prim_findnext,\
    prim_find_array

#define PRIMS_DB_NAMES "ADDPENNIES", "MOVETO", "PENNIES",    \
    "DBCMP", "DBREF", "CONTENTS", "EXITS", "NEXT",           \
    "NAME", "SETNAME", "MATCH", "RMATCH", "COPYOBJ",         \
    "SET", "MLEVEL", "FLAG?", "PLAYER?", "THING?",           \
    "ROOM?", "PROGRAM?", "EXIT?", "OK?", "LOCATION",         \
    "OWNER", "GETLINK", "SETLINK", "SETOWN", "NEWOBJECT",    \
    "NEWROOM", "NEWEXIT",  "RECYCLE",              \
    "SETLOCKSTR", "GETLOCKSTR", "PART_PMATCH", "CONTROLS",   \
    "TRUENAME", "CHECKPASSWORD", "PMATCH", "NEWPLAYER",      \
    "COPYPLAYER", "TOADPLAYER", "OBJMEM", "GETLINKS",        \
    "MOVEPENNIES", "INSTANCES", "COMPILED?", "SETPASSWORD",  \
    "NEWPASSWORD", "NEXTENTRANCE", "ISPOWER?", "POWER?",     \
    "NEWPROGRAM", "COMPILE", "UNCOMPILE", "CONTENTS_ARRAY",  \
    "EXITS_ARRAY", "GETLINKS_ARRAY", "GETOBJINFO", "FINDNEXT",\
    "FIND_ARRAY"

#define PRIMS_DB_CNT 60

