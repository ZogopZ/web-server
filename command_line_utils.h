#ifndef _COMMAND_LINE_UTILS_H_
#define _COMMAND_LINE_UTILS_H_

int get_serving_port(void);

int get_command_port(void);

int get_num_of_threads(void);

int get_arg_root_dir(void);

char* get_root_dir(void);

int parse_cli_args(int argc, char** argv);

#endif /* _COMMAND_LINE_UTILS_H_ */
