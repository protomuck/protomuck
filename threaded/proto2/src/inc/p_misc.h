extern void prim_time(PRIM_PROTOTYPE);
extern void prim_date(PRIM_PROTOTYPE);
extern void prim_gmtoffset(PRIM_PROTOTYPE);
extern void prim_systime(PRIM_PROTOTYPE);
extern void prim_timesplit(PRIM_PROTOTYPE);
extern void prim_timefmt(PRIM_PROTOTYPE);
extern void prim_queue(PRIM_PROTOTYPE);
extern void prim_enqueue(PRIM_PROTOTYPE);
extern void prim_kill(PRIM_PROTOTYPE);
extern void prim_timestamps(PRIM_PROTOTYPE);
extern void prim_refstamps(PRIM_PROTOTYPE);
extern void prim_fork(PRIM_PROTOTYPE);
extern void prim_pid(PRIM_PROTOTYPE);
extern void prim_stats(PRIM_PROTOTYPE);
extern void prim_abort(PRIM_PROTOTYPE);
extern void prim_ispidp(PRIM_PROTOTYPE);
extern void prim_parselock(PRIM_PROTOTYPE);
extern void prim_unparselock(PRIM_PROTOTYPE);
extern void prim_prettylock(PRIM_PROTOTYPE);
extern void prim_cancallp(PRIM_PROTOTYPE);
extern void prim_timer_start(PRIM_PROTOTYPE);
extern void prim_timer_stop(PRIM_PROTOTYPE);
extern void prim_event_count(PRIM_PROTOTYPE);
extern void prim_event_exists(PRIM_PROTOTYPE);
extern void prim_event_send(PRIM_PROTOTYPE);
extern void prim_pnameokp(PRIM_PROTOTYPE);
extern void prim_nameokp(PRIM_PROTOTYPE);
extern void prim_watchpid(PRIM_PROTOTYPE);
extern void prim_getpids(PRIM_PROTOTYPE);
extern void prim_getpidinfo(PRIM_PROTOTYPE);
extern void prim_read_wants_blanks(PRIM_PROTOTYPE);
extern void prim_get_read_wants_blanks(PRIM_PROTOTYPE);
extern void prim_debugger_break(PRIM_PROTOTYPE);
extern void prim_debug_on(PRIM_PROTOTYPE);
extern void prim_debug_off(PRIM_PROTOTYPE);
extern void prim_debug_line(PRIM_PROTOTYPE);
extern void prim_systime_precise(PRIM_PROTOTYPE);
extern void prim_htoi(PRIM_PROTOTYPE);
extern void prim_itoh(PRIM_PROTOTYPE);
extern void prim_onevent(PRIM_PROTOTYPE);
extern void prim_interrupt_level(PRIM_PROTOTYPE);
extern void prim_touch(PRIM_PROTOTYPE);
extern void prim_use(PRIM_PROTOTYPE);
extern void prim_MD5hash(PRIM_PROTOTYPE);
extern void prim_SHA1hash(PRIM_PROTOTYPE);

/* From p_html.c */
extern void prim_commandtext(PRIM_PROTOTYPE);
extern void prim_stopmidi(PRIM_PROTOTYPE);
extern void prim_playmidi(PRIM_PROTOTYPE);
extern void prim_unescape_url(PRIM_PROTOTYPE);
extern void prim_escape_url(PRIM_PROTOTYPE);

#define PRIMS_MISC_FUNCS prim_time, prim_date, prim_gmtoffset,            \
    prim_systime, prim_timesplit, prim_timefmt, prim_queue, prim_kill,    \
    prim_timestamps, prim_fork, prim_pid, prim_stats, prim_abort,         \
    prim_ispidp, prim_parselock, prim_unparselock, prim_prettylock,       \
    prim_commandtext, prim_playmidi, prim_stopmidi, prim_cancallp,        \
    prim_timer_start, prim_timer_stop, prim_event_count, prim_event_send, \
    prim_pnameokp, prim_nameokp, prim_event_exists, prim_watchpid,        \
    prim_getpids, prim_getpidinfo, prim_read_wants_blanks,                \
    prim_debugger_break, prim_debug_on, prim_debug_off, prim_debug_line,  \
    prim_systime_precise, prim_htoi, prim_itoh, prim_unescape_url,        \
    prim_escape_url, prim_onevent, prim_interrupt_level, prim_refstamps,  \
    prim_touch, prim_use, prim_MD5hash, prim_SHA1hash, prim_enqueue,      \
    prim_get_read_wants_blanks

#define PRIMS_MISC_NAMES "TIME", "DATE", "GMTOFFSET", "SYSTIME",          \
    "TIMESPLIT", "TIMEFMT", "QUEUE", "KILL", "TIMESTAMPS", "FORK", "PID", \
    "STATS", "ABORT", "ISPID?", "PARSELOCK", "UNPARSELOCK", "PRETTYLOCK", \
    "COMMANDTEXT", "PLAYMIDI", "STOPMIDI", "CANCALL?", "TIMER_START",     \
    "TIMER_STOP", "EVENT_COUNT", "EVENT_SEND", "PNAME-OK?", "NAME-OK?",   \
    "EVENT_EXISTS", "WATCHPID", "GETPIDS", "GETPIDINFO",                  \
    "READ_WANTS_BLANKS", "DEBUGGER_BREAK", "DEBUG_ON", "DEBUG_OFF",       \
    "DEBUG_LINE", "SYSTIME_PRECISE", "HTOI", "ITOH", "UNESCAPE_URL",      \
    "ESCAPE_URL", "ONEVENT", "INTERRUPT_LEVEL", "REFSTAMPS",              \
    "TOUCH", "USE", "MD5HASH", "SHA1HASH", "ENQUEUE",                     \
	"GET_READ_WANTS_BLANKS"
   
#define PRIMLIST_MISC   { "TIME",                  LM1,     0, prim_time },               \
                        { "DATE",                  LM1,     0, prim_date },               \
                        { "GMTOFFSET",             LM1,     0, prim_gmtoffset },          \
                        { "SYSTIME",               LM1,     0, prim_systime },            \
                        { "TIMESPLIT",             LM1,     1, prim_timesplit },          \
                        { "TIMEFMT",               LM1,     2, prim_timefmt },            \
                        { "QUEUE",                 LM3,     3, prim_queue },              \
                        { "KILL",                  LM1,     1, prim_kill },               \
                        { "TIMESTAMPS",            LM2,     1, prim_timestamps },         \
                        { "FORK",                  LWIZ,    0, prim_fork },               \
                        { "PID",                   LM1,     0, prim_pid },                \
                        { "STATS",                 LM3,     1, prim_stats },              \
                        { "ABORT",                 LM1,     1, prim_abort },              \
                        { "ISPID?",                LM1,     1, prim_ispidp },             \
                        { "PARSELOCK",             LM1,     1, prim_parselock },          \
                        { "UNPARSELOCK",           LM1,     1, prim_unparselock },        \
                        { "PRETTYLOCK",            LM1,     1, prim_prettylock },         \
                        { "COMMANDTEXT",           LM1,     3, prim_commandtext },        \
                        { "PLAYMIDI",              LM1,     3, prim_playmidi },           \
                        { "STOPMIDI",              LM1,     1, prim_stopmidi },           \
                        { "CANCALL?",              LM1,     2, prim_cancallp },           \
                        { "TIMER_START",           LM1,     2, prim_timer_start },        \
                        { "TIMER_STOP",            LM1,     1, prim_timer_stop },         \
                        { "EVENT_COUNT",           LM1,     0, prim_event_count },        \
                        { "EVENT_SEND",            LM3,     3, prim_event_send },         \
                        { "PNAME-OK?",             LM1,     1, prim_pnameokp },           \
                        { "NAME-OK?",              LM1,     1, prim_nameokp },            \
                        { "EVENT_EXISTS",          LM1,     1, prim_event_exists },       \
                        { "WATCHPID",              LM3,     1, prim_watchpid },           \
                        { "GETPIDS",               LARCH,   1, prim_getpids },            \
                        { "GETPIDINFO",            LARCH,   1, prim_getpidinfo },         \
                        { "READ_WANTS_BLANKS",     LM1,     0, prim_read_wants_blanks },  \
                        { "DEBUGGER_BREAK",        LM1,     0, prim_debugger_break },     \
                        { "DEBUG_ON",              LM1,     0, prim_debug_on },           \
                        { "DEBUG_OFF",             LM1,     0, prim_debug_off },          \
                        { "DEBUG_LINE",            LM1,     0, prim_debug_line },         \
                        { "SYSTIME_PRECISE",       LM1,     0, prim_systime_precise },    \
                        { "HTOI",                  LM1,     1, prim_htoi },               \
                        { "ITOH",                  LM1,     1, prim_itoh },               \
                        { "UNESCAPE_URL",          LM1,     1, prim_unescape_url },       \
                        { "ESCAPE_URL",            LM1,     1, prim_escape_url },         \
                        { "ONEVENT",               LM1,     4, prim_onevent },            \
                        { "INTERRUPT_LEVEL",       LM1,     0, prim_interrupt_level },    \
                        { "REFSTAMPS",             LM2,     1, prim_refstamps },          \
                        { "TOUCH",                 LM3,     1, prim_touch },              \
                        { "USE",                   LWIZ,    1, prim_use },                \
                        { "MD3HASH",               LM1,     1, prim_MD5hash },            \
                        { "SHA1HASH",              LM1,     1, prim_SHA1hash },           \
                        { "ENQUEUE",               LARCH,   4, prim_enqueue },            \
                        { "GET_READ_WANTS_BLANKS", LM1,     0, prim_get_read_wants_blanks }
   

#define PRIMS_MISC_CNT 50
