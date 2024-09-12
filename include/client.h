typedef struct {
    int* fd_id;
    char username[8];
    char status[];
} Client;


typedef enum {
    TYPE_NEW_USER = 13,
    TYPE_NEW_STATUS = 14,
    TYPE_USER_LIST = 15,
    TYPE_TEXT_FROM = 16,
    TYPE_PUBLIC_TEXT_FROM = 17,
    TYPE_JOINED_ROOM = 18,
    TYPE_ROOM_USER_LIST = 19,
    TYPE_ROOM_TEXT_FROM = 20,
    TYPE_LEFT_ROOM = 21,
    TYPE_DISCONNECTED = 22,
    TYPE_RESPONSE = 23,
    TYPE_UNKNOWN = 24
} MessageType;

enum Command {
    CMD_HELP,
    CMD_ECHO,
    CMD_LOGIN,
    CMD_UNKNOWN
};

typedef void (*process_message_func)(Client *client, int, char *, const char *);

typedef struct {
    Client *client;
    int socket;
    process_message_func process_message;
} listener_args_t;


enum Command get_command_type(const char* command);


void handle_sigint(int sig);

int identify_client(int sock, const char* user);

void ask_for_username(char *username, int max_len);
int create_listener(int sock);





void process_message(Client *client, int, char *, const char *);


void handle_new_user(Client *client, int socket, char *buffer);
void handle_new_status(Client *client, int socket, char *buffer);
void handle_user_list(Client *client, int socket, char *buffer);
void handle_text_from(Client *client, int socket, char *buffer);
void handle_public_text_from(Client *client, int socket, char *buffer);
void handle_joined_room(Client *client, int socket, char *buffer);
void handle_room_user_list(Client *client, int socket, char *buffer);
void handle_room_text_from(Client *client, int socket, char *buffer);
void handle_left_room(Client *client, int socket, char *buffer);
void handle_disconnected(Client *client, int socket, char *buffer);
void handle_response(Client *client, int socket, char *buffer);


// Get input from the user
bool get_input(char *buffer);
// Parse the command and arguments from the input buffer
bool parse_command(const char *buffer, char *command, char *args);
// Check if the command is '\leave'
bool is_leave_command(const char *command);
// Execute the parsed command and pass the arguments
void execute_command(int sock, const char *command, const char *args);
// Handle the '\help' command
void handle_help(const char *args);
// Handle the '\echo' command
void handle_echo(const char *args);
// Handle login command
void handle_login(int sock, const char *command);
// Handle unknown commands
void handle_unknown(const char *command);


MessageType get_type(const char *message_type) {
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
        return TYPE_RESPONSE;
    } else {
        return TYPE_UNKNOWN;
    }
}