
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
# if defined(__APPLE__) && defined(HAVE_SYS_MALLOC_H)
#  include <sys/malloc.h>
# endif
#elif defined(HAVE_MALLOC_H)
# include <malloc.h>
#endif

#include "db.h"
#include "externs.h"

#define WORD_HASH_SIZE (100000)
#define SIZE_HASH_SIZE (100000)

struct queue_node {
    struct queue_node *next;
    struct queue_node *prev;
    int count;
    int spcount;
    int val;
    int len;
    char word[32];
};

struct queue_node *head = NULL;
struct queue_node *tail = NULL;

int total_words = 0;
int counted_words = 0;
hash_tab wordhash[WORD_HASH_SIZE];

struct queue_node *sizehash[100000];

int
notify(int player, const char *msg)
{
    return printf("%s\n", msg);
}



int
string_compare(const char *s1, const char *s2)
{
    return strcmp(s1, s2);
}

char *
string_dup(const char *s)
{
    char *p = (char *) malloc(strlen(s) + 1);

    strcpy(p, s);
    return p;
}


void
queue_remove_node(struct queue_node *node)
{
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        head = node->next;
    }
    if (node->next) {
        node->next->prev = node->prev;
    } else {
        tail = node->prev;
    }
}

void
queue_insert_after(struct queue_node *where, struct queue_node *node)
{
    if (where) {
        node->next = where->next;
        where->next = node;
    } else {
        node->next = head;
        head = node;
    }
    node->prev = where;
    if (node->next) {
        node->next->prev = node;
    } else {
        tail = node;
    }
}


void
queue_promote(struct queue_node *node)
{
    struct queue_node *prev;
    struct queue_node *p;
    int oldval;

    oldval = node->val;
    node->val =
        (node->count * (node->len - 2)) + (node->spcount * (node->len - 1));

    /* remove node from sizehash table if necessary */
    if (oldval && oldval < SIZE_HASH_SIZE) {
        p = sizehash[oldval];
        if (p) {
            if (p == node) {
                if (node->prev) {
                    if (node->prev->val == oldval) {
                        sizehash[oldval] = node->prev;
                    } else {
                        sizehash[oldval] = NULL;
                    }
                } else {
                    sizehash[oldval] = NULL;
                }
            }
        }
    }

    if (node->val < SIZE_HASH_SIZE) {
        int limit;
        int i;

        limit = 16;
        i = node->val;
        do {
            prev = sizehash[i];
        } while (!prev && ++i < SIZE_HASH_SIZE && --limit);
        if (i >= SIZE_HASH_SIZE || !limit) {
            prev = node->prev;
            while (prev && prev->val < node->val) {
                prev = prev->prev;
            }
        }
    } else {
        prev = node->prev;
        while (prev && prev->val < node->val) {
            prev = prev->prev;
        }
    }

    if (prev != node->prev) {
        queue_remove_node(node);
        queue_insert_after(prev, node);
    }

    /* put entry in size hash */
    if (node->val < SIZE_HASH_SIZE) {
        sizehash[node->val] = node;
    }
}


void
list_top_4k_words(void)
{
    int count = 4096;
    int lastval;
    struct queue_node *node;

    lastval = 0x3fffffff;
    for (node = head; node && count--; node = node->next) {
        /* printf("%-32s %5d %5d\n", node->word, node->count, node->spcount); */
        printf("%s\n", node->word);
        fflush(stdout);
        if (lastval < node->val)
            abort();
        lastval = node->val;
    }
}

struct queue_node *
queue_add_node(const char *word, int pri)
{
    struct queue_node *nw;
    hash_data hd;

    nw = (struct queue_node *) malloc(sizeof(struct queue_node));
    if (!nw)
        abort();

    if (!head) {
        head = nw;
        nw->prev = NULL;
    }
    nw->next = NULL;
    nw->prev = tail;
    if (tail) {
        tail->next = nw;
    }
    tail = nw;
    nw->count = 0;
    nw->spcount = 0;
    strcpy(nw->word, word);
    nw->len = strlen(nw->word);
    if (nw->word[nw->len - 1] == ' ') {
        nw->word[nw->len - 1] = '\0';
        nw->len--;
        nw->spcount += pri;
    } else {
        nw->count += pri;
    }
    nw->val = 0;
    queue_promote(nw);

    hd.pval = (void *) nw;
    (void) add_hash(nw->word, hd, wordhash, WORD_HASH_SIZE);

    return nw;
}

void
add_to_list(const char *word)
{
    struct queue_node *node;
    hash_data *exp;
    char buf[32];
    short spcflag = 0;

    if (strlen(word) < 3) {
        return;
    }

    strcpy(buf, word);
    if (buf[strlen(buf) - 1] == ' ') {
        buf[strlen(buf) - 1] = '\0';
        spcflag++;
    }
    exp = find_hash(buf, wordhash, WORD_HASH_SIZE);
    if (exp) {
        node = (struct queue_node *) exp->pval;
        if (spcflag) {
            node->spcount++;
        } else {
            node->count++;
        }
        queue_promote(node);
        counted_words++;
    } else {
        /* printf("%s\n", buf); */
        queue_add_node(word, 1);
        total_words++;
        counted_words++;
    }
}

void
remember_word_variants(const char *in)
{
    char word[32];

    strcpy(word, in);
    add_to_list(word);
    /*
       t = word + strlen(word);
       while (t > word) {
       h = word;
       while (h < t) {
       add_to_list(h);
       h++;
       }
       t--;
       if (*t == ' ') {
       t--;
       }
       *t = '\0';
       }
     */
}

void
remember_words_in_string(const char *in)
{
    char buf[32];
    const char *h;
    char *o;

    h = in;
    while (*h) {
        o = buf;
        while (*h && !isalnum(*h)) {
            h++;
        }
        while (*h && isalnum(*h) && (o - buf < 30)) {
            if (isupper(*h)) {
                *o++ = tolower(*h++);
            } else {
                *o++ = *h++;
            }
        }
        if (*h == ' ') {
            *o++ = ' ';
        }
        *o++ = '\0';
        if (strlen(buf) > 3) {
            remember_word_variants(buf);
        }
    }
}


int
main(int argc, char **argv)
{
    char buf[16384];
    char *p;

    queue_add_node("felorin", 99999999);
    queue_add_node("anthro", 99999999);
    queue_add_node("phile ", 99999999);
    queue_add_node("morph", 99999999);
    queue_add_node("revar", 99999999);
    queue_add_node("sion ", 99999999);
    queue_add_node("tion ", 99999999);
    queue_add_node("post", 99999999);
    queue_add_node("ing ", 99999999);
    queue_add_node("ion ", 99999999);
    queue_add_node("est ", 99999999);
    queue_add_node("ies ", 99999999);
    queue_add_node("ism ", 99999999);
    queue_add_node("ish ", 99999999);
    queue_add_node("ary ", 99999999);
    queue_add_node("ous ", 99999999);
    queue_add_node("dis", 99999999);
    queue_add_node("non", 99999999);
    queue_add_node("pre", 99999999);
    queue_add_node("sub", 99999999);
    queue_add_node("al ", 99999999);
    queue_add_node("ic ", 99999999);
    queue_add_node("ly ", 99999999);
    queue_add_node("le ", 99999999);
    queue_add_node("es ", 99999999);
    queue_add_node("ed ", 99999999);
    queue_add_node("er ", 99999999);

    while (!feof(stdin)) {
        fgets(buf, sizeof(buf), stdin);
        p = strchr(buf, ':');
        if (p && (atoi(p + 1) == 10)) {
            p = strchr(p + 1, ':');
            if (p) {
                remember_words_in_string(p + 1);
            }
        }
    }
    list_top_4k_words();
    /* printf("%d unique words found.\n", total_words); */
    /* printf("%d counted words.\n", counted_words); */
    return 0;
}
