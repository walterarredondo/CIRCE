#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include <pthread.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <signal.h>
#include "client.h"
#include "json_utils.h"
#include "connection.h"
#define PORT 1234
#define LOCALHOST "127.0.0.1"
#define MAX_BUFFER 8192
#define TYPE_MAX_LENGHT 32
#define USER_MAX_LENGHT 9

volatile sig_atomic_t stop = 0;

int main(int argc, char const* argv[]) {
    struct sockaddr_in address; 
    struct server_config config = initialize_config();
    int sock;
    char buffer[MAX_BUFFER];
    char command[16];
    char msg[MAX_BUFFER];
    bool running = true;
    
    sock = create_socket();
    set_socket_options(sock,&config);
    setup_server_address(&config, &address);
    signal(SIGINT, handle_sigint);
    if (connect_to_server(sock, &address) && create_listener(sock)){
        while (running && !stop) {
            if (!get_input(buffer)) {
                continue;
            }

            if (parse_command(buffer, command, msg)) {
                if (is_leave_command(command)) {
                    running = false;
                } else {
                    execute_command(sock, command, msg);
                }
            } else {
                printf("Invalid input. Commands should start with '\\'.\n");
            }
        } 
    }
    close(sock);
    return 0;
}

void *listener(void *arg){
    listener_args_t *args = (listener_args_t *)arg;
    int sock = args->socket;
    Client *client = args->client;
    free(arg);

    pthread_detach(pthread_self());

    ssize_t valread = 1;
    char buffer[MAX_BUFFER];
    char msg_type[TYPE_MAX_LENGHT];

    while (valread != 0){
        memset(buffer, 0, sizeof(buffer)); 
        memset(msg_type, 0, sizeof(msg_type)); 
        valread = read(sock, buffer, MAX_BUFFER);
        if(valread <= 0){
            continue;
        }
        printf("echo:\n%s\n",buffer);
        if(!json_field_matches(buffer,"type",msg_type,sizeof(msg_type))){
            printf("not a valid json: field 'type' missing\n");
            continue;
        }
       process_message(client, sock, buffer, msg_type);
    }
    close(sock);
    return NULL;
}

int create_listener(int sock){
    pthread_t ptid; 
    listener_args_t *args = malloc(sizeof(listener_args_t));
    if (args == NULL) {
        perror("malloc failure");
        exit(EXIT_FAILURE);
    }
    args->socket = sock;
    args->process_message = process_message; 
    pthread_create(&ptid, NULL, &listener, (void *)args); 
    return 1;
}


// Execute the parsed command and pass the arguments
void execute_command(int sock, const char *command, const char *args) {
    switch (get_command_type(command)) {
        case CMD_HELP:
            handle_help(args);
            break;
        case CMD_ECHO:
            handle_echo(args);
            break;
        case CMD_LOGIN:
            handle_login(sock, args);
            break;
        default:
            handle_unknown(command);
            break;
    }
}


void handle_login(int sock, const char *username){
    //char username[50];  // Maximum username length of 49 characters (plus null terminator)
    //ask_for_username(username, sizeof(username));
    identify_client(sock, username);
}

void process_message(Client *client, int socket, char *buffer, const char *msg_type){
    // Client-specific message processing
    printf("Client processing message:\n%s\n", buffer);
}



void ask_for_username(char *username, int max_len) {
    printf("Enter your username: ");
    fgets(username, max_len, stdin);

    // Remove trailing newline character, if any
    size_t len = strlen(username);
    if (len > 0 && username[len - 1] == '\n') {
        username[len - 1] = '\0';
    }
}

int identify_client(int sock, const char *user){
    char json_str[256] = "";  // Start with an empty JSON string
    const char *fields_and_values[][2] = {
        {"type", "IDENTIFY"},
        {"username", user}
    };

    size_t num_fields = sizeof(fields_and_values) / sizeof(fields_and_values[0]);

    if (build_json_response(json_str, sizeof(json_str), fields_and_values, num_fields)) {
        send(sock, json_str, strlen(json_str), 0);
        printf("JSON sent to the server:\n%s\n", json_str);
    } else {
        printf("Failed to build JSON ID.\n");
        return 0;
    }
    return 1;
}



// Get input from the user
bool get_input(char *buffer) {
    printf("> ");
    return fgets(buffer, MAX_BUFFER, stdin) != NULL;
}

// Parse the command and arguments from the input buffer
bool parse_command(const char *buffer, char *command, char *args) {
    if (sscanf(buffer, "%s %[^\n]", command, args) >= 1 && command[0] == '\\') {
        return true;
    }
    return false;
}

// Check if the command is '\leave'
bool is_leave_command(const char *command) {
    return strcmp(command, "\\leave") == 0;
}

// Handle the '\help' command
void handle_help(const char *args) {
    printf("Available commands:\n");
    printf("\\help - Show this help message\n");
    printf("\\echo <text> - Echo the provided text\n");
    printf("\\login USER - send identification message to the server\n");
    printf("\\leave - Exit the program\n");
}

// Handle the '\echo' command
void handle_echo(const char *args) {
    printf("Echo: %s\n", args);
}

// Handle unknown commands
void handle_unknown(const char *command) {
    printf("Unknown command: %s\n", command);
}

enum Command get_command_type(const char* command) {
    if (strcmp(command, "\\help") == 0) {
        return CMD_HELP;
    } else if (strcmp(command, "\\echo") == 0) {
        return CMD_ECHO;
    } else if (strcmp(command, "\\login") == 0) {
        return CMD_LOGIN;
    } else {
        return CMD_UNKNOWN;
    }
}

void handle_sigint(int sig){
    //implement a way to close each socket, and close each thread
    printf("\nleaving...\ngoodbye!\n");
    stop = 1;
}