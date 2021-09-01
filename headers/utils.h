
#ifndef SOLPRGAGAIN_UTILS_H
#define SOLPRGAGAIN_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>

#include <errno.h>
#include <ctype.h>

#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include <limits.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/un.h>



void safe_pthread_mutex_lock(pthread_mutex_t *mtx);
void safe_pthread_mutex_unlock(pthread_mutex_t *mtx);
void safe_pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mtx);
void safe_pthread_cond_signal(pthread_cond_t* cond);
void safe_pthread_cond_broadcast(pthread_cond_t* cond);
void safe_pthread_create(pthread_t tID,void *(* __start_routine)(void *),void *arg,int doDetach);
void* safe_malloc(size_t size);
void* safe_calloc(size_t nmemb, size_t size);
void* safe_realloc(void* ptr, size_t size);
ssize_t readn(int fd, void *buf, size_t size);
ssize_t writen(int fd, void *buf, size_t size);

#define lock(lck)       safe_pthread_mutex_lock(lck)
#define unlock(lck)     safe_pthread_mutex_unlock(lck)
#define wait(cond,lck)  safe_pthread_cond_wait(cond,lck)
#define signal(cond)    safe_pthread_cond_signal(cond)
#define broadcast(cond) safe_pthread_cond_broadcast(cond)
#define safe_free(pointer) if (pointer != NULL) {free(pointer); pointer = NULL;} // setting to null avoid double frees


//Common flags
#define MAXPATH 24
#define MAXCONFS 6
#define MAXNAME 256
#define MAXFILES 1024






// OP Codes for server and client





#endif
