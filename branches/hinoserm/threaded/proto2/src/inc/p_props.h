extern void prim_getpropfval(PRIM_PROTOTYPE);
extern void prim_getpropval(PRIM_PROTOTYPE);
extern void prim_getpropstr(PRIM_PROTOTYPE);
extern void prim_remove_prop(PRIM_PROTOTYPE);
extern void prim_envprop(PRIM_PROTOTYPE);
extern void prim_envpropstr(PRIM_PROTOTYPE);
extern void prim_addprop(PRIM_PROTOTYPE);
extern void prim_nextprop(PRIM_PROTOTYPE);
extern void prim_propdirp(PRIM_PROTOTYPE);
extern void prim_parsempi(PRIM_PROTOTYPE);
extern void prim_parseprop(PRIM_PROTOTYPE);
extern void prim_getprop(PRIM_PROTOTYPE);
extern void prim_setprop(PRIM_PROTOTYPE);
extern void prim_propqueue(PRIM_PROTOTYPE);
extern void prim_envpropqueue(PRIM_PROTOTYPE);
extern void prim_islockedp(PRIM_PROTOTYPE);
extern void prim_array_filter_prop(PRIM_PROTOTYPE);
extern void prim_reflist_find(PRIM_PROTOTYPE);
extern void prim_reflist_add(PRIM_PROTOTYPE);
extern void prim_reflist_del(PRIM_PROTOTYPE);
extern void prim_parsepropex(PRIM_PROTOTYPE);
extern void prim_copyprops(PRIM_PROTOTYPE);
extern void prim_testlock(PRIM_PROTOTYPE);
extern void prim_lockedp(PRIM_PROTOTYPE);
extern void prim_checklock(PRIM_PROTOTYPE);
extern void prim_array_filter_smart(PRIM_PROTOTYPE);

#define PRIMS_PROPS_FUNCS prim_getpropval, prim_getpropstr, prim_remove_prop, \
   prim_envprop, prim_envpropstr, prim_addprop, prim_nextprop, prim_propdirp, \
   prim_parsempi, prim_parseprop, prim_getprop, prim_setprop, prim_getpropfval,\
   prim_propqueue, prim_envpropqueue, prim_islockedp, prim_array_filter_prop, \
   prim_reflist_find, prim_reflist_add, prim_reflist_del, prim_parsepropex,   \
   prim_copyprops, prim_testlock, prim_lockedp, prim_checklock, prim_array_filter_smart

#define PRIMS_PROPS_NAMES "GETPROPVAL", "GETPROPSTR", "REMOVE_PROP",  \
    "ENVPROP", "ENVPROPSTR", "ADDPROP", "NEXTPROP", "PROPDIR?",       \
    "PARSEMPI", "PARSEPROP", "GETPROP", "SETPROP", "GETPROPFVAL",     \
    "PROPQUEUE", "ENVPROPQUEUE", "ISLOCKED?", "ARRAY_FILTER_PROP",    \
    "REFLIST_FIND", "REFLIST_ADD", "REFLIST_DEL", "PARSEPROPEX",      \
    "COPYPROPS", "TESTLOCK", "LOCKED?", "CHECKLOCK", "ARRAY_FILTER_SMART"

#define PRIMLIST_PROPS  { "GETPROPVAL",         LM1,     2, prim_getpropval },         \
                        { "GETPROPSTR",         LM1,     2, prim_getpropstr },         \
                        { "REMOVE_PROP",        LM1,     2, prim_remove_prop },        \
                        { "ENVPROP",            LM1,     2, prim_envprop },            \
                        { "ENVPROPSTR",         LM1,     2, prim_envpropstr },         \
                        { "ADDPROP",            LM1,     4, prim_addprop },            \
                        { "NEXTPROP",           LM1,     2, prim_nextprop },           \
                        { "PROPDIR?",           LM2,     2, prim_propdirp },           \
                        { "PARSEMPI",           LWIZ,    4, prim_parsempi },           \
                        { "PARSEPROP",          LM3,     4, prim_parseprop },          \
                        { "GETPROP",            LM1,     2, prim_getprop },            \
                        { "SETPROP",            LM1,     3, prim_setprop },            \
                        { "GETPROPFVAL",        LM1,     2, prim_getpropfval },        \
                        { "PROPQUEUE",          LARCH,   4, prim_propqueue },          \
                        { "ENVPROPQUEUE",       LARCH,   4, prim_envpropqueue },      \
                        { "ISLOCKED?",          LM1,     3, prim_islockedp },          \
                        { "ARRAY_FILTER_PROP",  LM1,     3, prim_array_filter_prop },  \
                        { "REFLIST_FIND",       LM1,     3, prim_reflist_find },       \
                        { "REFLIST_ADD",        LM1,     3, prim_reflist_add },        \
                        { "REFLIST_DEL",        LM1,     3, prim_reflist_del },        \
                        { "PARSEPROPEX",        LM3,     4, prim_parsepropex },        \
                        { "COPYPROPS",          LARCH,   5, prim_copyprops },          \
                        { "TESTLOCK",           LM1,     2, prim_testlock },           \
                        { "LOCKED?",            LM1,     2, prim_lockedp },            \
                        { "CHECKLOCK",          LM1,     3, prim_checklock },          \
                        { "ARRAY_FILTER_SMART", LM1,     4, prim_array_filter_smart }

#define PRIMS_PROPS_CNT 26