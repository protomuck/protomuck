
#include "copyright.h"
#include "config.h"
#include "params.h"

#include "db.h"
#include "tune.h"
#include "props.h"
#include "externs.h"
#include "interface.h"

PropPtr
propdir_new_elem(PropPtr *l, char *path)
{
    PropPtr p;
    char *n;
    while (*path && *path == PROPDIR_DELIMITER) path++;
    if (!*path) return(NULL);
    n = index(path, PROPDIR_DELIMITER);
    while (n && *n == PROPDIR_DELIMITER) *(n++) = '\0';
    if (n && *n) {
	/* just another propdir in the path */
        p = locate_prop(*l, path);
	if (!p) {
	    /* propdir didn't exist */
	    p = new_prop(l, path);
	}
	return(propdir_new_elem(&PropDir(p), n));
    } else {
	/* aha, we are finally to the property itself. */
        p = locate_prop(*l, path);
	if (!p) {
	    /* property didn't exist */
	    p = new_prop(l, path);
	}
	return(p);
    }
}


/* returns pointer to the updated propdir structure's root node */
/* l is the pointer to the root propdir node */
/* path is the name of the property to delete */
PropPtr 
propdir_delete_elem(PropPtr l, char *path)
{
    PropPtr p;
    char *n;
    if (!l) return(NULL);
    while (*path && *path == PROPDIR_DELIMITER) path++;
    if (!*path) return(l);
    n = index(path, PROPDIR_DELIMITER);
    while (n && *n == PROPDIR_DELIMITER) *(n++) = '\0';
    if (n && *n) {
	/* just another propdir in the path */
        p = locate_prop(l, path);
	if (p && PropDir(p)) {
	    /* yup, found the propdir */
	    SetPDir(p, propdir_delete_elem(PropDir(p), n));
	    if (!PropDir(p) && PropType(p) == PROP_DIRTYP) {
	        l = delete_prop(&l, PropName(p));
	    }
	}
	/* return the updated plist pntr */
	return(l);
    } else {
	/* aha, we are finally to the property itself. */
	p = locate_prop(l, path);
	if (p && PropDir(p))
	    delete_proplist(PropDir(p));
	(void)delete_prop(&l, path);
	return(l);
    }
}


/* returns pointer to given property */
/* l is the pointer to the root propdir node */
/* path is the name of the property to find */
PropPtr 
propdir_get_elem(PropPtr l, char *path)
{
    PropPtr p;
    char *n;
    if (!l) return(NULL);
    while (*path && *path == PROPDIR_DELIMITER) path++;
    if (!*path) return(NULL);
    n = index(path, PROPDIR_DELIMITER);
    while (n && *n == PROPDIR_DELIMITER) *(n++) = '\0';
    if (n && *n) {
	/* just another propdir in the path */
        p = locate_prop(l, path);
	if (p && PropDir(p)) {
	    /* yup, found the propdir */
	    return(propdir_get_elem(PropDir(p), n));
	}
	return(NULL);
    } else {
	/* aha, we are finally to the property subname itself. */
        if((p = locate_prop(l, path)))
	    return(p);  /* found the property! */
	return(NULL);   /* nope, doesn't exist */
    }
}


/* returns pointer to first property in the given propdir */
/* l is the pointer to the root propdir node */
/* path is the name of the propdir to find the first node of */
PropPtr 
propdir_first_elem(PropPtr l, char *path)
{
    PropPtr p;
    while (*path && *path == PROPDIR_DELIMITER) path++;
    if (!*path) return(first_node(l));
    p = propdir_get_elem(l, path);
    if(p && PropDir(p))
	return(first_node(PropDir(p)));  /* found the property! */
    return(NULL);   /* nope, doesn't exist */
}


/* returns pointer to next property after the given one in the propdir */
/* l is the pointer to the root propdir node */
/* path is the name of the property to find the next node after */
/* Note: Finds the next alphabetical property, regardless of the existence
	  of the original property given. */
PropPtr 
propdir_next_elem(PropPtr l, char *path)
{
    PropPtr p;
    char *n;
    if (!l) return(NULL);
    while (*path && *path == PROPDIR_DELIMITER) path++;
    if (!*path) return(NULL);
    n = index(path, PROPDIR_DELIMITER);
    while (n && *n == PROPDIR_DELIMITER) *(n++) = '\0';
    if (n && *n) {
	/* just another propdir in the path */
        p = locate_prop(l, path);
	if (p && PropDir(p)) {
	    /* yup, found the propdir */
	    return(propdir_next_elem(PropDir(p), n));
	}
	return(NULL);
    } else {
	/* aha, we are finally to the property subname itself. */
        return(next_node(l, path));
    }
}


/* return true if a property contains a propdir */
int
propdir_check(PropPtr l, char *path)
{
    PropPtr p;
    if ((p = propdir_get_elem(l, path)))
	return(PropDir(p) != NULL);
    return(0);
}


