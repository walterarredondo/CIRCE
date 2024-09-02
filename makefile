SRC_DIR = ./src/c
BIN_DIR = ./bin
LIB_DIR = /usr/local/include/unity
INCLUDE_DIR = ./include
TEST_DIR = ./tests

# Compiling the main program
main.o: $(SRC_DIR)/main.c $(INCLUDE_DIR)/server.h
	@mkdir -p $(BIN_DIR)
	gcc -Wall -Wextra -std=c11 -O2 -o $(BIN_DIR)/main $(SRC_DIR)/main.c $(SRC_DIR)/server.c -I$(INCLUDE_DIR) -ljansson

server: $(SRC_DIR)/server.c $(INCLUDE_DIR)/server.h
	gcc -Wall -Wextra -std=c11 -O2 -o $(BIN_DIR)/server $(SRC_DIR)/server.c -I$(INCLUDE_DIR) -ljansson -lpthread

client: $(SRC_DIR)/client.c $(INCLUDE_DIR)/client.h
	gcc -Wall -Wextra -std=c11 -O2 -o $(BIN_DIR)/client $(SRC_DIR)/client.c -I$(INCLUDE_DIR) -ljansson -lpthread

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

