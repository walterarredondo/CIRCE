#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/server.h"


Server Server_init(){
    Server server;
    return server;    
}


char* Server_acceptClient(Server* server) {
    char response[] = "{ \"type\": \"RESPONSE\", \"operation\": \"IDENTIFY\", \"result\": \"SUCCESS\", \"extra\": \"Kimberly\" }";
    return response;
}

