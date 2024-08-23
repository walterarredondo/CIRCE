SRC_DIR = ./src/c
BIN_DIR = ./bin
LIB_DIR =/usr/local/include/unity

main.o : $(SRC_DIR)/main.c 
	@mkdir -p $(BIN_DIR)
	gcc -Wall -Wextra -std=c11 -O2 -o $(BIN_DIR)/main  $(SRC_DIR)/main.c  -I$(LIB_DIR)

execute : $(BIN_DIR)/main
	$(BIN_DIR)/main 

clean : 
	rm -rf $(BIN_DIR)