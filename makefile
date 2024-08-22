main.o : ./src/c/main.c
	gcc -Wall -Wextra -std=c11 -o ./target/main  ./src/c/main.c

execute : ./target/main
	./target/main 

clean : 
	rm ./target/main