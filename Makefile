CC=clang
WARN=-Wall -Wextra -Werror

bookcpu:
	mkdir -p bin
	$(CC) main.c -o bin/bookcpu $(WARN)

bookcpu-test: clean bookcpu
	bin/bookcpu images/echo

bkasm:
	mkdir -p bin
	$(CC) bkasm.c -o bin/bkasm $(WARN)

bkasm-test: clean bkasm
	bin/bkasm asm/test.bkasm images/test

clean:
	rm -f bin/*
