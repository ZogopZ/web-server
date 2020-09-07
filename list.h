#ifndef _LIST_H_
#define _LIST_H_

#define CHECKED 100
#define NOT_CHECKED 101
#define ENLIST_SUCCESS 1
#define ENLIST_NO_SUCCESS 0

typedef struct list
{
    struct list *next;
    int slot;
    int type;
    int socket;
}list_node_t;

extern list_node_t *list_head;

list_node_t* get_list_head(void);

list_node_t* new_list_node(int, int);

int enlist(list_node_t*);

void free_list(void);


#endif /* _LIST_H_ */
