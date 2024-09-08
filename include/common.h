#ifndef COMMON_H
#define COMMON_H

#define MAX_BUFFER 8192
#define TYPE_MAX_LENGHT 32
#define USER_MAX_LENGHT 9




typedef void (*process_message_func)(int, char *, const char *);

typedef struct {
    int socket;
    process_message_func process_message;
} listener_args_t;


void *listener(void *arg);
#endif