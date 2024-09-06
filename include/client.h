typedef struct {
    int* fd_id;
    char username[8];
    char status[];
} client;

int identify_client(int sock, char* user);

void ask_for_username(char *username, int max_len);