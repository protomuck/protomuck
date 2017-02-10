
#ifndef MSGPARSE_H
#define MSGPARSE_H

#define MAX_MFUN_NAME_LEN 16
#define MAX_MFUN_LIST_LEN 512
#define MPI_MAX_VARIABLES 32
#define MPI_MAX_FUNCTIONS 32

#define MFUN_LITCHAR '`'
#define MFUN_LEADCHAR '{'
#define MFUN_ARGSTART ':'
#define MFUN_ARGSEP ','
#define MFUN_ARGEND '}'

#define UNKNOWN ((dbref)-88)
#define PERMDENIED ((dbref)-89)

#undef WIZZED_DELAY

extern bool Archperms(dbref what);
extern bool Wizperms(dbref what);
extern bool Mageperms(dbref what);

extern void mesg_init(void);

extern bool safeputprop(dbref obj, dbref perms, const char *buf, const char *val);
extern const char *safegetprop_limited(dbref player, dbref what, dbref whom, dbref perms, const char *inbuf);
extern const char *safegetprop_strict(dbref player, dbref what, dbref perms, const char *inbuf);
extern const char *safegetprop(dbref player, dbref what, dbref perms, const char *inbuf);

extern const char *stripspaces(char *buf, char *in);
extern const char *string_substitute(const char *str, const char *oldstr, const char *newstr, char *buf, int maxlen);
extern const char *cr2slash(char *buf, const char *in);

extern int get_list_count(dbref trig, dbref what, dbref perms, const char *listname);
extern const char *get_list_item(dbref trig, dbref what, dbref perms, const char *listname, int itemnum);
extern const char *get_concat_list(dbref player, dbref what, dbref perms, dbref obj, const char *listname, char *buf, int maxchars, int mode);

extern bool isneighbor(dbref d1, dbref d2);
extern bool mesg_read_perms(dbref player, dbref perms, dbref obj);
extern bool mesg_local_perms(dbref player, dbref perms, dbref obj);

extern dbref mesg_dbref_raw(int descr, dbref player, dbref what, dbref perms, const char *buf);
extern dbref mesg_dbref(int descr, dbref player, dbref what, dbref perms, char *buf);
extern dbref mesg_dbref_mage(int descr, dbref player, dbref what, dbref perms, char *buf);
extern dbref mesg_dbref_strict(int descr, dbref player, dbref what, dbref perms, char *buf);
extern dbref mesg_dbref_local(int descr, dbref player, dbref what, dbref perms, char *buf);

extern const char *ref2str(dbref obj, char *buf);
extern bool truestr(const char *buf);

extern bool check_mvar_overflow(int count);
extern bool new_mvar(const char *varname, char *buf);
extern char *get_mvar(const char *varname);
extern bool free_top_mvar(void);

extern bool new_mfunc(const char *funcname, const char *buf);
extern const char *get_mfunc(const char *funcname);
extern bool free_mfuncs(int downto);



#define MFUNARGS int descr, dbref player, dbref what, dbref perms, int argc, \
                argv_typ argv, char *buf, int mesgtyp

#define CHECKRETURN(vari,funam,num) if (!vari) { snprintf(buf, BUFFER_LEN, "%s %c%s%c (%s)", get_mvar("how"), MFUN_LEADCHAR, funam, MFUN_ARGEND, num);  notify_nolisten(player, buf, 1);  return NULL; }

#define ABORT_MPI(funam,mesg) { snprintf(buf, BUFFER_LEN, "%s %c%s%c: %s", get_mvar("how"), MFUN_LEADCHAR, funam, MFUN_ARGEND, mesg);  notify_nolisten(player, buf, 1);  return NULL; }

typedef char argv_typ[10][BUFFER_LEN];

#define MesgParse(in,out) mesg_parse(descr, player, what, perms, (in), (out), BUFFER_LEN, mesgtyp)


#endif /* MSGPARSE_H */

