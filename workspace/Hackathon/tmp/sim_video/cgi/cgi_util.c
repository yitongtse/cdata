#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


// This function copy filename to stdout
//
int copy_template (char* filename)
{
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("File not found\n");
        return 0;
    }

    char *line = NULL;
    size_t len = 0;
    size_t read;

    while ((read = getline(&line, &len, fp)) != -1) {
        printf("%s", line);
    }

    free(line);
    fclose(fp);
    return 0;
}


// This function copy filename to stdout, but replace a line with this pattern:
//   {DATA}
// by the string stored in the data argument
//
int sub_template (char* filename, char* data)
{
    FILE* fp = fopen(filename, "r");
    if (fp == NULL) {
        printf("File not found\n");
        return 0;
    }

    char *line = NULL;
    size_t len = 0;
    size_t read;

    while ((read = getline(&line, &len, fp)) != -1) {
        if (strcmp(line, "{DATA}\n") == 0) {
            // input line matches the pattern
            printf("%s", data);
        } else {
            printf("%s", line);
        }
    }

    free(line);
    fclose(fp);
    return 0;
}
