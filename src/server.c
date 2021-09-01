

#include "../headers/server.h"
#include "../headers/server_utils.h"
#include "../headers/log.h"
#include "../headers/memory.h"
#include "../headers/prot.h"
#include "../headers/queue.h"

typedef struct __job{

    int op;                     //read from client_thread still op
    int flags;
    char filename[MAXNAME];
    List * client_queue;
    int err;                    //error to report to client
    int status_request;         //read from client_thread is response [OK/NOT_OK]
    int client_fd;              //Workers are allowed to send only files to the client process (for now)

}Job;

typedef struct __args_thrd_client{

    int client_fd;
    List * queue_of_response;

}Client;

volatile sig_atomic_t serverMode = 1;

void handle_graceful(int sig){
    serverMode = GRACEFUL;
}
void handle_shutdown(int sig){
    serverMode = OFF;
}
//Locks for logfile
pthread_mutex_t lockLogFile = PTHREAD_MUTEX_INITIALIZER;

config_t configFile;
FILE * logFile;
Server_metadata serverMetadata;
List * workList = NULL;


#define push_node_in_worklist(werk)              err = push_node_to_list(workList, werk);\
                    if (err < 0){\
                        fprintf(stderr,"Error inserting node inside list workList\n");      \
                        exit(EXIT_FAILURE);                                                                    \
                    }

#define set_response_for_client_and_push(error,status)  work_to_be_done->err = error; \
    work_to_be_done->status_request = status; \
    Node *resp_to = create_work(work_to_be_done); \
    push_node_to_list(work_to_be_done->client_queue, resp_to);

#define error_handling_minus(call,message)  \
    if ((call) == -1) { \
        fprintf(stderr,message);                                      \
        exit(EXIT_FAILURE);                                           \
    }

Job * jobInit(int op,int flags,char *filename, List * queue,int client_fd){
    if ((filename == NULL) || (queue == NULL)){
        fprintf(stderr, "Passed queue and filename as null in jobInit!\n");
        return NULL;
    }
    Job * job = (Job *) safe_malloc(sizeof(Job));
    size_t length = strlen(filename);
    if (length >= MAXNAME){
        fprintf(stderr,"File name too big!\n");
        return NULL;
    }
    strncpy(job->filename,filename,length);
    job->op = op;
    job->flags = flags;
    job->client_queue = queue;
    job->status_request = INITED;
    job->err = INITED;
    job->client_fd = client_fd;
    return job;
}

Node * create_work(Job * job){
    if (job == NULL){
        fprintf(stderr,"Passed job is null!\n");
        return NULL;
    }
    Node * node = create_node(job); // No need to check, can return null only if job is null
    return node;
}
void flood_list(List * list,int N){
    for(int i = 0;i < N;i++) {
        Job * job = jobInit(END_WORKER, 0, "Nop", list, 123);
        Node * work = create_work(job);
        push_node_to_list(list,work);
    }


}

int send_response(Job * resp){
    int job_error, status;
    status = resp->status_request;
    job_error = resp->err;
    int err = send_number(resp->client_fd,status);
    if (err < 0) return -1;
    err = send_number(resp->client_fd,job_error);
    if (err < 0) return -1;
    return 0;
}

void * worker_thread(void * args){

    worker_add();
    if (workList == NULL){fprintf(stderr,"workingList is null!\n"); exit(EXIT_FAILURE);}
    while(serverMode != OFF) {
        //Can't fail, workList can't be null, so the only way it can return null it when server has mode OFF or there isn't any work to do and the server is in gracefull state
        //Node *node = pop_element_from_tail_worker_thrd(workList);
        Node * node = pop_element_from_tail(workList);
        if (node == NULL){ //Only way node could be null is by not having to do nothing and graceful as mode on server
            worker_sub();
            //fprintf(stderr,"Closing worker!\n\n");
            pthread_exit(NULL);
        }
        if (serverMode != OFF) {
            if (node->obj == NULL){
                fprintf(stderr,"Passed null obj to work_thread closing server!\n");
                exit(EXIT_FAILURE);
            }
            Job *work_to_be_done = (Job *) node->obj;
            safe_free(node);
            switch (work_to_be_done->op) {
                case OPEN_FILE: {
                    if (work_to_be_done->flags == O_CREATE) {
                        _log("Requested create file from client!\n");
                        if (inMem(work_to_be_done->filename) == 1) { //if file already exists
                            set_response_for_client_and_push(EEXIST,NOT_OK);
                            continue;
                        }
                        cache_algorithm(1,0);
                        if (errno == ECANCELED){
                            _log("Action is legal, cache algorithm not initiated\n");
                        }
                        FileMemory *file = create_file(work_to_be_done->filename);
                        if (file == NULL) {
                            fprintf(stderr, "Unable to create file!\n");
                            exit(EXIT_FAILURE);
                        }
                        add_file_to_memory(file);
                        _log("File %s has been created\n",file->filename);
                        set_file_open(file);
                        //Don't need to check,  it can fail only if there is already a file with the same name
                        set_response_for_client_and_push(0,OK);
                        continue;
                    } else {
                        _log("Requested open file from client! File : %s\n",work_to_be_done->filename);
                        //If we want to open a file (Or at least we don't want to create a file)
                        if (inMem(work_to_be_done->filename) == 0) { // if file isn't in memory
                            set_response_for_client_and_push(ENOENT,NOT_OK);
                            continue;
                        }
                        FileMemory *file = find_file(work_to_be_done->filename,
                                                     configFile.policy); //Can only fail if filename isn't in memory
                        set_file_open(file);
                        set_response_for_client_and_push(0,OK);
                        continue;
                    }
                }
                case CLOSE_FILE: {
                    FileMemory * file = find_file(work_to_be_done->filename,configFile.policy);
                    if (file == NULL){
                        set_response_for_client_and_push(ENOENT,NOT_OK);
                        continue;
                    }
                   //_log("Requested close file from client File %s\n",work_to_be_done->filename);
                    set_file_close(file);
                    _log("File %s has been closed\n",file->filename);
                    set_response_for_client_and_push(0,OK);
                }
                case READ_FILE: { //Legge tutto il contenuto del file dal server
                    FileMemory * file = find_file(work_to_be_done->filename,configFile.policy);
                    if (file == NULL){
                        int err = send_number(work_to_be_done->client_fd,NO_CONTINUE);
                        error_handling_minus(err,"Error sending number\n");
                        set_response_for_client_and_push(ENOENT,NOT_OK);
                        continue;
                    }
                    _log("Request to read file from client %s\n",work_to_be_done->filename);
                    int err = send_number(work_to_be_done->client_fd,CONTINUE);
                    error_handling_minus(err,"Error sending number");

                    if (file->isOpen){
                        err = send_number(work_to_be_done->client_fd,CONTINUE);
                        error_handling_minus(err,"Error sending number\n");
                        err = send_number(work_to_be_done->client_fd,file->usedMemory);
                        error_handling_minus(err,"Error sending number\n");
                        // Files can be sent and wrote to only in mutual exclusion,reading is fine without lock
                        lock(&(file->lockFile));
                        err = send_message(work_to_be_done->client_fd,file->file->_IO_buf_base,file->usedMemory);
                        unlock(&(file->lockFile));
                        _log("Sent %zu bytes to client, filename : %s\n",file->usedMemory,file->filename);
                        error_handling_minus(err,"Error sending number\n");
                        set_response_for_client_and_push(0,OK);
                        continue;
                    }else{
                        err = send_number(work_to_be_done->client_fd,NO_CONTINUE);
                        error_handling_minus(err,"Error sending number\n");
                        set_response_for_client_and_push(EACCES,NOT_OK);
                        continue;
                    }

                }
                case WRITE_FILE:{ //Scrive tutto il contenuto dal client al server
                    FileMemory * file = find_file(work_to_be_done->filename,configFile.policy);
                    _log("Requested write file from client File : %s\n",work_to_be_done->filename);

                    if (file == NULL){
                        int err = send_number(work_to_be_done->client_fd,NOT_OK);
                        error_handling_minus(err,"Error sending number\n");
                        set_response_for_client_and_push(ENONET,NOT_OK);
                        continue;
                    }
                    int err = send_number(work_to_be_done->client_fd,CONTINUE);

                    error_handling_minus(err,"Error sending number\n");
                    //Recv how much we have to write
                    size_t byteToWrite = recv_number(work_to_be_done->client_fd);
                    cache_algorithm(0,byteToWrite);
                    if (errno == E2BIG){
                        err = send_number(work_to_be_done->client_fd,NO_CONTINUE);
                        error_handling_minus(err,"Error sending message");
                        set_response_for_client_and_push(E2BIG,NOT_OK);
                        continue;
                    }
                    err = send_number(work_to_be_done->client_fd,CONTINUE);
                    error_handling_minus(err,"Error sending message");
                    //Write function
                    char * msg = recv_message(work_to_be_done->client_fd);
                    if (msg == NULL){
                        set_response_for_client_and_push(EFAULT,NOT_OK);
                        continue;
                    }
                    file = find_file(work_to_be_done->filename,FIFO); //
                    if (file == NULL){
                        int err = send_number(work_to_be_done->client_fd,NOT_OK);
                        error_handling_minus(err,"Error sending number\n");
                        set_response_for_client_and_push(ENOENT,NOT_OK);
                        continue;
                    }
                    err = send_number(work_to_be_done->client_fd,CONTINUE);
                    write_to_file(file,msg,byteToWrite);
                    _log("Wrote %zu bytes to server, filename: %s\n",file->usedMemory,file->filename);
                    set_response_for_client_and_push(0,OK);
                    free(msg);
                    continue;
                }
                case APPEND_FILE:{
                    FileMemory * file = find_file(work_to_be_done->filename,configFile.policy);

                    if (file == NULL){
                        int err = send_number(work_to_be_done->client_fd,NOT_OK);
                        error_handling_minus(err,"Error sending number\n");
                        set_response_for_client_and_push(ENONET,NOT_OK);
                        continue;
                    }
                    int err = send_number(work_to_be_done->client_fd,CONTINUE);

                    error_handling_minus(err,"Error sending number\n");
                    //Recv how much we have to write
                    size_t byteToWrite = recv_number(work_to_be_done->client_fd);
                    cache_algorithm(0,byteToWrite);
                    if (errno == E2BIG){
                        err = send_number(work_to_be_done->client_fd,NO_CONTINUE);
                        error_handling_minus(err,"Error sending message");
                        set_response_for_client_and_push(E2BIG,NOT_OK);
                        continue;
                    }
                    //Write function
                    char * msg = recv_message(work_to_be_done->client_fd);
                    if (msg == NULL){
                        set_response_for_client_and_push(EFAULT,NOT_OK);
                        continue;
                    }
                    write_to_file(file,msg,byteToWrite);
                    _log("Wrote %zu bytes to server, filename: %s\n",file->usedMemory,file->filename);
                    set_response_for_client_and_push(0,OK);
                    free(msg);
                    continue;
                }
                case READN_FILE:{
                    int number_of_files = work_to_be_done->flags;
                    unsigned int actual_files = 0;
                    actual_files = get_actual_files();
                    if (actual_files == 0){
                        int err = send_number(work_to_be_done->client_fd,NO_CONTINUE);
                        error_handling_minus(err,"Number not sent correctly\n");
                        set_response_for_client_and_push(ENODATA,NOT_OK);
                        continue;
                    }
                    if (number_of_files > actual_files){
                        //Se il server ha meno di N files, li invia tutti
                        int err = send_number(work_to_be_done->client_fd,actual_files);
                        error_handling_minus(err,"Error sending number\n");
                        number_of_files = actual_files;
                    }else {
                        if (number_of_files == 0){
                            number_of_files = actual_files;
                        }
                        int err = send_number(work_to_be_done->client_fd, number_of_files);
                        error_handling_minus(err, "Error sending number\n");
                    }
                    //Doesn't need to check in theory, at this point of the switch statement
                    //there should be at least one file,so get_head() isn't likely to return NULL
                    //Can still happen if timing is correct tho

                    FileMemory * file = get_head();
                    if (file == NULL){
                        _log("Trying to get head ,but there is no head\n");
                        int err = send_number(work_to_be_done->client_fd,NO_CONTINUE);
                        error_handling_minus(err,"Error sending number\n");
                        set_response_for_client_and_push(ENODATA,NOT_OK);
                        continue;
                    }
                    int err = send_number(work_to_be_done->client_fd,CONTINUE);
                    error_handling_minus(err,"Error sending number\n");

                    for(int i = 0;i < number_of_files;i++){
                        err = send_message(work_to_be_done->client_fd,
                                           file->file->_IO_buf_base,
                                           file->usedMemory);
                        error_handling_minus(err,"Error sending file\n");
                        err = send_number(work_to_be_done->client_fd,file->usedMemory);
                        error_handling_minus(err,"Error sending number\n");
                        _log("Sent %zu bytes to client, filename : %s\n",file->usedMemory,file->filename);
                        file = next(file);
                    }
                    set_response_for_client_and_push(0,OK);
                    continue;
                }
                case REMOVE_FILE:{
                    FileMemory * file = find_file(work_to_be_done->filename,configFile.policy);
                    if (file == NULL){
                        set_response_for_client_and_push(errno,NOT_OK);
                        continue;
                    }
                    _log("Removing file %s \n",file->filename);
                    //remove generic files can't fail because it can return -1 only if the file doesn't exist
                    int err = remove_generic_file(file); // Can't assume it's LRU policy and we can't pop from tail
                    error_handling_minus(err,"Error removing files"); //But just in case
                    set_response_for_client_and_push(0,OK);
                    continue;

                }
                case CLOSE_CONNECTION:{
                    set_response_for_client_and_push(0,OK);
                    _log("Closing client connection!\n");
                    continue;
                }
                case END_WORKER:{
                    safe_free(work_to_be_done);
                    worker_sub();
                    pthread_exit(NULL);
                }


            }
            //safe_free(node->obj);
            //safe_free(node);

        }
        //fprintf(stderr,"Closing worker!\n\n");
        worker_sub();
        //fprintf(stderr,"Calling pthread_exit\n");
        pthread_exit(NULL);
    }
    pthread_exit(NULL);
}


void * client_thread(void * args) {
    int err = 0;
    Client *arg = (Client *) args;
    int client_fd = arg->client_fd;
    List * list = arg->queue_of_response;
    int receiveMessage = 1;
    while(receiveMessage && serverMode != OFF) {
        // get_req
        int op = recv_number(client_fd);
        char * msg = recv_message(client_fd);
        int flags = recv_number(client_fd);
        if (msg == NULL){
            continue;
        }
        //
        Job * job = jobInit(op,flags,msg,list,client_fd);
        Node * work = create_work(job);
        safe_free(msg);
        push_node_in_worklist(work);
        Node * done = pop_element_from_tail(list);
        Job * job_done = (Job *) done->obj;
        op = job_done->op;
        err = send_response(job_done);
        if (err < 0){
            // if there is an error sending response it will be reported in log
            _log("Error sending response to client %d, OP : %d failed\n",
                 job_done->client_fd,
                 op);
            fprintf(stderr,"Error sending response, check log file\n");
        }
        safe_free(done);
        safe_free(job_done);
        if ( op == CLOSE_CONNECTION){
            receiveMessage = 0;
            safe_free(list);
            break;
        }
    }
    pthread_exit(NULL);
}

void u_l(void){
    unlink(configFile.socket_path);
    remove(configFile.socket_path);
}

int main(int argc,char *argv[]){

    if (argc > 2){
        fprintf(stderr,"There must be only one argument and it has to be the config file path\n");
        exit(EXIT_FAILURE);
    }

    char * config_path = (argc == 1) ? "config.txt" : argv[1];

    if (get_config(config_path) == -1){
        fprintf(stderr,"Unable to read config file, closing server.\n");
        exit(EXIT_FAILURE);
    }
    //puts(configFile.socket_path);
    int err = initLogfile(configFile.log_path);
    if (err == -1){
        fprintf(stderr,"Error initializing logFile\n");
        exit(EXIT_FAILURE);
    }

    _log("Logging started successfully\n");
    atexit(u_l);
    if (configFile.policy == LRU){
        _log("LRU Policy has been chosen\n");
    }else{
        _log("FIFO Policy has been chosen\n");
    }
    initMemory(configFile);
    workList = initList();



    // Setting up socket
    int serverfd = socket(AF_UNIX,SOCK_STREAM,0);
    if (serverfd < 0){
        fprintf(stderr,"Error creating socket");
        _log("Closing server : Cannot create socket");
        perror("[socket error]: ");
        exit(EXIT_FAILURE);
    }
    struct sockaddr_un serverInfo;
    memset(&serverInfo, 0, sizeof(serverInfo));
    serverInfo.sun_family = AF_UNIX;
    strncpy(serverInfo.sun_path,configFile.socket_path,strlen(configFile.socket_path)+1);
    _log("Setting up socket...\n");
    if (bind(serverfd,(struct sockaddr *) &serverInfo,sizeof(serverInfo)) < 0){
        fprintf(stderr,"Cannot bind to socket\n");
        perror("Error:");
        return -1;
    }
    _log("Binded socket!\n");
    if (listen(serverfd,SOMAXCONN) <  0 ){
        fprintf(stderr,"Error listening on socket\n");
        _log("Error listening on socket\n");
        exit(EXIT_FAILURE);
    }


    //Setup for using pselect
    /*
     * 1) Setting a sigset_t to block signals because we need to be able to listen to SIGHUP SIGINT and SIGQUIT only during pselect, otherwise
     *      unexpected behaviour may ensue
     *
     * 2) After obtaining the old sigset, we use it on pselect because it applies the given sigmask while waiting on fd_sets,
     *     so we need to have the old mask which can listen to SIGHUP SIGINT and SIGQUIT
     *
     * 3) Before using pselect we need to change behaviour for our signal handlers
     *
     * 4) Now that we have pselect setted up we just need to call it
     *
     * */

    //-----------------------Signals setup-------------------------//
    sigset_t mask, oldmask;
    memset(&mask,0,sizeof(mask));
    memset(&oldmask,0,sizeof(oldmask));

    block_PIPE_SIGINT_SIGQUIT_SIGUP(mask,oldmask); //It does call sigmask
    add_SIGPIPE_BLOCK(oldmask); //Adding SIGPIPE to oldmask should mask SIGPIPE during pselect,we don't want a random sigpipe triggering pselect

    struct sigaction graceful, shutdown;
    memset(&graceful,0,sizeof(graceful));
    memset(&shutdown,0,sizeof(shutdown));
    graceful.sa_handler = handle_graceful;
    shutdown.sa_handler = handle_shutdown;
    err = sigaction(SIGHUP,&graceful,0);
    check_under_zero(err,"Error setting up action for sighup\n");
    err = sigaction(SIGINT,&shutdown,0);
    check_under_zero(err,"Error setting up action for sigint\n");
    err = sigaction(SIGQUIT,&shutdown,0);
    check_under_zero(err,"Error setting up action for sigquit\n");

    //-----------------------Workers------------------------------//

    pthread_t workerz[configFile.Nslaves];
    for(int i = 0;i < configFile.Nslaves;i++){
        workerz[i] = 0;
        safe_pthread_create(workerz[i],worker_thread,NULL,1);
    }

    //-----------------------Select setup-------------------------//

    fd_set set, tmpset;
    FD_ZERO(&set);
    FD_ZERO(&tmpset);
    FD_SET(serverfd,&set);
    printf("%d\n",getpid());
    fflush(stdout);
    pthread_t cID = 0;
    while(serverMode){
        tmpset = set;
        if(serverMode == ON) {
            err = pselect(FD_SETSIZE, &tmpset, NULL, NULL, NULL, &oldmask);
            if (err < 0 && errno != EINTR) {
                perror("Error select:");
                exit(EXIT_FAILURE);
            }
        }
        if (err && serverMode == ON){
            int client_fd = accept(serverfd,NULL,0);
            if (client_fd < 0){
                perror("Error:");
                exit(EXIT_FAILURE);
            }
            _log("Client connected!");
            Client args;
            args.client_fd = client_fd;
            args.queue_of_response = initList();
            safe_pthread_create(cID,client_thread,&args,1);

        }
        if (err && serverMode == GRACEFUL){
            _log("Arrived SIGHUP!\n");
            flood_list(workList,configFile.Nslaves);
            wake_up_all_readers(workList);
            wait_on_worker_number();
            //sleep(5);
            break;
        }


    }

   // pthread_join(cID,(void **)NULL);

    printFiles();
    print_server_general();
    clean_cache();

    closeLog();
    safe_free(workList);

    return 0;


}
