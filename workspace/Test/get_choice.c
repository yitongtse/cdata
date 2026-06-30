#include <stdio.h>
#include <stdlib.h>
#include <strings.h>


const char *choices[] = {
  "Apple",
  "Orange",
  "Banana",
  ""                    // end of choices
};


int find_choice (const char *item, const char *choice[])
{
    int i = 0;
    while (strlen(choice[i]) > 0) {
        if (!strcmp(item, choice[i])) {
            return i;
        }
        i++;
    }
    return -1;
}


int main()
{
    printf("%s: %d\n", "Orange", find_choice("Orange", choices));
    printf("%s: %d\n", "Pear", find_choice("Pear", choices));
}
