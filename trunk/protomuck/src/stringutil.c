#include "copyright.h"
#include "config.h"
#include "db.h"
#include "tune.h"
#include "props.h"
#include "params.h"
#include "interface.h"
#include "interp.h"

/* String utilities */

#include <stdio.h>
#include <ctype.h>
#include "externs.h"

extern const char *uppercase, *lowercase;

#define DOWNCASE(x) (lowercase[x])

#ifdef COMPRESS
extern const char *uncompress(const char *);
#else
#define uncompress(x) x
#endif				/* COMPRESS */

/*
 * routine to be used instead of strcasecmp() in a sorting routine
 * Sorts alphabetically or numerically as appropriate.
 * This would compare "network100" as greater than "network23"
 * Will compare "network007" as less than "network07"
 * Will compare "network23a" as less than "network23b"
 * Takes same params and returns same comparitive values as strcasecmp.
 * This ignores minus signs and is case insensitive.
 */
int alphanum_compare(const char *t1, const char *s2)
{
    int n1, n2, cnt1, cnt2;
    const char *u1, *u2;
    register const char *s1 = t1;
  
    while (*s1 && DOWNCASE(*s1) == DOWNCASE(*s2))
        s1++, s2++;
  
    /* if at a digit, compare number values instead of letters. */
    if (isdigit(*s1) && isdigit(*s2)) {
        u1 = s1;  u2 = s2;
        n1 = n2 = 0;  /* clear number values */
        cnt1 = cnt2 = 0;
  
        /* back up before zeros */
	if (s1 > t1 && *s2 == '0') s1--, s2--;     /* curr chars are diff */
        while (s1 > t1 && *s1 == '0') s1--, s2--;  /* prev chars are same */
        if (!isdigit(*s1)) s1++, s2++;
  
        /* calculate number values */
        while (isdigit(*s1)) cnt1++, n1 = (n1 * 10) + (*s1++ - '0');
        while (isdigit(*s2)) cnt2++, n2 = (n2 * 10) + (*s2++ - '0');
        
        /* if more digits than int can handle... */
        if (cnt1 > 8 || cnt2 > 8) {
            if (cnt1 == cnt2) return (*u1 - *u2);  /* cmp chars if mag same */
            return (cnt1 - cnt2);                  /* compare magnitudes */
        }
  
        /* if number not same, return count difference */
        if (n1 && n2 && n1 != n2) return (n1 - n2);
                                    
        /* else, return difference of characters */
        return (*u1 - *u2);
    }
    /* if characters not digits, and not the same, return difference */
    return (DOWNCASE(*s1) - DOWNCASE(*s2));
}

int
string_compare(register const char *s1, register const char *s2)
{
    while (*s1 && DOWNCASE(*s1) == DOWNCASE(*s2))
	s1++, s2++;

    return (DOWNCASE(*s1) - DOWNCASE(*s2));
}

const char *
exit_prefix(register const char *string, register const char *prefix)
{
    const char *p;
    const char *s = string;
    while (*s) {
        p = prefix;
	string = s;
        while(*s && *p && DOWNCASE(*s) == DOWNCASE(*p)) {
            s++; p++;
        }
	while (*s && isspace(*s)) s++;
        if (!*p && (!*s || *s == EXIT_DELIMITER)) {
	    return string;
        }
        while(*s && (*s != EXIT_DELIMITER)) s++;
        if (*s) s++;
	while (*s && isspace(*s)) s++;
    }
    return 0;
}

int
string_prefix(register const char *string, register const char *prefix)
{
    while (*string && *prefix && DOWNCASE(*string) == DOWNCASE(*prefix))
	string++, prefix++;
    return *prefix == '\0';
}

/* accepts only nonempty matches starting at the beginning of a word */
const char *
string_match(register const char *src, register const char *sub)
{
    if (*sub != '\0') {
	while (*src) {
	    if (string_prefix(src, sub))
		return src;
	    /* else scan to beginning of next word */
	    while (*src && isalnum(*src))
		src++;
	    while (*src && !isalnum(*src))
		src++;
	}
    }
    return 0;
}

/*
 * pronoun_substitute()
 *
 * %-type substitutions for pronouns
 *
 * %a/%A for absolute possessive (his/hers/its, His/Hers/Its)
 * %s/%S for subjective pronouns (he/she/it, He/She/It)
 * %o/%O for objective pronouns (him/her/it, Him/Her/It)
 * %p/%P for possessive pronouns (his/her/its, His/Her/Its)
 * %r/%R for reflexive pronouns (himself/herself/itself,
 *                                Himself/Herself/Itself)
 * %n    for the player's name.
 */
char   *
pronoun_substitute(int descr, dbref player, const char *str)
{
    char    c;
    char    d;
    char    prn[3];
    static char buf[BUFFER_LEN * 2];
    char    orig[BUFFER_LEN];
    char   *result;
    const char *self_sub;	/* self substitution code */
    dbref   mywhere = player;
    int sex;

    static const char *subjective[4] = {"", "it", "she", "he"};
    static const char *possessive[4] = {"", "its", "her", "his"};
    static const char *objective[4] = {"", "it", "her", "him"};
    static const char *reflexive[4] = {"", "itself", "herself", "himself"};
    static const char *absolute[4] = {"", "its", "hers", "his"};

    prn[0] = '%';
    prn[2] = '\0';

#ifdef COMPRESS
    str = uncompress(str);
#endif				/* COMPRESS */

    strcpy(orig, str);
    str = orig;

    sex = genderof(descr, player);
    result = buf;
    while (*str) {
	if (*str == '%') {
	    *result = '\0';
	    prn[1] = c = *(++str);
	    if (c == '%') {
		*(result++) = '%';
		str++;
	    } else {
		mywhere = player;
		d = (isupper(c)) ? c : toupper(c);

#ifdef COMPRESS
		if (d == 'A' || d == 'S' || d == 'O' ||
			d == 'P' || d == 'R' || d == 'N') {
		    self_sub = uncompress(get_property_class(mywhere, prn));
		} else {
		    self_sub = uncompress(envpropstr(&mywhere, prn));
		}
#else
		if (d == 'A' || d == 'S' || d == 'O' ||
			d == 'P' || d == 'R' || d == 'N') {
		    self_sub = get_property_class(mywhere, prn);
		} else {
		    self_sub = envpropstr(&mywhere, prn);
		}
#endif

		if (self_sub) {
		    if (((result - buf) + strlen(self_sub)) > (BUFFER_LEN - 2))
			return buf;
		    strcat(result, self_sub);
		    if (isupper(prn[1]) && islower(*result))
			*result = toupper(*result);
		    result += strlen(result);
		    str++;
		} else if (sex == GENDER_UNASSIGNED) {
		    switch (c) {
			case 'n':
			case 'N':
			case 'o':
			case 'O':
			case 's':
			case 'S':
			case 'r':
			case 'R':
			    strcat(result, PNAME(player));
			    break;
			case 'a':
			case 'A':
			case 'p':
			case 'P':
			    strcat(result, PNAME(player));
			    strcat(result, "'s");
			    break;
			default:
			    result[0] = *str;
			    result[1] = 0;
			    break;
		    }
		    str++;
		    result += strlen(result);
		    if ((result - buf) > (BUFFER_LEN - 2)) {
			buf[BUFFER_LEN - 1] = '\0';
			return buf;
		    }
		} else {
		    switch (c) {
			case 'a':
			case 'A':
			    strcat(result, absolute[sex]);
			    break;
			case 's':
			case 'S':
			    strcat(result, subjective[sex]);
			    break;
			case 'p':
			case 'P':
			    strcat(result, possessive[sex]);
			    break;
			case 'o':
			case 'O':
			    strcat(result, objective[sex]);
			    break;
			case 'r':
			case 'R':
			    strcat(result, reflexive[sex]);
			    break;
			case 'n':
			case 'N':
			    strcat(result, PNAME(player));
			    break;
			default:
			    *result = *str;
			    result[1] = '\0';
			    break;
		    }
		    if (isupper(c) && islower(*result)) {
			*result = toupper(*result);
		    }
		    result += strlen(result);
		    str++;
		    if ((result - buf) > (BUFFER_LEN - 2)) {
			buf[BUFFER_LEN - 1] = '\0';
			return buf;
		    }
		}
	    }
	} else {
	    if ((result - buf) > (BUFFER_LEN - 2)) {
		buf[BUFFER_LEN - 1] = '\0';
		return buf;
	    }
	    *result++ = *str++;
	}
    }
    *result = '\0';
    return buf;
}

#ifndef MALLOC_PROFILING

char   *
alloc_string(const char *string)
{
    char   *s;

    /* NULL, "" -> NULL */
    if (!string || !*string)
	return 0;

    if ((s = (char *) malloc(strlen(string) + 1)) == 0) {
	abort();
    }
    strcpy(s, string);
    return s;
}

struct shared_string *
alloc_prog_string(const char *s)
{
    struct shared_string *ss;
    int     length;

    if (s == NULL || *s == '\0')
	return (NULL);

    length = strlen(s);
    if ((ss = (struct shared_string *)
	 malloc(sizeof(struct shared_string) + length)) == NULL)
	abort();
    ss->links = 1;
    ss->length = length;
    bcopy(s, ss->data, ss->length + 1);
    return (ss);
}

#endif

#if !defined(MALLOC_PROFILING)
char   *
string_dup(const char *s)
{
    char   *p;

    p = (char *) malloc(1 + strlen(s));
    if (p)
	(void) strcpy(p, s);
    return (p);
}
#endif



char *
intostr(int i)
{
    static char num[16];
    int j, k;
    char *ptr2;

    k = i;
    ptr2 = num+14;
    num[15] = '\0';
    if (i < 0) i = -i;
    while (i) {
	j = i % 10;
	*ptr2-- = '0' + j;
	i /= 10;
    }
    if (!k) *ptr2-- = '0';
    if (k < 0) *ptr2-- = '-';
    return (++ptr2);
}


/*
 * Encrypt one string with another one.
 */

#define CHARCOUNT 96

static char enarr[256];
static int charset_count[] = {96, 0};
static int initialized_crypt = 0;

void
init_crypt(void)
{
    int i;
    for (i = 0; i <= 255; i++) enarr[i] = (char) i;
    for (i = 'A'; i <= 'M'; i++) enarr[i] = (char)enarr[i] + 13;
    for (i = 'N'; i <= 'Z'; i++) enarr[i] = (char)enarr[i] - 13;
    enarr['\r'] = 127;
    enarr[127] = '\r';
    initialized_crypt = 1;
}


const char *
strencrypt(const char *data, const char *key)
{
    static char linebuf[BUFFER_LEN];
    char buf[BUFFER_LEN + 8];
    const char *cp;
    unsigned char *ptr;
    unsigned char *ups;
    const unsigned char *upt;
    int linelen;
    int count;
    int seed, seed2, seed3;
    int limit = BUFFER_LEN;
    int result;

    if (!initialized_crypt)
        init_crypt();

    seed = 0;
    for (cp = key; *cp; cp++)
	seed = ((((*cp) ^ seed) + 170) % 192);

    seed2 = 0;
    for (cp = data; *cp; cp++)
	seed2 = ((((*cp) ^ seed2) + 21) & 0xff);
    seed3 = seed2 = ((seed2 ^ (seed ^ (RANDOM() >> 24))) & 0x3f);

    count = seed + 11;
    for (upt = (const unsigned char *) data, ups = (unsigned char *) buf, cp = key; *upt; upt++) {
	count = (((*cp) ^ count) + (seed ^ seed2)) & 0xff;
	seed2 = ((seed2 + 1) & 0x3f);
	if (!*(++cp)) cp = key;
	result = (enarr[*upt] - (32 - (CHARCOUNT - 96))) + count + seed;
	*ups = enarr[(result % CHARCOUNT) + (32 - (CHARCOUNT - 96))];
	count = (((*upt) ^ count) + seed) & 0xff;
	ups++;
    }
    *ups++ = '\0';

    ptr = (unsigned char *) linebuf;

    linelen = strlen(data);
    *ptr++ = (' ' + 1);
    *ptr++ = (' ' + seed3);
    limit--;
    limit--;

    for(cp = buf; cp < &buf[linelen]; cp++){
	limit--;
	if (limit < 0) break;
	*ptr++ = *cp;
    }
    *ptr++ = '\0';
    return linebuf;
}



const char *
strdecrypt(const char *data, const char *key)
{
    char linebuf[BUFFER_LEN];
    static char buf[BUFFER_LEN];
    const char *cp;
    unsigned char *ptr;
    unsigned char *ups;
    const unsigned char *upt;
    int linelen;
    int outlen;
    int count;
    int seed, seed2;
    int result;
    int chrcnt;

    if (!initialized_crypt)
        init_crypt();

    ptr = (unsigned char *) linebuf;

    if ((data[0] - ' ') < 1 || (data[0] - ' ') > 1) {
        return "";
    }

    linelen = strlen(data);
    chrcnt = charset_count[(data[0] - ' ') - 1];
    seed2 = (data[1] - ' ');

    strcpy(linebuf, data + 2);

    seed = 0;
    for (cp = key; *cp; cp++)
	seed = (((*cp) ^ seed) + 170) % 192;

    count = seed + 11;
    outlen = strlen(linebuf);
    upt = (const unsigned char *) linebuf;
    ups = (unsigned char *) buf;
    cp = key;
    while ((const char *)upt < &linebuf[outlen]) {
	count = (((*cp) ^ count) + (seed ^ seed2)) & 0xff;
	if (!*(++cp)) cp = key;
	seed2 = ((seed2 + 1) & 0x3f);

	result = (enarr[*upt] - (32 - (chrcnt - 96))) - (count + seed);
	while (result < 0) result += chrcnt;
	*ups = enarr[result + (32 - (chrcnt - 96))];

	count = (((*ups) ^ count) + seed) & 0xff;
	ups++; upt++;
    }
    *ups++ = '\0';

    return buf;
}

const char *
color_lookup( dbref player, const char *color, const char *defcolor, int intrecurse )
{
   const char *tempcolor;
   char buf[BUFFER_LEN];

   if( (!color) || (!*color) )
	return defcolor;
    if (player != NOTHING ) {
    if( !strcasecmp( "SUCC", color ) || !strcasecmp( "CSUCC", color ) ) {
      tempcolor = get_uncompress(GETMESG(player, "_/COLORS/SUCC"));
      if(!tempcolor)
         tempcolor = get_uncompress(GETMESG(OWNER(player), "_/COLORS/SUCC"));
      if(!tempcolor)
         tempcolor = get_uncompress(GETMESG(0, "_/COLORS/SUCC"));
      if(!tempcolor)
  	   tempcolor = CCSUCC;
      color = tempcolor;
    } else if( !strcasecmp( "FAIL", color ) || !strcasecmp( "CFAIL", color)) {
      tempcolor = get_uncompress(GETMESG(player, "_/COLORS/FAIL"));
      if(!tempcolor)
         tempcolor = get_uncompress(GETMESG(OWNER(player), "_/COLORS/FAIL"));
      if(!tempcolor)
         tempcolor = get_uncompress(GETMESG(0, "_/COLORS/FAIL"));
      if(!tempcolor)
  	   tempcolor = CCFAIL;
      color = tempcolor;
    } else if( !strcasecmp( "INFO", color ) || !strcasecmp( "CINFO", color)) {
      tempcolor = get_uncompress(GETMESG(player, "_/COLORS/INFO"));
      if(!tempcolor)
         tempcolor = get_uncompress(GETMESG(OWNER(player), "_/COLORS/INFO"));
      if(!tempcolor)
         tempcolor = get_uncompress(GETMESG(0, "_/COLORS/INFO"));
      if(!tempcolor)
  	   tempcolor = CCINFO;
      color = tempcolor;
    } else if( !strcasecmp( "NOTE", color ) || !strcasecmp( "CNOTE", color)) {
      tempcolor = get_uncompress(GETMESG(player, "_/COLORS/NOTE"));
      if(!tempcolor)
         tempcolor = get_uncompress(GETMESG(OWNER(player), "_/COLORS/NOTE"));
      if(!tempcolor)
         tempcolor = get_uncompress(GETMESG(0, "_/COLORS/NOTE"));
      if(!tempcolor)
  	   tempcolor = CCNOTE;
      color = tempcolor;
    } else if( !strcasecmp( "MOVE", color ) || !strcasecmp( "CMOVE", color)) {
      tempcolor = get_uncompress(GETMESG(player, "_/COLORS/MOVE"));
      if(!tempcolor)
         tempcolor = get_uncompress(GETMESG(OWNER(player), "_/COLORS/MOVE"));
      if(!tempcolor)
         tempcolor = get_uncompress(GETMESG(0, "_/COLORS/MOVE"));
      if(!tempcolor)
  	   tempcolor = CCMOVE;
      color = tempcolor;
    } {
      strcpy(buf, "_/COLORS/");
      strcat(buf, color);
      tempcolor = get_uncompress(GETMESG(player, buf));
      if(!tempcolor)
         tempcolor = get_uncompress(GETMESG(OWNER(player), buf));
      if(!tempcolor)
         tempcolor = get_uncompress(GETMESG(0, buf));
      if(tempcolor)
         color = tempcolor;
    }

    if (intrecurse < 5) {
       (void) intrecurse++;
       return color_lookup(player, color, defcolor, intrecurse);
    }
    }//End of player != NOTHING check. Too lazy to indent all that.
    if( !strcasecmp( "NORMAL", color )) {
	return ANSINORMAL;
    } else if( !strcasecmp( "FLASH", color ) || !strcasecmp( "BLINK", color )) {
	return ANSIFLASH;
    } else if( !strcasecmp( "INVERT", color ) || !strcasecmp( "REVERSE", color)) {
	return ANSIINVERT;
    } else if( !strcasecmp( "UNDERLINE", color )) {
	return ANSIUNDERLINE;
    } else if( !strcasecmp( "BOLD", color )) {
	return ANSIBOLD;
    } else if( !strcasecmp( "BLACK", color )) {
	return ANSIBLACK;
    } else if( !strcasecmp( "CRIMSON", color )) {
	return ANSICRIMSON;
    } else if( !strcasecmp( "FOREST", color )) {
	return ANSIFOREST;
    } else if( !strcasecmp( "BROWN", color )) {
	return ANSIBROWN;
    } else if( !strcasecmp( "NAVY", color )) {
	return ANSINAVY;
    } else if( !strcasecmp( "VIOLET", color )) {
	return ANSIVIOLET;
    } else if( !strcasecmp( "AQUA", color )) {
	return ANSIAQUA;
    } else if( !strcasecmp( "GRAY", color )) {
	return ANSIGRAY;
    } else if( !strcasecmp( "GLOOM", color )) {
	return ANSIGLOOM;
    } else if( !strcasecmp( "RED", color )) {
	return ANSIRED;
    } else if( !strcasecmp( "GREEN", color )) {
	return ANSIGREEN;
    } else if( !strcasecmp( "YELLOW", color )) {
	return ANSIYELLOW;
    } else if( !strcasecmp( "BLUE", color )) {
	return ANSIBLUE;
    } else if( !strcasecmp( "PURPLE", color )) {
	return ANSIPURPLE;
    } else if( !strcasecmp( "CYAN", color )) {
	return ANSICYAN;
    } else if( !strcasecmp( "WHITE", color )) {
	return ANSIWHITE;
    } else if( !strcasecmp( "CBLACK", color )) {
	return ANSICBLACK;
    } else if( !strcasecmp( "CRED", color )) {
	return ANSICRED;
    } else if( !strcasecmp( "CGREEN", color )) {
	return ANSICGREEN;
    } else if( !strcasecmp( "CYELLOW", color )) {
	return ANSICYELLOW;
    } else if( !strcasecmp( "CBLUE", color )) {
	return ANSICBLUE;
    } else if( !strcasecmp( "CPURPLE", color )) {
	return ANSICPURPLE;
    } else if( !strcasecmp( "CCYAN", color )) {
	return ANSICCYAN;
    } else if( !strcasecmp( "CWHITE", color )) {
	return ANSICWHITE;
    } else if( !strcasecmp( "BBLACK", color )) {
	return ANSIBBLACK;
    } else if( !strcasecmp( "BRED", color )) {
	return ANSIBRED;
    } else if( !strcasecmp( "BGREEN", color )) {
	return ANSIBGREEN;
    } else if( !strcasecmp( "BBROWN", color )) {
	return ANSIBBROWN;
    } else if( !strcasecmp( "BBLUE", color )) {
	return ANSIBBLUE;
    } else if( !strcasecmp( "BPURPLE", color )) {
	return ANSIBPURPLE;
    } else if( !strcasecmp( "BCYAN", color )) {
	return ANSIBCYAN;
    } else if( !strcasecmp( "BGRAY", color )) {
	return ANSIBGRAY;
    } else {
	return defcolor;
    }
}

char *
parse_ansi( dbref player, char *buf, const char *from, const char *defcolor )
{
    char *to, *color, cbuf[BUFFER_LEN + 2];
    const char *ansi;

    to=buf;
    while(*from) {
	if(*from == '^') {
	    from++;
	    color = cbuf;
	    while(*from && *from != '^')
		*(color++) = (*(from++));
	    *color = '\0';
	    if(*from) from++;
	    if(*cbuf) {
		if((ansi = color_lookup(player, cbuf, defcolor, 1)))
		    while(*ansi)
			*(to++) = (*(ansi++));
	    } else
		*(to++) = '^';
	} else
	    *(to++) = (*(from++));
    }
    *to='\0';
    return buf;
}

char *
tct( const char *in, char out[BUFFER_LEN] )
{
    char *p=out;
    if(!out) perror("tct: Null buffer");

    if(in && (*in))
	while ( *in && (p - out < (BUFFER_LEN-2)) )
	    if ( (*(p++) = (*(in++))) == '^')
		*(p++) = '^';
    *p = '\0';
    return out;
}

char *
unparse_ansi( char *buf, const char *from )
{
    char *to;

    buf[0]='\0';
    to=buf;
    while(*from) {
	if(*from == '^') { /* Found a ^ mark */
	    from++;
	    if(*from == '^') /* It's an escaped one, print 1 ^ */
		*(to++) = (*(from++));
	    else
                while(isgraph(*(from++))) if(*from == '^') { from++; break; }
	} else
	    *(to++) = (*(from++));
    }
    *to='\0';
    return buf;
}


char *
strip_ansi(char *buf, const char *input)
{
	const char *is;
	char *os;

	buf[0] = '\0';
	os = buf;

	is = input;

	while (*is) {
		if (*is == ESCAPE_CHAR) {
			is++;
			if (*is == '[') {
				is++;
				while (isdigit(*is) || *is == ';')
					is++;
				if (*is == 'm')
					is++;
			} else {
				is++;
			}
		} else {
			*os++ = *is++;
		}
	}
	*os = '\0';

	return buf;
}


char *
strip_bad_ansi(char *buf, const char *input)
{
	const char *is;
	char *os;
	int aflag = 0;

	buf[0] = '\0';
	os = buf;

	is = input;

	while (*is) {
		if (*is == ESCAPE_CHAR) {
			if (is[1] == '\0') {
				is++;
			} else if (is[1] != '[') {
				is++;
				is++;
			} else {
				aflag = 1;
				*os++ = *is++;	/* esc */
				*os++ = *is++;	/*  [  */
				while (isdigit(*is) || *is == ';') {
					*os++ = *is++;
				}
				if (*is != 'm') {
					*os++ = 'm';
				}
				*os++ = *is++;
			}
		} else {
			*os++ = *is++;
		}
	}
	if (aflag) {
		*os++ = '\033';
		*os++ = '[';
            *os++ = '0';
		*os++ = 'm';
	}
	*os = '\0';

	return buf;
}

char *
escape_ansi(char *buf, const char *input)
{
   const char *is;
   char *os;

   is = input;
   buf[0] = '\0';
   os = buf;
   while (*is) {
      if (*is == ESCAPE_CHAR) {
         *os++ = '\\';
         *os++ = '[';
         (void) *is++;
      } else {
         *os++ = *is++;
      }
   }
   *os = '\0';

   return buf;
}


char *
parse_mush_ansi( char *buf, char *from )
{
   char *to, color, *ansi;

   to = buf;
   while(*from) {
      if(*from == '%') {
         (void) *from++;
         color = (*(from++));
         if(color == 'c') {
            color = (*(from++));
            switch (color) {
               case 'x':
                  ansi = ANSICBLACK; break;
               case 'r':
                  ansi = ANSICRED; break;
               case 'g':
                  ansi = ANSICGREEN; break;
               case 'y':
                  ansi = ANSICYELLOW; break;
               case 'b':
                  ansi = ANSICBLUE; break;
               case 'm':
                  ansi = ANSICPURPLE; break;
               case 'c':
                  ansi = ANSICCYAN; break;
               case 'w':
                  ansi = ANSICWHITE; break;
               case 'X':
                  ansi = ANSIBBLACK; break;
               case 'R':
                  ansi = ANSIBRED; break;
               case 'G':
                  ansi = ANSIBGREEN; break;
               case 'Y':
                  ansi = ANSIBBROWN; break;
               case 'B':
                  ansi = ANSIBBLUE; break;
               case 'M':
                  ansi = ANSIBPURPLE; break;
               case 'C':
                  ansi = ANSIBCYAN; break;
               case 'W':
                  ansi = ANSIBGRAY; break;
               case 'i': case 'I':
                  ansi = ANSIINVERT; break;
               case 'f': case 'F':
                  ansi = ANSIFLASH; break;
               case 'h': case 'H':
                  ansi = ANSIBOLD; break;
               case 'u': case 'U':
                  ansi = ANSIUNDERLINE; break;
               default:
                  ansi = ANSINORMAL; break;
            }
            if(*ansi)
               while(*ansi)
                  *(to++) = (*(ansi++));
         } else {
            if(color == '%') *(to++) = '%';
/*            if(*from) *from++; */
         }
      } else *(to++) = (*(from++));
   }
   *to='\0';
   return buf;
}


char *
unparse_mush_ansi( char *buf, char *from )
{
   char *to, color, *ansi;
 
   to = buf;
   while(*from) {
      if(*from == '%') {
         (void) *from++;
         color = (*(from++));
         if(color == 'c') {
            color = (*(from++));
            switch (color) {
               default:
                  ansi = ""; break;
            }
            if(*ansi)
               while(*ansi)
                  *(to++) = (*(ansi++));
         } else if(color == '%') *(to++) = '%';
/*         if(*from) *from++; */
      } else *(to++) = (*(from++));
   }
   *to='\0';
   return buf;
}



char *
mush_tct( const char *in, char out[BUFFER_LEN] )
{
    char *p=out;

    if (!(in) || !(out)) {
       return out;
    } 

    if(in && (*in))
	while ( *in && (p - out < (BUFFER_LEN-2)) )
	    if ( (*(p++) = (*(in++))) == '%')
		*(p++) = '%';
    *p = '\0';
    return out;
}

char *
parse_tilde_ansi( char *buf, char *from )
{
    char *to, color, *ansi;
    int isbold = 0;
    to = buf;
    while(*from) {
        if ( (*(from) == '~') && (*(from+1) == '&') ) {
            from += 2;
            if (!*from) break;
        
            if ( (*(from) == '~') && (*(from+1) == '&') ) {
                /* Escape ~&~& into ~& */ 
                ansi = "~&";
                if ( ((to - buf) + strlen(ansi)) < BUFFER_LEN )
                    while (*ansi) *(to++) = (*(ansi++));
                from += 2;
                if (!*from) break;
            } else if (TildeAnsiDigit(*from)) {
                char attr;

                /* ~&### pattern */
                if ( (!from[1]) || (!from[2]) ||
                   (!TildeAnsiDigit(from[1])) || (!TildeAnsiDigit(from[2])) ) 
                    continue;

                /* Check for bold or not in first digit. */
                attr = *from;
                isbold = (attr == '1')? 1 : 0;
                from++;
                if(!*from) break; /* Just double checking */

                /* second position, foreground color */
                ansi = NULL;
                switch(*from) { 
                    case '0': ansi = isbold ? ANSIGLOOM  : ANSIBLACK  ; break;
                    case '1': ansi = isbold ? ANSIRED    : ANSICRIMSON; break;
                    case '2': ansi = isbold ? ANSIGREEN  : ANSIFOREST ; break;
                    case '3': ansi = isbold ? ANSIYELLOW : ANSIBROWN  ; break;
                    case '4': ansi = isbold ? ANSIBLUE   : ANSINAVY   ; break;
                    case '5': ansi = isbold ? ANSIPURPLE : ANSIVIOLET ; break;
                    case '6': ansi = isbold ? ANSICYAN   : ANSIAQUA   ; break;
                    case '7': ansi = isbold ? ANSIWHITE  : ANSIGRAY   ; break;
                    case '-':                                           break;
                }
                if (ansi && (((to - buf) + strlen(ansi)) < BUFFER_LEN) ) 
                    while (*ansi) 
                        *(to++) = (*(ansi++));

                /* Take care of other first position attribute possibiliies. */
                ansi = NULL;
                switch(attr) {
                    case '2': /* Need both to set invert, like old lib-ansi.muf */
                    case '8':
                        ansi = ANSIINVERT; break;            
                    case '5': /* set for blinking foreground */
                        ansi = ANSIFLASH; break;

                    case '-': /* leave alone */
                        break;
                }
                if (ansi && (((to - buf) + strlen(ansi)) < BUFFER_LEN) ) 
                    while (*ansi) 
                        *(to++) = (*(ansi++));
                from++;
                if (!*from) break;
            
                /* third and last position, background color */
                ansi = NULL;
                switch (*from) {
                    case '0': ansi = ANSIBBLACK  ;  break;
                    case '1': ansi = ANSIBRED    ;  break;
                    case '2': ansi = ANSIBGREEN  ;  break;
                    case '3': ansi = ANSIBBROWN  ;  break;
                    case '4': ansi = ANSIBBLUE   ;  break;
                    case '5': ansi = ANSIBPURPLE ;  break;
                    case '6': ansi = ANSIBCYAN   ;  break;
                    case '7': ansi = ANSIBGRAY   ;  break;
                    case '-':                       break;
                }
                if (ansi && (((to - buf) + strlen(ansi)) < BUFFER_LEN) ) 
                    while (*ansi) 
                        *(to++) = (*(ansi++));
                from++;
                if (!*from) break;

            } else {
                /* The single letter attributes */
                ansi = NULL;
                switch(*from) {
                    case 'r': /* RESET to normal colors. */
                    case 'R':
                        ansi = ANSINORMAL; break;
                    case 'c': /* this used to clear the screen, its retained */
                    case 'C': /* for parsing only, doesnt actually do it.    */
                        ansi = "CLS"; break;
                    case 'b': /* this is for BELL.. or CTRL-G */
                    case 'B':
                        ansi = "BEEP"; break;
                }
                if (ansi && (((to - buf) + strlen(ansi)) < BUFFER_LEN) ) 
                    while (*ansi) 
                        *(to++) = (*(ansi++));
                from++;
                if (!*from) break;
            } 
        } else *(to++) = (*(from++));
    }
    *to ='\0';

    if ((to - buf) + strlen(ANSINORMAL) < BUFFER_LEN)
        strcat(to, ANSINORMAL);
    return buf;
}
       
int
tilde_striplen( const char *word)
{
    const char *from;
    
    from = word;
    /* Technically, this test should never even have to be done. */
    if ( (*(from+0) == '~') && (*(from+1) == '&') &&
         (*(from+3) == '~') && (*(from+4) == '&') )
        return 4;
    else if ( (*(from+0) == '~') && (*(from+1) == '&') ) {
        from += 2;
        if (*from) {
            if (TildeAnsiDigit(*from)) { /* Eat 3 digit pattern */
                if (from[1] && from[2] &&
                    TildeAnsiDigit(from[1]) &&
                    TildeAnsiDigit(from[2]) )
                    from += 3;
            } else from++; /* Eat 1 character pattern */
        }
    }
    return from - word; /* Return the length of the ansi word. */
}

char *
unparse_tilde_ansi ( char *buf, char *from ) 
{
    /* If escaped tilde ansi, take off first pair of ~& */
    /* Otherwise remove # of characters according to tilde_striplen*/
    int count;
    char *to;
    
    to = buf;
    while (*from) {
        if ( (*(from+0) == '~') && (*(from+1) == '&') &&
             (*(from+2) == '~') && (*(from+3) == '&') ) {
            from += 2;
            *(to++) = (*(from++));
            *(to++) = (*(from++));
        } else if ( (*from == '~') && (*(from+1) == '&') ) {
            count = tilde_striplen(from);
            if ((count > 0) && (count <= strlen(from)) ) 
                from += count;
            else 
                break;
        } else *(to++) = (*(from++));
    }
    *to = '\0';
    return buf;
}
        
char *
tilde_tct( const char *in, char out[BUFFER_LEN])
{
    char *p = out;
    if (!(in) || !(out)) {
        return out;
    }
  
    if (in && (*in))
        while ( *in && (p - out < (BUFFER_LEN - 3)) )
            if ( ((*(p++) = (*(in++)) ) == '~') &&
                 ((*(p++) = (*(in++)) ) == '&') ) {
                *(p++) = '~';
                *(p++) = '&';
            }
    *p = '\0';
    return out;
}
