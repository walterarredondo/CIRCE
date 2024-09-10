#ifndef CIRCE_SERVER_H
#define CIRCE_SERVER_H


typedef struct {
    GHashTable *user_table;  // Hash table to manage users
} Server;

typedef struct {
    char *username;
    char *status;
    int socket;
} UserInfo;


typedef void (*process_message_func)(Server *server, int, char *, const char *);

typedef struct {
    Server *server;
    int socket;
    process_message_func process_message;
} listener_args_t;

typedef enum {
    TYPE_IDENTIFY,
    TYPE_RESPONSE,
    TYPE_NEW_USER,
    TYPE_STATUS,
    TYPE_USERS,
    TYPE_TEXT,
    TYPE_PUBLIC_TEXT,
    TYPE_NEW_ROOM,
    TYPE_INVITE,
    TYPE_JOIN_ROOM,
    TYPE_ROOM_USERS,
    TYPE_ROOM_TEXT,
    TYPE_LEAVE_ROOM,
    TYPE_DISCONNECT,
    TYPE_UNKNOWN
} MessageType;

Server *server_init();
MessageType get_type(const char *message_type);
GList *create_listener_thread(Server *server, int new_socket, GList *thread_list);
static void handle_sigint(int _);
void server_cleanup(Server *server);
UserInfo* create_user(const char *username, const char *status, int socket);
void free_user(gpointer data);
void server_add_user(Server *server, const char *username, const char *status, int socket);
void server_remove_user(Server *server, const char *username);


char* Server_acceptClient();

int identify_user(int sock, char* buffer, char * id, size_t max_size);


void *listener(void *arg);


void process_message(Server *server, int, char *, const char *);
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



int send_json_response(int sock, char *json_str, size_t size_json_str, const char *fields_and_values[][2], size_t num_fields);
int identify_response_success(int sock, char *user);
int identify_response_failed(int sock, char *user);
void send_json_except_user(Server *server, const char *exclude_username);

#endif //CIRCE_SERVER_H