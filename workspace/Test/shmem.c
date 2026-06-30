#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <sys/mman.h>


char line[128];
char *buf;

int main (int argc, char** argv)
{
    int fd = shm_open("Test", O_RDWR | O_CREAT, 0666);
    assert(fd >= 0);

    lseek(fd, 0, SEEK_END);
    sprintf(line, "Hello World!\n");
    write(fd, line, sizeof(line));

    buf = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    assert(buf);

    printf("Old content: %s", buf);

    sprintf(buf, "Test\n");
    printf("New content: %s", buf);

    close(fd);

    return 0;
}
