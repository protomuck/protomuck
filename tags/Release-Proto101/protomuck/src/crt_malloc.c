/* crt_malloc.c -- CrT's own silly malloc wrappers for debugging.	*/


#include "config.h"
#include "interface.h"
#include "externs.h"

#ifdef MALLOC_PROFILING

#ifndef CRT_DEBUG_ALSO		/* Let user set switch in extern head file. */
#define CRT_DEBUG_ALSO FALSE	/* Default to reasonable val if s/he didn't.*/
#endif

#ifndef CRT_NEW_TO_CHECK
#define CRT_NEW_TO_CHECK (128)	/* Make it a nonzero power of two.	*/
#endif

#ifndef CRT_OLD_TO_CHECK
#define CRT_OLD_TO_CHECK (128)
#endif

#ifndef CRT_MAGIC
#define CRT_MAGIC ((char)231)
#endif

#ifndef CRT_FREE_MAGIC
#define CRT_FREE_MAGIC ((char)~CRT_MAGIC)
#endif

struct CrT_block_rec {
    const char*              file;
    int                      line;

    int                      tot_bytes_alloc;
    int                      tot_allocs_done;
    int                      live_blocks;
    int                      live_bytes;
    int                      max_blocks;
    int                      max_bytes;
    time_t                   max_bytes_time;

    struct CrT_block_rec *   next;
};
typedef struct CrT_block_rec  A_Block;
typedef struct CrT_block_rec*   Block;

static Block block_list = NULL;

struct CrT_header_rec {
    Block b;
    size_t size;
#ifdef CRT_DEBUG_ALSO
    struct CrT_header_rec * next;
    struct CrT_header_rec * prev;
    char                  * end;
#endif
};
typedef struct CrT_header_rec  A_Header;
typedef struct CrT_header_rec*   Header;

#ifdef CRT_DEBUG_ALSO
static A_Block Root_Owner = {

    __FILE__,		/* file */
    __LINE__,		/* line */

    0,                  /* tot_bytes_alloc */
    0,                  /* tot_allocs_done */
    0,                  /* live_blocks     */
    0,                  /* live_bytes      */
    0,                  /* max_blocks      */
    0,                  /* max_bytes       */
    0,                  /* max_bytes_time  */

    NULL                /* next            */
};

static char root_end = CRT_MAGIC;

static A_Header root = {
    &Root_Owner,  /* b    */
    0,            /* size */
    &root,        /* next */
    &root,	  /* prev */
    &root_end,    /* end  */
};

static Header rover = &root;

/* Macro to insert block m into 'root' linklist: */
#undef  INSERT
#define INSERT(m) {			\
    root. next->prev =  (m);		\
   (m)        ->next =  root.next;	\
   (m)        ->prev = &root;		\
    root.       next =  (m);		\
    }

/* Macro to delete block m from 'root' linklist: */
#undef  DELETE
#define DELETE(m) {			\
   (m)->next->prev = (m)->prev;		\
   (m)->prev->next = (m)->next;		\
    }

/* An array tracking last CRT_NEW_TO_CHECK blocks touched: */
static Header just_touched[ CRT_NEW_TO_CHECK ] = {
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,

    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,

    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,

    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,
    &root,    &root,    &root,    &root,

};
static int next_touched = 0;	/* Always indexes above. */

#endif

#undef malloc
#undef calloc
#undef realloc
#undef free

/* Number of bytes of overhead we add to a block: */
#ifdef CRT_DEBUG_ALSO
#define CRT_OVERHEAD_BYTES (sizeof(A_Header) +1)
#else
#define CRT_OVERHEAD_BYTES (sizeof(A_Header)   )
#endif

static int block_count(
    Block b
) {
    int    i;
    for   (i = 0;   b;   b = b->next)   ++i;
    return i;
}

static Block blocks_nth(	/* Return nth block in list, first is #0. */
    Block b,
    int   n
) {
    int    i;
    for   (i = 0;   i < n;   b = b->next)   ++i;
    return b;
}

static Block blocks_split(	
    Block b,
    int   n	/* Nth block becomes first in 2nd list. */
) {
    Block tail = blocks_nth( b, n-1 );
    Block head = tail->next;
    tail->next = NULL;
    return head;
}

static Block blocks_merge(	/* Merge two sorted lists into one. */
    Block b0,
    Block b1
) {
    A_Block head;
    Block   tail = &head;

    while (b0 || b1) {
	Block nxt;

	if      (!b1)                             { nxt = b0; b0 = b0->next; }
        else if (!b0)                             { nxt = b1; b1 = b1->next; }
        else if (b0->live_bytes < b1->live_bytes) { nxt = b0; b0 = b0->next; }
        else                                      { nxt = b1; b1 = b1->next; }

	tail->next = nxt;
        tail       = nxt;
    }

    tail->next = NULL;

    return head.next;
}

static Block blocks_sort(	/* Sort 'b1' on 'live_bytes'. */
    Block b1
) {
    int len   = block_count( b1 );
    if (len < 2)      return b1  ;

    {   Block b2 = blocks_split( b1, len/2 );

	return blocks_merge( 
	    blocks_sort( b1 ),
	    blocks_sort( b2 )
	);
    }
}

static Block block_alloc(
    const char* file,
    int         line
) {
    Block b = malloc( sizeof( A_Block ) );

    b->file = file;
    b->line = line;

    b->tot_bytes_alloc = 0;
    b->tot_allocs_done = 0;
    b->live_blocks     = 0;
    b->live_bytes      = 0;
    b->max_blocks      = 0;
    b->max_bytes       = 0;

    b->next    = block_list;
    block_list = b;
    return b;
}

static Block block_find(
    const char* file,
    int         line
) {
    Block b;
    for  (b = block_list;   b;   b = b->next) {
        if (         b->line == line
	&&  !strcmp( b->file,   file )
        ){
	    return b;
    }   }
    return block_alloc( file, line );
}

const char *
CrT_timestr( time_t when )
{
    static char buf[20];
    struct tm *da_time;

    da_time = localtime(&when);
    sprintf(
	buf, "%02d%02d%02d%02d",
	da_time->tm_mday, da_time->tm_hour,
	da_time->tm_min, da_time->tm_sec
    );
    return buf;
}

static dbref summarize_player;

static void summarize(
    void (*fn)( char* )
) {
    Block b;

    int sum_blks = 0;
    int sum_byts = 0;
    int sum_totblks = 0;
    int sum_totbyts = 0;

    char buf[ 256 ];

    block_list = blocks_sort( block_list );

#undef  X
#define X(t) (*fn)(t);
X("         File Line CurBlks CurBytes MaxBlks MaxBytes MxByTime TotBlks TotBytes");
X("------------- ---- ------- -------- ------- -------- -------- ------- --------")

    for  (b = block_list;   b;   b = b->next) {

	sum_blks    += b->live_blocks;
	sum_byts    += b->live_bytes;

	sum_totblks += b->tot_allocs_done;
	sum_totbyts += b->tot_bytes_alloc;

	sprintf(buf,

	    "%13s%5d:%7d %8d %7d %8d %8s %7d %8d",

	    b->file,
	    b->line,

	    b->live_blocks,
	    b->live_bytes,

	    b->max_blocks,
	    b->max_bytes,

	    CrT_timestr(b->max_bytes_time),

	    b->tot_allocs_done,
	    b->tot_bytes_alloc
	);
	X(buf);
    }
    sprintf(buf,
	" Cumulative totals:%7d %8d                           %7d %8d",
	sum_blks   , sum_byts,
	sum_totblks, sum_totbyts
    );
    X(buf);
}

static void summarize_notify( char* t ) {
    notify( summarize_player, t );
}

void CrT_summarize(
    dbref player
) {
    summarize_player = player;
    summarize( summarize_notify );
}

static FILE* summarize_fd;

static void summarize_to_file( char* t ) {
    fprintf( summarize_fd, "%s\n", t );
}

void CrT_summarize_to_file(
    const char *file,
    const char *comment
) {
    if ((summarize_fd = fopen(file, "a")) == 0) return;
    if (comment && *comment) {
	fprintf(summarize_fd, "%s\n", comment);
    }

    {   time_t lt = current_systime;
	fprintf(summarize_fd, "%s", ctime(&lt));
    }

    summarize( summarize_to_file );

    fclose( summarize_fd );
}


#ifdef CRT_DEBUG_ALSO

static void
crash2(
    char* err
) {
    abort();
}

static void
crash(
    char*       err,
    Header      m,
    const char* file,
    int         line
) {
    char buf[ 256 ];
    sprintf(buf,
	"Err found at %s:%d in block from %s:%d",
	file,             line,
	m->b->file, m->b->line
    );
    crash2(buf);	/* Makes above easy to read from dbx 'where'.	*/
}

static void
check_block(
    Header       m,
    const char*  file,
    int          line
) {
    if (*m->end  != CRT_MAGIC)   crash("Bottom overwritten", m, file, line );
    if ( m->next->prev != m  )   crash("next->prev != m"   , m, file, line );
    if ( m->prev->next != m  )   crash("prev->next != m"   , m, file, line );
}

static void
check_old_blocks(
    const char* file,
    int         line
) {
    int i = CRT_OLD_TO_CHECK;
    while (i --> 0) {
	check_block( rover, file, line );
	rover = rover->next;
    }
}

static void check_new_blocks(
    const char* file,
    int         line
) {
    int i = CRT_NEW_TO_CHECK;
    while (i --> 0)   check_block( just_touched[i], file, line );
}

void CrT_check(
    const char* file,
    int         line
) {
    check_old_blocks( file, line );
    check_new_blocks( file, line );
}

int CrT_check_everything(
    const char* file,
    int         line
) {
    Header m = root.next;
    int    i = 0;;
    for ( ;  m != &root;  ++i, m = m->next)   check_block( m, file, line );
    return i;
}

#endif /* CRT_DEUG_ALSO */

void* CrT_malloc(
    size_t        size,
    const char *  file,
    int           line
) {
    Block  b = block_find( file, line );

    Header m = (Header) malloc( size + CRT_OVERHEAD_BYTES );

    if (!m) {
        fprintf(stderr, "CrT_malloc(): Out of Memory!\n");
        abort();
    }

    m->b    = b;
    m->size = size;



#ifdef CRT_DEBUG_ALSO
    /* Look around for trashed ram blocks: */
    CrT_check(file,line);

    /* Remember where end of block is: */
    m->end  = &((char*)m)[ size + (CRT_OVERHEAD_BYTES-1) ];

    /* Write a sacred value at end of block: */
   *m->end  = CRT_MAGIC;

    /* Thread m into linklist of allocated blocks: */
    INSERT(m);

    /* Remember we've touched 'm' recently: */
    just_touched[ ++next_touched & (CRT_NEW_TO_CHECK-1) ] = m;

#endif


    b->tot_bytes_alloc += size;
    b->tot_allocs_done ++;
    b->live_blocks     ++;
    b->live_bytes      += size;

    if (b->live_bytes > b->max_bytes) {
	b->max_bytes = b->live_bytes;
	b->max_bytes_time = current_systime;
    }
    if (b->live_blocks > b->max_blocks)
	b->max_blocks = b->live_blocks;

    return (void*) (m+1);
}

void* CrT_calloc(
    size_t        num,
    size_t        siz,
    const char *  file,
    int           line
)
{
    size_t size = siz*num;
    Block  b    = block_find( file, line );
    Header m    = (Header) calloc( size+CRT_OVERHEAD_BYTES, 1 );

    if (!m) {
        fprintf(stderr, "CrT_calloc(): Out of Memory!\n");
        abort();
    }

    m->b    = b;
    m->size = size;



#ifdef CRT_DEBUG_ALSO

    CrT_check(file,line);

    m->end  = &((char*)m)[ size + (CRT_OVERHEAD_BYTES-1) ];

   *m->end  = CRT_MAGIC;

    INSERT(m);

    just_touched[ ++next_touched & (CRT_NEW_TO_CHECK-1) ] = m;

#endif



    b->tot_bytes_alloc += size;
    b->tot_allocs_done ++;
    b->live_blocks     ++;
    b->live_bytes      += size;

    if (b->live_bytes > b->max_bytes) {
	b->max_bytes = b->live_bytes;
	b->max_bytes_time = current_systime;
    }
    if (b->live_blocks > b->max_blocks)
	b->max_blocks = b->live_blocks;

    return (void*) (m+1);
}

void* CrT_realloc(
    void *        p,
    size_t        size,
    const char *  file,
    int           line
) {
    Header m = ((Header) p) -1;
    Block  b = m->b;

#ifdef CRT_DEBUG_ALSO
    check_block(m,__FILE__,__LINE__);
    CrT_check(file,line);
    if (rover == m) {
	rover = rover->next;
    }
    DELETE(m);
    {   int i = CRT_NEW_TO_CHECK;
	while (i --> 0) {
	    if (just_touched[i] == m) {
		just_touched[i] = &root;
    }	}   }
#endif

    m        =  (Header) realloc(   m,   size + CRT_OVERHEAD_BYTES  );

    if (!m) {
        fprintf(stderr, "CrT_realloc(): Out of Memory!\n");
        abort();
    }

    b->live_bytes -= m->size;
    b->live_bytes +=    size;

    m->size        =    size;

#ifdef CRT_DEBUG_ALSO

    m->end  = &((char*)m)[ size + (CRT_OVERHEAD_BYTES-1) ];

   *m->end  = CRT_MAGIC;

    INSERT(m);

    just_touched[ ++next_touched & (CRT_NEW_TO_CHECK-1) ] = m;

#endif



    if (b->live_bytes > b->max_bytes) {
	b->max_bytes = b->live_bytes;
	b->max_bytes_time = current_systime;
    }

    return (void*) (m+1);
}

void CrT_free(
    void *       p,
    const char * file,
    int          line
) {
    Header m = ((Header) p) -1;
    Block  b = m->b;

#ifdef CRT_DEBUG_ALSO
    if (*m->end == CRT_FREE_MAGIC) crash("Duplicate free()", m, file, line );

    check_block(m,__FILE__,__LINE__);
    CrT_check(file,line);
    if (rover == m) {
	rover =  rover->next;
    }
    DELETE(m);
    {   int i = CRT_NEW_TO_CHECK;
	while (i --> 0) {
	    if (just_touched[i] == m) {
		just_touched[i] = &root;
    }	}   }
    *m->end = CRT_FREE_MAGIC;
#endif

    b->live_bytes  -= m->size;
    b->live_blocks --;

    free(m);
}

char   *
CrT_alloc_string(const char *string, const char *file, int line)
{
    char   *s;

    /* NULL, "" -> NULL */
    if (!string || !*string)
	return 0;

    if ((s = (char *) CrT_malloc(strlen(string) + 1, file, line)) == 0) {
	abort();
    }
    strcpy(s, string);
    return s;
}

char   *
CrT_string_dup(const char *s, const char *file, int line)
{
    char   *p;

    p = (char *) CrT_malloc(1 + strlen(s), file, line);
    if (p) strcpy(p, s);
    return (p);
}

#endif /* MALLOC_PROFILING */

