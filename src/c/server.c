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
#include "logger.h"

static const char* PATH = "./log/server_logger";

// Flag to indicate if Ctrl+C (SIGINT) was caught
static volatile sig_atomic_t stop = 0;

int main(int argc, char const* argv[]) {
    int new_socket;
    struct sockaddr_in address; 
    GList *thread_pool = NULL;
    setbuf(stdout, NULL); 
    Server *server = server_init(); 



    struct server_config* config = initialize_config();
    char ip_address[16] = "";  
    int port = 0;             
    if (argc > 1 && parse_arguments(argc, argv, ip_address, &port) == 0) {
        if(strlen(ip_address)>0){
            set_ip_address(config,ip_address);
            log_server_message(PATH, LOG_SUCCESS,"IP address set to: %s", ip_address);
        } else if (port != 0) {
            set_port(config, port);
            log_server_message(PATH, LOG_SUCCESS,"Port set to: %i", port);
        }
    }

    int server_fd = create_socket();
    set_socket_options(server_fd,config);
    bind_socket(server_fd, &address, config);
    signal(SIGINT, handle_sig);
    while (!stop){
        start_listening(server_fd, config);
        log_server_message(PATH, LOG_SUCCESS, "Server is listening...");
        int new_socket = accept_connection(server_fd, &address);
        UserInfo *user = initialize_user(new_socket);
        thread_pool = create_thread_pool(server, user, thread_pool);
    }
    log_server_message(PATH, LOG_INFO,"quitting gracefully. goodbye!");
    g_list_free_full(thread_pool, free); 
    close(server_fd);
    return EXIT_SUCCESS;
}



UserInfo *initialize_user(int sock){
    UserInfo *user = (UserInfo *)malloc(sizeof(UserInfo));
    if (user == NULL) {
        // Manejo de error si malloc falla
        fprintf(stderr, "Error al asignar memoria\n");
        exit(EXIT_FAILURE);
    }
    user->socket = sock;
    return user;
}

void *server_listener(void *arg){
    server_listener_args_t *args = (server_listener_args_t *)arg;
    UserInfo *user_info = args->user_info;
    Server *server = args->server;
    int sock = user_info->socket;

    if (server->user_table == NULL) {
        log_server_message(PATH, LOG_ERROR, "Error while creating user_table");
        return NULL;
    }


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
        user_info->socket = sock;
        process_message(server, user_info, buffer, msg_type);
    }
    server_remove_user(server,user_info);
    log_server_message(PATH, LOG_SUCCESS, "User removed correctly");
    free(arg);
    return NULL;
}


void process_message(Server *server, UserInfo *user_info, char *buffer, const char *message_type) {
    // Use strcmp to compare strings and switch-case for handling various message types
    log_server_message(PATH, LOG_INFO,"Server processing message: %s", buffer);
    MessageType type = server_get_type(message_type);
    switch (type) {
        case TYPE_IDENTIFY:
            handle_identify(server, user_info, buffer);
            break;
        case TYPE_STATUS:
            handle_status(server, user_info, buffer);
            break;
        case TYPE_USERS:
            handle_users();
            break;
        case TYPE_TEXT:
            handle_text();
            break;
        case TYPE_PUBLIC_TEXT:
            handle_public_text(server, user_info, buffer);
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
            handle_disconnect(server, user_info);
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



void handle_identify(Server *server, UserInfo *user_info, char* buffer){
    char username[USER_MAX_LENGHT];  
    int sock = user_info->socket;
    if(!parse_user(user_info, buffer, username, USER_MAX_LENGHT)){
        log_server_message(PATH, LOG_ERROR, "Idetification failed");
        close(sock);
        return;
    }
    UserInfo *found_user = g_hash_table_lookup(server->user_table, username);
    if (found_user) {
        if(!identify_response_failed(user_info, username)){
            log_server_message(PATH, LOG_ERROR, "failed sending the failed identification response.");
        }
        log_server_message(PATH, LOG_ERROR, "User already exists, choose another one %s", found_user->username);
    } else {
        //adding user to the server
        server_add_user(server, user_info, username,1,sock);
        if(!identify_response_success(user_info)){
            log_server_message(PATH, LOG_ERROR,"Response to identification failed");
        }
        log_server_message(PATH, LOG_SUCCESS,"User '%s' added to the server user list.", user_info->username);
        new_user_identified_response(server, user_info);
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

void handle_status(Server *server, UserInfo *user_info, char *json_str) {
    char status[MAX_INPUT_MSG]; 
    if(!json_extract_field_value(json_str, "status", status, MAX_INPUT_MSG)){
        log_server_message(PATH, LOG_ERROR, "Invalid JSON");
        return;
    }
    if(!isValidStatus(status))
    {
        log_server_message(PATH, LOG_ERROR, "Invalid STATUS");
        return;
    };
    const char *fields_and_values[][2] = {
        {"type", "STATUS"},
        {"status",status},
    };
    size_t num_fields = sizeof(fields_and_values) / sizeof(fields_and_values[0]);
    send_json_except_user(server,user_info, fields_and_values, num_fields);
    log_server_message(PATH, LOG_SUCCESS,"All responses were sent to clients.");
    
}

int isValidStatus(char *status) {
    switch (get_status(status)) {
        case ACTIVE:
        case AWAY:
        case BUSY:
            return 1; // Valid statuses
        default:
            return 0; // Invalid status
    }
}

void handle_users() {
    printf("Handling USERS\n");
}

void handle_text() {
    printf("Handling TEXT\n");
}

void handle_public_text(Server *server, UserInfo *user_info, const char *json_str) {
    char msg[MAX_INPUT_MSG]; 
    if(!json_extract_field_value(json_str, "text", msg, MAX_INPUT_MSG)){
        log_server_message(PATH, LOG_ERROR, "Invalid JSON");
        return;
    }
    const char *fields_and_values[][2] = {
        {"type", "PUBLIC_TEXT_FROM"},
        {"username", user_info->username},
        {"text",msg}
    };
    size_t num_fields = sizeof(fields_and_values) / sizeof(fields_and_values[0]);

    send_json_except_user(server,user_info, fields_and_values, num_fields);
    log_server_message(PATH, LOG_SUCCESS,"All responses were sent to clients.");

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

void handle_disconnect(Server *server, UserInfo *user_info) {
    close(user_info->socket);
    log_server_message(PATH, LOG_SUCCESS, "Disconnection successful");
    print_user_table(server);
}


int parse_user(UserInfo *user_info, char* json_str, char * username, size_t max_size){
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

int identify_response_success(UserInfo *user_info){
    char json_str[256] = "";  // Start with an empty JSON string

    const char *fields_and_values[][2] = {
        {"type", "RESPONSE"},
        {"operation", "IDENTIFY"},
        {"result", "SUCCESS"},
        {"extra", user_info->username}
    };
    size_t num_fields = sizeof(fields_and_values) / sizeof(fields_and_values[0]);
    return send_json_response(user_info->socket, json_str, sizeof(json_str), fields_and_values, num_fields);
}


int identify_response_failed(UserInfo *user_info, const char *user){
    char json_str[256] = "";  // Start with an empty JSON string
    const char *fields_and_values[][2] = {
        {"type", "RESPONSE"},
        {"operation", "IDENTIFY"},
        {"result", "USER_ALREADY_EXISTS"},
        {"extra", user}
    };
    size_t num_fields = sizeof(fields_and_values) / sizeof(fields_and_values[0]);
    return send_json_response(user_info->socket, json_str, sizeof(json_str), fields_and_values, num_fields);
}

void new_user_identified_response(Server *server, UserInfo *user_info){
        const char *fields_and_values[][2] = {
            {"type", "NEW_USER"},
            {"username", user_info->username}
        };
        size_t num_fields = sizeof(fields_and_values) / sizeof(fields_and_values[0]);

        send_json_except_user(server,user_info, fields_and_values, num_fields);
        log_server_message(PATH, LOG_SUCCESS,"All responses were sent to clients.");
}



void send_json_except_user(Server *server, UserInfo *user_info, const char *fields_and_values[][2], size_t num_fields) {
    GHashTableIter iter;
    gpointer key, value;
    char json_str[1024] = ""; // Buffer for the JSON string response
    const char *exclude_username =  user_info->username;
    
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
UserInfo* create_user(UserInfo *user,const char *username, int status, int socket) {
    //UserInfo *user = g_new(UserInfo, 1);
    user->username = g_strdup(username);  // Create a copy of the username
    user->status = status;      // Create a copy of the status
    user->socket = socket;
    return user;
}

// Function to free the memory allocated for a user
void free_user(gpointer data) {
    UserInfo *user = (UserInfo *)data;
    g_free(user);
}


// Function to add a user to the server's user table
void server_add_user(Server *server, UserInfo *user_info, char *username, int status, int socket) {
    user_info = create_user(user_info, username, status, socket);
    g_hash_table_insert(server->user_table, user_info->username, user_info);
}

// Function to remove a user from the server's user table
void server_remove_user(Server *server, UserInfo *user_info) {
    gboolean removed = g_hash_table_remove(server->user_table, user_info->username);
    if (!removed) {
        log_server_message(PATH, LOG_ERROR, "error removing");
    }
}

static void handle_sig(int _){
    (void)_;
    //implement a way to close each socket, and close each thread
    log_server_message(PATH, LOG_INFO,"\ngoodbye...\n");
    stop = 1;
    kill(-getpgrp(), SIGQUIT); 
}

GList *create_thread_pool(Server *server, UserInfo *user, GList *thread_list) {
    pthread_t ptid; 
    // Allocate memory for listener arguments
    server_listener_args_t *args = malloc(sizeof(server_listener_args_t));
    if (args == NULL) {
        perror("malloc failure");
        exit(EXIT_FAILURE);
    }

    // Set the arguments for the listener
    args->server = server;
    args->user_info = user;

    // Create the new thread
    if (pthread_create(&ptid, NULL, &server_listener, (void *)args) != 0) {
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


MessageType server_get_type(const char *message_type) {
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


// Function to print each key-value pair
void print_entry(gpointer key, gpointer value, gpointer user_data) {
    log_server_message(PATH, LOG_INFO,"USER: %s", (char *)key);
}

// Function to print all entries in the GHashTable
void print_user_table(Server *srv) {
    if (srv->user_table != NULL) {
        log_server_message(PATH, LOG_SUCCESS,"Printing Users:");
        g_hash_table_foreach(srv->user_table, print_entry, NULL);
    } else {
        log_server_message(PATH, LOG_ERROR,"Table is NULL");
    }
}