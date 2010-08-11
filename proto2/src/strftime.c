#include "config.h"

void
int2str(char *buf, int val, int len, char pref)
{
    int lp;

    buf[lp = len] = '\0';
    while (lp--) {
        buf[lp] = '0' + (val % 10);
        val /= 10;
    }
    while (((++lp) < (len - 1)) && (buf[lp] == '0'))
        buf[lp] = pref;
    if (!pref)
        (void) strcpy(buf, buf + lp);
}


int
format_time(char *buf, int max_len, const char *fmt, struct tm *tmval)
{

#if defined(USE_STRFTIME)       /* && !defined(WIN32) */
    return (strftime(buf, max_len, fmt, tmval));
#else
    int pos, ret;
    char tmp[256];

    /* struct timezone tz; */
    pos = 0;
    max_len--;
    while ((*fmt) && (pos < max_len))
        if ((buf[pos++] = *(fmt++)) == '%')
            if (*fmt) {
                pos--;
                *tmp = '\0';
                switch (*(fmt++)) {
                    case 'a':
                        switch (tmval->tm_wday) {
                            case 0:
                                (void) strcpy(tmp, "Sun");
                                break;
                            case 1:
                                (void) strcpy(tmp, "Mon");
                                break;
                            case 2:
                                (void) strcpy(tmp, "Tue");
                                break;
                            case 3:
                                (void) strcpy(tmp, "Wed");
                                break;
                            case 4:
                                (void) strcpy(tmp, "Thu");
                                break;
                            case 5:
                                (void) strcpy(tmp, "Fri");
                                break;
                            case 6:
                                (void) strcpy(tmp, "Sat");
                                break;
                        }
                        break;
                    case 'A':
                        switch (tmval->tm_wday) {
                            case 0:
                                (void) strcpy(tmp, "Sunday");
                                break;
                            case 1:
                                (void) strcpy(tmp, "Monday");
                                break;
                            case 2:
                                (void) strcpy(tmp, "Tuesday");
                                break;
                            case 3:
                                (void) strcpy(tmp, "Wednesday");
                                break;
                            case 4:
                                (void) strcpy(tmp, "Thursday");
                                break;
                            case 5:
                                (void) strcpy(tmp, "Friday");
                                break;
                            case 6:
                                (void) strcpy(tmp, "Saturday");
                                break;
                        }
                        break;
                    case 'h':
                    case 'b':
                        switch (tmval->tm_mon) {
                            case 0:
                                (void) strcpy(tmp, "Jan");
                                break;
                            case 1:
                                (void) strcpy(tmp, "Feb");
                                break;
                            case 2:
                                (void) strcpy(tmp, "Mar");
                                break;
                            case 3:
                                (void) strcpy(tmp, "Apr");
                                break;
                            case 4:
                                (void) strcpy(tmp, "May");
                                break;
                            case 5:
                                (void) strcpy(tmp, "Jun");
                                break;
                            case 6:
                                (void) strcpy(tmp, "Jul");
                                break;
                            case 7:
                                (void) strcpy(tmp, "Aug");
                                break;
                            case 8:
                                (void) strcpy(tmp, "Sep");
                                break;
                            case 9:
                                (void) strcpy(tmp, "Oct");
                                break;
                            case 10:
                                (void) strcpy(tmp, "Nov");
                                break;
                            case 11:
                                (void) strcpy(tmp, "Dec");
                                break;
                        }
                        break;
                    case 'B':
                        switch (tmval->tm_mon) {
                            case 0:
                                (void) strcpy(tmp, "January");
                                break;
                            case 1:
                                (void) strcpy(tmp, "February");
                                break;
                            case 2:
                                (void) strcpy(tmp, "March");
                                break;
                            case 3:
                                (void) strcpy(tmp, "April");
                                break;
                            case 4:
                                (void) strcpy(tmp, "May");
                                break;
                            case 5:
                                (void) strcpy(tmp, "June");
                                break;
                            case 6:
                                (void) strcpy(tmp, "July");
                                break;
                            case 7:
                                (void) strcpy(tmp, "August");
                                break;
                            case 8:
                                (void) strcpy(tmp, "September");
                                break;
                            case 9:
                                (void) strcpy(tmp, "October");
                                break;
                            case 10:
                                (void) strcpy(tmp, "November");
                                break;
                            case 11:
                                (void) strcpy(tmp, "December");
                                break;
                        }
                        break;
                    case 'c':
                        if (!(ret = format_time(buf + pos, max_len - pos,
                                                (char *) "%x %X\0", tmval)))
                            return (0);
                        pos += ret;
                        break;
                    case 'C':
                        if (!(ret = format_time(buf + pos, max_len - pos,
                                                (char *) "%A %B %e, %Y\0",
                                                tmval)))
                            return (0);
                        pos += ret;
                        break;
                    case 'd':
                        int2str(tmp, tmval->tm_mday, 2, '0');
                        break;
                    case 'D':
                        if (!(ret = format_time(buf + pos, max_len - pos,
                                                (char *) "%m/%d/%y\0", tmval)))
                            return (0);
                        pos += ret;
                        break;
                    case 'e':
                        int2str(tmp, tmval->tm_mday, 2, ' ');
                        break;
                    case 'H':
                        int2str(tmp, tmval->tm_hour, 2, '0');
                        break;
                    case 'I':
                        int2str(tmp, (tmval->tm_hour + 11) % 12 + 1, 2, '0');
                        break;
                    case 'j':
                        int2str(tmp, tmval->tm_yday + 1, 3, '0');
                        break;
                    case 'k':
                        int2str(tmp, tmval->tm_hour, 2, ' ');
                        break;
                    case 'l':
                        int2str(tmp, (tmval->tm_hour + 11) % 12 + 1, 2, ' ');
                        break;
                    case 'm':
                        int2str(tmp, tmval->tm_mon + 1, 2, '0');
                        break;
                    case 'M':
                        int2str(tmp, tmval->tm_min, 2, '0');
                        break;
                    case 'p':
                        tmp[0] = ((tmval->tm_hour < 12) ? 'A' : 'P');
                        tmp[1] = 'M';
                        tmp[2] = '\0';
                        break;
                    case 'r':
                        if (!(ret = format_time(buf + pos, max_len - pos,
                                                (char *) "%I:%M:%S %p\0",
                                                tmval)))
                            return (0);
                        pos += ret;
                        break;
                    case 'R':
                        if (!(ret = format_time(buf + pos, max_len - pos,
                                                (char *) "%H:%M\0", tmval)))
                            return (0);
                        pos += ret;
                        break;
                    case 'S':
                        int2str(tmp, tmval->tm_sec, 2, '0');
                        break;
                    case 'T':
                        if (!(ret = format_time(buf + pos, max_len - pos,
                                                (char *) "%H:%M:%S\0", tmval)))
                            return (0);
                        pos += ret;
                        break;
                    case 'U':
                        int2str(tmp,
                                (tmval->tm_yday +
                                 ((7 + tmval->tm_wday -
                                   (tmval->tm_yday % 7)) % 7)) / 7, 2, '0');
                        break;
                    case 'w':
                        tmp[0] = tmval->tm_wday + '0';
                        tmp[1] = '\0';
                        break;
                    case 'W':
                        int2str(tmp,
                                (tmval->tm_yday +
                                 ((13 + tmval->tm_wday -
                                   (tmval->tm_yday % 7)) % 7)) / 7, 2, '0');
                        break;
                    case 'x':
                        if (!(ret = format_time(buf + pos, max_len - pos,
                                                (char *) "%m/%d/%y\0", tmval)))
                            return (0);
                        pos += ret;
                        break;
                    case 'X':
                        if (!(ret = format_time(buf + pos, max_len - pos,
                                                (char *) "%H:%M:%S\0", tmval)))
                            return (0);
                        pos += ret;
                        break;
                    case 'y':
                        int2str(tmp, (tmval->tm_year % 100), 2, '0');
                        break;
                    case 'Y':
                        int2str(tmp, tmval->tm_year + 1900, 4, '0');
                        break;
                    case 'Z':
#ifdef HAVE_TM_ZONE
#ifdef HAVE_TZNAME
#ifdef __CYGWIN__
                        tzset();
#endif
                        strcpy(tmp, tzname[tmval->tm_isdst]);
#else
                        strcpy(tmp, tmval->tm_zone);
#endif
#else
                        strcpy(tmp, "   ");
#endif /* !HAVE_TM_ZONE */
                        break;
                    case '%':
                        tmp[0] = '%';
                        tmp[1] = '\0';
                        break;
                    default:
                        tmp[0] = '%';
                        tmp[1] = *(fmt - 1);
                        tmp[2] = '\0';
                        break;
                }
                if ((pos + strlen(tmp)) > max_len)
                    return (0);
                (void) strcpy(buf + pos, tmp);
                pos += strlen(tmp);
            }
    buf[pos] = '\0';
    return (pos);
#endif /* USE_STRFTIME */
}

time_t
get_tz_offset(void)
{
/*
 * SunOS don't seem to have timezone as a "extern long", but as
 * a structure. This makes it very hard (at best) to check for,
 * therefor I'm checking for tm_gmtoff. --WF
 */
#if defined(BRAINDEAD_OS)
    return 0;
#elif defined(HAVE_STRUCT_TM_TM_GMTOFF)
    time_t now;

    time(&now);
    return (int) (localtime(&now)->tm_gmtoff);
#elif defined(WIN_VC) || defined(WIN32)
    return (int) _timezone;
#elif (HAVE_DECL__TIMEZONE)
    tzset();
    return (int) _timezone;
#else
    return (int) timezone;
#endif
}
