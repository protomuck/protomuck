/* $Header: /export/home/davin/tmp/protocvs/proto1.0/src/help.c,v 1.1.1.1 2000-09-20 18:26:11 akari Exp $
 *
 * $Log: not supported by cvs2svn $
 * Revision 1.3  1996/10/02 18:25:20  loki
 * Added support for WinNT compiling for cross-compile.
 * (WInNT doesn't do directories right.)
 *
 * Revision 1.2  1996/09/26 17:20:02  loki
 * Fixed a minor bug in info.
 *
 */

#include "copyright.h"
#include "config.h"

/* commands for giving help */

#include "db.h"
#include "props.h"
#include "params.h"
#include "tune.h"
#include "interface.h"
#include "externs.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>

/*
 * Ok, directory stuff IS a bit ugly.
 */
#if defined(DIRENT) || defined(_POSIX_VERSION)
# include <dirent.h>
# define NLENGTH(dirent) (strlen((dirent)->d_name))
#else /* not (DIRENT or _POSIX_VERSION) */
# define dirent direct
# define NLENGTH(dirent) ((dirent)->d_namlen)
# ifdef SYSNDIR
#  include <sys/ndir.h>
# endif /* SYSNDIR */
# ifdef SYSDIR
#  include <sys/dir.h>
# endif /* SYSDIR */
# ifdef NDIR
#  include <ndir.h>
# endif /* NDIR */
#endif /* not (DIRENT or _POSIX_VERSION) */

#if defined(DIRENT) || defined(_POSIX_VERSION) || defined(SYSNDIR) || defined(SYSDIR) || defined(NDIR)
# define DIR_AVALIBLE
#endif

void 
spit_file_segment(dbref player, const char *filename, const char *seg)
{
    FILE   *f;
    char    buf[BUFFER_LEN];
    char    segbuf[BUFFER_LEN];
    char   *p;
    int     startline, endline, currline;

    startline = endline = currline = 0;
    if (seg && *seg) {
	strcpy(segbuf, seg);
	for (p = segbuf; isdigit(*p); p++);
	if (*p) {
	    *p++ = '\0';
	    startline = atoi(segbuf);
	    while (*p && !isdigit(*p)) p++;
	    if (*p) endline = atoi(p);
	} else {
	    endline = startline = atoi(segbuf);
	}
    }
    if ((f = fopen(filename, "r")) == NULL) {
	sprintf(buf, CINFO "%s is missing.  Management has been notified.",
		filename);
	anotify(player, buf);
	fputs("spit_file:", stderr);
	perror(filename);
    } else {
	while (fgets(buf, sizeof buf, f)) {
	    for (p = buf; *p; p++)
		if (*p == '\n') {
		    *p = '\0';
		    break;
		}
	    currline++;
	    if ((!startline || (currline >= startline)) &&
		    (!endline || (currline <= endline))) {
		if (*buf) {
		    notify(player, buf);
		} else {
		    notify(player, "  ");
		}
	    }
	}
	fclose(f);
    }
}

void 
spit_file_segment_lines(dbref player, const char *filename, const char *seg)
{
    FILE   *f;
    char    buf[BUFFER_LEN];
    char    segbuf[BUFFER_LEN];
    char   *p;
    int     startline, endline, currline;

    startline = endline = currline = 0;
    if (seg && *seg) {
	strcpy(segbuf, seg);
	for (p = segbuf; isdigit(*p); p++);
	if (*p) {
	    *p++ = '\0';
	    startline = atoi(segbuf);
	    while (*p && !isdigit(*p)) p++;
	    if (*p) endline = atoi(p);
	} else {
	    endline = startline = atoi(segbuf);
	}
    }
    if ((f = fopen(filename, "r")) == NULL) {
	sprintf(buf, CINFO "%s is missing.  Management has been notified.",
		filename);
	anotify(player, buf);
	fputs("spit_file:", stderr);
	perror(filename);
    } else {
	while (fgets(buf, sizeof buf, f)) {
	    for (p = buf; *p; p++)
		if (*p == '\n') {
		    *p = '\0';
		    break;
		}
	    currline++;
	    if ((!startline || (currline >= startline)) &&
		    (!endline || (currline <= endline))) {
		if (*buf) {
		    notify_fmt(player, "%2d: %s", currline, buf);
		} else {
		    notify(player, "  ");
		}
	    }
	}
	fclose(f);
    }
}

void 
get_file_line( const char *filename, char *retbuf, int line )
{
    FILE   *f;
    char    buf[BUFFER_LEN];
    char   *p;
    int     currline=0;

    retbuf[0]='\0';
    if ((f = fopen(filename, "r"))) {
	while (fgets(buf, sizeof buf, f)) {
	    for (p = buf; *p; p++)
		if (*p == '\n') {
		    *p = '\0';
		    break;
		}
	    currline++;
	    if (currline == line){
		if (*buf) {
		    strcpy( retbuf, buf );
		    break;
		}
	    }
	}
	fclose(f);
    }
}

void 
spit_file(dbref player, const char *filename)
{
    spit_file_segment(player, filename, "");
}


void 
index_file(dbref player, const char *onwhat, const char *file)
{
    FILE   *f;
    char    buf[BUFFER_LEN];
    char    topic[BUFFER_LEN];
    char   *p;
    int     arglen, found;

    *topic = '\0';
    strcpy(topic, onwhat);
    if (*onwhat) {
	strcat(topic, "|");
    }

    if ((f = fopen(file, "r")) == NULL) {
	sprintf(buf, YELLOW
		"%s is missing.  Management has been notified.", file);
	anotify(player, buf);
	fprintf(stderr, "help: No file %s!\n", file);
    } else {
	arglen = strlen(topic);
	if (*topic && (arglen > 1)) {
	    do {
		do {
		    if (!(fgets(buf, sizeof buf, f))) {
			sprintf(buf, CINFO "There is no help for \"%s\"",
				onwhat);
			anotify(player, buf);
			fclose(f);
			return;
		    }
		} while (*buf != '~');
		do {
		    if (!(fgets(buf, sizeof buf, f))) {
			sprintf(buf, CINFO "There is no help for \"%s\"",
				onwhat);
			anotify(player, buf);
			fclose(f);
			return;
		    }
		} while (*buf == '~');
		p = buf;
		found = 0;
		buf[strlen(buf) - 1] = '|';
		while (*p && !found) {
		    if (strncasecmp(p, topic, arglen)) {
			while (*p && (*p != '|')) p++;
			if (*p) p++;
		    } else {
			found = 1;
		    }
		}
	    } while (!found);
	}
	while (fgets(buf, sizeof buf, f)) {
	    if (*buf == '~')
		break;
	    for (p = buf; *p; p++)
		if (*p == '\n') {
		    *p = '\0';
		    break;
		}
	    if (*buf) {
		notify(player, buf);
	    } else {
		notify(player, "  ");
	    }
	}
	fclose(f);
    }
}


int 
show_subfile(dbref player, const char *dir, const char *topic, const char *seg,
	     int partial)
{
    char   buf[256];
    struct stat st;
#ifdef DIR_AVALIBLE
    DIR         *df;
    struct dirent *dp;
#endif 

    if (!topic || !*topic) return 0;

    if ((*topic == '.') || (*topic == '~') || (index(topic, '/'))) {
	return 0;
    }
    if (strlen(topic) > 64) return 0;


#ifdef DIR_AVALIBLE
    /* TO DO: (1) exact match, or (2) partial match, but unique */
    *buf = 0;

    if ((df = (DIR *) opendir(dir)))
    {
	while ((dp = readdir(df)))
	{
	    if ((partial  && string_prefix(dp->d_name, topic)) ||
		(!partial && !string_compare(dp->d_name, topic))
		)
	    {
		sprintf(buf, "%s/%s", dir, dp->d_name);
		break;
	    }
	}
	closedir(df);
    }
    
    if (!*buf)
    {
	return 0; /* no such file or directory */
    }
#else /* !DIR_AVALIBLE */
    sprintf(buf, "%s/%s", dir, topic);
#endif /* !DIR_AVALIBLE */

    if (stat(buf, &st)) {
	return 0;
    } else {
	spit_file_segment(player, buf, seg);
	return 1;
    }
}


void 
do_man(dbref player, char *topic, char *seg)
{
    if(Guest(player)) {
	anotify_fmt(player, CFAIL "%s", tp_noguest_mesg);
	return;
    }

    if (show_subfile(player, MAN_DIR, topic, seg, FALSE)) return;
    index_file(player, topic, MAN_FILE);
}


void 
do_mpihelp(dbref player, char *topic, char *seg)
{
    if(Guest(player)) {
	anotify_fmt(player, CFAIL "%s", tp_noguest_mesg);
	return;
    }

    if (show_subfile(player, MPI_DIR, topic, seg, FALSE)) return;
    index_file(player, topic, MPI_FILE);
}


void 
do_help(dbref player, char *topic, char *seg)
{
    if (show_subfile(player, HELP_DIR, topic, seg, FALSE)) return;
    index_file(player, topic, HELP_FILE);
}

void
do_sysparm(dbref player, const char *topic)
{
    if(Guest(player))
        return;

    index_file(player, topic, SYSPARM_HELP_FILE);
}  


void 
do_news(dbref player, char *topic, char *seg)
{
    if (show_subfile(player, NEWS_DIR, topic, seg, FALSE)) return;
    index_file(player, topic, NEWS_FILE);
}


void 
add_motd_text_fmt(const char *text)
{
    char    buf[80];
    const char *p = text;
    int     count = 4;

    buf[0] = buf[1] = buf[2] = buf[3] = ' ';
    while (*p) {
	while (*p && (count < 68))
	    buf[count++] = *p++;
	while (*p && !isspace(*p) && (count < 76))
	    buf[count++] = *p++;
	buf[count] = '\0';
	log2file(MOTD_FILE, "%s", buf);
	while (*p && isspace(*p))
	    p++;
	count = 0;
    }
}


void 
do_motd(dbref player, char *text)
{
    time_t  lt;

    if (!*text || !Mage(OWNER(player))) {
	spit_file(player, MOTD_FILE);
	return;
    }
    if (!string_compare(text, "clear")) {
	unlink(MOTD_FILE);
	log2file(MOTD_FILE, "- - - - - - - - - - - - - - - - - - - "
		 "- - - - - - - - - - - - - - - - - - -");
	anotify(player, CSUCC "MOTD cleared.");
	return;
    }
    lt = time(NULL);
    log2file(MOTD_FILE, "%.16s", ctime(&lt));
    add_motd_text_fmt(text);
    log2file(MOTD_FILE, "- - - - - - - - - - - - - - - - - - - "
	     "- - - - - - - - - - - - - - - - - - -");
    anotify(player, CSUCC "MOTD updated.");
}


void 
do_info(dbref player, const char *topic, const char *seg)
{
#if defined(DIR_AVALIBLE) && !defined(WINNT)
    DIR         *df;
    struct dirent *dp;
    int         f;
    int         cols;
#endif

    if (*topic) {
	if (!show_subfile(player, INFO_DIR, topic, seg, TRUE))
	{
	    notify(player, NO_INFO_MSG);
	}
    } else {
#if defined(DIR_AVALIBLE) && !defined(WINNT)
	char buf[BUFFER_LEN];
	/* buf = (char *) calloc(1, 80); */
	(void) strcpy(buf, "    ");
	f = 0;
	cols = 0;
	if ((df = (DIR *) opendir(INFO_DIR))) 
	{
	    while ((dp = readdir(df)))
	    {
		
		if (*(dp->d_name) != '.') 
		{
		    if (!f)
			anotify(player, CINFO "Available information files are:");
		    if ((cols++ > 2) || 
			((strlen(buf) + strlen(dp->d_name)) > 64)) 
		    {
			notify(player, buf);
			(void) strcpy(buf, "    ");
			cols = 0;
		    }
		    (void) strcat(strcat(buf, dp->d_name), "  ");
		    f = strlen(buf);
		    while ((f % 20) != 4)
			buf[f++] = ' ';
		    buf[f] = '\0';
		}
	    }
	    closedir(df);
	}
	if (f)
	    notify(player, buf);
	else
	    anotify(player, CINFO "No information files are available.");
	/* free(buf); */
#else /* !DIR_AVALIBLE */
	anotify(player, CINFO "Type 'info index' for a list of files.");
#endif /* !DIR_AVALIBLE */
    }
}

