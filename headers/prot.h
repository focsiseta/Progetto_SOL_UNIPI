//
// Created by focsiseta on 7/31/21.
//

#ifndef _PROT_H
#define _PROT_H

char * recv_message(int from_fd);
int send_message(int to_fd,const char *message,size_t msg_size);
unsigned long int recv_number(int from_fd);
int send_number(int to_fd,unsigned long int number);
const char * error_to_str(int err);

#define O_CREATE 0x01

#define OPEN_FILE 0x05
#define CLOSE_FILE 0x06
#define READ_FILE 0x07
#define WRITE_FILE 0x08
#define READN_FILE 0x09
#define APPEND_FILE 0x0A
#define REMOVE_FILE 0x0B
#define CLOSE_CONNECTION 0x0C

#define OK 0x01
#define NOT_OK 0x00
#define INITED 0x123
#define CONTINUE 0x321
#define NO_CONTINUE 0x432

#endif
