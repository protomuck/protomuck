/* Copyright (c) 1994, by Cynbe ru Taren.                               */

#include "reg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#ifndef WIN32
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
#endif
#include <signal.h>

#include "config.h"
#include "db.h"
#include "props.h"
#include "externs.h"

/* Stuff you may want to customize */

#define CHECK_TRIX FALSE

/* What to do on fatal error */
#ifndef REG_FATAL
#define REG_FATAL(x) {fprintf(stderr, (x));abort();}
#endif

/* Define this when debugging trix: */
/* #define CHECK_TRIX 1 */


/* Stuff you shouldn't normally need to customize */

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#define REG_OBJ 0 
#define REG_JERK "@/jerks"
#define REG_NAME "@/badnames"
#define REG_SITE "@/sites"
#define REG_LOGIN "@/login"
#define REG_WELC "@/welcome"


/* Define our states */

#define REG_OPEN        (1)     /* Nice happy site */
#define REG_LOCKOUT     (2)     /* TJ -- site banned totally */
#define REG_GUEST       (3)     /* TJ -- site cannot connect to guests */
#define REG_USER        (4)     /* Set on users to allow site connects */
#define REG_REQUEST     (5)     /* People from this site abuse 'request' */
#define REG_BLOCKED     (6)     /* Can't even see login screen from here */

/* Default state:                                                       */
#ifndef REG_DEFAULT
#define REG_DEFAULT REG_OPEN
#endif

int loginState( int, int );
int siteState( int, int );
int siteStatekey( int, int, const char * );
int siteMatch( int site, int template );
const char *strcasestr( const char *, const char * );
int reg_trivial_regex_match( char *, char * );

#ifdef CHECK_TRIX
void reg_check_trix( void );
#endif

/* reg_make_password -- Randomly generate a password */

void
reg_make_password( char* buf ) {
    static char*consonant = "bcdfghjklmnprstvwxz";    
    static char*vowel     = "aeiouy";

    int consonants        = strlen( consonant );
    int vowels            = strlen( vowel     );

    /* We construct a random CVCCVC password: */
    *buf++ = consonant[ rand() % consonants ];
    *buf++ = vowel[     rand() % vowels     ];
    *buf++ = consonant[ rand() % consonants ];
    *buf++ = consonant[ rand() % consonants ];
    *buf++ = vowel[     rand() % vowels     ];
    *buf++ = consonant[ rand() % consonants ];
    *buf++ = '\0';
}

/* reg_site_can_request -- FALSE unless user can request a character */

int
reg_site_can_request( int site ) {
    switch (siteState( site, REG_OBJ )) {
	case REG_BLOCKED:
	case REG_LOCKOUT:
	case REG_GUEST:
	case REG_REQUEST:
		return FALSE;
	default:
		return TRUE;
	}
}

/* reg_site_is_barred -- TRUE unless site can connect */

int
reg_site_is_barred( int site ) {
    switch (siteState( site, REG_OBJ )) {
	case REG_BLOCKED:
	case REG_LOCKOUT:
	case REG_GUEST:
		return TRUE;
	default:
		return FALSE;
	}
}

/* reg_site_is_blocked -- TRUE unless site can see connect screen */

int
reg_site_is_blocked( int site ) {
    if( site == 0x7F000001 ) return FALSE; /* localhost */
    switch (siteState( site, REG_OBJ )) {
	case REG_BLOCKED:
		return TRUE;
	default:
		return FALSE;
	}
}


/* reg_user_is_barred -- TRUE unless user can connect */

int
reg_user_is_barred( int site, int user ) {
    int barred = FALSE;
    int guest = FALSE;
    int login = FALSE;
    int opensite = FALSE;

    /* Barring is default on registered sites, */
    /* and mandatory on locked out sites: */
    switch (siteState( site, REG_OBJ )) {
	case REG_BLOCKED:
	case REG_LOCKOUT:
		barred = TRUE;
		break;
	case REG_GUEST:
		guest = TRUE;
		break;
        case REG_OPEN:
                opensite = TRUE;
                break;
    default:
	break;
    }

    if (!Guest(user) && get_property_class( user, REG_LOGIN ))
	login = TRUE;
    else if (TMage(user)) return FALSE;

    if (guest && Guest(user)) return TRUE;

    if (barred) /* Check user for ability to log in from a site */
	if (!TMage(user) && !( siteState(site, user) == REG_USER ))
	    return TRUE;

    if (login) /* Check user for ability to log in from her login list */
	if (!( loginState(site, user) == REG_USER ))
	    return TRUE;

    /* Player is not barred if @/id property set */
    if (TMage(user) || get_property_class( user, "@/id" ))   return FALSE;

    return (!opensite);
}

/* reg_user_is_suspended -- return message of why if so */

const char *
reg_user_is_suspended( int user ) {
    int i;
    const char *m;

    m = get_property_class( user, "@/lockout-msg" );
#ifdef COMPRESS
    if (m) m = uncompress(m);
#endif
    if (m) return m;

    i = get_property_value( user, "@/suspend-until" );
    if (!i)               return NULL;
    if (time(NULL) > i)   return NULL;

    /* Player is suspended.  Is there a suspend msg? */
    if(get_property_class( user, "@/suspend-msg" )) {
#ifdef COMPRESS
	if (m) m = uncompress(m);
#endif
	return m;
    }

    return "Temporary suspension.";
}

/* reg_email_is_a_jerk -- TRUE iff email is listed in #0/@/jerks/ */

/* reg_trix -- TRUE iff 'trix' matchest 'str' */

/* Here we implement trivial regex matching, supporting only */
/* '*' for zero or more unspecified characters, and */
/* '|' for alternate patterns ('or'). */

#undef  MAX_TRIX
#define MAX_TRIX 4096

/* reg_wildcard_match    -- Handle '*'s in trivices */
/* We return TRUE iff all the '*'-delimited substrings match 'str': */

static int
reg_wildcard_match( char *trix, const char *str, int anchored_match ) {
    for (;;) {

	char* end;
	while (*trix=='*') {

	    /* Step past '*'.   */
	    ++trix;

	    /* Remember next substring can match anywhere in 'str': */
	    anchored_match = FALSE;
	}

	/* Stop if at end of trix: */
	if (!*trix)   return !anchored_match || !*str;


	/* Find end of next constant substring (marked by '*' or null): */
	end = strchr( trix, '*' );
	if (!end)  end = trix + strlen( trix );

	/* Copy next constant substring trix to a buf, since I  */
	/* dislike fns that gratuitously modify input:                  */
	{   char substring[ MAX_TRIX ];
	    const char*match;
	    int len = end - trix;
	    strncpy( substring, trix, len );
	    substring[ len ] = '\0';

	    /* Set 'match' to loc of 'substring' in 'str', else fail: */
	    if   (anchored_match) {
		match = !strncasecmp( str, substring, len ) ? str : NULL;
	    } else {
		match =  strcasestr(  str, substring      );
	    }
	    if (! match)  return FALSE;

	    /* Done if no more trix: */
	    if (! *end )  return !match[len];

	    /* Succeed if remaining substrings match remaining 'str': */
	    if (reg_wildcard_match( end, match+strlen(substring), TRUE )) {
		return TRUE;
	    }

	    /* Fail if match was anchored: */
	    if (anchored_match)   return FALSE;

	    /* Look for further matches of 'substring' in 'str': */
	    str = match+1;
    }   }
}

/* reg_alternation_match -- Handle '|'s in trice */
/* We return TRUE iff one of the '|'-delimited patterns matches 'str': */

int
reg_alternation_match( char *trix, const char *str ) {
    /* Empty pattern matches empty string: */
    if (!*trix)  return !*str;

    /* Over all '|'-delimited patterns: */
    while (*trix) {

	/* Find end of next wildcard trix (marked by '|' or null): */
	char* end = strchr( trix, '|' );
	if (!end)  end = trix + strlen( trix );

	/* Copy next wildcard trix to a buf, since I    */
	/* dislike fns that gratuitously modify input:  */
	{   char wildcard[ MAX_TRIX ];
	    strcpy( wildcard, trix );
	    wildcard[ end-trix ] = '\0';

	    /* Succeed if wildcard matches our 'str': */
	    if (reg_wildcard_match(  wildcard, str, TRUE ))   return TRUE;

	    /* Fail if no more wildcards to try: */
	    if (!*end)   return FALSE;

	    /* Step 'trix' to start of next wildcard: */
	    trix = end+1;
    }   }

    /* No match, fail: */
    return FALSE;
}

int
reg_trix( char *trix, const char *str ) {
    return reg_alternation_match( trix, str );
}

#ifdef CHECK_TRIX
/* reg_check_trix -- Selftest trivial regex implementation */

/* reg_check_a_trix -- Check one trixmatch against expected result */
void
reg_check_a_trix( char *trix, char *str, int expected_result ) {
    int  actual_result  = reg_trix( trix, str );
    if (!actual_result != !expected_result) {
	printf(
	    "reg_trix bug: reg_trix(\"%s\",\"%s\") d=%d\n",
	    trix,
	    str,
	    actual_result
	);
    }
}

void
reg_check_trix( void ) {
    reg_check_a_trix( "*"       , "",           1 );
    reg_check_a_trix( "*"       , "a",          1 );
    reg_check_a_trix( ""        , "",           1 );
    reg_check_a_trix( "a"       , "",           0 );

    reg_check_a_trix( ""        , "a",          0 );
    reg_check_a_trix( "a"       , "a",          1 );
    reg_check_a_trix( "abc"     , "abc",        1 );
    reg_check_a_trix( "ab*c"    , "abc",        1 );

    reg_check_a_trix( "abc*"    , "abc",        1 );
    reg_check_a_trix( "*abc"    , "abc",        1 );
    reg_check_a_trix( "*abc*"   , "abc",        1 );
    reg_check_a_trix( "**abc*"  , "abc",        1 );

    reg_check_a_trix( "a*c"     , "abc",        1 );
    reg_check_a_trix( "a**c"    , "abc",        1 );
    reg_check_a_trix( "ab*"     , "abc",        1 );
    reg_check_a_trix( "ab**"    , "abc",        1 );

    reg_check_a_trix( "*bc"     , "abc",        1 );
    reg_check_a_trix( "abc|def" , "abc",        1 );
    reg_check_a_trix( "abc|def" , "def",        1 );
    reg_check_a_trix( "abc|def" , "ghi",        0 );

    reg_check_a_trix( "a*c|d*f" , "abc",        1 );
    reg_check_a_trix( "a*c|d*f" , "def",        1 );
    reg_check_a_trix( "a*c|d*f" , "dxf",        1 );
    reg_check_a_trix( "a*c|d*f" , "axc",        1 );

    reg_check_a_trix( "a*c|d*f" , "axb",        0 );
    reg_check_a_trix( "adc|d*f" , "abc",        0 );
    reg_check_a_trix( "adc|*"   , "abc",        1 );
    reg_check_a_trix( "abc"     , "aabc",       0 );

    reg_check_a_trix( "abc"     , "abcc",       0 );
    reg_check_a_trix( "abc*"    , "abcc",       1 );
    reg_check_a_trix( "*abc"    , "aabc",       1 );
    reg_check_a_trix( "*abc*"   , "aabcc",      1 );
}

#endif

/* Check 'email' against all jerk email address patterns 'jerk' */
int
reg_email_is_a_jerk( const char *email ) {
    char buf[256], buf2[4096];
    strcpy( buf, REG_JERK );
    if (is_propdir(REG_OBJ, buf)) {
	char*jerk;
	strcat(buf, "/");
	while ((jerk = next_prop_name(REG_OBJ, buf2, buf))) {
	    strcpy(buf, jerk);
	    /* 'jerk' is now something like '@/jerks/chris@cs.univ.edu'; */
	    /* Bump 'jerk' pointer so it is instead 'chris@cs.univ.ecu': */
	    jerk = 1 + strrchr( jerk, '/' );
	    if (reg_trix( jerk, email ))        return TRUE;
	}
    }
    return FALSE;
}

/* Check 'name' against all not so nice patterns */
int
name_is_bad( const char *name ) {
    char *jerk;
    char buf[256], buf2[4096];

    strcpy( buf, REG_NAME );
    if (is_propdir(REG_OBJ, buf)) {
	strcat(buf, "/");
	while ((jerk = next_prop_name(REG_OBJ, buf2, buf))) {
	    strcpy(buf, jerk);
	    jerk = 1 + strrchr( jerk, '/' );
	    if (reg_trix( jerk, name ))         return TRUE;
	}
    }
    return FALSE;
}

/* siteMatch -- TRUE iff 'site' matches 'template' */

int
siteMatch( int site, int template ) {
    int mask = 0;

    if (!(template & 0x000000FF)) mask = 0x000000FF;
    if (!(template & 0x0000FFFF)) mask = 0x0000FFFF;
    if (!(template & 0x00FFFFFF)) mask = 0x00FFFFFF;
    if (!(template & 0xFFFFFFFF)) mask = 0xFFFFFFFF;

    return (site & ~mask) == template;
}

/* siteState -- Look up state of given site */

int
loginState( int site, int obj ) {
    return siteStatekey( site, obj, REG_LOGIN );
}

int
siteState( int site, int obj ) {
    return siteStatekey( site, obj, REG_SITE );
}

int
siteStatekey( int site, int obj, const char *startkey ) {
    char key[BUFFER_LEN], buf[BUFFER_LEN];
    PropPtr p, q;

    strcpy( key, startkey );

    for (p = first_prop(obj, key, &q, buf ); p; p = next_prop(q,p,buf)) {
	int   s0,s1,s2,s3;
	char  state[ BUFFER_LEN ];
	if (5==sscanf( buf, "%d.%d.%d.%d %s", &s0,&s1,&s2,&s3, state )) {
	    int s = (s0<<24) | (s1<<16) | (s2<<8) | s3;
	    if (siteMatch( site, s )) {
		switch (state[0]) {

		    case 'x':
		    case 'X':
			return REG_BLOCKED;
			break;
		    case 'l':
		    case 'L':
			return REG_LOCKOUT;
			break;
		    case 'g':
		    case 'G':
			return REG_GUEST;
			break;
		    case 'u':
		    case 'U':
			return REG_USER;
			break;
		    case 'r':
		    case 'R':
			return REG_REQUEST;
			break;
		    case 'o':
		    case 'O':
			return REG_OPEN;
			break;
		    default:
			return REG_DEFAULT;
    }   }   }   }

    return REG_DEFAULT;
}

const char *reg_site_welcome( int site ) {
    char key[BUFFER_LEN], buf[BUFFER_LEN];
    PropPtr p, q;
    const char *m;

    strcpy( key, REG_WELC );

    for (p = first_prop(REG_OBJ, key, &q, buf ); p; p = next_prop(q,p,buf)) {
	int   sl,s0,s1,s2,s3;
	if (5==sscanf( buf, "%d %d.%d.%d.%d", &sl,&s0,&s1,&s2,&s3 )) {
	    int s = (s0<<24) | (s1<<16) | (s2<<8) | s3;
	    if (siteMatch( site, s )) {
		strcpy( key, REG_WELC );
		strcat( key, "/" );
		strcat( key, buf );
		m = get_property_class(REG_OBJ, key);
#ifdef COMPRESS
		if( m && *m ) m = uncompress(m);
#endif
		return m;
    }   }   }

    return NULL;
}

/* strcasestr -- Caseless substring match */
/* strcpy_tolower -- strcpy that folds to lower case */

char *
strcpy_tolower( char *dst, const char *src ) {
    while ((*dst++ = tolower( *src++ )));
    return dst;
}

#define MAX_STRCASESTR_ARG (4096)
const char *
strcasestr( const char *s1, const char *s2 ) {
    static char t1[ MAX_STRCASESTR_ARG ];
    static char t2[ MAX_STRCASESTR_ARG ];
    if (strlen(s1) > MAX_STRCASESTR_ARG-1
    ||  strlen(s2) > MAX_STRCASESTR_ARG-1
    ){
	return NULL;
    }
    strcpy_tolower( t1, s1 );
    strcpy_tolower( t2, s2 );
    return strstr(  t1, t2 );
}

