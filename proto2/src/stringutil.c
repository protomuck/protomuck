#include "copyright.h"
#include "config.h"
#include "db.h"
#include "tune.h"
#include "props.h"
#include "params.h"
#include "interface.h"
#include "interp.h"

/* String utilities */

#include "externs.h"

#define DOWNCASE(x) (tolower(x))

/*
 * routine to be used instead of strcasecmp() in a sorting routine
 * Sorts alphabetically or numerically as appropriate.
 * This would compare "network100" as greater than "network23"
 * Will compare "network007" as less than "network07"
 * Will compare "network23a" as less than "network23b"
 * Takes same params and returns same comparitive values as strcasecmp.
 * This ignores minus signs and is case insensitive.
 */
int
alphanum_compare(const char *t1, const char *s2)
{
    int n1, n2, cnt1, cnt2;
    const char *u1, *u2;
    const char *s1 = t1;

    while (*s1 && *s2 && DOWNCASE(*s1) == DOWNCASE(*s2))
        s1++, s2++;

    /* if at a digit, compare number values instead of letters. */
    if (isdigit(*s1) && isdigit(*s2)) {
        u1 = s1;
        u2 = s2;
        n1 = n2 = 0;            /* clear number values */
        cnt1 = cnt2 = 0;

        /* back up before zeros */
        if (s1 > t1 && *s2 == '0')
            s1--, s2--;         /* curr chars are diff */
        while (s1 > t1 && *s1 == '0')
            s1--, s2--;         /* prev chars are same */
        if (!isdigit(*s1))
            s1++, s2++;

        /* calculate number values */
        while (isdigit(*s1))
            cnt1++, n1 = (n1 * 10) + (*s1++ - '0');
        while (isdigit(*s2))
            cnt2++, n2 = (n2 * 10) + (*s2++ - '0');

        /* if more digits than int can handle... */
        if (cnt1 > 8 || cnt2 > 8) {
            if (cnt1 == cnt2)
                return (*u1 - *u2); /* cmp chars if mag same */
            return (cnt1 - cnt2); /* compare magnitudes */
        }

        /* if number not same, return count difference */
        if (n1 && n2 && n1 != n2)
            return (n1 - n2);

        /* else, return difference of characters */
        return (*u1 - *u2);
    }
    /* if characters not digits, and not the same, return difference */
    return (DOWNCASE(*s1) - DOWNCASE(*s2));
}

int
string_compare(const char *s1, const char *s2)
{
    while (*s1 && *s2 && DOWNCASE(*s1) == DOWNCASE(*s2))
        s1++, s2++;

    return (DOWNCASE(*s1) - DOWNCASE(*s2));

    //return (strcasecmp(s1, s2));
}

const char *
exit_prefix(const char *string, const char *prefix)
{
    const char *p;
    const char *s = string;

    while (*s) {
        p = prefix;
        string = s;
        while (*s && *p && DOWNCASE(*s) == DOWNCASE(*p)) {
            s++;
            p++;
        }
        while (*s && isspace(*s))
            s++;
        if (!*p && (!*s || *s == EXIT_DELIMITER)) {
            return string;
        }
        while (*s && (*s != EXIT_DELIMITER))
            s++;
        if (*s)
            s++;
        while (*s && isspace(*s))
            s++;
    }
    return 0;
}

int
string_prefix(const char *string, const char *prefix)
{
    while (*string && *prefix && DOWNCASE(*string) == DOWNCASE(*prefix))
        string++, prefix++;
    return *prefix == '\0';
}

/* accepts only nonempty matches starting at the beginning of a word */
const char *
string_match(const char *src, const char *sub)
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

#define MUSH_TAB "    "

char *
mushformat_substitute(const char *str)
{
    char c;
    //char d;
    char prn[3];
    static char buf[BUFFER_LEN * 2];
    char orig[BUFFER_LEN];
    char *result;

    prn[0] = '%';
    prn[2] = '\0';

    strcpy(orig, str);
    str = orig;

    result = buf;
    while (*str) {
        if (*str == '%') {
            *result = '\0';
            prn[1] = c = *(++str);
            if (!c) {
                *(result++) = '%';
                continue;
            } else if (c == '%') {
                *(result++) = '%';
                str++;
            } else {
                c = (isupper(c)) ? c : toupper(c);
                switch (c) {
                    case 'T': /* tab */
                        strcatn(result, sizeof(buf) - (result - buf),
                                MUSH_TAB);
                        break;
                    case 'B': /* single whitespace, kinda pointless */
                        strcatn(result, sizeof(buf) - (result - buf),
                                " ");
                        break;
                    case 'R': /* carriage return */
                        if (*--result == '\r') {
                            result++;
                            /* necessary to make \r\r work */
                            strcatn(result, sizeof(buf) - (result - buf),
                                    " \r");
                        } else {
                            result++;
                            strcatn(result, sizeof(buf) - (result - buf),
                                    "\r");
                        }
                        break;
                    default:
                        *result = *str;
                        result[1] = '\0';
                        break;
                }
                result += strlen(result);
                str++;
                if ((result - buf) > (BUFFER_LEN - 2)) {
                    buf[BUFFER_LEN - 1] = '\0';
                    return buf;
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

/*
 * pronoun_substitute()
 *
 * %-type substitutions for pronouns
 *
 * %a/%A for absolute possessive (his/hers/hirs/its, His/Hers/Hirs/Its)
 * %s/%S for subjective pronouns (he/she/sie/it, He/She/Sie/It)
 * %o/%O for objective pronouns (him/her/hir/it, Him/Her/Hir/It)
 * %p/%P for possessive pronouns (his/her/hir/its, His/Her/Hir/Its)
 * %r/%R for reflexive pronouns (himself/herself/hirself/itself,
 *                                Himself/Herself/Hirself/Itself)
 * %n    for the player's name.
 */
char *
pronoun_substitute(int descr, dbref player, const char *str)
{
    char c;
    char d;
    char prn[3];
    char globprop[128];
    static char buf[BUFFER_LEN * 2];
    char orig[BUFFER_LEN];
    char *result;
    const char *sexstr;
    const char *self_sub;       /* self substitution code */
    const char *temp_sub;
    dbref mywhere = player;
    int sex;

    static const char *subjective[4] = { "", "it", "she", "he" };
    static const char *possessive[4] = { "", "its", "her", "his" };
    static const char *objective[4] = { "", "it", "her", "him" };
    static const char *reflexive[4] = { "", "itself", "herself", "himself" };
    static const char *absolute[4] = { "", "its", "hers", "his" };

    prn[0] = '%';
    prn[2] = '\0';

    strcpy(orig, str);
    str = orig;

    sex = genderof(descr, player);
    sexstr = get_property_class(player, tp_sex_prop);
    /*
       if (sexstr) {
       sexstr = do_parse_mesg(descr, player, player, sexstr, "(Lock)", sexbuf, MPI_ISLOCK);
       }
     */
    while (sexstr && isspace(*sexstr))
        sexstr++;
    if (!sexstr || !*sexstr) {
        sexstr = "_default";
    }
    result = buf;
    while (*str) {
        if (*str == '%') {
            *result = '\0';
            prn[1] = c = *(++str);
            if (!c) {
                *(result++) = '%';
                continue;
            } else if (c == '%') {
                *(result++) = '%';
                str++;
            } else {
                mywhere = player;
                d = (isupper(c)) ? c : toupper(c);

                snprintf(globprop, sizeof(globprop), "_pronouns/%.64s/%s",
                         sexstr, prn);
                if (d == 'A' || d == 'S' || d == 'O' || d == 'P' || d == 'R'
                    || d == 'N') {
                    self_sub = get_property_class(mywhere, prn);
                } else {
                    self_sub = envpropstr(&mywhere, prn);
                }
                if (!self_sub) {
                    self_sub = get_property_class(player, globprop);
                }
                if (!self_sub) {
                    self_sub = get_property_class(0, globprop);
                }
                if (!self_sub && (sex == GENDER_UNASSIGNED)) {
                    snprintf(globprop, sizeof(globprop),
                             "_pronouns/_default/%s", prn);

                    if (!(self_sub = get_property_class(player, globprop)))
                        self_sub = get_property_class(0, globprop);
                }
                if (self_sub) {
                    temp_sub = NULL;
                    if (self_sub[0] == '%' && toupper(self_sub[1]) == 'N') {
                        temp_sub = self_sub;
                        self_sub = PNAME(player);
                    }
                    if (((result - buf) + strlen(self_sub)) > (BUFFER_LEN - 2))
                        return buf;
                    strcatn(result, sizeof(buf) - (result - buf), self_sub);
                    if (isupper(prn[1]) && islower(*result))
                        *result = toupper(*result);
                    result += strlen(result);
                    str++;
                    if (temp_sub) {
                        if (((result - buf) + strlen(temp_sub + 2)) >
                            (BUFFER_LEN - 2))
                            return buf;
                        strcatn(result, sizeof(buf) - (result - buf),
                                temp_sub + 2);
                        if (isupper(temp_sub[1]) && islower(*result))
                            *result = toupper(*result);
                        result += strlen(result);
                        str++;
                    }
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
                            strcatn(result, sizeof(buf) - (result - buf),
                                    PNAME(player));
                            break;
                        case 'a':
                        case 'A':
                        case 'p':
                        case 'P':
                            strcatn(result, sizeof(buf) - (result - buf),
                                    PNAME(player));
                            strcatn(result, sizeof(buf) - (result - buf), "'s");
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
                            strcatn(result, sizeof(buf) - (result - buf),
                                    absolute[sex]);
                            break;
                        case 's':
                        case 'S':
                            strcatn(result, sizeof(buf) - (result - buf),
                                    subjective[sex]);
                            break;
                        case 'p':
                        case 'P':
                            strcatn(result, sizeof(buf) - (result - buf),
                                    possessive[sex]);
                            break;
                        case 'o':
                        case 'O':
                            strcatn(result, sizeof(buf) - (result - buf),
                                    objective[sex]);
                            break;
                        case 'r':
                        case 'R':
                            strcatn(result, sizeof(buf) - (result - buf),
                                    reflexive[sex]);
                            break;
                        case 'n':
                        case 'N':
                            strcatn(result, sizeof(buf) - (result - buf),
                                    PNAME(player));
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
 
char *
pronoun_substitute(int descr, dbref player, const char *str)
{
    char c;
    char d;
    char prn[3];
    static char buf[BUFFER_LEN * 2];
    char orig[BUFFER_LEN];
    char *result;
    const char *self_sub;   
    dbref mywhere = player;
    int sex;

    static const char *subjective[4] = { "", "it", "she", "he" };
    static const char *possessive[4] = { "", "its", "her", "his" };
    static const char *objective[4] = { "", "it", "her", "him" };
    static const char *reflexive[4] = { "", "itself", "herself", "himself" };
    static const char *absolute[4] = { "", "its", "hers", "his" };

    prn[0] = '%';
    prn[2] = '\0';

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

                if (d == 'A' || d == 'S' || d == 'O' ||
                    d == 'P' || d == 'R' || d == 'N') {
                    self_sub = get_property_class(mywhere, prn);
                } else {
                    self_sub = envpropstr(&mywhere, prn);
                }

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
*/
#ifndef MALLOC_PROFILING

char *
alloc_string(const char *string)
{
    char *s;

    /* NULL, "" -> NULL */
    if (!string || !*string)
        return 0;

    if ((s = (char *) malloc(strlen(string) + 1)) == 0) {
        fprintf(stderr, "PANIC: alloc_string() Out of Memory.\n");
        abort();
    }
    strcpy(s, string);
    return s;
}

struct shared_string *
alloc_prog_string(const char *s)
{
    struct shared_string *ss;
    int length;

    if (s == NULL || *s == '\0')
        return (NULL);

    length = strlen(s);
    if ((ss = (struct shared_string *)
         malloc(sizeof(struct shared_string) + length)) == NULL) {
        fprintf(stderr, "PANIC: alloc_prog_string() Out of Memory.\n");
        abort();
    }
    ss->links = 1;
    ss->length = length;
    bcopy(s, ss->data, ss->length + 1);
    return (ss);
}

#endif

#if !defined(MALLOC_PROFILING)
char *
string_dup(const char *s)
{
    char *p;

    p = (char *) malloc(1 + strlen(s));
    if (p)
        (void) strcpy(p, s);
    return (p);
}
#endif

char *
intostr(char *buf, int i)
{
    sprintf(buf, "%d", i);
    return (buf);
}


/*
 * Encrypt one string with another one.
 */

#define CHARCOUNT 96

static char enarr[256];
static int charset_count[] = { 96, 0 };
static int initialized_crypt = 0;

void
init_crypt(void)
{
    int i;

    for (i = 0; i <= 255; i++)
        enarr[i] = (char) i;
    for (i = 'A'; i <= 'M'; i++)
        enarr[i] = (char) enarr[i] + 13;
    for (i = 'N'; i <= 'Z'; i++)
        enarr[i] = (char) enarr[i] - 13;
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
    for (upt = (const unsigned char *) data, ups = (unsigned char *) buf, cp =
         key; *upt; upt++) {
        count = (((*cp) ^ count) + (seed ^ seed2)) & 0xff;
        seed2 = ((seed2 + 1) & 0x3f);
        if (!*(++cp))
            cp = key;
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

    for (cp = buf; cp < &buf[linelen]; cp++) {
        limit--;
        if (limit < 0)
            break;
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
    unsigned char *ups;
    const unsigned char *upt;
    int outlen;
    int count;
    int seed, seed2;
    int result;
    int chrcnt;

    if (!initialized_crypt)
        init_crypt();

    if ((data[0] - ' ') < 1 || (data[0] - ' ') > 1) {
        return "";
    }

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
    while ((const char *) upt < &linebuf[outlen]) {
        count = (((*cp) ^ count) + (seed ^ seed2)) & 0xff;
        if (!*(++cp))
            cp = key;
        seed2 = ((seed2 + 1) & 0x3f);

        result = (enarr[*upt] - (32 - (chrcnt - 96))) - (count + seed);
        while (result < 0)
            result += chrcnt;
        *ups = enarr[result + (32 - (chrcnt - 96))];

        count = (((*ups) ^ count) + seed) & 0xff;
        ups++;
        upt++;
    }
    *ups++ = '\0';

    return buf;
}

/* This function is where the custom color support in Proto comes from. 
 * See documentation for details on how to use it. Players can set
 * personal prefs, global defaults are set on #0.
 */
const char *
color_lookup(dbref player, const char *color, const char *defcolor,
             int intrecurse)
{
    const char *tempcolor;
    char buf[BUFFER_LEN];

    if ((!color) || (!*color))
        return defcolor;
    if (player != NOTHING) {
        if (!strcasecmp("SUCC", color) || !strcasecmp("CSUCC", color)) {
            tempcolor = GETMESG(player, "_/COLORS/SUCC");
            if (!tempcolor)
                tempcolor = GETMESG(OWNER(player), "_/COLORS/SUCC");
            if (!tempcolor)
                tempcolor = GETMESG(0, "_/COLORS/SUCC");
            if (!tempcolor)
                tempcolor = CCSUCC;
            color = tempcolor;
        } else if (!strcasecmp("FAIL", color) || !strcasecmp("CFAIL", color)) {
            tempcolor = GETMESG(player, "_/COLORS/FAIL");
            if (!tempcolor)
                tempcolor = GETMESG(OWNER(player), "_/COLORS/FAIL");
            if (!tempcolor)
                tempcolor = GETMESG(0, "_/COLORS/FAIL");
            if (!tempcolor)
                tempcolor = CCFAIL;
            color = tempcolor;
        } else if (!strcasecmp("INFO", color) || !strcasecmp("CINFO", color)) {
            tempcolor = GETMESG(player, "_/COLORS/INFO");
            if (!tempcolor)
                tempcolor = GETMESG(OWNER(player), "_/COLORS/INFO");
            if (!tempcolor)
                tempcolor = GETMESG(0, "_/COLORS/INFO");
            if (!tempcolor)
                tempcolor = CCINFO;
            color = tempcolor;
        } else if (!strcasecmp("NOTE", color) || !strcasecmp("CNOTE", color)) {
            tempcolor = GETMESG(player, "_/COLORS/NOTE");
            if (!tempcolor)
                tempcolor = GETMESG(OWNER(player), "_/COLORS/NOTE");
            if (!tempcolor)
                tempcolor = GETMESG(0, "_/COLORS/NOTE");
            if (!tempcolor)
                tempcolor = CCNOTE;
            color = tempcolor;
        } else if (!strcasecmp("MOVE", color) || !strcasecmp("CMOVE", color)) {
            tempcolor = GETMESG(player, "_/COLORS/MOVE");
            if (!tempcolor)
                tempcolor = GETMESG(OWNER(player), "_/COLORS/MOVE");
            if (!tempcolor)
                tempcolor = GETMESG(0, "_/COLORS/MOVE");
            if (!tempcolor)
                tempcolor = CCMOVE;
            color = tempcolor;
        }
        {
            strcpy(buf, "_/COLORS/");
            strcat(buf, color);
            tempcolor = GETMESG(player, buf);
            if (!tempcolor)
                tempcolor = GETMESG(OWNER(player), buf);
            if (!tempcolor)
                tempcolor = GETMESG(0, buf);
            if (tempcolor)
                color = tempcolor;
        }

        if (intrecurse < 5) {
            (void) intrecurse++;
            return color_lookup(player, color, defcolor, intrecurse);
        }
    }                           /* End of player != NOTHING check. Too lazy to indent all that. */
    if (!strcasecmp("NORMAL", color)) {
        return ANSINORMAL;
    } else if (!strcasecmp("BOLD", color) || !strcasecmp("BRIGHT", color)) {
        return ANSIBOLD;
    } else if (!strcasecmp("DIM", color) || !strcasecmp("HALFBRIGHT", color)) {
        return ANSIDIM;
    } else if (!strcasecmp("ITALIC", color) || !strcasecmp("ITALICS", color)) {
        return ANSIITALIC;
    } else if (!strcasecmp("UNDERLINE", color) || !strcasecmp("UNDERSCORE", color)) {
        return ANSIUNDERLINE;
    } else if (!strcasecmp("FLASH", color) || !strcasecmp("BLINK", color)) {
        return ANSIFLASH;
    } else if (!strcasecmp("FLASH2", color) || !strcasecmp("BLINK2", color)) {
        return ANSIFLASH2;
    } else if (!strcasecmp("INVERT", color) || !strcasecmp("REVERSE", color)) {
        return ANSIINVERT;
    } else if (!strcasecmp("INVISIBLE", color) || !strcasecmp("HIDDEN", color)) {
        return ANSIINVISIBLE;
    } else if (!strcasecmp("BLACK", color)) {
        return ANSIBLACK;
    } else if (!strcasecmp("CRIMSON", color)) {
        return ANSICRIMSON;
    } else if (!strcasecmp("FOREST", color)) {
        return ANSIFOREST;
    } else if (!strcasecmp("BROWN", color)) {
        return ANSIBROWN;
    } else if (!strcasecmp("NAVY", color)) {
        return ANSINAVY;
    } else if (!strcasecmp("VIOLET", color)) {
        return ANSIVIOLET;
    } else if (!strcasecmp("AQUA", color)) {
        return ANSIAQUA;
    } else if (!strcasecmp("GRAY", color)) {
        return ANSIGRAY;
    } else if (!strcasecmp("GLOOM", color)) {
        return ANSIGLOOM;
    } else if (!strcasecmp("RED", color)) {
        return ANSIRED;
    } else if (!strcasecmp("GREEN", color)) {
        return ANSIGREEN;
    } else if (!strcasecmp("YELLOW", color)) {
        return ANSIYELLOW;
    } else if (!strcasecmp("BLUE", color)) {
        return ANSIBLUE;
    } else if (!strcasecmp("PURPLE", color)) {
        return ANSIPURPLE;
    } else if (!strcasecmp("CYAN", color)) {
        return ANSICYAN;
    } else if (!strcasecmp("WHITE", color)) {
        return ANSIWHITE;
    } else if (!strcasecmp("CBLACK", color)) {
        return ANSICBLACK;
    } else if (!strcasecmp("CRED", color)) {
        return ANSICRED;
    } else if (!strcasecmp("CGREEN", color)) {
        return ANSICGREEN;
    } else if (!strcasecmp("CYELLOW", color)) {
        return ANSICYELLOW;
    } else if (!strcasecmp("CBLUE", color)) {
        return ANSICBLUE;
    } else if (!strcasecmp("CPURPLE", color)) {
        return ANSICPURPLE;
    } else if (!strcasecmp("CCYAN", color)) {
        return ANSICCYAN;
    } else if (!strcasecmp("CWHITE", color)) {
        return ANSICWHITE;
    } else if (!strcasecmp("BBLACK", color)) {
        return ANSIBBLACK;
    } else if (!strcasecmp("BRED", color)) {
        return ANSIBRED;
    } else if (!strcasecmp("BGREEN", color)) {
        return ANSIBGREEN;
    } else if (!strcasecmp("BBROWN", color)) {
        return ANSIBBROWN;
    } else if (!strcasecmp("BBLUE", color)) {
        return ANSIBBLUE;
    } else if (!strcasecmp("BPURPLE", color)) {
        return ANSIBPURPLE;
    } else if (!strcasecmp("BCYAN", color)) {
        return ANSIBCYAN;
    } else if (!strcasecmp("BGRAY", color)) {
        return ANSIBGRAY;
    } else if (!strcasecmp("HBLACK", color)) {
        return ANSIHBLACK;
    } else if (!strcasecmp("HRED", color)) {
        return ANSIHRED;
    } else if (!strcasecmp("HGREEN", color)) {
        return ANSIHGREEN;
    } else if (!strcasecmp("HYELLOW", color) || !strcasecmp("HBROWN", color)) {
        return ANSIHYELLOW;
    } else if (!strcasecmp("HBLUE", color)) {
        return ANSIHBLUE;
    } else if (!strcasecmp("HPURPLE", color)) {
        return ANSIHPURPLE;
    } else if (!strcasecmp("HCYAN", color)) {
        return ANSIHCYAN;
    } else if (!strcasecmp("HWHITE", color) || !strcasecmp("HGRAY", color)) {
        return ANSIHWHITE;
    } else {
        return defcolor;
    }
}

/* parse_ansi: Converts Neon ANSI tags into standard ANSI for
 * output. I.e, ^RED^ -> \[[1;30m
 */
char *
parse_ansi(dbref player, char *buf, const char *from, const char *defcolor)
{
    char *to, *color, cbuf[BUFFER_LEN + 2];
    const char *ansi;

    to = buf;
    while (*from) {
        if (*from == '^') {
            from++;
            color = cbuf;
            while (*from && *from != '^')
                *(color++) = (*(from++));
            *color = '\0';
            if (*from)
                from++;
            if (*cbuf) {
                if ((ansi = color_lookup(player, cbuf, defcolor, 1)))
                    while (*ansi)
                        *(to++) = (*(ansi++));
            } else
                *(to++) = '^';
        } else
            *(to++) = (*(from++));
    }
    *to = '\0';
    return buf;
}

/* tct: This escapes Neon ANSI tags. I.e, ^RED^ -> ^^RED^^ */
char *
tct(const char *in, char out[BUFFER_LEN])
{
    char *p = out;

    if (!out)
        perror("tct: Null buffer");

    if (in && (*in))
        while (*in && (p - out < (BUFFER_LEN - 2)))
            if ((*(p++) = (*(in++))) == '^')
                *(p++) = '^';
    *p = '\0';
    return out;
}

/* This function strips out Neon ANSI tags. I.e., ^RED^ 
 * would be removed.
 */
char *
unparse_ansi(char *buf, const char *from)
{
    char *to;

    buf[0] = '\0';
    to = buf;
    while (*from) {
        if (*from == '^') {
            from++;

            if (*from == '^')
                *(to++) = '^';
            else
                while (*from && *from != '^')
                    from++;

            if (*from)
                from++;
        } else {
            *(to++) = (*(from++));
        }
    }
    *to = '\0';
    return buf;
}

/* This strips standard ANSI tags. I.e., \[[1;30m would be removed. */
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

/* strip_bad_ansi removes invalid ANSI tags from the string
 * before trying to notify it out. 
 */
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
                *os++ = *is++;  /* esc */
                *os++ = *is++;  /*  [  */
                while (isdigit(*is) || *is == ';') {
                    *os++ = *is++;
                }
                if (*is != 'm') {
                    *os++ = 'm';
                }
                *os++ = *is++;
            }
        } else {
            if ((*is == '\r') || (*is == '\n')) {
                while ((*is == '\r') || (*is == '\n'))
                    is++;
                if (!(*is) && (aflag)) {
                    *os++ = '\033';
                    *os++ = '[';
                    *os++ = '0';
                    *os++ = 'm';
                    aflag = 0;
                }
                *os++ = '\r';
                *os++ = '\n';
            } else {
                *os++ = *is++;
            }
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

/* This escapes standard ANSI. I.e., \[[1;30m -> \\[[1;30m */
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

/* parse_mush_ansi converts MUSH ANSI tags into standard ANSI for
 * output. I.e, %cr -> \[[1;30m
 */
char *
parse_mush_ansi(char *buf, char *from)
{
    char *to, color, *ansi;

    to = buf;
    while (*from) {
        if (*from == '%' && (*(from+1) != '\0')) {
            (void) *from++;
            color = (*(from++));
            if (color == 'c') {
                color = (*(from++));
                switch (color) {
                    case 'x':
                        ansi = ANSICBLACK;
                        break;
                    case 'r':
                        ansi = ANSICRED;
                        break;
                    case 'g':
                        ansi = ANSICGREEN;
                        break;
                    case 'y':
                        ansi = ANSICYELLOW;
                        break;
                    case 'b':
                        ansi = ANSICBLUE;
                        break;
                    case 'm':
                        ansi = ANSICPURPLE;
                        break;
                    case 'c':
                        ansi = ANSICCYAN;
                        break;
                    case 'w':
                        ansi = ANSICWHITE;
                        break;
                    case 'X':
                        ansi = ANSIBBLACK;
                        break;
                    case 'R':
                        ansi = ANSIBRED;
                        break;
                    case 'G':
                        ansi = ANSIBGREEN;
                        break;
                    case 'Y':
                        ansi = ANSIBBROWN;
                        break;
                    case 'B':
                        ansi = ANSIBBLUE;
                        break;
                    case 'M':
                        ansi = ANSIBPURPLE;
                        break;
                    case 'C':
                        ansi = ANSIBCYAN;
                        break;
                    case 'W':
                        ansi = ANSIBGRAY;
                        break;
                    case 'i':
                    case 'I':
                        ansi = ANSIINVERT;
                        break;
                    case 'f':
                    case 'F':
                        ansi = ANSIFLASH;
                        break;
                    case 'h':
                    case 'H':
                        ansi = ANSIBOLD;
                        break;
                    case 'u':
                    case 'U':
                        ansi = ANSIUNDERLINE;
                        break;
                    default:
                        ansi = ANSINORMAL;
                        break;
                }
                if (*ansi)
                    while (*ansi)
                        *(to++) = (*(ansi++));
            } else {
                if (color == '%')
                    *(to++) = '%';
/*            if(*from) *from++; */
            }
        } else
            *(to++) = (*(from++));
    }
    *to = '\0';
    return buf;
}

/* unparse_mush_ansi: strip MUSH ANSI tags from a string.
 * I.e., %cr would be removed. 
 */
char *
unparse_mush_ansi(char *buf, char *from)
{
    char *to, color, *ansi;

    to = buf;
    while (*from) {
        if (*from == '%') {
            (void) *from++;
            color = (*(from++));
            if (color == 'c') {
                color = (*(from++));
                /* switch (color) {
                    default: */
                        ansi = "";
                        /* break;
                } */
                if (*ansi)
                    while (*ansi)
                        *(to++) = (*(ansi++));
            } else if (color == '%')
                *(to++) = '%';
/*         if(*from) *from++; */
        } else
            *(to++) = (*(from++));
    }
    *to = '\0';
    return buf;
}


/* mush_tct: Escapes MUSH ANSI tags. I.e., %cr -> %%cr */
char *
mush_tct(const char *in, char out[BUFFER_LEN])
{
    char *p = out;

    if (!(in) || !(out)) {
        return out;
    }

    if (in && (*in))
        while (*in && (p - out < (BUFFER_LEN - 2)))
            if ((*(p++) = (*(in++))) == '%')
                *(p++) = '%';
    *p = '\0';
    return out;
}

/* parse_tilde_ansi: Convert FB/Glow style tilde ANSI into
 * standard ANSI for output. I.e., ~&110 -> \[[1;30m 
 */
char *
parse_tilde_ansi(char *buf, char *from)
{
    char *to, *ansi;
    int isbold = 0;

    to = buf;
    while (*from) {
        if ((*(from) == '~') && (*(from + 1) == '&')) {
            from += 2;
            if (!*from)
                break;

            if ((*(from) == '~') && (*(from + 1) == '&')) {
                /* Escape ~&~& into ~& */
                ansi = "~&";
                if (((to - buf) + strlen(ansi)) < BUFFER_LEN)
                    while (*ansi)
                        *(to++) = (*(ansi++));
                from += 2;
                if (!*from)
                    break;
            } else if (TildeAnsiDigit(*from)) {
                char attr;

                /* ~&### pattern */
                if ((!from[1]) || (!from[2]) ||
                    (!TildeAnsiDigit(from[1])) || (!TildeAnsiDigit(from[2])))
                    continue;

                /* Check for bold or not in first digit. */
                attr = *from;
                isbold = (attr == '1') ? 1 : 0;
                from++;
                if (!*from)
                    break;      /* Just double checking */

                /* second position, foreground color */
                ansi = NULL;
                switch (*from) {
                    case '0':
                        ansi = (char *)(isbold ? ANSIGLOOM : ANSIBLACK);
                        break;
                    case '1':
                        ansi = (char *)(isbold ? ANSIRED : ANSICRIMSON);
                        break;
                    case '2':
                        ansi = (char *)(isbold ? ANSIGREEN : ANSIFOREST);
                        break;
                    case '3':
                        ansi = (char *)(isbold ? ANSIYELLOW : ANSIBROWN);
                        break;
                    case '4':
                        ansi = (char *)(isbold ? ANSIBLUE : ANSINAVY);
                        break;
                    case '5':
                        ansi = (char *)(isbold ? ANSIPURPLE : ANSIVIOLET);
                        break;
                    case '6':
                        ansi = (char *)(isbold ? ANSICYAN : ANSIAQUA);
                        break;
                    case '7':
                        ansi = (char *)(isbold ? ANSIWHITE : ANSIGRAY);
                        break;
                    case '-':
                        break;
                }
                if (ansi && (((to - buf) + strlen(ansi)) < BUFFER_LEN))
                    while (*ansi)
                        *(to++) = (*(ansi++));

                /* Take care of other first position attribute possibiliies. */
                ansi = NULL;
                switch (attr) {
                    case '2':  /* Need both to set invert, like old lib-ansi.muf */
                    case '8':
                        ansi = ANSIINVERT;
                        break;
                    case '5':  /* set for blinking foreground */
                        ansi = ANSIFLASH;
                        break;

                    case '-':  /* leave alone */
                        break;
                }
                if (ansi && (((to - buf) + strlen(ansi)) < BUFFER_LEN))
                    while (*ansi)
                        *(to++) = (*(ansi++));
                from++;
                if (!*from)
                    break;

                /* third and last position, background color */
                ansi = NULL;
                switch (*from) {
                    case '0':
                        ansi = ANSIBBLACK;
                        break;
                    case '1':
                        ansi = ANSIBRED;
                        break;
                    case '2':
                        ansi = ANSIBGREEN;
                        break;
                    case '3':
                        ansi = ANSIBBROWN;
                        break;
                    case '4':
                        ansi = ANSIBBLUE;
                        break;
                    case '5':
                        ansi = ANSIBPURPLE;
                        break;
                    case '6':
                        ansi = ANSIBCYAN;
                        break;
                    case '7':
                        ansi = ANSIBGRAY;
                        break;
                    case '-':
                        break;
                }
                if (ansi && (((to - buf) + strlen(ansi)) < BUFFER_LEN))
                    while (*ansi)
                        *(to++) = (*(ansi++));
                from++;
                if (!*from)
                    break;

            } else {
                /* The single letter attributes */
                ansi = NULL;
                switch (*from) {
                    case 'r':  /* RESET to normal colors. */
                    case 'R':
                        ansi = ANSINORMAL;
                        break;
                    case 'c':  /* this used to clear the screen, its retained */
                    case 'C':  /* for parsing only, doesnt actually do it.    */
                        ansi = "CLS";
                        break;
                    case 'b':  /* this is for BELL.. or CTRL-G */
                    case 'B':
                        ansi = "BEEP";
                        break;
                }
                if (ansi && (((to - buf) + strlen(ansi)) < BUFFER_LEN))
                    while (*ansi)
                        *(to++) = (*(ansi++));
                from++;
                if (!*from)
                    break;
            }
        } else
            *(to++) = (*(from++));
    }
    *to = '\0';

    if ((to - buf) + strlen(ANSINORMAL) < BUFFER_LEN)
        strcat(to, ANSINORMAL);
    return buf;
}

/* tilde_striplen: Used in order to determine the # of characters
 * to remove when stripping tilde ANSI from a string.
 */
size_t
tilde_striplen(const char *word)
{
    const char *from;

    from = word;
    /* Technically, this test should never even have to be done. */
    if ((*(from + 0) == '~') && (*(from + 1) == '&') &&
        (*(from + 3) == '~') && (*(from + 4) == '&'))
        return 4;
    else if ((*(from + 0) == '~') && (*(from + 1) == '&')) {
        from += 2;
        if (*from) {
            if (TildeAnsiDigit(*from)) { /* Eat 3 digit pattern */
                if (from[1] && from[2] &&
                    TildeAnsiDigit(from[1]) && TildeAnsiDigit(from[2]))
                    from += 3;
            } else
                from++;         /* Eat 1 character pattern */
        }
    }
    return from - word;         /* Return the length of the ansi word. */
}

/* unparse_tilde_ansi: This removes tilde style ANSI tags from
 * a string. I.e., ~&110 would be removed.
 */
char *
unparse_tilde_ansi(char *buf, char *from)
{
    /* If escaped tilde ansi, take off first pair of ~& */
    /* Otherwise remove # of characters according to tilde_striplen */
    size_t count;
    char *to;

    to = buf;
    while (*from) {
        if ((*(from + 0) == '~') && (*(from + 1) == '&') &&
            (*(from + 2) == '~') && (*(from + 3) == '&')) {
            from += 2;
            *(to++) = (*(from++));
            *(to++) = (*(from++));
        } else if ((*from == '~') && (*(from + 1) == '&')) {
            count = tilde_striplen(from);
            if ((count > 0) && (count <= strlen(from)))
                from += count;
            else
                break;
        } else
            *(to++) = (*(from++));
    }
    *to = '\0';
    return buf;
}

/* tilde_tct: escapes tilde style ANSI tags. 
 * i.e., ~&110 -> ~~&110
 */
char *
tilde_tct(const char *in, char out[BUFFER_LEN])
{
    char *p = out;

    if (!(in) || !(out)) {
        return out;
    }

    if (in && (*in))
        while (*in && (p - out < (BUFFER_LEN - 3)))
            if (((*(p++) = (*(in++))) == '~') && ((*(p++) = (*(in++))) == '&')) {
                *(p++) = '~';
                *(p++) = '&';
            }
    *p = '\0';
    return out;
}

int
is_valid_pose_separator(char ch)
{
    return (ch == '\'') || (ch == ' ') || (ch == ',') || (ch == '-');
}


void
prefix_message(char *Dest, const char *Src, const char *Prefix,
               int BufferLength, int SuppressIfPresent)
{
    int PrefixLength = strlen(Prefix);
    int CheckForHangingEnter = 0;

    while ((BufferLength > PrefixLength) && (*Src != '\0')) {
        if (*Src == '\r') {
            Src++;
            continue;
        }

        if (!SuppressIfPresent || strncmp(Src, Prefix, PrefixLength)
            || (!is_valid_pose_separator(Src[PrefixLength])
                && (Src[PrefixLength] != '\r')
                && (Src[PrefixLength] != '\0'))) {

            strcpy(Dest, Prefix);

            Dest += PrefixLength;
            BufferLength -= PrefixLength;

            if (BufferLength > 1) {
                if (!is_valid_pose_separator(*Src)) {
                    *Dest++ = ' ';
                    BufferLength--;
                }
            }
        }

        while ((BufferLength > 1) && (*Src != '\0')) {
            *Dest++ = *Src;
            BufferLength--;

            if (*Src++ == '\r') {
                CheckForHangingEnter = 1;
                break;
            }
        }
    }

    if (CheckForHangingEnter && (Dest[-1] == '\r'))
        Dest--;

    *Dest = '\0';
}


int
is_prop_prefix(const char *Property, const char *Prefix)
{
    while (*Property == PROPDIR_DELIMITER)
        Property++;

    while (*Prefix == PROPDIR_DELIMITER)
        Prefix++;

    while (*Prefix) {
        if (*Property == '\0')
            return 0;
        if (*Property++ != *Prefix++)
            return 0;
    }

    return (*Property == '\0') || (*Property == PROPDIR_DELIMITER);
}

int
has_suffix(const char *text, const char *suffix)
{
    int tlen = text ? strlen(text) : 0;
    int slen = suffix ? strlen(suffix) : 0;

    if (!tlen || !slen || (tlen < slen))
        return 0;

    return !string_compare(text + tlen - slen, suffix);
}

int
has_suffix_char(const char *text, char suffix)
{
    int tlen = text ? strlen(text) : 0;

    if (tlen < 1)
        return 0;
    return text[tlen - 1] == suffix;
}

/* isascii_str():                                     */
/*   Checks all characters in a string with isascii() */
/*   and returns the result.                          */

bool
isascii_str(const char *str)
{
    const char *p;

    for (p = str; *p; p++)
        if (!isascii(*p))
            return 0;

    return 1;
}

char *
strcatn(char *buf, size_t bufsize, const char *src)
{
    size_t pos = strlen(buf);
    char *dest = &buf[pos];

    while (++pos < bufsize && *src) {
        *dest++ = *src++;
    }
    if (pos <= bufsize) {
        *dest = '\0';
    }
    return buf;
}

char *
strcpyn(char *buf, size_t bufsize, const char *src)
{
    size_t pos = 0;
    char *dest = buf;

    while (++pos < bufsize && *src) {
        *dest++ = *src++;
    }
    *dest = '\0';
    return buf;
}
