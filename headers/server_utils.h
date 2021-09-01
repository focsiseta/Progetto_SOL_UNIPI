#ifndef _SERVER_UTILS_H
#define _SERVER_UTILS_H

#include "server.h"
#include <signal.h>

#define check_under_zero(call,msg) if ((call) < 0){fprintf(stderr,msg); perror("Error:"); exit(EXIT_FAILURE);}
extern volatile sig_atomic_t serverMode;

void worker_add(void);
void worker_sub(void);
void wait_on_worker_number(void);
void block_PIPE_SIGINT_SIGQUIT_SIGUP(sigset_t set, sigset_t old);
void add_SIGPIPE_BLOCK(sigset_t mask);
void handle_graceful(int sig);
void handle_shutdown(int sig);

#define ON 0x01
#define OFF 0x00
#define GRACEFUL 0x02


#endif
