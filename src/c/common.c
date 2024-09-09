#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include "json_utils.h"
#include "common.h"
#define MAX_BUFFER 8192
#define TYPE_MAX_LENGHT 32
#define USER_MAX_LENGHT 9

void *listener(void *arg){
    listener_args_t *args = (listener_args_t *)arg;
    int new_socket = args->socket;
    process_message_func process_message = args->process_message; 

    pthread_detach(pthread_self());

    free(arg);
    ssize_t valread = 1;
    char buffer[MAX_BUFFER] = { 0 };
    char msg_type[TYPE_MAX_LENGHT] = { 0 };

    while (valread != 0){
        valread = read(new_socket, buffer, MAX_BUFFER); // subtract 1 for the null
        if(valread <= 0){
            continue;
        }
        if(!json_field_matches(buffer,"type",msg_type,sizeof(msg_type))){
            printf("not a valid json: field 'type' missing\n");
            continue;
        }
       process_message(new_socket, buffer, msg_type);
    }

    close(new_socket);
    return NULL;
}
