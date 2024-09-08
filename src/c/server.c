
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <glib.h>
#include "server.h"
#include "json_utils.h"
#include "connection.h"
#include "common.h"
#define PORT 1234
#define MAX_BUFFER 8192
#define TYPE_MAX_LENGHT 32
#define USER_MAX_LENGHT 9

Server *server;

// Flag to indicate if Ctrl+C (SIGINT) was caught
volatile sig_atomic_t stop = 0;

int main(int argc, char const* argv[]) {
    int new_socket;
    struct sockaddr_in address; 
    server = server_init(); 

    struct server_config config = initialize_config();
    int server_fd = create_socket();
    set_socket_options(server_fd,&config);
    bind_socket(server_fd, &address, &config);

    signal(SIGINT, handle_sigint);
    while (!stop){
        pthread_t ptid; 
        start_listening(server_fd, &config);
        new_socket = accept_connection(server_fd, &address);

        listener_args_t *args = malloc(sizeof(listener_args_t));
        if (args == NULL) {
            perror("malloc failure");
            exit(EXIT_FAILURE);
        }
        args->socket = new_socket;
        args->process_message = process_message; 
        pthread_create(&ptid, NULL, &listener, (void *)args); 
    }

    close(server_fd);
    return 0;
}


void process_message(int sock, char *buffer, const char *message_type) {
    // Use strcmp to compare strings and switch-case for handling various message types
    printf("type: %s\n",message_type);
    if (strcmp(message_type, "IDENTIFY") == 0) {
        handle_identify(sock, buffer);
    } else if (strcmp(message_type, "RESPONSE") == 0) {
        handle_response();
    } else if (strcmp(message_type, "NEW_USER") == 0) {
        handle_new_user();
    } else if (strcmp(message_type, "STATUS") == 0) {
        handle_status();
    } else if (strcmp(message_type, "USERS") == 0) {
        handle_users();
    } else if (strcmp(message_type, "TEXT") == 0) {
        handle_text();
    } else if (strcmp(message_type, "PUBLIC_TEXT") == 0) {
        handle_public_text();
    } else if (strcmp(message_type, "NEW_ROOM") == 0) {
        handle_new_room();
    } else if (strcmp(message_type, "INVITE") == 0) {
        handle_invite();
    } else if (strcmp(message_type, "JOIN_ROOM") == 0) {
        handle_join_room();
    } else if (strcmp(message_type, "ROOM_USERS") == 0) {
        handle_room_users();
    } else if (strcmp(message_type, "ROOM_TEXT") == 0) {
        handle_room_text();
    } else if (strcmp(message_type, "LEAVE_ROOM") == 0) {
        handle_leave_room();
    } else if (strcmp(message_type, "DISCONNECT") == 0) {
        handle_disconnect();
    } else {
        printf("Unknown message type: %s\n", message_type);
    }
}



void handle_identify(int sock, char* buffer){
    char username[USER_MAX_LENGHT];  
    printf("Handling IDENTIFY\n");
    if(!identify_user(sock, buffer, username, USER_MAX_LENGHT)){
        char* failed_id = "Idetification failed\n";
        close(sock);
        return;
    }
    printf("username is: %s\n",username);
    UserInfo *found_user = g_hash_table_lookup(server->user_table, username);
    if (found_user) {
        if(identify_response_failed(sock,username)){
            printf("failed sending the failed identification response\n");
        }
        printf("\nUser already exists, choose another one %s\n", found_user->username);
    } else {
        server_add_user(username,"ONLINE",sock);
        if(!identify_response_success(sock,username)){
            printf("Response to identification failed\n");
        }
        //send for each client connected response
        printf("\nUser %s added to the server user list.\n", username);
    }

}

void handle_response() {
    printf("Handling RESPONSE\n");
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


int identify_user(int sock, char* json_str, char * username, size_t max_size){
    const char *required_fields[] = {"type", "username"};
    size_t fields_len = sizeof(required_fields) /  sizeof(required_fields[0]); 
    if(!verify_json_fields(json_str,required_fields,fields_len)){
        printf("missing fields\n");
    }

    char user[USER_MAX_LENGHT]; 
    const char *tag = "username";
    if(!json_string_field_size(json_str,tag,user, max_size)){
        printf("invalid size of field\n");
        return 0;
    }
    strncpy(username, user, USER_MAX_LENGHT - 1);
    username[USER_MAX_LENGHT-1] = '\0'; // Ensure null-termination
    printf("client identified succesfully\n");
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

    if (build_json_response(json_str, sizeof(json_str), fields_and_values, num_fields)) {
        send(sock, json_str, strlen(json_str), 0);
        printf("JSON sent to the client:\n%s\n", json_str);
    } else {
        printf("Failed to build JSON response.\n");
        return 0;
    }
    return 1;
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

    if (build_json_response(json_str, sizeof(json_str), fields_and_values, num_fields)) {
        send(sock, json_str, strlen(json_str), 0);
        printf("JSON sent to the client:\n%s\n", json_str);
    } else {
        printf("Failed to build JSON response.\n");
        return 0;
    }
    return 1;
}


// Function to initialize the server and the user table
Server* server_init() {
    Server *server = g_new(Server, 1);
    server->user_table = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, free_user);
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
void server_add_user(const char *username, const char *status, int socket) {
    g_hash_table_insert(server->user_table, g_strdup(username), create_user(username, status, socket));
}

// Function to remove a user from the server's user table
void server_remove_user(const char *username) {
    g_hash_table_remove(server->user_table, username);
}

void handle_sigint(int sig){
    //implement a way to close each socket, and close each thread
    printf("\nleaving...\ngoodbye!\n");
    stop = 1;
}