

typedef struct {
} Server;

Server Server_init();

char* Server_acceptClient();

int identify(char*);

void *listener(void *arg);
