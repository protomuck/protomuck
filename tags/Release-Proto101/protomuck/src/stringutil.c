#include "copyright.h"
#include "config.h"
#include "db.h"
#include "tune.h"
#include "props.h"
#include "params.h"
#include "interface.h"

/* String utilities */

#include <stdio.h>
#include <ctype.h>
#include "externs.h"

extern const char *uppercase, *lowercase;

#define DOWNCASE(x) (lowercase[x])

#ifdef COMPRESS
extern const char *uncompress(const char *);

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

#endif

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
    for (upt = data, ups = buf, cp = key; *upt; upt++) {
	count = (((*cp) ^ count) + (seed ^ seed2)) & 0xff;
	seed2 = ((seed2 + 1) & 0x3f);
	if (!*(++cp)) cp = key;
	result = (enarr[*upt] - (32 - (CHARCOUNT - 96))) + count + seed;
	*ups = enarr[(result % CHARCOUNT) + (32 - (CHARCOUNT - 96))];
	count = (((*upt) ^ count) + seed) & 0xff;
	ups++;
    }
    *ups++ = '\0';

    ptr = linebuf;

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

    ptr = linebuf;

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
    upt = linebuf;
    ups = buf;
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
         *is++;
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
         *from++;
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
         *from++;
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

