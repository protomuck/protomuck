/* Property struct */
struct plist {
    unsigned short flags;
    short   height;		/* satisfy the avl monster.  */
    union {
	char           *str;
	struct boolexp *lok;
	int             val;
      float          fval;
	dbref           ref;
	int             pos;
    }       data;
    struct plist *left, *right, *dir;
    char   key[1];
};

/* property node pointer type */
typedef struct plist *PropPtr;

/* Type for prop pointer/values */
#define PTYPE	char *

/* propload queue types */
#define PROPS_UNLOADED 0x0
#define PROPS_LOADED   0x1
#define PROPS_PRIORITY 0x2
#define PROPS_CHANGED  0x3


/* property value types */
#define PROP_DIRTYP   0x0
#define PROP_STRTYP   0x2
#define PROP_INTTYP   0x3
#define PROP_FLTTYP   0x6
#define PROP_LOKTYP   0x4
#define PROP_REFTYP   0x5
#define PROP_TYPMASK  0x7

/* Property flags.  Unimplemented as yet. */
#define PROP_UREAD      0x0010
#define PROP_UWRITE     0x0020
#define PROP_WREAD      0x0040
#define PROP_WWRITE     0x0080

/* half implemented.  Will be used for stuff like password props. */
#define PROP_SYSPERMS   0x0100

/* If set, this prop's string value uses Dr.Cat's compression code. */
#define PROP_COMPRESSED 0x0008

/* Internally used prop flags.  Never stored on disk. */
#define PROP_ISUNLOADED 0x0200
#define PROP_TOUCHED    0x0400


/* Macros */
#define AVL_LF(x) (x)->left
#define AVL_RT(x) (x)->right

#define SetPDir(x,y) {(x)->dir = y;}
#define PropDir(x) ((x)->dir)

#define SetPDataStr(x,z) {(x)->data.str = z;}
#define SetPDataVal(x,z) {(x)->data.val = z;}
#define SetPDataRef(x,z) {(x)->data.ref = z;}
#define SetPDataLok(x,z) {(x)->data.lok = z;}
#define SetPDataFVal(x,z) {(x)->data.fval = z;}

#define PropDataStr(x) ((x)->data.str)
#define PropDataVal(x) ((x)->data.val)
#define PropDataRef(x) ((x)->data.ref)
#define PropDataLok(x) ((x)->data.lok)
#define PropDataFVal(x) ((x)->data.fval)

#define PropName(x) ((x)->key)

#define SetPFlags(x,y) {(x)->flags = ((x)->flags & PROP_TYPMASK) | (short)y;}
#define PropFlags(x) ((x)->flags & ~PROP_TYPMASK)

#define SetPType(x,y) {(x)->flags = ((x)->flags & ~PROP_TYPMASK) | (short)y;}
#define PropType(x) ((x)->flags & PROP_TYPMASK)

#define SetPFlagsRaw(x,y) {(x)->flags = (short)y;}
#define PropFlagsRaw(x) ((x)->flags)

/* property access macros */
#define Prop_ReadOnly(name) \
    (Prop_Check(name, PROP_RDONLY) || Prop_Check(name, PROP_RDONLY2))
#define Prop_Private(name) Prop_Check(name, PROP_PRIVATE)
#define Prop_SeeOnly(name) Prop_Check(name, PROP_SEEONLY)
#define Prop_Hidden(name) Prop_Check(name, PROP_HIDDEN)


/* Routines as they need to be:

     PropPtr locate_prop(PropPtr list, char *name)
       if list is NULL, return NULL.

     PropPtr new_prop(PropPtr *list, char *name)
       if *list is NULL, create a new propdir, then insert the prop

     PropPtr delete_prop(PropPtr *list, char *name)
       when last prop in dir is deleted, destroy propdir & change *list to NULL

     PropPtr first_node(PropPtr list)
       if list is NULL, return NULL

     PropPtr next_node(PropPtr list, char *name)
       if list is NULL, return NULL

 */

extern void clear_propnode(PropPtr p);
extern void delete_proplist(PropPtr p);
extern PropPtr alloc_propnode(const char *name);
extern void free_propnode(PropPtr node);
extern PropPtr first_node(PropPtr p);
extern PropPtr next_node(PropPtr p, char *c);
extern void putprop(FILE * f, PropPtr  p);
extern int Prop_Check(const char *name, const char what);
extern PropPtr locate_prop(PropPtr l, char *path);
extern PropPtr new_prop(PropPtr *l, char *path);
extern PropPtr delete_prop(PropPtr *list, char *name);
extern int size_proplist(PropPtr avl);

extern void set_property_nofetch(dbref player, const char *type, int flags, PTYPE value);
extern void set_property(dbref player, const char *type, int flags, PTYPE value);
extern void add_prop_nofetch(dbref player, const char *type, const char *pclass, int value);
extern void add_property(dbref player, const char *type, const char *pclass, int value);
extern void remove_property_list(dbref player, int all);
extern void remove_property_nofetch(dbref player, const char *type);
extern void remove_property(dbref player, const char *type);
extern int has_property(int descr, dbref player, dbref what, const char *type, const char *pclass, int value);
extern int has_property_strict(int descr, dbref player, dbref what, const char *type, const char *pclass, int value);
extern PropPtr get_property(dbref player, const char *type);

extern const char *get_property_class(dbref player, const char *type);
extern float get_property_fvalue(dbref player, const char *type);
extern int get_property_value(dbref player, const char *type);
extern dbref get_property_dbref(dbref player, const char *pclass);
extern struct boolexp *get_property_lock(dbref player, const char *type);
extern int genderof(int descr, dbref player);
extern struct plist *copy_prop(dbref old);
extern void copy_proplist(dbref obj, PropPtr *new2, PropPtr old);
extern PropPtr first_prop(dbref player, const char *dir, PropPtr *list, char *name);
extern PropPtr first_prop_nofetch(dbref player, const char *dir, PropPtr *list, char *name);
extern PropPtr next_prop(PropPtr list, PropPtr prop, char *name);
extern char *next_prop_name(dbref player, char *outbuf, char *name);
extern int is_propdir(dbref player, const char *dir);
extern const char *envpropstr(dbref *where, const char *propname);
extern PropPtr envprop(dbref *where, const char *propname, int typ);
extern PropPtr envprop_cmds(dbref *where, const char *propname, int typ);
extern PropPtr regenvprop(dbref *where, const char *propname, int typ);
extern void delete_proplist(PropPtr p);

#ifdef DISKBASE
extern int fetchprops_priority(dbref obj, int mode);
extern int fetchprops_nostamp(dbref obj);
extern void fetchprops(dbref obj);
extern void unloadprops_with_prejudice(dbref obj);
extern int disposeprops_notime(dbref obj);
extern int disposeprops(dbref obj);
extern void dirtyprops(dbref obj);
extern void undirtyprops(dbref obj);
extern int propfetch(dbref obj, PropPtr p);
#endif /* DISKBASE */

extern PropPtr propdir_new_elem(PropPtr *l, char *path);
extern PropPtr propdir_delete_elem(PropPtr l, char *path);
extern PropPtr propdir_get_elem(PropPtr l, char *path);
extern PropPtr propdir_first_elem(PropPtr l, char *path);
extern PropPtr propdir_next_elem(PropPtr l, char *path);
extern int propdir_check(PropPtr l, char *path);

extern void db_putprop(FILE *f, const char *dir, PropPtr p);
extern int db_get_single_prop(FILE *f, dbref obj, int pos);
extern void db_getprops(FILE *f, dbref obj);
extern void db_dump_props(FILE *f, dbref obj);


/* From property.c */

extern void db_putprop(FILE *f, const char *dir, PropPtr p);
extern void db_dump_props(FILE *f, dbref obj);
extern void db_dump_props_rec(dbref obj, FILE *f, const char *dir, PropPtr p);
extern void db_getprops(FILE *f, dbref obj);
extern char *displayprop(dbref player, dbref obj, const char *name, char *buf);
extern int size_properties(dbref player, int load);
extern void untouchprops_incremental(int limit);
extern int Prop_SysPerms(dbref obj, const char *type);


