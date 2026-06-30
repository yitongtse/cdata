#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <fcntl.h>
#include <errno.h>
#include "../cli.h"


/* CLIs
show name <num_time>
exit
*/


enum {
    CLI_SHOW_NAME,
    CLI_EXIT,
    CLI_LAST
};


// Prototypes
int cli_show_name(cli_control_block *ccb);
int cli_exit(cli_control_block *ccb);


//// CLI Parser

CLI_ARR_START(cli_elem)

CLI_ARR_STR (0, show,
             CLI_NULL(),
             0, "Show command")

CLI_ARR_STR (1, name,
             CLI_NULL(),
             0, "Show program name")

CLI_ARR_NUM (2, 1, 10,
             CLI_FUN(cli_show_name, CLI_SHOW_NAME),
             0, "Number of times to show (1 - 10)")

CLI_ARR_STR (0, exit,
             CLI_FUN(cli_exit, CLI_EXIT),
             0, "Exit")

CLI_ARR_END

//// End of CLI parser


char *prog_name;


int cli_show_name (cli_control_block *ccb)
{
    int i;

    cli_prepare_cc(ccb);

    for (i = 0; i < ccb->var[2]; i++) {
        printf("Program name: %s\n", prog_name);
    }

    return 0;
}


int cli_exit (cli_control_block *ccb)
{
    cli_prepare_cc(ccb);
    cli_main_loop_end();
    return 0;
}


int main (int argc, char **argv)
{
    int ret_val = 0;

    prog_name = argv[0];

    // Initialize internal cli components
    cli_init();
    cli_set_prompt("#> ");

    // Add generic CLIs
    cli_add_elems(cli_elem);

    // Main loop
    ret_val = cli_main_loop();

    return (ret_val);
}

