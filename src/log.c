
#include "../headers/log.h"
#include "../headers/server.h"




//Return -1 if errors happen during opening, returns 0 if no errors
int initLogfile(char *path){
    if (path == NULL){
        return -1;
    }
    if ((logFile = fopen(path, "w")) == NULL){
        fprintf(stderr,"Cannot create logfile.\n");
        return -1;
    }
    return 0;
}

void _log(const char *format, ...){

    va_list arg_list;
    va_start(arg_list,format);

    lock(&lockLogFile);
    if (vfprintf(logFile,format,arg_list) == -1){
        fprintf(stderr,"Error writing inside log\n");
        return;
    }
    fflush(logFile);
    unlock(&lockLogFile);

}
//Returns -1 if errors
int closeLog(void){
    int err;
    if ((err = fclose(logFile)) == EOF){
        fprintf(stderr,"Error during fclose() of the log file\n");
        perror("Error -> :");
        return -1;
    }

    return 0;
}

