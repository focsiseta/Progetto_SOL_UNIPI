#include "../headers/memory.h"
#include "../headers/server.h"
#include "../headers/log.h"

void initMemory(config_t conf) {

    serverMetadata.MaxFiles = conf.max_files;
    serverMetadata.MaxMemory = conf.max_memory;
    serverMetadata.alg_trigger = 0;
    serverMetadata.nCurrentFile = 0;
    serverMetadata.nCurrentMemoryUsed = 0;
    serverMetadata.MaxRecvFile = 0;
    serverMetadata.MaxUsedMemory = 0;
    int err = pthread_mutex_init(&serverMetadata.memoryLock, NULL);
    if (err != 0) {
        _log("Error initializing mutex on main memory\n");
        fprintf(stderr,"Error initializing mutex in initMemory\n");
        exit(EXIT_FAILURE);
    }
    err = pthread_mutex_init(&serverMetadata.metadataLock, NULL);
    if (err != 0) {
        _log("Error initializing mutex on main metadata\n");
        fprintf(stderr,"Error initializing mutex in initMemory\n");
        exit(EXIT_FAILURE);
    }
    serverMetadata.memory = NULL;
}

unsigned int get_actual_files(void){
    lock(&serverMetadata.metadataLock);
    unsigned int toReturn = serverMetadata.nCurrentFile;
    unlock(&serverMetadata.metadataLock);
    return toReturn;
}

//Creates file without checking if it already exists returns NULL if error occured
FileMemory * create_file(char * _filename){
    if (strlen(_filename) >= MAXNAME){
        fprintf(stderr,"Choosen name for the file is too long\n");
        return NULL;
    }
    FileMemory * file = (FileMemory *)safe_malloc(sizeof(FileMemory));
    if (file == NULL){
        fprintf(stderr,"Malloc has failed\n");
        _log("Closing server because malloc has failed.\n");
        exit(EXIT_FAILURE);
    }
    strncpy(file->filename,_filename, strlen(_filename)+1);
    file->file = open_memstream(&(file->buffer),&file->usedMemory);
    if (file->file == NULL){
        fprintf(stderr,"Open_Memsream has failed\n");
        _log("Closing server because open_memstream has failed.\n");
        exit(EXIT_FAILURE);
    }
    int err = pthread_mutex_init(&(file->lockFile),NULL);
    if (err != 0){
        fprintf(stderr,"pthread_mutex_init has failed during file creation.\n");
        _log("pthread mutex init has failed during file creation.\n");
        exit(EXIT_FAILURE);

    }
    file->next = NULL;
    file->prev = NULL;
    file->isOpen = 0;
    return file;

}

void set_file_open(FileMemory * file){
    if (file == NULL){
        fprintf(stderr,"Setting open on null file!\n");
        exit(EXIT_FAILURE);
    }
    lock(&(file->lockFile));
    file->isOpen = 1;
    unlock(&(file->lockFile));
}
void set_file_close(FileMemory * file){
    if (file == NULL){
        fprintf(stderr,"Setting close on null file!\n");
        exit(EXIT_FAILURE);
    }
    lock(&(file->lockFile));
    file->isOpen = 0;
    unlock(&(file->lockFile));
}

int equalNames(const char *str1, const char *str2){
    int cond = strncmp(str1,str2,strlen(str1));
    int cond2 = strncmp(str2,str1, strlen(str2));
    if ((cond == 0) && (cond2 == 0)){
        return 1;
    }
    return 0;
}
//Returns -1 if error, 0 if file isn't in memory and 1 if it is
int inMem(char *filename){
    if (filename == NULL) return -1;
    lock(&serverMetadata.memoryLock);
    FileMemory * cursor = serverMetadata.memory;
    int files = serverMetadata.nCurrentFile;
    for(int i = 0; i < files;i++){
        if (equalNames(cursor->filename,filename) == 1){
            unlock(&serverMetadata.memoryLock);
            return 1;
        }
        cursor = next(cursor);
        if (cursor == NULL ){
            break;
        }
    }
    unlock(&serverMetadata.memoryLock);
    return 0;

}
//Every time this function is called on file:
/*
 * Checks what type of policy and if :
 *  LRU: moves the file to the head because it has recently been used
 *      (So we can pop the oldest, seeing that our linked list is FIFO)
 *  FIFO: doesn't do anything because linked list is already FIFO
*/
void policy_routine(cache_policy_t policy,FileMemory *file){
    if (file == NULL) {
        fprintf(stderr,"Passed null file");
        return;
    }
    switch (policy) {

        case FIFO: return;
        case LRU: {
            //_log("LRU Policy has been chosen!\n");
            lock(&serverMetadata.memoryLock);
            //When it's first
            if (equalNames(serverMetadata.memory->filename,file->filename)){
                unlock(&serverMetadata.memoryLock);
                return;
            }

            FileMemory * cursor = serverMetadata.memory;
            FileMemory * prev = NULL;
            while(cursor != NULL){
                if(equalNames(cursor->filename,file->filename) == 1){
                    break;
                }
                prev = cursor;
                cursor = cursor->next;
            }
            FileMemory * next  = NULL;
            if (cursor->next != NULL) next = cursor->next;
            prev->next = next;
            cursor->next = serverMetadata.memory;
            serverMetadata.memory = cursor;
            unlock(&serverMetadata.memoryLock);
            break;
        }

    }
}
//if file isn't in memory returns null and sets errno to ENOENT
FileMemory * find_file(char * filename,cache_policy_t policy){
    errno = 0;
    if (filename == NULL){
        return NULL;
    }
    if (inMem(filename) == 0){
        errno = ENOENT;
        //fprintf(stderr,"Error trying to find a file that doesn't exist\n");
        _log("Searched a file that doesn't exist %s\n",filename);
        return NULL;
    }
    lock(&serverMetadata.memoryLock);
    FileMemory * cursor = serverMetadata.memory;
    while (cursor != NULL){
        if (equalNames(cursor->filename,filename) == 1){
            unlock(&serverMetadata.memoryLock);
            policy_routine(policy,cursor);
            return cursor;
        }
        cursor = cursor->next;
    }
    unlock(&serverMetadata.memoryLock);

    return NULL;


}
//If successfull returns the address of the added file else returns null
void * add_file_to_memory(FileMemory * file){
    //To head insert
    errno = 0;
    if (file == NULL) return NULL;
    if (inMem(file->filename)) {
        errno = EEXIST;
        fprintf(stderr,"Same file name detected!\n");
        return NULL;
    }
    if (get_actual_files() == 0){
        lock_metadata_and_memory;
        serverMetadata.memory = file;
        serverMetadata.nCurrentFile++;
        unlock_metadata_and_memory;

        return file;
    }
    lock_metadata_and_memory;
    FileMemory * tmp = serverMetadata.memory;
    serverMetadata.memory = file;
    serverMetadata.memory->prev = NULL;
    serverMetadata.memory->next = tmp;
    tmp->prev = file;
    serverMetadata.nCurrentFile++;
    unlock_metadata_and_memory;
    return file;
}

void printFiles(void){
    puts("===========FILE_PRINT============");
    FileMemory * head = get_head();
    for (int i = 0; i < get_actual_files();i++){
        puts(head->filename);
        head = next(head);
    }
    puts("===============================");
}

void free_file(FileMemory *file){
    //Decidere se far lockare alla funzione di pop i metadati e quindi sottrarre 1 o farlo direttamente a free,
    // secondo me meglio a free perche' nel momento in cui si chiama rimuove a prescindere dalla posizone
    if (file == NULL){
        return;
    }
    lock(&(file->lockFile));
    if (file->file != NULL) {
        fclose(file->file);
        file->file = NULL;
    }
    safe_free(file->buffer);
    unlock(&(file->lockFile));
    safe_free(file);
}
//Returns null if fails else returns the address of popped file
FileMemory * pop_from_tail(void){

    lock_metadata_and_memory;
    FileMemory * cursor = serverMetadata.memory;
    if (cursor == NULL){
        fprintf(stderr,"No file present!\n");
        unlock_metadata_and_memory;
        return NULL;
    }
    if (cursor->next == NULL){
        serverMetadata.memory = NULL;
        serverMetadata.nCurrentFile -= 1;
        serverMetadata.nCurrentMemoryUsed -= cursor->usedMemory;
        unlock_metadata_and_memory;
        return cursor;
    }
    FileMemory * prev = cursor;
    while(cursor != NULL && cursor->next != NULL){
        prev = cursor;
        cursor = cursor->next;
    }
    if (cursor->next != NULL){
        fprintf(stderr,"Selected not the last element");
        unlock_metadata_and_memory;
        return NULL;
    }

    if (prev != NULL) prev->next = NULL;
    cursor->prev = NULL;
    cursor->next = NULL;
    serverMetadata.nCurrentFile -= 1;
    serverMetadata.nCurrentMemoryUsed -= cursor->usedMemory;
    unlock_metadata_and_memory;
    return cursor;
}

//Removes file from list
FileMemory * pop_from_head(void){
    lock_metadata_and_memory;
    if (serverMetadata.memory == NULL){
        unlock_metadata_and_memory;
        fprintf(stderr,"Memory is empty!\n");
        return NULL;
    }
    FileMemory * to_ret = serverMetadata.memory;
    if (to_ret->next == NULL){
        serverMetadata.memory = NULL;
        serverMetadata.nCurrentFile -= 1;
        serverMetadata.nCurrentMemoryUsed -= to_ret->usedMemory;
        to_ret->next = NULL;
        to_ret->prev = NULL;
        unlock_metadata_and_memory;
        return to_ret;

    }
    serverMetadata.memory = to_ret->next;
    to_ret->next = NULL;
    to_ret->prev = NULL;
    serverMetadata.nCurrentFile -= 1;
    serverMetadata.nCurrentMemoryUsed -= to_ret->usedMemory;
    unlock_metadata_and_memory;
    return to_ret;


}
void alg_counter(void){
    lock(&(serverMetadata.metadataLock));
    serverMetadata.alg_trigger++;
    unlock(&(serverMetadata.metadataLock));
    _log("Called cache alg. !\n");
}
int remove_generic_file(FileMemory * file){
    if (inMem(file->filename) == 0){
        return -1;
    }
    find_file(file->filename,LRU);
    FileMemory * lol = pop_from_head();
    free_file(lol);
    //unlock_metadata_and_memory;
    return 1;

}
int is_action_legal(int number_of_files,int amount_of_memory){

    lock(&(serverMetadata.metadataLock));
    size_t current_memory = serverMetadata.nCurrentMemoryUsed;
    unsigned int current_files = serverMetadata.nCurrentFile;
    //If current memory + memory to add is > MaxMemory action is illegal
    //Same applies for current files + number files
    if (((current_memory + amount_of_memory) > serverMetadata.MaxMemory) ||
        ((current_files + number_of_files) > serverMetadata.MaxFiles)){
        unlock(&(serverMetadata.metadataLock));
        return 0;
    }
    if (serverMetadata.MaxUsedMemory < (serverMetadata.nCurrentMemoryUsed + amount_of_memory)){
        serverMetadata.MaxUsedMemory = (serverMetadata.nCurrentMemoryUsed + amount_of_memory);
    }
    if (serverMetadata.MaxRecvFile < (serverMetadata.nCurrentFile + number_of_files)){
        serverMetadata.MaxRecvFile = (serverMetadata.nCurrentFile + number_of_files);
    }
    unlock(&(serverMetadata.metadataLock));
    return 1;
}
void cache_algorithm(int number_of_files,size_t amount_of_memory){
    //Horrible locking because pop_from_tail needs to get lock on metadata so I need to make sure that when I call it the lock is unlocked
    errno = 0;
    int triggered  = 0;
    //If action is legal no need to call the cache algorithm
    if (is_action_legal(number_of_files,amount_of_memory)){
        errno = ECANCELED;
        return;
    }


    lock(&(serverMetadata.metadataLock));
    ssize_t number_of_files_to_remove = (int)((serverMetadata.nCurrentFile + number_of_files) - (int)serverMetadata.MaxFiles);
    unsigned int MaxFiles = serverMetadata.MaxFiles;
    size_t MaxMemory = serverMetadata.MaxMemory;
    unlock(&(serverMetadata.metadataLock));

    if (number_of_files > MaxFiles || amount_of_memory > MaxMemory){
        errno = E2BIG;
        return;
    }



    if (number_of_files_to_remove > 0 && (MaxFiles > number_of_files)){
        if (triggered == 0){
            alg_counter();
            triggered = 1;
        }
        for(int i = 0; i < number_of_files_to_remove;i++){ // Removes enough files to make room for the others
            FileMemory * to_remove = pop_from_tail();
            free_file(to_remove);

        }
    }
    lock(&(serverMetadata.metadataLock));
    size_t current_memory = serverMetadata.nCurrentMemoryUsed;
    ssize_t amount_of_memory_to_remove = (int)((current_memory + amount_of_memory) - (int)serverMetadata.MaxMemory);
    unlock(&(serverMetadata.metadataLock));
    //if amount_of_memory_to_remove is negative it means that MaxMemory is bigger than the whole occupied memory after OP,so it's legal to do OP
    //same principle applies above (kinda)
    if (amount_of_memory_to_remove > 0 && (MaxMemory > amount_of_memory)){
        if (triggered == 0){
            alg_counter();
            triggered = 1;
        }
        while(amount_of_memory_to_remove > 0 && get_actual_files() != 0){
            FileMemory * to_remove = pop_from_tail();
            current_memory -= to_remove->usedMemory;
            free_file(to_remove);
            amount_of_memory_to_remove = ( current_memory + amount_of_memory) - MaxMemory;
        }
    }
}

void print_server_general(void){
    printf("===\nCurrent files in memory %u / %u \nCurrent occupied memory %zu / %zu\nCurrent occupied memory (MB) %zu / %zu\nServer pid %d\nCache alg. has been triggered for %d times\nServer Max Recv Files %u\nServer Max Used Memory %u\n\n===",serverMetadata.nCurrentFile,
           serverMetadata.MaxFiles,serverMetadata.nCurrentMemoryUsed,
           serverMetadata.MaxMemory,
           (serverMetadata.nCurrentMemoryUsed/ 1000000),
           (serverMetadata.MaxMemory / 1000000),
           getpid(),
           serverMetadata.alg_trigger,
           serverMetadata.MaxRecvFile,
           serverMetadata.MaxUsedMemory);
}

void clean_cache(void){
    while(get_actual_files() != 0){
        FileMemory * tmp = pop_from_head();
        free_file(tmp);
    }
}

size_t write_to_file(FileMemory * file,char * buffer, size_t size){
    lock(&(serverMetadata.metadataLock));
    lock(&(file->lockFile));
    int wrote = 0;
    while (wrote < size){
        wrote += fwrite(buffer,sizeof(char),size,file->file);
        fflush(file->file);
    }
    serverMetadata.nCurrentMemoryUsed += wrote;
    unlock(&(file->lockFile));
    unlock(&(serverMetadata.metadataLock));
    return wrote;
}

FileMemory * get_head(void){
    lock(&(serverMetadata.memoryLock));
    FileMemory * head = serverMetadata.memory;
    unlock(&(serverMetadata.memoryLock));
    return head;
}
FileMemory * next(FileMemory * start){
    if (start == NULL) return NULL;
    return start->next;
}

