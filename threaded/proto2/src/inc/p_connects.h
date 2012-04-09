extern void prim_awakep(PRIM_PROTOTYPE);
extern void prim_online(PRIM_PROTOTYPE);
extern void prim_online_array(PRIM_PROTOTYPE);
extern void prim_concount(PRIM_PROTOTYPE);
extern void prim_condbref(PRIM_PROTOTYPE);
extern void prim_conidle(PRIM_PROTOTYPE);
extern void prim_contime(PRIM_PROTOTYPE);
extern void prim_conhost(PRIM_PROTOTYPE);
extern void prim_conuser(PRIM_PROTOTYPE);
extern void prim_conipnum(PRIM_PROTOTYPE);
extern void prim_conport(PRIM_PROTOTYPE);
extern void prim_conboot(PRIM_PROTOTYPE);
extern void prim_connotify(PRIM_PROTOTYPE);
extern void prim_condescr(PRIM_PROTOTYPE);
extern void prim_descrcon(PRIM_PROTOTYPE);
extern void prim_nextdescr(PRIM_PROTOTYPE);
extern void prim_descr_array(PRIM_PROTOTYPE);
extern void prim_descriptors(PRIM_PROTOTYPE);
extern void prim_descr_setuser(PRIM_PROTOTYPE);
extern void prim_descr_setuser_nopass(PRIM_PROTOTYPE);
extern void prim_descrflush(PRIM_PROTOTYPE);
extern void prim_descr(PRIM_PROTOTYPE);
extern void prim_welcome_user(PRIM_PROTOTYPE);
extern void prim_descrp(PRIM_PROTOTYPE);
extern void prim_motd_notify(PRIM_PROTOTYPE);
extern void prim_descr_logout(PRIM_PROTOTYPE);
extern void prim_descrdbref(PRIM_PROTOTYPE);
extern void prim_descridle(PRIM_PROTOTYPE);
extern void prim_descrtime(PRIM_PROTOTYPE);
extern void prim_descrhost(PRIM_PROTOTYPE);
extern void prim_descruser(PRIM_PROTOTYPE);
extern void prim_descripnum(PRIM_PROTOTYPE);
extern void prim_descrport(PRIM_PROTOTYPE);
extern void prim_descrconport(PRIM_PROTOTYPE);
extern void prim_firstdescr(PRIM_PROTOTYPE);
extern void prim_lastdescr(PRIM_PROTOTYPE);
extern void prim_descrleastidle(PRIM_PROTOTYPE);
extern void prim_descrmostidle(PRIM_PROTOTYPE);
extern void prim_descrboot(PRIM_PROTOTYPE);
extern void prim_getdescrinfo(PRIM_PROTOTYPE);
extern void prim_descr_set(PRIM_PROTOTYPE);
extern void prim_descr_flagp(PRIM_PROTOTYPE);
extern void prim_bandwidth(PRIM_PROTOTYPE);
extern void prim_descrbufsize(PRIM_PROTOTYPE);
extern void prim_descr_sslp(PRIM_PROTOTYPE);
extern void prim_descr_sendfile(PRIM_PROTOTYPE);
extern void prim_descrtype(PRIM_PROTOTYPE);
extern void prim_suid(PRIM_PROTOTYPE);
extern void prim_mccp_start(PRIM_PROTOTYPE);
extern void prim_mccp_end(PRIM_PROTOTYPE);
    
#define PRIMLIST_CONNECTS  { "AWAKE?",                LM1,     1, prim_awakep },                \
                           { "ONLINE",                LM3,     0, prim_online },                \
                           { "CONCOUNT",              LM1,     0, prim_concount },              \
                           { "CONDBREF",              LM3,     1, prim_condbref },              \
                           { "CONIDLE",               LM2,     1, prim_conidle },               \
                           { "CONTIME",               LM2,     1, prim_contime },               \
                           { "CONHOST",               LMAGE,   1, prim_conhost },               \
                           { "CONUSER",               LARCH,   1, prim_conuser },               \
                           { "CONBOOT",               LWIZ,    1, prim_conboot },               \
                           { "CONNOTIFY",             LM3,     2, prim_connotify },             \
                           { "CONDESCR",              LM3,     1, prim_condescr },              \
                           { "DESCRCON",              LM3,     1, prim_descrcon },              \
                           { "NEXTDESCR",             LMAGE,   1, prim_nextdescr },             \
                           { "DESCRIPTORS",           LM3,     1, prim_descriptors },           \
                           { "DESCR_SETUSER",         LWIZ,    3, prim_descr_setuser },         \
                           { "DESCR_SETUSER_NOPASS",  LBOY,    2, prim_descr_setuser_nopass },  \
                           { "CONIPNUM",              LMAGE,   1, prim_conipnum },              \
                           { "CONPORT",               LARCH,   1, prim_conport },               \
                           { "ONLINE_ARRAY",          LM3,     0, prim_online_array },          \
                           { "DESCR_ARRAY",           LM3,     1, prim_descr_array },           \
                           { "DESCRFLUSH",            LM3,     1, prim_descrflush },            \
                           { "DESCR",                 LM1,     0, prim_descr },                 \
                           { "DESCR_WELCOME_USER",    LM3,     1, prim_welcome_user },          \
                           { "DESCR?",                LM3,     1, prim_descrp },                \
                           { "MOTD_NOTIFY",           LM3,     1, prim_motd_notify },           \
                           { "DESCR_LOGOUT",          LARCH,   1, prim_descr_logout },          \
                           { "DESCRDBREF",            LM3,     1, prim_descrdbref },            \
                           { "DESCRIDLE",             LM2,     1, prim_descridle },             \
                           { "DESCRTIME",             LM2,     1, prim_descrtime },             \
                           { "DESCRHOST",             LMAGE,   1, prim_descrhost },             \
                           { "DESCRUSER",             LARCH,   1, prim_descruser },             \
                           { "DESCRIPNUM",            LM1,     1, prim_descripnum },            \
                           { "DESCRPORT",             LARCH,   1, prim_descrport },             \
                           { "DESCRCONPORT",          LARCH,   1, prim_descrconport },          \
                           { "FIRSTDESCR",            LM1,     1, prim_firstdescr },            \
                           { "LASTDESCR",             LM1,     1, prim_lastdescr },             \
                           { "DESCRLEASTIDLE",        LM1,     1, prim_descrleastidle },        \
                           { "DESCRMOSTIDLE",         LM1,     1, prim_descrmostidle },         \
                           { "DESCRBOOT",             LARCH,   1, prim_descrboot },             \
                           { "GETDESCRINFO",          LARCH,   1, prim_getdescrinfo },          \
                           { "DESCR_SET",             LARCH,   2, prim_descr_set },             \
                           { "DESCR_FLAG?",           LM1,     2, prim_descr_flagp },           \
                           { "BANDWIDTH",             LMAGE,   0, prim_bandwidth },             \
                           { "DESCRBUFSIZE",          LM3,     1, prim_descrbufsize },          \
                           { "DESCR_SSL?",            LM3,     1, prim_descr_sslp },            \
                           { "DESCR_SENDFILE",        LBOY,    4, prim_descr_sendfile },        \
                           { "DESCRTYPE",             LM2,     1, prim_descrtype },             \
                           { "MCCP_START",            LM3,     1, prim_mccp_start },            \
                           { "MCCP_END",              LMAGE,   1, prim_mccp_end }
    

#define PRIMS_CONNECTS_CNT 52
