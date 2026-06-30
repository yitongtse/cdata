#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>


char file1[] = "test1.txt";
char file2[] = "test2.txt";
char file[] = "test.txt";


int main()
{
    int rc;
    int ver = 0;
    FILE *fp;

    while (1) {
       // Update 1st file
       fp = fopen(file1, "w+");
       assert(fp);
       fprintf(fp, "File 1: ver %d\n", ver++);
       fclose(fp);

       printf("Linking to first file...\n");
       rc = unlink(file);
       if (rc == -1) {
          printf("Failed: %d\n", errno);
       }
       rc = symlink(file1, file);
       if (rc == -1) {
          printf("Failed: %d\n", errno);
       }
       //sleep(1);


       // Update 2nd file
       fp = fopen(file2, "w+");
       assert(fp);
       fprintf(fp, "File 2: ver %d\n", ver++);
       fclose(fp);

       printf("Linking to second file...\n");
       rc = unlink(file);
       if (rc == -1) {
          printf("Failed: %d\n", errno);
       }
       rc = symlink(file2, file);
       if (rc == -1) {
          printf("Failed: %d\n", errno);
       }
       //sleep(1);
    }
}
