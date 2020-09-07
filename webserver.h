#ifndef _SERVER_H_
#define _SERVER_H_

#define SHUTDOWN -111
#define BUSY -99
#define COMMAND -333
#define SERVICE -444
#define EMPTY -222
#define OK 200
#define NOT_FOUND 404
#define FORBIDDEN 403
#define MAXMSG  1024

extern pthread_mutex_t mutex;
extern pthread_cond_t condition;
extern pthread_cond_t cond_exit;

extern long int total_bytes_served;
extern int pages_served;
extern int status;

extern int fd_serv;
extern int fd_comm;
extern int max_fd;

extern fd_set set;

int server();

#endif /* _SERVER_H_ */
