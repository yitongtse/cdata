#include <stdio.h>
#include <stdlib.h>
#include <time.h>


int main()
{
    char buf[256];
    time_t cur_time = time(NULL);
    struct tm* loc_time = localtime(&cur_time);

    printf("%4d/%02d/%02d %02d:%02d:%02d\n",
           loc_time->tm_year + 1900, loc_time->tm_mon + 1,
           loc_time->tm_mday, loc_time->tm_hour, loc_time->tm_min,
           loc_time->tm_sec);

    strftime(buf, 256, "%Y/%m/%d %H:%M:%S", loc_time);
    printf("%s\n", buf);

    strftime(buf, 256, "%Y%m%dT%H%M%S.0Z", loc_time);
    printf("%s\n", buf);
    return 0;
}
