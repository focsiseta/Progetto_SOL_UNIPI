//
// Created by focsiseta on 7/26/21.
//
#include "../headers/utils.h"


void safe_pthread_mutex_lock(pthread_mutex_t* mtx){
    int err;
    if( (err = pthread_mutex_lock(mtx)) != 0){
        errno = err;
        perror("Error in pthread_mutex_lock");
        exit(EXIT_FAILURE);
    }
}

void safe_pthread_create(pthread_t tID,void *(* __start_routine)(void *),void *arg,int doDetach){
    int err = pthread_create(&tID,NULL,__start_routine,arg);
    if (err != 0){
        fprintf(stderr,"Can't create thread\n");
        exit(EXIT_FAILURE);
    }
    if (doDetach == 1){
        err = pthread_detach(tID);
        if (err != 0){
            fprintf(stderr,"Can't deatch thread\n");
            exit(EXIT_FAILURE);
        }
    }
}

void safe_pthread_mutex_unlock(pthread_mutex_t* mtx){
    int err;
    if( (err = pthread_mutex_unlock(mtx)) != 0){
        errno = err;
        perror("Error in pthread_mutex_unlock");
        exit(EXIT_FAILURE);
    }
}

void safe_pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mtx){
    int err;
    if( (err = pthread_cond_wait(cond, mtx)) != 0){
        errno = err;
        perror("Error in pthread_cond_wait");
        exit(EXIT_FAILURE);
    }
}

void safe_pthread_cond_signal(pthread_cond_t* cond){
    int err;
    if( (err = pthread_cond_signal(cond)) != 0){
        errno = err;
        perror("Error in pthread_cond_signal");
        exit(EXIT_FAILURE);
    }
}

void safe_pthread_cond_broadcast(pthread_cond_t* cond){
    int err;
    if( (err = pthread_cond_broadcast(cond)) != 0){
        errno = err;
        perror("Error in pthread_cond_broadcast");
        exit(EXIT_FAILURE);
    }
}

void* safe_malloc(size_t size){
    void* buf;
    if( (buf = malloc(size)) == NULL){
        perror("Error in memory allocation (malloc)");
        exit(EXIT_FAILURE);
    }
    //setting all to 0 just to be careful
    memset(buf,0,size);
    return buf;
}

void* safe_calloc(size_t nmemb, size_t size){
    void* buf;
    if( (buf = calloc(nmemb, size)) == NULL){
        perror("Error in memory allocation (calloc)");
        exit(EXIT_FAILURE);
    }
    return buf;
}

void* safe_realloc(void* ptr, size_t size){
    void* buf;
    if( (buf = realloc(ptr, size)) == NULL){
        perror("Error in memory allocation (realloc)");
        exit(EXIT_FAILURE);
    }
    return buf;
}

ssize_t  /* Read "n" bytes from a descriptor */
readn(int fd, void *ptr, size_t n) {
    size_t   nleft;
    ssize_t  nread;

    nleft = n;
    while (nleft > 0) {
        if((nread = read(fd, ptr, nleft)) < 0) {
            if (nleft == n) return -1; /* error, return -1 */
            else break; /* error, return amount read so far */
        } else if (nread == 0) break; /* EOF */
        nleft -= nread;
        ptr   += nread;
    }
    return(n - nleft); /* return >= 0 */
}

ssize_t  /* Write "n" bytes to a descriptor */
writen(int fd, void *ptr, size_t n) {
    size_t   nleft;
    ssize_t  nwritten;

    nleft = n;
    while (nleft > 0) {
        if((nwritten = write(fd, ptr, nleft)) < 0) {
            if (nleft == n) return -1; /* error, return -1 */
            else break; /* error, return amount written so far */
        } else if (nwritten == 0) break;
        nleft -= nwritten;
        ptr   += nwritten;
    }
    return(n - nleft); /* return >= 0 */
}


