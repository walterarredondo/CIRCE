#include "unity.h"
#include <string.h>
#include "../include/server.h" 

Server server; //Variable global necesaria para llamar a las macros de la biblioteca Unity

// Función para configurar el entorno antes de cada prueba
void setUp(void) {
    // Inicializar el servidor u otros recursos necesarios para las pruebas
}

// Función para liberar recursos después de cada prueba
void tearDown(void) {
    // Finalizar el servidor o liberar cualquier recurso asignado en setUp
}

// Prueba: Verifica que el servidor se inicializa correctamente
void test_ServerInitialization(void) {
    server = Server_init();
}

// Prueba: Verifica que el servidor puede aceptar una conexión de un cliente
void test_ServerAcceptsClientConnection(void) {
    // Simula una conexión de cliente (esto podría requerir un cliente simulado)

    
    char* server_response = Server_acceptClient(&server);
    char expected_string[] = "{ \"type\": \"RESPONSE\", \"operation\": \"IDENTIFY\", \"result\": \"SUCCESS\", \"extra\": \"Kimberly\" }";
    
    // Verifica que el cliente fue aceptado correctamente
    TEST_ASSERT_EQUAL_STRING(expected_string, server_response);
}


// Punto de entrada para ejecutar todas las pruebas
int main(void) {
    test_ServerInitialization();
    RUN_TEST(test_ServerAcceptsClientConnection);
    
    return UNITY_END();
}

