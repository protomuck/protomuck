/* CrT's own silly little malloc wrappers for debugging purposes: */

#ifndef _CRT_MALLOC_H
#define _CRT_MALLOC_H

#include <sys/types.h>

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
# if defined(__APPLE__) && defined(HAVE_SYS_MALLOC_H)
#  include <sys/malloc.h>
# endif
#elif defined(HAVE_MALLOC_H)
# include <malloc.h>
#endif

extern void CrT_check(const char *, int);
extern int CrT_check_everything(const char *, int);
extern void CrT_summarize_to_file(const char *file, const char *comment);
extern void *CrT_malloc(size_t size, const char *whatfile, int whatline);
extern void *CrT_calloc(size_t num, size_t size, const char *whatfile, int whatline);
extern void *CrT_realloc(void *p, size_t size, const char *whatfile, int whatline);
extern void CrT_free(void *p, const char *whatfile, int whatline);
extern char *CrT_string_dup(const char *, const char *, int);
extern char *CrT_alloc_string(const char *, const char *, int);
extern struct shared_string *CrT_alloc_prog_string(const char *, const char *, int, int, int);

#define malloc(x)                      CrT_malloc(      x,      __FILE__, __LINE__)
#define calloc(x,y)                    CrT_calloc(      x, y,   __FILE__, __LINE__)
#define realloc(x,y)                   CrT_realloc(     x, y,   __FILE__, __LINE__)
#define free(x)                        CrT_free(        x,      __FILE__, __LINE__)
#define string_dup(x)                  CrT_string_dup(  x,      __FILE__, __LINE__)
#define alloc_string(x)                CrT_alloc_string(x,      __FILE__, __LINE__)
#define alloc_prog_string_exact(x,y,z) CrT_alloc_prog_string(x, __FILE__, __LINE__, y, z)
#define alloc_prog_string(x)           alloc_prog_string_exact(x, -2, -2)

#endif /* _CRT_MALLOC_H */

