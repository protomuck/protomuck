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

#define PRIMS_FILE_FUNCS prim_fwrite, prim_fappend, prim_fread,     \
        prim_freadn, prim_fcr, prim_fpublish, prim_bread,           \
        prim_bwrite, prim_bappend, prim_fsize, prim_fstats,         \
        prim_curid, prim_fsinfo, prim_frm, prim_fren, prim_freadto, \
        prim_fnameokp, prim_getdir, prim_mkdir, prim_rmdir

#define PRIMS_FILE_NAMES "FWRITE", "FAPPEND", "FREAD", "FREADN",  \
        "FCR", "FPUBLISH", "BREAD", "BWRITE", "BAPPEND", "FSIZE", \
        "FSTATS", "CURID", "FSINFO", "FRM", "FREN", "FREADTO",    \
        "FNAME-OK?", "GETDIR", "MKDIR", "RMDIR"

#if defined(FILE_PRIMS) && !defined(WIN_VC)
#define PRIMS_FILE_CNT 20
#else
#define PRIMS_FILE_CNT 0
#endif

