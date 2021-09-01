#ifndef __PARSER
#define __PARSER

#include "utils.h"

typedef enum {
    FIFO,
    LRU
}cache_policy_t;

typedef struct conf{

    unsigned int Nslaves;       //Number of "workers" to be spawned
    size_t max_memory;          //How large is our cache in MBytes
    unsigned int max_files;     //How many files the server can contain
    char socket_path[MAXPATH];  //Path of the socket to be used
    char log_path[MAXPATH];     //Path of the log file
    cache_policy_t policy;

}config_t;

int get_config(const char * path);
void  printConfigFile(void);

#endif