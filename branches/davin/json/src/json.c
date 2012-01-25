#include "copyright.h"
#include "config.h"
/* --- */
#ifdef JSON_SUPPORT
#include <jansson.h>
#include "db.h"
#include "tune.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "strings.h"
#include "interp.h"
#include "json.h"

/*                  MUCK<->JSON serialization by Brevantes


   JSON encapsulation handled via Petri Lehtinen's Jansson library. Good stuff:
   http://www.digip.org/jansson/doc/2.2/apiref.html
   
   This library implements bi-directional serialization of MUCK data and JSON.
   JSON is an implementation neutral way of serializing objects, and is capable
   of representing the majority of prop-safe data in a way that the rest of the
   internet can understand it. Uses of JSON serialization include:

   + communicating with JSON-aware webservices (or implementing your own)
   + transferring props (or entire objects) from one game to another via socket
   + DB offloading of proptrees/objects to flat files for archival purposes,
     without having to write your own storage implementation.

   MUCK list arrays are JSON "arrays".

   MUCK dictionaries are JSON "objects".

   MUCK ints and floats are both handled. This explains the JSON encoding best:
   http://www.digip.org/jansson/doc/2.2/apiref.html#number

   Strings are JSON strings, but we have to strip out any non-ASCII characters
   that we don't interpret as a custom encoding of MUCK-specific data types.
   (dbrefs and locks, see below)

   JSON "null" is #-1, JSON "false" is #-10, JSON "true" is #-11. Nothing has
   been changed to give these dbrefs any additional meaning inside of props
   or on the stacks, other than reserving #-10 and #-11 for this purpose inside
   of inc/db.h. (in other words, they remain stack legal and prop illegal)

   Dbrefs are encoded as strings with a prefix of Unicode characters in the
   private use area of the Unicode spec. This means that the prefix will almost
   certainly be larger (in bytes) than the unencoded dbref, it also means that
   an ACCIDENTAL collision is improbable. Deliberate collisions should not be
   interpreted as naughty behavior, but as conscious attempt to encode a MUCK
   object and treated appropriately. Specifics about the byte encoding can be
   found in inc/json.h.

   Locks aren't implemented yet but will probably be encoded similarly.


   Limitations:
   + Haven't worked out an approach I like for serializing lock props just yet.
   + MUCK doesn't have a solution for descriptors opting-in to UTF-8 messages
     yet. Rendering these to player descriptors may result in undesirable
     client rendering behavior. That is outside the scope of this library.
   + Compression isn't in the JSON standard. Implement it in your transport
     layer if you need it.

   Things to consider:
   + Making this code less monolithic; chunks of code from the tree building
     functions could be broken out for more general use. Right now json_t 
     objects are extremely short lived (just long enough to encode or decode
     them to another format), so there's not much value in doing so just yet.

   Things that probably won't happen:
   + JSON objects on the stack. Existing stack constructs make excellent analogs
     to JSON types and there's no real need for it. At most, all I can see doing
     is implementing a selective "shadowing" (opt-in) of existing stack
     structures with pointers to equivilant JSON objects. I'd need to see a
     purpose for it before bothering; right now the only gain I see is that it
     would avoid having to assemble a tree from scratch every time.

*/

/* get_json_error is used to package the values contained in a json_error_t
   struct into a MUCK array that can be returned to the stack. */
stk_array *
get_json_error(json_error_t *error) {
    struct inst temp1, temp2;
    stk_array *errarr = new_array_dictionary();

    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("line");
    temp2.type = PROG_INTEGER;
    temp2.data.number = error->line;
    array_setitem(&errarr, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);

    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("column");
    temp2.type = PROG_INTEGER;
    temp2.data.number = error->column;
    array_setitem(&errarr, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);

    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("position");
    temp2.type = PROG_INTEGER;
    temp2.data.number = error->position;
    array_setitem(&errarr, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);

    /* not terribly useful after some testing; returns '<stream>' for files
       and '<string>' for reading from a buffer. we already know that stuff
       based on the prim we're using.
    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("source");
    temp2.type = PROG_STRING;
    temp2.data.string = alloc_prog_string(error->source);
    array_setitem(&errarr, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);
    */

    temp1.type = PROG_STRING;
    temp1.data.string = alloc_prog_string("text");
    temp2.type = PROG_STRING;
    temp2.data.string = alloc_prog_string(error->text);
    array_setitem(&errarr, &temp1, &temp2);
    CLEAR(&temp1);
    CLEAR(&temp2);

    return errarr;
} 

json_t *
array_to_json(stk_array *arr, dbref player) {
    char buf[BUFFER_LEN];
    const char *lckptr;

    json_t *jitem;      /* JSON representation of an item in a MUCK array */
    json_t *jroot;      /* root JSON node */

    /* backfill queue: used for filling in the contents of leaf node arrays
       after we've finished looping through the contents of the arrays that
       contained them. */
    json_queue *qcur = (json_queue *) malloc(sizeof(json_queue));
    json_queue *qtail;
    json_queue *qprev = NULL;

    /* MUCK array iteration */
    array_iter arrkey;
    array_data *arritem;

    /* constant for unsupported stack objects - this will probably be replaced
       with its own hash table if I implement string representation of them */
    json_t *unsupported = json_string("<unsupported stack type>");

    /* hash tables for identifying duplicate objects */
    json_t *strdict = json_object();
    json_t *intdict = json_object();
    json_t *fltdict = json_object();
    json_t *refdict = json_object();
    json_t *lckdict = json_object();
    json_t *hashref;


    /* set up the first iteration of the loop */
    qcur->nxt = NULL;
    qcur->mnode = arr;
    qtail = qcur;
    
    switch (arr->type) {
        case (ARRAY_PACKED):
            jroot = qcur->jnode = json_array();
            break;
        case (ARRAY_DICTIONARY):
            jroot = qcur->jnode = json_object();
            break;
        default:
            return NULL;
    }

    /* do it */
    do {
        if (qprev) {
            free(qprev);
        }
        if (array_first(qcur->mnode, &arrkey)) {
            do {
                arritem = array_getitem(qcur->mnode, &arrkey);
                switch (arritem->type) {
                    /* JSON container types */
                    case (PROG_ARRAY):
                        switch (arritem->data.array->type) {
                            case (ARRAY_PACKED):
                                jitem = json_array();
                                break;
                            case (ARRAY_DICTIONARY):
                                jitem = json_object();
                                break;
                            default:
                                return NULL;
                        }
                        qtail->nxt = (json_queue *) malloc(sizeof(json_queue));
                        qtail->nxt->jnode = jitem;
                        qtail->nxt->mnode = arritem->data.array;
                        qtail->nxt->nxt = NULL;
                        qtail = qtail->nxt;
                        break;
                    /* JSON number types */
                    case (PROG_INTEGER):
                        sprintf(buf, "%d", arritem->data.number);
                        if ((hashref = json_object_get(intdict, buf))) {
                            jitem = hashref;
                        } else {
                            jitem = json_integer(arritem->data.number);
                            json_object_set_new(fltdict, buf, jitem);
                        }
                        break;
                    case (PROG_FLOAT):
                        sprintf(buf, "%f", arritem->data.fnumber);
                        if ((hashref = json_object_get(fltdict, buf))) {
                            jitem = hashref;
                        } else {
                            jitem = json_real(arritem->data.fnumber);
                            json_object_set_new(fltdict, buf, jitem);
                        }
                        break;
                    /* JSON string types */
                    case (PROG_STRING):
                        sprintf(buf, "%s", DoNullInd(arritem->data.string));
                        if ((hashref = json_object_get(strdict, buf))) {
                            jitem = hashref;
                        } else {
                            jitem = json_string(DoNullInd(arritem->data.string));
                            json_object_set_new(strdict, buf, jitem);
                        }
                        break;
                    case (PROG_OBJECT):
                        switch (arritem->data.objref) {
                            /* boolean values are constants in JSON,
                               so don't bother hashing them */
                            case (NOTHING):
                                jitem = json_null();
                                break;
                            case (JFALSE):
                                jitem = json_false();
                                break;
                            case (JTRUE):
                                jitem = json_true();
                                break;
                            default:
                                sprintf(buf, JSON_PREFIX_DBREF "%d",
                                        arritem->data.objref);
                                if ((hashref = json_object_get(refdict, buf + sizeof(JSON_PREFIX_DBREF - 1)))) {
                                    jitem = hashref;
                                } else {
                                    jitem = json_string(buf);
                                    json_object_set_new(refdict, buf + sizeof(JSON_PREFIX_DBREF - 1), jitem);
                                }
                                break;
                        }
                        break;
                    case (PROG_LOCK):
                        lckptr = unparse_boolexp(player, arritem->data.lock, 0);
                        sprintf(buf, JSON_PREFIX_LOCK "%s", lckptr);

                        if ((hashref = json_object_get(lckdict, buf + sizeof(JSON_PREFIX_LOCK - 1)))) {
                            jitem = hashref;
                        } else {
                            jitem = json_string(buf);
                            json_object_set_new(lckdict, buf + sizeof(JSON_PREFIX_LOCK - 1), jitem);
                        }
                    default:
                        /* TODO: return a text representation */
                        jitem = unsupported;
                }
                /* jitem now contains a new JSON node, store it */
                switch (qcur->mnode->type) {
                    case (ARRAY_PACKED):
                        json_array_append(qcur->jnode, jitem);
                        break;
                    case (ARRAY_DICTIONARY):
                        json_object_set(qcur->jnode,
                                        DoNullInd(arrkey.data.string), jitem);
                        break;
                }
            /* get next item in current MUF array */    
            } while (array_next(qcur->mnode, &arrkey));
        /* done with the current MUF array, backtrack to the next one */
        qprev = qcur;
        } 
    } while((qcur = qcur->nxt));

    if (qprev) {
        free(qprev);
    }

    /* de-allocates our "unsupported" constant if nothing ended up using it */
    json_decref(unsupported);

    /* de-allocates JSON references held by the hash tables */
    json_decref(strdict);
    json_decref(intdict);
    json_decref(fltdict);
    json_decref(refdict);
    json_decref(lckdict);
    
    return jroot;
}

/* search_jcontainer_cache is used for deduping leaf-node containers when a JSON
   container is being decoded. Duplicate containers will be imported as linked
   stk_arrays of the appropriate type.
   
   "cache" should be a pointer to a newly malloc'd json_queue struct on its
   first run, otherwise it should be a pointer to the first item in the queue. 

   "container" is the object to scan the queue for. If a pointer to this object
   does not exist in the queue *and* it has a reference count >= "numrefs"
   (described below), it will be added to the end of the cache.

   "numrefs" is the minimum number of references to "container" that should be
   considered when deciding whether or not to add it to the queue. This allows
   the cache to skip items that don't need to be considered. A freshly
   constructed JSON tree (i.e. imported from a decode) will have a reference
   count of 1 for of the json_t objects within it, so "2" is a safe default to
   use unless you've copied any of the container objects before we got here
   (particularly if you copied the root node).

   ----

   Returns a null pointer if "numrefs" is <2, since it would defeat the purpose
   of using a cache entirely.

   If numrefs >=2, a pointer to a cache entry will be returned. The calling
   function will need to check for the presence of an allocated "mnode" member;
   if it contains a pointer, that's the address of the stk_array that should be
   be used. 

   If the mnode member is empty the item was not in the cache, and a pointer to
   a stk_array that should be associated with the new cache entry should be
   assigned to it.  */
json_queue *
search_jcontainer_cache(json_queue *cache, json_t *container, int numrefs) {
    json_queue *qcur;
    json_queue *qlast;

    if (!cache || ((container->refcount < numrefs) && numrefs > 1)) {
        return NULL;
    }

    qcur = cache;

    do {
        if (json_equal(qcur->jnode, container)) {
            return qcur;
        }
        qlast = qcur;
    } while ((qcur = qcur->nxt));

    qlast->nxt = qcur = (json_queue *) malloc(sizeof(json_queue));
 
    qcur->jnode = container;
    qcur->mnode = NULL;

    return qcur;
}

/* json_to_array converts JSON object *jroot to a MUCK array (list type or
   dictionary depending on the type of JSON root node), and returns a reference
   to the new array.
 
   "minrefs" is the integer passed to search_jcontainer_queue. Refer to the
   comments for that function, but 2 is a safe default for freshly built json_t
   trees that haven't been copied. Values <2 disable it.

   Even if container deduplication is disabled, duplicate strings will *always*
   be identified. */ 
stk_array *
json_to_array(json_t *jroot, int minrefs) {
    char buf[BUFFER_LEN];
    char *bufptr;
    char *tailptr;       /* used for importing dbrefs */
    long int newint;      /* used for importing dbrefs */

    json_t *jitem;        /* JSON structure */ 
    stk_array *mroot;     /* root of the MUF array we're decoding to */
    struct inst mitem;    /* MUCK representation of a JSON structure */
    struct inst temp1;
    void *jiter = NULL;   /* Jansson iterator for JSON objects */
    json_t *jnode_cur;
    stk_array *mnode_cur;
    char sha1[41];

    /* backfill queue: used for filling in the contents of leaf node arrays
       after we've finished looping through the contents of the arrays that
       contained them. */
    json_queue *qcur = (json_queue *) malloc(sizeof(json_queue));
    json_queue *qtail;
    json_queue *qprev = NULL;
    array_tree *mtree;    /* pointer to an array_tree being backfilled */


    int jidx = 0; /* for JSON arrays; current index being operated on. */


    /* hash tables for identifying duplicate objects */
    stk_array *strdict = new_array_dictionary();
    struct inst *hashref;

    json_queue *jarrcache = NULL;
    json_queue *jobjcache = NULL;

    if (minrefs > 1 ) {
        jarrcache = (json_queue *) malloc(sizeof(json_queue));
        jarrcache->mnode = NULL;
        jarrcache->nxt = NULL;
        jobjcache = (json_queue *) malloc(sizeof(json_queue));
        jobjcache->mnode = NULL;
        jobjcache->nxt = NULL;
    }

    json_queue *cacheref;

    /* set up the first iteration of the loop */
    qcur->nxt = NULL;
    qcur->jnode = jroot;
    qtail = qcur;
    
    switch (json_typeof(jroot)) {
        case (JSON_ARRAY):
            mroot = qcur->mnode = new_array_packed(json_array_size(jroot));
            break;
        case (JSON_OBJECT):
            mroot = qcur->mnode = new_array_dictionary();
            break;
        default:
            return NULL;
    }

    /* do it */
    do {
        if (qprev) {
            free(qprev);
        }

        /* set up the first iteration */
        switch (json_typeof(qcur->jnode)) {
            case (JSON_ARRAY):
                jidx = 0;
                jitem = json_array_get(qcur->jnode, 0);
                break;
            case (JSON_OBJECT):
                jiter = json_object_iter(qcur->jnode);
                jitem = json_object_iter_value(jiter);
                break;
            default:
                /* never reached */
                break;
        }

        if (jitem) {
            do {
                mnode_cur = qcur->mnode;
                jnode_cur = qcur->jnode;
                switch (json_typeof(jitem)) {
                    /* JSON container types */
                    case (JSON_ARRAY):
                        mitem.type = PROG_ARRAY;
                        if ((cacheref =
                             search_jcontainer_cache(jarrcache,jitem,minrefs))) {
                            if (cacheref->mnode) {
                                /* if we got here, then we matched a cached
                                   JSON container. re-use the last stk_array
                                   we created for it, and *don't* add it to the
                                   end of the processing queue. */
                                temp1.type = PROG_ARRAY;
                                temp1.data.array = cacheref->mnode;
                                copyinst(&temp1, &mitem);
                                temp1.data.array = NULL;
                            } else {
                                /* cache wasn't hit, but this needs to be added
                                   to the cache. */
                                mitem.data.array = new_array_packed(json_array_size(jitem));
                                cacheref->mnode = mitem.data.array;

                                qtail->nxt = (json_queue *) malloc(sizeof(json_queue));
                                qtail->nxt->jnode = jitem;
                                qtail->nxt->mnode = mitem.data.array;
                                qtail->nxt->nxt = NULL;
                                qtail = qtail->nxt;
                            }
                        } else {
                            /* do nothing with the container queue this round */
                            mitem.data.array = new_array_packed(json_array_size(jitem));

                            qtail->nxt = (json_queue *) malloc(sizeof(json_queue));
                            qtail->nxt->jnode = jitem;
                            qtail->nxt->mnode = mitem.data.array;
                            qtail->nxt->nxt = NULL;
                            qtail = qtail->nxt;
                        }
                        break;
                    case (JSON_OBJECT):
                        mitem.type = PROG_ARRAY;
                        if ((cacheref =
                             search_jcontainer_cache(jobjcache,jitem,minrefs))) {
                            if (cacheref->mnode) {
                                /* if we got here, then we matched a cached
                                   JSON container. re-use the last stk_array
                                   we created for it, and *don't* add it to the
                                   end of the processing queue. */
                                temp1.type = PROG_ARRAY;
                                temp1.data.array = cacheref->mnode;
                                copyinst(&temp1, &mitem);
                                temp1.data.array = NULL;
                            } else {
                                /* cache wasn't hit, but this needs to be added
                                   to the cache. */
                                mitem.data.array = new_array_dictionary();
                                cacheref->mnode = mitem.data.array;

                                qtail->nxt = (json_queue *) malloc(sizeof(json_queue));
                                qtail->nxt->jnode = jitem;
                                qtail->nxt->mnode = mitem.data.array;
                                qtail->nxt->nxt = NULL;
                                qtail = qtail->nxt;
                            }
                        } else {
                            /* do nothing with the container queue this round */
                            mitem.data.array = new_array_dictionary();

                            qtail->nxt = (json_queue *) malloc(sizeof(json_queue));
                            qtail->nxt->jnode = jitem;
                            qtail->nxt->mnode = mitem.data.array;
                            qtail->nxt->nxt = NULL;
                            qtail = qtail->nxt;
                        }
                        break;
                    /* JSON number types - no MUCK refcounts for these */
                    case (JSON_INTEGER):
                        mitem.type = PROG_INTEGER;
                        mitem.data.number = json_integer_value(jitem);
                        break;
                    case (JSON_REAL):
                        mitem.type = PROG_FLOAT;
                        mitem.data.fnumber = json_real_value(jitem);
                        break;
                    /* JSON boolean types - these are constants */
                    case (JSON_NULL):
                        mitem.type = PROG_OBJECT;
                        mitem.data.objref = NOTHING;
                        break;
                    case (JSON_TRUE):
                        mitem.type = PROG_OBJECT;
                        mitem.data.objref = JTRUE;
                        break;
                    case (JSON_FALSE):
                        mitem.type = PROG_OBJECT;
                        mitem.data.objref = JFALSE;
                        break;
                    /* JSON string types - these map either to MUCK strings or
                       special data types that we've encoded. MUCK doesn't track
                       refcounts for dbrefs, but does for strings and locks. */
                    case (JSON_STRING):
                        strcpy(buf, json_string_value(jitem));
                        /* is this encoded as an integer? */
                        if ( memcmp((const char *)buf,
                                     (const char *)JSON_PREFIX_DBREF,
                                      sizeof(JSON_PREFIX_DBREF)) == 0 ) {
                            /* seems to be a dbref, does the rest check out? */
                            bufptr = (char *) buf + (sizeof(JSON_PREFIX_DBREF) - 1);
                            /* import the rest of the string as base 10 */
                            newint = strtol(bufptr, &tailptr, 10);
                            /* any trailing data means this is bogus */
                            if (newint && (*tailptr == '\0')) {
                                /* we consider #-1, #-10, and #-11 to be out of
                                   band because they encode to the NULL, FALSE,
                                   and TRUE JSON types */
                                if ( newint != -1 && newint != -10 &&
                                        newint != -11 ) {
                                    mitem.type = PROG_OBJECT;
                                    mitem.data.objref = (dbref) newint; 
                                    break;
                                }
                            }
                        }
                        /* is this encoded as a lock? */
                        if ( memcmp((const char *)buf,
                                     (const char *)JSON_PREFIX_LOCK,
                                      sizeof(JSON_PREFIX_LOCK)) == 0 ) {
                            /* seems to be a dbref, does the rest check out? */
                            bufptr = (char *) buf + (sizeof(JSON_PREFIX_LOCK) - 1);
                            /* import the rest of the string as base 10 */
                            newint = strtol(bufptr, &tailptr, 10);
                            /* any trailing data means this is bogus */
                            if (newint && (*tailptr == '\0')) {
                                /* we consider #-1, #-10, and #-11 to be out of
                                   band because they encode to the NULL, FALSE,
                                   and TRUE JSON types */
                                if ( newint != -1 && newint != -10 &&
                                        newint != -11 ) {
                                    mitem.type = PROG_OBJECT;
                                    mitem.data.objref = (dbref) newint; 
                                    break;
                                }
                            }
                        }
                        if (strlen(buf) >= 42) {
                            /* hash long strings to a shorter, unique entity */
                            SHA1hex((char *)sha1, buf, strlen(buf));
                            temp1.type = PROG_STRING;
                            temp1.data.string = alloc_prog_string(sha1);
                        } else {
                            /* use string as it is */
                            temp1.type = PROG_STRING;
                            temp1.data.string = alloc_prog_string(buf);
                        }
                        if ((hashref = array_getitem(strdict, &temp1))) {
                            mitem = *hashref;
                        } else {
                            copyinst(&temp1, &mitem);
                            array_setitem(&strdict, &temp1, &mitem);
                        }
                        CLEAR(&temp1);
                        break;
                    default:
                        /* should never get here */
                        return NULL;
                }

                /* store the object we just decoded in the array the current
                   loop is processing. these array packing routines are
                   slightly tweaked from array_setitem in array.c, and are
                   designed with serial packing in mind. list arrays must not
                   exceed the item count they were instantiated with, and
                   dictionary items must always extend the current array by one
                   key. */
                switch (mnode_cur->type) {
                    case (ARRAY_PACKED):
                        copyinst(&mitem, &mnode_cur->data.packed[jidx]);
                        break;
                    case (ARRAY_DICTIONARY):
                        strcpy(buf,json_object_iter_key(jiter));
                        temp1.type = PROG_STRING;
                        temp1.data.string = alloc_prog_string(buf);
                        qcur->mnode->items++;
                        mtree = array_tree_insert(&(mnode_cur->data.dict), &temp1);
                        copyinst(&mitem, &mtree->data);
                        CLEAR(&temp1);
                        break;
                }
                CLEAR(&mitem);
                /* get next item in current JSON container */    
                switch (json_typeof(jnode_cur)) {
                    case (JSON_ARRAY):
                        jitem = json_array_get(jnode_cur, ++jidx);
                        break;
                    case (JSON_OBJECT):
                        jiter = json_object_iter_next(jnode_cur, jiter);
                        jitem = json_object_iter_value(jiter);
                        break;
                    default:
                        /* never reached */
                        break;
                }
            } while (jitem);
        /* done with the current MUF array, backtrack to the next one */
        qprev = qcur;
        } 
    } while((qcur = qcur->nxt));

    if (qprev) {
        free(qprev);
    }

    if (jarrcache) {
        qcur = jarrcache;
        jarrcache = qcur->nxt;
        do {
            free(qcur);
            qcur = jarrcache;
        } while (qcur && (jarrcache = qcur->nxt));
    } 
    if (jobjcache) {
        qcur = jobjcache;
        jobjcache = qcur->nxt;
        do {
            free(qcur);
            qcur = jobjcache;
        } while (qcur && (jobjcache = qcur->nxt));
    } 

    if (strdict) {
        temp1.type = PROG_ARRAY;
        temp1.data.array = strdict;
        CLEAR(&temp1);
    }

    return mroot;
}

#endif /* JSON_SUPPORT */
