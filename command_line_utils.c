#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tools.h"
#include "command_line_utils.h"

int serving_port;
int command_port;
int num_of_threads;
int arg_root_dir;
char* root_dir;

int parse_cli_args(int argc, char** argv)
{

    if (argc != 9)
    {
        printf("Error, too few arguments ...\n");
        return 1;
    }
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-p") == 0)
            serving_port = atoi(argv[i+1]);
        else if (strcmp(argv[i], "-c") == 0)
            command_port = atoi(argv[i+1]);
        else if (strcmp(argv[i], "-t") == 0)
            num_of_threads = atoi(argv[i+1]);
        else if (strcmp(argv[i], "-d") == 0)
        {
            arg_root_dir = i+1;
            root_dir = mallocNcheck(strlen(argv[arg_root_dir]) + 1);
            strcpy(root_dir, argv[arg_root_dir]);
        }
    }

return 0;
}

int get_serving_port(void)
{
    return (serving_port);
}
int get_command_port(void)
{
    return (command_port);
}
int get_num_of_threads(void)
{
    return (num_of_threads);
}
int get_arg_root_dir(void)
{
    return (arg_root_dir);
}
char* get_root_dir(void)
{
    return root_dir;
}

