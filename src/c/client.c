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

#define PORT 1234
#define LOCALHOST "127.0.0.1"
#define MAX_BUFFER 8192
#define TYPE_MAX_LENGHT 32
#define USER_MAX_LENGHT 9


int main(int argc, char const* argv[]) {
    int sock;
    struct sockaddr_in server_addr;
    char *message = "hello";

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error\n");
        return 1;
    }

    // Set up the server address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, LOCALHOST, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported\n");
        return 1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed\n");
        return 1;
    }



    char username[50];  // Maximum username length of 49 characters (plus null terminator)
    ask_for_username(username, sizeof(username));

    identify_client(sock, username);


    for (size_t i = 0; i < 10; i++)
    {
        // Send the message "hello" to the server
        sleep(1);
        //printf("Message %i sent: %s\n", i,  message);
    }


    printf("closing the client, goodbye");
    

    // Close the socket
    close(sock);

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
