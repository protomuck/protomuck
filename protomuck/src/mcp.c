/* mcp.c: MUD Client Protocol.
   Part of the FuzzBall distribution. */

#include <stdlib.h>
#include <ctype.h>
#include <malloc.h>
#include <string.h>
#include <sys/time.h>
#include "db.h"
#include "externs.h"
#include "mcp.h"
#include "mcppkg.h"
#include "tune.h"
#include "interface.h"

#define MCP_MESG_PREFIX		"#$#"
#define MCP_QUOTE_PREFIX	"#$\""
#define MCP_MESG_DELIMITER	"-"
#define MCP_ARG_DEFERRED	"*"
#define MCP_ARG_EMPTY		"\"\""
#define MCP_ARG_DELIMITER	":"
#define MCP_ARGLINE_DELIMCHAR1	'\n'
#define MCP_ARGLINE_DELIMCHAR2	'\r'
#define MCP_SEPARATOR		" "
#define MCP_INIT_PKG		"mcp"
#define MCP_DATATAG		"_data-tag"
#define MCP_INIT_MESG		"mcp "
#define MCP_NEGOTIATE_PKG	"mcp-negotiate"





/* Defined elsewhere.  Used to send text to a connection */
void SendText(McpFrame * mfr, const char *mesg);


McpPkg *mcp_PackageList = NULL;



int
strcmp_nocase(const char *s1, const char *s2)
{
	while (*s1 && tolower(*s1) == tolower(*s2))
		s1++, s2++;
	return (tolower(*s1) - tolower(*s2));
}



int
strncmp_nocase(const char *s1, const char *s2, int cnt)
{
	while (cnt && *s1 && tolower(*s1) == tolower(*s2))
		s1++, s2++, cnt--;
	if (!cnt) {
		return 0;
	} else {
		return (tolower(*s1) - tolower(*s2));
	}
}

int mcp_internal_parse(McpFrame * mfr, const char *in);



/*****************************************************************/
/***                          ************************************/
/*** MCP PACKAGE REGISTRATION ************************************/
/***                          ************************************/
/*****************************************************************/



/*****************************************************************
 *
 * void mcp_package_register(
 *              const char* pkgname,
 *              McpVer minver,
 *              McpVer maxver,
 *              McpPkg_CB callback,
 *              void* context,
 *              ContextCleanup_CB cleanup
 *          );
 *
 *
 *****************************************************************/

void
mcp_package_register(const char *pkgname, McpVer minver, McpVer maxver, McpPkg_CB callback,
					 void *context, ContextCleanup_CB cleanup)
{
	McpPkg *nu = (McpPkg *) malloc(sizeof(McpPkg));

	nu->pkgname = (char *) malloc(strlen(pkgname) + 1);
	strcpy(nu->pkgname, pkgname);
	nu->minver = minver;
	nu->maxver = maxver;
	nu->callback = callback;
	nu->context = context;
	nu->cleanup = cleanup;

	mcp_package_deregister(pkgname);
	nu->next = mcp_PackageList;
	mcp_PackageList = nu;
}






/*****************************************************************
 *
 * void mcp_package_deregister(
 *              const char* pkgname,
 *          );
 *
 *
 *****************************************************************/

void
mcp_package_deregister(const char *pkgname)
{
	McpPkg *ptr = mcp_PackageList;
	McpPkg *prev = NULL;

	while (ptr && !strcmp_nocase(ptr->pkgname, pkgname)) {
		mcp_PackageList = ptr->next;
		if (ptr->cleanup)
			ptr->cleanup(ptr->context);
		if (ptr->pkgname)
			free(ptr->pkgname);
		free(ptr);
		ptr = mcp_PackageList;
	}

	prev = mcp_PackageList;
	if (ptr)
		ptr = ptr->next;

	while (ptr) {
		if (!strcmp_nocase(pkgname, ptr->pkgname)) {
			prev->next = ptr->next;
			if (ptr->cleanup)
				ptr->cleanup(ptr->context);
			if (ptr->pkgname)
				free(ptr->pkgname);
			free(ptr);
			ptr = prev->next;
		} else {
			prev = ptr;
			ptr = ptr->next;
		}
	}
}








/*****************************************************************/
/***                    ******************************************/
/*** MCP INITIALIZATION ******************************************/
/***                    ******************************************/
/*****************************************************************/


void mcp_basic_handler(McpFrame * mfr, McpMesg * mesg, void *dummy);
void mcp_negotiate_handler(McpFrame * mfr, McpMesg * mesg, McpVer version, void *dummy);
void mcppkg_simpleedit(McpFrame * mfr, McpMesg * mesg, McpVer ver, void *context);


/*****************************************************************
 *
 * void mcp_initialize();
 *
 *   Initializes MCP globally at startup time.
 *
 *****************************************************************/

void
mcp_initialize(void)
{
	McpVer oneoh = { 1, 0 };
	McpVer twooh = { 2, 0 };

	/* McpVer twoone = {2,1}; */

	mcp_package_register(MCP_NEGOTIATE_PKG, oneoh, twooh, mcp_negotiate_handler, NULL, NULL);
	mcp_package_register("org-fuzzball-notify", oneoh, oneoh, mcppkg_simpleedit, NULL, NULL);
	mcp_package_register("org-fuzzball-simpleedit", oneoh, oneoh, mcppkg_simpleedit, NULL, NULL);
	mcp_package_register("dns-org-mud-moo-simpleedit", oneoh, oneoh, mcppkg_simpleedit, NULL, NULL);
}








/*****************************************************************/
/***                       ***************************************/
/*** MCP CONNECTION FRAMES ***************************************/
/***                       ***************************************/
/*****************************************************************/



/*****************************************************************
 *
 * void mcp_frame_init(McpFrame* mfr, connection_t con);
 *
 *   Initializes an McpFrame for a new connection.
 *   You MUST call this to initialize a new McpFrame.
 *   The caller is responsible for the allocation of the McpFrame.
 *
 *****************************************************************/

void
mcp_frame_init(McpFrame * mfr, connection_t con)
{
	McpMesg reply;

	/* McpVer twoone = {2,1}; */

	mfr->descriptor = con;
	mfr->enabled = 1;
	mfr->version.verminor = 0;
	mfr->version.vermajor = 0;
	mfr->authkey = NULL;
	mfr->packages = NULL;
	mfr->messages = NULL;


        if ( ( (((struct descriptor_data *) con)->type == CT_MUCK  ) ||
		   (((struct descriptor_data *) con)->type == CT_PUEBLO)   )
             && tp_enable_mcp) {
    	    mcp_mesg_init(&reply, MCP_INIT_PKG, "");
   	    mcp_mesg_arg_append(&reply, "version", "2.1");
	    mcp_mesg_arg_append(&reply, "to", "2.1");
	    mcp_frame_output_mesg(mfr, &reply);
	    mcp_mesg_clear(&reply);
        }
	mfr->enabled = 0;
}





/*****************************************************************
 *
 * void mcp_frame_clear(McpFrame* mfr);
 *
 *   Cleans up an McpFrame for a closing connection.
 *   You MUST call this when you are done using an McpFrame.
 *
 *****************************************************************/

void
mcp_frame_clear(McpFrame * mfr)
{
	McpPkg *tmp = mfr->packages;
	McpMesg *tmp2 = mfr->messages;

	while (tmp) {
		mfr->packages = tmp->next;
		if (tmp->pkgname)
			free(tmp->pkgname);
		free(tmp);
		tmp = mfr->packages;
	}
	while (tmp2) {
		mfr->messages = tmp2->next;
		mcp_mesg_clear(tmp2);
		free(tmp2);
		tmp2 = mfr->messages;
	}
}





/*****************************************************************
 *
 * void mcp_frame_package_add(
 *              McpFrame* mfr,
 *              const char* package,
 *              McpVer minver,
 *              McpVer maxver
 *          );
 *
 *   Attempt to register a package for this connection.
 *   Returns EMCP_SUCCESS if the package was deemed supported.
 *   Returns EMCP_NOMCP if MCP is not supported on this connection.
 *   Returns EMCP_NOPACKAGE if the package versions didn't overlap.
 *
 *****************************************************************/

int
mcp_frame_package_add(McpFrame * mfr, const char *package, McpVer minver, McpVer maxver)
{
	McpPkg *nu;
	McpPkg *ptr;
	McpVer selver;

	if (!mfr->enabled) {
		return EMCP_NOMCP;
	}

	for (ptr = mcp_PackageList; ptr; ptr = ptr->next) {
		if (!strcmp_nocase(ptr->pkgname, package)) {
			break;
		}
	}
	if (!ptr) {
		return EMCP_NOPACKAGE;
	}

	mcp_frame_package_remove(mfr, package);

	selver = mcp_version_select(ptr->minver, ptr->maxver, minver, maxver);
	nu = (McpPkg *) malloc(sizeof(McpPkg));
	nu->pkgname = (char *) malloc(strlen(ptr->pkgname) + 1);
	strcpy(nu->pkgname, ptr->pkgname);
	nu->minver = selver;
	nu->maxver = selver;
	nu->callback = ptr->callback;
	nu->context = ptr->context;
	nu->next = NULL;

	if (!mfr->packages) {
		mfr->packages = nu;
	} else {
		McpPkg *lastpkg = mfr->packages;

		while (lastpkg->next)
			lastpkg = lastpkg->next;
		lastpkg->next = nu;
	}
	return EMCP_SUCCESS;
}





/*****************************************************************
 *
 * void mcp_frame_package_remove(
 *              McpFrame* mfr,
 *              char* package,
 *              McpVer minver,
 *              McpVer maxver
 *          );
 *
 *   Cleans up an McpFrame for a closing connection.
 *   You MUST call this when you are done using an McpFrame.
 *
 *****************************************************************/

void
mcp_frame_package_remove(McpFrame * mfr, const char *package)
{
	McpPkg *tmp;
	McpPkg *prev;

	while (mfr->packages && !strcmp_nocase(mfr->packages->pkgname, package)) {
		tmp = mfr->packages;
		mfr->packages = tmp->next;
		if (tmp->pkgname)
			free(tmp->pkgname);
		free(tmp);
	}

	prev = mfr->packages;
	while (prev && prev->next && !strcmp_nocase(prev->next->pkgname, package)) {
		tmp = prev->next;
		prev->next = tmp->next;
		if (tmp->pkgname)
			free(tmp->pkgname);
		free(tmp);
	}
}





/*****************************************************************
 *
 * McpVer mcp_frame_package_supported(
 *              McpFrame* mfr,
 *              char* package
 *          );
 *
 *   Returns the supported version of the given package.
 *   Returns {0,0} if the package is not supported.
 *
 *****************************************************************/

McpVer
mcp_frame_package_supported(McpFrame * mfr, const char *package)
{
	McpPkg *ptr;
	McpVer errver = { 0, 0 };

	if (!mfr->enabled) {
		return errver;
	}

	for (ptr = mfr->packages; ptr; ptr = ptr->next) {
		if (!strcmp_nocase(ptr->pkgname, package)) {
			break;
		}
	}
	if (!ptr) {
		return errver;
	}

	return (ptr->minver);
}





/*****************************************************************
 *
 * void mcp_frame_package_docallback(
 *              McpFrame* mfr,
 *              McpMesg* msg
 *          );
 *
 *   Executes the callback function for the given message.
 *   Returns EMCP_SUCCESS if the call completed successfully.
 *   Returns EMCP_NOMCP if MCP is not supported for that connection.
 *   Returns EMCP_NOPACKAGE if the package is not supported.
 *
 *****************************************************************/

int
mcp_frame_package_docallback(McpFrame * mfr, McpMesg * msg)
{
	McpPkg *ptr = NULL;

	if (!strcmp_nocase(msg->package, MCP_INIT_PKG)) {
		mcp_basic_handler(mfr, msg, NULL);
	} else {
		if (!mfr->enabled) {
			return EMCP_NOMCP;
		}

		for (ptr = mfr->packages; ptr; ptr = ptr->next) {
			if (!strcmp_nocase(ptr->pkgname, msg->package)) {
				break;
			}
		}
		if (!ptr) {
			if (!strcmp_nocase(msg->package, MCP_NEGOTIATE_PKG)) {
				McpVer twooh = { 2, 0 };

				mcp_negotiate_handler(mfr, msg, twooh, NULL);
			} else {
				return EMCP_NOPACKAGE;
			}
		} else {
			ptr->callback(mfr, msg, ptr->maxver, ptr->context);
		}
	}
	return EMCP_SUCCESS;
}





/*****************************************************************
 *
 * int mcp_frame_process_input(
 *           McpFrame* mfr,
 *           const char* linein,
 *           char *outbuf,
 *           int bufsize
 *      );
 *
 *   Check a line of input for MCP commands.
 *   Returns 0 if the line was an out-of-band MCP message.
 *   Returns 1 if the line was in-band data.
 *     outbuf will contain the in-band data on return, if any.
 *
 *****************************************************************/

int
mcp_frame_process_input(McpFrame * mfr, const char *linein, char *outbuf, int bufsize)
{
	if (!strncmp_nocase(linein, MCP_MESG_PREFIX, 3)) {
		/* treat it as an out-of-band message, and parse it. */
		if (mfr->enabled || !strncmp_nocase(MCP_INIT_MESG, linein + 3, 4)) {
			if (!mcp_internal_parse(mfr, linein + 3)) {
				strncpy(outbuf, linein, bufsize);
				outbuf[bufsize - 1] = '\0';
				return 1;
			}
			return 0;
		} else {
			strncpy(outbuf, linein, bufsize);
			outbuf[bufsize - 1] = '\0';
			return 1;
		}
	} else if (mfr->enabled && !strncmp_nocase(linein, MCP_QUOTE_PREFIX, 3)) {
		/* It's quoted in-band data.  Strip the quoting. */
		strncpy(outbuf, linein + 3, bufsize);
	} else {
		/* It's in-band data.  Return it raw. */
		strncpy(outbuf, linein, bufsize);
	}
	outbuf[bufsize - 1] = '\0';
	return 1;
}





/*****************************************************************
 *
 * void mcp_frame_output_inband(
 *             McpFrame* mfr,
 *             const char* lineout
 *         );
 *
 *   Sends a string to the given connection, using MCP escaping
 *     if needed and supported.
 *
 *****************************************************************/

void
mcp_frame_output_inband(McpFrame * mfr, const char *lineout)
{
	if (!mfr->enabled ||
		(strncmp(lineout, MCP_MESG_PREFIX, 3) && strncmp(lineout, MCP_QUOTE_PREFIX, 3))
			) {
		SendText(mfr, lineout);
	} else {
		SendText(mfr, MCP_QUOTE_PREFIX);
		SendText(mfr, lineout);
	}
	/* SendText(mfr, "\r\n"); */
}






/*****************************************************************
 *
 * int mcp_frame_output_mesg(
 *             McpFrame* mfr,
 *             McpMesg* msg
 *         );
 *
 *   Sends an MCP message to the given connection.
 *   Returns EMCP_SUCCESS if successful.
 *   Returns EMCP_NOMCP if MCP isn't supported on this connection.
 *   Returns EMCP_NOPACKAGE if this connection doesn't support the needed package.
 *
 *****************************************************************/

int
mcp_frame_output_mesg(McpFrame * mfr, McpMesg * msg)
{
	char outbuf[BUFFER_LEN * 2];
	char mesgname[128];
	char datatag[32];
	McpArg *anarg = NULL;
	int mlineflag = 0;
	char *p;
	char *out;

	if (!mfr->enabled && strcmp_nocase(msg->package, MCP_INIT_PKG)) {
		return EMCP_NOMCP;
	}

	/* Create the message name from the package and message subnames */
	strncpy(mesgname, msg->package, sizeof(mesgname));
	mesgname[sizeof(mesgname)] = '\0';
	if (msg->mesgname && *msg->mesgname) {
		strcat(mesgname, MCP_MESG_DELIMITER);
		strncat(mesgname, msg->mesgname, sizeof(mesgname) - strlen(mesgname));
		mesgname[sizeof(mesgname)] = '\0';
	}

	strcpy(outbuf, MCP_MESG_PREFIX);
	strcat(outbuf, mesgname);
	if (strcmp_nocase(mesgname, MCP_INIT_PKG)) {
		McpVer nullver = { 0, 0 };

		strcat(outbuf, " ");
		strcat(outbuf, mfr->authkey);
		if (strcmp_nocase(msg->package, MCP_NEGOTIATE_PKG)) {
			McpVer ver = mcp_frame_package_supported(mfr, msg->package);

			if (!mcp_version_compare(ver, nullver)) {
				return EMCP_NOPACKAGE;
			}
		}
	}

	/* If the argument lines contain newlines, split them into separate lines. */
	for (anarg = msg->args; anarg; anarg = anarg->next) {
		if (anarg->value) {
			McpArgPart *ap = anarg->value;

			while (ap) {
				p = ap->value;
				while (*p) {
					if (*p == MCP_ARGLINE_DELIMCHAR1 || *p == MCP_ARGLINE_DELIMCHAR2) {
						McpArgPart *nu = (McpArgPart *) malloc(sizeof(McpArgPart));

						nu->next = ap->next;
						ap->next = nu;
						*p++ = '\0';
						nu->value = (char *) malloc(strlen(p) + 1);
						strcpy(nu->value, p);
						ap->value = (char *) realloc(ap->value, strlen(ap->value) + 1);
						ap = nu;
						p = nu->value;
					} else {
						p++;
					}
				}
				ap = ap->next;
			}
		}
	}

	/* Build the initial message string */
	out = outbuf;
	for (anarg = msg->args; anarg; anarg = anarg->next) {
		out += strlen(out);
		strcat(out, MCP_SEPARATOR);
		if (!anarg->value) {
			anarg->was_shown = 1;
			strcat(out, anarg->name);
			out += strlen(out);
			strcat(out, MCP_ARG_DELIMITER);
			strcat(out, MCP_SEPARATOR);
			strcat(out, MCP_ARG_EMPTY);
		} else {
			int totlen = strlen(anarg->value->value) + strlen(anarg->name) + 5;

			if (anarg->value->next || totlen > ((BUFFER_LEN - (out - outbuf)) / 2)) {
				/* Value is multi-line or too long.  Send on separate line(s). */
				mlineflag = 1;
				anarg->was_shown = 0;
				strcat(out, anarg->name);
				strcat(out, MCP_ARG_DEFERRED);
				strcat(out, MCP_ARG_DELIMITER);
				strcat(out, MCP_SEPARATOR);
				strcat(out, MCP_ARG_EMPTY);
			} else {
				anarg->was_shown = 1;
				strcat(out, anarg->name);
				strcat(out, MCP_ARG_DELIMITER);
				strcat(out, MCP_SEPARATOR);
				out += strlen(out);
				*out++ = '"';
				p = anarg->value->value;
				while (*p) {
					if (*p == '"' || *p == '\\') {
						*out++ = '\\';
					}
					*out++ = *p++;
				}
				*out++ = '"';
				*out = '\0';
			}
		}
	}

	/* If the message is multi-line, make sure it has a _data-tag field. */
	if (mlineflag) {
		strcat(out, MCP_DATATAG);
		strcat(out, MCP_ARG_DELIMITER);
		strcat(out, MCP_SEPARATOR);
		out += strlen(out);
		sprintf(datatag, "%.8lX", random() ^ random());
		strcat(out, datatag);
	}

	/* Send the initial line. */
	SendText(mfr, outbuf);
	SendText(mfr, "\r\n");

	if (mlineflag) {
		/* Start sending arguments whose values weren't already sent. */
		/* This is usually just multi-line argument values. */
		for (anarg = msg->args; anarg; anarg = anarg->next) {
			if (!anarg->was_shown) {
				McpArgPart *ap = anarg->value;

				while (ap) {
					*outbuf = '\0';
					strcpy(outbuf, MCP_MESG_PREFIX);
					strcat(outbuf, "* ");
					strcat(outbuf, datatag);
					strcat(outbuf, MCP_SEPARATOR);
					strcat(outbuf, anarg->name);
					strcat(outbuf, MCP_ARG_DELIMITER);
					strcat(outbuf, MCP_SEPARATOR);
					strcat(outbuf, ap->value);
					strcat(outbuf, "\r\n");
					SendText(mfr, outbuf);
					ap = ap->next;
				}
			}
		}

		/* Let the other side know we're done sending multi-line arg vals. */
		strcpy(outbuf, MCP_MESG_PREFIX);
		strcat(outbuf, ": ");
		strcat(outbuf, datatag);
		strcat(outbuf, "\r\n");
		SendText(mfr, outbuf);
	}
	return EMCP_SUCCESS;
}









/*****************************************************************/
/***                   *******************************************/
/***  MESSAGE METHODS  *******************************************/
/***                   *******************************************/
/*****************************************************************/



/*****************************************************************
 *
 * void mcp_mesg_init(
 *         McpMesg*    msg,
 *         const char* package,
 *         const char* mesgname
 *     );
 *
 *   Initializes an MCP message.
 *   You MUST initialize a message before using it.
 *   You MUST also mcp_mesg_clear() a mesg once you are done using it.
 *
 *****************************************************************/

void
mcp_mesg_init(McpMesg * msg, const char *package, const char *mesgname)
{
	msg->package = (char *) malloc(strlen(package) + 1);
	strcpy(msg->package, package);
	msg->mesgname = (char *) malloc(strlen(mesgname) + 1);
	strcpy(msg->mesgname, mesgname);
	msg->datatag = NULL;
	msg->args = NULL;
	msg->incomplete = 0;
	msg->bytes = 0;
	msg->next = NULL;
}




/*****************************************************************
 *
 * void mcp_mesg_clear(
 *              McpMesg* msg
 *          );
 *
 *   Clear the given MCP message.
 *   You MUST clear every message after you are done using it, to
 *     free the memory used by the message.
 *
 *****************************************************************/

void
mcp_mesg_clear(McpMesg * msg)
{
	if (msg->package)
		free(msg->package);
	if (msg->mesgname)
		free(msg->mesgname);
	if (msg->datatag)
		free(msg->datatag);

	while (msg->args) {
		McpArg *tmp = msg->args;

		msg->args = tmp->next;
		if (tmp->name)
			free(tmp->name);
		while (tmp->value) {
			McpArgPart *ptr2 = tmp->value;

			tmp->value = tmp->value->next;
			if (ptr2->value)
				free(ptr2->value);
			free(ptr2);
		}
		free(tmp);
	}
	msg->bytes = 0;
}









/*****************************************************************/
/***                  ********************************************/
/*** ARGUMENT METHODS ********************************************/
/***                  ********************************************/
/*****************************************************************/



/*****************************************************************
 *
 * int mcp_mesg_arg_linecount(
 *         McpMesg* msg,
 *         const char* name
 *     );
 *
 *   Returns the count of the number of lines in the given arg of
 *   the given message.
 *
 *****************************************************************/

int
mcp_mesg_arg_linecount(McpMesg * msg, const char *name)
{
	McpArg *ptr = msg->args;
	int cnt = 0;

	while (ptr && strcmp_nocase(ptr->name, name)) {
		ptr = ptr->next;
	}
	if (ptr) {
		McpArgPart *ptr2 = ptr->value;

		while (ptr2) {
			ptr2 = ptr2->next;
			cnt++;
		}
	}
	return cnt;
}




/*****************************************************************
 *
 * char* mcp_mesg_arg_getline(
 *         McpMesg* msg,
 *         const char* argname
 *         int linenum;
 *     );
 *
 *   Gets the value of a named argument in the given message.
 *
 *****************************************************************/

char *
mcp_mesg_arg_getline(McpMesg * msg, const char *argname, int linenum)
{
	McpArg *ptr = msg->args;

	while (ptr && strcmp_nocase(ptr->name, argname)) {
		ptr = ptr->next;
	}
	if (ptr) {
		McpArgPart *ptr2 = ptr->value;

		while (linenum > 0 && ptr2) {
			ptr2 = ptr2->next;
			linenum--;
		}
		if (ptr2) {
			return (ptr2->value);
		}
		return NULL;
	}
	return NULL;
}




/*****************************************************************
 *
 * int mcp_mesg_arg_append(
 *         McpMesg* msg,
 *         const char* argname,
 *         const char* argval
 *     );
 *
 *   Appends to the list value of the named arg in the given mesg.
 *   If that named argument doesn't exist yet, it will be created.
 *   This is used to construct arguments that have lists as values.
 *   Returns the success state of the call.  EMCP_SUCCESS if the
 *   call was successful.  EMCP_ARGCOUNT if this would make too
 *   many arguments in the message.  EMCP_ARGLENGTH is this would
 *   cause an argument to exceed the max allowed number of lines.
 *
 *****************************************************************/

int
mcp_mesg_arg_append(McpMesg * msg, const char *argname, const char *argval)
{
	McpArg *ptr = msg->args;
	int namelen = strlen(argname);
	int vallen = argval? strlen(argval) : 0;

	if (namelen > MAX_MCP_ARGNAME_LEN) {
		return EMCP_ARGNAMELEN;
	}
	if (vallen + msg->bytes > MAX_MCP_MESG_SIZE) {
		return EMCP_MESGSIZE;
	}
	while (ptr && strcmp_nocase(ptr->name, argname)) {
		ptr = ptr->next;
	}
	if (!ptr) {
		if (namelen + vallen + msg->bytes > MAX_MCP_MESG_SIZE) {
			return EMCP_MESGSIZE;
		}
		ptr = (McpArg *) malloc(sizeof(McpArg));
		ptr->name = (char *) malloc(namelen + 1);
		strcpy(ptr->name, argname);
		ptr->value = NULL;
		ptr->last = NULL;
		ptr->next = NULL;
		if (!msg->args) {
			msg->args = ptr;
		} else {
			int limit = MAX_MCP_MESG_ARGS;
			McpArg *lastarg = msg->args;

			while (lastarg->next) {
				if (!limit-->0) {
					free(ptr->name);
					free(ptr);
					return EMCP_ARGCOUNT;
				}
				lastarg = lastarg->next;
			}
			lastarg->next = ptr;
		}
		msg->bytes += sizeof(McpArg) + namelen + 1;
	}

	if (argval) {
		McpArgPart *nu = (McpArgPart *) malloc(sizeof(McpArgPart));

		nu->value = (char *) malloc(vallen + 1);
		strcpy(nu->value, argval);
		nu->next = NULL;

		if (!ptr->last) {
			ptr->value = ptr->last = nu;
		} else {
			ptr->last->next = nu;
			ptr->last = nu;
		}
		msg->bytes += sizeof(McpArgPart) + vallen + 1;
	}
	ptr->was_shown = 0;
	return EMCP_SUCCESS;
}




/*****************************************************************
 *
 * void mcp_mesg_arg_remove(
 *         McpMesg* msg,
 *         const char* argname
 *     );
 *
 *   Removes the named argument from the given message.
 *
 *****************************************************************/

void
mcp_mesg_arg_remove(McpMesg * msg, const char *argname)
{
	McpArg *ptr = msg->args;
	McpArg *prev = NULL;

	while (ptr && !strcmp_nocase(ptr->name, argname)) {
		msg->args = ptr->next;
		msg->bytes -= sizeof(McpArg);
		if (ptr->name) {
			free(ptr->name);
			msg->bytes -= strlen(ptr->name) + 1;
		}
		while (ptr->value) {
			McpArgPart *ptr2 = ptr->value;

			ptr->value = ptr->value->next;
			msg->bytes -= sizeof(McpArgPart);
			if (ptr2->value) {
				msg->bytes -= strlen(ptr2->value) + 1;
				free(ptr2->value);
			}
			free(ptr2);
		}
		free(ptr);
		ptr = msg->args;
	}

	prev = msg->args;
	if (ptr)
		ptr = ptr->next;

	while (ptr) {
		if (!strcmp_nocase(argname, ptr->name)) {
			prev->next = ptr->next;
			msg->bytes -= sizeof(McpArg);
			if (ptr->name) {
				free(ptr->name);
				msg->bytes -= strlen(ptr->name) + 1;
			}
			while (ptr->value) {
				McpArgPart *ptr2 = ptr->value;

				ptr->value = ptr->value->next;
				msg->bytes -= sizeof(McpArgPart);
				if (ptr2->value) {
					msg->bytes -= strlen(ptr2->value) + 1;
					free(ptr2->value);
				}
				free(ptr2);
			}
			free(ptr);
			ptr = prev->next;
		} else {
			prev = ptr;
			ptr = ptr->next;
		}
	}
}




/*****************************************************************/
/***                 *********************************************/
/*** VERSION METHODS *********************************************/
/***                 *********************************************/
/*****************************************************************/




/*****************************************************************
 *
 * int mcp_version_compare(McpVer v1, McpVer v2);
 *
 *   Compares two McpVer structs.
 *   Results are similar to strcmp():
 *     Returns negative if v1 <  v2
 *     Returns 0 (zero) if v1 == v2
 *     Returns positive if v1 >  v2
 *
 *****************************************************************/

int
mcp_version_compare(McpVer v1, McpVer v2)
{
	if (v1.vermajor != v2.vermajor) {
		return (v1.vermajor - v2.vermajor);
	}
	return (v1.verminor - v2.verminor);
}




/*****************************************************************
 *
 * McpVer mcp_version_select(
 *                McpVer min1,
 *                McpVer max1,
 *                McpVer min2,
 *                McpVer max2
 *            );
 *
 *   Given the min and max package versions supported by a client
 *     and server, this will return the highest version that is
 *     supported by both.
 *   Returns a McpVer of {0, 0} if there is no version overlap.
 *
 *****************************************************************/

McpVer
mcp_version_select(McpVer min1, McpVer max1, McpVer min2, McpVer max2)
{
	McpVer result = { 0, 0 };

	if (mcp_version_compare(min1, max1) > 0) {
		return result;
	}
	if (mcp_version_compare(min2, max2) > 0) {
		return result;
	}
	if (mcp_version_compare(min1, max2) > 0) {
		return result;
	}
	if (mcp_version_compare(min2, max1) > 0) {
		return result;
	}
	if (mcp_version_compare(max1, max2) > 0) {
		return max2;
	} else {
		return max1;
	}
}






/*****************************************************************/
/***                       ***************************************/
/***  MCP PACKAGE HANDLER  ***************************************/
/***                       ***************************************/
/*****************************************************************/

void
mcp_basic_handler(McpFrame * mfr, McpMesg * mesg, void *dummy)
{
	McpVer myminver = { 2, 1 };
	McpVer mymaxver = { 2, 1 };
	McpVer minver = { 0, 0 };
	McpVer maxver = { 0, 0 };
	McpVer nullver = { 0, 0 };
	const char *ptr;
	const char *auth;

	if (!*mesg->mesgname) {
		auth = mcp_mesg_arg_getline(mesg, "authentication-key", 0);
		if (auth) {
			mfr->authkey = (char *) malloc(strlen(auth) + 1);
			strcpy(mfr->authkey, auth);
		} else {
			McpMesg reply;
			char authval[128];

			mcp_mesg_init(&reply, MCP_INIT_PKG, "");
			mcp_mesg_arg_append(&reply, "version", "2.1");
			mcp_mesg_arg_append(&reply, "to", "2.1");
			sprintf(authval, "%.8lX", random() ^ random());
			mcp_mesg_arg_append(&reply, "authentication-key", authval);
			mfr->authkey = (char *) malloc(strlen(authval) + 1);
			strcpy(mfr->authkey, authval);
			mcp_frame_output_mesg(mfr, &reply);
			mcp_mesg_clear(&reply);
		}

		ptr = mcp_mesg_arg_getline(mesg, "version", 0);
		if (!ptr)
			return;
		while (isdigit(*ptr))
			minver.vermajor = (minver.vermajor * 10) + (*ptr++ - '0');
		if (*ptr++ != '.')
			return;
		while (isdigit(*ptr))
			minver.verminor = (minver.verminor * 10) + (*ptr++ - '0');

		ptr = mcp_mesg_arg_getline(mesg, "to", 0);
		if (!ptr) {
			maxver = minver;
		} else {
			while (isdigit(*ptr))
				maxver.vermajor = (maxver.vermajor * 10) + (*ptr++ - '0');
			if (*ptr++ != '.')
				return;
			while (isdigit(*ptr))
				maxver.verminor = (maxver.verminor * 10) + (*ptr++ - '0');
		}

		mfr->version = mcp_version_select(myminver, mymaxver, minver, maxver);
		if (mcp_version_compare(mfr->version, nullver)) {
			McpMesg cando;
			char verbuf[32];
			McpPkg *p = mcp_PackageList;

			mfr->enabled = 1;
			while (p) {
				if (strcmp_nocase(p->pkgname, MCP_INIT_PKG)) {
					mcp_mesg_init(&cando, MCP_NEGOTIATE_PKG, "can");
					mcp_mesg_arg_append(&cando, "package", p->pkgname);
					sprintf(verbuf, "%d.%d", p->minver.vermajor, p->minver.verminor);
					mcp_mesg_arg_append(&cando, "min-version", verbuf);
					sprintf(verbuf, "%d.%d", p->maxver.vermajor, p->maxver.verminor);
					mcp_mesg_arg_append(&cando, "max-version", verbuf);
					mcp_frame_output_mesg(mfr, &cando);
					mcp_mesg_clear(&cando);
				}
				p = p->next;
			}
			mcp_mesg_init(&cando, MCP_NEGOTIATE_PKG, "end");
			mcp_frame_output_mesg(mfr, &cando);
			mcp_mesg_clear(&cando);
		}
	}
}





/*****************************************************************/
/***                                 *****************************/
/***  MCP-NEGOTIATE PACKAGE HANDLER  *****************************/
/***                                 *****************************/
/*****************************************************************/

void
mcp_negotiate_handler(McpFrame * mfr, McpMesg * mesg, McpVer version, void *dummy)
{
	McpVer minver = { 0, 0 };
	McpVer maxver = { 0, 0 };
	const char *ptr;
	const char *pkg;

	if (!strcmp(mesg->mesgname, "can")) {
		pkg = mcp_mesg_arg_getline(mesg, "package", 0);
		if (!pkg)
			return;
		ptr = mcp_mesg_arg_getline(mesg, "min-version", 0);
		if (!ptr)
			return;
		while (isdigit(*ptr))
			minver.vermajor = (minver.vermajor * 10) + (*ptr++ - '0');
		if (*ptr++ != '.')
			return;
		while (isdigit(*ptr))
			minver.verminor = (minver.verminor * 10) + (*ptr++ - '0');

		ptr = mcp_mesg_arg_getline(mesg, "max-version", 0);
		if (!ptr) {
			maxver = minver;
		} else {
			while (isdigit(*ptr))
				maxver.vermajor = (maxver.vermajor * 10) + (*ptr++ - '0');
			if (*ptr++ != '.')
				return;
			while (isdigit(*ptr))
				maxver.verminor = (maxver.verminor * 10) + (*ptr++ - '0');
		}

		mcp_frame_package_add(mfr, pkg, minver, maxver);
	} else if (!strcmp(mesg->mesgname, "end")) {
		/* nothing to do for end of package negotiations. */
	}
}





/*****************************************************************/
/****************                *********************************/
/**************** INTERNAL STUFF *********************************/
/****************                *********************************/
/*****************************************************************/

int
mcp_intern_is_ident(const char **in, char *buf, int buflen)
{
	int origbuflen = buflen;

	if (!isalpha(**in) && **in != '_')
		return 0;
	while (isalpha(**in) || **in == '_' || isdigit(**in) || **in == '-') {
		if (--buflen > 0) {
			*buf++ = **in;
		}
		(*in)++;
	}
	if (origbuflen > 0)
		*buf = '\0';
	return 1;
}


int
mcp_intern_is_simplechar(char in)
{
	if (in == '*' || in == ':' || in == '\\' || in == '"')
		return 0;
	if (!isprint(in) || in == ' ')
		return 0;
	return 1;
}


int
mcp_intern_is_unquoted(const char **in, char *buf, int buflen)
{
	int origbuflen = buflen;

	if (!mcp_intern_is_simplechar(**in))
		return 0;
	while (mcp_intern_is_simplechar(**in)) {
		if (--buflen > 0) {
			*buf++ = **in;
		}
		(*in)++;
	}
	if (origbuflen > 0)
		*buf = '\0';
	return 1;
}


int
mcp_intern_is_quoted(const char **in, char *buf, int buflen)
{
	int origbuflen = buflen;
	const char *old = *in;

	if (**in != '"')
		return 0;
	(*in)++;
	while (**in) {
		if (**in == '\\') {
			(*in)++;
			if (**in && --buflen > 0) {
				*buf++ = **in;
			}
		} else if (**in == '"') {
			break;
		} else {
			if (--buflen > 0) {
				*buf++ = **in;
			}
		}
		(*in)++;
	}
	if (**in != '"') {
		*in = old;
		return 0;
	}
	(*in)++;
	if (origbuflen > 0) {
		*buf = '\0';
	}
	return 1;
}


int
mcp_intern_is_keyval(McpMesg * msg, const char **in)
{
	char keyname[128];
	char value[BUFFER_LEN];
	const char *old = *in;
	int deferred = 0;

	if (!isspace(**in))
		return 0;
	while (isspace(**in))
		(*in)++;
	if (!mcp_intern_is_ident(in, keyname, sizeof(keyname))) {
		*in = old;
		return 0;
	}
	if (**in == '*') {
		msg->incomplete = 1;
		deferred = 1;
		(*in)++;
	}
	if (**in != ':') {
		*in = old;
		return 0;
	}
	(*in)++;
	if (!isspace(**in)) {
		*in = old;
		return 0;
	}
	while (isspace(**in))
		(*in)++;
	if (!mcp_intern_is_unquoted(in, value, sizeof(value)) &&
		!mcp_intern_is_quoted(in, value, sizeof(value))
			) {
		*in = old;
		return 0;
	}

	if (deferred) {
		mcp_mesg_arg_append(msg, keyname, NULL);
	} else {
		mcp_mesg_arg_append(msg, keyname, value);
	}
	return 1;
}



int
mcp_intern_is_mesg_start(McpFrame * mfr, const char *in)
{
	char mesgname[128];
	char authkey[128];
	char *subname = NULL;
	McpMesg *newmsg = NULL;
	McpPkg *pkg = NULL;
	int longlen = 0;

	if (!mcp_intern_is_ident(&in, mesgname, sizeof(mesgname)))
		return 0;
	if (strcmp_nocase(mesgname, MCP_INIT_PKG)) {
		if (!isspace(*in))
			return 0;
		while (isspace(*in))
			in++;
		if (!mcp_intern_is_unquoted(&in, authkey, sizeof(authkey)))
			return 0;
		if (strcmp(authkey, mfr->authkey))
			return 0;
	}

	if (strncmp_nocase(mesgname, MCP_INIT_PKG, 3)) {
		for (pkg = mfr->packages; pkg; pkg = pkg->next) {
			int pkgnamelen = strlen(pkg->pkgname);

			if (!strncmp_nocase(pkg->pkgname, mesgname, pkgnamelen)) {
				if (mesgname[pkgnamelen] == '\0' || mesgname[pkgnamelen] == '-') {
					if (pkgnamelen > longlen) {
						longlen = pkgnamelen;
					}
				}
			}
		}
	}
	if (!longlen) {
		int neglen = strlen(MCP_NEGOTIATE_PKG);

		if (!strncmp_nocase(mesgname, MCP_NEGOTIATE_PKG, neglen)) {
			longlen = neglen;
		} else if (!strcmp_nocase(mesgname, MCP_INIT_PKG)) {
			longlen = strlen(mesgname);
		} else {
			return 0;
		}
	}
	subname = mesgname + longlen;
	if (*subname) {
		*subname++ = '\0';
	}

	newmsg = (McpMesg *) malloc(sizeof(McpMesg));
	mcp_mesg_init(newmsg, mesgname, subname);
	while (*in) {
		if (!mcp_intern_is_keyval(newmsg, &in)) {
			mcp_mesg_clear(newmsg);
			free(newmsg);
			return 0;
		}
	}

	/* Okay, we've recieved a valid message. */
	if (newmsg->incomplete) {
		/* It's incomplete.  Remember it to finish later. */
		const char *msgdt = mcp_mesg_arg_getline(newmsg, MCP_DATATAG, 0);

		newmsg->datatag = (char *) malloc(strlen(msgdt) + 1);
		strcpy(newmsg->datatag, msgdt);
		mcp_mesg_arg_remove(newmsg, MCP_DATATAG);
		newmsg->next = mfr->messages;
		mfr->messages = newmsg;
	} else {
		/* It's complete.  Execute the callback function for this package. */
		mcp_frame_package_docallback(mfr, newmsg);
		mcp_mesg_clear(newmsg);
		free(newmsg);
	}
	return 1;
}


int
mcp_intern_is_mesg_cont(McpFrame * mfr, const char *in)
{
	char datatag[128];
	char keyname[128];
	McpMesg *ptr;

	if (*in != '*') {
		return 0;
	}
	in++;
	if (!isspace(*in))
		return 0;
	while (isspace(*in))
		in++;
	if (!mcp_intern_is_unquoted(&in, datatag, sizeof(datatag)))
		return 0;
	if (!isspace(*in))
		return 0;
	while (isspace(*in))
		in++;
	if (!mcp_intern_is_ident(&in, keyname, sizeof(keyname)))
		return 0;
	if (*in != ':')
		return 0;
	in++;
	if (!isspace(*in))
		return 0;
	in++;

	for (ptr = mfr->messages; ptr; ptr = ptr->next) {
		if (!strcmp(datatag, ptr->datatag)) {
			mcp_mesg_arg_append(ptr, keyname, in);
			break;
		}
	}
	if (!ptr) {
		return 0;
	}
	return 1;
}


int
mcp_intern_is_mesg_end(McpFrame * mfr, const char *in)
{
	char datatag[128];
	McpMesg *ptr, **prev;

	if (*in != ':') {
		return 0;
	}
	in++;
	if (!isspace(*in))
		return 0;
	while (isspace(*in))
		in++;
	if (!mcp_intern_is_unquoted(&in, datatag, sizeof(datatag)))
		return 0;
	if (*in)
		return 0;
	prev = &mfr->messages;
	for (ptr = mfr->messages; ptr; ptr = ptr->next, prev = &ptr->next) {
		if (!strcmp(datatag, ptr->datatag)) {
			*prev = ptr->next;
			break;
		}
	}
	if (!ptr) {
		return 0;
	}
	ptr->incomplete = 0;
	mcp_frame_package_docallback(mfr, ptr);
	mcp_mesg_clear(ptr);
	free(ptr);

	return 1;
}


int
mcp_internal_parse(McpFrame * mfr, const char *in)
{
	if (mcp_intern_is_mesg_cont(mfr, in))
		return 1;
	if (mcp_intern_is_mesg_end(mfr, in))
		return 1;
	if (mcp_intern_is_mesg_start(mfr, in))
		return 1;
	return 0;
}


/*
* $Log: not supported by cvs2svn $
* Revision 1.11  2001/02/02 05:03:44  revar
* Added descr, trigger, player, and prog_uid datums to SEND_EVENT context.
* Updated man.txt docs for SEND_EVENT changes and WATCHPID.
* Added MCP message size limit.  Defaults to half a meg.
*
* Revision 1.10  2001/01/06 23:01:17  revar
* Fixed bug with \r's in MCP arguments.
*
* Revision 1.9  2000/12/28 03:02:08  revar
* Fixed support for Linux mallinfo() calls in @memory.
* Fixed a crasher bug in ARRAY_NUNION, ARRAY_NDIFF, and ARRAY_NINTERSECT.
* Fixed support for catching exceptions thrown in other muf programs.
* Fixed some obscure bugs with getting gmt_offset on some systems.
* Changed a whole lot of variables from 'new', 'delete', and 'class' to
*  possibly allow moving to C++ eventually.
* Added FINDNEXT primitive.
* Updated TODO list.
*
* Revision 1.8  2000/11/23 10:30:22  revar
* Changes for BSD compatability.
* Changes to correct various sprintf format strings.
*
* Revision 1.7  2000/08/23 10:00:02  revar
* Added @tops, @muftops, and @mpitops profiling commands.
* Changed examine to show a program's cumulative runtimes.
* Changes @ps to show process' %CPU usage.
*
* Revision 1.6  2000/07/18 18:12:40  winged
* Various fixes to fix warnings under -Wall -Wstrict-prototypes -Wno-format -- not all problems are found or fixed yet
*
* Revision 1.5  2000/07/07 09:23:11  revar
* Fixed microscopic memory leak with re-registering MCP package handlers.
*
* Revision 1.4  2000/04/30 11:03:30  revar
* Fixed MCP crasher bug when client fails to send authentication key.
*
* Revision 1.3  2000/03/29 12:21:02  revar
* Reformatted all code into consistent format.
* 	Tabs are 4 spaces.
* 	Indents are one tab.
* 	Braces are generally K&R style.
* Added ARRAY_DIFF, ARRAY_INTERSECT and ARRAY_UNION to man.txt.
* Rewrote restart script as a bourne shell script.
*
* Revision 1.2  2000/02/10 06:11:55  winged
* Added log to bottom and comment to top
*
*/


