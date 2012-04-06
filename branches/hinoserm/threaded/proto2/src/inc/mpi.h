/* MPI msgparse.c header file. */

#ifndef MPI_H
#define MPI_H

#define MPI_ISPUBLIC	0x00  /* never test for this one */
#define MPI_ISPRIVATE	0x01
#define MPI_ISLISTENER	0x02
#define MPI_ISLOCK	0x04
#define MPI_ISDEBUG	0x08
#define MPI_NOHOW       0x20


extern char *
mesg_parse(int descr, dbref player, dbref what, dbref perms,
            const char *inbuf, char *outbuf,
            int maxchars, int mesgtyp);


extern char *
do_parse_mesg_2(int descr, dbref player, dbref what, dbref perms, const char *inbuf,
	    const char *abuf, char *outbuf, int mesgtyp, const char *match_args, const char *match_cmdname);


extern char *
do_parse_mesg(int descr, dbref player, dbref what, const char *inbuf,
	    const char *abuf, char *outbuf, int mesgtyp, const char *match_args, const char *match_cmdname);

#endif /* MPI_H */

