#include <stdio.h>
#include <stdlib.h>


typedef struct {
   char code[3];
   char state[29];
} record_t;


//char code[][3] = {"CA", "NC", "AZ", "NY"};
//char *state[] = {"California", "North Carolina", "Arizona", "New York"};

record_t record[4] = {"CA", "California", "NC", "North Carolina",
                      "AZ", "Arizona", "NY", "New York"};

int main (int argc, char** argv)
{
    int i;

    for (i=0; i<4; i++) {
        printf("%d: %s %s\n", i, record[i].code, record[i].state);
    }
}
