/* Interface for the CGI parsing functions for the Web interface.
 * --Loki
 */

#ifndef CGI_H
#define CGI_H

extern void unescape_url(char *url);
extern void escape_url(char *out, char *in);

#define SPACE       ' '
#define TAB         '\t'
#define LF          '\n'
#define CR          '\r'
#define PLUS        '+' 
#define PERCENT     '%' 
#define EQUALS      '='
#define AMPERSAND   '&'
#define HYPHEN      '-'
#define UNDERSCORE  '_'
#define ATSIGN      '@'
#define SLASH       '/'

#define SEPARATOR   "&"
#define TRUE        1  
#define FALSE       0 

#endif /* CGI_H */

