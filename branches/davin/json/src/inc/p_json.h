#ifndef P_JSON_H
#define P_JSON_H

extern void prim_array_jencode(PRIM_PROTOTYPE);
extern void prim_array_jdecode(PRIM_PROTOTYPE);
extern void prim_jdecode_file(PRIM_PROTOTYPE);
extern void prim_jencode_file(PRIM_PROTOTYPE);

#ifdef JSON_SUPPORT

#ifdef FILE_PRIMS

#define PRIMS_JSON_FUNCS prim_array_jencode, prim_array_jdecode, prim_jencode_file, prim_jdecode_file
#define PRIMS_JSON_NAMES "ARRAY_JENCODE", "ARRAY_JDECODE", "JENCODE_FILE", "JDECODE_FILE"
#define PRIMS_JSON_CNT 4

#else /* FILE_PRIMS */

#define PRIMS_JSON_FUNCS prim_array_jencode, prim_array_jdecode
#define PRIMS_JSON_NAMES "ARRAY_JENCODE", "ARRAY_JDECODE"
#define PRIMS_JSON_CNT 2

#endif /* FILE_PRIMS */


#else /* JSON_SUPPORT */
#define PRIMS_JSON_CNT 0
#endif /* JSON_SUPPORT */


#endif /* P_JSON_H */
