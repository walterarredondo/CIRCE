

typedef struct {
} Server;

Server Server_init();

char* Server_acceptClient();



void* listener(void* arg);