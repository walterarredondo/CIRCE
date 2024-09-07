#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "connection.h"


// Function to initialize server configuration with defaults
struct server_config initialize_config() {
    struct server_config config;
    config.port = DEFAULT_PORT;
    config.reuse_address = 1;
    config.listen_queue_size = DEFAULT_QUEUE_SIZE;
    strcpy(config.ip_address, DEFAULT_IP);
    return config;
}

// Getter and Setter functions to update configuration parameters
void set_port(struct server_config *config, int port) {
    config->port = port;
}

int get_port(struct server_config *config) {
    return config->port;
}

void set_ip_address(struct server_config *config, const char *ip_address) {
    strncpy(config->ip_address, ip_address, sizeof(config->ip_address)-1);
    config->ip_address[sizeof(config->ip_address)-1] = '\0';
}

const char* get_ip_address(struct server_config *config) {
    return config->ip_address;
}

void set_reuse_address(struct server_config *config, int reuse) {
    config->reuse_address = reuse;
}

int get_reuse_address(struct server_config *config) {
    return config->reuse_address;
}

void set_listen_queue_size(struct server_config *config, int size) {
    config->listen_queue_size = size;
}

int get_listen_queue_size(struct server_config *config) {
    return config->listen_queue_size;
}

// Function to create a socket
int create_socket() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    return server_fd;
}

// Function to set socket options based on the configuration
void set_socket_options(int server_fd, struct server_config *config) {
    if (config->reuse_address) {
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("setsockopt failed");
            exit(EXIT_FAILURE);
        }
    }
}

// Function to bind the socket using the configuration
void bind_socket(int server_fd, struct sockaddr_in *address, struct server_config *config) {
    address->sin_family = AF_INET;
    address->sin_addr.s_addr = inet_addr(config->ip_address);  // Use IP from config
    address->sin_port = htons(config->port);  // Use port from config

    if (bind(server_fd, (struct sockaddr*)address, sizeof(*address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
}

// Function to start listening with configurable queue size
void start_listening(int server_fd, struct server_config *config) {
    if (listen(server_fd, config->listen_queue_size) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }
    printf("Server is listening...\n");
}

// Function to accept a new connection
int accept_connection(int server_fd, struct sockaddr_in *address) {
    socklen_t addrlen = sizeof(*address);
    int new_socket = accept(server_fd, (struct sockaddr*)address, &addrlen);
    if (new_socket < 0) {
        perror("accept failed");
        exit(EXIT_FAILURE);
    }
    return new_socket;
}

