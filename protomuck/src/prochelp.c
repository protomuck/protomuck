#include <stdio.h>
#ifdef WIN32
# include <string.h>
#else
# include <strings.h>
#endif
#include <ctype.h>


#define HTML_TOPICHEAD		"<br><p><h4><a name=\"%s\">"
#define HTML_TOPICHEAD_BREAK	"<br>\n"
#define HTML_TOPICBODY		"</a></h4>\n"
#define HTML_TOPICEND		"<br><p>\n"

#define HTML_SECTION		"<hr><h3 align=center>%s</h3>\n"
#define HTML_PARAGRAPH		"<p>\n"
#define HTML_CODEBEGIN		"<pre>\n"
#define HTML_CODEEND		"</pre>\n"

#define HTML_INDEXBEGIN		"<table width=\"100%%\" align=center border=0>\n  <tr>\n"
#define HTML_INDEXEND		"  </tr>\n</table>\n\n"
#define HTML_INDEX_NEWROW	"  </tr>\n  <tr>\n"
#define HTML_INDEX_ENTRY	"    <td><a href=\"#%s\">%s</a>\n"

#define HTML_SECTIDX_HEAD	"<p><hr><p>\n<h3><a name=\"%s\">%s</a></h3>\n"



char *
string_dup(const char *s)
{
    char *p;

    p = (char *) malloc(strlen(s) + 1);
    if (p)
        strcpy(p, s);
    return p;
}


char sect[256] = "";

struct topiclist {
    struct topiclist *next;
    char *topic;
    char *section;
    int printed;
};

struct topiclist *topichead;
struct topiclist *secthead;

void
add_section(const char *str)
{
    struct topiclist *ptr, *top;

    if (!str || !*str)
        return;
    top = (struct topiclist *) malloc(sizeof(struct topiclist));
    top->topic = NULL;
    top->section = (char *) string_dup(sect);
    top->printed = 0;
    top->next = NULL;

    if (!secthead) {
        secthead = top;
        return;
    }
    for (ptr = secthead; ptr->next; ptr = ptr->next) ;
    ptr->next = top;
}

void
add_topic(const char *str)
{
    struct topiclist *ptr, *top;
    char buf[256];
    const char *p;
    char *s;

    if (!str || !*str)
        return;

    p = str;
    s = buf;
    do {
        *s++ = tolower(*p);
    } while (*p++);

    top = (struct topiclist *) malloc(sizeof(struct topiclist));
    top->topic = (char *) string_dup(buf);
    top->section = (char *) string_dup(sect);
    top->printed = 0;

    if (!topichead) {
        topichead = top;
        top->next = NULL;
        return;
    }

    if (strcasecmp(str, topichead->topic) < 0) {
        top->next = topichead;
        topichead = top;
        return;
    }

    ptr = topichead;
    while (ptr->next && strcasecmp(str, ptr->next->topic) > 0) {
        ptr = ptr->next;
    }
    top->next = ptr->next;
    ptr->next = top;
}

void
print_section_topics(FILE * f, FILE * hf, int cols)
{
    struct topiclist *ptr;
    struct topiclist *sptr;
    char sectname[256];
    char *sectptr;
    char *osectptr;
    char buf[256];
    char buf2[256];
    int cnt;
    int width;
    int hcol;
    char *currsect;

    width = 78 / cols;
    for (sptr = secthead; sptr; sptr = sptr->next) {
        currsect = sptr->section;
        cnt = 0;
        hcol = 0;
        buf[0] = '\0';
        strcpy(sectname, currsect);
        sectptr = index(sectname, '|');
        if (sectptr) {
            *sectptr++ = '\0';
            osectptr = sectptr;
            sectptr = rindex(sectptr, '|');
            if (sectptr) {
                sectptr++;
            }
            if (!sectptr) {
                sectptr = osectptr;
            }
        }
        if (!sectptr) {
            sectptr = "";
        }

        fprintf(hf, HTML_SECTIDX_HEAD, sectptr, sectname);
        fprintf(f, "~\n~\n%s\n%s\n\n", currsect, sectname);
        fprintf(hf, HTML_INDEXBEGIN);
        for (ptr = topichead; ptr; ptr = ptr->next) {
            if (!strcasecmp(currsect, ptr->section)) {
                ptr->printed++;
                cnt++;
                hcol++;
                if (hcol > cols) {
                    fprintf(hf, HTML_INDEX_NEWROW);
                    hcol = 1;
                }
                fprintf(hf, HTML_INDEX_ENTRY, ptr->topic, ptr->topic);
                if (cnt == cols) {
                    sprintf(buf2, "%-0.*s", width - 1, ptr->topic);
                } else {
                    sprintf(buf2, "%-*.*s", width, width - 1, ptr->topic);
                }
                strcat(buf, buf2);
                if (cnt >= cols) {
                    fprintf(f, "%s\n", buf);
                    buf[0] = '\0';
                    cnt = 0;
                }
            }
        }
        fprintf(hf, HTML_INDEXEND);
        if (cnt)
            fprintf(f, "%s\n", buf);
        fprintf(f, "\n");
    }
}

void
print_sections(FILE * f, FILE * hf, int cols)
{
    struct topiclist *ptr;
    struct topiclist *sptr;
    char sectname[256];
    char *osectptr;
    char *sectptr;
    char buf[256];
    char buf2[256];
    int cnt;
    int width;
    int hcol;
    char *currsect;

    fprintf(f, "You can get more help on the following topics:\n\n");
    fprintf(hf,
            "<h4>You can get more help on the following topics:</h4>\n<ul>");
    width = 78 / cols;
    for (sptr = secthead; sptr; sptr = sptr->next) {
        currsect = sptr->section;
        cnt = 0;
        hcol = 0;
        buf[0] = '\0';
        strcpy(sectname, currsect);
        sectptr = index(sectname, '|');
        if (sectptr) {
            *sectptr++ = '\0';
            osectptr = sectptr;
            sectptr = rindex(sectptr, '|');
            if (sectptr) {
                sectptr++;
            }
            if (!sectptr) {
                sectptr = osectptr;
            }
        }
        if (!sectptr) {
            sectptr = "";
        }

        fprintf(hf, "  <li><a href=\"#%s\">%s</a></li>\n", sectptr, sectname);
        fprintf(f, "  %-40s (%s)\n", sectname, sectptr);
    }
    fprintf(hf, "</ul>\n\n");
}

void
print_topics(FILE * f, FILE * hf, int cols)
{
    struct topiclist *ptr;
    char buf[256];
    char buf2[256];
    int cnt = 0;
    int width;
    int hcol = 0;

    fprintf(f, "You can get more help on the following topics:\n\n");
    fprintf(hf, HTML_INDEXBEGIN);
    width = 78 / cols;
    buf[0] = '\0';
    for (ptr = topichead; ptr; ptr = ptr->next) {
        cnt++;
        hcol++;
        if (hcol > cols) {
            fprintf(hf, HTML_INDEX_NEWROW);
            hcol = 1;
        }
        fprintf(hf, HTML_INDEX_ENTRY, ptr->topic, ptr->topic);
        if (cnt == cols) {
            sprintf(buf2, "%-0.*s", width - 1, ptr->topic);
        } else {
            sprintf(buf2, "%-*.*s", width, width - 1, ptr->topic);
        }
        strcat(buf, buf2);
        if (cnt >= cols) {
            fprintf(f, "%s\n", buf);
            buf[0] = '\0';
            cnt = 0;
        }
    }
    fprintf(hf, HTML_INDEXEND);
    if (cnt)
        fprintf(f, "%s\n", buf);
}

int
find_topics(FILE * infile)
{
    char buf[4096];
    char *s, *p;
    int longest, lng;

    longest = 0;
    while (!feof(infile)) {
        do {
            if (!fgets(buf, sizeof(buf), infile)) {
                *buf = '\0';
                break;
            } else {
                if (!strncmp(buf, "~~section ", 10)) {
                    buf[strlen(buf) - 1] = '\0';
                    strcpy(sect, (buf + 10));
                    add_section(sect);
                }
            }
        } while (!feof(infile)
                 && (*buf != '~' || buf[1] == '@' || buf[1] == '~'
                     || buf[1] == '<' || buf[1] == '!'));

        do {
            if (!fgets(buf, sizeof(buf), infile)) {
                *buf = '\0';
                break;
            } else {
                if (!strncmp(buf, "~~section ", 10)) {
                    buf[strlen(buf) - 1] = '\0';
                    strcpy(sect, (buf + 10));
                    add_section(sect);
                }
            }
        } while (*buf == '~' && !feof(infile));

        for (s = p = buf; *s; s++) {
            if (*s == '|' || *s == '\n') {
                *s++ = '\0';
                add_topic(p);
                lng = strlen(p);
                if (lng > longest)
                    longest = lng;
                p = s;
                break;
            }
        }
    }
    return (longest);
}


void
process_lines(FILE * infile, FILE * outfile, FILE * htmlfile, int cols)
{
    FILE *docsfile;
    char *sectptr;
    char buf[4096];
    int nukenext = 0;
    int topichead = 0;
    int codeblock = 0;
    char *ptr;

    docsfile = stdout;
    while (!feof(infile)) {
        if (!fgets(buf, sizeof(buf), infile)) {
            return;
        }
        if (buf[0] == '~') {
            if (buf[1] == '~') {
                if (!strncmp(buf, "~~file ", 7)) {
                    fclose(docsfile);
                    buf[strlen(buf) - 1] = '\0';
                    if (!(docsfile = fopen(buf + 7, "w"))) {
                        fprintf(stderr, "Error: can't write to %s", buf + 7);
                        exit(1);
                    }
                } else if (!strncmp(buf, "~~section ", 10)) {
                    buf[strlen(buf) - 1] = '\0';
                    sectptr = index(buf + 10, '|');
                    if (sectptr) {
                        *sectptr = '\0';
                    }
                    fprintf(outfile, "~%*s\n", (38 + strlen(buf + 10) / 2),
                            (buf + 10));
                    fprintf(docsfile, "%*s\n", (38 + strlen(buf + 10) / 2),
                            (buf + 10));
                    fprintf(htmlfile, HTML_SECTION, (buf + 10));
                } else if (!strcmp(buf, "~~code\n")) {
                    fprintf(htmlfile, HTML_CODEBEGIN);
                    codeblock = 1;
                } else if (!strcmp(buf, "~~endcode\n")) {
                    fprintf(htmlfile, HTML_CODEEND);
                    codeblock = 0;
                } else if (!strcmp(buf, "~~sectlist\n")) {
                    print_sections(outfile, htmlfile, cols);
                } else if (!strcmp(buf, "~~secttopics\n")) {
                    print_section_topics(outfile, htmlfile, cols);
                } else if (!strcmp(buf, "~~index\n")) {
                    print_topics(outfile, htmlfile, cols);
                }
            } else if (buf[1] == '!') {
                fprintf(outfile, "%s", buf + 2);
            } else if (buf[1] == '@') {
                fprintf(htmlfile, "%s", buf + 2);
            } else if (buf[1] == '<') {
                fprintf(outfile, "%s", buf + 2);
                fprintf(docsfile, "%s", buf + 2);
            } else if (buf[1] == '#') {
                fprintf(outfile, "~%s", buf + 2);
                fprintf(docsfile, "%s", buf + 2);
            } else {
                if (!nukenext) {
                    fprintf(htmlfile, HTML_TOPICEND);
                }
                nukenext = 1;
                fprintf(outfile, "%s", buf);
                fprintf(docsfile, "%s", buf + 1);
                fprintf(htmlfile, "%s", buf + 1);
            }
        } else if (nukenext) {
            nukenext = 0;
            topichead = 1;
            fprintf(outfile, "%s", buf);
            for (ptr = buf; *ptr && *ptr != '|' && *ptr != '\n'; ptr++) {
                *ptr = tolower(*ptr);
            }
            *ptr = '\0';
            fprintf(htmlfile, HTML_TOPICHEAD, buf);
        } else if (buf[0] == ' ') {
            nukenext = 0;
            if (topichead) {
                topichead = 0;
                fprintf(htmlfile, HTML_TOPICBODY);
            } else if (!codeblock) {
                fprintf(htmlfile, HTML_PARAGRAPH);
            }
            fprintf(outfile, "%s", buf);
            fprintf(docsfile, "%s", buf);
            fprintf(htmlfile, "%s", buf);
        } else {
            fprintf(outfile, "%s", buf);
            fprintf(docsfile, "%s", buf);
            fprintf(htmlfile, "%s", buf);
            if (topichead) {
                fprintf(htmlfile, HTML_TOPICHEAD_BREAK);
            }
        }
    }
    fclose(docsfile);
}


int
main(int argc, char **argv)
{
    FILE *infile, *outfile, *htmlfile;
    int cols;

    if (argc != 4) {
        fprintf(stderr,
                "Usage: %s inputrawfile outputhelpfile outputhtmlfile\n",
                argv[0]);
        return 1;
    }

    if (!strcmp(argv[1], argv[2])) {
        fprintf(stderr,
                "%s: cannot use same file for input rawfile and output helpfile\n");
        return 1;
    }

    if (!strcmp(argv[1], argv[3])) {
        fprintf(stderr,
                "%s: cannot use same file for input rawfile and output htmlfile\n");
        return 1;
    }

    if (!strcmp(argv[3], argv[2])) {
        fprintf(stderr, "%s: cannot use same file for htmlfile and helpfile\n");
        return 1;
    }

    if (!strcmp(argv[1], "-")) {
        infile = stdin;
    } else {
        if (!(infile = fopen(argv[1], "r"))) {
            fprintf(stderr, "%s: cannot read %s\n", argv[0], argv[1]);
            return 1;
        }
    }

    if (!(outfile = fopen(argv[2], "w"))) {
        fprintf(stderr, "%s: cannot write to %s\n", argv[0], argv[2]);
        return 1;
    }

    if (!(htmlfile = fopen(argv[3], "w"))) {
        fprintf(stderr, "%s: cannot write to %s\n", argv[0], argv[3]);
        return 1;
    }

    cols = 78 / (find_topics(infile) + 1);
    fseek(infile, 0L, 0);

    process_lines(infile, outfile, htmlfile, cols);

    fclose(infile);
    fclose(outfile);
    fclose(htmlfile);
    return 0;
}
