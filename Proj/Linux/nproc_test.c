#include <stdio.h>
#include <sys/sysinfo.h>

void main (int argc, char **argv)
{
    printf("Avail cores: %d\n", get_nprocs());
    printf("Config cores: %d\n", get_nprocs_conf());
}
