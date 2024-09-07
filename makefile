SRC_DIR = ./src/c
BIN_DIR = ./bin
LIB_DIR = /usr/local/include/unity
INCLUDE_DIR = ./include
TEST_DIR = ./tests
GLIB_DIR = /usr/include/glib-2.0
CFLAGS = $(shell pkg-config --cflags glib-2.0)
LDLIBS = $(shell pkg-config --libs glib-2.0)

# Default target
all: clean server client 

# Compiling the main program
main.o: $(SRC_DIR)/main.c 
	@mkdir -p $(BIN_DIR)
	gcc -Wall -Wextra -std=c11 -O2 -o $(BIN_DIR)/main $(SRC_DIR)/main.c -I$(INCLUDE_DIR) -ljansson

server: $(SRC_DIR)/server.c $(INCLUDE_DIR)/server.h  $(INCLUDE_DIR)/json_utils.h  $(INCLUDE_DIR)/connection.h
	gcc -Wall -Wextra -std=c11 -O2 -o $(BIN_DIR)/server $(SRC_DIR)/server.c $(SRC_DIR)/json_utils.c $(SRC_DIR)/connection.c -I$(INCLUDE_DIR) $(CFLAGS) $(LDLIBS) -lglib-2.0 -ljansson -lpthread

client: $(SRC_DIR)/client.c $(INCLUDE_DIR)/client.h  $(INCLUDE_DIR)/json_utils.h $(INCLUDE_DIR)/connection.h
	gcc -Wall -Wextra -std=c11 -O2 -o $(BIN_DIR)/client $(SRC_DIR)/client.c -I$(INCLUDE_DIR) $(SRC_DIR)/json_utils.c $(SRC_DIR)/connection.c $(CFLAGS) $(LDLIBS) -ljansson -lpthread

utils: $(SRC_DIR)/json_utils.c $(INCLUDE_DIR)/json_utils.h  
	gcc -Wall -Wextra -std=c11 -O2 -o $(BIN_DIR)/json_utils $(SRC_DIR)/json_utils.c -I$(INCLUDE_DIR) -ljansson 

connection: $(SRC_DIR)/connection.c 
	gcc -Wall -Wextra -std=c11 -O2 -o $(BIN_DIR)/connection $(SRC_DIR)/connection.c -I$(SRC_DIR)/connection.c -I$(INCLUDE_DIR) -lpthread
	
# Execute the main program
execute: $(BIN_DIR)/main
	$(BIN_DIR)/main 

# Compiling and running tests
test: $(TEST_DIR)/test_server.c $(SRC_DIR)/server.c $(INCLUDE_DIR)/server.h
	@mkdir -p $(BIN_DIR)
	gcc -Wall -Wextra -std=c11 -O2 -o $(BIN_DIR)/test_server $(TEST_DIR)/test_server.c $(SRC_DIR)/server.c -I$(LIB_DIR) -I$(INCLUDE_DIR)  -lunity
	$(BIN_DIR)/test_server

# Clean up
clean:
	rm -rf $(BIN_DIR)/*

