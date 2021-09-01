#include "../headers/clientutils.h"

int serverfd = 0;

#define can_continue_function     can_continue = recv_number(serverfd);\
    if (can_continue != CONTINUE){\
        int status = get_resp();\
        fprintf(stderr,"%s", error_to_str(errno));\
        return status;\
    }
#define get_resp_and_return(macro)    status = get_resp();\
                                if (status == NOT_OK){\
                                    return status;\
                                }\
                                return status;

int get_resp(void){
    errno = 0;
    int status = recv_number(serverfd);
    int error = recv_number(serverfd);
    errno = error;
    if (status == OK && error != 0){
        //Uuuh that's weird
        return -1;
    }
    return status;

}

//Imposta il tempo in req partendo dagli ms
int setFromMsec(long msec, struct timespec* req){
    if(msec < 0 || req == NULL){
        errno = EINVAL;
        return -1;
    }

    req->tv_sec = msec / 1000;
    msec = msec % 1000;
    req->tv_nsec = msec * 1000;

    return 0;
}

//Imposta res da quando viene chiamata questa funzione + nsec + sec
int addToCurrentTime(long sec, long nsec, struct timespec* res){
    clock_gettime(CLOCK_REALTIME, res);
    res->tv_sec += sec;
    res->tv_nsec += nsec;
    return 0;
}
int openConnection(const char * sockname,int msec,const struct timespec abstime){
    serverfd = socket(AF_UNIX,SOCK_STREAM,0);
    if (serverfd < 0 ){
        fprintf(stderr,"%s", strerror(errno));
        exit(EXIT_FAILURE);
    }
    struct sockaddr_un connInfo;
    memset(&connInfo, 0, sizeof(struct sockaddr_un));
    connInfo.sun_family = AF_UNIX;
    strcpy(connInfo.sun_path,sockname);


    //quanto aspettiamo tra un tentativo e l'altro
    struct timespec wait_time;
    setFromMsec(msec, &wait_time);
    //tempo corrente
    struct timespec curr_time;
    clock_gettime(CLOCK_REALTIME, &curr_time);
    int ret = 0;
    while ((ret = connect(serverfd,(struct sockaddr *)&connInfo,sizeof(connInfo))) == -1
           && curr_time.tv_sec < abstime.tv_sec){
        nanosleep(&wait_time,NULL);
        clock_gettime(CLOCK_REALTIME, &curr_time);
    }
    if (ret == -1){
        return -1;
    }
    return 0;
}

off_t file_size(const char * filename){
    struct stat st;
    if (stat(filename,&st) == 0){
        return st.st_size;
    }
    return -1;
}

int send_req(int op,const char * filename,int flags){
    int err = send_number(serverfd,op);
    if (err < 0){
        fprintf(stderr,"Error sending number!\n");
        exit(EXIT_FAILURE);
    }
    err = send_message(serverfd,filename, strlen(filename)+1);
    if (err < 0){
        fprintf(stderr,"Error sending message!\n");
        exit(EXIT_FAILURE);
    }
    err = send_number(serverfd,flags);
    if (err < 0){
        fprintf(stderr,"Error sending number!\n");
        exit(EXIT_FAILURE);
    }
    return 0;

}

int visitDir(char *pathname, char *fileList[],int * acc){

    char *realp = (char *) safe_malloc(sizeof(char) * MAXNAME);
    realpath(pathname,realp);
    DIR * dir = NULL;
    if ((dir = opendir(realp)) == NULL){
        perror("Errore aprendo la cartella specificata in -w :");
        return 0;
    }
    struct dirent * entry = NULL;
    //char * toCatfileList[MAXFILES] = {"\0"};
    while((entry = readdir(dir)) != NULL){
        if(strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0){
            errno = 0;
            continue;
        }
        if(entry->d_type != DT_DIR){
            fileList[*acc] = (char *) safe_malloc(sizeof(char) * MAXNAME);
            strcat(fileList[*acc],realp);
            strcat(fileList[*acc],"/");
            strcat(fileList[*acc],entry->d_name);
            (*acc)++;

        }else{
            char *dirname = (char *) safe_malloc(sizeof(char) * MAXNAME);
            strcat(dirname,realp);
            strcat(dirname,"/");
            strcat(dirname,entry->d_name);
            visitDir(dirname,fileList,acc);
        }
    }

    closedir(dir);
    return *acc;
}
int readNFiles(int N, const char * dirname){
    errno = 0;
    if (N < 0) N = 0;
    int hasDirname = (dirname != NULL);
    send_req(READN_FILE,"readNString",N);
    int real_N = recv_number(serverfd);
    if (real_N == 0){
        fprintf(stderr,"Server can provide only 0 files\n");
        return NOT_OK;
    }
    struct stat st = {0};
    if (hasDirname){
        if (stat(dirname, &st) == -1) {
            mkdir(dirname, 0700);
        }
    }
    char * buffer[real_N];
    int can_continue = 0;
    can_continue_function;
    for (int i = 0; i < real_N; i++){
        buffer[i] = recv_message(serverfd);
        if (buffer[i] == NULL){
            buffer[i] = "Default string";
        }
        size_t file_size = recv_number(serverfd);
        if (file_size == 0){
            //fprintf(stderr,"File size is 0\n");
            continue;
        }
        if (hasDirname){
            char filename[MAXNAME] = {'\0'};
            sprintf(filename,"%s/READN_FILE_%d",dirname,i);
            int file_fd = open(filename,O_CREAT | O_RDWR,0644);
            if (file_fd == -1){
                perror("[ReadN] Cannot create file");
                break;
            }
            size_t wroteBytes = 0;
            while(wroteBytes < file_size){
                wroteBytes += write(file_fd,buffer[i],file_size);
            }
            //printf("[%s] Wrote %zu bytes : %s\n","READN",wroteBytes,filename);
        }


    }
    int status = 0;
    get_resp_and_return(READN);


}
int removeFile(const char* pathname){
    if (pathname == NULL) return 0;
    send_req(REMOVE_FILE,pathname,0);
    int status = 0;
    get_resp_and_return(REMOVE);

}

int closeConnection(const char * sockname){
    char sock_tmp[PATH_MAX] = {'\0'};
    if (sockname == NULL){
        strncpy(sock_tmp,"/tmp/sok", strlen("/tmp/sok")+1);
    }else{
        strncpy(sock_tmp,sockname, strlen(sockname));
    }
    send_req(CLOSE_CONNECTION,"Close connection",0);
    int resp = get_resp();
    if (resp == OK){
        close(serverfd);
        return resp;
    }
    return NOT_OK; // This shouldn't be reachable but here anyway

}
int openFile(const char * pathname,int flags){
    send_req(OPEN_FILE,pathname,flags);
    int status = 0;
    get_resp_and_return(OPEN);
}

int readFile(const char * pathname, void ** buf, size_t * size){
    send_req(READ_FILE,pathname,0);

    int can_continue = 0 ;

    can_continue_function;
    can_continue_function;
    size_t file_size = recv_number(serverfd);
    printf("File size : %zu\n",file_size);
    if (file_size == 0){
        fprintf(stderr,"Zero file size arrived, have you wrote something to the server ?\n");
    }

    char * file_cont = recv_message(serverfd);
    if (file_cont == NULL){
        return -1;
    }
    *buf = file_cont;
    *size = file_size;
    int status = 0;
    get_resp_and_return(READ);
}

int writeFile(const char * pathname, const char * dirname){

    send_req(WRITE_FILE,pathname,0);

    int can_continue = 0;
    can_continue_function;
    off_t err = file_size(pathname);

    if (err < 0){
        fprintf(stderr,"%s\n", strerror(errno));
        return -1;
    }
    off_t size = err;
    err = send_number(serverfd,size); //Send file size
    if (err < 0){
        fprintf(stderr,"Error sending number for WRITE\n");
        return -1;
    }
    can_continue_function;
    FILE * fp = fopen(pathname,"r+");
    if (fp == NULL){
        fprintf(stderr,"%s\n", strerror(errno));
        return -1;
    }
    char file_cont[size];
    while(feof(fp) == 0){
        fread(file_cont,sizeof(char),size,fp);
    }
    err = send_message(serverfd,file_cont,size);
    if (err != 0){
        fprintf(stderr,"File not properly wrote\n");
        return -1;
    }
    can_continue_function;
    int status = 0;
    get_resp_and_return(WRITE);

}

int appendToFile(const char * pathname,void *buf,size_t size,const char *dirname){
    send_req(APPEND_FILE,pathname,0);

    int can_continue = 0;
    can_continue_function;
    int err = send_number(serverfd,size); //Send file size
    if (err < 0){
        fprintf(stderr,"Error sending number for APPEND\n");
        return -1;
    }
    err = send_message(serverfd,(char *) buf,size);
    if (err != 0){
        fprintf(stderr,"File not properly wrote\n");
        return -1;
    }
    int status = 0;
    get_resp_and_return(APPEND);

}

int closeFile(char *pathname){
    send_req(CLOSE_FILE,pathname,0);
    int status = 0;
    get_resp_and_return(CLOSE);
}

//Crea una linked list con le operazioni passate da linea di comando
Action * parseArgs(int argc, const char *argv[]){
    char *socket;
    char *dirname_d;
    char *W_fileNames[MAXREADABLEFILE];
    char *r_fileNames[MAXREADABLEFILE];
    char *c_fileNames[MAXREADABLEFILE];
    Action * head = NULL;
    size_t w_N = 0;
    //Per raccogliere tutti i paramentri presenti
    char * w_args;
    int opt;
    w_args = (char *) malloc(sizeof(char)*MAXNAME);
    while((opt = getopt(argc,(char **)argv,":hf:w:W:r:R:d:t:c:p")) != -1){
        switch (opt)
        {
            case 'h':{
                puts("-h                Printa quello che stai leggendo a schermo adesso");
                puts("-f filename       Specifica il nome del socket AF_UNIX a cui connettersi [ATTENZIONE]CONNETTERSI SEMPRE[ATTENZIONE]");
                puts("-w dirname[,n=0]  Invia al server i file della cartella dirname, se n=0 non limite superiore");
                puts("-W file[,file2]   Lista dei nomi dei file da inviare");
                puts("-r file[,file2]   Lista dei nomi dei file da leggere nel server");
                puts("-R [n=0]          Legge n file qualsiasi dal server");
                puts("-d dirname        Nome della cartella (Path assoluto) dove andare a memoriazzare i file letti tramite -r altrimenti non");
                puts("                  vengono scritti su disco, nel caso non esista viene creata");
                puts("-t time           Tempo in ms che interrcorre tra l'invio di due richieste successive al server");
                puts("-c file[,file2]   Lista di file da rimuovere dal server se presenti");
                puts("-p                Abilita le stampe su stdout per ogni operazione e vengono printate <tipo di op,file di riferimento,esito,dove si sono effettuate letture e scritture>");
                puts(" ");
            }
                break;
            case 'f': {
                socket = optarg;
                Action * new = initAction(NULL,0,'f',socket);
                add_to_tail(&head,new);

                break;
            }
            case 'w' :{
                //Copio la stringa in w_args
                strncpy(w_args,optarg,strlen(optarg));
                char * svptr = NULL;
                dirname_d = strtok_r(w_args,",",&svptr);
                //Se null significa che non e' stato specificato il numero
                char *num = strtok_r(NULL,",",&svptr);
                if (num == NULL){
                    w_N = 0;
                    Action * new = initAction(NULL,w_N,'w',dirname_d);
                    add_to_tail(&head,new);
                    continue;
                }
                w_N = atoi(&num[0]);
                Action * new = initAction(NULL,w_N,'w',dirname_d);
                add_to_tail(&head,new);
                break;
            }
            case 'W': {
                //char *tmp = (char *) malloc(sizeof(char) * MAXPATH);
                int counter = 0;
                char *svptr,* save;
                save = strtok_r(optarg,",",&svptr);
                W_fileNames[0] = (char *) malloc(sizeof(char) * (strlen(save)+1));
                strncpy(W_fileNames[0],save,strlen(save));
                while((save = strtok_r(NULL,",",&svptr)) != NULL){
                    counter++;
                    W_fileNames[counter] = (char *) malloc(sizeof(char) * (strlen(save)+1));
                    strncpy(W_fileNames[counter],save,strlen(save));
                }
                counter++;
                W_fileNames[counter] = NULL;
                //memset(W_fileNames[counter],0,1);
                Action * new = initAction((const char **) W_fileNames,0,'W',NULL);
                add_to_tail(&head,new);


                break;
            }
            case 'r':{
                int counter = 0;
                char *svptr,* save;
                save = strtok_r(optarg,",",&svptr);
                r_fileNames[0] = (char *) malloc(sizeof(char) * (strlen(save)+1));
                strncpy(r_fileNames[0],save,strlen(save));
                while((save = strtok_r(NULL,",",&svptr)) != NULL){
                    counter++;
                    r_fileNames[counter] = (char *) malloc(sizeof(char) * (strlen(save)+1));
                    strncpy(r_fileNames[counter],save,strlen(save));
                }
                counter++;
                r_fileNames[counter] = NULL;
                Action * new = initAction((const char **) r_fileNames,0,'r',NULL);
                add_to_tail(&head,new);
                break;
            }
            case 'R':{
                int R = atoi(&optarg[0]);
                Action * new = initAction(NULL,R,'R',NULL);
                add_to_tail(&head,new);
                break;
            }
            case 'd':{
                Action * new = initAction(NULL,0,'d',optarg);
                add_to_tail(&head,new);
                break;
            }
            case 't':{
                int t = atoi(&optarg[0]);
                Action * new = initAction(NULL,t,'t',NULL);
                add_to_tail(&head,new);
                break;
            }
            case 'c':{
                int counter = 0;
                char *svptr,* save;
                save = strtok_r(optarg,",",&svptr);
                c_fileNames[0] = (char *) malloc(sizeof(char) * (strlen(save)+1));
                strncpy(c_fileNames[0],save,strlen(save));
                while((save = strtok_r(NULL,",",&svptr)) != NULL){
                    counter++;
                    c_fileNames[counter] = (char *) malloc(sizeof(char) * (strlen(save)+1));
                    strncpy(c_fileNames[counter],save,strlen(save));
                }
                counter++;
                c_fileNames[counter] = NULL;
                Action * new = initAction((const char **) c_fileNames,0,'c',NULL);
                add_to_tail(&head,new);
                break;
            }
            case 'p': {
                Action * new = initAction(NULL,0,'p',NULL);
                add_to_tail(&head,new);
                break;
            }

            case '?': continue;
            case ':': printf("Nessun argomento per -%c\n",optopt);
            default:
                break;
        }
    }
    return head;

}

void add_to_tail(Action **head,Action * new){
    nullcheck(new);
    if (*head == NULL){
        *head = new;
        return;
    }
    Action * cursor = *head;
    Action * prev = NULL;
    while(cursor != NULL){
        prev = cursor;
        cursor = cursor->next;
    }
    prev->next = new;
}
Action * lookup(Action * head,char op){
    nullcheck(head);
    Action * cursor = head;
    while(cursor != NULL){
        if(cursor->op == op){
            return cursor;
        }
        cursor = cursor->next;
    }
    return NULL;

}
Action * initAction(const char **args,int n,char op,char * singleArg){
    Action * new = (Action *) safe_malloc(sizeof(Action));
    new->op = op;
    new->args = NULL;
    if(args != NULL){
        new->args = (char **) safe_malloc(sizeof(char *) * MAXREADABLEFILE);
        int i = 0;
        for(i = 0;args[i] != NULL;i++){
            size_t nameLen = strlen(args[i])+1; //Null char
            new->args[i] = (char *) safe_malloc(sizeof(char) * nameLen);
            strncpy(new->args[i],args[i],nameLen);
        }
        i++;
        new->args[i] = NULL;
    }
    new->singleArg = NULL;
    if(singleArg != NULL){
        size_t nameLen = strlen(singleArg)+1;
        new->singleArg = (char *) safe_malloc(sizeof(char) * nameLen);
        strncpy(new->singleArg,singleArg,nameLen);
    }
    new->n = n;
    new->next = NULL;
    return new;

}

void printActions(Action *head){
    Action * cursor = head;
    while(cursor != NULL){
        printf("OP : %c\n",cursor->op);
        if(cursor->args != NULL){
            int counter = 0;
            while(cursor->args[counter] != NULL){
                printf("File %d : %s\n",counter,cursor->args[counter]);
                counter++;
            }
        }
        if(cursor->singleArg != NULL){
            printf("File : %s\n",cursor->singleArg);
        }
        printf("Number %d\n",cursor->n);
        cursor = cursor->next;
    }
}








