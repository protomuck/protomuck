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
extern void prim_descrflush(PRIM_PROTOTYPE);
extern void prim_descr(PRIM_PROTOTYPE);
extern void prim_descr_htmlp(PRIM_PROTOTYPE);
extern void prim_descr_pueblop(PRIM_PROTOTYPE);
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

#define PRIMS_CONNECTS_FUNCS prim_awakep, prim_online, prim_concount,      \
    prim_condbref, prim_conidle, prim_contime, prim_conhost, prim_conuser, \
    prim_conboot, prim_connotify, prim_condescr, prim_descrcon,            \
    prim_nextdescr, prim_descriptors, prim_descr_setuser,		         \
    prim_conipnum, prim_conport, prim_online_array, prim_descr_array,      \
    prim_descrflush, prim_descr, prim_descr_htmlp, prim_descr_pueblop,     \
    prim_welcome_user, prim_descrp, prim_motd_notify,                      \
    prim_descr_logout, prim_descrdbref, prim_descridle, prim_descrtime,    \
    prim_descrhost, prim_descruser, prim_descripnum, prim_descrport,       \
    prim_descrconport, prim_firstdescr, prim_lastdescr,                    \
    prim_descrleastidle, prim_descrmostidle, prim_descrboot,               \
    prim_getdescrinfo, prim_descr_set, prim_descr_flagp, prim_bandwidth,   \
    prim_descrbufsize, prim_descr_sslp

#define PRIMS_CONNECTS_NAMES "AWAKE?", "ONLINE", "CONCOUNT",  \
    "CONDBREF", "CONIDLE", "CONTIME", "CONHOST", "CONUSER",   \
    "CONBOOT", "CONNOTIFY", "CONDESCR", "DESCRCON",           \
    "NEXTDESCR", "DESCRIPTORS", "DESCR_SETUSER",	        \
    "CONIPNUM", "CONPORT", "ONLINE_ARRAY", "DESCR_ARRAY",     \
    "DESCRFLUSH", "DESCR", "DESCR_HTML?", "DESCR_PUEBLO?",    \
    "DESCR_WELCOME_USER", "DESCR?", "MOTD_NOTIFY",            \
    "DESCR_LOGOUT", "DESCRDBREF", "DESCRIDLE", "DESCRTIME",   \
    "DESCRHOST", "DESCRUSER", "DESCRIPNUM", "DESCRPORT",      \
    "DESCRCONPORT", "FIRSTDESCR", "LASTDESCR",                \
    "DESCRLEASTIDLE", "DESCRMOSTIDLE", "DESCRBOOT",           \
    "GETDESCRINFO", "DESCR_SET", "DESCR_FLAG?", "BANDWIDTH",  \
    "DESCRBUFSIZE", "DESCR_SSL?"

#define PRIMS_CONNECTS_CNT 46

