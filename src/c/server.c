#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "server.h"
#include <stdbool.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <client.h>

#define PORT 1234
#define MAX_BUFFER 512


int main(int argc, char const* argv[]) {
    
    int server_fd, new_socket;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);

    //creates a new socket and return a file descriptor, returns -1 if it fails
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Configuring the socket to reuse address
    int opt = 1 ;// true for enabling reuseaddress
    if (setsockopt(server_fd, SOL_SOCKET,
                   SO_REUSEADDR, &opt,
                   sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr*)&address,
             sizeof(address))
        < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    while (true){
        pthread_t ptid; 
        
        printf("Server is Listening...\n");

        //prepare to accept the connection
        if (listen(server_fd, 3) < 0) {
            perror("listen failure");
            exit(EXIT_FAILURE);
        }
        
        //waits for connection on socket
        if ((new_socket
            = accept(server_fd, (struct sockaddr*)&address,
                    &addrlen))
            < 0) {
            perror("accept failure");
            exit(EXIT_FAILURE);
        }
        printf("Client connected: %i\n" , new_socket);

        // Pass the new_socket handle to the client_manager function
        int *new_sock_ptr = malloc(sizeof(int));
        if (new_sock_ptr == NULL) {
            perror("malloc failure");
            exit(EXIT_FAILURE);
        }
        *new_sock_ptr = new_socket;




        pthread_create(&ptid, NULL, &listener, new_sock_ptr); 

    }

    // closing the connected socket
    close(new_socket);
    // closing the listening socket
    close(server_fd);


    return 0;
}

void* listener(void* arg){
    int new_socket = *(int*)arg;
    free(arg);

    ssize_t valread;
    char buffer[512] = { 0 };
    pthread_detach(pthread_self()); 

    while (true)
    {
        valread = read(new_socket, buffer, MAX_BUFFER-1); // subtract 1 for the null
        printf("%s\n", buffer);
        send(new_socket, buffer, strlen(buffer), 0);
        printf("echo message sent\n");
    }

}


Server Server_init(){
    Server server;
    return server;    
}


char* Server_acceptClient() {
    static char response[] = "{ \"type\": \"RESPONSE\", \"operation\": \"IDENTIFY\", \"result\": \"SUCCESS\", \"extra\": \"Kimberly\" }";
    return response;
}



