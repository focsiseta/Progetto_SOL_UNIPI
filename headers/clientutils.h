
#ifndef _CLIENTUTILS_H
#define _CLIENTUTILS_H

#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>
#include <getopt.h>
#include <time.h>
#include <fcntl.h>
#include <getopt.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../headers/prot.h"
#include "../headers/utils.h"

#define DEFAULT_TIME 1

typedef enum {
    FIFO,
    LRU
}cache_policy_t;

typedef struct conf{

    unsigned int Nslaves;       //Number of "workers" to be spawned
    size_t max_memory;          //How large is our cache in MBytes
    unsigned int max_files;     //How many files the server can contain
    char socket_path[256];  //Path of the socket to be used
    char log_path[256];     //Path of the log file
    cache_policy_t policy;

}config_t;

off_t file_size(const char * filename);
int send_req(int op,const char * filename,int flags);
int visitDir(char *pathname, char *fileList[],int * acc);
int addToCurrentTime(long sec, long nsec, struct timespec* res);
int get_resp(void);
int openConnection(const char * sockname,int msec,const struct timespec abstime);
int openFile(const char *pathname,int flags);
int readNFiles(int N, const char * dirname);
int closeConnection(const char *pathname);
int readFile(const char * pathname,void **buf,size_t * size);
int writeFile(const char* pathname,const char * dirname);
int appendToFile(const char * pathname,void *buf,size_t size,const char *dirname);
int removeFile(const char* pathname);
int closeFile(char *pathname);
extern int serverfd;


typedef struct action{

    char op;
    char *singleArg;
    char **args; //MAXREADBLEFILE come numero massimo di file che possono essere indicati negli argomenti
    int n;
    struct action *next;

}Action;

#define next_Action do{ \
    action = action->next;\
}while(0)

#define nullcheck(pointer) if (pointer == NULL) {exit(EXIT_FAILURE);}

Action * parseArgs(int argc, const char *argv[]);
Action * initAction(const char **args,int n,char op,char * singleArg);
void printActions(Action *head);
Action * lookup(Action * head,char op);
void add_to_tail(Action **head,Action * ne);
int setFromMsec(long msec, struct timespec* req);


#define OPEN    "\x1b[35m openFile/createFile \x1b[0m"
#define READ    "\x1b[35m readFile \x1b[0m"
#define WRITE   "\x1b[35m writeFile \x1b[0m"
#define READN   "\x1b[35m readNFiles \x1b[0m"
#define APPEND  "\x1b[35m appendToFile \x1b[0m"
#define REMOVE  "\x1b[35m removeFile \x1b[0m"
#define CLOSE   "\x1b[35m closeFile \x1b[0m"
#define GOOD     "\x1b[32m Positive \x1b[0m"//Tipo di esito
#define BAD      "\x1b[31m Negative \x1b[0m"

#define MAXREADABLEFILE 1024
#endif
