extern void prim_fwrite(PRIM_PROTOTYPE);
extern void prim_fappend(PRIM_PROTOTYPE);
extern void prim_fread(PRIM_PROTOTYPE);
extern void prim_freadn(PRIM_PROTOTYPE);
extern void prim_fcr(PRIM_PROTOTYPE);
extern void prim_fpublish(PRIM_PROTOTYPE);
extern void prim_bread(PRIM_PROTOTYPE);
extern void prim_bwrite(PRIM_PROTOTYPE);
extern void prim_bappend(PRIM_PROTOTYPE);
extern void prim_fsize(PRIM_PROTOTYPE);
extern void prim_fstats(PRIM_PROTOTYPE);
extern void prim_curid(PRIM_PROTOTYPE);
extern void prim_fsinfo(PRIM_PROTOTYPE);
extern void prim_frm(PRIM_PROTOTYPE);
extern void prim_fren(PRIM_PROTOTYPE);
extern void prim_freadto(PRIM_PROTOTYPE);
extern void prim_fnameokp(PRIM_PROTOTYPE);
extern void prim_getdir(PRIM_PROTOTYPE);
extern void prim_mkdir(PRIM_PROTOTYPE);
extern void prim_rmdir(PRIM_PROTOTYPE);

#define PRIMLIST_FILE { "FWRITE",      LBOY,  3, prim_fwrite     }, \
                      { "FAPPEND",     LBOY,  2, prim_fappend    }, \
                      { "FREAD",       LBOY,  2, prim_fread      }, \
                      { "FREADN",      LBOY,  3, prim_freadn     }, \
                      { "FCR",         LBOY,  1, prim_fcr        }, \
                      { "FPUBLISH",    LBOY,  1, prim_fpublish   }, \
                      { "BREAD",       LBOY,  2, prim_bread      }, \
                      { "BWRITE",      LBOY,  3, prim_bwrite     }, \
                      { "BAPPEND",     LBOY,  2, prim_bappend    }, \
                      { "FSIZE",       LBOY,  1, prim_fsize      }, \
                      { "FSTATS",      LBOY,  1, prim_fstats     }, \
                      { "CURID",       LBOY,  0, prim_curid      }, \
                      { "FSINFO",      LBOY,  0, prim_fsinfo     }, \
                      { "FRM",         LBOY,  1, prim_frm        }, \
                      { "FREN",        LBOY,  2, prim_fren       }, \
                      { "FREADTO",     LBOY,  3, prim_freadto    }, \
                      { "FNAME-OK?",   LBOY,  1, prim_fnameokp   }, \
                      { "GETDIR",      LBOY,  1, prim_getdir     }, \
                      { "MKDIR",       LBOY,  1, prim_mkdir      }, \
                      { "RMDIR",       LBOY,  1, prim_rmdir      }
 
#if defined(FILE_PRIMS) && !defined(WIN_VC)
#define PRIMS_FILE_CNT 20
#else
#define PRIMS_FILE_CNT 0
#endif

