#include <time.h>
#include <netdb.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "list.h"
#include "tools.h"
#include "webserver.h"
#include "command_line_utils.h"


pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_exit = PTHREAD_COND_INITIALIZER;

long int total_bytes_served;
int pages_served;
int status;

int fd_serv;
int fd_comm;
int max_fd;

fd_set set;

void* webserver_thread()
{
    list_node_t *temp;

    while (1)
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&condition, &mutex);
        if ((temp = list_head) == NULL)
                continue;
        else
        {
            while (temp != NULL)
            {
                if (temp->type == SERVICE)
                {
                    status = read_service(temp->socket, temp->slot);
                    close(temp->socket);
                    FD_CLR(temp->socket, &set);
                    temp->socket = EMPTY;
                    temp->type = EMPTY;
                    if (status == BUSY)
                        break;
                }
                else
                    temp = temp->next;
                if (temp == NULL)
                    continue;
            }
        }
        if (status == SHUTDOWN)
        {
            printf("-THREAD-: I am thread %ld and "
                   "I will now exit. BYE...\n", pthread_self());
            pthread_cond_signal(&cond_exit);
            pthread_mutex_unlock(&mutex);
            break;
        }
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int server()
{
    int read_sockets, i, error, retval;
    pthread_t thread_id[get_num_of_threads()];
    list_node_t *temp;
    time_t start_time;

    time(&start_time);
    i = 0;
    fd_serv = initialize_server(get_serving_port());
    fd_comm = initialize_server(get_command_port());
    setnonblocking(fd_serv);                            //Set fd_serv O_NONBLOCK
    setnonblocking(fd_comm);                            //Set fd_comm O_NONBLOCK
    max_fd = max(fd_comm, fd_serv);                     //Get the max value for select usage

    for (i = 0; i < get_num_of_threads(); i++)          //Thread creation
    {
        error = pthread_create(&(thread_id[i]), 0, &webserver_thread, NULL);
        if (error != 0)
            printf("\ncan't create thread :[%s]", strerror(error));
    }

    while (1)
    {
        build_select_list();                            //Initialize set for select usage
        read_sockets = select (max_fd + 1, &set, NULL, NULL, NULL);
        if (read_sockets < 0)
        {
            perror ("select");
            exit (EXIT_FAILURE);
        }
        else
        {
            if (FD_ISSET(fd_serv, &set))                //Connection request on service socket.
            {
                if ((retval = handle_new_connection(fd_serv)) == BUSY)
                    continue;
            }
            else if (FD_ISSET(fd_comm, &set))           //Connection request on command socket.
            {
                if ((retval = handle_new_connection(fd_comm)) == BUSY)
                    continue;
            }
            pthread_mutex_lock(&mutex);
            if ((temp = get_list_head()) == NULL)
                    printf("NO ACTIVITY\n");
            else
            {
                while (temp != NULL)
                {
                    if (FD_ISSET(temp->socket,&set) && temp->type == COMMAND)
                    {                                   //Activity on already connected command socket
                        if (read_command(temp->socket, temp->slot, start_time) == SHUTDOWN)
                        {
                            FD_CLR(temp->socket, &set);
                            shutdown_server(thread_id);
                            return 0;
                        }
                    }                                   //Activity on already connected service socket
                    else if (FD_ISSET(temp->socket,&set) && temp->type == SERVICE)
                        pthread_cond_broadcast(&condition);

                    temp = temp->next;
                    if (temp == NULL)
                        break;
                }

            }
        }
        pthread_mutex_unlock(&mutex);
    }

    return 0;
}
