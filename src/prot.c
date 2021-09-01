
#include "../headers/utils.h"


//If send fails returns -1 else returns the number of bytes wrote
int send_number(int to_fd,unsigned long int number){
    errno = 0;
    ssize_t err = send(to_fd,(void *)&number,sizeof(unsigned long int),0);
    if (err == -1){
        fprintf(stderr,"Error sending number message!\n");
        perror("Error : ->");
        return -1;
    }
    return err;
}
//if successfull returns the number of bytes else returns 0
unsigned long int recv_number(int from_fd){
    unsigned long int msg;
    ssize_t err = recv(from_fd,(void *)&msg,sizeof(unsigned long int),0);
    if (err == -1){
        //fprintf(stderr,"Error receving number message!\n");
        return 0;
    }
    return msg;
}

//Returns -1 bytes haven't been completely wrote
int send_message(int to_fd,char *message,size_t msg_size){

    int err = send_number(to_fd,(unsigned long int) msg_size);
    if (err == -1){
        exit(EXIT_FAILURE);
    }
    size_t wroteBytes = writen(to_fd,message,msg_size);
    if (wroteBytes != msg_size){
        fprintf(stderr,"Message hasn't been sent fully %zu / %zu \n",wroteBytes,msg_size);
        return -1;
    }
    return 0;

}
//If it fails returns NULL
char * recv_message(int from_fd){

    ssize_t msgSize = recv_number(from_fd);
    if (msgSize == -1){
        return NULL;
    }
    char * buffer = (char *) safe_malloc(sizeof(char) * (msgSize+1));
    ssize_t byteRead = readn(from_fd,buffer,msgSize);
    if (byteRead != msgSize){
        fprintf(stderr,"Message hasn't been read fully");
        safe_free(buffer);
        return NULL;
    }
    return buffer;
}

const char * error_to_str(int err){
    switch (err) {

        case ENOENT: return "No file has been found\n";
        case EEXIST: return "File already exists\n";
        case EFAULT: return "Buffer not received fully\n";
        case EACCES: return "File is not open\n";
        case ENODATA: return "There aren't enough files in server\n";
        case E2BIG: return "File is to big to be wrote in server memory\n";

    }
    errno = err;
    perror("Error");
    return "Uncommented error\n";
}