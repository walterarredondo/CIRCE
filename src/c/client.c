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
#include <ctype.h>
#include <glib.h> 
#include "client.h"
#include "json_utils.h"
#include "logger.h"

static const char* PATH = "./log/client_logger";

volatile sig_atomic_t stop = 0;

int main(int argc, char const* argv[]) {
    struct sockaddr_in address; 
    int sock;
    char buffer[MAX_BUFFER];
    char command[16];
    char msg[MAX_BUFFER];
    bool running = true;
    setbuf(stdout, NULL);

    char ip_address[16] = "";  
    int port = 0;             
    struct server_config* config = initialize_config();
    if (argc > 1 && parse_arguments(argc, argv, ip_address, &port) == 0) {
        if(strlen(ip_address)>0){
            set_ip_address(config,ip_address);
            log_file_formatted_message(PATH, LOG_SUCCESS,"IP address set to: %s", ip_address);
        } else if (port != 0) {
            set_port(config, port);
            log_file_formatted_message(PATH, LOG_SUCCESS,"Port set to: %i", port);
        }
    }

    sock = create_socket();
    set_socket_options(sock,config);
    setup_server_address(config, &address);
    signal(SIGINT, handle_sigint);
    Client *client = client_init(sock);;
    client_listener_args_t args;
    if (connect_to_server(sock, &address) && client_create_listener(client, &client_process_message, &args, &client_listener)){
        log_print_prompt(LOG_USER,"Connected to server");
        while (running && !stop) {
            if (!get_input(buffer)) {
                continue;
            }
            g_strstrip(buffer); 
            if (parse_command(buffer, command, msg)) {
                int count;
                if (is_leave_command(command)) {
                    running = false;
                } else {
                    count = count_tokens(msg, " ");
                    execute_command(client, command, msg, count);
                }
            } else {
                if(strlen(buffer) == 0){
                    log_print_prompt(LOG_USER,"Invalid input. Commands should start with '\\'.");
                    continue;
                }
                if(client->logged_in){
                    //send message
                    char *str = strcat(command, " ");
                    char *str2 = strcat(command, msg);
                    send_public_text(client,str2);
                    printf("\033[F\033[K");
                    print_my_msg(command);
                } else {
                    log_print_prompt(LOG_USER, "To send messages you need to login first!");
                }
            }
            buffer[0] = '\0';
            command[0] = '\0';
            msg[0] = '\0';
        } 
    }
    free(client);
    free(config);
    return 0;
}

void *client_listener(void *arg){
    client_listener_args_t *args = (client_listener_args_t *)arg;
    int sock = args->socket;
    Client *client = args->client;

    pthread_detach(pthread_self());

    ssize_t valread = 1;
    char buffer[MAX_BUFFER] = { 0 };
    char msg_type[TYPE_MAX_LENGHT] = { 0 };

    while (valread != 0){
        valread = read(sock, buffer, MAX_BUFFER);
        if(valread <= 0){
            continue;
        }
        g_strstrip(buffer); 
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
       client_process_message(client, buffer, msg_type);
    }
    free(arg);
    close(sock);
    return NULL;
}

// Execute the parsed command and pass the arguments
void execute_command(Client *client, const char *command, const char *args, int n_params) {
    int sock = client->socket;
    bool logged = client->logged_in;
    switch (get_command_type(command)) {
        case CMD_HELP:
            handle_help(args);
            break;
        case CMD_ECHO:
            handle_echo(args);
            break;
        case CMD_LOGIN:
            handle_login(client, args, n_params);
            break;
        case CMD_LOGOUT:
            handle_logout(client);
            break;
        case CMD_STATUS:
            handle_status_client(client, args, n_params);
            break;
        default:
            handle_unknown(command);
            break;
    }
}

Client *client_init(int sock)
{
    Client* client = (Client*)malloc(sizeof(Client));
    if (client == NULL) {
        perror("malloc failure");
        exit(EXIT_FAILURE);
    }
    client->logged_in = false;
    client->socket = sock;
    client->status = ACTIVE; // Set default status if needed
    return client;
}

void handle_login(Client *client, const char *username, int n_params){
    int sock = client->socket;
    bool logged = client->logged_in;
    if(logged){
        log_print_prompt(LOG_USER,"You're already logged in. Log out first.");
        return;
    }
    if(n_params == 1 && strlen(username) > 2 && strlen(username) < 9){
        identify_client(sock, username);
    } else {
        log_print_prompt(LOG_USER,"Username has to be between 3 and 8 characters long");
        log_file_message(PATH, LOG_ERROR, "Invalid username");
    }
}

void handle_logout(Client *client){
    bool logged = client->logged_in;
    if(!logged){
        log_print_prompt(LOG_USER,"You're already logged out. Log out first.");
        return;
    }
    if(de_identify_client(client)){
        log_print_prompt(LOG_USER,"You're logged out");
        login_client(client,false,ACTIVE,"");
    }
}

void handle_status_client(Client *client, const char *status, int params){
    int sock = client->socket;
    bool logged = client->logged_in;
    Status stat;
    if(params != 1 || (stat = get_status(status)) == INVALID){
        log_server_message(PATH, LOG_USER, "not a valid status");
    }
    update_status(client, status);
}

int update_status(Client *client, const char* status){
    client->status = get_status(status);
    int sock = client->socket;
    char json_str[256] = "";  // Start with an empty JSON string
    const char *fields_and_values[][2] = {
        {"type", "STATUS"},
        {"status", status}
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




void client_process_message(Client *client,  char *buffer, const char *msg_type) {
    int socket = client->socket;
    log_file_formatted_message(PATH, LOG_INFO,"Client processing message: %s", buffer);
    ClientMessageType type = client_get_type(msg_type);
    switch (type) {
        case TYPE_NEW_USER:
            handle_client_new_user(client, buffer);
            break;
        case TYPE_NEW_STATUS:
            handle_new_status(client, buffer);
            break;
        case TYPE_USER_LIST:
            handle_user_list(client, buffer);
            break;
        case TYPE_TEXT_FROM:
            handle_text_from(client, buffer);
            break;
        case TYPE_PUBLIC_TEXT_FROM:
            handle_public_text_from(client, buffer);
            break;
        case TYPE_JOINED_ROOM:
            handle_joined_room(client, buffer);
            break;
        case TYPE_ROOM_USER_LIST:
            handle_room_user_list(client, buffer);
            break;
        case TYPE_ROOM_TEXT_FROM:
            handle_room_text_from(client, buffer);
            break;
        case TYPE_LEFT_ROOM:
            handle_left_room(client, buffer);
            break;
        case TYPE_DISCONNECTED:
            handle_disconnected(client);
            break;
        case TYPE_CLIENT_RESPONSE:
            handle_client_response(client, buffer);
            break;
        case TYPE_NOT_KNOWN:
        default:
            //handle_unknown(client, buffer);
            break;
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


int de_identify_client(Client *client){
    int sock = client->socket;
    char json_str[256] = "";  // Start with an empty JSON string
    const char *fields_and_values[][2] = {
        {"type", "DISCONNECT"},
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

int send_public_text(Client *client, char *msg){
    int sock = client->socket;
    char json_str[256] = "";  // Start with an empty JSON string
    const char *fields_and_values[][2] = {
        {"type", "PUBLIC_TEXT"},
        {"text",msg}
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

int count_tokens(const char *str, const char *delim) {
    char *token;
    char str_copy[256];  // Ensure the string is long enough for the copy
    int count = 0;

    // Copy the string, since strtok modifies the original string
    strncpy(str_copy, str, sizeof(str_copy));
    str_copy[sizeof(str_copy) - 1] = '\0';  // Make sure it's null-terminated

    // Get the first token
    token = strtok(str_copy, delim);

    // Walk through other tokens
    while (token != NULL) {
        count++;
        token = strtok(NULL, delim);
    }

    return count;
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
    printf("\\logout - logout from the server\n");
    printf("\\status ACTIVE/AWAY/BUSY - change the current status to\n");
    printf("\\leave - Exit the program\n");
}

// Handle the '\echo' command
void handle_echo(const char *args) {
    log_print_prompt(LOG_USER,"%s\n", args);
}

// Handle unknown commands
void handle_unknown(const char *command) {
}



void handle_client_new_user(Client *client,  char *json_str) {
    // TODO: Implement handling for new user
    char user[9];
    json_extract_field_value(json_str,FIELD_USERNAME,user,USER_MAX_LENGHT);
    log_print_prompt(LOG_USER, "\n%s joined the chat", user);
}

void handle_new_status(Client *client,  char *buffer) {
    // TODO: Implement handling for new status
}

void handle_user_list(Client *client,  char *buffer) {
    // TODO: Implement handling for user list
}

void handle_text_from(Client *client,  char *buffer) {
    // TODO: Implement handling for text from user
}

void handle_public_text_from(Client *client,  char *json_str) {
    char msg[MAX_INPUT_MSG], user[USER_MAX_LENGHT]; 
    if(!json_extract_field_value(json_str, "text", msg, MAX_INPUT_MSG) || !json_extract_field_value(json_str, "username", user, USER_MAX_LENGHT)){
        log_server_message(PATH, LOG_ERROR, "Invalid JSON");
        return;
    }
    printf("\n");
    printf("\033[F\033[K");
    print_msg(user, msg);
}

void handle_joined_room(Client *client,  char *buffer) {
    // TODO: Implement handling for joined room
}

void handle_room_user_list(Client *client,  char *buffer) {
    // TODO: Implement handling for room user list
}

void handle_room_text_from(Client *client,  char *buffer) {
    // TODO: Implement handling for room text from user
}

void handle_left_room(Client *client,  char *buffer) {
    // TODO: Implement handling for left room
}

void handle_disconnected(Client *client) {
    
    //receive disconnect notification
}

void handle_client_response(Client *client,  char *json_str) {
    int socket = client->socket;
    char operation[VALUE_MAX_LENGHT] = { 0 };
    json_extract_field_value(json_str, FIELD_OPERATION, operation, VALUE_MAX_LENGHT);

    if (strcmp(operation, "IDENTIFY") == 0) {
        handle_identify_response(client, json_str);
    } else if (strcmp(operation, "LEAVE_ROOM") == 0) {
        handle_leave_room_response(client, json_str);
    } else if (strcmp(operation, "ROOM_TEXT") == 0) {
        handle_room_text_response(client, json_str);
    } else if (strcmp(operation, "ROOM_USERS") == 0) {
        handle_room_users_response(client, json_str);
    } else if (strcmp(operation, "JOIN_ROOM") == 0) {
        handle_join_room_response(client, json_str);
    } else if (strcmp(operation, "INVITE") == 0) {
        handle_invite_response(client, json_str);
    } else if (strcmp(operation, "TEXT") == 0) {
        handle_text_response(client, json_str);
    } else {
        handle_unknown_operation_response(client, json_str);
        log_file_formatted_message(PATH, LOG_ERROR,"wrong response. Received: ", json_str);
    } 
}

void handle_identify_response(Client *client,  char *json_str) {
    int socket = client->socket;
    char value[VALUE_MAX_LENGHT] = { 0 };
    json_extract_field_value(json_str, FIELD_RESULT,value, VALUE_MAX_LENGHT);
    if(strcmp(value, RESULT_SUCCESS) == 0){
        char extra[9] = { 0 };
        json_extract_field_value(json_str, FIELD_EXTRA,extra, VALUE_MAX_LENGHT);
        log_print_prompt(LOG_USER,"Welcome, %s!", extra);
        login_client(client,true,ACTIVE,extra);
    }else if(strcmp(value, RESULT_USER_ALREADY_EXISTS) == 0){
        log_file_formatted_message(PATH, LOG_ERROR,"Identification failed. Received: %s", json_str);
        log_print_prompt(LOG_USER,"User already taken. Choose another one and log in","");
    }

}


void handle_leave_room_response(Client *client,  char *json_str){

}
void handle_room_text_response(Client *client,  char *json_str)
{

}
void handle_room_users_response(Client *client,  char *json_str)
{

}
void handle_join_room_response(Client *client,  char *json_str)
{

}
void handle_invite_response(Client *client,  char *json_str)
{

}
void handle_text_response(Client *client,  char *json_str)
{

}

void handle_unknown_operation_response(Client *client,  char *json_str)
{

}

void login_client(Client *client, bool logged, Status status, const char *user) {
    update_client(client, logged, status);
    snprintf(client->username, sizeof(client->username), "%s", user);
}
void update_client(Client *client, bool logged, Status status) {
    client->logged_in = logged;  // true
    client->status = status;
}

ClientMessageType client_get_type(const char *message_type) {
    if (strcmp(message_type, "NEW_USER") == 0) {
        return TYPE_NEW_USER;
    } else if (strcmp(message_type, "STATUS_UPDATE") == 0) {
        return TYPE_NEW_STATUS;
    } else if (strcmp(message_type, "USER_LIST") == 0) {
        return TYPE_USER_LIST;
    } else if (strcmp(message_type, "TEXT_FROM") == 0) {
        return TYPE_TEXT_FROM;
    } else if (strcmp(message_type, "PUBLIC_TEXT_FROM") == 0) {
        return TYPE_PUBLIC_TEXT_FROM;
    } else if (strcmp(message_type, "JOINED_ROOM") == 0) {
        return TYPE_JOINED_ROOM;
    } else if (strcmp(message_type, "ROOM_USER_LIST") == 0) {
        return TYPE_ROOM_USER_LIST;
    } else if (strcmp(message_type, "ROOM_TEXT_FROM") == 0) {
        return TYPE_ROOM_TEXT_FROM;
    } else if (strcmp(message_type, "LEFT_ROOM") == 0) {
        return TYPE_LEFT_ROOM;
    } else if (strcmp(message_type, "DISCONNECTED") == 0) {
        return TYPE_DISCONNECTED;
    } else if (strcmp(message_type, "RESPONSE") == 0) {
        return TYPE_CLIENT_RESPONSE;
    } else {
        return TYPE_NOT_KNOWN;
    }
}

enum Command get_command_type(const char* command) {
    if (strcmp(command, "\\help") == 0) {
        return CMD_HELP;
    } else if (strcmp(command, "\\echo") == 0) {
        return CMD_ECHO;
    } else if (strcmp(command, "\\login") == 0) {
        return CMD_LOGIN;
    } else if (strcmp(command, "\\logout") == 0) {
        return CMD_LOGOUT;
    } else if (strcmp(command, "\\status") == 0) {
        return CMD_STATUS;
    } else {
        log_print_prompt(LOG_USER, "Command not found");
        return CMD_UNKNOWN;
    }
}



int client_create_listener(Client *client, client_process_message_func process_message, client_listener_args_t *args, void *(*listener)(void *)) {
    pthread_t ptid; 
    args = malloc(sizeof(client_listener_args_t));
    if (args == NULL) {
        perror("malloc failure");
        exit(EXIT_FAILURE);
    }
    args->client = client;
    args->socket = client->socket;
    args->client_process_message = process_message; 
    pthread_create(&ptid, NULL, listener, (void *)args); 
    return 1;
}

//void handle_not_known(Client *client,  char *buffer);

void handle_sigint(int sig){
    //implement a way to close each socket, and close each thread
    const char *goodbye = "\nLeaving... Goodbye!";
    log_print_file_message(PATH,LOG_INFO,goodbye);
    stop = 1;
}