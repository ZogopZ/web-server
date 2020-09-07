#include <stdio.h>
#include <stdlib.h>

#include "command_line_utils.h"
#include "webserver.h"



int main(int argc, char **argv)
{


    if (parse_cli_args(argc, argv) == 1)
            exit(EXIT_FAILURE);

    server();
    free(get_root_dir());
    return 0;
}
