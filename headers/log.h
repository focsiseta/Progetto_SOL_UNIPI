//
// Created by focsiseta on 7/26/21.
//

#ifndef _LOG_H
#define _LOG_H

#include "server.h"
#include "utils.h"

int initLogfile(char * path);
void _log(const char *format, ...);
int closeLog(void);


#endif