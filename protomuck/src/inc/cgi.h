/* Interface for the CGI parsing functions for the Web interface.
 * --Loki
 */

extern char x2c(char *what);
extern void unescape_url(char *url);
extern char * getcgivar(char *cgiinput, char *param);
extern void escape_url(char *out, char *in);


