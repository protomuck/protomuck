#define BIG_BUF		2048

#define ESEND_COMM_ERROR	-1
#define ESEND_BAD_LIST		-2
#define ESEND_CANT_CONNECT	-3
#define ESEND_CANT_FIND_MAILER	-4

extern int email_start_send(const char *to, const char *from, const char *subject, int *mysock);
extern int email_send_line(char *line, int mysock);
extern int email_end_send(int mysock);
