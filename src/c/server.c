#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"


Server Server_init(){
    Server server;
    return server;    
}


char* Server_acceptClient() {
    static char response[] = "{ \"type\": \"RESPONSE\", \"operation\": \"IDENTIFY\", \"result\": \"SUCCESS\", \"extra\": \"Kimberly\" }";
    return response;
}

