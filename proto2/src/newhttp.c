#include "copyright.h"
#include "config.h"

#ifdef NEWHTTPD

/*****************************************************************************/
/* ProtoMUCK Webserver v1.0 written by Hinoserm (foxsteve).                  */
/* Many thanks to The_Blob for helping with the HTTP protocol.               */
/*****************************************************************************/
/* TODO:                                                                     */
/*---------------------------------------------------------------------------*/
/* Finish proper mimetypes handling. I'd like to have it parse a file with   */
/*  the same format as /etc/mime.types and use that as a database of mime    */
/*  types based on file extensions.                                          */
/*---------------------------------------------------------------------------*/
/* Redesign the http_dourl() and related functions to be more like how the   */
/*  http_dofile() function works. (IE: proper index handling of HTMufs)      */
/*---------------------------------------------------------------------------*/
/* Eventually, I'd like to see MPI-in-props support, but not anytime soon.   */
/*---------------------------------------------------------------------------*/
/* Implement a-lot- more control via @tunes. This will likely be finished    */
/*  before I release the webserver, and is a top priority.                   */
/*---------------------------------------------------------------------------*/
/* URLs-in-props for redirections. This -will- be finished before I'm done.  */
/*---------------------------------------------------------------------------*/
/* The code needs some overall cleanup and proper commenting, and I'd like   */
/*  to write out a detailed document on it's use.                            */
/*---------------------------------------------------------------------------*/
/* Sorting for the directory listing stuff would be nice.                    */
/*****************************************************************************/

#include "db.h"
#include "netresolve.h"
#include "interface.h"
#include "mufevent.h"
#include "externs.h"
#include "newhttp.h"
#include "params.h"
#include "interp.h"
#include "match.h"
#include "props.h"
#include "tune.h"
#include "cgi.h"
#include "mpi.h"

#define MALLOC(result, type, number) { if (!((result) = (type *)malloc((number)*sizeof(type)))) \
                                         panic("Out of memory"); }


#define FREE(x) (free((void *) x))

#ifdef MCCP_ENABLED
void mccp_start(struct descriptor_data *d, int version);
#endif

int httpucount = 0;    /* number of total HTTP users */
int httpfcount = 0;    /* number of total HTTP file transfers */

struct http_statstruct http_statcodes[] = {
    {100, "Continue"},
    {101, "Switching Protocols"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    {203, "Non-Authoritative Information"},
    {204, "No Content"},
    {205, "Reset Content"},
    {206, "Partial Content"},
    {300, "Multiple Choices"},
    {301, "Moved Permanently"},
    {302, "Found"},
    {303, "See Other"},
    {304, "Not Modified"},
    {305, "Use Proxy"},
    {306, "(Unused)"},
    {307, "Temporary Redirect"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    {406, "Not Acceptable"},
    {407, "Proxy Authentication Required"},
    {408, "Request Timeout"},
    {409, "Conflict"},
    {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Request Entity Too Large"},
    {414, "Request-URI Too Large"},
    {415, "Unsupported Media Type"},
    {416, "Requested Range Not Satisfiable"},
    {417, "Expectation Failed"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    {502, "Bad Gateway"},
    {503, "Service Unavailable"},
    {504, "Gateway Timeout"},
    {505, "HTTP Version Not Supported"},
    {0, ""}
};

struct http_mimestruct http_mimetypes[] = {
    {"htm", "text/html"},
    {"html", "text/html"},
    {"shtml", "text/html"},
    {"txt", "text/plain"},
    {"text", "text/plain"},
    {"muf", "text/plain"},
    {"m", "text/plain"},
    {"gif", "image/gif"},
    {"jpg", "image/jpeg"},
    {"jpeg", "image/jpeg"},
    {"png", "image/png"},
    {"pdf", "application/pdf"},
    {"zip", "application/zip"},
    {"gz", "application/x-gzip"},
    {"exe", "application/octet-stream"},
    {"bin", "application/octet-stream"},
    {"tar", "application/x-tar"},
    {"mp3", "audio/mpeg"},
    {"ra", "audio/x-realaudio"},
    {"wav", "audio/x-wav"},
    {"avi", "video/x-msvideo"},
    {"mpg", "video/mpeg"},
    {"mpeg", "video/mpeg"},
    {NULL, NULL}
};

const char *http_defaulthomes[] = {
    "index.html", "index.htm",
    "home.html", "home.htm",
    "", NULL
};

struct http_method http_methods[] = {
    {"GET", 0x2BF, http_handler_get},
    {"POST", 0x25C, http_handler_post},
    {NULL, 0, NULL}
};

int
array_set_strkey_arrval(stk_array **arr, const char *key, stk_array *arr2)
{
    struct inst value;
    int result;

    value.type = PROG_ARRAY;
    value.data.array = arr2;

    result = array_set_strkey(arr, key, &value);

    CLEAR(&value);

    return result;
}

void
http_log(struct descriptor_data *d, int debuglvl, char *format, ...)
{
    char buf[BUFFER_LEN];
    char tbuf[40];
    va_list args;
    FILE *fp;
    time_t lt = current_systime;

    va_start(args, format);
    vsprintf(buf, format, args);
    va_end(args);

    /* Finish me! */
    if (debuglvl <= tp_web_logwall_lvl)
        wall_logwizards(buf);

    if (debuglvl <= tp_web_logfile_lvl) {
        *tbuf = '\0';
        if ((fp = fopen(HTTP_LOG, "a")) == NULL) {
            fprintf(stderr, "Unable to open %s!\n", HTTP_LOG);
            fprintf(stderr, "%.16s: [%d]: %s", ctime(&lt),
                    d ? d->descriptor : -1, buf);
        } else {
            format_time(tbuf, 32, "%c\0", localtime(&lt));
            fprintf(fp, "%.32s: [%d]: %s", tbuf, d ? d->descriptor : -1, buf);
            fclose(fp);
        }
    }
}

/* queue_text():                                        */
/*   Works exactly like queue_write(), but can format   */
/*   like sprintf().                                    */
int
queue_text(struct descriptor_data *d, char *format, ...)
{
    va_list args;
    char buf[BUFFER_LEN];

    va_start(args, format);
    vsprintf(buf, format, args);
    va_end(args);

    return queue_write(d, buf, strlen(buf));
}

/* http_split():                                        */
/*   Splits string s at char c. Returns a pointer.      */
char *
http_split(char *s, int c)
{
    char *p;

    for (p = s; *p && (*p != c); p++) ;
    if (*p)
        *p++ = '\0';

    return p;
}

/* http_statlookup():                                   */
/*   Looks up the reason phrase for a status code from  */
/*   the http_statcodes[] table and returns it.         */
const char *
http_statlookup(int status)
{
    struct http_statstruct *s = http_statcodes;

    while (s->code && s->code != status)
        s++;

    return (s->msg);
}

/* http_methodlookup():                                 */
/*   Looks up data from the http_methods[] table and    */
/*   returns it.                                        */
struct http_method *
http_methodlookup(const char *method)
{
    struct http_method *m = http_methods;

    while (m->method && string_compare(m->method, method))
        m++;

    return (m);
}

/* http_mimelookup():                                   */
/* This isn't used! It might be in the future. Finish me! */
const char *
http_mimelookup(const char *ext)
{
    struct http_mimestruct *m = http_mimetypes;

    while (m->ext && string_compare(m->ext, ext))
        m++;

    return (m->type);
}

/* http_fieldlookup():                                  */
/*   Looks up a field from the d->fields linked-list    */
/*   and returns a pointer to it.                       */
struct http_field *
http_fieldlookup(struct descriptor_data *d, const char *field)
{
    struct http_field *f = d->http->fields;

    while (f && !(f->field && !string_compare(f->field, field)))
        f = f->next;

    return f;
}

/* http_gethost():                                      */
/*   Return the 'Host: ' data for a descr, or return    */
/*   tp_servername if there's no field.                 */
const char *
http_gethost(struct descriptor_data *d)
{
    struct http_field *f;

    if (d->http->fields && (f = http_fieldlookup(d, "Host")) && f->data
        && *f->data) {
        return f->data;
    } else {
        return tp_servername;
    }
}

/* http_sendheader():                                   */
/*   Create and send an HTTP header based on the info   */
/*   provided. If content_type isn't given, it defaults */
/*   to 'text/plain'. If content_length is below zero,  */
/*   it won't be given.                                 */
void
http_sendheader(struct descriptor_data *d, int statcode,
                const char *content_type, int content_length)
{
    struct http_field *f;
    int iscompressed=0;

    char tbuf[BUFFER_LEN];
    time_t t = time(NULL) + get_tz_offset();

    format_time(tbuf, BUFFER_LEN, "%a, %d %b %Y %T GMT", localtime(&t));

    queue_text(d, "HTTP/1.1 %d %s\r\nDate: %s\r\n"
               "Server: ProtoMUCK/%s\r\n"
               "Connection: close\r\n",
               statcode, http_statlookup(statcode), tbuf, PROTOBASE);

    if (content_length >= 0)
        queue_text(d, "Content-Length: %d\r\n", content_length);

    if (content_type)
        queue_text(d, "Content-Type: %s\r\n", content_type);
    else
        queue_text(d, "Content-Type: text/plain\r\n");


    f = http_fieldlookup(d, "Accept-Encoding");
    
    if (f && f->data && equalstr("*gzip*", f->data)) {
        iscompressed = 1;
        queue_text(d, "Content-Encoding: gzip\r\n");
    }

    queue_text(d, "\r\n");

    if (iscompressed) {
       process_output(d);
       mccp_start(d, 0);
    }
}

void
http_sendredirect(struct descriptor_data *d, const char *url)
{
    const char *statmsg = http_statlookup(301);
    char *host = alloc_string(http_gethost(d));
    time_t t = time(NULL) + get_tz_offset();
    char tbuf[50];
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];

    escape_url(buf2, (char *) url);
    http_split(host, ':');
    format_time(tbuf, 48, "%a, %d %b %Y %T GMT", localtime(&t));

    sprintf(buf, "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
            "<html><head>\r\n  <title>%d %s</title>\r\n  </head><body>\r\n"
            "<h1>%s</h1>\r\n  <p>The document has moved <a href=\"%s\">here"
            "</a>.</p>\r\n  <hr />\r\n  <address>ProtoMUCK %s Server at %s"
            " Port %d</address>\r\n</body></html>\r\n", 301, statmsg, statmsg,
            buf2, PROTOBASE, host, d->cport);
    queue_text(d, "HTTP/1.1 %d %s\r\nDate: %s\r\nServer: ProtoMUCK/%s\r\n"
               "Location: %s\r\nConnection: close\r\nContent-Type: text/h"
               "tml; charset=iso-8859-1\r\nContent-Length: %d\r\n\r\n", 301,
               statmsg, tbuf, PROTOBASE, buf2, strlen(buf));
    queue_text(d, buf);

    d->booted = 4;

    free((void *) host);
}

/* http_senderror():                                    */
/*   Create and send an error page.                     */
void
http_senderror(struct descriptor_data *d, int statcode, const char *msg)
{
    const char *statmsg;
    char *host = alloc_string(http_gethost(d));
    char buf[BUFFER_LEN];

    statmsg = http_statlookup(statcode);

    http_log(d, 2, "ERROR:   '%d, %s'\n", statcode, msg);
    http_split(host, ':');

    sprintf(buf, "<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\r\n"
            "<html><head>\r\n  <title>%d %s</title>\r\n"
            "</head><body>\r\n  <h1>%s</h1>\r\n"
            "  <p>%s<br /></p>\r\n  <hr />\r\n"
            "  <address>ProtoMUCK %s Server at %s Port %d</address>\r\n"
            "</body></html>\r\n", statcode, statmsg, statmsg, msg, PROTOBASE,
            host, d->cport);

    http_sendheader(d, statcode, "text/html; charset=iso-8859-1", strlen(buf));
    queue_text(d, buf);

    free((void *) host);

    d->http->body.elen = 0;
    d->http->body.len = 0;
    d->booted = 4;
    return;
}

/* smart_prop_getref():                                 */
/*   Get a dbref from a prop, smartly.                  */
dbref
smart_prop_getref(dbref what, const char *propname)
{
    const char *tmpchar = NULL;
    dbref ref = NOTHING;

    if (((ref = get_property_dbref(what, propname)) != NOTHING) ||
        (tmpchar = get_property_class(what, propname))) {
        if (tmpchar) {
            if (*tmpchar == NUMBER_TOKEN && number(tmpchar + 1)) {
                ref = (dbref) atoi(++tmpchar);
            } else if (*tmpchar == REGISTERED_TOKEN) {
                ref = find_registered_obj(what, tmpchar);
            } else if (number(tmpchar)) {
                ref = (dbref) atoi(tmpchar);
            } else {
                ref = NOTHING;
            }
        } else {
            if (ref == AMBIGUOUS)
                ref = NOTHING;
        }
    }

    return ref;
}

char *
http_parsempi(struct descriptor_data *d, dbref what, const char *yerf,
              char *buf)
{
    d->http->flags |= HS_MPI;

    if (yerf)
        return (do_parse_mesg
                (d->descriptor, OWNER(what), what, yerf, "WWW", buf, 0));
    else
        return "";
}

int
http_parsedest(struct descriptor_data *d)
{
    char buf[BUFFER_LEN];
    char buf2[BUFFER_LEN];
    char *cgi, *dir = NULL;
    char *p, *q;
    const char *s;
    dbref ref = NOTHING;

    strcpy(buf, d->http->dest);
    cgi = http_split(buf, '?');

    for (p = buf; *p && *p == '/'; p++) ;
    unescape_url(p);

    if (tp_web_allow_vhosts && OkObj(tp_www_root)
        && (d->http->smethod->flags & HS_VHOST)) {
        char *host = alloc_string(http_gethost(d));

        http_split(host, ':');
        http_log(d, 3, "VHOST:   '%s'\n", host);
        sprintf(buf2, "@vhosts/%s", host);
        if (is_propdir(tp_www_root, buf2)) {
            sprintf(buf2, "@vhosts/%s/rootObj", host);
            ref = smart_prop_getref(tp_www_root, buf2);

            sprintf(buf2, "@vhosts/%s/rootDir", host);
            s = get_property_class(tp_www_root, buf2);

            dir = alloc_string(s);
            if (OkObj(ref) || dir)
                d->http->flags |= HS_VHOST;
        }

        /* else { */
        /* I don't plan to support <player>.somename.com vhosts, */
        /* but if I did, this is where it'd go.                  */
        /* } */
        free((void *) host);
    }

    if (tp_web_allow_players && *p == '~'
        && (d->http->smethod->flags & HS_PLAYER)
        && !(d->http->flags & HS_VHOST)) {
        p++;
        q = http_split(p, '/');
        http_log(d, 4, "PLAYER:  '%s'\n", p);

        ref = lookup_player(p);
        if (!OkObj(ref)) {
            http_senderror(d, 404, "Player not found.");
            return 1;
        }
        d->http->flags |= HS_PLAYER;
        p = q;
    }

    if (!OkObj(ref))
        ref = tp_www_root;

    if (dir)
        d->http->rootdir = dir;
    else
        d->http->rootdir = string_dup("/_/www");

    d->http->cgidata = string_dup(cgi);
    d->http->newdest = string_dup(p);
    d->http->rootobj = ref;

    /* http_log(d, 3, "URL:     '%s'\n", d->http->newdest); */
    http_log(d, 4, "ROOTOBJ: '%s'\n", unparse_object(1, d->http->rootobj));
    http_log(d, 4, "ROOTDIR: '%s'\n", d->http->rootdir);
    if (cgi && *cgi)
        http_log(d, 5, "CGIDATA: '%s'\n", d->http->cgidata);

    return 0;

}

stk_array *
http_formarray(const char *data)
{
    stk_array *nw, *nw2;
    struct inst temp1;
    char *p, *q, *s, *buf;
    int i;

    nw = new_array_dictionary();

    if (!(buf = (char *) malloc(strlen(data) + 2 * sizeof(char))))
        return (nw);

    strcpy(buf, data);

    do {
        p = http_split(buf, '&');
        q = http_split(buf, '=');

        unescape_url(buf);
        if (strlen(buf)) {
            http_log(NULL, 6, "FIELD:   '%s' (%d)\n", buf, strlen(buf));
            unescape_url(q);
            nw2 = new_array_packed(0);
            do {
                s = http_split(q, '\n');
                i = strlen(q);
                while (i-- > 0 && q[i] == '\r')
                    q[i] = '\0';

                temp1.type = PROG_STRING;
                temp1.data.string = alloc_prog_string(q);
                array_appenditem(&nw2, &temp1);
                CLEAR(&temp1);

                http_log(NULL, 6, "LINE:    '%s' (%d)\n", q, strlen(q));
                strcpy(q, s);
            } while (*q);

            array_set_strkey_arrval(&nw, buf, nw2);
        }
        strcpy(buf, p);
    } while (*buf);

    free((void *) buf);

    return (nw);

}

stk_array *
http_makearray(struct descriptor_data *d)
{
    stk_array *nw = new_array_dictionary();
    char *p = d->http->body.data;

    array_set_strkey_intval(&nw, "DESCR", d->descriptor);
    array_set_strkey_intval(&nw, "CONNECTED", d->connected);
    array_set_strkey_strval(&nw, "HOST", host_as_hex(d->hu->h->a));
    array_set_strkey_intval(&nw, "CONNECTED_AT", (int) d->connected_at);
    array_set_strkey_intval(&nw, "LAST_TIME", (int) d->last_time);
    array_set_strkey_intval(&nw, "COMMANDS", d->commands);
    array_set_strkey_intval(&nw, "PORT", d->cport);
    array_set_strkey_strval(&nw, "HOSTNAME", d->hu->h->name);
    array_set_strkey_strval(&nw, "USERNAME", d->hu->u->user);
    array_set_strkey_strval(&nw, "Method", d->http->method);
    array_set_strkey_strval(&nw, "TheDEST", d->http->dest);
    array_set_strkey_strval(&nw, "HTTPVer", d->http->ver);
    array_set_strkey_intval(&nw, "SID", d->descriptor);
    array_set_strkey_intval(&nw, "Flags", d->http->flags);

    if ((d->http->smethod->flags & HS_BODY) && d->http->body.len && p) {
        array_set_strkey_intval(&nw, "BODYLen", d->http->body.len);
        if (strlen(p) < BUFFER_LEN)
            array_set_strkey_strval(&nw, "BODY", p);
        array_set_strkey_arrval(&nw, "POSTData", http_formarray(p));
    }

    if (d->http->cgidata && strlen(d->http->cgidata) < BUFFER_LEN) {
        array_set_strkey_strval(&nw, "CGIParams", d->http->cgidata);
        if (*d->http->cgidata)
            array_set_strkey_arrval(&nw, "CGIData",
                                    http_formarray(d->http->cgidata));
    }

    if (d->http->fields) {
        struct inst temp1;
        struct http_field *f;
        stk_array *nw2 = new_array_packed(0);
        stk_array *nw3 = new_array_dictionary();

        for (f = d->http->fields; f; f = f->next) {
            if (f->field && f->data && strlen(f->data) < BUFFER_LEN) {
                temp1.type = PROG_STRING;
                temp1.data.string = alloc_prog_string(f->field);
                array_appenditem(&nw2, &temp1);
                array_set_strkey_strval(&nw3, f->field, f->data);
                CLEAR(&temp1);
            }
        }

        array_set_strkey_arrval(&nw, "HeaderFields", nw2);
        array_set_strkey_arrval(&nw, "HeaderData", nw3);
    }

    return nw;
}

int
http_compile(struct descriptor_data *d, dbref player, dbref ref)
{
    struct line *tmpline;

    if (!DBFETCH(ref)->sp.program.start) {
        tmpline = DBFETCH(ref)->sp.program.first;
        DBFETCH(ref)->sp.program.first = read_program(ref);
        do_compile(d->descriptor, player, ref, 0);
        free_prog_text(DBFETCH(ref)->sp.program.first);
        DBSTORE(ref, sp.program.first, tmpline);
    }

    return (DBFETCH(ref)->sp.program.siz);
}

int
http_dohtmuf(struct descriptor_data *d, const char *prop)
{
    char buf[BUFFER_LEN];
    const char *m = NULL;
    struct frame *tmpfr;
    dbref ref, player;

    if (*prop && (Prop_Hidden(prop) || Prop_Private(prop)))
        return 0;

    if ((ref = smart_prop_getref(d->http->rootobj, prop)) < 0)
        return 0;

    if (!OkObj(ref)) {
        http_senderror(d, 403, "Invalid program dbref.");
        return 1;
    } else if (Typeof(ref) != TYPE_PROGRAM) {
        http_senderror(d, 415, "Dbref not of type program.");
        return 1;
    } else if (!tp_web_allow_playerhtmuf && (d->http->flags & HS_PLAYER)) {
        http_senderror(d, 403, "Player HTMuf programs are currently disabled.");
        return 1;
    }

    player = OWNER(ref);
    /* Insanity check! */
    if (!OkObj(player))
        return 0;

    /* Permissions checks. Player and program must      */
    /* be web_htmuf_mlvl+, and program must be LINK_OK. */
    if (MLevel(player) < tp_web_htmuf_mlvl || MLevel(ref) < tp_web_htmuf_mlvl) {
        http_senderror(d, 403,
                       "Permission denied. (MLevel of program or player too low.)");
        return 1;
    } else if (!(FLAGS(ref) & LINK_OK)) {
        http_senderror(d, 403, "Permission denied. (Program not set LINK_OK.)");
        return 1;
    } else if (!http_compile(d, player, ref)) {
        http_senderror(d, 500, "Program not compilable.");
        return 1;
    }

    /* Get the type. */
    if ((m = get_property_class(ref, "/_type")))
        strcpy(buf, m);
    else
        strcpy(buf, "text/html");

    d->http->flags |= HS_HTMUF;

    /* Send the header. */
    if (string_compare(buf, "noheader"))
        http_sendheader(d, 200, buf, -1);

    /* Do it! */
    sprintf(match_args, "%d|%s|%s|%s", d->descriptor, d->hu->h->name,
            d->hu->u->user, d->http->cgidata);
    strcpy(match_cmdname, "(WWW)");
    tmpfr =
        interp(d->descriptor, player, NOTHING, ref, d->http->rootobj,
               BACKGROUND, STD_HARDUID, 0);
    if (tmpfr) {
        struct inst temp1;
        stk_array *nw = new_array_dictionary();

        array_set_strkey_intval(&nw, "caller_pid", tmpfr->pid);
        array_set_strkey_intval(&nw, "descr", tmpfr->descr);
        array_set_strkey_refval(&nw, "caller_prog", ref);
        array_set_strkey_refval(&nw, "trigger", tmpfr->trig);
        array_set_strkey_refval(&nw, "prog_uid", player);
        array_set_strkey_refval(&nw, "player", player);
        array_set_strkey_arrval(&nw, "data", http_makearray(d));

        temp1.type = PROG_ARRAY;
        temp1.data.array = nw;
        muf_event_add(tmpfr, "USER.SOCKINFO", &temp1, 0);
        CLEAR(&temp1);

        interp_loop(player, ref, tmpfr, 1);

        d->http->pid = tmpfr->pid;
    }
    d->booted = 4;
    return 1;
}

int
http_doproplist(struct descriptor_data *d, dbref what, const char *prop,
                int statcode)
{
    char buf[BUFFER_LEN];
    const char *m = NULL;
    int lines = 0;
    int i;

    if (!OkObj(what))
        return 0;

    if (*prop && (Prop_Hidden(prop) || Prop_Private(prop)))
        return 0;

    /* Get lines count. */
    sprintf(buf, "%s#", prop);
    if ((lines = get_property_value(what, buf))
        || (m = get_property_class(what, buf))) {
        if (number(m))
            lines = atoi(m);
    }

    /* If 0 or below, exit. */
    sprintf(buf, "%s#/%d", prop, lines);
    if (lines <= 0 || !get_property_class(what, buf))
        return 0;

    /* Get the type. */
    sprintf(buf, "%s/_type", prop);
    if ((m = get_property_class(what, buf)))
        strcpy(buf, m);
    else
        strcpy(buf, "text/html");

    d->http->flags |= HS_PROPLIST;
    /* Send the header. */
    if (string_compare(buf, "noheader"))
        http_sendheader(d, statcode, buf, -1);

    /* Send the list. */
    for (i = 1; i <= lines; i++) {
        sprintf(buf, "%s#/%d", prop, i);
        if ((m = get_property_class(what, buf))) {
            if (tp_web_allow_mpi && *m == '&')
                m = http_parsempi(d, what, ++m, buf);

            queue_text(d, "%s\r\n", m);
        }
    }

    d->booted = 4;
    return i;
}

int
http_sendfile(struct descriptor_data *d, const char *filename)
{
    const char *p, *type;
    long i;

    if (tp_web_max_files && (httpfcount + 1) > tp_web_max_files) {
        http_senderror(d, 503, "Too many file transfers.");
        return 1;
    }

    if ((i = descr_sendfile(d, -1, -1, filename, -1)) < 0)
        return 0;

    if (tp_web_max_filesize && i > (tp_web_max_filesize * 1024L)) {
        http_senderror(d, 500, "Requested file too large.");
        return 1;
    }

    if ((p = strrchr(filename, '.'))) {
        if (!(type = http_mimelookup(++p)))
            type = "application/octet-stream";
    } else {
        type = "application/octet-stream";
    }

    httpfcount++;

    /* This should send out proper file dates. */

    http_sendheader(d, 200, type, i);
    descr_sendfileblock(d);

    return 1;
}

int
fileexists(const char *filename)
{
    FILE *fp;

    if ((fp = fopen(filename, "r")))
        fclose(fp);

    return ((fp != NULL));
}

void
http_listdir(struct descriptor_data *d, const char *dir, DIR * df)
{
    char buf2[BUFFER_LEN];
    char buf3[BUFFER_LEN];
    char buf[BUFFER_LEN];
    char url[BUFFER_LEN];
    struct dirent *dp;
    struct stat fs;
    char tbuf[60];
    DIR *dh;
    int i;

    if (!tp_web_allow_dirlist) {
        http_senderror(d, 403, "Directory listings are currently disabled.");
        return;
    }

    http_sendheader(d, 200, "text/html; charset=iso-8859-1", -1);
    queue_text(d,
               "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\">\r\n"
               "<html><head>\r\n<title>Index of /%s</title>\r\n</head><body>"
               "\r\n<h1>Index of /%s</h1>\r\n<pre>   Name                    "
               "       Last modified           Size<hr />\r\n",
               d->http->newdest, d->http->newdest);

    while ((dp = readdir(df))) {
        if (*(dp->d_name) != '.') {
            strcpy(buf, dp->d_name);
            sprintf(buf2, "%s/%s", dir, dp->d_name);
            if ((dh = opendir(buf2))) {
                closedir(dh);
                strcat(buf, "/");
                strcpy(buf2, "-");
                strcpy(tbuf, "");
            } else {
                sprintf(buf2, "%s/%s%s", HTTP_DIR, d->http->newdest, buf);
                if (!stat(buf2, &fs)) {
                    sprintf(buf2, "%d", (int) fs.st_size);
                    format_time(tbuf, 40, "%d-%b-%Y %H:%M\0",
                                localtime(&fs.st_mtime));
                } else {
                    continue;
                }
            }

            sprintf(url, "/%s%s", d->http->newdest, buf);
            escape_url(buf3, url);
            i = 30 - strlen(buf);
            queue_text(d, "   <a href=\"%s\">%s</a>%-*s %-24s %s\r\n", buf3,
                       buf, i < 0 ? 0 : i, "", tbuf, buf2);
        }
    }

    queue_text(d, "<hr /></pre>\r\n<address>ProtoMUCK %s Server at %s"
               " Port %d</address>\r\n</body></html>\r\n", PROTOBASE,
               http_gethost(d), d->cport);
}

int
http_dofile(struct descriptor_data *d)
{
    char buf[BUFFER_LEN];
    const char **m = http_defaulthomes;
    int i = 0;
    DIR *df;

    if (strstr(d->http->newdest, "..") != NULL)
        return 0;

    if ((i = strlen(d->http->newdest)) > BUFFER_LEN)
        return 0;

    if (!*d->http->newdest || (i && d->http->newdest[i - 1] == '/')) {
        for (; *m; m++) {
            if (**m) {
                sprintf(buf, "%s/%s%s", HTTP_DIR, d->http->newdest, *m);
                if ((i = http_sendfile(d, buf)))
                    return i;
            }
        }
    }

    sprintf(buf, "%s/%s", HTTP_DIR, d->http->newdest);

    if ((df = opendir(buf))) {
        if (i && d->http->newdest[i - 1] != '/') {
            sprintf(buf, "http://%s/%s/", http_gethost(d), d->http->newdest);
            http_sendredirect(d, buf);
        } else {
            http_listdir(d, buf, df);
            d->booted = 4;
        }

        closedir(df);
        return 1;
    }

    return (http_sendfile(d, buf));
}

int
http_doprop(struct descriptor_data *d, const char *prop)
{
    char buf[BUFFER_LEN];
    const char *m, *s;

    if (!OkObj(d->http->rootobj))
        return 0;

    if (*prop && (Prop_Hidden(prop) || Prop_Private(prop)))
        return 0;

    /* Get the propery value. */
    if (!(m = get_property_class(d->http->rootobj, prop)))
        return 0;

    if (!*m)
        return 0;

    if (tp_web_allow_mpi && *m == '&') {
        sprintf(buf, "%s/_type", prop);
        if ((s = get_property_class(d->http->rootobj, buf)))
            strcpy(buf, m);
        else
            strcpy(buf, "text/html");

        /* Send the header. */
        if (string_compare(buf, "noheader"))
            http_sendheader(d, 200, buf, -1);

        queue_text(d, http_parsempi(d, d->http->rootobj, ++m, buf));
    } else {
        d->http->flags |= HS_REDIRECT;
        http_sendredirect(d, m);
    }

    d->booted = 4;
    return 1;
}

int
http_dourl(struct descriptor_data *d)
{
    char prop[BUFFER_LEN];
    int i;

    if (!OkObj(d->http->rootobj)) {
        http_senderror(d, 404, "Page not found.");
        return -1;
    }

    sprintf(prop, "%s/%s", d->http->rootdir, d->http->newdest);
    i = strlen(prop);
    while ((i-- > 0) && (prop[i] == '/'))
        prop[i] = '\0';

    if ((d->http->smethod->flags & HS_PROPLIST)
        && http_doproplist(d, d->http->rootobj, prop, 200))
        return 1;

    if (tp_web_allow_htmuf && (d->http->smethod->flags & HS_HTMUF)
        && http_dohtmuf(d, prop))
        return 1;

    if (tp_web_allow_mpi && http_doprop(d, prop))
        return 1;

    if (tp_web_allow_files && (d->http->smethod->flags & HS_FILE)
        && http_dofile(d))
        return 1;

    /* If it's a propdir and nothing else was found, */
    /* send a 403 error. Eventually, this'll be used */
    /* to display a directory listing.               */
    if (is_propdir(d->http->rootobj, prop)) {
        http_senderror(d, 403, "Access denied.");
        return -1;
    }

    http_senderror(d, 404, "Page not found.");
    return 0;
}

/* ---Handlers--- */
void
http_handler_get(struct descriptor_data *d)
{
    http_dourl(d);

    return;
}

void
http_handler_post(struct descriptor_data *d)
{
    http_dourl(d);

    return;
}

/* ---End Handlers--- */

/* http_processcontent():                               */
/*   Process message body stuff. Called in interface.c. */
int
http_processcontent(struct descriptor_data *d, const char in)
{
    if (d->booted || !d->http->body.elen)
        return 1;

    d->http->body.data[d->http->body.len] = in;
    d->http->body.len++;

    /* Update that idletime! */
    d->last_time = time(NULL);

    /* Finished? */
    if (d->http->body.len >= d->http->body.elen) {
        d->http->body.data[d->http->body.len] = '\0';
        http_finish(d);
        return 1;
    }

    return 0;
}

/* http_process_input():                                */
/*   Process input stuff. Called in interface.c.        */
void
http_process_input(struct descriptor_data *d, const char *input)
{
    struct http_field *f;
    char buf[BUFFER_LEN];
    char *p, *q;
    int i;

    if (d->http->body.elen || d->booted || d->type != CT_HTTP)
        return;                 /* It's not ours. Handle it elsewhere. */

    strcpy(buf, input);
    i = strlen(buf);
    while (i-- > 0 && buf[i] && (buf[i] == '\n' || buf[i] == '\r'))
        buf[i] = '\0';

    http_log(d, 5, "INPUT:   '%s' (%d)\n", buf, strlen(input));

    d->last_time = time(NULL);
    d->commands++;

    if (!strlen(buf)) {
        /* Empty string means bare newline. */
        if (!d->http->smethod)
            http_processheader(d);
        return;
    }

    if (!d->http->method) {
        p = http_split(buf, ' ');
        q = http_split(p, ' ');
        d->http->method = string_dup(buf);
        d->http->ver = string_dup(q);

        /* Strip all but one / from beginning of dest. */
        for (; *p && *p == '/' && *(p + 1) == '/'; p++) ;
        d->http->dest = string_dup(p);

        http_log(d, 1, "WWW: %d %s '%s' %s(%s)\n", d->descriptor,
                 d->http->method, d->http->dest, d->hu->h->name,
                 d->hu->u->user);
        http_log(d, 4, "VER:     '%s'\n", d->http->ver);
    } else {
        p = http_split(buf, ':');
        while (*p && isspace(*p))
            p++;

        if (!buf[0])
            return;

        if ((f = http_fieldlookup(d, buf))) {
            /* If one already exists, copy the new data into it. */
            if (f->data) {
                if ((i = strlen(f->data) + strlen(p) + 4) < BUFFER_LEN) {
                    MALLOC(q, char, i);

                    sprintf(q, "%s, %s", f->data, p);
                    free((void *) f->data);
                    f->data = q;
                }
            } else {
                f->data = string_dup(p);
            }
        } else {
            MALLOC(f, struct http_field, 1);

            f->field = string_dup(buf);
            f->data = string_dup(p);
            f->next = d->http->fields;

            d->http->fields = f;
        }
    }

    return;
}

/* http_processheader():                                */
/*   Called from process_input() when a bare newline is */
/*   received. Processes all the header information.    */
void
http_processheader(struct descriptor_data *d)
{
    struct http_field *f;

    if (!OkObj(tp_www_root)) {
        /* Bad webroot @tune. */
        http_senderror(d, 503, "Service unavailable. (Bad webroot @tune)");
        return;
    }

    if (!d->http->method || (d->http->method && !*d->http->method)
        || !d->http->dest || !d->http->ver || (d->http->ver
                                               && !*d->http->ver)) {
        /* No method? Bad request. */
        http_senderror(d, 400, "A malformed request was sent to the server.");
        return;
    }

    if (string_compare(d->http->ver, "HTTP/1.1")
        && string_compare(d->http->ver, "HTTP/1.0")) {
        /* No point wasting time if it's not the right version. */
        http_senderror(d, 505, "Only HTTP/1.1 is supported at this time.");
        return;
    }

    d->http->smethod = http_methodlookup(d->http->method);

    if (!d->http->smethod || !d->http->smethod->handler) {
        /* No method? No handler? No service. */
        http_senderror(d, 501, "Method not implemented or not supported.");
        return;
    }

    if (d->http->fields && (d->http->smethod->flags & HS_BODY)
        && (f = http_fieldlookup(d, "Content-Length"))) {
        /* Handle message-body stuff. */
        if (number(f->data)) {
            d->http->flags |= HS_BODY;
            /* Content-Length can be zero, which is perfectly legal. */
            /* If it is, just continue through to http_finish(). */
            if ((d->http->body.elen = atoi(f->data))) {
                if (d->http->body.elen < 0) { /* It -can- be 0, but not below zero. */
                    http_senderror(d, 400,
                                   "A malformed request was sent to the server.");
                } else if (tp_web_max_filesize
                           && d->http->body.elen >
                           (tp_web_max_filesize * 1024L)) {
                    http_senderror(d, 413, "Message body too large.");
                } else
                    if (!
                        (d->http->body.data =
                         (char *) malloc(d->http->body.elen +
                                         2 * sizeof(char)))) {
                    http_senderror(d, 413, "Not enough memory.");
                }

                /* Musn't continue onto http_finish(). */
                return;
            }
        } else {
            /* Content-Length wasn't a number, which is illegal. Error out. */
            http_senderror(d, 400,
                           "A malformed request was sent to the server.");
            return;
        }
    } else if (d->http->smethod->flags & HS_BODY) {
        /* No Content-Length field, and method is set to require one. */
        http_senderror(d, 411, "Method requires a Content-Length field.");
        return;
    }

    /* Certain checks fall through to here. */
    http_finish(d);
}

/* http_finish():                                       */
/*   Called from various other functions. It handles    */
/*   actually doing stuff, like calling method handlers.*/
void
http_finish(struct descriptor_data *d)
{
    if (d->http->body.len && d->http->body.len < MAX_COMMAND_LEN
        && d->http->body.data)
        http_log(d, 4, "BODY:    '%s' (%d)\n", d->http->body.data,
                 d->http->body.len);

    if (http_parsedest(d))
        return;

    if (d->http->smethod->handler)
        d->http->smethod->handler(d);

    return;
}

void
http_initstruct(struct descriptor_data *d)
{
    d->http = (struct http_struct *) malloc(sizeof(struct http_struct));

    if (d->type == CT_HTTP) {
        d->http->rootobj = NOTHING;
        d->http->rootdir = NULL;
        d->http->smethod = NULL;
        d->http->cgidata = NULL;
        d->http->newdest = NULL;
        d->http->method = NULL;
        d->http->fields = NULL;
        d->http->dest = NULL;
        d->http->ver = NULL;
        d->http->flags = 0;
        d->http->pid = 0;

        d->http->body.data = NULL;
        d->http->body.elen = 0;
        d->http->body.len = 0;
        d->http->body.curr = 0;

        if (tp_web_max_users && httpucount++ > tp_web_max_users) {
            http_senderror(d, 503, "Too many users connected to service.");
        }
    }

    return;
}

void
http_deinitstruct(struct descriptor_data *d)
{
    struct http_field *f;

    if (!d->http)
        return;

    if (d->http->dest)
        free((void *) d->http->dest);
    if (d->http->ver)
        free((void *) d->http->ver);
    if (d->http->method)
        free((void *) d->http->method);
    if (d->http->rootdir)
        free((void *) d->http->rootdir);
    if (d->http->cgidata)
        free((void *) d->http->cgidata);
    if (d->http->newdest)
        free((void *) d->http->newdest);
    if (d->http->body.data)
        free((void *) d->http->body.data);

    while ((f = d->http->fields)) {
        d->http->fields = f->next;
        if (f->field)
            free((void *) f->field);
        if (f->data)
            free((void *) f->data);
        free((void *) f);
    }

    httpucount--;

    free((void *) d->http);
    d->http = NULL;

    return;
}

#endif /* NEWHTTPD */

#define CHAR64(c) (((c) < 0 || (c) > 127) ? -1 : index_64[(c)])

static char basis_64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char index_64[128] = {
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1,
    -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1,
    -1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1
};

int
http_encode64(const char *_in, unsigned inlen, char *_out)
{
    const unsigned char *in = (const unsigned char *) _in;
    unsigned char *out = (unsigned char *) _out;
    unsigned char oval;

    while (inlen >= 3) {
        *out++ = basis_64[in[0] >> 2];
        *out++ = basis_64[((in[0] << 4) & 0x30) | (in[1] >> 4)];
        *out++ = basis_64[((in[1] << 2) & 0x3c) | (in[2] >> 6)];
        *out++ = basis_64[in[2] & 0x3f];
        in += 3;
        inlen -= 3;
    }

    if (inlen > 0) {
        *out++ = basis_64[in[0] >> 2];
        oval = (in[0] << 4) & 0x30;
        if (inlen > 1)
            oval |= in[1] >> 4;
        *out++ = basis_64[oval];
        *out++ = (inlen < 2) ? '=' : basis_64[(in[1] << 2) & 0x3c];
        *out++ = '=';
    }

    *out = '\0';
    return 0;
}

int
http_decode64(const char *in, unsigned inlen, char *out)
{

    unsigned lup;
    int c1, c2, c3, c4;

    if (in[0] == '+' && in[1] == ' ')
        in += 2;

    if (*in == '\0')
        return -1;

    for (lup = 0; lup < inlen / 4; lup++) {
        c1 = in[0];
        if (CHAR64(c1) == -1)
            return -1;
        c2 = in[1];
        if (CHAR64(c2) == -1)
            return -1;
        c3 = in[2];
        if (c3 != '=' && CHAR64(c3) == -1)
            return -1;
        c4 = in[3];
        if (c4 != '=' && CHAR64(c4) == -1)
            return -1;
        in += 4;
        *out++ = (CHAR64(c1) << 2) | (CHAR64(c2) >> 4);
        if (c3 != '=') {
            *out++ = ((CHAR64(c2) << 4) & 0xf0) | (CHAR64(c3) >> 2);
            if (c4 != '=') {
                *out++ = ((CHAR64(c3) << 6) & 0xc0) | CHAR64(c4);
            }
        }
    }

    *out = '\0';
    return 0;

}
