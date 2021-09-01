#ifndef _SERVER_H
#define _SERVER_H

#include "utils.h"
#include "parser.h"
#include "server_utils.h"
#include "memory.h"

#define END_WORKER 0x987






//Visible to everyone
extern FILE * logFile;
extern pthread_mutex_t lockLogFile;
extern config_t configFile;
extern Server_metadata serverMetadata;



#endif
