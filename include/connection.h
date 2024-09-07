#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define DEFAULT_PORT 1234
#define DEFAULT_IP "127.0.0.1"
#define DEFAULT_QUEUE_SIZE 3

// Structure to hold server configuration options
struct server_config {
    int port;
    char ip_address[16];  // For storing IPv4 address
    int reuse_address;
    int listen_queue_size;
};

// Function declarations

// Initialize server configuration with default values
struct server_config initialize_config();

// Setters and Getters for server configuration parameters
void set_port(struct server_config *config, int port);
int get_port(struct server_config *config);

void set_ip_address(struct server_config *config, const char *ip_address);
const char* get_ip_address(struct server_config *config);

void set_reuse_address(struct server_config *config, int reuse);
int get_reuse_address(struct server_config *config);

void set_listen_queue_size(struct server_config *config, int size);
int get_listen_queue_size(struct server_config *config);

// Socket-related functions
int create_socket();
void set_socket_options(int server_fd, struct server_config *config);
void bind_socket(int server_fd, struct sockaddr_in *address, struct server_config *config);
void start_listening(int server_fd, struct server_config *config);
int accept_connection(int server_fd, struct sockaddr_in *address);

#endif // SERVER_UTILS_H