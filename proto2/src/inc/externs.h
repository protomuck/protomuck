#include "copyright.h"

/*
 * Revision 1.14  2000/11/12 08:37:12  akari
 * Added force_level prim.
 * Added idle/unidle propqueues.
 * Added new parseprop.
 * Added {telldescr:} MPI function.
 * Added system-only props.
 * Added custom color settings. (This was added a long time ago,
 *    just now getting mentioned.)
 * Added custom 404 screen for webserver. Under _/www/http/404
 *    on the www_root room.
 * Removed @hopper support all together.
 * Finished command prop support.
 * Added @logincommand props to be put on #0 and be called from the
 *    login screen.
 * Added COMMAND flag to go along with command props. Only objects
 *    set with the COMMAND flag will be searched for command props.
 * If a player examines an exit linked to a MUF set VIEWABLE, then
 *    the dbref of the MUF program will be shown the player as well.
 * @tune user_idle_propqueue. Disables or enables _idle/ and _unidle/
 *    propqueues.
 * @tune enable_idle_msgs. Causes a room to be notified when a player
 *    reaches the point of being idle.
 * @tune idletime. The time at which the server considers a player idle
 *    for use with the idle propqueues and idle_msgs.
 *
 * Revision 1.3  1996/09/19 07:15:13  jtraub
 * removed do_rob and do_kill commands
 *
 * Revision 1.2  1996/09/19 05:49:55  jtraub
 * removed hackandslash in-server code
 *
 */
/* Definition of 'McpFrame' */

#ifdef MCP_SUPPORT
# include "mcp.h"
#endif
#include "dbsearch.h"

/* Prototypes for externs not defined elsewhere */

extern char match_args[];
extern char match_cmdname[];

/* From interface.c */
extern short verboseload;

/* From debugger.c */
extern int muf_debugger(int descr, dbref player, dbref program,
                        const char *text, struct frame *fr);
extern void muf_backtrace(dbref player, dbref program, int count,
                          struct frame *fr);
extern void list_proglines(dbref player, dbref program, struct frame *fr,
                           int start, int end);

/* From disassem.c */
extern void disassemble(dbref player, dbref program);

/* from event.c */
extern time_t next_muckevent_time(void);
extern void next_muckevent(void);
extern void dump_db_now(void);

#ifdef DELTADUMPS
extern void delta_dump_now(void);
#endif
extern time_t next_cron_time(void);
extern void check_cron_time(void);
extern time_t next_archive_time(void);
extern void check_archive_time();
extern int auto_archive_now(void);

/* from timequeue.c */
typedef struct timenode {
    struct timenode *next;
    int typ;
    int subtyp;
    time_t when;
    int descr;
    dbref called_prog;
    char *called_data;
    char *command;
    char *str3;
    dbref uid;
    dbref loc;
    dbref trig;
    struct frame *fr;
    struct inst *where;
    int eventnum;
} *timequeue;
extern stk_array *get_pids(dbref ref);
extern stk_array *get_pidinfo(int pid);
extern int scan_instances(dbref program);
extern void handle_read_event(int descr, dbref player, const char *command,
                              struct timenode *event);
extern int add_muf_read_event(int descr, dbref player, dbref prog,
                              struct frame *fr);
extern int add_muf_tread_event(int descr, dbref player, dbref prog,
                               struct frame *fr, int delay);
extern int add_muf_queue_event(int descr, dbref player, dbref loc, dbref trig,
                               dbref prog, const char *argstr,
                               const char *cmdstr, int listen_p);
extern int add_muf_delay_event(int delay, int descr, dbref player, dbref loc,
                               dbref trig, dbref prog, struct frame *fr,
                               const char *mode);
extern int add_muf_timer_event(int descr, dbref player, dbref prog,
                               struct frame *fr, int delay, char *id);
extern int add_muf_delayq_event(int delay, int descr, dbref player, dbref loc,
                                dbref trig, dbref prog, const char *argstr,
                                const char *cmdstr, int listen_p);
extern int add_mpi_event(int delay, int descr, dbref player, dbref loc,
                         dbref trig, const char *mpi, const char *cmdstr,
                         const char *argstr, int listen_p, int omesg_p);
extern int add_event(int event_type, int subtyp, int dtime, int descr,
                     dbref player, dbref loc, dbref trig, dbref program,
                     struct frame *fr, const char *strdata, const char *strcmd,
                     const char *str3);
extern void listenqueue(int descr, dbref player, dbref where, dbref trigger,
                 dbref what, dbref xclude, const char *propname,
                 const char *toparg, int mlev, int mt, int mpi_p);
extern void next_timequeue_event(void);
extern int dequeue_prog_descr(int descr, int sleeponly);
extern int in_timequeue(int pid);
extern void purge_timenode_free_pool();
extern struct frame *timequeue_pid_frame(int pid);
extern time_t next_event_time(void);
extern void list_events(dbref program);
extern int descr_running_queue(int descr);
extern int dequeue_prog(dbref program, int sleeponly);
extern int dequeue_process(int procnum);
extern int dequeue_timers(int pid, char *id);
extern void read_event_notify(int descr, dbref player);
extern int control_process(dbref player, int procnum);
extern void do_dequeue(int descr, dbref player, const char *arg1);
extern void propqueue(int descr, dbref player, dbref where, dbref trigger,
                      dbref what, dbref xclude, const char *propname,
                      const char *toparg, int mlev, int mt);
extern void envpropqueue(int descr, dbref player, dbref where, dbref trigger,
                         dbref what, dbref xclude, const char *propname,
                         const char *toparg, int mlev, int mt);
extern char *time_format_2(time_t dt);

/* From compress.c */
extern void init_compress_from_file(FILE * dicto);
extern void save_compress_words_to_file(FILE * f);

/* From create.c */
extern void do_action(int descr, dbref player, const char *action_name,
                      const char *source_name);
extern void do_attach(int descr, dbref player, const char *action_name,
                      const char *source_name);
extern void do_open(int descr, dbref player, const char *direction,
                    const char *linkto);
extern void do_link(int descr, dbref player, const char *name,
                    const char *room_name);
extern void do_dig(int descr, dbref player, const char *name,
                   const char *pname);
extern void do_create(dbref player, char *name, char *cost);
extern void do_prog(int descr, dbref player, const char *name);
#ifdef MCP_SUPPORT
extern void do_mcpprogram(int descr, dbref player, const char *name);
extern void do_mcpedit(int descr, dbref player, const char *name);
#endif
extern void do_edit(int descr, dbref player, const char *name);
extern int unset_source(dbref player, dbref loc, dbref action);
extern int _link_exit(int descr, dbref player, dbref exit, char *dest_name,
                               register dbref *dest_list, register bool dryrun);
#define link_exit(A, B, C, D, E) (_link_exit(A, B, C, D, E, 0))
#define link_exit_dry(A, B, C, D, E) (_link_exit(A, B, C, D, E, 1))
extern void set_source(dbref player, dbref action, dbref source);
extern bool exit_loop_check(register dbref source, register dbref dest);

/* from diskprop.c */
extern void diskbase_debug(dbref player);
extern int fetchprops_priority(dbref obj, int mode);
extern void dispose_all_oldprops(void);

#ifdef FLUSHCHANGED
extern void undirtyprops(dbref obj);
#endif

/* from edit.c */
extern struct macrotable *new_macro(const char *name, const char *definition, dbref player);
extern char *macro_expansion(struct macrotable *node, const char *match);
extern void match_and_list(int descr, dbref player, const char *name,
                           char *linespec, int editor);
extern void do_list(dbref player, dbref program, int arg[], int argc,
                    int commentit);

/* From game.c */
extern void do_auto_archive(int descr, dbref player);
extern void do_dump(dbref player, const char *newfile);
extern void do_shutdown(dbref player, const char *muckname, const char *msg);
extern void do_restart(dbref player, const char *muckname, const char *msg);
extern void fork_and_dump(void);
extern void archive_site();
extern void dump_database(void);
extern int prop_command(int descr, dbref player, char *command, char *arg,
                        char *type, int mt);
extern void dump_warning(void);
extern int dumper_pid;
extern char dump_done;

#ifdef DELTADUMPS
extern void dump_deltas(void);
#endif

/* From hashtab.c */
extern unsigned hash(register const char *s, unsigned hash_size);
extern hash_data *find_hash(const char *s, hash_tab *table, unsigned size);
extern hash_entry *add_hash(const char *name, hash_data data, hash_tab *table,
                            unsigned size);
extern int free_hash(const char *name, hash_tab *table, unsigned size);
extern void kill_hash(hash_tab *table, unsigned size, int freeptrs);

/* From help.c */
extern void get_file_line(const char *filename, char *retbuf, int line);
extern void spit_file(dbref player, const char *filename);
extern void spit_file_segment_lines(dbref player, const char *filename,
                                    const char *seg);
extern void do_help(dbref player, char *topic, char *seg);
extern void do_mpihelp(dbref player, char *topic, char *seg);
extern void do_news(dbref player, char *topic, char *seg);
extern void do_man(dbref player, char *topic, char *seg);
extern void do_motd(dbref player, char *text);
extern void do_info(dbref player, const char *topic, const char *seg);
extern void do_sysparm(dbref player, const char *topic);

/* From look.c */
extern int size_object(register dbref i, register int load);
extern void look_room(int descr, dbref player, dbref room);

/* extern void look_room_simple(dbref player, dbref room); */
extern const char *power_description(register dbref thing);
extern void do_look_around(register int descr, register dbref player);
extern void do_look_at(int descr, dbref player, register const char *name,
                       const char *detail);
extern void do_examine(int descr, dbref player, const char *name,
                       const char *dir);
extern void do_inventory(register dbref player);
extern void do_score(register dbref player);
extern void do_find(register dbref player, register const char *name, const char *flags);
extern void do_owned(register dbref player, const char *name, const char *flags);
extern void do_sweep(int descr, dbref player, const char *name);
extern void do_trace(int descr, register dbref player, const char *name, register int depth);
extern void do_entrances(int descr, register dbref player, const char *name,
                         const char *flags);
extern void do_contents(int descr, register dbref player, const char *name,
                        const char *flags);
extern void exec_or_notify_2(int descr, dbref player, dbref thing,
                           const char *message, const char *whatcalled, bool typ);
#define exec_or_notify(A, B, C, D, E) (exec_or_notify_2(A, B, C, D, E, 1))
#define exec_or_html_notify(A, B, C, D, E) (exec_or_notify_2(A, B, C, D, E, 0))
extern char init_checkflags(dbref player, register const char *flags,
                           struct flgchkdat *check);
extern bool checkflags(register dbref what, struct flgchkdat check);

/* From move.c */
extern void moveto(dbref what, dbref where);
extern void enter_room(int descr, dbref player, dbref loc, dbref exit);
extern void send_home(int descr, dbref thing, int homepuppet);
extern void send_contents(int descr, dbref loc, dbref dest);
extern int parent_loop_check(dbref source, dbref dest);
extern int can_move(int descr, dbref player, const char *direction, int lev);
extern void do_move(int descr, dbref player, const char *direction, int lev);
extern void do_leave(int descr, dbref player);
extern void do_get(int descr, dbref player, const char *what, const char *obj);
extern void do_drop(int descr, dbref player, const char *name, const char *obj);
extern void do_recycle(int descr, dbref player, const char *name);
extern void recycle(int descr, dbref player, dbref thing);

/* From player.c */
extern void clear_players(void);
extern dbref lookup_player(const char *name);
extern void do_password(dbref player, const char *old, const char *newobj);
extern void add_player(dbref who);
extern void delete_player(dbref who);
extern void clear_players(void);

/* From predicates.c */
extern bool ok_name(register const char *name);
extern bool ok_password(register const char *password);
extern bool ok_player_name(register const char *name);
extern bool test_lock(register int descr, register dbref player, register dbref thing,
                      register const char *lockprop);
extern bool test_lock_false_default(register int descr, register dbref player, 
                                  register dbref thing, register const char *lockprop);
extern bool can_link_to(register dbref who, object_flag_type what_type, register dbref where);
extern bool can_link(register dbref who, register dbref what);
extern bool could_doit(int descr, register dbref player, register dbref thing);
extern bool could_doit2(int descr, register dbref player, register dbref thing, char *prop,
                       register bool tryprog);
extern bool can_doit(int descr, register dbref player, register dbref thing,
                    const char *default_fail_msg);
extern bool can_see(register dbref player, register dbref thing, register bool can_see_location);
#define controls(W, H)     (newcontrols(W, H, 0)) 
#define truecontrols(W, H) (newcontrols(W, H, 1)) 
extern bool newcontrols(register dbref who, register dbref what, register bool true_c);
extern bool restricted(register dbref player, register dbref thing, object_flag_type flag);
extern bool restricted2(register dbref player, register dbref thing, object_flag_type flag);
extern bool payfor(register dbref who, register int cost);

/* From rob.c */
extern void do_give(int descr, dbref player, const char *recipient, int amount);

/* From set.c */
extern int controls_link(dbref who, dbref what);
extern void do_name(int descr, dbref player, const char *name, char *newname);
extern void do_describe(int descr, dbref player, const char *name,
                        const char *description);
extern void do_ansidescribe(int descr, dbref player, const char *name,
                            const char *description);
extern void do_htmldescribe(int descr, dbref player, const char *name,
                            const char *description);
extern void do_idescribe(int descr, dbref player, const char *name,
                         const char *description);
extern void do_iansidescribe(int descr, dbref player, const char *name,
                             const char *description);
extern void do_ihtmldescribe(int descr, dbref player, const char *name,
                             const char *description);
extern void do_fail(int descr, dbref player, const char *name,
                    const char *message);
extern void do_success(int descr, dbref player, const char *name,
                       const char *message);
extern void do_drop_message(int descr, dbref player, const char *name,
                            const char *message);
extern void do_osuccess(int descr, dbref player, const char *name,
                        const char *message);
extern void do_ofail(int descr, dbref player, const char *name,
                     const char *message);
extern void do_odrop(int descr, dbref player, const char *name,
                     const char *message);
extern void do_oecho(int descr, dbref player, const char *name,
                     const char *message);
extern void do_pecho(int descr, dbref player, const char *name,
                     const char *message);
extern void do_doing(int descr, dbref player, const char *name,
                     const char *mesg);
extern void do_propset(int descr, dbref player, const char *name,
                       const char *prop);
extern int setlockstr(int descr, dbref player, dbref thing,
                      const char *keyname);
extern void do_lock(int descr, dbref player, const char *name,
                    const char *keyname);
extern void do_unlock(int descr, dbref player, const char *name);
extern void do_unlink(int descr, dbref player, const char *name);
extern void do_chown(int descr, dbref player, const char *name,
                     const char *newobj);
extern void do_set(int descr, dbref player, const char *name, const char *flag);
extern void do_mush_set(int descr, dbref player, char *name, char *setting,
                        char *command);
extern void do_chlock(int descr, dbref player, const char *name,
                      const char *keyname);
extern void do_conlock(int descr, dbref player, const char *name,
                       const char *keyname);
extern void do_flock(int descr, dbref player, const char *name,
                     const char *keyname);
extern void do_relink(int, dbref, const char *, const char *);
extern void do_unlink_quiet(int, dbref, const char *);

/* From speech.c */
extern void do_pose(int descr, dbref player, const char *message);
extern void do_whisper(int descr, dbref player, const char *arg1,
                       const char *arg2);
extern void do_wall(dbref player, const char *message);
extern void do_gripe(dbref player, const char *message);
extern void do_say(int descr, dbref player, const char *message);
extern void do_page(int descr, dbref player, const char *arg1,
                    const char *arg2);
extern int notify_listeners(int descr, dbref who, dbref xprog, dbref obj,
                            dbref room, const char *msg, int isprivate);
extern int ansi_notify_listeners(int descr, dbref who, dbref xprog, dbref obj,
                                 dbref room, const char *msg, int isprivate);
extern int notify_html_listeners(int descr, dbref who, dbref xprog, dbref obj,
                                 dbref room, const char *msg, int isprivate);
extern void notify_except(dbref first, dbref exception, const char *msg,
                          dbref who);
extern void notify_html_except(dbref first, dbref exception, const char *msg,
                               dbref who);
extern void anotify_except(dbref first, dbref exception, const char *msg,
                           dbref who);
extern void parse_omessage(int descr, dbref player, dbref dest, dbref exit,
                           const char *msg, const char *prefix,
                           const char *whatcalled);

/* From stringutil.c */
extern int alphanum_compare(const char *s1, const char *s2);
extern int string_compare(const char *s1, const char *s2);
extern const char *exit_prefix(register const char *string,
                               register const char *prefix);
extern int string_prefix(const char *string, const char *prefix);
extern const char *string_match(const char *src, const char *sub);
extern char *pronoun_substitute(int descr, dbref player, const char *str);
extern char *intostr(char *buf, int i);
extern char *html_escape(const char *str);
extern char *parse_ansi(dbref player, char *buf, const char *from,
                        const char *defcolor);
extern char *unparse_ansi(char *buf, const char *from);
extern char *tct(const char *in, char out[BUFFER_LEN]);
extern char *strip_ansi(char *buf, const char *input);
extern char *strip_bad_ansi(char *buf, const char *input);
extern char *escape_ansi(char *buf, const char *input);
extern char *parse_mush_ansi(char *buf, char *from);
extern char *unparse_mush_ansi(char *buf, char *from);
extern char *mush_tct(const char *in, char out[BUFFER_LEN]);
extern char *parse_tilde_ansi(char *buf, char *from);
extern char *unparse_tilde_ansi(char *buf, char *from);
extern char *tilde_tct(const char *in, char out[BUFFER_LEN]);
extern void prefix_message(char *, const char *, const char *, int, int);
extern int is_prop_prefix(const char *, const char *);
extern int has_suffix(const char *, const char *);
extern int has_suffix_char(const char *, char);

#if !defined(MALLOC_PROFILING)
extern char *string_dup(const char *s);
#endif


/* From utils.c */
extern int member(dbref thing, dbref list);
extern dbref remove_first(dbref first, dbref what);

/* From wiz.c */
extern void do_wizchat(dbref player, const char *arg);
extern void do_memory(dbref who);
extern void do_newpassword(dbref player, const char *name,
                           const char *password);
extern void do_frob(int descr, dbref player, const char *name,
                    const char *recip);
extern void do_fixw(dbref player, const char *msg);
extern void do_serverdebug(int descr, dbref player, const char *arg1,
                           const char *arg2);
extern void do_hopper(dbref player, const char *arg);
extern void do_teleport(int descr, dbref player, const char *arg1,
                        const char *arg2);
extern void do_force(int descr, dbref player, const char *what, char *command);
extern void do_stats(dbref player, const char *name);
extern void do_toad(int descr, dbref player, const char *name,
                    const char *recip);
extern void do_purge(int descr, dbref player, const char *arg1,
                     const char *arg2);
extern void do_boot(dbref player, const char *name);
extern void do_powers(int descr, dbref player, const char *name,
                      const char *power);
extern void do_pcreate(dbref player, const char *arg1, const char *arg2);
extern void do_usage(dbref player);
extern void do_muf_topprofs(dbref player, char *arg1);
extern void do_mpi_topprofs(dbref player, char *arg1);
extern void do_all_topprofs(dbref player, char *arg1);

/* From boolexp.c */
extern int size_boolexp(struct boolexp *b);
extern int eval_boolexp(int descr, dbref player, struct boolexp *b,
                        dbref thing);
extern int eval_boolexp2(int descr, dbref player, struct boolexp *b,
                         dbref thing);
extern struct boolexp *parse_boolexp(int descr, dbref player,
                                     const char *string, int dbloadp);
extern struct boolexp *copy_bool(struct boolexp *old);
extern struct boolexp *getboolexp(FILE * f);
extern struct boolexp *negate_boolexp(struct boolexp *b);
extern void free_boolexp(struct boolexp *b);

/* From unparse.c */
extern char flag_2char(char *flag);
extern char power_2char(char *flag);
extern const char *unparse_flags(dbref thing, char buf[BUFFER_LEN]);
extern const char *ansi_unparse_object(dbref player, dbref object);
extern const char *unparse_object(dbref player, dbref object);
extern const char *unparse_boolexp(dbref player, struct boolexp *b,
                                   int fullname);
extern const char *ansiname(dbref loc, char buf[BUFFER_LEN]);

/* From compress.c */
#ifdef COMPRESS
extern void save_compress_words_to_file(FILE * f);
extern void init_compress_from_file(FILE * dicto);
extern const char *compress(const char *);
extern void init_compress(void);
#endif /* COMPRESS */
extern const char *uncompress(const char *);

/* From edit.c */
extern void interactive(int descr, dbref player, const char *command);

/* From compile.c */
/* extern void kill_def(const char *defname); */
extern long size_prog(dbref prog);
extern void uncompile_program(dbref i);
extern void do_uncompile(dbref player);
extern void do_proginfo(dbref player, const char *arg);
extern void free_unused_programs(void);
extern void do_compile(int descr, dbref in_player, dbref in_program,
                       int force_err_display);
extern int get_primitive(const char *);
extern void clear_primitives(void);
extern void init_primitives(void);

/* From interp.c */
extern struct inst *interp_loop(dbref player, dbref program,
                                struct frame *fr, int rettyp);
extern struct frame *interp(int descr, dbref player, dbref location,
                            dbref program, dbref source, int nosleeping,
                            int whichperms, int forced_pid);
extern void prog_clean(struct frame *fr);

/* From log.c */
extern void log2file(char *myfilename, char *format, ...);
extern void log2filetime(char *myfilename, char *format, ...);
extern void log_command(char *format, ...);
extern void log_error(char *format, ...);
extern void log_gripe(char *format, ...);
extern void log_muf(char *format, ...);
extern void log_conc(char *format, ...);
extern void log_status_nowall(char *format, ...);
extern void log_status(char *format, ...);
extern void show_status(char *format, ...);
extern void log_other(char *format, ...);
extern void notify_fmt(dbref player, char *format, ...);
extern void anotify_fmt(dbref player, char *format, ...);
extern void log_program_text(struct line *first, dbref player, dbref i);

/* From signal.c */
extern void set_signals(void);

/* From strftime.c */
extern int format_time(char *buf, int max_len, const char *fmt,
                       struct tm *tmval);

/* From timestamp.c */
extern void ts_newobject(struct object *thing);
extern void ts_useobject(dbref thing);
extern void ts_lastuseobject(dbref thing);
extern void ts_modifyobject(dbref thing);

/* From smatch.c */
extern int equalstr(char *s, char *t);

extern void CrT_summarize(dbref player);
extern time_t get_tz_offset(void);

extern int force_level;

/* From credits.c */

extern void do_credits(dbref player);

extern void disassemble(dbref player, dbref program);

/* From sanity.c */

extern int sanity_violated;
extern void sanfix(dbref player);
extern void san_main(void);
extern void sane_dump_object(dbref player, const char *arg);
extern void sanity(dbref player);
extern void sanechange(dbref player, const char *command);

/* From tune.c */
extern void do_tune(dbref player, char *parmname, char *parmval);
extern int tune_count_parms(void);
extern void tune_load_parms_from_file(FILE * f, dbref player, int cnt);
extern void tune_save_parms_to_file(FILE * f);
extern void tune_load_parmsfile(dbref player);
extern void tune_save_parmsfile(void);

/* From netresolve.c */

extern void host_add(int, const char *);
extern void host_del(int);
extern char *host_fetch(int);
extern void host_init(void);
extern void host_load(void);
extern void host_save(void);
extern void host_shutdown(void);

/* from random.c */
extern void *init_seed(char *seed);
extern void delete_seed(void *buffer);
extern unsigned long rndn(void *buffer);
extern void MD5base64(char *dest, const void *orig, int len);

#ifdef MCP_SUPPORT

/* from mcppkgs.c */
extern void show_mcp_error(McpFrame *mfr, char *topic, char *text);

#endif

/* from p_socket.c */
#ifdef MUF_SOCKETS
extern void muf_socket_sendevent(struct muf_socket_queue *curr);
#endif /* MUF_SOCKETS */

/* For MPI profiling */
extern time_t mpi_prof_start_time;

/* For select idle profiling */
extern time_t sel_prof_start_time;
extern long sel_prof_idle_sec;
extern long sel_prof_idle_usec;
extern unsigned long sel_prof_idle_use;

/* from p_socket.c */
extern int muf_socket_events();
