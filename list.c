#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "tools.h"
#include "list.h"
#include "webserver.h"

list_node_t *list_head = NULL;

list_node_t* get_list_head(void)
{
    return list_head;
}

list_node_t* new_list_node(int new_connection, int socket)
{
    list_node_t *neos;

    neos = (list_node_t*) mallocNcheck(sizeof(list_node_t));
    memset(neos, 0, sizeof(list_node_t));
    neos->socket = new_connection;
    if (socket == fd_comm)                      //New connection in command port.
        neos->type = COMMAND;
    else if (socket == fd_serv)                 //New connection in service port.
        neos->type = SERVICE;
    neos->next = NULL;
    return neos;
}

int enlist(list_node_t *neos)
{
    list_node_t *list_temp;

    list_temp = list_head;

    if (list_head == NULL || list_head->type == EMPTY)
    {
        if (list_head == NULL)                  //If list is empty
        {
            list_head = neos;
            list_head->slot = 1;
        }
        else if (list_head->type == EMPTY)      //If list is not empty, but
        {                                       //list_head was previously used
            list_head->type = neos->type;       //and is now available for reuse
            list_head->socket = neos->socket;
            free(neos);
        }
        return ENLIST_SUCCESS;
    }
    else
    {
        while (list_temp->next != NULL)
        {
            if (list_temp->next->type == EMPTY)
            {
                list_temp->next->type = neos->type;
                list_temp->next->socket = neos->socket;
                free(neos);
                return ENLIST_SUCCESS;          //enlist new connection in previously used slot, which is now empty.
            }
            list_temp = list_temp->next;
        }
       list_temp->next = neos;
       list_temp->next->slot = list_temp->slot + 1;
       return ENLIST_SUCCESS;                   //enlist new connection in new slot
    }
    return ENLIST_NO_SUCCESS;
}

void free_list(void)
{
    list_node_t *temp;

    if (list_head != NULL)
    {
        while (1)
        {
            temp = list_head;
            if (temp->next != NULL)
            {
                list_head = temp->next;
                if (temp->type != EMPTY)        //Check for not closed file descriptors
                {                               //due to SHUTDOWN
                    close(temp->socket);
                    printf("~SERVER~: Closing connection in slot:%d\n", temp->slot);
                }
                free(temp);
            }
            else
            {
                if (temp->type != EMPTY)        //Check for not closed file descriptors
                {                               //due to SHUTDOWN
                    close(temp->socket);
                    printf("~SERVER~: Closing connection in slot:%d\n", temp->slot);
                }
                free(temp);
                break;
            }
        }
    }
    return;
}
