#include "../headers/server_utils.h"
#include "../headers/utils.h"
#include "../headers/queue.h"

List * initList(void){
    List * leest = (List *) safe_malloc(sizeof(List));
    int err = pthread_cond_init(&(leest->listCond),NULL);
    if (err != 0){
        errno = err;
        perror("Error cond_init :");
        exit(EXIT_FAILURE);
    }
    err = pthread_mutex_init(&(leest->listLock),NULL);
    if (err != 0){
        errno = err;
        perror("Error mutex_init");
        exit(EXIT_FAILURE);
    }
    leest->number_of_nodes = 0;
    leest->list_head = NULL;
    leest->tail = NULL;
    return leest;
}

//If elements gets added without problem returns 0 else -1, head first insert
int push_node_to_list(List * list, Node * element){ 

    if (list == NULL){
        fprintf(stderr,("Passed a null pointer to add_node_to_list as list\n"));
        return -1;
    }
    if (element == NULL){
        fprintf(stderr,("Passed a null pointer to add_node_to_list as element\n"));
        return -1;
    }
    lock(&(list->listLock));
    //if first element
    if (list->list_head == NULL || (list->number_of_nodes == 0)){
        element->prev = NULL;
        element->next = NULL;
        list->list_head = element;
        list->tail = element;
        list->number_of_nodes += 1;
        signal(&(list->listCond));
        unlock(&(list->listLock));
        return 0;
    }
    //Setup node
    Node * complete_list = list->list_head;
    element->prev = NULL;
    element->next = complete_list;
    complete_list->prev = element;
    list->list_head = element;
    list->number_of_nodes += 1;
    signal(&(list->listCond)); //Wakes up one of many if there are more than one
    unlock(&(list->listLock));
    return 0;

}
//returns null if list is null
Node * pop_element_from_tail(List * list){
    if (list == NULL){
        fprintf(stderr,"Passed null to pop_from_element as list!\n");
        return NULL;
    }
    lock(&(list->listLock));

    while(list->number_of_nodes == 0) {
        //fprintf(stderr,"Waiting on queue");
        wait(&(list->listCond), &(list->listLock));
    }
    //fprintf(stderr,"Thing recived\n");
    if (list->tail == NULL && list->list_head == NULL){
        unlock(&(list->listLock));
        fprintf(stderr,"Something really wrong has been going on!\n");
        exit(EXIT_FAILURE);
    }
    Node * toReturn = list->tail;

    //If there is only one element
    if (list->number_of_nodes == 1){

        toReturn = list->tail; //Address of the tail

        //Nulling just for safety
        list->tail->prev = NULL;
        list->tail->next = NULL;
        list->list_head = NULL;
        list->tail = NULL;
        list->number_of_nodes -= 1;
        unlock(&(list->listLock));
        return toReturn;
    }
    //If there are more than one
    toReturn = list->tail;
    list->tail = toReturn->prev;
    list->tail->next = NULL;
    list->number_of_nodes -= 1;
    toReturn->prev = NULL;
    toReturn->next = NULL;
    unlock(&(list->listLock));
    return toReturn;

}
//Returns null if obj passed is null
Node * create_node(void * obj) {
    if (obj == NULL) {
        fprintf(stderr, "Passed obj is null!\n");
        return NULL;
    }
    Node *node = (Node *) safe_malloc(sizeof(Node));
    node->obj = obj;
    node->prev = NULL;
    node->next = NULL;
    return node;
}

void wake_up_all_readers(List * list){
    lock(&(list->listLock));
    broadcast(&(list->listCond));
    unlock(&(list->listLock));
}
















