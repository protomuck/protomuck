#include "copyright.h"
#include "config.h"

#define MALLOC(result, type, number) do {   \
                                       if (!((result) = (type *) malloc ((number) * sizeof (type)))) \
                                       abort();                             \
                                     } while (0)

#define FREE(x) (free((void *) x))


struct hostcache {
    int ipnum;
    char name[128];
    struct hostcache *next;
    struct hostcache **prev;
} *hostcache_list = NULL;

void
host_del(int ip)
{
    struct hostcache *ptr;
    for (ptr = hostcache_list; ptr; ptr = ptr->next) {
        if (ptr->ipnum == ip) {
            if (ptr->next) {
                ptr->next->prev = ptr->prev;
	    }
	    *ptr->prev = ptr->next;
	    FREE(ptr);
	    return;
        }
    }
}

char *
host_fetch(int ip)
{
    struct hostcache *ptr;

    for (ptr = hostcache_list; ptr; ptr = ptr->next) {
        if (ptr->ipnum == ip) {
            if (ptr != hostcache_list) {
                *ptr->prev = ptr->next;
                if (ptr->next) {
                    ptr->next->prev = ptr->prev;
                }
                ptr->next = hostcache_list;
                if (ptr->next) {
                    ptr->next->prev = &ptr->next;
                }
                ptr->prev = &hostcache_list;
                hostcache_list = ptr;
            }
            return (ptr->name);
        }
    }
    return NULL;
}

void
host_add(int ip, const char *name)
{
    struct hostcache *ptr;

    MALLOC(ptr, struct hostcache, 1);
    ptr->next = hostcache_list;
    if (ptr->next) {
        ptr->next->prev = &ptr->next;
    }
    ptr->prev = &hostcache_list;
    hostcache_list = ptr;
    ptr->ipnum = ip;
    strcpy(ptr->name, name);
}

void
host_free()
{
    struct hostcache *next, *list;

    if( !hostcache_list ) return;

    list = hostcache_list;
    hostcache_list = NULL;

    while( list ) {
	next = list->next;
	FREE( list );
	list = next;
    }
}

void
host_load()
{
    FILE *f;
    int ip;
    char name[80];
    char *p = name;

    if(!( f = fopen( "nethost.cache", "r" ))) return;

    if( hostcache_list ) host_free();

    while( fscanf( f, "%x %s\n", &ip, p ) == 2 )
	host_add(ip, name);	

    fclose( f );
}

void
host_save()
{
    FILE *f;
    struct hostcache *ptr;

    if(!( f = fopen( "nethost.cache", "w" ))) return;

    for (ptr = hostcache_list; ptr; ptr = ptr->next)
	fprintf( f, "%X %s\n", ptr->ipnum, ptr->name );

    fclose( f );
}

void
host_init()
{
    host_load();
}

void
host_shutdown()
{
    host_save();
    host_free();
}
