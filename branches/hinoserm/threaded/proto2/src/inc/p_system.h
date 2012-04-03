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
extern void prim_sysparm_array(PRIM_PROTOTYPE);

#define PRIMLIST_SYSTEM { "SYSPARM",        LM1,   1, prim_sysparm       }, \
                        { "SETSYSPARM",     LARCH, 2, prim_setsysparm    }, \
                        { "VERSION",        LM1,   0, prim_version       }, \
                        { "FORCE",          LARCH, 2, prim_force         }, \
                        { "FORCE_LEVEL",    LM1,   0, prim_force_level   }, \
                        { "LOGSTATUS",      LARCH, 1, prim_logstatus     }, \
                        { "DUMP",           LARCH, 0, prim_dump          }, \
                        { "DELTA",          LARCH, 0, prim_delta         }, \
                        { "SHUTDOWN",       LBOY,  1, prim_shutdown      }, \
                        { "RESTART",        LBOY,  1, prim_restart       }, \
                        { "ARMAGEDDON",     LBOY,  1, prim_armageddon    }, \
                        { "SYSPARM_ARRAY",  LM1,   1, prim_sysparm_array }

#define PRIMS_SYSTEM_CNT 12
