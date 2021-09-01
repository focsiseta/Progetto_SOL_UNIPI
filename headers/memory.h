

#ifndef _MEMORY_H
#define _MEMORY_H


#include "utils.h"
#include "parser.h"


typedef struct __memory_element{

    FILE * file;
    char * buffer;
    size_t usedMemory;
    char filename[MAXNAME];
    int isOpen;
    pthread_mutex_t lockFile;
    struct __memory_element * next;
    struct __memory_element * prev;

}FileMemory;

typedef struct _server__ {

    size_t nCurrentMemoryUsed;
    size_t MaxMemory;
    unsigned int nCurrentFile;
    unsigned int MaxFiles;
    FileMemory * memory;
    pthread_mutex_t memoryLock;
    pthread_mutex_t metadataLock;
    int alg_trigger;
    unsigned int MaxRecvFile;
    unsigned int MaxUsedMemory;

}Server_metadata;

#define lock_metadata_and_memory         lock(&serverMetadata.metadataLock);\
                                         lock(&serverMetadata.memoryLock)

#define unlock_metadata_and_memory         unlock(&serverMetadata.metadataLock);\
                                         unlock(&serverMetadata.memoryLock)
void initMemory(config_t conf);
void free_file(FileMemory *file);
FileMemory * create_file(char * _filename);
void * add_file_to_memory(FileMemory * file);
FileMemory * find_file(char * filename,cache_policy_t policy);
void printFiles(void);
int inMem(char *filename);
FileMemory * pop_from_tail(void);
void set_file_open(FileMemory * file);
void set_file_close(FileMemory * file);
int remove_generic_file(FileMemory * file);
void print_server_general(void);
void clean_cache(void);
void cache_algorithm(int number_of_files,size_t amount_of_memory);
size_t write_to_file(FileMemory * file,char * buffer, size_t size);
unsigned int get_actual_files(void);
FileMemory * get_head(void);
FileMemory * next(FileMemory * start);
void rebuild(void);

#endif
