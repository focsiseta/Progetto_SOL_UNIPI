//
// Created by focsiseta on 7/26/21.
//

#ifndef _QUEUE_H
#define _QUEUE_H

#include "server_utils.h"
#include "utils.h"


typedef struct node {

    struct node *next;
    struct node *prev;
    void * obj;

}Node;


typedef struct __list_metadata_lock{
    pthread_cond_t listCond;
    pthread_mutex_t listLock;
    unsigned int number_of_nodes;
    Node * tail;
    Node * list_head;
}List;

List * initList(void);
int push_node_to_list(List * list, Node * element);
Node * pop_element_from_tail(List * list);
Node * pop_element_from_tail_worker_thrd(List * list);
Node * create_node(void * obj);
void wake_up_all_readers(List * list);

#endif
