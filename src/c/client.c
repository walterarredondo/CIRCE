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
#include "logger.h"
#define MAX_BUFFER 2048
#define MAX_BUFFER_LOG 1024
#define TYPE_MAX_LENGHT 32
#define VALUE_MAX_LENGHT 32
#define USER_MAX_LENGHT 9
#define FIELD_OPERATION "operation"
#define FIELD_RESULT "result"
#define FIELD_EXTRA "extra"
#define VALUE_IDENTIFY "IDENTIFY"
#define RESULT_SUCCESS "SUCCESS"
#define RESULT_USER_ALREADY_EXISTS "USER_ALREADY_EXISTS"

static const char* PATH = "./log/client_logger";

volatile sig_atomic_t stop = 0;

int main(int argc, char const* argv[]) {
    struct sockaddr_in address; 
    struct server_config config = initialize_config();
    int sock;
    char buffer[MAX_BUFFER];
    char command[16];
    char msg[MAX_BUFFER];
    bool running = true;
    setbuf(stdout, NULL); 
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
                log_print_file_message(PATH, LOG_ERROR,"Invalid input. Commands should start with '\\'.");
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

        log_file_formatted_message(PATH, LOG_INFO,"received json: %s",buffer);
        if(!json_field_matches(buffer,"type",msg_type,sizeof(msg_type))){
            log_file_message(PATH, LOG_ERROR,"not a valid json: field 'type' missing");
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


//void handle_unknown(Client *client, int socket, char *buffer);

void process_message(Client *client, int socket, char *buffer, const char *msg_type) {
    // Client-specific message processing
    log_file_formatted_message(PATH, LOG_INFO,"Client processing message: %s", buffer);
    MessageType type = get_type(msg_type);
    switch (type) {
        case TYPE_NEW_USER:
            handle_new_user(client, socket, buffer);
            break;
        case TYPE_NEW_STATUS:
            handle_new_status(client, socket, buffer);
            break;
        case TYPE_USER_LIST:
            handle_user_list(client, socket, buffer);
            break;
        case TYPE_TEXT_FROM:
            handle_text_from(client, socket, buffer);
            break;
        case TYPE_PUBLIC_TEXT_FROM:
            handle_public_text_from(client, socket, buffer);
            break;
        case TYPE_JOINED_ROOM:
            handle_joined_room(client, socket, buffer);
            break;
        case TYPE_ROOM_USER_LIST:
            handle_room_user_list(client, socket, buffer);
            break;
        case TYPE_ROOM_TEXT_FROM:
            handle_room_text_from(client, socket, buffer);
            break;
        case TYPE_LEFT_ROOM:
            handle_left_room(client, socket, buffer);
            break;
        case TYPE_DISCONNECTED:
            handle_disconnected(client, socket, buffer);
            break;
        case TYPE_RESPONSE:
            handle_response(client, socket, buffer);
            break;
        case TYPE_UNKNOWN:
        default:
            //handle_unknown(client, socket, buffer);
            break;
    }
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
        log_file_formatted_message(PATH, LOG_INFO, "JSON sent to the server: %s", json_str);
    } else {
        log_file_message(PATH, LOG_ERROR,"Failed to build JSON ID.");
        return 0;
    }
    return 1;
}



// Get input from the user
bool get_input(char *buffer) {
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
        log_print_prompt(LOG_USER, "Command not found", "");
        return CMD_UNKNOWN;
    }
}

void handle_sigint(int sig){
    //implement a way to close each socket, and close each thread
    const char *goodbye = "\nLeaving... Goodbye!";
    log_print_file_message(PATH,LOG_INFO,goodbye);
    stop = 1;
}




void handle_new_user(Client *client, int socket, char *buffer) {
    // TODO: Implement handling for new user
}

void handle_new_status(Client *client, int socket, char *buffer) {
    // TODO: Implement handling for new status
}

void handle_user_list(Client *client, int socket, char *buffer) {
    // TODO: Implement handling for user list
}

void handle_text_from(Client *client, int socket, char *buffer) {
    // TODO: Implement handling for text from user
}

void handle_public_text_from(Client *client, int socket, char *buffer) {
    // TODO: Implement handling for public text from user
}

void handle_joined_room(Client *client, int socket, char *buffer) {
    // TODO: Implement handling for joined room
}

void handle_room_user_list(Client *client, int socket, char *buffer) {
    // TODO: Implement handling for room user list
}

void handle_room_text_from(Client *client, int socket, char *buffer) {
    // TODO: Implement handling for room text from user
}

void handle_left_room(Client *client, int socket, char *buffer) {
    // TODO: Implement handling for left room
}

void handle_disconnected(Client *client, int socket, char *buffer) {
    // TODO: Implement handling for disconnection
}

void handle_response(Client *client, int socket, char *json_str) {
    char operation[VALUE_MAX_LENGHT] = { 0 };
    json_extract_field_value(json_str, FIELD_OPERATION, operation, VALUE_MAX_LENGHT);

    if (strcmp(operation, "IDENTIFY") == 0) {
        handle_identify_response(client, socket, json_str);
    } else if (strcmp(operation, "LEAVE_ROOM") == 0) {
        handle_leave_room_response(client, socket, json_str);
    } else if (strcmp(operation, "ROOM_TEXT") == 0) {
        handle_room_text_response(client, socket, json_str);
    } else if (strcmp(operation, "ROOM_USERS") == 0) {
        handle_room_users_response(client, socket, json_str);
    } else if (strcmp(operation, "JOIN_ROOM") == 0) {
        handle_join_room_response(client, socket, json_str);
    } else if (strcmp(operation, "INVITE") == 0) {
        handle_invite_response(client, socket, json_str);
    } else if (strcmp(operation, "TEXT") == 0) {
        handle_text_response(client, socket, json_str);
    } else {
        handle_unknown_operation_response(client, socket, json_str);
        log_file_formatted_message(PATH, LOG_ERROR,"wrong response. Received: ", json_str);
    } 
}

void handle_identify_response(Client *client, int socket, char *json_str) {
    char value[VALUE_MAX_LENGHT] = { 0 };
    json_extract_field_value(json_str, FIELD_RESULT,value, VALUE_MAX_LENGHT);
    if(strcmp(value, RESULT_SUCCESS) == 0){
        char extra[VALUE_MAX_LENGHT];
        json_extract_field_value(json_str, FIELD_EXTRA,extra, VALUE_MAX_LENGHT);
        log_print_prompt(LOG_USER,"Welcome, %s!", extra);
    }else if(strcmp(value, RESULT_USER_ALREADY_EXISTS) == 0){
        log_file_formatted_message(PATH, LOG_ERROR,"Identification failed. Received: %s", json_str);
        log_print_prompt(LOG_USER,"User already taken. Choose another one and log in","");
    }

}


void handle_leave_room_response(Client *client, int socket, char *json_str){

}
void handle_room_text_response(Client *client, int socket, char *json_str)
{

}
void handle_room_users_response(Client *client, int socket, char *json_str)
{

}
void handle_join_room_response(Client *client, int socket, char *json_str)
{

}
void handle_invite_response(Client *client, int socket, char *json_str)
{

}
void handle_text_response(Client *client, int socket, char *json_str)
{

}
void handle_unknown_operation_response(Client *client, int socket, char *json_str)
{

}
