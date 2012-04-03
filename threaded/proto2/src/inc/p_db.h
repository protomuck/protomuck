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
extern void prim_truecontrols(PRIM_PROTOTYPE);
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
extern void prim_isflagp(PRIM_PROTOTYPE);
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
extern void prim_entrances_array(PRIM_PROTOTYPE);
extern void prim_lflags_update(PRIM_PROTOTYPE);

#define PRIMLIST_DB { "ADDPENNIES",         LM2, 2, prim_addpennies },      \
                    { "MOVETO",             LM1, 2, prim_moveto },          \
                    { "PENNIES",            LM1, 1, prim_pennies },         \
                    { "DBCMP",              LM1, 2, prim_dbcomp },          \
                    { "DBREF",              LM1, 1, prim_dbref },           \
                    { "CONTENTS",           LM1, 1, prim_contents },        \
                    { "EXITS",              LM1, 1, prim_exits },           \
                    { "NEXT",               LM1, 1, prim_next },            \
                    { "NAME",               LM1, 1, prim_name },            \
                    { "SETNAME",            LM1, 2, prim_setname },         \
                    { "MATCH",              LM1, 1, prim_match },           \
                    { "RMATCH",             LM1, 2, prim_rmatch },          \
                    { "COPYOBJ",            LM1, 1, prim_copyobj },         \
                    { "SET",                LM1, 2, prim_set },             \
                    { "MLEVEL",             LM1, 1, prim_mlevel },          \
                    { "FLAG?",              LM1, 2, prim_flagp },           \
                    { "PLAYER?",            LM1, 1, prim_playerp },         \
                    { "THING?",             LM1, 1, prim_thingp },          \
                    { "ROOM?",              LM1, 1, prim_roomp },           \
                    { "PROGRAM?",           LM1, 1, prim_programp },        \
                    { "EXIT?",              LM1, 1, prim_exitp },           \
                    { "OK?",                LM1, 1, prim_okp },             \
                    { "LOCATION",           LM1, 1  , prim_location },      \
                    { "OWNER",              LM1, 1, prim_owner },           \
                    { "GETLINK",            LM1, 1, prim_getlink },         \
                    { "SETLINK",            LM1, 2, prim_setlink },         \
                    { "SETOWN",             LM1, 2, prim_setown },          \
                    { "NEWOBJECT",          LM2, 2, prim_newobject },       \
                    { "NEWROOM",            LM2, 2, prim_newroom },         \
                    { "NEWEXIT",            LMAGE, 2, prim_newexit },       \
                    { "RECYCLE",            LM3, 1, prim_recycle },         \
                    { "TRUECONTROLS",       LM1, 2, prim_truecontrols },    \
                    { "SETLOCKSTR",         LM1, 2, prim_setlockstr },      \
                    { "GETLOCKSTR",         LM1, 1, prim_getlockstr },      \
                    { "PART_PMATCH",        LM1, 1, prim_part_pmatch },     \
                    { "CONTROLS",           LM1, 2, prim_controls },        \
                    { "TRUENAME",           LM1, 1, prim_truename },        \
                    { "CHECKPASSWORD",      LARCH, 2, prim_checkpassword }, \
                    { "PMATCH",             LM1, 1, prim_pmatch },          \
                    { "NEWPLAYER",          LARCH, 2, prim_newplayer },     \
                    { "COPYPLAYER",         LARCH, 3, prim_copyplayer },    \
                    { "TOADPLAYER",         LARCH, 2, prim_toadplayer },    \
                    { "OBJMEM",             LM1, 1, prim_objmem },          \
                    { "GETLINKS",           LM1, 1, prim_getlinks },        \
                    { "MOVEPENNIES",        LM2, 3, prim_movepennies },     \
                    { "INSTANCES",          LM1, 1, prim_instances },       \
                    { "COMPILED?",          LM1, 1, prim_compiledp },       \
                    { "SETPASSWORD",        LMAGE, 3, prim_setpassword },   \
                    { "NEWPASSWORD",        LARCH, 2, prim_newpassword },   \
                    { "NEXTENTRANCE",       LMAGE, 2, prim_nextentrance },  \
                    { "ISPOWER?",           LM1, 1, prim_ispowerp },        \
                    { "POWER?",             LM1, 2, prim_powerp },          \
                    { "NEWPROGRAM",         LBOY, 1, prim_newprogram },     \
                    { "COMPILE",            LBOY, 2, prim_compile },        \
                    { "UNCOMPILE",          LBOY, 1, prim_uncompile },      \
                    { "CONTENTS_ARRAY",     LM1, 1, prim_contents_array },  \
                    { "EXITS_ARRAY",        LM1, 1, prim_exits_array },     \
                    { "GETLINKS_ARRAY",     LM1, 1, prim_getlinks_array },  \
                    { "GETOBJINFO",         LM3, 1, prim_getobjinfo },      \
                    { "FINDNEXT",           LM2, 4, prim_findnext },        \
                    { "FIND_ARRAY",         LM2, 3, prim_find_array },      \
                    { "ENTRANCES_ARRAY",    LM1, 1, prim_entrances_array }, \
                    { "ISFLAG?",            LM1, 1, prim_isflagp },         \
                    { "LFLAGS_UPDATE",      LARCH, 0, prim_lflags_update }

#define PRIMS_DB_CNT 64
