
#include "../headers/server.h"
#include "../headers/utils.h"

pthread_mutex_t workerCounterLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t waitOnworkers = PTHREAD_COND_INITIALIZER;

unsigned int workerCounter = 0;

void worker_add(void){
    lock(&workerCounterLock);
    workerCounter++;
    unlock(&workerCounterLock);
}
void worker_sub(void){
    lock(&workerCounterLock);
    if (workerCounter == 0){
        signal(&waitOnworkers);
        unlock(&workerCounterLock);
        return;
    }
    workerCounter--;
    if (workerCounter == 0){
        signal(&waitOnworkers);
    }
    unlock(&workerCounterLock);
}

int get_worker_number(void){
    lock(&workerCounterLock);
    int counter = workerCounter;
    unlock(&workerCounterLock);
    return counter;
}

void wait_on_worker_number(void){
    lock(&workerCounterLock);
    while(workerCounter != 0){
        wait(&waitOnworkers,&workerCounterLock);
    }
    unlock(&workerCounterLock);
}
void block_PIPE_SIGINT_SIGQUIT_SIGUP(sigset_t set, sigset_t old){

    int err = sigemptyset(&set);
    check_under_zero(err,"error zeroing set\n");
    err = sigaddset(&set,SIGQUIT);
    check_under_zero(err,"Error setting sigquit\n");
    err = sigaddset(&set,SIGINT);
    check_under_zero(err,"Error setting sigint\n");
    err = sigaddset(&set,SIGHUP);
    check_under_zero(err,"Error setting sighup\n");
    err = sigaddset(&set,SIGPIPE);
    check_under_zero(err,"Error setting sigpipe\n");
    err = pthread_sigmask(SIG_BLOCK,&set,&old);
    check_under_zero(err,"Error setting sigmask\n");

}

void add_SIGPIPE_BLOCK(sigset_t mask){
    int err = sigaddset(&mask,SIGPIPE);
    check_under_zero(err,"Error setting sigpipe\n");
}

