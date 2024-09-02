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
#include "server.h"

#define PORT 1234
#define MAX_BUFFER 512
#define LOCALHOST "127.0.0.2"


int main(int argc, char const* argv[]) {
    int sock;
    struct sockaddr_in server_addr;
    char *message = "hello";

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return 1;
    }

    // Set up the server address struct
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, LOCALHOST, &server_addr.sin_addr) <= 0) {
        perror("Invalid address/ Address not supported");
        return 1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Connection failed");
        return 1;
    }


    for (size_t i = 0; i < 10; i++)
    {
        // Send the message "hello" to the server
        sleep(1);
        send(sock, message, strlen(message), 0);
        printf("Message sent: %s\n", message);
    }
    

    // Close the socket
    close(sock);

    return 0; 
}

