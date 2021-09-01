#include "../headers/clientutils.h"


config_t configFile;


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
    } while(0)

#define isOP(op_type,op) (strcmp(op_type,op) == 0)
int apiCall(int retOfCall,char *op_type,char *filename,int N,int doPrint){
    int err = errno;
    if(doPrint){
        //openFile
        if(isOP(op_type,OPEN)){
            if (retOfCall == OK){
                printf("[%s] on %s : %s\n",OPEN,filename,GOOD);
            }else{
                printf("[%s] on %s : %s\n",OPEN,filename,BAD);
                fprintf(stderr,"%s", error_to_str(err));
            }
        }
        if(isOP(op_type,WRITE)){
            if(retOfCall == OK){
                printf("[%s] on %s: %s wrote %d bytes\n",op_type,filename,GOOD,N);
            }else{
                printf("[%s] on %s: %s\n",op_type,filename,BAD);
                fprintf(stderr,"%s", error_to_str(err));
            }
        }
        //readFile
        if(isOP(op_type,READ)){
            if(retOfCall == OK){
                printf("[%s] on %s : %s\n",op_type,filename,GOOD);
            }else{
                printf("[%s] on %s : %s\n",op_type,filename,BAD);
                fprintf(stderr,"%s", error_to_str(err));
            }
        }
        if(isOP(op_type,READN)){
            if(retOfCall == OK){
                printf("[%s] on %d files of the server : %s\n",op_type,N,GOOD);
            }else{
                printf("[%s] on %s : %s\n",op_type,filename,BAD);
                fprintf(stderr,"%s", error_to_str(err));

            }
        }
        if(isOP(op_type,REMOVE)){
            if(retOfCall == OK){
                printf("[%s] on %s : %s\n",op_type,filename,GOOD);
            }else{
                printf("[%s] on %s : %s\n",op_type,filename,BAD);
                fprintf(stderr,"%s", error_to_str(err));

            }
        }

    }
    return 0;
}

int get_config(const char * path){

    FILE * fp_config;
    fp_config = fopen(path,"r");
    if (fp_config == NULL){
        fprintf(stderr,"Error opening file config\n");
        perror("Error:");
        return -1;
    }
    int err;
    for(int i = 0; i < 6;i++){
        char flag[6] = {'\0'};

        scan(fp_config,"%5c",flag,err);
        //fgets is used instead of scan because fgets stops at newline char. and it's really useful for strings

        if (strncmp(flag,"[LPT]",strlen("[LPT]")) == 0){
            //If Log path

            if(fgets(configFile.log_path,256,fp_config) == NULL){
                fprintf(stderr,"Error reading config file.\n");
                return -1;
            }
            continue;
        }
        if (strncmp(flag,"[SPT]",strlen("[SPT]")) == 0){
            //If socket path
            if(fgets(configFile.socket_path,256,fp_config) == NULL){
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
            continue;
        }
    }
    fclose(fp_config);
    return 0;

}


int main(int argc, char **argv){

    if (argc < 2){
        printf("To see documentation launch with -h");
        exit(EXIT_FAILURE);
    }
    Action * action = parseArgs(argc,(const char **)argv);
    Action * socket = lookup(action,'f');
    Action * print = lookup(action,'p');
    Action * dirToWrite = lookup(action,'d');
    int doPrint = (print != NULL);
    int err = get_config("config.txt");
    struct timespec abstime;
    int response = 0;
    if (socket == NULL) {
        puts("No -f detected, using default config to select socket\n");
        response = openConnection(configFile.socket_path,DEFAULT_TIME,abstime);
    }else{
        sprintf(socket->singleArg,"%s\n",socket->singleArg); //Because the parser adds a new line to the file name
        response = openConnection(socket->singleArg,DEFAULT_TIME,abstime);
    }
    if (response == -1){
        perror("Quitting client because can't connect! Error :->");
        exit(EXIT_FAILURE);
    }
    struct timespec wait_between;
    setFromMsec(0,&wait_between);
    while(action != NULL){
        int resp = 0;
        switch (action->op) {
            case 't': {
                setFromMsec(action->n, &wait_between);
                nanosleep(&wait_between, NULL);
                next_Action;
                break;
            }
            case 'w': { 
                // Entra in una cartella e invia tutti i file di quella cartella, se ci sono altre cartelle nella cartella allora invia ricorsivamente anche quelle
                DIR *dir = NULL;
                int acc = 0;
                char *fileNames[MAXFILES] = {"\0"};

                if ((dir = opendir(action->singleArg)) == NULL) {
                    perror("Errore aprendo la cartella specificata in -w :");
                    next_Action;
                }
                visitDir(action->singleArg, fileNames, &acc);
                if (action->n != 0) {
                    for (int i = 0; i < action->n && (i < acc); i++) {

                        size_t f_size = file_size(fileNames[i]);
                        if (f_size == -1) {
                            break;
                        }
                        err = openFile(fileNames[i], O_CREATE);
                        apiCall(err, OPEN, fileNames[i], 0, doPrint);
                        err = writeFile(fileNames[i], NULL);
                        apiCall(err, WRITE, fileNames[i], file_size(fileNames[i]), doPrint);
                    }
                }
                next_Action;
                break;
            }
            case 'W': {
                int i = 0;
                while (action->args[i] != NULL) {
                    char *tmp = (char *) safe_malloc(sizeof(char) * MAXNAME);
                    realpath(action->args[i], tmp);
                    apiCall(openFile(tmp, O_CREATE), OPEN, tmp, 0, doPrint);
                    apiCall(writeFile(tmp, NULL), WRITE, tmp, file_size(tmp), doPrint);
                    free(tmp);
                    i++;
                }
                next_Action;
                break;
            }
            case 'r': {
                //Se abbiamo dato l'opezione -d
                if (dirToWrite != NULL) {
                    DIR *folder = NULL;
                    folder = opendir(dirToWrite->singleArg);
                    if (folder == NULL) { //Se la cartella non esiste
                        mkdir(dirToWrite->singleArg, 0644);
                    } else { //Se la cartella esiste, ok chiudila
                        closedir(folder);
                    }
                }
                for (int i = 0; action->args[i] != NULL; i++) {
                    char *file = NULL;
                    size_t size;
                    char *tmp = (char *) safe_malloc(sizeof(char) * MAXNAME);
                    realpath(action->args[i], tmp);
                    err = openFile(action->args[i], 0);
                    apiCall(err, OPEN, tmp, 0, doPrint);
                    err = readFile(tmp, (void **) &file, &size);
                    apiCall(err, READ, tmp, 0, doPrint);
                    if (dirToWrite != NULL && file != NULL) {
                        memset(tmp, 0, MAXNAME);
                        sprintf(tmp, "%s/%s", dirToWrite->singleArg, action->args[i]);
                        int file_fd = open(tmp, O_CREAT | O_RDWR, 0644);
                        if (file_fd == -1) {
                            perror("Cannot create file -> :");
                            break;
                        }
                        size_t wroteBytes = 0;
                        while (wroteBytes < size) {
                            wroteBytes += write(file_fd, file, size);
                        }
                        //printf("Scritti %zu bytes: %s\n", wroteBytes, tmp);

                    }
                    free(tmp);
                }
            }
                next_Action;
                break;
            case 'c':{
                for(int i = 0;action->args[i] != NULL;i++){
                    char *tmp = (char *) safe_malloc(sizeof(char) * MAXNAME);
                    realpath(action->args[i],tmp);
                    apiCall(removeFile(tmp),REMOVE,tmp,0,doPrint);
                    free(tmp);
                }
                next_Action;
                break;
            }
            case 'R':{
                apiCall(readNFiles(action->n,dirToWrite->singleArg),READN,NULL,action->n,doPrint);
                next_Action;
                break;
            }
            default:
                next_Action;
                break;
        }
    }
    if (socket != NULL){
        closeConnection(socket->singleArg);
    }else{
        closeConnection(configFile.socket_path);
    }


    return 0;


}
