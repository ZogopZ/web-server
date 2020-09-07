#ifndef _TOOLS_H_
#define _TOOLS_H_

int read_command(int, int, time_t);

int read_service(int, int);

int handle_new_connection(int);

void build_select_list(void);

void getNstat_file(int, char*);

void createNsend_message(int, int, char*);

int initialize_server(int);

void shutdown_server(pthread_t*);

int max(int, int);

void* mallocNcheck(size_t);

void setnonblocking(int sock);

#endif /* _TOOLS_H_ */
