#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include <pthread.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "client.h"
#include "json_utils.h"
#include "connection.h"
#define PORT 1234
#define LOCALHOST "127.0.0.1"
#define MAX_BUFFER 8192
#define TYPE_MAX_LENGHT 32
#define USER_MAX_LENGHT 9


int main(int argc, char const* argv[]) {
    int new_socket;
    struct sockaddr_in address; 
    struct server_config config = initialize_config();

    int client_fd = create_socket();
    set_socket_options(client_fd,&config);
    //bind_socket(client_fd, &address, &config);
    setup_server_address(&config, &address);
    connect_to_server(client_fd, &address);

    char username[50];  // Maximum username length of 49 characters (plus null terminator)
    ask_for_username(username, sizeof(username));

    identify_client(client_fd, username);


    sleep(10);


    printf("closing the client, goodbye");
    // Close the socket
    close(client_fd);

    return 0; 
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

int identify_client(int sock, char *user){
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
