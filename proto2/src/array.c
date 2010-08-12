/* array.c is where all of the behind the scenes work is done for
 * MUF array support. Making the arrays, balancing the trees,
 * sorting them, and clearing them properly are all handled
 * here. It was adopted into ProtoMuck from FB6, though slight
 * differences exist between FB's version and Proto's. - Akari
 */

/* Primitives Package */

#include "copyright.h"
#include "config.h"
/* --- */
#include "db.h"
#include "tune.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "strings.h"
#include "interp.h"


/*
  AVL binary tree code by Lynx (or his instructor)

  Modified for MUCK use by Sthiss
  Remodified by Revar
*/

#ifndef AVL_RT
#define AVL_RT(x)  (x->right)
#endif
#ifndef AVL_LF
#define AVL_LF(x)  (x->left)
#endif
#define AVL_KEY(x) (&(x->key))
#define AVL_COMPARE(x,y) array_tree_compare(x,y,0, 0)
#define valid_obj(a) (a > -1 && a < db_top)


/* This key function helps sort dictionary arrays by comparing the
 * key values, and also sorting list arrays by comparing the data
 * values. It differs slightly from the FB6 implementaton in that
 * it sorts dbref objects by their name rather than dbref#. 
 * The 'int objname' parameter toggles this behavior. When it is 0
 * dbrefs are sorted by number instead of the object name. -Akari
 */
static int
array_tree_compare(array_iter *a, array_iter *b, int case_sens, int objname)
{
    char pad_char[] = "";

    if (a->type != b->type) {
        if (a->type == PROG_INTEGER && b->type == PROG_FLOAT) {
            if (fabs
                (((double) a->data.number -
                  b->data.fnumber) / (double) a->data.number) < DBL_EPSILON) {
                return 0;
            } else if (a->data.number > b->data.fnumber) {
                return 1;
            } else {
                return -1;
            }
        } else if (a->type == PROG_FLOAT && b->type == PROG_INTEGER) {
            if (a->data.fnumber == b->data.fnumber) {
                return 0;
            } else
                if (fabs((a->data.fnumber - b->data.number) / a->data.fnumber) <
                    DBL_EPSILON) {
                return 0;
            } else if (a->data.fnumber > b->data.number) {
                return 1;
            } else {
                return -1;
            }
        } else if (a->type == PROG_OBJECT && b->type == PROG_STRING
                   && objname && valid_obj(a->data.objref)) {
            char *astr = (char *) NAME(a->data.objref);
            char *bstr = (b->data.string) ? b->data.string->data : pad_char;

            if (case_sens) {
                return strcmp(astr, bstr);
            } else {
                return string_compare(astr, bstr);
            }
        } else if (a->type == PROG_STRING && b->type == PROG_OBJECT
                   && objname && valid_obj(b->data.objref)) {
            char *astr = (a->data.string) ? a->data.string->data : pad_char;
            char *bstr = (char *) NAME(b->data.objref);

            if (case_sens) {
                return strcmp(astr, bstr);
            } else {
                return string_compare(astr, bstr);
            }
        }
        return (a->type - b->type);
    }
    /* Indexes are of same type if we reached here. */
    if (a->type == PROG_OBJECT && objname && valid_obj(a->data.objref)
        && valid_obj(b->data.objref)) {
        char *astr = (char *) NAME(a->data.objref);
        char *bstr = (char *) NAME(b->data.objref);

        if (case_sens) {
            return strcmp(astr, bstr);
        } else {
            return string_compare(astr, bstr);
        }
    }
    if (a->type == PROG_OBJECT) {
        if (a->data.objref > b->data.objref)
            return 1;
        else if (a->data.objref < b->data.objref)
            return -1;
        else
            return 0;
    }
    if (a->type == PROG_FLOAT) {
        if (a->data.fnumber == b->data.fnumber) {
            return 0;
        } else
            if (fabs((a->data.fnumber - b->data.fnumber) / a->data.fnumber) <
                DBL_EPSILON) {
            return 0;
        } else if (a->data.fnumber > b->data.fnumber) {
            return 1;
        } else {
            return -1;
        }
    } else if (a->type == PROG_STRING) {
        char pad_char[] = "";
        char *astr = (a->data.string) ? a->data.string->data : pad_char;
        char *bstr = (b->data.string) ? b->data.string->data : pad_char;

        if (case_sens) {
            return strcmp(astr, bstr);
        } else {
            return string_compare(astr, bstr);
        }
    } else if (a->type == PROG_ARRAY) {
        /* Sort arrays by memory address. */
        /* This is a bug, really. */
        /* in a perfect world, we'd compare the array elements recursively. */
        return (a->data.array - b->data.array);
    } else if (a->type == PROG_LOCK) {
        /* Sort locks by memory address. */
        /* This is a bug, really. */
        /* in a perfect world, we'd compare the locks by element. */
        char *la;
        char *lb;
        int retval = 0;

        la = (char *) unparse_boolexp((dbref) 1, a->data.lock, 0);
        la = string_dup(la);
        lb = (char *) unparse_boolexp((dbref) 1, b->data.lock, 0);
        retval = strcmp(la, lb);
        free(la);
        return retval;
    } else if (a->type == PROG_ADD) {
        int result = (a->data.addr->progref - b->data.addr->progref);

        if (result) {
            return result;
        }
        return (a->data.addr->data - b->data.addr->data);
    } else {
        return (a->data.number - b->data.number);
    }
}

static array_tree *
array_tree_find(array_tree *avl, array_iter *key)
{
    int cmpval;

    while (avl) {
        cmpval = AVL_COMPARE(key, AVL_KEY(avl));
        if (cmpval > 0) {
            avl = AVL_RT(avl);
        } else if (cmpval < 0) {
            avl = AVL_LF(avl);
        } else {
            break;
        }
    }
    return avl;
}

static int
array_tree_height_of(array_tree *node)
{
    if (node != NULL)
        return node->height;
    else
        return 0;
}

static int
array_tree_height_diff(array_tree *node)
{
    if (node != NULL)
        return (array_tree_height_of(AVL_RT(node)) -
                array_tree_height_of(AVL_LF(node)));
    else
        return 0;
}

/*\
|*| Note to self: don't do : max (x++,y)
|*| Kim
\*/
#if !defined(max)
# define max(a, b)       (a > b ? a : b)
#endif

static void
array_tree_fixup_height(array_tree *node)
{
    if (node)
        node->height = (int) 1 +
            max(array_tree_height_of(AVL_LF(node)),
                array_tree_height_of(AVL_RT(node)));
}

static array_tree *
array_tree_rotate_left_single(array_tree *a)
{
    array_tree *b = AVL_RT(a);

    AVL_RT(a) = AVL_LF(b);
    AVL_LF(b) = a;

    array_tree_fixup_height(a);
    array_tree_fixup_height(b);

    return b;
}

static array_tree *
array_tree_rotate_left_double(array_tree *a)
{
    array_tree *b = AVL_RT(a);
    array_tree *c = AVL_LF(b);

    AVL_RT(a) = AVL_LF(c);
    AVL_LF(b) = AVL_RT(c);
    AVL_LF(c) = a;
    AVL_RT(c) = b;

    array_tree_fixup_height(a);
    array_tree_fixup_height(b);
    array_tree_fixup_height(c);

    return c;
}

static array_tree *
array_tree_rotate_right_single(array_tree *a)
{
    array_tree *b = AVL_LF(a);

    AVL_LF(a) = AVL_RT(b);
    AVL_RT(b) = a;

    array_tree_fixup_height(a);
    array_tree_fixup_height(b);

    return (b);
}

static array_tree *
array_tree_rotate_right_double(array_tree *a)
{
    array_tree *b = AVL_LF(a);
    array_tree *c = AVL_RT(b);

    AVL_LF(a) = AVL_RT(c);
    AVL_RT(b) = AVL_LF(c);
    AVL_RT(c) = a;
    AVL_LF(c) = b;

    array_tree_fixup_height(a);
    array_tree_fixup_height(b);
    array_tree_fixup_height(c);

    return c;
}

static array_tree *
array_tree_balance_node(array_tree *a)
{
    int dh = array_tree_height_diff(a);

    if (abs(dh) < 2) {
        array_tree_fixup_height(a);
    } else {
        if (dh == 2) {
            if (array_tree_height_diff(AVL_RT(a)) >= 0) {
                a = array_tree_rotate_left_single(a);
            } else {
                a = array_tree_rotate_left_double(a);
            }
        } else if (array_tree_height_diff(AVL_LF(a)) <= 0) {
            a = array_tree_rotate_right_single(a);
        } else {
            a = array_tree_rotate_right_double(a);
        }
    }
    return a;
}

array_tree *
array_tree_alloc_node(array_iter *key)
{
    array_tree *new_node;

    new_node = (array_tree *) malloc(sizeof(array_tree));
    if (!new_node) {
        fprintf(stderr, "array_tree_alloc_node(): Out of Memory!\n");
        abort();
    }

    AVL_LF(new_node) = NULL;
    AVL_RT(new_node) = NULL;
    new_node->height = 1;

    copyinst(key, AVL_KEY(new_node));
    new_node->data.type = PROG_INTEGER;
    new_node->data.data.number = 0;
    return new_node;
}

void
array_tree_free_node(array_tree *p)
{
    assert(AVL_LF(p) == NULL);
    assert(AVL_RT(p) == NULL);
    CLEAR(AVL_KEY(p));
    CLEAR(&p->data);
    free(p);
}


static array_tree *
array_tree_insert(array_tree **avl, array_iter *key)
{
    array_tree *ret;
    register array_tree *p = *avl;
    register int cmp;
    static int balancep;

    if (p) {
        cmp = AVL_COMPARE(key, AVL_KEY(p));
        if (cmp > 0) {
            ret = array_tree_insert(&(AVL_RT(p)), key);
        } else if (cmp < 0) {
            ret = array_tree_insert(&(AVL_LF(p)), key);
        } else {
            balancep = 0;
            return (p);
        }
        if (balancep) {
            *avl = array_tree_balance_node(p);
        }
        return ret;
    } else {
        p = *avl = array_tree_alloc_node(key);
        balancep = 1;
        return (p);
    }
}

static array_tree *
array_tree_getmax(array_tree *avl)
{
    if (avl && AVL_RT(avl))
        return array_tree_getmax(AVL_RT(avl));
    return avl;
}

static array_tree *
array_tree_remove_node(array_iter *key, array_tree **root)
{
    array_tree *save;
    array_tree *tmp;
    array_tree *avl = *root;
    int cmpval;

    save = avl;
    if (avl) {
        cmpval = AVL_COMPARE(key, AVL_KEY(avl));
        if (cmpval < 0) {
            save = array_tree_remove_node(key, &AVL_LF(avl));
        } else if (cmpval > 0) {
            save = array_tree_remove_node(key, &AVL_RT(avl));
        } else if (!(AVL_LF(avl))) {
            avl = AVL_RT(avl);
        } else if (!(AVL_RT(avl))) {
            avl = AVL_LF(avl);
        } else {
            tmp =
                array_tree_remove_node(AVL_KEY(array_tree_getmax(AVL_LF(avl))),
                                       &AVL_LF(avl));
            if (!tmp)
                abort();        /* this shouldn't be possible. */
            AVL_LF(tmp) = AVL_LF(avl);
            AVL_RT(tmp) = AVL_RT(avl);
            avl = tmp;
        }
        if (save) {
            AVL_LF(save) = NULL;
            AVL_RT(save) = NULL;
        }
        *root = array_tree_balance_node(avl);
    }
    return save;
}


static array_tree *
array_tree_delete(array_iter *key, array_tree *avl)
{
    array_tree *save;

    save = array_tree_remove_node(key, &avl);
    if (save)
        array_tree_free_node(save);
    return avl;
}


void
array_tree_delete_all(array_tree *p)
{
    if (!p)
        return;
    array_tree_delete_all(AVL_LF(p));
    array_tree_delete_all(AVL_RT(p));
    AVL_LF(p) = NULL;
    AVL_RT(p) = NULL;
    array_tree_free_node(p);
}


array_tree *
array_tree_first_node(array_tree *list)
{
    if (!list)
        return ((array_tree *) NULL);

    while (AVL_LF(list))
        list = AVL_LF(list);

    return (list);
}


array_tree *
array_tree_last_node(array_tree *list)
{
    if (!list)
        return NULL;

    while (AVL_RT(list))
        list = AVL_RT(list);

    return (list);
}


array_tree *
array_tree_prev_node(array_tree *ptr, array_iter *key)
{
    array_tree *from;
    int cmpval;

    if (!ptr)
        return NULL;
    if (!key)
        return NULL;
    cmpval = AVL_COMPARE(key, AVL_KEY(ptr));
    if (cmpval > 0) {
        from = array_tree_prev_node(AVL_RT(ptr), key);
        if (from)
            return from;
        return ptr;
    } else if (cmpval < 0) {
        return array_tree_prev_node(AVL_LF(ptr), key);
    } else if (AVL_LF(ptr)) {
        from = AVL_LF(ptr);
        while (AVL_RT(from))
            from = AVL_RT(from);
        return from;
    } else {
        return NULL;
    }
}



array_tree *
array_tree_next_node(array_tree *ptr, array_iter *key)
{
    array_tree *from;
    int cmpval;

    if (!ptr)
        return NULL;
    if (!key)
        return NULL;
    cmpval = AVL_COMPARE(key, AVL_KEY(ptr));
    if (cmpval < 0) {
        from = array_tree_next_node(AVL_LF(ptr), key);
        if (from)
            return from;
        return ptr;
    } else if (cmpval > 0) {
        return array_tree_next_node(AVL_RT(ptr), key);
    } else if (AVL_RT(ptr)) {
        from = AVL_RT(ptr);
        while (AVL_LF(from))
            from = AVL_LF(from);
        return from;
    } else {
        return NULL;
    }
}



/*****************************************************************
 *  Stack Array Handling Routines
 *****************************************************************/

stk_array *
new_array(void)
{
    stk_array *new2;

    new2 = (stk_array *) malloc(sizeof(stk_array));
    new2->links = 1;
    new2->items = 0;
    new2->type = ARRAY_UNDEFINED;
    new2->data.packed = NULL;

    return new2;
}


stk_array *
new_array_packed(int size)
{
    stk_array *new2;
    int i;

    if (size < 0) {
        return NULL;
    }

    new2 = new_array();
    new2->items = size;
    new2->type = ARRAY_PACKED;
    if (size < 1)
        size = 1;
    new2->data.packed = (array_data *) malloc(sizeof(array_data) * size);
    for (i = size; i-- > 0;) {
        new2->data.packed[i].type = PROG_INTEGER;
        new2->data.packed[i].line = 0;
        new2->data.packed[i].data.number = 0;
    }
    return new2;
}


stk_array *
new_array_dictionary(void)
{
    stk_array *new2;

    new2 = new_array();
    new2->type = ARRAY_DICTIONARY;
    return new2;
}


stk_array *
array_decouple(stk_array *arr)
{
    stk_array *new2;

    if (!arr) {
        return NULL;
    }

    new2 = new_array();
    new2->type = arr->type;
    switch (arr->type) {
        case ARRAY_PACKED:{
            int i;

            new2->items = arr->items;
            new2->data.packed =
                (array_data *) malloc(sizeof(array_data) * arr->items);
            for (i = arr->items; i-- > 0;) {
                copyinst(&arr->data.packed[i], &new2->data.packed[i]);
            }
            return new2;
            break;
        }

        case ARRAY_DICTIONARY:{
            array_iter idx;
            array_data *val;

            if (array_first(arr, &idx)) {
                do {
                    val = array_getitem(arr, &idx);
                    array_setitem(&new2, &idx, val);
                } while (array_next(arr, &idx));
            }
            return new2;
            break;
        }

        default:
            break;
    }
    return NULL;
}


stk_array *
array_promote(stk_array *arr)
{
    stk_array *new2;
    int i;
    array_iter idx;

    if (!arr) {
        return NULL;
    }
    if (arr->type != ARRAY_PACKED) {
        return NULL;
    }

    new2 = new_array_dictionary();

    idx.type = PROG_INTEGER;
    for (i = 0; i < arr->items; i++) {
        idx.data.number = i;
        array_setitem(&new2, &idx, array_getitem(arr, &idx));
    }
    array_free(arr);

    return new2;
}


void
array_free(stk_array *arr)
{
    if (!arr) {
        return;
    }
    arr->links--;
    if (arr->links > 0) {
        return;
    }
    switch (arr->type) {
        case ARRAY_PACKED:{
            int i;

            for (i = arr->items; i-- > 0;) {
                CLEAR(&arr->data.packed[i]);
            }
            free(arr->data.packed);
            break;
        }
        case ARRAY_DICTIONARY:
            array_tree_delete_all(arr->data.dict);

        default:{
            break;
        }
    }
    arr->items = 0;
    arr->data.packed = NULL;
    arr->type = ARRAY_UNDEFINED;
    free(arr);
}


int
array_count(stk_array *arr)
{
    if (!arr) {
        return 0;
    }
    return arr->items;
}


int
array_idxcmp(array_iter *a, array_iter *b)
{
    return array_tree_compare(a, b, 0, 0);
}

int
array_idxcmp_case(array_iter *a, array_iter *b, int case_sens, int objnames)
{
    return array_tree_compare(a, b, case_sens, objnames);
}

int
array_contains_key(stk_array *arr, array_iter *item)
{
    if (!arr || !arr->items) {
        return 0;
    }
    switch (arr->type) {
        case ARRAY_PACKED:{
            if (item->type != PROG_INTEGER) {
                return 0;
            }
            if (item->data.number >= 0 && item->data.number < arr->items) {
                return 1;
            }
            return 0;
            break;
        }
        case ARRAY_DICTIONARY:{
            if (array_tree_find(arr->data.dict, item)) {
                return 1;
            }
            return 0;
        }
        default:{
            break;
        }
    }
    return 0;
}


int
array_contains_value(stk_array *arr, array_data *item)
{
    if (!arr || !arr->items) {
        return 0;
    }
    switch (arr->type) {
        case ARRAY_PACKED:{
            int i;

            for (i = arr->items; i-- > 0;) {
                if (!array_tree_compare(&arr->data.packed[i], item, 0, 0)) {
                    return 1;
                }
            }
            return 0;
            break;
        }
        case ARRAY_DICTIONARY:{
            array_tree *p;

            p = array_tree_first_node(arr->data.dict);
            if (!p)
                return 0;
            while (p) {
                if (!array_tree_compare(&p->data, item, 0, 0)) {
                    return 1;
                }
                p = array_tree_next_node(arr->data.dict, &p->data);
            }
            return 0;
        }
        default:{
            break;
        }
    }
    return 0;
}


int
array_first(stk_array *arr, array_iter *item)
{
    if (!arr || !arr->items) {
        return 0;
    }
    switch (arr->type) {
        case ARRAY_PACKED:{
            item->type = PROG_INTEGER;
            item->data.number = 0;
            return 1;
            break;
        }
        case ARRAY_DICTIONARY:{
            array_tree *p;

            p = array_tree_first_node(arr->data.dict);
            if (!p)
                return 0;
            copyinst(&p->key, item);
            return 1;
        }
        default:{
            break;
        }
    }
    return 0;
}


int
array_last(stk_array *arr, array_iter *item)
{
    if (!arr || !arr->items) {
        return 0;
    }
    switch (arr->type) {
        case ARRAY_PACKED:{
            item->type = PROG_INTEGER;
            item->data.number = arr->items - 1;
            return 1;
            break;
        }
        case ARRAY_DICTIONARY:{
            array_tree *p;

            p = array_tree_last_node(arr->data.dict);
            if (!p)
                return 0;
            copyinst(&p->key, item);
            return 1;
        }
        default:{
            break;
        }
    }
    return 0;
}


int
array_prev(stk_array *arr, array_iter *item)
{
    if (!arr || !arr->items) {
        return 0;
    }
    switch (arr->type) {
        case ARRAY_PACKED:{
            int idx;

            if (item->type == PROG_STRING) {
                CLEAR(item);
                return 0;
            } else if (item->type == PROG_FLOAT) {
                if (item->data.fnumber >= arr->items) {
                    idx = arr->items - 1;
                } else {
                    idx = (int) (item->data.fnumber - 1.0);
                }
            } else {
                idx = item->data.number - 1;
            }
            CLEAR(item);
            if (idx >= arr->items) {
                idx = arr->items - 1;
            } else if (idx < 0) {
                return 0;
            }
            item->type = PROG_INTEGER;
            item->data.number = idx;
            return 1;
            break;
        }
        case ARRAY_DICTIONARY:{
            array_tree *p;

            p = array_tree_prev_node(arr->data.dict, item);
            CLEAR(item);
            if (!p)
                return 0;
            copyinst(&p->key, item);
            return 1;
        }
        default:{
            break;
        }
    }
    return 0;
}


int
array_next(stk_array *arr, array_iter *item)
{
    if (!arr || !arr->items) {
        return 0;
    }
    switch (arr->type) {
        case ARRAY_PACKED:{
            int idx;

            if (item->type == PROG_STRING) {
                CLEAR(item);
                return 0;
            } else if (item->type == PROG_FLOAT) {
                if (item->data.fnumber < 0.0) {
                    idx = 0;
                } else {
                    idx = (int) (item->data.fnumber + 1.0);
                }
            } else {
                idx = item->data.number + 1;
            }
            CLEAR(item);
            if (idx >= arr->items) {
                return 0;
            } else if (idx < 0) {
                idx = 0;
            }
            item->type = PROG_INTEGER;
            item->data.number = idx;
            return 1;
            break;
        }
        case ARRAY_DICTIONARY:{
            array_tree *p;

            p = array_tree_next_node(arr->data.dict, item);
            CLEAR(item);
            if (!p)
                return 0;
            copyinst(&p->key, item);
            return 1;
        }
        default:{
            break;
        }
    }
    return 0;
}


array_data *
array_getitem(stk_array *arr, array_iter *idx)
{
    if (!arr || !idx) {
        return NULL;
    }
    switch (arr->type) {
        case ARRAY_PACKED:
            if (idx->type != PROG_INTEGER) {
                return NULL;
            }
            if (idx->data.number < 0 || idx->data.number >= arr->items) {
                return NULL;
            }
            return &arr->data.packed[idx->data.number];
            break;

        case ARRAY_DICTIONARY:{
            array_tree *p;

            p = array_tree_find(arr->data.dict, idx);
            if (!p) {
                return NULL;
            }
            return &p->data;
        }

        default:
            break;
    }
    return NULL;
}


int
array_setitem(stk_array **harr, array_iter *idx, array_data *item)
{
    stk_array *arr;

    if (!harr)
        return -1;
    if (!*harr)
        return -2;
    if (!idx)
        return -3;
    arr = *harr;
    switch (arr->type) {
        case ARRAY_PACKED:{
            if (idx->type != PROG_INTEGER) {
                return -4;
            }
            if (idx->data.number >= 0 && idx->data.number < arr->items) {
                if (arr->links > 1) {
                    arr->links--;
                    arr = *harr = array_decouple(arr);
                }
                CLEAR(&arr->data.packed[idx->data.number]);
                copyinst(item, &arr->data.packed[idx->data.number]);
                return arr->items;
            } else if (idx->data.number == arr->items) {
                if (arr->links > 1) {
                    arr->links--;
                    arr = *harr = array_decouple(arr);
                }
                arr->data.packed = (array_data *)
                    realloc(arr->data.packed,
                            sizeof(array_data) * (arr->items + 1));
                copyinst(item, &arr->data.packed[arr->items]);
                return (++arr->items);
            } else {
                return -5;
            }
            break;
        }

        case ARRAY_DICTIONARY:{
            array_tree *p;

            if (arr->links > 1) {
                arr->links--;
                arr = *harr = array_decouple(arr);
            }
            p = array_tree_find(arr->data.dict, idx);
            if (p) {
                CLEAR(&p->data);
            } else {
                arr->items++;
                p = array_tree_insert(&arr->data.dict, idx);
            }
            copyinst(item, &p->data);
            return arr->items;
            break;
        }

        default:
            break;
    }
    return -6;
}



int
array_insertitem(stk_array **harr, array_iter *idx, array_data *item)
{
    stk_array *arr;
    int i;

    if (!harr || !*harr || !idx) {
        return -1;
    }
    arr = *harr;
    switch (arr->type) {
        case ARRAY_PACKED:{
            if (idx->type != PROG_INTEGER) {
                return -1;
            }
            if (idx->data.number < 0 || idx->data.number > arr->items) {
                return -1;
            }
            if (arr->links > 1) {
                arr->links--;
                arr = *harr = array_decouple(arr);
            }
            arr->data.packed = (array_data *)
                realloc(arr->data.packed,
                        sizeof(array_data) * (arr->items + 1));
            for (i = arr->items++; i > idx->data.number; i--) {
                copyinst(&arr->data.packed[i - 1], &arr->data.packed[i]);
                CLEAR(&arr->data.packed[i - 1]);
            }
            copyinst(item, &arr->data.packed[i]);
            return arr->items;
            break;
        }

        case ARRAY_DICTIONARY:{
            array_tree *p;

            if (arr->links > 1) {
                arr->links--;
                arr = *harr = array_decouple(arr);
            }
            p = array_tree_find(arr->data.dict, idx);
            if (p) {
                CLEAR(&p->data);
            } else {
                arr->items++;
                p = array_tree_insert(&arr->data.dict, idx);
            }
            copyinst(item, &p->data);
            return arr->items;
            break;
        }

        default:
            break;
    }
    return -1;
}



int
array_appenditem(stk_array **harr, array_data *item)
{
    struct inst key;

    if (!harr || !*harr)
        return -1;
    if ((*harr)->type != ARRAY_PACKED)
        return -1;
    key.type = PROG_INTEGER;
    key.data.number = array_count(*harr);

    return array_setitem(harr, &key, item);
}

stk_array *
array_getrange(stk_array *arr, array_iter *start, array_iter *end)
{
    stk_array *new2;
    array_data *tmp;
    int sidx, eidx;

    if (!arr) {
        return NULL;
    }
    switch (arr->type) {
        case ARRAY_PACKED:{
            array_iter idx;
            array_iter didx;

            if (start->type != PROG_INTEGER) {
                return NULL;
            }
            if (end->type != PROG_INTEGER) {
                return NULL;
            }
            sidx = start->data.number;
            eidx = end->data.number;
            if (sidx < 0) {
                sidx = 0;
            } else if (sidx > arr->items) {
                return NULL;
            }
            if (eidx >= arr->items) {
                eidx = arr->items - 1;
            } else if (eidx < 0) {
                return NULL;
            }
            if (sidx > eidx) {
                return NULL;
            }
            idx.type = PROG_INTEGER;
            idx.data.number = sidx;
            didx.type = PROG_INTEGER;
            didx.data.number = 0;
            new2 = new_array_packed(eidx - sidx + 1);
            while (idx.data.number <= eidx) {
                tmp = array_getitem(arr, &idx);
                if (!tmp)
                    break;
                array_setitem(&new2, &didx, tmp);
                didx.data.number++;
                idx.data.number++;
            }
            return new2;
            break;
        }

        case ARRAY_DICTIONARY:{
            stk_array *new2;
            array_tree *s;
            array_tree *e;

            new2 = new_array_dictionary();
            s = array_tree_find(arr->data.dict, start);
            if (!s) {
                s = array_tree_next_node(arr->data.dict, start);
                if (!s) {
                    return new2;
                }
            }
            e = array_tree_find(arr->data.dict, end);
            if (!e) {
                e = array_tree_prev_node(arr->data.dict, end);
                if (!e) {
                    return new2;
                }
            }
            if (array_tree_compare(&s->key, &e->key, 0, 0) > 0) {
                return new2;
            }
            while (s) {
                array_setitem(&new2, &s->key, &s->data);
                if (s == e)
                    break;
                s = array_tree_next_node(arr->data.dict, &s->key);
            }
            return new2;
            break;
        }

        default:
            break;
    }
    return NULL;
}


int
array_setrange(stk_array **harr, array_iter *start, stk_array *inarr)
{
    stk_array *arr;
    array_iter idx;

    if (!harr || !*harr) {
        return -1;
    }
    arr = *harr;
    if (!inarr || !inarr->items) {
        return arr->items;
    }
    switch (arr->type) {
        case ARRAY_PACKED:{
            if (!start) {
                return -1;
            }
            if (start->type != PROG_INTEGER) {
                return -1;
            }
            if (start->data.number < 0 || start->data.number > arr->items) {
                return -1;
            }
            if (arr->links > 1) {
                arr->links--;
                arr = *harr = array_decouple(arr);
            }
            if (array_first(inarr, &idx)) {
                do {
                    array_setitem(&arr, start, array_getitem(inarr, &idx));
                    start->data.number++;
                } while (array_next(inarr, &idx));
            }
            return arr->items;
            break;
        }

        case ARRAY_DICTIONARY:{
            if (arr->links > 1) {
                arr->links--;
                arr = *harr = array_decouple(arr);
            }
            if (array_first(inarr, &idx)) {
                do {
                    array_setitem(&arr, &idx, array_getitem(inarr, &idx));
                } while (array_next(inarr, &idx));
            }
            return arr->items;
            break;
        }

        default:
            break;
    }
    return -1;
}


int
array_insertrange(stk_array **harr, array_iter *start, stk_array *inarr)
{
    stk_array *arr;
    array_data *itm;
    array_iter idx;
    array_iter didx;

    if (!harr || !*harr) {
        return -1;
    }
    arr = *harr;
    if (!inarr || !inarr->items) {
        return arr->items;
    }
    switch (arr->type) {
        case ARRAY_PACKED:{
            if (!start) {
                return -1;
            }
            if (start->type != PROG_INTEGER) {
                return -1;
            }
            if (start->data.number < 0 || start->data.number > arr->items) {
                return -1;
            }
            if (arr->links > 1) {
                arr->links--;
                arr = *harr = array_decouple(arr);
            }
            arr->data.packed = (array_data *)
                realloc(arr->data.packed,
                        sizeof(array_data) * (arr->items + inarr->items));
            copyinst(start, &idx);
            copyinst(start, &didx);
            idx.data.number = arr->items - 1;
            didx.data.number = arr->items + inarr->items - 1;
            while (idx.data.number >= start->data.number) {
                itm = array_getitem(arr, &idx);
                copyinst(itm, &arr->data.packed[didx.data.number]);
                CLEAR(itm);
                idx.data.number--;
                didx.data.number--;
            }
            if (array_first(inarr, &idx)) {
                do {
                    itm = array_getitem(inarr, &idx);
                    copyinst(itm, &arr->data.packed[start->data.number]);
                    start->data.number++;
                } while (array_next(inarr, &idx));
            }
            arr->items += inarr->items;
            return arr->items;
            break;
        }

        case ARRAY_DICTIONARY:{
            if (arr->links > 1) {
                arr->links--;
                arr = *harr = array_decouple(arr);
            }
            if (array_first(inarr, &idx)) {
                do {
                    array_setitem(&arr, &idx, array_getitem(inarr, &idx));
                } while (array_next(inarr, &idx));
            }
            return arr->items;
            break;
        }

        default:
            break;
    }
    return -1;
}


int
array_delrange(stk_array **harr, array_iter *start, array_iter *end)
{
    stk_array *arr;
    array_data *itm;
    int sidx, eidx, totsize;
    array_iter idx;
    array_iter didx;

    if (!harr || !*harr) {
        return -1;
    }
    arr = *harr;
    switch (arr->type) {
        case ARRAY_PACKED:{
            if (start->type != PROG_INTEGER) {
                return -1;
            }
            if (end->type != PROG_INTEGER) {
                return -1;
            }
            if (arr->items == 0) { /* nothing to do here */
                return 0;
            }

            sidx = start->data.number;
            eidx = end->data.number;
            if (sidx < 0) {
                sidx = 0;
            } else if (sidx >= arr->items) {
                return -1;
            }
            if (eidx >= arr->items) {
                eidx = arr->items - 1;
            } else if (eidx < 0) {
                return -1;
            }
            if (sidx > eidx) {
                return -1;
            }
            if (arr->links > 1) {
                arr->links--;
                arr = *harr = array_decouple(arr);
            }
            start->data.number = sidx;
            end->data.number = eidx;
            copyinst(end, &idx);
            copyinst(start, &didx);
            idx.data.number += 1;
            while (idx.data.number < arr->items) {
                itm = array_getitem(arr, &idx);
                copyinst(itm, &arr->data.packed[didx.data.number]);
                CLEAR(itm);
                idx.data.number++;
                didx.data.number++;
            }
            arr->items -= (eidx - sidx + 1);
            totsize = (arr->items) ? arr->items : 1;
            arr->data.packed =
                (array_data *) realloc(arr->data.packed,
                                       sizeof(array_data) * totsize);
            return arr->items;
            break;
        }

        case ARRAY_DICTIONARY:{
            array_tree *s;
            array_tree *e;

            s = array_tree_find(arr->data.dict, start);
            if (!s) {
                s = array_tree_next_node(arr->data.dict, start);
                if (!s) {
                    return arr->items;
                }
            }
            e = array_tree_find(arr->data.dict, end);
            if (!e) {
                e = array_tree_prev_node(arr->data.dict, end);
                if (!e) {
                    return arr->items;
                }
            }
            if (array_tree_compare(&s->key, &e->key, 0, 0) > 0) {
                return arr->items;
            }
            if (arr->links > 1) {
                arr->links--;
                arr = *harr = array_decouple(arr);
            }
            copyinst(&s->key, &idx);
            while (s && array_tree_compare(&s->key, &e->key, 0, 0) <= 0) {
                arr->data.dict = array_tree_delete(&s->key, arr->data.dict);
                arr->items--;
                s = array_tree_next_node(arr->data.dict, &idx);
            }
            CLEAR(&idx);
            return arr->items;
            break;
        }

        default:
            break;
    }
    return -1;
}


int
array_delitem(stk_array **harr, array_iter *itm)
{
    array_iter idx;
    int result;

    copyinst(itm, &idx);
    result = array_delrange(harr, itm, &idx);
    CLEAR(&idx);
    return result;
}


/*\
|*| Must not code b-trees, must not code b-trees...
\*/

/*\
|*| I use 4-space tabs, darn it.
\*/


/*\
|*| array_demote_only
|*| array demote discards the values of a dictionary, and
|*| returns a packed list of the keys.
|*| (Useful because keys are ordered and unique, presumably.)
|*| (This allows the keys to be abused as sets.)
\*/
stk_array *
array_demote_only(stk_array *arr, int threshold)
{
    stk_array *demoted_array = NULL;
    int items_left = 0;
    array_iter current_key;
    array_iter *current_value;
    array_iter new_index;

    if (!arr || ARRAY_DICTIONARY != arr->type)
        return NULL;

    new_index.type = PROG_INTEGER;;
    new_index.data.number = 0;
    demoted_array = new_array_packed(0);
    items_left = array_first(arr, &current_key);
    while (items_left) {
        current_value = array_getitem(arr, &current_key);
        if (PROG_INTEGER == current_value->type
            && current_value->data.number >= threshold) {
            array_insertitem(&demoted_array, &new_index, &current_key);
            new_index.data.number++;
        }
        items_left = array_next(arr, &current_key);
    }
    return demoted_array;
}

/*\
|*| array_mash
|*| Takes the lists of values from the first array and
|*| uses each value as a key in the second array.  For
|*| each key, the passed "change_by" value is applied to
|*| any existing value.  If the value does not exist, it
|*| is set.
|*| This will be the core of the different/union/intersection
|*| code.
|*| Of course, this is going to absolutely blow chunks when
|*| passed an array with value types that can't be used as
|*| key values in an array.  Blast.  That may be infeasible
|*| regardless, though.
\*/
void
array_mash(stk_array *arr_in, stk_array **mash, int value)
{

    int still_values = 0;
    array_iter current_key;
    array_iter *current_keyval;
    array_iter *current_value;
    array_data temp_value;

    if (NULL == arr_in || NULL == mash || NULL == *mash)
        return;

    still_values = array_first(arr_in, &current_key);
    while (still_values) {
        current_keyval = array_getitem(arr_in, &current_key);
        current_value = array_getitem(*mash, current_keyval);
        if (NULL != current_value) {
            if (PROG_INTEGER == current_value->type) {
                copyinst(current_value, &temp_value);
                temp_value.data.number += value;
                array_setitem(mash, current_keyval, &temp_value);
            }
        } else {
            temp_value.type = PROG_INTEGER;
            temp_value.data.number = value;
            array_insertitem(mash, current_keyval, &temp_value);
        }
        still_values = array_next(arr_in, &current_key);
    }
}



int
array_is_homogenous(stk_array *arr, int typ)
{
    array_iter idx;
    array_data *dat;
    int failedflag = 0;

    if (array_first(arr, &idx)) {
        do {
            dat = array_getitem(arr, &idx);
            if (dat->type != typ) {
                failedflag = 1;
            }
        } while (array_next(arr, &idx));
    }
    return (!failedflag);
}





int
array_set_strkey(stk_array **harr, const char *key, struct inst *val)
{
    struct inst name;
    int result;

    name.type = PROG_STRING;
    name.data.string = alloc_prog_string(key);

    result = array_setitem(harr, &name, val);

    CLEAR(&name);

    return result;
}



int
array_set_strkey_intval(stk_array **arr, const char *key, int val)
{
    struct inst value;
    int result;

    value.type = PROG_INTEGER;
    value.data.number = val;

    result = array_set_strkey(arr, key, &value);

    CLEAR(&value);

    return result;
}



int
array_set_strkey_strval(stk_array **harr, const char *key, const char *val)
{
    struct inst value;
    int result;

    value.type = PROG_STRING;
    value.data.string = alloc_prog_string(val);

    result = array_set_strkey(harr, key, &value);

    CLEAR(&value);

    return result;
}


int
array_set_strkey_refval(stk_array **harr, const char *key, dbref val)
{
    struct inst value;
    int result;

    value.type = PROG_OBJECT;
    value.data.objref = val;

    result = array_set_strkey(harr, key, &value);

    CLEAR(&value);

    return result;
}



const char *
array_get_intkey_strval(stk_array *arr, int key)
{
    struct inst ikey;
    array_data *value;

    ikey.type = PROG_INTEGER;
    ikey.data.number = key;

    value = array_getitem(arr, &ikey);

    CLEAR(&ikey);

    if (!value || value->type != PROG_STRING) {
        return NULL;
    } else if (!value->data.string) {
        return "";
    } else {
        return value->data.string->data;
    }
}

int
array_set_intkey(stk_array **harr, int key, struct inst *val)
{
    struct inst name;
    int result;

    name.type = PROG_INTEGER;
    name.data.number = key;

    result = array_setitem(harr, &name, val);

    CLEAR(&name);

    return result;

}

int
array_set_intkey_strval(stk_array **harr, int key, const char *val)
{
    struct inst value;
    int result;

    value.type = PROG_STRING;
    value.data.string = alloc_prog_string(val);

    result = array_set_intkey(harr, key, &value);

    CLEAR(&value);

    return result;
}

/* There was a bunch of redundant code in p_db.c doing this, so I */
/*  added it as a global function. -Hinoserm                      */
int
array_appendref(stk_array **arr, dbref theref)
{
    struct inst temp;
    int i;

    temp.type = PROG_OBJECT;
    temp.data.objref = theref;
    i = array_appenditem(arr, &temp);
    CLEAR(&temp);

    return (i);
}
