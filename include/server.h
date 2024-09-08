#ifndef CIRCE_SERVER_H
#define CIRCE_SERVER_H
#include <glib.h>


typedef struct {
    GHashTable *user_table;  // Hash table to manage users
} Server;

typedef struct {
    char *username;
    char *status;
    int socket;
} UserInfo;


Server *server_init();
void handle_sigint(int sig);
void server_cleanup(Server *server);
UserInfo* create_user(const char *username, const char *status, int socket);
void free_user(gpointer data);
void server_add_user(const char *username, const char *status, int socket);
void server_remove_user(const char *username);


char* Server_acceptClient();

int identify_user(int sock, char* buffer, char * id, size_t max_size);


//void *listener(void *arg);


void process_message(int, char *, const char *);
void handle_identify();
void handle_response();
void handle_new_user();
void handle_status();
void handle_users();
void handle_text();
void handle_public_text();
void handle_new_room();
void handle_invite();
void handle_join_room();
void handle_room_users();
void handle_room_text();
void handle_leave_room();
void handle_disconnect();



int identify_response_success(int sock, char *user);
int identify_response_failed(int sock, char *user);

#endif //CIRCE_SERVER_H