#include "config.h"
#include "params.h"
#include "defaults.h"
#include "interface.h"
#include "tune.h"
#include "externs.h"
#include "array.h"
#include "interp.h"

#ifdef W4_TUNEABLES
/* Certain @tuneables are readable and writtable by W4+ only */
#define WBOY 8
#define RBOY 8
#define WMAN 9
#define RMAN 9
#else
/* W3 has full access to all @tuneables */
#define WBOY 7
#define RBOY 7
#define WMAN 7
#define RMAN 7
#endif

const char *tp_dumpwarn_mesg = DUMPWARN_MESG;
const char *tp_deltawarn_mesg = DELTAWARN_MESG;
const char *tp_dumpdeltas_mesg = DUMPDELTAS_MESG;
const char *tp_dumping_mesg = DUMPING_MESG;
const char *tp_dumpdone_mesg = DUMPDONE_MESG;
const char *tp_leave_message = LEAVE_MESSAGE;
const char *tp_huh_mesg = HUH_MESG;

#ifdef MAILSERVER
const char *tp_mailserver = MAILSERVER;
#endif
const char *tp_servername = "localhost";
const char *tp_noperm_mesg = NOPERM_MESSAGE;
const char *tp_noguest_mesg = NOGUEST_MESSAGE;
const char *tp_idleboot_msg = IDLE_MESG;
const char *tp_penny = PENNY;
const char *tp_pennies = PENNIES;
const char *tp_cpenny = CPENNY;
const char *tp_cpennies = CPENNIES;
const char *tp_muckname = MUCKNAME;
const char *tp_userflag_name = "";
const char *tp_reg_email = "admin@your.host.here";
const char *tp_proplist_counter_fmt = "P#";
const char *tp_proplist_entry_fmt = "P#/N";
const char *tp_unidle_command = "";
const char *tp_unidle_command_msg = "";
const char *tp_ssl_keyfile_passwd = "";
const char *tp_reslvd_address = "127.0.0.1:12111";

struct tune_str_entry {
    const char *group;
    const char *name;
    const char **str;
    int writemlev;
    int readmlev;
    int isdefault;
};

struct tune_str_entry tune_str_list[] = {
    {"Database", "dumpwarn_mesg", &tp_dumpwarn_mesg, LARCH, LMUF, 1},
    {"Database", "deltawarn_mesg", &tp_deltawarn_mesg, LARCH, LMUF, 1},
    {"Database", "dumpdeltas_mesg", &tp_dumpdeltas_mesg, LARCH, LMUF, 1},
    {"Database", "dumping_mesg", &tp_dumping_mesg, LARCH, LMUF, 1},
    {"Database", "dumpdone_mesg", &tp_dumpdone_mesg, LARCH, LMUF, 1},
    {"Message", "huh_mesg", &tp_huh_mesg, LARCH, LMUF, 1},
    {"Message", "leave_message", &tp_leave_message, LARCH, LMUF, 1},
    {"Idletime", "idleboot_message", &tp_idleboot_msg, LARCH, LMUF, 1},
    {"Message", "noperm_mesg", &tp_noperm_mesg, LARCH, LMUF, 1},
    {"Guest", "noguest_mesg", &tp_noguest_mesg, LARCH, LMUF, 1},
    {"Currency", "penny", &tp_penny, LARCH, LMUF, 1},
    {"Currency", "pennies", &tp_pennies, LARCH, LMUF, 1},
    {"Currency", "cpenny", &tp_cpenny, LARCH, LMUF, 1},
    {"Currency", "cpennies", &tp_cpennies, LARCH, LMUF, 1},
    {"Identity", "muckname", &tp_muckname, WBOY, LMUF, 1},
    {"Identity", "userflag_name", &tp_userflag_name, LARCH, LMUF, 1},
#ifdef MAILSERVER
    {"Legacy", "mailserver", &tp_mailserver, WBOY, LWIZ, 1},
#endif
    {"Identity", "servername", &tp_servername, WBOY, LMUF, 1},
    {"Legacy", "reg_email", &tp_reg_email, WBOY, LARCH, 1},
    {"Props", "proplist_counter_fmt", &tp_proplist_counter_fmt, LARCH, LMUF, 1},
    {"Props", "proplist_entry_fmt", &tp_proplist_entry_fmt, LARCH, LMUF, 1},
    {"IdleTime", "unidle_command", &tp_unidle_command, LARCH, LMUF, 1},
    {"IdleTime", "unidle_command_msg", &tp_unidle_command_msg, LARCH, LMUF, 1},
    {"SSL", "ssl_keyfile_passwd", &tp_ssl_keyfile_passwd, WBOY, LBOY, 1},
    {"System", "reslvd_address", &tp_reslvd_address, WBOY, LWIZ, 1},
    {NULL, NULL, NULL, 0, 0, 0}
};


/* times */
time_t tp_dump_interval = DUMP_INTERVAL;
time_t tp_dump_warntime = DUMP_WARNTIME;
time_t tp_monolithic_interval = MONOLITHIC_INTERVAL;
time_t tp_clean_interval = CLEAN_INTERVAL;
time_t tp_aging_time = AGING_TIME;
time_t tp_connidle = CONNIDLE;
time_t tp_maxidle = MAXIDLE;
time_t tp_idletime = IDLETIME;
time_t tp_cron_interval = CRON_INTERVAL;
time_t tp_archive_interval = ARCHIVE_INTERVAL;

struct tune_time_entry {
    const char *group;
    const char *name;
    time_t *tim;
    int writemlev;
    int readmlev;
};

struct tune_time_entry tune_time_list[] = {
    {"Database", "dump_interval", &tp_dump_interval, LARCH, LMUF},
    {"Database", "dump_warntime", &tp_dump_warntime, LARCH, LMUF},
    {"Database", "monolithic_interval", &tp_monolithic_interval, LARCH, LMUF},
    {"System", "clean_interval", &tp_clean_interval, LARCH, LMUF},
    {"System", "aging_time", &tp_aging_time, LARCH, LMUF},
    {"Idletime", "connidle", &tp_connidle, LARCH, LMUF},
    {"Idletime", "maxidle", &tp_maxidle, LARCH, LMUF},
    {"Idletime", "idletime", &tp_idletime, LARCH, LMUF},
    {"MUF", "cron_interval", &tp_cron_interval, LARCH, LMUF},
    {"Database", "archive_interval", &tp_archive_interval, LARCH, LMUF},
    {NULL, NULL, NULL, 0, 0}
};

/* integers */
int tp_textport = TINYPORT;
int tp_puebloport = TINYPORT - 2;
int tp_sslport = TINYPORT + 1;
int tp_max_object_endowment = MAX_OBJECT_ENDOWMENT;
int tp_object_cost = OBJECT_COST;
int tp_exit_cost = EXIT_COST;
int tp_link_cost = LINK_COST;
int tp_room_cost = ROOM_COST;
int tp_lookup_cost = LOOKUP_COST;
int tp_max_pennies = MAX_PENNIES;
int tp_penny_rate = PENNY_RATE;
int tp_start_pennies = START_PENNIES;
int tp_command_burst_size = COMMAND_BURST_SIZE;
int tp_commands_per_time = COMMANDS_PER_TIME;
int tp_command_time_msec = COMMAND_TIME_MSEC;
int tp_max_delta_objs = MAX_DELTA_OBJS;
int tp_max_loaded_objs = MAX_LOADED_OBJS;
int tp_max_process_limit = MAX_PROCESS_LIMIT;
int tp_max_plyr_processes = MAX_PLYR_PROCESSES;
int tp_max_instr_count = MAX_INSTR_COUNT;
int tp_instr_slice = INSTR_SLICE;
int tp_mpi_max_commands = MPI_MAX_COMMANDS;
int tp_pause_min = PAUSE_MIN;
int tp_free_frames_pool = FREE_FRAMES_POOL;
int tp_max_output = MAX_OUTPUT;
int tp_rand_screens = 0;
int tp_listen_mlev = LISTEN_MLEV;
int tp_playermax_limit = PLAYERMAX_LIMIT;
int tp_process_timer_limit = 4;
int tp_dump_copies = 10;
int tp_min_progbreak_lev = 0;

#ifdef MCP_SUPPORT
int tp_mcp_muf_mlev = MCP_MUF_MLEV;
#endif
int tp_max_wiz_preempt_count = 0;
int tp_wizhidden_access_bit = 3;
int tp_userflag_mlev = 0;

int tp_wwwport = TINYPORT - 1;  /* hinoserm */

#ifdef NEWHTTPD                 /* hinoserm */
int tp_web_logfile_lvl = 2;     /* hinoserm */
int tp_web_logwall_lvl = 1;     /* hinoserm */
int tp_web_htmuf_mlvl = LM2;    /* hinoserm */
int tp_web_max_files = 4;       /* hinoserm */
int tp_web_max_filesize = 4096; /* 4mb, in kb -hinoserm */
int tp_web_max_users = 10;      /* hinoserm */
#endif /* hinoserm */
#ifdef SQL_SUPPORT
int tp_mysql_result_limit = 40;
#endif
struct tune_val_entry {
    const char *group;
    const char *name;
    int *val;
    int writemlev;
    int readmlev;
};

struct tune_val_entry tune_val_list[] = {
    {"System", "mainport", &tp_textport, WBOY, LMUF},
    {"System", "sslport", &tp_sslport, WBOY, LMUF},
    {"System", "wwwport", &tp_wwwport, WBOY, LMUF},
    {"System", "puebloport", &tp_puebloport, WBOY, LMUF},
    {"Currency", "max_object_endowment", &tp_max_object_endowment, LARCH, LMUF},
    {"Currency", "object_cost", &tp_object_cost, LARCH, LMUF},
    {"Currency", "exit_cost", &tp_exit_cost, LARCH, LMUF},
    {"Currency", "link_cost", &tp_link_cost, LARCH, LMUF},
    {"Currency", "room_cost", &tp_room_cost, LARCH, LMUF},
    {"Currency", "lookup_cost", &tp_lookup_cost, LARCH, LMUF},
    {"Currency", "max_pennies", &tp_max_pennies, LARCH, LMUF},
    {"Currency", "penny_rate", &tp_penny_rate, LARCH, LMUF},
    {"Currency", "start_pennies", &tp_start_pennies, LARCH, LMUF},
    {"System", "command_burst_size", &tp_command_burst_size, LARCH, LMUF},
    {"System", "commands_per_time", &tp_commands_per_time, LARCH, LMUF},
    {"System", "command_time_msec", &tp_command_time_msec, LARCH, LMUF},
    {"Database", "max_delta_objs", &tp_max_delta_objs, LARCH, LMUF},
    {"Database", "max_loaded_objs", &tp_max_loaded_objs, LARCH, LMUF},
    {"Database", "wizhidden_access_bit", &tp_wizhidden_access_bit, WBOY, LMAGE},
    {"Database", "userflag_mlev", &tp_userflag_mlev, LARCH, LMUF},
    {"MUF", "max_process_limit", &tp_max_process_limit, LARCH, LMUF},
    {"MUF", "max_plyr_processes", &tp_max_plyr_processes, LARCH, LMUF},
    {"MUF", "max_instr_count", &tp_max_instr_count, LARCH, LMUF},
    {"MUF", "instr_slice", &tp_instr_slice, LARCH, LMUF},
    {"MPI", "mpi_max_commands", &tp_mpi_max_commands, LARCH, LMUF},
    {"System", "pause_min", &tp_pause_min, LARCH, LMUF},
    {"MUF", "free_frames_pool", &tp_free_frames_pool, LARCH, LMUF},
    {"System", "max_output", &tp_max_output, LARCH, LMUF},
    {"System", "rand_screens", &tp_rand_screens, LARCH, LMUF},
    {"MUF", "listen_mlev", &tp_listen_mlev, WBOY, LMUF},
    {"Maxplayers", "playermax_limit", &tp_playermax_limit, WBOY, LMUF},
    {"MUF", "process_timer_limit", &tp_process_timer_limit, LARCH, LMUF},
    {"Database", "dump_copies", &tp_dump_copies, WBOY, LMUF},
    {"MUF", "min_progbreak_lev", &tp_min_progbreak_lev, LARCH, LMAGE},
#ifdef MCP_SUPPORT
    {"MUF", "mcp_muf_mlev", &tp_mcp_muf_mlev, LARCH, LMAGE},
#endif
    {"MUF", "max_wiz_preempt_count", &tp_max_wiz_preempt_count, LARCH, LMAGE},
#ifdef SQL_SUPPORT
    {"MUF", "mysql_result_limit", &tp_mysql_result_limit, LBOY, LARCH},
#endif
#ifdef NEWHTTPD                 /* hinoserm */
    {"HTTPD", "web_logfile_lvl", &tp_web_logfile_lvl, WBOY, LMUF}, /* hinoserm */
    {"HTTPD", "web_logwall_lvl", &tp_web_logwall_lvl, LARCH, LMUF}, /* hinoserm */
    {"HTTPD", "web_htmuf_mlvl", &tp_web_htmuf_mlvl, LARCH, LMUF}, /* hinoserm */
    {"HTTPD", "web_max_files", &tp_web_max_files, WBOY, LMUF}, /* hinoserm */
    {"HTTPD", "web_max_filesize", &tp_web_max_filesize, WBOY, LMUF}, /* hinoserm */
    {"HTTPD", "web_max_users", &tp_web_max_users, LARCH, LMUF}, /* hinoserm */
#endif /* hinoserm */
    {NULL, NULL, NULL, 0, 0}
};


/* dbrefs */
dbref tp_quit_prog = -1;
dbref tp_login_who_prog = -1;
dbref tp_player_start = PLAYER_START;
dbref tp_player_prototype = -1;
dbref tp_cron_prog = -1;
dbref tp_default_parent = GLOBAL_ENVIRONMENT;

#ifdef NEWHTTPD
dbref tp_www_root = -1;         /* hinoserm */
#endif /* hinoserm */

struct tune_ref_entry {
    const char *group;
    const char *name;
    int typ;
    dbref *ref;
    int writemlev;
    int readmlev;
};

struct tune_ref_entry tune_ref_list[] = {
    {"System", "quit_prog", TYPE_PROGRAM, &tp_quit_prog, WBOY, LMAGE},
    {"System", "login_who_prog", TYPE_PROGRAM, &tp_login_who_prog, WBOY, LMAGE},
    {"Pcreate", "player_start", TYPE_ROOM, &tp_player_start, LARCH, LMAGE},
    {"Pcreate", "player_prototype", TYPE_PLAYER, &tp_player_prototype, LARCH,
     LMAGE},
    {"MUF", "cron_prog", TYPE_PROGRAM, &tp_cron_prog, LARCH, LMAGE},
    {"Building", "default_parent", TYPE_ROOM, &tp_default_parent, LARCH, LMAGE},
#ifdef NEWHTTPD                 /* hinoserm */
    {"System", "www_root", TYPE_ROOM, &tp_www_root, LARCH, LMAGE}, /* hinoserm */
#endif /* hinoserm */
    {NULL, NULL, 0, NULL, 0, 0}
};

/* booleans */
int tp_hostnames = HOSTNAMES;
int tp_log_commands = LOG_COMMANDS;
int tp_log_interactive = LOG_INTERACTIVE;
int tp_log_connects = LOG_CONNECTS;
int tp_log_failed_commands = LOG_FAILED_COMMANDS;
int tp_log_programs = LOG_PROGRAMS;
int tp_log_guests = LOG_GUESTS;
int tp_log_suspects = LOG_SUSPECTS;
int tp_log_wizards = LOG_WIZARDS;
int tp_log_files = LOG_FILES;
int tp_log_sockets = LOG_SOCKETS;
int tp_log_failedhelp = LOG_FAILEDHELP;
int tp_dbdump_warning = DBDUMP_WARNING;
int tp_deltadump_warning = DELTADUMP_WARNING;
int tp_periodic_program_purge = PERIODIC_PROGRAM_PURGE;
int tp_secure_who = SECURE_WHO;
int tp_who_doing = WHO_DOING;
int tp_realms_control = REALMS_CONTROL;
int tp_listeners = LISTENERS;
int tp_listeners_obj = LISTENERS_OBJ;
int tp_listeners_env = LISTENERS_ENV;
int tp_zombies = ZOMBIES;
int tp_wiz_vehicles = WIZ_VEHICLES;
int tp_wiz_puppets = 0;
int tp_wiz_name = 0;
int tp_recycle_frobs = 0;
int tp_m1_name_notify = M1_NAME_NOTIFY;
int tp_teleport_to_player = TELEPORT_TO_PLAYER;
int tp_secure_teleport = SECURE_TELEPORT;
int tp_exit_darking = EXIT_DARKING;
int tp_thing_darking = THING_DARKING;
int tp_dark_sleepers = DARK_SLEEPERS;
int tp_who_hides_dark = WHO_HIDES_DARK;
int tp_compatible_priorities = COMPATIBLE_PRIORITIES;
int tp_do_mpi_parsing = DO_MPI_PARSING;
int tp_look_propqueues = LOOK_PROPQUEUES;
int tp_dump_propqueues = 0;
int tp_lock_envcheck = LOCK_ENVCHECK;
int tp_diskbase_propvals = DISKBASE_PROPVALS;
int tp_idleboot = IDLEBOOT;
int tp_playermax = PLAYERMAX;
int tp_db_readonly = 0;
int tp_building = 1;
int tp_restricted_building = 1;
int tp_all_can_build_rooms = 1;
int tp_pcreate_copy_props = 0;
int tp_enable_home = 1;
int tp_enable_idle_msgs = 0;
int tp_user_idle_propqueue = 0;
int tp_use_self_on_command = 1;
int tp_quiet_moves = 0;
int tp_quiet_connects = 0;
int tp_proplist_int_counter = 0;

#ifdef MCP_SUPPORT
int tp_enable_mcp = 1;
#endif
#ifdef CONTROLS_SUPPORT
int tp_wiz_realms = 1;
#endif
int tp_enable_commandprops = 1;
int tp_old_parseprop = 0;
int tp_mpi_needflag = 0;
int tp_guest_needflag = 0;
int tp_mortalwho = 1;
int tp_fb_controls = 1;
int tp_allow_old_trigs = 1;
int tp_multi_wizlevels = 1;
int tp_auto_archive = 0;
int tp_optimize_muf = OPTIMIZE_MUF;
int tp_compatible_muf_perms = 0;
int tp_allow_unidle = 0;
int tp_alt_infinity_handler = 1;
int tp_autolinking = 1;

/* int tp_require_has_mpi_arg = 0; */

#ifdef NEWHTTPD                 /* hinoserm */
int tp_web_allow_players = 1;   /* hinoserm */
int tp_web_allow_playerhtmuf = 0; /* hinoserm */
int tp_web_allow_htmuf = 1;     /* hinoserm */
int tp_web_allow_vhosts = 1;    /* hinoserm */
int tp_web_allow_files = 1;     /* hinoserm */
int tp_web_allow_dirlist = 1;   /* hinoserm */
int tp_web_allow_mpi = 1;       /* alynna */
#endif /* hinoserm */

struct tune_bool_entry {
    const char *group;
    const char *name;
    int *boolv;
    int writemlev;
    int readmlev;
};

struct tune_bool_entry tune_bool_list[] = {
    {"Commands", "use_hostnames", &tp_hostnames, WBOY, LMAGE},
    {"Logs", "log_commands", &tp_log_commands, WBOY, LWIZ},
    {"Logs", "log_interactive", &tp_log_interactive, WBOY, LWIZ},
    {"Logs", "log_connects", &tp_log_connects, WBOY, LWIZ},
    {"Logs", "log_failed_commands", &tp_log_failed_commands, WBOY, LWIZ},
    {"Logs", "log_programs", &tp_log_programs, WBOY, LWIZ},
    {"Logs", "log_guests", &tp_log_guests, WBOY, LWIZ},
    {"Logs", "log_suspects", &tp_log_suspects, WBOY, LWIZ},
    {"Logs", "log_wizards", &tp_log_wizards, WBOY, LWIZ},
    {"Logs", "log_files", &tp_log_files, WBOY, LWIZ},
    {"Logs", "log_sockets", &tp_log_sockets, WBOY, LWIZ},
    {"Logs", "log_failedhelp", &tp_log_failedhelp, WBOY, LWIZ},
    {"Database", "dbdump_warning", &tp_dbdump_warning, LARCH, LMUF},
    {"Database", "deltadump_warning", &tp_deltadump_warning, LARCH, LMUF},
    {"System", "periodic_program_purge", &tp_periodic_program_purge, LARCH,
     LMUF},
    {"Commands", "secure_who", &tp_secure_who, LARCH, LMUF},
    {"Commands", "who_doing", &tp_who_doing, LARCH, LMUF},
    {"Building", "realms_control", &tp_realms_control, WBOY, LMUF},
#ifdef CONTROLS_SUPPORT
    {"Building", "wiz_realms", &tp_wiz_realms, WBOY, LMUF},
#endif
    {"Building", "allow_listeners", &tp_listeners, LARCH, LMUF},
    {"Building", "allow_listeners_obj", &tp_listeners_obj, LARCH, LMUF},
    {"Building", "allow_listeners_env", &tp_listeners_env, LARCH, LMUF},
    {"Building", "allow_zombies", &tp_zombies, LARCH, LMUF},
    {"Building", "wiz_vehicles", &tp_wiz_vehicles, LARCH, LMUF},
    {"Building", "wiz_puppets", &tp_wiz_puppets, LARCH, LMUF},
    {"Pcreate", "wiz_name", &tp_wiz_name, LARCH, LMUF},
    {"Pcreate", "recycle_frobs", &tp_recycle_frobs, WBOY, LMUF},
    {"MUF", "m1_name_notify", &tp_m1_name_notify, LARCH, LMUF},
    {"Commands", "teleport_to_player", &tp_teleport_to_player, LARCH, LMUF},
    {"Commands", "secure_teleport", &tp_secure_teleport, LARCH, LMUF},
    {"Building", "exit_darking", &tp_exit_darking, LARCH, LMUF},
    {"Building", "thing_darking", &tp_thing_darking, LARCH, LMUF},
    {"Building", "dark_sleepers", &tp_dark_sleepers, LARCH, LMUF},
    {"Commands", "who_hides_dark", &tp_who_hides_dark, LARCH, LMAGE},
    {"System", "old_priorities", &tp_compatible_priorities, LARCH, LMUF},
    {"MPI", "do_mpi_parsing", &tp_do_mpi_parsing, LARCH, LMUF},
    {"Props", "look_propqueues", &tp_look_propqueues, LARCH, LMUF},
    {"Props", "dump_propqueues", &tp_dump_propqueues, LARCH, LMUF},
    {"Props", "lock_envcheck", &tp_lock_envcheck, LARCH, LMUF},
    {"Database", "diskbase_propvals", &tp_diskbase_propvals, WBOY, LMAGE},
    {"Idletime", "idleboot", &tp_idleboot, LARCH, LMUF},
    {"Playermax", "playermax", &tp_playermax, LARCH, LMUF},
    {"Building", "db_readonly", &tp_db_readonly, WBOY, LMUF},
    {"Building", "building", &tp_building, LARCH, LMUF},
    {"Building", "all_can_build_rooms", &tp_all_can_build_rooms, LARCH, LMUF},
    {"Building", "restricted_building", &tp_restricted_building, LARCH, LMUF},
    {"Pcreate", "pcreate_copy_props", &tp_pcreate_copy_props, LARCH, LMUF},
    {"Commands", "allow_home", &tp_enable_home, LARCH, LMUF},
    {"Idletime", "enable_idle_msgs", &tp_enable_idle_msgs, LARCH, LMUF},
    {"Idletime", "user_idle_propqueue", &tp_user_idle_propqueue, LARCH, LMUF},
    {"Commands", "use_self_on_command", &tp_use_self_on_command, LARCH, LMUF},
    {"Commands", "quiet_moves", &tp_quiet_moves, LARCH, LMUF},
    {"Commands", "quiet_connects", &tp_quiet_connects, LARCH, LMUF},
    {"Props", "proplist_int_counter", &tp_proplist_int_counter, LARCH, LMUF},
#ifdef MCP_SUPPORT
    {"System", "enable_mcp", &tp_enable_mcp, WBOY, LMUF},
#endif
    {"Props", "enable_commandprops", &tp_enable_commandprops, WBOY, LMUF},
    {"MUF", "old_parseprop", &tp_old_parseprop, WBOY, LMUF},
    {"MPI", "mpi_needflag", &tp_mpi_needflag, WBOY, LMUF},
    {"Guests", "guest_needflag", &tp_guest_needflag, WBOY, LMUF},
    {"Commands", "mortalwho", &tp_mortalwho, LARCH, LMUF},
    {"System", "fb_controls", &tp_fb_controls, LBOY, LMUF},
    {"Props", "allow_old_trigs", &tp_allow_old_trigs, LARCH, LMUF},
    {"System", "multi_wizlevels", &tp_multi_wizlevels, LBOY, LMUF},
    {"Database", "auto_archive", &tp_auto_archive, LBOY, LMAGE},
    {"MUF", "optimize_muf", &tp_optimize_muf, LBOY, LMAGE},
    {"MUF", "compatible_muf_perms", &tp_compatible_muf_perms, LBOY, LMAGE},
    {"Idletime", "allow_unidle", &tp_allow_unidle, LARCH, LMUF},
    {"Math", "alt_infinity_handler", &tp_alt_infinity_handler, LARCH, LMUF},
    {"MPI", "autolinking", &tp_autolinking, LARCH, LMUF},
/*    {"MPI", "require_has_mpi_arg", &tp_require_has_mpi_arg, LARCH, LMUF}, */
#ifdef NEWHTTPD
    {"HTTPD", "web_allow_players", &tp_web_allow_players, LARCH, LMUF},
    {"HTTPD", "web_allow_playerhtmuf", &tp_web_allow_playerhtmuf, LARCH, LMUF},
    {"HTTPD", "web_allow_htmuf", &tp_web_allow_htmuf, LARCH, LMUF},
    {"HTTPD", "web_allow_vhosts", &tp_web_allow_vhosts, LARCH, LMUF},
    {"HTTPD", "web_allow_files", &tp_web_allow_files, LARCH, LMUF},
    {"HTTPD", "web_allow_dirlist", &tp_web_allow_dirlist, LARCH, LMUF},
    {"HTTPD", "web_allow_mpi", &tp_web_allow_mpi, LARCH, LMUF},
#endif
    {NULL, NULL, NULL, 0, 0}
};

stk_array *
tune_parms_array(const char *pattern, int mlev)
{
    struct tune_str_entry *tstr = tune_str_list;
    struct tune_time_entry *ttim = tune_time_list;
    struct tune_val_entry *tval = tune_val_list;
    struct tune_ref_entry *tref = tune_ref_list;
    struct tune_bool_entry *tbool = tune_bool_list;
    stk_array *nu = new_array_packed(0);
    struct inst temp1;
    char pat[BUFFER_LEN];
    char buf[BUFFER_LEN];
    int i = 0;

    strcpy(pat, pattern);
    while (tbool->name) {
        if (tbool->readmlev <= mlev) {
            strcpy(buf, tbool->name);
            if (!*pattern || equalstr(pat, buf)) {
                stk_array *item = new_array_dictionary();

                array_set_strkey_strval(&item, "type", "boolean");
                array_set_strkey_strval(&item, "group", tbool->group);
                array_set_strkey_strval(&item, "name", tbool->name);
                array_set_strkey_intval(&item, "value", *tbool->boolv ? 1 : 0);
                array_set_strkey_intval(&item, "readmlev", tbool->readmlev);
                array_set_strkey_intval(&item, "writemlev", tbool->writemlev);
                temp1.type = PROG_ARRAY;
                temp1.data.array = item;
                array_set_intkey(&nu, i++, &temp1);
                CLEAR(&temp1);
            }
        }
        tbool++;
    }

    while (ttim->name) {
        if (ttim->readmlev <= mlev) {
            strcpy(buf, ttim->name);
            if (!*pattern || equalstr(pat, buf)) {
                stk_array *item = new_array_dictionary();

                array_set_strkey_strval(&item, "type", "timespan");
                array_set_strkey_strval(&item, "group", ttim->group);
                array_set_strkey_strval(&item, "name", ttim->name);
                array_set_strkey_intval(&item, "value", *ttim->tim);
                array_set_strkey_intval(&item, "readmlev", ttim->readmlev);
                array_set_strkey_intval(&item, "writemlev", ttim->writemlev);
                temp1.type = PROG_ARRAY;
                temp1.data.array = item;
                array_set_intkey(&nu, i++, &temp1);
                CLEAR(&temp1);
            }
        }
        ttim++;
    }

    while (tval->name) {
        if (tval->readmlev <= mlev) {
            strcpy(buf, tval->name);
            if (!*pattern || equalstr(pat, buf)) {
                stk_array *item = new_array_dictionary();

                array_set_strkey_strval(&item, "type", "integer");
                array_set_strkey_strval(&item, "group", tval->group);
                array_set_strkey_strval(&item, "name", tval->name);
                array_set_strkey_intval(&item, "value", *tval->val);
                array_set_strkey_intval(&item, "readmlev", tval->readmlev);
                array_set_strkey_intval(&item, "writemlev", tval->writemlev);
                temp1.type = PROG_ARRAY;
                temp1.data.array = item;
                array_set_intkey(&nu, i++, &temp1);
                CLEAR(&temp1);
            }
        }
        tval++;
    }

    while (tref->name) {
        if (tref->readmlev <= mlev) {
            strcpy(buf, tref->name);
            if (!*pattern || equalstr(pat, buf)) {
                stk_array *item = new_array_dictionary();

                array_set_strkey_strval(&item, "type", "dbref");
                array_set_strkey_strval(&item, "group", tref->group);
                array_set_strkey_strval(&item, "name", tref->name);
                array_set_strkey_refval(&item, "value", *tref->ref);
                array_set_strkey_intval(&item, "readmlev", tref->readmlev);
                array_set_strkey_intval(&item, "writemlev", tref->writemlev);
                switch (tref->typ) {
                    case NOTYPE:
                        array_set_strkey_strval(&item, "objtype", "any");
                        break;
                    case TYPE_PLAYER:
                        array_set_strkey_strval(&item, "objtype", "player");
                        break;
                    case TYPE_THING:
                        array_set_strkey_strval(&item, "objtype", "thing");
                        break;
                    case TYPE_ROOM:
                        array_set_strkey_strval(&item, "objtype", "room");
                        break;
                    case TYPE_EXIT:
                        array_set_strkey_strval(&item, "objtype", "exit");
                        break;
                    case TYPE_PROGRAM:
                        array_set_strkey_strval(&item, "objtype", "program");
                        break;
                    case TYPE_GARBAGE:
                        array_set_strkey_strval(&item, "objtype", "garbage");
                        break;
                    default:
                        array_set_strkey_strval(&item, "objtype", "unknown");
                        break;
                }
                temp1.type = PROG_ARRAY;
                temp1.data.array = item;
                array_set_intkey(&nu, i++, &temp1);
                CLEAR(&temp1);
            }
        }
        tref++;
    }

    while (tstr->name) {
        if (tstr->readmlev <= mlev) {
            strcpy(buf, tstr->name);
            if (!*pattern || equalstr(pat, buf)) {
                stk_array *item = new_array_dictionary();

                array_set_strkey_strval(&item, "type", "string");
                array_set_strkey_strval(&item, "group", tstr->group);
                array_set_strkey_strval(&item, "name", tstr->name);
                array_set_strkey_strval(&item, "value", *tstr->str);
                array_set_strkey_intval(&item, "readmlev", tstr->readmlev);
                array_set_strkey_intval(&item, "writemlev", tstr->writemlev);
                temp1.type = PROG_ARRAY;
                temp1.data.array = item;
                array_set_intkey(&nu, i++, &temp1);
                CLEAR(&temp1);
            }
        }
        tstr++;
    }
    return nu;
}


static const char *
timestr_full(time_t dtime)
{
    static char buf[32];
    int days, hours, minutes;

    days = dtime / 86400;
    dtime %= 86400;
    hours = dtime / 3600;
    dtime %= 3600;
    minutes = dtime / 60;
    dtime %= 60;

    sprintf(buf, "%3dd %2d:%02d:%02d", days, hours, minutes, (int) dtime);

    return buf;
}


int
tune_count_parms(void)
{
    int total = 0;
    struct tune_str_entry *tstr = tune_str_list;
    struct tune_time_entry *ttim = tune_time_list;
    struct tune_val_entry *tval = tune_val_list;
    struct tune_ref_entry *tref = tune_ref_list;
    struct tune_bool_entry *tbool = tune_bool_list;

    while ((tstr++)->name)
        total++;
    while ((ttim++)->name)
        total++;
    while ((tval++)->name)
        total++;
    while ((tref++)->name)
        total++;
    while ((tbool++)->name)
        total++;

    return total;
}

void
tune_show_strings(dbref player, char *name)
{
    int total = 0;
    const char *lastname = NULL;
    char buf[BUFFER_LEN + 50], tbuf[BUFFER_LEN];
    struct tune_str_entry *tstr = tune_str_list;

    while (tstr->name) {
        strcpy(buf, tstr->name);
        if (MLevel(OWNER(player)) >= tstr->readmlev) {
            sprintf(buf, SYSCYAN "(str)  " SYSRED "%c" SYSGREEN "%-24s"
                    SYSRED " = " SYSCYAN "%.4096s",
                    (WLevel(OWNER(player)) >= tstr->writemlev) ? ' ' : '-',
                    tstr->name, tct(*tstr->str, tbuf));
            lastname = tstr->name;
            anotify_nolisten2(player, buf);
            total++;
        }
        tstr++;
    }
    anotify_fmt(player, CINFO "%d string-parm%s listed.", total,
                (total == 1) ? "" : "s");
}

void
tune_show_times(dbref player, char *name)
{
    int total = 0;
    const char *lastname = NULL;
    char buf[BUFFER_LEN + 50];
    struct tune_time_entry *ttim = tune_time_list;

    while (ttim->name) {
        strcpy(buf, ttim->name);
        if (MLevel(OWNER(player)) >= ttim->readmlev) {
            sprintf(buf, SYSPURPLE "(time) " SYSRED "%c" SYSGREEN "%-24s"
                    SYSRED " = " SYSPURPLE "%s",
                    (WLevel(OWNER(player)) >= ttim->writemlev) ? ' ' : '-',
                    ttim->name, timestr_full(*ttim->tim));
            lastname = ttim->name;
            anotify_nolisten2(player, buf);
            total++;
        }
        ttim++;
    }
    anotify_fmt(player, CINFO "%d time-parm%s listed.", total,
                (total == 1) ? "" : "s");
}

void
tune_show_vals(dbref player, char *name)
{
    int total = 0;
    const char *lastname = NULL;
    char buf[BUFFER_LEN + 50];
    struct tune_val_entry *tval = tune_val_list;

    while (tval->name) {
        strcpy(buf, tval->name);
        if (MLevel(OWNER(player)) >= tval->readmlev) {
            sprintf(buf, SYSGREEN "(int)  " SYSRED "%c" SYSGREEN "%-24s"
                    SYSRED " = " SYSYELLOW "%d",
                    (WLevel(OWNER(player)) >= tval->writemlev) ? ' ' : '-',
                    tval->name, *tval->val);
            lastname = tval->name;
            anotify_nolisten2(player, buf);
            total++;
        }
        tval++;
    }
    anotify_fmt(player, CINFO "%d int-parm%s listed.", total,
                (total == 1) ? "" : "s");
}

void
tune_show_refs(dbref player, char *name)
{
    int total = 0;
    const char *lastname = NULL;
    char buf[BUFFER_LEN + 50];
    struct tune_ref_entry *tref = tune_ref_list;

    while (tref->name) {
        strcpy(buf, tref->name);
        if (MLevel(OWNER(player)) >= tref->readmlev) {
            sprintf(buf, SYSYELLOW "(ref)  " SYSRED "%c" SYSGREEN "%-24s"
                    SYSRED " = %s",
                    (WLevel(OWNER(player)) >= tref->writemlev) ? ' ' : '-',
                    tref->name, ansi_unparse_object(player, *tref->ref));
            lastname = tref->name;
            anotify_nolisten2(player, buf);
            total++;
        }
        tref++;
    }
    anotify_fmt(player, CINFO "%d ref-parm%s listed.", total,
                (total == 1) ? "" : "s");
}

void
tune_show_bool(dbref player, char *name)
{
    int total = 0;
    const char *lastname = NULL;
    char buf[BUFFER_LEN + 50];
    struct tune_bool_entry *tbool = tune_bool_list;

    while (tbool->name) {
        strcpy(buf, tbool->name);
        if (MLevel(OWNER(player)) >= tbool->readmlev) {
            sprintf(buf, SYSWHITE "(bool) " SYSRED "%c" SYSGREEN "%-24s"
                    SYSRED " = " SYSBLUE "%s",
                    (WLevel(OWNER(player)) >= tbool->writemlev) ? ' ' : '-',
                    tbool->name, ((*tbool->boolv) ? "yes" : "no"));
            lastname = tbool->name;
            anotify_nolisten2(player, buf);
            total++;
        }
        tbool++;
    }
    anotify_fmt(player, CINFO "%d bool-parm%s listed.", total,
                (total == 1) ? "" : "s");
}

void
tune_display_parms(dbref player, char *name)
{
    int total = 0;
    const char *lastname = NULL;
    char buf[BUFFER_LEN + 50], tbuf[BUFFER_LEN];
    struct tune_str_entry *tstr = tune_str_list;
    struct tune_time_entry *ttim = tune_time_list;
    struct tune_val_entry *tval = tune_val_list;
    struct tune_ref_entry *tref = tune_ref_list;
    struct tune_bool_entry *tbool = tune_bool_list;

    while (tstr->name) {
        strcpy(buf, tstr->name);
        if ((MLevel(OWNER(player)) >= tstr->readmlev) &&
            (!*name || equalstr(name, buf))) {
            sprintf(buf, SYSCYAN "(str)  " SYSRED "%c" SYSGREEN "%-24s"
                    SYSRED " = " SYSCYAN "%.4096s",
                    (WLevel(OWNER(player)) >= tstr->writemlev) ? ' ' : '-',
                    tstr->name, tct(*tstr->str, tbuf));
            lastname = tstr->name;
            anotify_nolisten2(player, buf);
            total++;
        }
        tstr++;
    }

    while (ttim->name) {
        strcpy(buf, ttim->name);
        if ((MLevel(OWNER(player)) >= ttim->readmlev) &&
            (!*name || equalstr(name, buf))) {
            sprintf(buf, SYSPURPLE "(time) " SYSRED "%c" SYSGREEN "%-24s"
                    SYSRED " = " SYSPURPLE "%s",
                    (WLevel(OWNER(player)) >= ttim->writemlev) ? ' ' : '-',
                    ttim->name, timestr_full(*ttim->tim));
            lastname = ttim->name;
            anotify_nolisten2(player, buf);
            total++;
        }
        ttim++;
    }

    while (tval->name) {
        strcpy(buf, tval->name);
        if ((MLevel(OWNER(player)) >= tval->readmlev) &&
            (!*name || equalstr(name, buf))) {
            sprintf(buf, SYSGREEN "(int)  " SYSRED "%c" SYSGREEN "%-24s"
                    SYSRED " = " SYSYELLOW "%d",
                    (WLevel(OWNER(player)) >= tval->writemlev) ? ' ' : '-',
                    tval->name, *tval->val);
            lastname = tval->name;
            anotify_nolisten2(player, buf);
            total++;
        }
        tval++;
    }
    while (tref->name) {
        strcpy(buf, tref->name);
        if ((MLevel(OWNER(player)) >= tref->readmlev) &&
            (!*name || equalstr(name, buf))) {
            sprintf(buf, SYSYELLOW "(ref)  " SYSRED "%c" SYSGREEN "%-24s"
                    SYSRED " = %s",
                    (WLevel(OWNER(player)) >= tref->writemlev) ? ' ' : '-',
                    tref->name, ansi_unparse_object(player, *tref->ref));
            lastname = tref->name;
            anotify_nolisten2(player, buf);
            total++;
        }
        tref++;
    }

    while (tbool->name) {
        strcpy(buf, tbool->name);
        if ((MLevel(OWNER(player)) >= tbool->readmlev) &&
            (!*name || equalstr(name, buf))) {
            sprintf(buf, SYSWHITE "(bool) " SYSRED "%c" SYSGREEN "%-24s" SYSRED
                    " = " SYSBLUE "%s",
                    (WLevel(OWNER(player)) >= tbool->writemlev) ? ' ' : '-',
                    tbool->name, ((*tbool->boolv) ? "yes" : "no"));
            lastname = tbool->name;
            anotify_nolisten2(player, buf);
            total++;
        }
        tbool++;
    }
    if ((total == 1) && lastname && *lastname) {
        do_sysparm(player, lastname);
    } else {
        anotify_fmt(player, CINFO "%d sysparm%s listed.", total,
                    (total == 1) ? "" : "s");
        anotify(player,
                SYSYELLOW
                "@tune str, time, int, ref, or bool to list by data types.");
    }
}


void
tune_save_parms_to_file(FILE * f)
{
    struct tune_str_entry *tstr = tune_str_list;
    struct tune_time_entry *ttim = tune_time_list;
    struct tune_val_entry *tval = tune_val_list;
    struct tune_ref_entry *tref = tune_ref_list;
    struct tune_bool_entry *tbool = tune_bool_list;

    while (tstr->name) {
        fprintf(f, "%s=%.4096s\n", tstr->name, *tstr->str);
        tstr++;
    }

    while (ttim->name) {
        fprintf(f, "%s=%s\n", ttim->name, timestr_full(*ttim->tim));
        ttim++;
    }

    while (tval->name) {
        fprintf(f, "%s=%d\n", tval->name, *tval->val);
        tval++;
    }

    while (tref->name) {
        fprintf(f, "%s=#%d\n", tref->name, *tref->ref);
        tref++;
    }

    while (tbool->name) {
        fprintf(f, "%s=%s\n", tbool->name, (*tbool->boolv) ? "yes" : "no");
        tbool++;
    }
}

void
tune_save_parmsfile()
{
    FILE *f;

    f = fopen(PARMFILE_NAME, "w");
    if (!f) {
        log_status_nowall("TUNE: Couldn't open %s\n", PARMFILE_NAME);
        return;
    }

    tune_save_parms_to_file(f);

    fclose(f);
}



const char *
tune_get_parmstring(const char *name, int mlev)
{
    static char buf[BUFFER_LEN + 50];
    struct tune_str_entry *tstr = tune_str_list;
    struct tune_time_entry *ttim = tune_time_list;
    struct tune_val_entry *tval = tune_val_list;
    struct tune_ref_entry *tref = tune_ref_list;
    struct tune_bool_entry *tbool = tune_bool_list;

    while (tstr->name) {
        if (!string_compare(name, tstr->name)) {
            if (tstr->readmlev > mlev)
                return "";
            return (*tstr->str);
        }
        tstr++;
    }

    while (ttim->name) {
        if (!string_compare(name, ttim->name)) {
            if (ttim->readmlev > mlev)
                return "";
            sprintf(buf, "%d", (int) *ttim->tim);
            return (buf);
        }
        ttim++;
    }

    while (tval->name) {
        if (!string_compare(name, tval->name)) {
            if (tval->readmlev > mlev)
                return "";
            sprintf(buf, "%d", *tval->val);
            return (buf);
        }
        tval++;
    }

    while (tref->name) {
        if (!string_compare(name, tref->name)) {
            if (tref->readmlev > mlev)
                return "";
            sprintf(buf, "#%d", *tref->ref);
            return (buf);
        }
        tref++;
    }

    while (tbool->name) {
        if (!string_compare(name, tbool->name)) {
            if (tbool->readmlev > mlev)
                return "";
            sprintf(buf, "%s", ((*tbool->boolv) ? "yes" : "no"));
            return (buf);
        }
        tbool++;
    }

    /* Parameter was not found.  Return null string. */
    strcpy(buf, "");
    return (buf);
}



/* return values:
 *  0 for success.
 *  1 for unrecognized parameter name.
 *  2 for bad parameter value syntax.
 *  3 for bad parameter value.
 */

#define TUNESET_SUCCESS 0
#define TUNESET_UNKNOWN 1
#define TUNESET_SYNTAX 2
#define TUNESET_BADVAL 3
#define TUNESET_NOPERM 4

/* Added in beta5 to help with shutdown memory profiling. From fb6 */
void
tune_freeparms(void)
{
    struct tune_str_entry *tstr = tune_str_list;

    while (tstr->name) {
        if (!tstr->isdefault) {
            free((char *) *tstr->str);
        }
        tstr++;
    }
}




int
tune_setparm(const dbref player, const char *parmname, const char *val)
{
    struct tune_str_entry *tstr = tune_str_list;
    struct tune_time_entry *ttim = tune_time_list;
    struct tune_val_entry *tval = tune_val_list;
    struct tune_ref_entry *tref = tune_ref_list;
    struct tune_bool_entry *tbool = tune_bool_list;
    char buf[BUFFER_LEN];
    char *parmval;

    strcpy(buf, val);
    parmval = buf;

    while (tstr->name) {
        if (!string_compare(parmname, tstr->name)) {
            if ((player != NOTHING)
                && (WLevel(OWNER(player)) < tstr->writemlev))
                return TUNESET_NOPERM;
            if (!tstr->isdefault)
                free((char *) *tstr->str);
            if (*parmval == '-')
                parmval++;
            *tstr->str = string_dup(parmval);
            tstr->isdefault = 0;
            return 0;
        }
        tstr++;
    }

    while (ttim->name) {
        if (!string_compare(parmname, ttim->name)) {
            int days, hrs, mins, secs, result;
            char *end;

            if ((player != NOTHING)
                && (WLevel(OWNER(player)) < ttim->writemlev))
                return TUNESET_NOPERM;
            days = hrs = mins = secs = 0;
            end = parmval + strlen(parmval) - 1;
            switch (*end) {
                case 's':
                case 'S':
                    *end = '\0';
                    if (!number(parmval))
                        return 2;
                    secs = atoi(parmval);
                    break;
                case 'm':
                case 'M':
                    *end = '\0';
                    if (!number(parmval))
                        return 2;
                    mins = atoi(parmval);
                    break;
                case 'h':
                case 'H':
                    *end = '\0';
                    if (!number(parmval))
                        return 2;
                    hrs = atoi(parmval);
                    break;
                case 'd':
                case 'D':
                    *end = '\0';
                    if (!number(parmval))
                        return 2;
                    days = atoi(parmval);
                    break;
                default:
                    result = sscanf(parmval, "%dd %2d:%2d:%2d",
                                    &days, &hrs, &mins, &secs);
                    if (result != 4)
                        return 2;
                    break;
            }
            *ttim->tim = (days * 86400) + (3600 * hrs) + (60 * mins) + secs;
            return 0;
        }
        ttim++;
    }

    while (tval->name) {
        if (!string_compare(parmname, tval->name)) {
            if ((player != NOTHING)
                && (WLevel(OWNER(player)) < tval->writemlev))
                return TUNESET_NOPERM;
            if (!number(parmval))
                return 2;
            *tval->val = atoi(parmval);
            return 0;
        }
        tval++;
    }

    while (tref->name) {
        if (!string_compare(parmname, tref->name)) {
            dbref obj;

            if ((player != NOTHING)
                && (WLevel(OWNER(player)) < tref->writemlev))
                return TUNESET_NOPERM;
            if (!strcmp(parmval, "me"))
                obj = player;
#ifndef SANITY
            else if (*parmval == '*')
                obj = lookup_player(parmval + 1);
#endif
            else {
                if (*parmval != NUMBER_TOKEN)
                    return 2;
                if (!number(parmval + 1))
                    return 2;
                obj = (dbref) atoi(parmval + 1);
            }
            if (obj < -1 || obj >= db_top)
                return 2;
            if (obj != -1 && tref->typ != NOTYPE && Typeof(obj) != tref->typ)
                return 3;
	    if ((!string_compare(parmname, "player_start") ||
	         !string_compare(parmname, "default_parent")) &&
		 obj == -1)
		obj = 0;
            *tref->ref = obj;
            return 0;
        }
        tref++;
    }

    while (tbool->name) {
        if (!string_compare(parmname, tbool->name)) {
            if ((player != NOTHING)
                && (WLevel(OWNER(player)) < tbool->writemlev))
                return TUNESET_NOPERM;
            if (*parmval == 'y' || *parmval == 'Y') {
                *tbool->boolv = 1;
            } else if (*parmval == 'n' || *parmval == 'N') {
                *tbool->boolv = 0;
            } else {
                return 2;
            }
            return 0;
        }
        tbool++;
    }

    return 1;
}

void
tune_load_parms_from_file(FILE * f, dbref player, int cnt)
{
    char buf[BUFFER_LEN];
    char *c, *p;
    int result = 0;

    while (!feof(f) && (cnt < 0 || cnt--)) {
        fgets(buf, sizeof(buf), f);
        if (*buf != NUMBER_TOKEN) {
            p = c = index(buf, '=');
            if (c) {
                *c++ = '\0';
                while (p > buf && isspace(*(--p)))
                    *p = '\0';
                while (isspace(*c))
                    c++;
                for (p = c; *p && *p != '\n'; p++) ;
                *p = '\0';
                for (p = buf; isspace(*p); p++) ;
                if (*p) {
                    result = tune_setparm(player, p, c);
                }
                switch (result) {
                    case TUNESET_SUCCESS:
                        strcat(p, ": Parameter set.");
                        break;
                    case TUNESET_UNKNOWN:
                        strcat(p, ": Unknown parameter.");
                        break;
                    case TUNESET_SYNTAX:
                        strcat(p, ": Bad parameter syntax.");
                        break;
                    case TUNESET_BADVAL:
                        strcat(p, ": Bad parameter value.");
                        break;
                }
                if (result && player != NOTHING)
                    notify(player, p);
            }
        }
    }
}

void
tune_load_parmsfile(dbref player)
{
    FILE *f;

    f = fopen(PARMFILE_NAME, "r");
    if (!f) {
        log_status_nowall("TUNE: Couldn't open %s\n", PARMFILE_NAME);
        return;
    }

    tune_load_parms_from_file(f, player, -1);

    fclose(f);
}


void
do_tune(dbref player, char *parmname, char *parmval)
{
    int result;

    if (!Mage(player)) {
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
        return;
    }

    if (!Arch(player) && *parmval) {
        anotify_fmt(player, CFAIL "%s", tp_noperm_mesg);
        return;
    }

    if (*parmname && *parmval) {
        result = tune_setparm(player, parmname, parmval);
        switch (result) {
            case TUNESET_SUCCESS:
                log_status("TUNE: %s(%d) tuned %s to %s\n",
                           NAME(player), player, parmname, parmval);
                anotify_nolisten2(player, CSUCC "Parameter set.");
                tune_display_parms(player, parmname);
                break;
            case TUNESET_UNKNOWN:
                anotify_nolisten2(player, CINFO "Unknown parameter.");
                break;
            case TUNESET_SYNTAX:
                anotify_nolisten2(player, CFAIL "Bad parameter syntax.");
                break;
            case TUNESET_BADVAL:
                anotify_nolisten2(player, CFAIL "Bad parameter value.");
                break;
            case TUNESET_NOPERM:
                anotify_nolisten2(player, CFAIL "Permission denied.");
        }
        return;
    } else if (*parmname) {
        /* if (!string_compare(parmname, "save")) {
           tune_save_parmsfile();
           anotify_nolisten2(player, CSUCC "Saved parameters to configuration file.");
           } else if (!string_compare(parmname, "load")) {
           tune_load_parmsfile(player);
           anotify_nolisten2(player, CSUCC "Restored parameters from configuration file.");
           } else
         */
        if (!string_compare(parmname, "strings") ||
            !string_compare(parmname, "str"))
            tune_show_strings(player, parmname);
        else if (!string_compare(parmname, "times") ||
                 !string_compare(parmname, "time"))
            tune_show_times(player, parmname);
        else if (!string_compare(parmname, "int") ||
                 !string_compare(parmname, "ints"))
            tune_show_vals(player, parmname);
        else if (!string_compare(parmname, "ref") ||
                 !string_compare(parmname, "refs") ||
                 !string_compare(parmname, "dbrefs") ||
                 !string_compare(parmname, "dbref"))
            tune_show_refs(player, parmname);
        else if (!string_compare(parmname, "bool") ||
                 !string_compare(parmname, "bools"))
            tune_show_bool(player, parmname);
        else
            tune_display_parms(player, parmname);
        return;
    } else if (!*parmval && !*parmname) {
        tune_display_parms(player, parmname);
    } else {
        anotify_nolisten2(player, CINFO "Tune what?");
        return;
    }
}
