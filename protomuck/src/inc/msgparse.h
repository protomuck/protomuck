#ifdef MPI

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


int Archperms(dbref what);
int Wizperms(dbref what);
int Mageperms(dbref what);

extern void mesg_init(void);

int safeputprop(dbref obj, dbref perms, char *buf, char *val);
const char *safegetprop(dbref player, dbref what, dbref perms, const char *inbuf);
const char *safegetprop_strict(dbref player, dbref what, dbref perms, const char *inbuf);

char *stripspaces(char *buf, char *in);
char *string_substitute(const char *str, const char *oldstr, const char *newstr, char *buf, int maxlen);
char *cr2slash(char *buf, const char *in);

int get_list_count(dbref trig, dbref what, dbref perms, const char *listname);
const char *get_list_item(dbref trig, dbref what, dbref perms, const char *listname, int itemnum);
char *get_concat_list(dbref player, dbref what, dbref perms, dbref obj, const char *listname, char *buf, int maxchars, int mode);

int isneighbor(dbref d1, dbref d2);
int mesg_read_perms(dbref player, dbref perms, dbref obj);
int mesg_local_perms(dbref player, dbref perms, dbref obj);

dbref mesg_dbref_raw(int descr, dbref player, dbref what, dbref perms, const char *buf);
dbref mesg_dbref(int descr, dbref player, dbref what, dbref perms, char *buf);
dbref mesg_dbref_mage(int descr, dbref player, dbref what, dbref perms, char *buf);
dbref mesg_dbref_strict(int descr, dbref player, dbref what, dbref perms, char *buf);
dbref mesg_dbref_local(int descr, dbref player, dbref what, dbref perms, char *buf);

char *ref2str(dbref obj, char *buf);
int truestr(char *buf);

int new_mvar(const char *varname, char *buf);
char *get_mvar(const char *varname);
int free_top_mvar(void);

int new_mfunc(const char *funcname, const char *buf);
const char *get_mfunc(const char *funcname);
int free_mfuncs(int downto);



#define MFUNARGS int descr, dbref player, dbref what, dbref perms, int argc, \
                argv_typ argv, char *buf, int mesgtyp

#define CHECKRETURN(vari,funam,num) if (!vari) { sprintf(buf, "%s %c%s%c (%s)", get_mvar("how"), MFUN_LEADCHAR, funam, MFUN_ARGEND, num);  notify_nolisten(player, buf, 1);  return NULL; }

#define ABORT_MPI(funam,mesg) { sprintf(buf, "%s %c%s%c: %s", get_mvar("how"), MFUN_LEADCHAR, funam, MFUN_ARGEND, mesg);  notify_nolisten(player, buf, 1);  return NULL; }

typedef char argv_typ[10][BUFFER_LEN];

#define MesgParse(in,out) mesg_parse(descr, player, what, perms, (in), (out), BUFFER_LEN, mesgtyp)


#endif /* MPI */


