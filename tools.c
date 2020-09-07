#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "list.h"
#include "tools.h"
#include "webserver.h"
#include "command_line_utils.h"

int read_command(int new_comm, int slot, time_t start_time)
{
    int hours, minutes, seconds;
    char message[10];
    time_t current_time, diff;

    memset(message, 0 , 10);

    read(new_comm, &message, 10);
    if ((strcmp(message, "SHUTDOWN\n") == 0) || (strcmp(message, "shutdown\n") == 0))
    {
        write(new_comm, "Message from Server: Server will now shut down...\n", 51);
        close(new_comm);
        close(fd_comm);
        return SHUTDOWN;
    }
    else if ((strcmp(message, "STATS\n") == 0) || (strcmp(message, "stats\n") == 0))
    {
        printf("############################################################\n");
        printf("#          Connection accepted:   FD: %2d in slot: %2d       #\n",
            new_comm, slot);
        printf("############################################################\n");
        time(&current_time);
        diff = current_time - start_time;
        hours = diff/3600; minutes = (diff-hours*3600)/60; seconds = (diff-hours*3600-minutes*60);
        printf("Server up for %.2d:%.2d.%.2d, served pages:%d, total bytes:%ld\n", hours, minutes, seconds, pages_served, total_bytes_served);
        printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
    }
    else
        printf("%s", message);
    return 0;
}

int read_service(int new_serv, int slot)
{
    int bytes_read;
    char message[MAXMSG];
    char c;

    bytes_read = 0;
    memset(message, 0 , MAXMSG);

    bytes_read = read(new_serv, &message, MAXMSG);
    if (bytes_read == 0)
        return BUSY;            //No link read, or user is hovering
    else if (bytes_read == MAXMSG && (read(new_serv, &c, 1) == 1))
    {                           //Link has suspiciously large size
	printf("############################################################\n");
        printf("#          Connection accepted: FD: %2d in slot: %2d        #\n",
            new_serv, slot);
        printf("############################################################\n");
        createNsend_message(new_serv, BUSY, message);
        while((bytes_read = read(new_serv, &message, MAXMSG)) > 0);
        printf("-THREAD-: I am thread %ld.\t"
               "Request of invalid size\n", pthread_self());
	printf("-THREAD-: I am thread %ld and I just finished a job!\n", pthread_self());
	printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
        return BUSY;
    }
    printf("############################################################\n");
    printf("#          Connection accepted: FD: %2d in slot: %2d         #\n",
        new_serv, slot);
    printf("############################################################\n");
    printf("%s\t\t\t---\n", message);
    getNstat_file(new_serv, message);
    printf("-THREAD-: I am thread %ld and I just finished a job!\n", pthread_self());
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
    return OK;
}

int handle_new_connection(int socket)
{
    int new_connection;
    list_node_t *neos;

    new_connection = accept(socket, NULL, NULL);
    if (new_connection < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }
    setnonblocking(new_connection);                 //Set new_connection O_NONBLOCK
    pthread_mutex_lock(&mutex);
    neos = new_list_node(new_connection, socket);
    if (enlist(neos) == ENLIST_NO_SUCCESS)          //Add to list
    {
        printf("############################################################\n");
        printf("#           Connection refused. Server is busy             #\n");
        close(new_connection);
        pthread_mutex_unlock(&mutex);
        return BUSY;
    }
    pthread_mutex_unlock(&mutex);
    return OK;
}

void build_select_list(void)
{
    list_node_t *temp;

    FD_ZERO(&set);
    FD_SET(fd_serv, &set);
    FD_SET(fd_comm, &set);

    pthread_mutex_lock(&mutex);

    if ((temp = get_list_head()) != NULL)
    {
        FD_SET(temp->socket, &set);
        max_fd = max(max_fd, temp->socket);
        while (temp->next != NULL)
        {
            FD_SET(temp->next->socket, &set);
            max_fd = max(max_fd, temp->next->socket);
            temp = temp->next;
        }
    }
    pthread_mutex_unlock(&mutex);

    return;
}

void getNstat_file(int new_serv, char message[MAXMSG])
{
    int i, j;
    char *file_path, *half_path;
    struct stat fileStat;

    j = 0;

    for (i = 0; i < MAXMSG; i++)
    {
        j++;
        if (message[i] == '\n')
        {                       //Get size of link from first line of message
            file_path = mallocNcheck(i - 13 + (int) strlen(get_root_dir()));
            half_path = mallocNcheck(i - 13);
            memset(file_path, 0, i - 13 + (int) strlen(get_root_dir()));
            memset(half_path, 0, i - 13);
            i = 0;
            break;
        }
    }                           //Get link from first line of message
    snprintf(half_path, j - 14, "%s", &message[i + 4]);
    sprintf(file_path, "%s%s", get_root_dir(), half_path);
    if (stat(file_path,&fileStat) == 0 &&
            (fileStat.st_mode & S_IRUSR) && (fileStat.st_mode & S_IROTH))
    {                           //Link exists in root_directory and is readable
        printf("SUCCESS\n");
        createNsend_message(new_serv, OK, file_path);
        free(file_path);
        free(half_path);
    }
    else if (stat(file_path,&fileStat) == 0 &&
             (!(fileStat.st_mode & S_IRUSR) || (fileStat.st_mode & S_IROTH)))
    {                           //Link exists in root_directory but is not readable
        printf("ERROR: FORBIDDEN FILE\n");
        createNsend_message(new_serv, FORBIDDEN, file_path);
        free(file_path);
        free(half_path);

    }
    else if (stat(file_path,&fileStat) < 0)
    {                           //Link doesn't exist in root_directory
        printf("ERROR: FILE DOESN'T EXIST\n");
        createNsend_message(new_serv, NOT_FOUND, file_path);
        free(file_path);
        free(half_path);
    }
    else
        puts("NOTHING");

    return;
}

void createNsend_message(int new_serv, int status, char* file_path)
{
    long int message_size;
    FILE *fp_file;
    char *message, *file_content, date[38];
    struct tm *time_info;
    struct stat fileStat;
    time_t current_time;

    time(&current_time);
    time_info = localtime(&current_time);
    stat(file_path,&fileStat);
    message_size = 0;
    memset(date, 0, 39);

    if (status == OK)
    {
        message_size = 17+38+34+18+25+20+2+8+fileStat.st_size+1;
        total_bytes_served = total_bytes_served + message_size;
        pages_served++;

        message = mallocNcheck(message_size);
        file_content = mallocNcheck(fileStat.st_size);
        memset(message, 0, message_size);
        memset(file_content, 0, fileStat.st_size);

        sprintf(message, "HTTP/1.1 200 OK\r\n");
        strftime(date, 38, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", time_info);
        sprintf(message + strlen(message), "%s", date);
        sprintf(message + strlen(message),
                         "Server: myhttpd/1.0.0 (Ubuntu64)\r\n"
                         "Content-Length: %ld\r\n"
                         "Content-Type: text/html\r\n"
                         "Connection: Closed\r\n\r\n", fileStat.st_size);
        printf("%s\n", message);
        if((fp_file = fopen(file_path, "r")) == NULL)
        {
            printf("can not open file \n");
            exit(EXIT_FAILURE);
        }
        fread(file_content, fileStat.st_size, 1, fp_file);
        memcpy(&message[strlen(message)], file_content, fileStat.st_size);
        fclose(fp_file);
        free(file_content);
    }
    else if (status == NOT_FOUND)
    {
        message_size = 24+38+34+18+25+20+2+8+49;
        total_bytes_served = total_bytes_served + message_size;

        message = mallocNcheck(message_size);
        memset(message, 0, message_size);

        sprintf(message, "HTTP/1.1 404 Not Found\r\n");
        strftime(date, 38, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", time_info);
        sprintf(message + strlen(message), "%s", date);
        sprintf(message + strlen(message), "Server: myhttpd/1.0.0 (Ubuntu64)\r\n"
                         "Content-Length: 49\r\n"
                         "Content-Type: text/html\r\n"
                         "Connection: Closed\r\n\r\n"
                         "<html>Sorry dude, couldn't find this file.</html>");
        printf("%s\n", message);
    }
    else if (status == FORBIDDEN)
    {
        message_size = 24+38+34+18+25+20+2+8+70;
        total_bytes_served = total_bytes_served + message_size;

        message = mallocNcheck(message_size);
        memset(message, 0, message_size);

        sprintf(message, "HTTP/1.1 403 Forbidden\r\n");
        strftime(date, 38, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", time_info);
        sprintf(message + strlen(message), "%s", date);
        sprintf(message + strlen(message), "Server: myhttpd/1.0.0 (Ubuntu64)\r\n"
                         "Content-Length: 70\r\n"
                         "Content-Type: text/html\r\n-1"
                         "Connection: Closed\r\n\r\n"
                         "<html>Trying to access this file but don't think I can make it.</html>");
        printf("%s\n", message);
    }
    else if (status == BUSY)
    {
        message_size = 19+38+34+18+25+20+2+8+MAXMSG;

        message = mallocNcheck(message_size);
        memset(message, 0, message_size);

        sprintf(message, "HTTP/1.1 -99 BUSY\r\n");
        strftime(date, 38, "Date: %a, %d %b %Y %H:%M:%S GMT\r\n", time_info);
        sprintf(message + strlen(message), "%s",date);
        sprintf(message + strlen(message), "Server: myhttpd/1.0.0 (Ubuntu64)\r\n"
                         "Content-Length: > MAXMSG\r\n"
                         "Content-Type: text/html\r\n"
                         "Connection: Closed\r\n\r\n"
                         "<html>Warning suspicious link.</html>");
        printf("%s\n", message);
    }
    write(new_serv, message, message_size);    
    free(message);
    return;
}

int initialize_server(int port)
{
    struct sockaddr_in server_addr;
    int fd_temp;

    int on = 1;

    fd_temp = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_temp < 0)
    {
        perror("socket");
        exit(1);
    }
    setsockopt(fd_temp, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(int));    //Set reusability of fd_temp
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);
    if (bind(fd_temp,(struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind");
        close(fd_temp);
        exit(1);
    }
    if (listen(fd_temp, 10) == -1)
    {
        perror("listen");
        close(fd_temp);
        exit(1);
    }
    return fd_temp;
}

void shutdown_server(pthread_t* thread_id)
{
    int i;

    i = 0;
    status = SHUTDOWN;

    while (i < get_num_of_threads())
    {
        pthread_cond_signal(&condition);
        pthread_cond_wait(&cond_exit, &mutex);
        i++;
    }
    printf("~SERVER~: All threads exited\n");
    free_list();
    pthread_mutex_unlock(&mutex);
    printf("~SERVER~: All connections closed\n");
    for (i = 0; i < get_num_of_threads(); i++)
    {
        printf("~SERVER~: Thread %ld will now join...\n", thread_id[i]);
        pthread_join(thread_id[i], NULL);
    }
    printf("~SERVER~: Destroying mutexes and condition variables...\n");
    pthread_cond_destroy(&cond_exit);
    pthread_cond_destroy(&condition);
    pthread_mutex_destroy(&mutex);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
}

int max(int fd_1, int fd_2)
{
    int max_fd = 0;

    if (fd_1 > fd_2)
        max_fd = fd_1;
    else if (fd_1 < fd_2)
        max_fd = fd_2;
    else
    {
        max_fd = fd_1;
    }
    return max_fd;
}

void* mallocNcheck(size_t incoming_size)
{
    void *ptr;

    ptr = malloc(incoming_size);
    if (ptr == NULL && incoming_size > 0)
    {
        printf("Not enough memory in heap\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void setnonblocking(int sock)
{
    int opts;

    opts = fcntl(sock,F_GETFL);
    if (opts < 0) {
        perror("fcntl(F_GETFL)");
        exit(EXIT_FAILURE);
    }
    opts = (opts | O_NONBLOCK);
    if (fcntl(sock,F_SETFL,opts) < 0)
    {
        perror("fcntl(F_SETFL)");
        exit(EXIT_FAILURE);
    }

    return;
}
