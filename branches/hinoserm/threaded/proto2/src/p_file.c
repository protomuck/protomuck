#ifndef PROTO_AS_ROOT
#define SECURE_FILE_PRIMS
#endif
#include "copyright.h"
#include "config.h"
#ifndef WIN32
#ifdef FILE_PRIMS
#include "db.h"
#include "tune.h"
#include "inst.h"
#include "externs.h"
#include "match.h"
#include "interface.h"
#include "params.h"
#include "strings.h"
#include "interp.h"

char *tempc = NULL;
char *directory = (char *)"$";


#ifdef WIN_VC
int getuid(void) { return 0; }
int getgid(void) { return 0; }
#endif

char *
set_directory(char *filename)
{
    char tempbuf[BUFFER_LEN] = FILE_PRIMS_DIRECTORY;

    strcat(tempbuf, filename);
    filename = tempbuf;
    return filename;
}

int
valid_name(char *filename)
{
    int valid = 1;

    if (strchr(filename, ' ') != NULL)
        valid = 0;
    if (strstr(filename, "..") != NULL)
        valid = 0;
    return valid;
}

char *
parse_token(char *filename)
{
    char *temp;

    if (strstr(filename, "$WELCOME.TXT") != NULL)
        return (char *)"data/welcome.txt";
    if (strstr(filename, "$WELCOME.HTML") != NULL)
        return (char *)"data/welcome.html";
    if (strstr(filename, "$NEWS.TXT") != NULL)
        return (char *)"data/news.txt";
    if (strstr(filename, "$MOTD.TXT") != NULL)
        return (char *)"data/motd.txt";
    if (strstr(filename, "$CONNECT.TXT") != NULL)
        return (char *)"data/connect.txt";
    if (strstr(filename, "$HELP.TXT") != NULL)
        return (char *)"data/help.txt";
    if (strstr(filename, "$MAN.TXT") != NULL)
        return (char *)"data/man.txt";
    if (strstr(filename, "$SYSPARMS.TXT") != NULL)
        return (char *)"data/sysparms.txt";
    if (strstr(filename, "$MPIHELP.TXT") != NULL)
        return (char *)"data/mpihelp.txt";
    if ((temp = strstr(filename, "$NEWS/")) != NULL) {
        char tempBuf[BUFFER_LEN] = "";
        char tempBuf2[BUFFER_LEN] = "data/news/";
        int i = 0;

        temp += 6;
        for (; *temp != '\0'; temp++, i++)
            tempBuf[i] = *temp;
        i++;
        tempBuf[i] = '\0';
        strcat(tempBuf2, tempBuf);
        filename = tempBuf2;
        return filename;
    }
    if ((temp = strstr(filename, "$HELP/")) != NULL) {
        char tempBuf[BUFFER_LEN] = "";
        char tempBuf2[BUFFER_LEN] = "data/help/";
        int i = 0;

        temp += 6;
        for (; *temp != '\0'; temp++, i++)
            tempBuf[i] = *temp;
        i++;
        tempBuf[i] = '\0';
        strcat(tempBuf2, tempBuf);
        filename = tempBuf2;
        return filename;
    }
    if ((temp = strstr(filename, "$MUF/")) != NULL) {
        char tempBuf[BUFFER_LEN] = "";
        char tempBuf2[BUFFER_LEN] = "muf/";
        int i = 0;

        temp += 5;
        for (; *temp != '\0'; temp++, i++)
            tempBuf[i] = *temp;
        i++;
        tempBuf[i] = '\0';
        strcat(tempBuf2, tempBuf);
        filename = tempBuf2;
        return filename;
    }
    if ((temp = strstr(filename, "$LOGS/")) != NULL) {
        char tempBuf[BUFFER_LEN] = "";
        char tempBuf2[BUFFER_LEN] = "logs/";
        int i = 0;

        temp += 6;
        for (; *temp != '\0'; temp++, i++)
            tempBuf[i] = *temp;
        i++;
        tempBuf[i] = '\0';
        strcat(tempBuf2, tempBuf);
        filename = tempBuf2;
        return filename;
    }
    if ((temp = strstr(filename, "$INFO/")) != NULL) {
        char tempBuf[BUFFER_LEN] = "";
        char tempBuf2[BUFFER_LEN] = "data/info/";
        int i = 0;

        temp += 6;
        for (; *temp != '\0'; temp++, i++)
            tempBuf[i] = *temp;
        i++;
        tempBuf[i] = '\0';
        strcat(tempBuf2, tempBuf);
        filename = tempBuf2;
        return filename;
    }
    if ((temp = strstr(filename, "$MAN/")) != NULL) {
        char tempBuf[BUFFER_LEN] = "";
        char tempBuf2[BUFFER_LEN] = "data/man/";
        int i = 0;

        temp += 5;
        for (; *temp != '\0'; temp++, i++)
            tempBuf[i] = *temp;
        i++;
        tempBuf[i] = '\0';
        strcat(tempBuf2, tempBuf);
        filename = tempBuf2;
        return filename;
    }
    if ((temp = strstr(filename, "$MPIHELP/")) != NULL) {
        char tempBuf[BUFFER_LEN] = "";
        char tempBuf2[BUFFER_LEN] = "data/mpihelp/";
        int i = 0;

        temp += 9;
        for (; *temp != '\0'; temp++, i++)
            tempBuf[i] = *temp;
        i++;
        tempBuf[i] = '\0';
        strcat(tempBuf2, tempBuf);
        filename = tempBuf2;
        return filename;
    }
    if ((temp = strstr(filename, "$WELCOME/")) != NULL) {
        char tempBuf[BUFFER_LEN] = "";
        char tempBuf2[BUFFER_LEN] = "data/welcome/";
        int i = 0;

        temp += 9;
        for (; *temp != '\0'; temp++, i++)
            tempBuf[i] = *temp;
        i++;
        tempBuf[i] = '\0';
        strcat(tempBuf2, tempBuf);
        filename = tempBuf2;
        return filename;
    }
    if ((temp = strstr(filename, "$PUBLIC_HTML/")) != NULL) {
        char tempBuf[BUFFER_LEN] = "";
        char tempBuf2[BUFFER_LEN] = "../../public_html/";
        int i = 0;

        temp += 13;
        for (; *temp != '\0'; temp++, i++)
            tempBuf[i] = *temp;
        i++;
        tempBuf[i] = '\0';
        strcat(tempBuf2, tempBuf);
        filename = tempBuf2;
        return filename;
    }
    if ((temp = strstr(filename, "$WWW/")) != NULL) {
        char tempBuf[BUFFER_LEN] = "";
        char tempBuf2[BUFFER_LEN] = "../../www/";
        int i = 0;

        temp += 4;
        for (; *temp != '\0'; temp++, i++)
            tempBuf[i] = *temp;
        i++;
        tempBuf[i] = '\0';
        strcat(tempBuf2, tempBuf);
        filename = tempBuf2;
        return filename;
    }
    return NULL;
}

void
prim_fwrite(PRIM_PROTOTYPE)
{
    FILE *fh;
    char *filename;
    char *writestring, *buf3;
    double offset;

    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();
#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Arguement 1 is not an integer.");
    if (oper1->data.number < 0)
        abort_interp("Arguement 1 is a negative number.");
    if (oper2->type != PROG_STRING)
        abort_interp("Arguement 2 is not a string.");
    if (!oper2->data.string)
        abort_interp("Arguement 2 is a null string.");
    if (oper3->type != PROG_STRING)
        abort_interp("Arguement 3 is not a string.");
    if (!oper3->data.string)
        abort_interp("Arguement 3 is a null string.");
    offset = oper1->data.number;
    filename = oper2->data.string->data;
    writestring = oper3->data.string->data;
#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(filename)))
        abort_interp("Invalid file name.");
    if (strchr(filename, '$') == NULL)
        filename = set_directory(filename);
    else
        filename = parse_token(filename);
    if (filename == NULL)
        abort_interp("Invalid shortcut used.");
#endif
    fh = fopen(filename, "w");
    if (fh == NULL) {
        result = 0;
    } else {
        fseek(fh, (int) offset, SEEK_SET);
        buf3 = writestring;
        while (*buf3) {
            if (*buf3 == '\r') {
                *buf3 = '\n';
            }
            buf3++;
        }
        fputs(writestring, fh);
        fclose(fh);
        result = 1;
        if (tp_log_files)
            log2filetime("logs/files", "#%d by %s FWRITE: %s \n", program,
                         unparse_object(PSafe, PSafe),
                         oper2->data.string->data);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushInt(result);
}

void
prim_fappend(PRIM_PROTOTYPE)
{
    FILE *fh;
    char *filename;
    char *writestring, *buf3;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    if (oper1->type != PROG_STRING)
        abort_interp("Argument 1 is not a string.");
    if (!oper1->data.string)
        abort_interp("Argument 1 is a null string.");
    if (oper2->type != PROG_STRING)
        abort_interp("Arguement 2 is not a string.");
    if (!oper2->data.string)
        abort_interp("Arguement 2 is a null string.");
    filename = oper1->data.string->data;
    writestring = oper2->data.string->data;
#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(filename)))
        abort_interp("Invalid file name.");
    if (strchr(filename, '$') == NULL)
        filename = set_directory(filename);
    else
        filename = parse_token(filename);
    if (filename == NULL)
        abort_interp("Invalid shortcut used.");
#endif
    fh = fopen(filename, "a");
    if (fh == NULL) {
        result = 0;
    } else {
        buf3 = writestring;
        while (*buf3) {
            if (*buf3 == '\r') {
                *buf3 = '\n';
            }
            buf3++;
        }
        fputs(writestring, fh);
        fclose(fh);
        result = 1;
        if (tp_log_files)
            log2filetime("logs/files", "#%d by %s FAPPEND: %s \n", program,
                         unparse_object(PSafe, PSafe),
                         oper1->data.string->data);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void
prim_fread(PRIM_PROTOTYPE)
{
    FILE *fh;
    char *filename;
    double offset;
    int tempchr;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        abort_interp("MUCK is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Arguement 1 is not an integer.");
    if (oper1->data.number < 0)
        abort_interp("Arguement 1 is a negative number.");
    if (oper2->type != PROG_STRING)
        abort_interp("Arguement 2 is not a string.");
    if (!oper2->data.string)
        abort_interp("Argueemnt 2 is a null string.");
    offset = oper1->data.number;
    filename = oper2->data.string->data;
#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(filename)))
        abort_interp("Invalid file name.");
    if (strchr(filename, '$') == NULL)
        filename = set_directory(filename);
    else
        filename = parse_token(filename);
    if (filename == NULL)
        abort_interp("Invalid shortcut used.");
#endif
    fh = fopen(filename, "r");
    if (fh == NULL) {
        result = 0;
    } else {
        fseek(fh, (int) offset, SEEK_SET);
        tempchr = fgetc(fh);

        if (tempchr == '\n')
            tempchr = '\r';

        if (tempchr == EOF) {
            result = 0;
        } else {
            result = 1;
            buf[0] = (char) tempchr;
            buf[1] = '\0';
        }

        fclose(fh);

        if (tp_log_files)
            log2filetime("logs/files", "#%d by %s FREAD: %s \n", program,
                         unparse_object(PSafe, PSafe),
                         oper2->data.string->data);
    }

    CLEAR(oper1);
    CLEAR(oper2);

    if (result)
        PushString(buf);
    else
        PushNullStr;
}

void
prim_freadn(PRIM_PROTOTYPE)
{
    FILE *fh;
    char *filename, *buf3;
    double offset;
    double range;
    char tempBuf[BUFFER_LEN] = "";
    int result = 0;
    int i;
    int found_end = 0;
    int tempChr;

    CHECKOP(3);
    oper1 = POP();              /*The range */
    oper2 = POP();              /*The offset */
    oper3 = POP();              /*The filename */
#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        /*Permissions checks */
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    /*Type Checking */
    if (oper1->type != PROG_INTEGER)
        abort_interp("Arguement 1 is not an integer.");
    if (oper1->data.number > BUFFER_LEN - 10)
        abort_interp("Range is too large. (1)");
    if (oper1->data.number < 0)
        abort_interp("Arguement 1 is a negative number.");
    if (oper2->type != PROG_INTEGER)
        abort_interp("Arguement 2 is not an integer.");
    if (oper2->data.number < 0)
        abort_interp("Arguement 2 is a negative number.");
    if (oper3->type != PROG_STRING)
        abort_interp("Arguement 3 is not a string.");
    if (!oper3->data.string)
        abort_interp("Argueemnt 3 is a null string.");
    /*Value assignments */
    range = oper1->data.number;
    offset = oper2->data.number;
    filename = oper3->data.string->data;
#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(filename)))
        abort_interp("Invalid file name.");
    if (strchr(filename, '$') == NULL)
        filename = set_directory(filename);
    else
        filename = parse_token(filename);
    if (filename == NULL)
        abort_interp("Invalid shortcut used.");
#endif
    fh = fopen(filename, "r");
    if (fh == NULL) {
        result = 0;
    } else {
        for (i = 0; i < range && found_end != 1; i++, offset++) {
            fseek(fh, (int) offset, SEEK_SET);
            tempChr = fgetc(fh);
            if (tempChr == EOF)
                found_end = 1;
            else
                tempBuf[i] = (char) tempChr;
        }
        i++;
        tempBuf[i] = '\0';
        fclose(fh);
/*      if (tempBuf[0] != EOF) */
/*          result = 1; */
        if (tp_log_files)
            log2filetime("logs/files", "#%d by %s FREADN: %s \n", program,
                         unparse_object(PSafe, PSafe),
                         oper3->data.string->data);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    buf3 = tempBuf;
    while (*buf3) {
        if (*buf3 == '\n') {
            *buf3 = '\r';
        }
        buf3++;
    }
    if (result)
        PushString(tempBuf);
    else
        PushNullStr;
}

void
prim_fcr(PRIM_PROTOTYPE)
{
    FILE *fh;
    char *filename;

    CHECKOP(1);
    oper1 = POP();
#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    if (oper1->type != PROG_STRING)
        abort_interp("Argument 1 is not a string.");
    if (!oper1->data.string)
        abort_interp("Argument 1 is a null string.");
    filename = oper1->data.string->data;
#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(filename)))
        abort_interp("Invalid file name.");
    if (strchr(filename, '$') == NULL)
        filename = set_directory(filename);
    else
        filename = parse_token(filename);
    if (filename == NULL)
        abort_interp("Invalid shortcut used.");
#endif
    fh = fopen(filename, "a");
    if (fh == NULL) {
        result = 0;
    } else {
        fputs("\n", fh);
        fclose(fh);
        result = 1;
        if (tp_log_files)
            log2filetime("logs/files", "#%d by %s FCR: %s \n", program,
                         unparse_object(PSafe, PSafe),
                         oper1->data.string->data);
    }
    CLEAR(oper1);
    PushInt(result);
}

void
prim_fpublish(PRIM_PROTOTYPE)
{
    char *filename;
    int result;

    CHECKOP(1);
    oper1 = POP();
#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    if (oper1->type != PROG_STRING)
        abort_interp("Arguement 1 is not a string.");
    if (!oper1->data.string)
        abort_interp("Arguement 1 is a null string.");
    filename = oper1->data.string->data;
#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(filename)))
        abort_interp("Invalid file name.");
    if (strchr(filename, '$') == NULL)
        filename = set_directory(filename);
    else
        filename = parse_token(filename);
    if (filename == NULL)
        abort_interp("Invalid shortcut used.");
#endif
#ifndef WIN_VC
    result =
        !chmod(filename,
               S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH |
               S_IXOTH);
#else
	result = 0;
#endif
    if (tp_log_files)
        log2filetime("logs/files", "#%d by %s FCHMOD: %s \n", program,
                     unparse_object(PSafe, PSafe), oper1->data.string->data);
    CLEAR(oper1);
    PushInt(result);
}

void
prim_bread(PRIM_PROTOTYPE)
{
    FILE *fh;                   /* Should return -1 for file open error. */
    char *filename;             /* -2 for EOF. */
    double offset;
    int result;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Arguement 1 is not an integer.");
    if (oper1->data.number < 0)
        abort_interp("Arguement 1 is a negative number.");
    if (oper2->type != PROG_STRING)
        abort_interp("Arguement 2 is not a string.");
    if (!oper2->data.string)
        abort_interp("Argueemnt 2 is a null string.");
    offset = oper1->data.number;
    filename = oper2->data.string->data;
#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(filename)))
        abort_interp("Invalid file name.");
    if (strchr(filename, '$') == NULL)
        filename = set_directory(filename);
    else
        filename = parse_token(filename);
    if (filename == NULL)
        abort_interp("Invalid shortcut used.");
#endif
    fh = fopen(filename, "r");
    if (fh == NULL) {
        result = -1;
    } else {
        fseek(fh, (int) offset, SEEK_SET);
        result = fgetc(fh);
        if (tp_log_files)
            log2filetime("logs/files", "#%d by %s BREAD: %s \n", program,
                         unparse_object(PSafe, PSafe),
                         oper2->data.string->data);
        if (result == EOF) {
            result = -2;
        }
        fclose(fh);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void
prim_bwrite(PRIM_PROTOTYPE)
{
    FILE *fh;
    char *filename;
    int result, tempdat;
    double offset;

    CHECKOP(3);
    oper1 = POP();
    oper2 = POP();
    oper3 = POP();
#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    if (oper1->type != PROG_INTEGER)
        abort_interp("Arguement 1 is not an integer.");
    if (oper1->data.number < 0)
        abort_interp("Arguement 1 is a negative number.");
    if (oper2->type != PROG_STRING)
        abort_interp("Arguement 2 is not a string.");
    if (!oper2->data.string)
        abort_interp("Arguement 2 is a null string.");
    if (oper3->type != PROG_INTEGER)
        abort_interp("Arguement 3 is not an integer.");
    if (oper3->data.number < 0)
        abort_interp("Arguement 3 is a negative number.");
    offset = oper1->data.number;
    filename = oper2->data.string->data;
    tempdat = oper3->data.number;
#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(filename)))
        abort_interp("Invalid file name.");
    if (strchr(filename, '$') == NULL)
        filename = set_directory(filename);
    else
        filename = parse_token(filename);
    if (filename == NULL)
        abort_interp("Invalid shortcut used.");
#endif
    fh = fopen(filename, "w");
    if (fh == NULL) {
        result = 0;
    } else {
        fseek(fh, (int) offset, SEEK_SET);
        fputc(tempdat, fh);
        fclose(fh);
        result = 1;
        if (tp_log_files)
            log2filetime("logs/files", "#%d by %s BWRITE : %s \n", program,
                         unparse_object(PSafe, PSafe),
                         oper2->data.string->data);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushInt(result);
}

void
prim_bappend(PRIM_PROTOTYPE)
{
    FILE *fh;
    char *filename;
    int result, tempdat;

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    if (oper1->type != PROG_STRING)
        abort_interp("Arguement 1 is not a string.");
    if (!oper1->data.string)
        abort_interp("Arguement 1 is a null string.");
    if (oper2->type != PROG_INTEGER)
        abort_interp("Arguement 2 is not an integer.");
    if (oper2->data.number < 0)
        abort_interp("Arguement 2 is a negative number.");
    filename = oper1->data.string->data;
    tempdat = oper2->data.number;
#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(filename)))
        abort_interp("Invalid file name.");
    if (strchr(filename, '$') == NULL)
        filename = set_directory(filename);
    else
        filename = parse_token(filename);
    if (filename == NULL)
        abort_interp("Invalid shortcut used.");
#endif
    fh = fopen(filename, "a");
    if (fh == NULL) {
        result = 0;
    } else {
        fputc(tempdat, fh);
        result = 1;
        if (tp_log_files)
            log2filetime("logs/files", "#%d by %s BAPPEND : %s \n", program,
                         unparse_object(PSafe, PSafe),
                         oper1->data.string->data);
    }
    fclose(fh);
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void
prim_fsize(PRIM_PROTOTYPE)
{
    FILE *fh;
    char *filename;
    long offset;

    CHECKOP(1);
    oper1 = POP();
#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    if (oper1->type != PROG_STRING)
        abort_interp("Arguement 1 is not a string.");
    if (!oper1->data.string)
        abort_interp("Arguement 1 is a null string.");
    filename = oper1->data.string->data;
#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(filename)))
        abort_interp("Invalid file name.");
    if (strchr(filename, '$') == NULL)
        filename = set_directory(filename);
    else
        filename = parse_token(filename);
    if (filename == NULL)
        abort_interp("Invalid shortcut used.");
#endif
    fh = fopen(filename, "r");
    if (!fh) {
        offset = -1;
    } else {
        fseek(fh, (int) 0, SEEK_END);
        offset = ftell(fh);
        if (tp_log_files)
            log2filetime("logs/files", "#%d by %s FSIZE: %s \n", program,
                         unparse_object(PSafe, PSafe),
                         oper1->data.string->data);
        fclose(fh);
    }
    CLEAR(oper1);
    PushInt(offset);
}

void
prim_fstats(PRIM_PROTOTYPE)
{
    struct stat fs;
    char *filename;

    CHECKOP(1);
    oper1 = POP();
#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    if (oper1->type != PROG_STRING)
        abort_interp("Arguement 1 is not a string.");
    if (!oper1->data.string)
        abort_interp("Arguement 1 is a null string.");
    filename = oper1->data.string->data;
#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(filename)))
        abort_interp("Invalid file name.");
    if (strchr(filename, '$') == NULL)
        filename = set_directory(filename);
    else
        filename = parse_token(filename);
    if (filename == NULL)
        abort_interp("Invalid shortcut used.");
#endif
    stat(filename, &fs);
    if (tp_log_files)
        log2filetime("logs/files", "#%d by %s FSTATS : %s \n", program,
                     unparse_object(PSafe, PSafe), oper1->data.string->data);
    CLEAR(oper1);
    CHECKOFLOW(6);
    PushInt(fs.st_gid);
    PushInt(fs.st_uid);
    PushInt(fs.st_ctime);
    PushInt(fs.st_atime);
    PushInt(fs.st_mtime);
    PushInt(fs.st_ino);
}

void
prim_curid(PRIM_PROTOTYPE)
{
    int curuid = getuid();
    int curgid = getgid();
#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    if (tp_log_files)
        log2filetime("logs/files", "#%d by %s CURID \n", program,
                     unparse_object(PSafe, PSafe));
    PushInt(curgid);
    PushInt(curuid);
}

void
prim_fsinfo(PRIM_PROTOTYPE)
{
    struct statfs fs;

#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    /* Returns: */
    /* Magic Number, Total Blocks, Free Blocks, Available Blocks, Total Files, */
    /* Free Files, Block Size (bytes), and Max Name Length. */
    statfs("/", &fs);
    if (tp_log_files)
        log2filetime("logs/files", "#%d by %s FSINFO \n", program,
                     unparse_object(PSafe, PSafe));
    CHECKOFLOW(8);
    PushInt(fs.f_type);
    PushInt(fs.f_blocks);
    PushInt(fs.f_bfree);
    PushInt(fs.f_bavail);
    PushInt(fs.f_files);
    PushInt(fs.f_ffree);
    PushInt(fs.f_bsize);
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
    {
        int x = MNAMELEN;

        PushInt(x);
    }
#else
    PushInt(fs.f_namelen);
#endif
}

void
prim_frm(PRIM_PROTOTYPE)
{
    char *filename;

    CHECKOP(1);
    oper1 = POP();
#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    if (oper1->type != PROG_STRING)
        abort_interp("Argument 1 is not a string.");
    if (!oper1->data.string)
        abort_interp("Argument 1 is a null string.");
    filename = oper1->data.string->data;
#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(filename)))
        abort_interp("Invalid file name.");
    if (strchr(filename, '$') == NULL)
        filename = set_directory(filename);
    else
        filename = parse_token(filename);
    if (filename == NULL)
        abort_interp("Invalid shortcut used.");
#endif
    result = unlink(filename);
    if (tp_log_files)
        log2filetime("logs/files", "#%d by %s FRM: %s \n", program,
                     unparse_object(PSafe, PSafe), oper1->data.string->data);
    CLEAR(oper1);
    PushInt(result);
}

void
prim_fren(PRIM_PROTOTYPE)
{
    char *oldname, *newname;
    char tempB[BUFFER_LEN] = "";

    CHECKOP(2);
    oper1 = POP();
    oper2 = POP();
#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    if (oper1->type != PROG_STRING)
        abort_interp("Argument 1 is not a string.");
    if (!oper1->data.string)
        abort_interp("Argument 1 is a null string.");
    if (oper2->type != PROG_STRING)
        abort_interp("Argument 2 is not a string.");
    if (!oper2->data.string)
        abort_interp("Argument 2 is a null string.");
    newname = oper1->data.string->data;
    oldname = oper2->data.string->data; /* ( s<old> s<new> -- i ) */
#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(newname)))
        abort_interp("Invalid file name. (2)");
    if (strchr(newname, '$') == NULL)
        newname = set_directory(newname);
    else
        newname = parse_token(newname);
    if (newname == NULL)
        abort_interp("Invalid shortcut used. (2)");
    strcpy(tempB, newname);
    if (!(valid_name(oldname)))
        abort_interp("Invalid file name. (1)");
    if (strchr(oldname, '$') == NULL)
        oldname = set_directory(oldname);
    else
        oldname = parse_token(oldname);
    if (oldname == NULL)
        abort_interp("Invalid shortcut used. (1)");
    newname = tempB;
#endif
    result = rename(oldname, newname);
    if (tp_log_files)
        log2filetime("logs/files", "#%d by %s FREN: %s -> %s \n", program,
                     unparse_object(PSafe, PSafe), oper2->data.string->data,
                     oper1->data.string->data);
    CLEAR(oper1);
    CLEAR(oper2);
    PushInt(result);
}

void
prim_freadto(PRIM_PROTOTYPE)
{
    FILE *fh;
    char *filename, *buf3;
    int offset;
    char tempBuf[BUFFER_LEN] = "";
    int i, found_end = 0;
    int checkChar;
    int tempChr;

    CHECKOP(3);

    oper1 = POP();              /*The search character. */
    oper2 = POP();              /*The offset value. */
    oper3 = POP();              /*The file name. */

    /*Permissions checks */
#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("FREADTO is BOY level prim only.");

    /*Type Checking */
    if (oper1->type != PROG_STRING)
        abort_interp("Arguement 1 is not a string.");
    if (!oper1->data.string)
        abort_interp("Arguement 1 is a null string.");
    if (oper2->type != PROG_INTEGER)
        abort_interp("Arguement 2 is not an integer.");
    if (oper2->data.number < 0)
        abort_interp("Arguement 2 is a negative number.");
    if (oper3->type != PROG_STRING)
        abort_interp("Arguement 3 is not a string.");
    if (!oper3->data.string)
        abort_interp("Arguement 3 is a null string.");

    /*Value Assignment */
    offset = oper2->data.number;
    filename = oper3->data.string->data;
#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(filename)))
        abort_interp("Invalid file name.");
    if (strchr(filename, '$') == NULL)
        filename = set_directory(filename);
    else
        filename = parse_token(filename);
    if (filename == NULL)
        abort_interp("Invalid shortcut used.");
#endif
    if (!(strcasecmp(oper1->data.string->data, "\r")))
        checkChar = '\n';
    else if (!(strcasecmp(oper1->data.string->data, "$EOF$")))
        checkChar = EOF;
    else
        checkChar = oper1->data.string->data[0];

    /* Routines */
    fh = fopen(filename, "r");
    if (fh == NULL)
        result = 0;
    else {
        for (i = 0; i < BUFFER_LEN - 4 && found_end != 1; i++, offset++) {
            fseek(fh, (int) offset, SEEK_SET);
            tempChr = (char) fgetc(fh);
            if (tempChr == EOF || tempChr == checkChar)
                found_end = 1;
            else
                tempBuf[i] = (char) tempChr;
            if (checkChar == '\n' && tempChr == '\n' && i == 0)
                tempBuf[i] = ' ';
        }
        i++;
        tempBuf[i] = '\0';
        fclose(fh);
        if (tempBuf[0] != EOF) 
            result = 1; 
        if (tp_log_files)
            log2filetime("logs/files", "#%d by %s FREADN: %s \n", program,
                         unparse_object(PSafe, PSafe),
                         oper3->data.string->data);
    }
    CLEAR(oper1);
    CLEAR(oper2);
    CLEAR(oper3);
    PushInt(offset);
    buf3 = tempBuf;
    while (*buf3) {
        if (*buf3 == '\n') {
            *buf3 = '\r';
        }
        buf3++;
    }
    if (result)
        PushString(tempBuf);
    else
        PushNullStr;
}

void
prim_fnameokp(PRIM_PROTOTYPE)
{
    char *filename;
    int result = 1;

    CHECKOP(1);
    oper1 = POP();              /* string */
    /* Permissions and Type checks */
#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    if (oper1->type != PROG_STRING)
        abort_interp("String arguement expected.");
    if (!oper1->data.string)
        abort_interp("Arguement is an empty string.");

    filename = oper1->data.string->data;
#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(filename)))
        result = 0;
    else {
        if (strchr(filename, '$') == NULL)
            filename = set_directory(filename);
        else
            filename = parse_token(filename);
        if (filename == NULL)
            result = 0;
    }
#endif
    CLEAR(oper1);
    PushInt(result);
}

void
prim_getdir(PRIM_PROTOTYPE)
{
    char tempDir[BUFFER_LEN];
    char *directoryName;
    char curName[BUFFER_LEN];
    stk_array *nu;
    struct inst temp1;
    DIR *df;
    DIR *tempDf;
    struct dirent *dp;

    CHECKOP(1);
    oper1 = POP();              /* string */

    /* Permissions and type checks */
#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    if (oper1->type != PROG_STRING)
        abort_interp("String arguement expected.");
    if (!oper1->data.string)
        abort_interp("Arguement is an empty string.");
    CHECKOFLOW(1);

    directoryName = oper1->data.string->data;
    result = 1;

#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(directoryName)))
        result = 0;
    else {
        if (strchr(directoryName, '$') == NULL)
            directoryName = set_directory(directoryName);
        else
            directoryName = parse_token(directoryName);
        if (directoryName == NULL)
            result = 0;
    }
    if (!result)
        abort_interp("Invalid file name.");
#endif
    result = 1;
    nu = new_array_packed(0);
    if ((df = (DIR *) opendir(directoryName))) {
        while ((dp = readdir(df))) {
            if (*(dp->d_name) != '.') {
                strcpy(curName, dp->d_name);
                strcpy(tempDir, directoryName);
                strcat(tempDir, "/");
                strcat(tempDir, dp->d_name);
                if ((tempDf = (DIR *) opendir(tempDir))) {
                    closedir(tempDf);
                    strcat(curName, "/");
                }
                temp1.type = PROG_STRING;
                temp1.data.string = alloc_prog_string(curName);
                array_appenditem(&nu, &temp1);
                CLEAR(&temp1);
            }
        }
        closedir(df);
    } else                      /* no directory */
        result = 0;

    CLEAR(oper1);
    if (!result)
        PushInt(result);
    else
        PushArrayRaw(nu);
}

void
prim_mkdir(PRIM_PROTOTYPE)
{
    char *directoryName;

    CHECKOP(1);
    oper1 = POP();              /* string */

    /* Permissions and type checks */
#ifndef PROTO_AS_ROOT
    if (getuid() == 0)
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    if (oper1->type != PROG_STRING)
        abort_interp("String arguement expected.");
    if (!oper1->data.string)
        abort_interp("Arguement is an empty string.");
    CHECKOFLOW(1);

    directoryName = oper1->data.string->data;
    result = 1;

#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(directoryName)))
        result = 0;
    else {
        if (strchr(directoryName, '$') == NULL)
            directoryName = set_directory(directoryName);
        else
            directoryName = parse_token(directoryName);
        if (directoryName == NULL)
            result = 0;
    }
    if (!result)
        abort_interp("Invalid file name.");
#endif
    /* Everything has been checked, make the directory now */
    result = mkdir(directoryName, S_IRWXU);
    result = !result;
    PushInt(result);
}

void
prim_rmdir(PRIM_PROTOTYPE)
{
    char *directoryName;
    
    CHECKOP(1);
    oper1 = POP();

#ifndef PROTO_AS_ROOT
    if (getuid() == 0 )
        abort_interp("Muck is running under root privs, file prims disabled.");
#endif
    
    if (mlev < LBOY)
        abort_interp("BOY primitive only.");
    if (oper1->type != PROG_STRING)
        abort_interp("Argument 1 is not a string.");
    if (!oper1->data.string)
        abort_interp("Argument 1 is a null string.");
    directoryName = oper1->data.string->data;

#ifdef SECURE_FILE_PRIMS
    if (!(valid_name(directoryName)))
        abort_interp("Invalid file name.");
    if (strchr(directoryName, '$') == NULL)
        directoryName = set_directory(directoryName);
    else
        directoryName = parse_token(directoryName);
    if (directoryName == NULL)
        abort_interp("Invalid shortcut used.");
#endif
    result = rmdir(directoryName);
    if (tp_log_files)
        log2filetime("logs/files", "#%d by %s RMDIR: %s \n", program,
                     unparse_object(PSafe, PSafe),
                     oper1->data.string->data);
    CLEAR(oper1);
    PushInt(result);
}
#endif /* FILE_PRIMS */
#endif /* WIN32 */