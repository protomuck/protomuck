#include "copyright.h"
#include "config.h"

#include "db.h"
#include "externs.h"
#include "params.h"
#include "tune.h"
#include "interface.h"
#include "props.h"

static char upb[BUFFER_LEN];

const char *
unparse_flags(dbref thing, char buf[BUFFER_LEN])
{
    char   *p;
    const char *type_codes = "R-EPFG";

    p = buf;
    if (Typeof(thing) != TYPE_THING)
	*p++ = type_codes[Typeof(thing)];
    if (FLAGS(thing) & ~TYPE_MASK) {
	/* print flags */
	if (FLAGS(thing) & BUILDER)
	    *p++ = 'B';
	if (FLAGS(thing) & JUMP_OK)
	    *p++ = 'J';
	if (FLAGS(thing) & LINK_OK)
	    *p++ = 'L';
	if (FLAGS(thing) & DARK)
	    *p++ = 'D';
	if (FLAGS(thing) & STICKY)
	    *p++ = 'S';
	if (FLAGS(thing) & QUELL)
	    *p++ = 'Q';
	if (FLAGS(thing) & CHOWN_OK)
	    *p++ = 'C';
	if (FLAGS(thing) & HAVEN)
	    *p++ = 'H';
	if (FLAGS(thing) & ABODE)
	    *p++ = 'A';
	if (FLAGS(thing) & VEHICLE)
	    *p++ = 'V';
	if (FLAGS(thing) & XFORCIBLE)
	    *p++ = 'X';
	if (FLAGS(thing) & ZOMBIE)
	    *p++ = 'Z';
    }
    if (FLAG2(thing)) {
	if (FLAG2(thing) & F2GUEST)
	    *p++ = 'G';
	if (FLAG2(thing) & F2IDLE)
	    *p++ = 'I';
	if (FLAG2(thing) & F2LOGWALL)
	    *p++ = '!';
	if (FLAG2(thing) & F2MUFCOUNT)
	    *p++ = '+';
	if (FLAG2(thing) & F2LIGHT)
	    *p++ = 'O';
      if (FLAG2(thing) & F2PARENT)
          *p++ = '%';
      if (FLAG2(thing) & F2PROTECT)
          *p++ = '*';
      if (FLAG2(thing) & F2ANTIPROTECT)
          *p++ = 'K';
      if (FLAG2(thing) & F2EXAMINE_OK)
          *p++ = 'Y';
      if (FLAG2(thing) & F2NO_COMMAND)
          *p++ = 'N';
      if (FLAG2(thing) & F2HIDDEN)
          *p++ = '#';
      if (FLAG2(thing) & F2PUEBLO)
          *p++ = '$';
      if (FLAG2(thing) & F2HTML)
	    *p++ = '&';
    }
    if (thing == 1) {
       *p++ = 'W' ; *p++ = '5' ;
    } else {
       switch (RawMLevel(thing)) {
	   case 0:					break;
 	   case LMPI:	*p++ = 'M'; break;
         case LMAN:  	*p++ = 'W'; *p++ = '5'; break;
         case LBOY:  	*p++ = 'W'; *p++ = '4'; break;
	   case LARCH:	*p++ = 'W'; if (tp_multi_wizlevels) { *p++ = '3'; } break;
	   case LWIZ:	*p++ = 'W'; *p++ = '2'; break;
	   case LMAGE:	*p++ = 'W';			break;
	   case LM3:	*p++ = 'M'; *p++ = '3'; break;
	   case LM2:	*p++ = 'M'; *p++ = '2'; break;
	   case LM1:	*p++ = 'M'; *p++ = '1'; break;
       }
    }
    if (POWERS(thing) && (Typeof(thing) == TYPE_PLAYER)) {
        *p++ = ':';
        if (POWERS(thing) & POW_ANNOUNCE)
            *p++ = 'a';
        if (POWERS(thing) & POW_BOOT)
            *p++ = 'b';
        if (POWERS(thing) & POW_CHOWN_ANYTHING)
            *p++ = 'c';
        if (POWERS(thing) & POW_EXPANDED_WHO)
            *p++ = 'x';
        if (POWERS(thing) & POW_HIDE)
            *p++ = 'h';
        if (POWERS(thing) & POW_IDLE)
            *p++ = 'i';
        if (POWERS(thing) & POW_LINK_ANYWHERE)
            *p++ = 'l';
        if (POWERS(thing) & POW_LONG_FINGERS)
            *p++ = 'g';
        if (POWERS(thing) & POW_NO_PAY)
            *p++ = 'n';
        if (POWERS(thing) & POW_OPEN_ANYWHERE)
            *p++ = 'o';
        if (POWERS(thing) & POW_PLAYER_CREATE)
            *p++ = 'p';
        if (POWERS(thing) & POW_SEARCH)
            *p++ = 's';
        if (POWERS(thing) & POW_SEE_ALL)
            *p++ = 'e';
        if (POWERS(thing) & POW_TELEPORT)
            *p++ = 't';
        if (POWERS(thing) & POW_SHUTDOWN)
            *p++ = 'd';
        if (POWERS(thing) & POW_CONTROL_MUF)
            *p++ = 'f';
        if (POWERS(thing) & POW_CONTROL_ALL)
            *p++ = 'r';
        if (POWERS(thing) & POW_ALL_MUF_PRIMS)
            *p++ = 'm';
    }
    *p = '\0';
    return buf;
}

const char *
unparse_object(dbref player, dbref loc)
{
    char tbuf[BUFFER_LEN];

    if (Typeof(player) != TYPE_PLAYER)
	player = OWNER(player);
    switch (loc) {
	case NOTHING:
	    return "*NOTHING*";
	case AMBIGUOUS:
	    return "*AMBIGUOUS*";
	case HOME:
	    return "*HOME*";
	default:
	    if (loc < 0 || loc > db_top)
#ifdef SANITY
	    {
	        sprintf(upb, "*INVALID*(#%d)", loc);
	        return upb;
	    }
#else
		return "*INVALID*";
#endif
#ifndef SANITY
	    if (!(FLAGS(player) & STICKY) &&
		    (TMage(player) || POWERS(player) & POW_SEE_ALL ||
		     can_link_to(player, NOTYPE, loc) ||
		     controls_link(player, loc) ||
		     ((Typeof(loc) != TYPE_PLAYER) &&
		     (FLAGS(loc) & CHOWN_OK))
		    )) {
		/* show everything */
#endif
		sprintf(upb, "%s(#%d%s)", NAME(loc), loc, unparse_flags(loc, tbuf));
		return upb;
#ifndef SANITY
	    } else {
		/* show only the name */
		return NAME(loc);
	    }
#endif
    }
}


const char *
ansiname(dbref loc, char buf[BUFFER_LEN])
{
    char tbuf[BUFFER_LEN];

    *buf = '\0';
    switch(Typeof(loc)) {
	case TYPE_PLAYER:
	    strcpy( buf, GREEN ); break;
	case TYPE_THING:
	    strcpy( buf, PURPLE ); break;
	case TYPE_EXIT:
	    strcpy( buf, BLUE ); break;
	case TYPE_PROGRAM:
	    strcpy( buf, RED ); break;
	case TYPE_ROOM:
	    strcpy( buf, CYAN ); break;
	default:
	    strcpy( buf, NORMAL );
    }

    strcat( buf, tct(NAME(loc),tbuf) );
    return buf;
}

const char *
ansi_unparse_object(dbref player, dbref loc)
{
    char tbuf[BUFFER_LEN], tbuf2[BUFFER_LEN];

    if (Typeof(player) != TYPE_PLAYER)
	player = OWNER(player);
    switch (loc) {
	case NOTHING:
	    return NORMAL "*NOTHING*";
	case AMBIGUOUS:
	    return PURPLE "*AMBIGUOUS*";
	case HOME:
	    return WHITE "*HOME*";
	default:
	    if (loc < 0 || loc > db_top)
		return RED "*INVALID*";
#ifndef SANITY
	    if (!(FLAGS(player) & STICKY) &&
		    (TMage(player) || POWERS(player) & POW_SEE_ALL ||
                     POWERS(player) & POW_SEARCH ||
		     can_link_to(player, NOTYPE, loc) ||
		     controls_link(player, loc) ||
		     ((Typeof(loc) != TYPE_PLAYER) &&
		     (FLAGS(loc) & CHOWN_OK))
		    )) {
#endif
		/* show everything */
		sprintf(upb, "%s" YELLOW "(#%d%s)",
		    ansiname(loc, tbuf), loc, unparse_flags(loc, tbuf2));
		return upb;
#ifndef SANITY
	    } else {
		/* show only the name */
		return ansiname(loc, upb);
	    }
#endif
    }
}

static char boolexp_buf[BUFFER_LEN];
static char *buftop;

static void 
unparse_boolexp1(dbref player, struct boolexp * b,
		 boolexp_type outer_type, int fullname)
{
    if ((buftop - boolexp_buf) > (BUFFER_LEN / 2))
	return;
    if (b == TRUE_BOOLEXP) {
	strcpy(buftop, "*UNLOCKED*");
	buftop += strlen(buftop);
    } else {
	switch (b->type) {
	    case BOOLEXP_AND:
		if (outer_type == BOOLEXP_NOT) {
		    *buftop++ = '(';
		}
		unparse_boolexp1(player, b->sub1, b->type, fullname);
		*buftop++ = AND_TOKEN;
		unparse_boolexp1(player, b->sub2, b->type, fullname);
		if (outer_type == BOOLEXP_NOT) {
		    *buftop++ = ')';
		}
		break;
	    case BOOLEXP_OR:
		if (outer_type == BOOLEXP_NOT || outer_type == BOOLEXP_AND) {
		    *buftop++ = '(';
		}
		unparse_boolexp1(player, b->sub1, b->type, fullname);
		*buftop++ = OR_TOKEN;
		unparse_boolexp1(player, b->sub2, b->type, fullname);
		if (outer_type == BOOLEXP_NOT || outer_type == BOOLEXP_AND) {
		    *buftop++ = ')';
		}
		break;
	    case BOOLEXP_NOT:
		*buftop++ = '!';
		unparse_boolexp1(player, b->sub1, b->type, fullname);
		break;
	    case BOOLEXP_CONST:
		if (fullname) {
#ifndef SANITY
		    strcpy(buftop, unparse_object(player, b->thing));
#endif
		} else {
		    sprintf(buftop, "#%d", b->thing);
		}
		buftop += strlen(buftop);
		break;
	    case BOOLEXP_PROP:
		strcpy(buftop, PropName(b->prop_check));
		strcat(buftop, ":");
		if (PropType(b->prop_check) == PROP_STRTYP)
		    strcat(buftop, PropDataStr(b->prop_check));
		buftop += strlen(buftop);
		break;
	    default:
		abort();	/* bad type */
		break;
	}
    }
}

const char *
unparse_boolexp(dbref player, struct boolexp * b, int fullname)
{
    buftop = boolexp_buf;
    unparse_boolexp1(player, b, BOOLEXP_CONST, fullname);  /* no outer type */
    *buftop++ = '\0';

    return boolexp_buf;
}








