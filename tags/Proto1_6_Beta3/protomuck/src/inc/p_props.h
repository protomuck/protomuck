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

#define PRIMS_PROPS_FUNCS prim_getpropval, prim_getpropstr, prim_remove_prop,   \
   prim_envprop, prim_envpropstr, prim_addprop, prim_nextprop, prim_propdirp,   \
   prim_parsempi, prim_parseprop, prim_getprop, prim_setprop, prim_getpropfval, \
   prim_propqueue, prim_envpropqueue, prim_islockedp, prim_array_filter_prop

#define PRIMS_PROPS_NAMES "GETPROPVAL", "GETPROPSTR", "REMOVE_PROP",  \
    "ENVPROP", "ENVPROPSTR", "ADDPROP", "NEXTPROP", "PROPDIR?",       \
    "PARSEMPI", "PARSEPROP", "GETPROP", "SETPROP", "GETPROPFVAL",     \
    "PROPQUEUE", "ENVPROPQUEUE", "ISLOCKED?", "ARRAY_FILTER_PROP"

#define PRIMS_PROPS_CNT 17

