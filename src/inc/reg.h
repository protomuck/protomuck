#ifndef INCLUDED_REG_H
#define INCLUDED_REG_H

extern int   reg_site_can_request(int);
extern int   reg_site_is_blocked(int);
extern int   reg_site_is_barred(int);
extern int   reg_user_is_barred(int,int);
extern int   reg_site_can_create(int);
extern int   reg_site_can_connect(int);
extern void  reg_make_password(char *);
extern int   reg_email_is_a_jerk(const char *);
extern int   name_is_bad(const char *);
extern const char *reg_user_is_suspended(int);
extern const char *reg_site_welcome(int);
extern const char *strcasestr2(const char *, const char *);


#endif /* INCLUDED_REG_H */
