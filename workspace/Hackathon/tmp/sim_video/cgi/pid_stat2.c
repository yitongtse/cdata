#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cgi_util.h"


#define TEMPLATE_DIR    "/home/pi/Hackathon/Site/cgi-bin/Templates/"
#define STAT_DIR        "/home/pi/Hackathon/Site/cgi-bin/analyzer_stat/"

char data[10000];

int main (int argc, char **argv)
{
    FILE *fp = fopen(STAT_DIR "pid_stat", "r");
    fgets(data, 10000, fp);
    printf("Context-type:text/html\n\n");
    return sub_template(TEMPLATE_DIR "pid_stat.html", data);
}
