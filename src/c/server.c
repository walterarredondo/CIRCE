#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <glib.h>
#include "server.h"
#include "json_utils.h"
#include "connection.h"
#include "logger.h"
#define MAX_BUFFER 2048
#define TYPE_MAX_LENGHT 32
#define USER_MAX_LENGHT 9

static const char* PATH = "./log/server_logger";

// Flag to indicate if Ctrl+C (SIGINT) was caught
static volatile sig_atomic_t stop = 0;

int main(int argc, char const* argv[]) {
    int new_socket;
    struct sockaddr_in address; 
    Server *server = server_init(); 
    GList *thread_list = NULL;
    setbuf(stdout, NULL); 


    struct server_config config = initialize_config();
    int server_fd = create_socket();
    set_socket_options(server_fd,&config);
    bind_socket(server_fd, &address, &config);
    signal(SIGINT, handle_sigint);
    while (!stop){
        start_listening(server_fd, &config);
        new_socket = accept_connection(server_fd, &address);
        thread_list = create_listener_thread(server, new_socket, thread_list);
    }
    log_server_message(PATH, LOG_INFO,"quitting gracefully. goodbye!");
    g_list_free_full(thread_list, free); 
    close(server_fd);
    return EXIT_SUCCESS;
}


void *listener(void *arg){
    listener_args_t *args = (listener_args_t *)arg;
    int sock = args->socket;
    Server *server = args->server;

    if (server->user_table == NULL) {
        log_server_message(PATH, LOG_ERROR, "Error while creating user_table");
        return NULL;
    }

    free(arg);

    pthread_detach(pthread_self());

    ssize_t valread = 1;
    char buffer[MAX_BUFFER] = { 0 };
    char msg_type[TYPE_MAX_LENGHT] = { 0 };

    while (valread != 0){
        valread = read(sock, buffer, MAX_BUFFER);
        if(valread <= 0){
            continue;
        }

        // Ensure null-termination after the last character read
        if (valread < MAX_BUFFER) {
            buffer[valread] = '\0';  // Set the last character to null terminator
        } else {
            buffer[MAX_BUFFER - 1] = '\0';  // Safeguard if buffer is filled
        }
        log_server_message(PATH, LOG_INFO,"received: %s",buffer);
        if(!json_field_matches(buffer,"type",msg_type,sizeof(msg_type))){
            log_server_message(PATH, LOG_ERROR, "not a valid json: field 'type' missing");
            continue;
        }
       process_message(server, sock, buffer, msg_type);
    }
    close(sock);
    return NULL;
}


void process_message(Server *server, int sock, char *buffer, const char *message_type) {
    // Use strcmp to compare strings and switch-case for handling various message types
    log_server_message(PATH, LOG_INFO,"Server processing message: %s", buffer);
    MessageType type = get_type(message_type);
    switch (type) {
        case TYPE_IDENTIFY:
            handle_identify(server, sock, buffer);
            break;
        case TYPE_STATUS:
            handle_status();
            break;
        case TYPE_USERS:
            handle_users();
            break;
        case TYPE_TEXT:
            handle_text();
            break;
        case TYPE_PUBLIC_TEXT:
            handle_public_text();
            break;
        case TYPE_NEW_ROOM:
            handle_new_room();
            break;
        case TYPE_INVITE:
            handle_invite();
            break;
        case TYPE_JOIN_ROOM:
            handle_join_room();
            break;
        case TYPE_ROOM_USERS:
            handle_room_users();
            break;
        case TYPE_ROOM_TEXT:
            handle_room_text();
            break;
        case TYPE_LEAVE_ROOM:
            handle_leave_room();
            break;
        case TYPE_DISCONNECT:
            handle_disconnect();
            break;
        case TYPE_RESPONSE:
            handle_response();
            break;
        case TYPE_UNKNOWN:
        default:
            log_server_message(PATH, LOG_INFO, "Unknown message type: %s", message_type);
            break;
    }
}



void handle_identify(Server *server, int sock, char* buffer){
    char username[USER_MAX_LENGHT];  
    if(!parse_user(sock, buffer, username, USER_MAX_LENGHT)){
        log_server_message(PATH, LOG_ERROR, "Idetification failed");
        close(sock);
        return;
    }
    UserInfo *found_user = g_hash_table_lookup(server->user_table, username);
    if (found_user) {
        if(!identify_response_failed(sock,username)){
            log_server_message(PATH, LOG_ERROR, "failed sending the failed identification response.");
        }
        log_server_message(PATH, LOG_ERROR, "User already exists, choose another one %s", found_user->username);
    } else {
        server_add_user(server, username,"ONLINE",sock);
        if(!identify_response_success(sock,username)){
            log_server_message(PATH, LOG_ERROR,"Response to identification failed");
        }
        log_server_message(PATH, LOG_SUCCESS,"User '%s' added to the server user list.", username);
        send_json_except_user(server,username);
        log_server_message(PATH, LOG_SUCCESS,"All responses to clients were sent.");
    }

}

void handle_response() {
    //printf("Handling RESPONSE\n");
    //send(new_socket, buffer, strlen(buffer), 0);
    //printf("echo message sent\n");
}

void handle_new_user() {
    printf("Handling NEW_USER\n");
}

void handle_status() {
    printf("Handling STATUS\n");
}

void handle_users() {
    printf("Handling USERS\n");
}

void handle_text() {
    printf("Handling TEXT\n");
}

void handle_public_text() {
    printf("Handling PUBLIC_TEXT\n");
}

void handle_new_room() {
    printf("Handling NEW_ROOM\n");
}

void handle_invite() {
    printf("Handling INVITE\n");
}

void handle_join_room() {
    printf("Handling JOIN_ROOM\n");
}

void handle_room_users() {
    printf("Handling ROOM_USERS\n");
}

void handle_room_text() {
    printf("Handling ROOM_TEXT\n");
}

void handle_leave_room() {
    printf("Handling LEAVE_ROOM\n");
}

void handle_disconnect() {
    printf("Handling DISCONNECT\n");
}


int parse_user(int sock, char* json_str, char * username, size_t max_size){
    const char *required_fields[] = {"type", "username"};
    size_t fields_len = sizeof(required_fields) /  sizeof(required_fields[0]); 
    if(!verify_json_fields(json_str,required_fields,fields_len)){
        log_server_message(PATH, LOG_ERROR,"Missing fields");
    }

    char user[USER_MAX_LENGHT]; 
    const char *tag = "username";
    if(!json_extract_field_value(json_str, tag, user, USER_MAX_LENGHT) || strlen(user)==0 )
    {
        log_server_message(PATH, LOG_ERROR, "Invalid username lenght");
        return 0;
    }
    strncpy(username, user, USER_MAX_LENGHT - 1);
    username[USER_MAX_LENGHT-1] = '\0'; // Ensure null-termination
    log_server_message(PATH, LOG_SUCCESS,"Client identified succesfully");
    return 1;
}

int identify_response_success(int sock, char *user){
    char json_str[256] = "";  // Start with an empty JSON string
    const char *fields_and_values[][2] = {
        {"type", "RESPONSE"},
        {"operation", "IDENTIFY"},
        {"result", "SUCCESS"},
        {"extra", user}
    };
    size_t num_fields = sizeof(fields_and_values) / sizeof(fields_and_values[0]);
    return send_json_response(sock, json_str, sizeof(json_str), fields_and_values, num_fields);
}


int identify_response_failed(int sock, char *user){
    char json_str[256] = "";  // Start with an empty JSON string
    const char *fields_and_values[][2] = {
        {"type", "RESPONSE"},
        {"operation", "IDENTIFY"},
        {"result", "USER_ALREADY_EXISTS"},
        {"extra", user}
    };
    size_t num_fields = sizeof(fields_and_values) / sizeof(fields_and_values[0]);
    return send_json_response(sock, json_str, sizeof(json_str), fields_and_values, num_fields);
}


void send_json_except_user(Server *server, const char *exclude_username) {
    GHashTableIter iter;
    gpointer key, value;
    char json_str[1024] = ""; // Buffer for the JSON string response
    const char *fields_and_values[][2] = {
        {"type", "NEW_USER"},
        {"username", exclude_username}
    };
    size_t num_fields = sizeof(fields_and_values) / sizeof(fields_and_values[0]);
    g_hash_table_iter_init(&iter, server->user_table);
    while (g_hash_table_iter_next(&iter, &key, &value)) {
        UserInfo *user = (UserInfo *)value;
        // Skip the user with the provided username
        if (g_strcmp0(user->username, exclude_username) == 0) {
            continue;
        }
        if (!send_json_response(user->socket, json_str, sizeof(json_str), fields_and_values,num_fields)) {
            log_server_message(PATH, LOG_ERROR,"Failed to send JSON response to user: %s", user->username);
        }
    }
}




int send_json_response(int sock, char *json_str, size_t json_str_size ,const char *fields_and_values[][2], size_t num_fields) {
    // Calculate the number of fields based on the size of fields_and_values
    // Build the JSON response

    if (build_json_response(json_str, json_str_size, fields_and_values, num_fields)) {
        // Send JSON response to the client
        send(sock, json_str, strlen(json_str), 0);
        log_server_message(PATH, LOG_SUCCESS,"JSON sent to the client:%s", json_str);
        return 1; // Success
    } else {
        log_server_message(PATH, LOG_ERROR,"Failed to build JSON response.");
        return 0; // Failure
    }
}



Server* server_init() {
    Server *server = (Server*)malloc(sizeof(Server));
    if (server == NULL) {
        // Handle memory allocation failure
        return NULL;
    }

    // Initialize the user table
    server->user_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, free_user);
    if (server->user_table == NULL) {
        // Handle hash table creation failure
        log_server_message(PATH, LOG_ERROR,"error null user_table");
        free(server);
        return NULL;
    }
    return server;
}

// Function to clean up the server and its user table
void server_cleanup(Server *server) {
    g_hash_table_destroy(server->user_table);
    g_free(server);
}


// Function to create a new user
UserInfo* create_user(const char *username, const char *status, int socket) {
    UserInfo *user = g_new(UserInfo, 1);
    user->username = g_strdup(username);  // Create a copy of the username
    user->status = g_strdup(status);      // Create a copy of the status
    user->socket = socket;
    return user;
}

// Function to free the memory allocated for a user
void free_user(gpointer data) {
    UserInfo *user = (UserInfo *)data;
    g_free(user->username);
    g_free(user->status);
    g_free(user);
}


// Function to add a user to the server's user table
void server_add_user(Server *server, const char *username, const char *status, int socket) {
    g_hash_table_insert(server->user_table, g_strdup(username), create_user(username, status, socket));
}

// Function to remove a user from the server's user table
void server_remove_user(Server *server, const char *username) {
    g_hash_table_remove(server->user_table, username);
}

static void handle_sigint(int _){
    (void)_;
    //implement a way to close each socket, and close each thread
    log_server_message(PATH, LOG_INFO,"\ngoodbye...\n");
    stop = 1;
    kill(-getpgrp(), SIGQUIT); 
}

GList *create_listener_thread(Server *server, int new_socket, GList *thread_list) {
    pthread_t ptid; 
    // Allocate memory for listener arguments
    listener_args_t *args = malloc(sizeof(listener_args_t));
    if (args == NULL) {
        perror("malloc failure");
        exit(EXIT_FAILURE);
    }

    // Set the arguments for the listener
    args->server = server;
    args->socket = new_socket;

    // Create the new thread
    if (pthread_create(&ptid, NULL, &listener, (void *)args) != 0) {
        perror("pthread_create failure");
        exit(EXIT_FAILURE);
    }

    // Copy the thread ID
    pthread_t *thread_copy = malloc(sizeof(pthread_t));
    if (thread_copy == NULL) {
        perror("malloc failure");
        exit(EXIT_FAILURE);
    }
    *thread_copy = ptid;

    // Append the thread to the thread list
    thread_list = g_list_append(thread_list, thread_copy);

    return thread_list;
}

MessageType get_type(const char *message_type) {
    if (strcmp(message_type, "IDENTIFY") == 0) {
        return TYPE_IDENTIFY;
    } else if (strcmp(message_type, "RESPONSE") == 0) {
        return TYPE_RESPONSE;
    } else if (strcmp(message_type, "STATUS") == 0) {
        return TYPE_STATUS;
    } else if (strcmp(message_type, "USERS") == 0) {
        return TYPE_USERS;
    } else if (strcmp(message_type, "TEXT") == 0) {
        return TYPE_TEXT;
    } else if (strcmp(message_type, "PUBLIC_TEXT") == 0) {
        return TYPE_PUBLIC_TEXT;
    } else if (strcmp(message_type, "NEW_ROOM") == 0) {
        return TYPE_NEW_ROOM;
    } else if (strcmp(message_type, "INVITE") == 0) {
        return TYPE_INVITE;
    } else if (strcmp(message_type, "JOIN_ROOM") == 0) {
        return TYPE_JOIN_ROOM;
    } else if (strcmp(message_type, "ROOM_USERS") == 0) {
        return TYPE_ROOM_USERS;
    } else if (strcmp(message_type, "ROOM_TEXT") == 0) {
        return TYPE_ROOM_TEXT;
    } else if (strcmp(message_type, "LEAVE_ROOM") == 0) {
        return TYPE_LEAVE_ROOM;
    } else if (strcmp(message_type, "DISCONNECT") == 0) {
        return TYPE_DISCONNECT;
    } else {
        return TYPE_UNKNOWN;
    }
}