#ifdef NEWHTTPD

extern unsigned int httpucount;
extern unsigned int httpfcount;

extern int  array_set_strkey_arrval(stk_array **arr, const char *key, stk_array *arr2);
extern void http_log(struct descriptor_data *d, int debuglvl, char *format, ...);
extern int  queue_text(struct descriptor_data *d, char *format, ...);
extern char *http_split(char *s, int c);
extern const char *http_statlookup(int status);
extern struct http_method *http_methodlookup(const char *method);
extern struct http_field  *http_fieldlookup(struct descriptor_data *d, const char *field);
extern const char *http_gethost(struct descriptor_data *d);
extern void http_sendheader(struct descriptor_data *d, int statcode, const char *content_type, int content_length);
extern void http_sendredirect(struct descriptor_data *d, const char *url);
extern void http_senderror(struct descriptor_data *d, int statcode, const char *msg);
extern int http_parsedest(struct descriptor_data *d);
extern stk_array *http_formarray(const char *data);
extern stk_array *http_makearray(struct descriptor_data *d);
extern int http_compile(struct descriptor_data *d, dbref player, dbref ref);
extern int http_dohtmuf(struct descriptor_data *d, const char *prop);
extern int http_doproplist(struct descriptor_data *d, dbref what, const char *prop, int statcode);
extern int http_sendfile(struct descriptor_data *d, const char *filename);
extern int http_dofile(struct descriptor_data *d);
extern int http_dourl(struct descriptor_data *d);
extern int http_processcontent(struct descriptor_data *d, const char in);
extern void http_process_input(struct descriptor_data *d, const char *input);
extern void http_processheader(struct descriptor_data *d);
extern void http_finish(struct descriptor_data *d);
extern void http_initstruct(struct descriptor_data *d);
extern void http_deinitstruct(struct descriptor_data *d);
extern void http_handler_get(struct descriptor_data *d);
extern void http_handler_post(struct descriptor_data *d);
extern int http_decode64(const char *in, unsigned inlen, char *out);
extern int http_encode64(const char *_in, unsigned inlen, char *_out);


struct http_statstruct {
    int         code;
    const char *msg;
};

struct http_mimestruct {
    const char *ext;
    const char *type;
};

/* The logfile. */
#define HTTP_LOG "logs/webserver"
#define HTTP_DIR "files/public_html"

/* The following flags are used in the http_methods[] table to tell */
/* which options the method supports, and is also used in the main  */
/* struct to tell what kind of page was found.                      */
#define HS_PROPLIST     0x1   /* Method supports proplists.         */
#define HS_REDIRECT     0x2   /* Method supports redirection.       */
#define HS_PLAYER       0x4   /* Method supports player webpages.   */
#define HS_HTMUF        0x8   /* Method supports HTMuf programs.    */
#define HS_VHOST       0x10   /* Method supports virtual hosts.     */
#define HS_FILE        0x20   /* Method supports server-side files. */
#define HS_BODY        0x40   /* Method requires a message body.    */
#define HS_MPI         0x80   /* Method supports MPI programs.      */

#endif /* NEWHTTPD */
