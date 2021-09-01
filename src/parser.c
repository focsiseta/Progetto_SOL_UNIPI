#include "../headers/parser.h"
#include "../headers/server.h"
#include "../headers/log.h"

#define scan(file, str, arg, err) do {\
        errno = 0; \
        err = fscanf((file), (str), (arg)); \
        if(err == EOF && errno == 0) {\
            fclose(fp_config);                          \
            return 0;\
        } else if(err == -1 && errno != 0) { \
            perror("Error while reading config file:"); \
            fprintf(stderr, "Can't continue with execution"); \
            return -1; \
        } \
    } while(false)

//returns 0 if succ else if not
int get_config(const char * path){

    FILE * fp_config;
    fp_config = fopen(path,"r");
    if (fp_config == NULL){
        fprintf(stderr,"Error opening file config\n");
        perror("Error:");
        return -1;
    }
    int err;
    for(int i = 0; i <= MAXCONFS;i++){
        char flag[6] = {'\0'};
        scan(fp_config,"%5c",flag,err);
        //fgets is used instead of scan because fgets stops at newline char. and it's really useful for strings

        if (strncmp(flag,"[LPT]",strlen("[LPT]")) == 0){
            //If Log path
            if(fgets(configFile.log_path,MAXPATH,fp_config) == NULL){
                fprintf(stderr,"Error reading config file.\n");
                return -1;
            }
            continue;
        }
        if (strncmp(flag,"[SPT]",strlen("[SPT]")) == 0){
            //If socket path
            if(fgets(configFile.socket_path,MAXPATH,fp_config) == NULL){
                //fflush(stdout);
                fprintf(stderr,"Error reading config file.\n");
                return -1;
            }
            continue;
        }
        if (strncmp(flag,"[NWK]",strlen("[NWK]")) == 0){
            //If # of slaves
            scan(fp_config,"%u\n",&configFile.Nslaves,err);
            continue;
        }
        if(strncmp(flag,"[MFL]",strlen("[MFL]")) == 0){
            //If Max File
            scan(fp_config,"%u\n",&configFile.max_files,err);
            continue;
        }
        if(strncmp(flag,"[MMB]",strlen("[MMB]")) == 0){
            //If Max MB
            scan(fp_config,"%lu\n",&configFile.max_memory,err);
            configFile.max_memory *= 1000000;
            continue;
        }

        if (strncmp(flag,"[PCY]",strlen("[PCY]")) == 0) {
            char str_policy[24] = {'\0'};
            if (fgets(str_policy, MAXPATH, fp_config) == NULL) {
                fprintf(stderr, "Error reading config file.\n");
                return -1;
            }
            if (strncmp(str_policy, "FIFO", strlen("FIFO")) == 0) {

                configFile.policy = FIFO;
                //_log("FIFO Policy has been chosen\n");
                //fprintf(stderr,"FIFO Policy!\n");
            } else {

                configFile.policy = LRU;
                //_log("LRU Policy has been chosen\n");
                //fprintf(stderr,"LRU Policy!\n");
            }

        }
    }
    fclose(fp_config);
    return 0;

}

void printConfigFile(void){
    printf(
            "Max files %u\n Max memory %lu \n Nslaves %u\n socket path %s\n log path %s\n policy %d\n",
            configFile.max_files,
            configFile.max_memory,
            configFile.Nslaves,
            configFile.socket_path,
            configFile.log_path,
            configFile.policy
            );
}