typedef struct {
    int* fd_id;
    char username[8];
    char status[];
} client;


enum Command {
    CMD_HELP,
    CMD_ECHO,
    CMD_LOGIN,
    CMD_UNKNOWN
};


enum Command get_command_type(const char* command);



int identify_client(int sock, const char* user);

void ask_for_username(char *username, int max_len);





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