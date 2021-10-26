CC=clang
WARN=-Wall -Wextra -Werror

all:
	mkdir -p bin
	$(CC) main.c -o bin/bookcpu 

run: clean all
	bin/bookcpu images/echo

clean:
	rm -f bin/*
