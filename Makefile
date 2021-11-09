CC=clang
WARN=-Wall -Wextra -Werror
MCTEST=test-mc
LYTEST=test

bookcpu:
	mkdir -p bin
	$(CC) main.c -o bin/bookcpu $(WARN)

bookcpu-test: clean bookcpu
	bin/bookcpu images/$(LYTEST)

bookcpu-test-mc: clean bookcpu
	bin/bookcpu -mc images/$(MCTEST)

bkasm:
	mkdir -p bin
	$(CC) bkasm.c -o bin/bkasm $(WARN)

bkasm-test: clean bkasm
	bin/bkasm asm/$(LYTEST).bkasm images/$(LYTEST)

bkasm-test-mc: clean bkasm
	bin/bkasm -m asm/$(MCTEST).bkasm images/$(MCTEST)

all: bookcpu bkasm

all-test: clean all
	bin/bkasm asm/$(LYTEST).bkasm images/$(LYTEST)
	bin/bookcpu images/$(LYTEST)
	
all-test-mc: clean all
	bin/bkasm -m asm/$(MCTEST).bkasm images/$(MCTEST)
	bin/bookcpu -mc images/$(MCTEST)

clean:
	rm -f bin/*
