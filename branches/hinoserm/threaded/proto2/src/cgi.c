#include "config.h"
#include "cgi.h"

#ifdef BUFFER_LEN
#undef BUFFER_LEN
#endif
#define BUFFER_LEN 4096

#ifdef WIN_VC
# define vcSTRCASECMP(x,y) stricmp((x),(y))
#else
# define vcSTRCASECMP(x,y) strcasecmp((x),(y))
#endif

/** Convert a two-char hex string into the char it represents **/
static char
x2c(char *what)
{
    char digit;

    digit = (what[0] >= 'A' ? ((what[0] & 0xdf) - 'A') + 10 : (what[0] - '0'));
    digit *= 16;
    digit += (what[1] >= 'A' ? ((what[1] & 0xdf) - 'A') + 10 : (what[1] - '0'));
    return (digit);
}

/** Reduce any %xx escape sequences to the characters they represent **/
void
unescape_url(char *url)
{
    register int i, j;
    char *p;

    for (p = url; *p; p++)
        if (*p == '+')
            *p = ' ';

    for (i = 0, j = 0; url[j]; ++i, ++j) {
        if ((url[i] = url[j]) == '%') {
            url[i] = x2c(&url[j + 1]);
            if (!url[j+1] || !url[j+2]) break;
            j += 2;
        }
    }
    url[i] = '\0';
}

void
escape_url(char *out, char *in)
{
    char *tmp = out;
    int cnt = 0;
    int isvalid_cgichar(char);

    for (; *in != '\0'; in++, tmp++, cnt++) {
        if (*in == SPACE)
            *tmp = PLUS;
        else if (isvalid_cgichar(*in))
            *tmp = *in;
        else {
            sprintf(tmp, "%%%.2X", *in);
            cnt += 2;
            tmp += 2;
        }
    }

    *tmp = '\0';

}

int
isvalid_cgichar(char c)
{
    if (isalnum(c) || (c == UNDERSCORE) || (c == HYPHEN) || (c == ':') ||
/*      (c == '\\') || */
        (c == '/') || (c == '.') ||
/*      (c == PLUS) ||
 *      (c == TAB) || 
 *      (c == ATSIGN) ||
 */     (c == PERCENT))
        return (1);

    else
        return (0);
}
