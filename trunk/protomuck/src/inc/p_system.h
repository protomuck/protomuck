extern void prim_sysparm(PRIM_PROTOTYPE);
extern void prim_setsysparm(PRIM_PROTOTYPE);
extern void prim_version(PRIM_PROTOTYPE);
extern void prim_force(PRIM_PROTOTYPE);
extern void prim_force_level(PRIM_PROTOTYPE);
extern void prim_logstatus(PRIM_PROTOTYPE);
extern void prim_dump(PRIM_PROTOTYPE);
extern void prim_delta(PRIM_PROTOTYPE);
extern void prim_shutdown(PRIM_PROTOTYPE);
extern void prim_restart(PRIM_PROTOTYPE);
extern void prim_armageddon(PRIM_PROTOTYPE);

#define PRIMS_SYSTEM_FUNCS prim_sysparm, prim_setsysparm, prim_version, \
    prim_force, prim_force_level, prim_logstatus, prim_dump, prim_delta,\
    prim_shutdown, prim_restart, prim_armageddon

#define PRIMS_SYSTEM_NAMES "SYSPARM", "SETSYSPARM", "VERSION", \
   "FORCE", "FORCE_LEVEL", "LOGSTATUS", "DUMP", "DELTA",       \
   "SHUTDOWN", "RESTART", "ARMAGEDDON"

#define PRIMS_SYSTEM_CNT 11
