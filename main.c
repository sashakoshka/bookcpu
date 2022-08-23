#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

#include "mccharmap.h"

// amount of 16 bit memory cells
#define MEM_SIZE 4096

// options
// This struct stores information about how the user wants to run the program.
// The information is collected by parseCommandLineArgs.
static struct {
	int minecraft;
	int altch;
	int stdin;
	int debug;
	int help;
	int PADDING; // delete this if another 4 bytes are added
	char *path;
} options = { 0 };

// machine
// This struct stores information about the state of the virtual CPU, such as
// its memory, registers, and the program counter.
static struct {
	int flag_gt, flag_eq, flag_lt;
	int counter;
	u_int16_t memory[MEM_SIZE];
	u_int16_t reg, ptr, opcode, address;
} machine = { 0 };

// function prototypes
u_int16_t readInput (void);
int  parseCommandLineArgs (int, char**);
void loadFile             (FILE*);
void runWithLegacySet     (void);
void runWithMinecraftSet  (void);
void debugCPUState        (void);

int main (int argc, char **argv) {
	FILE *image = NULL;
	
	// parse command line options
	if (parseCommandLineArgs(argc, argv)) { return EXIT_FAILURE; }

	if (options.help) {
		printf("Usage: %s [options] [image]\n", argv[0]);
		puts("Options:");
		puts("  -m    Enable minecraft instruction set");
		puts("  -c    Enable minecraft charset");
		puts("  -x    Read image file from stdin");
		puts("  -d    Enable debug logging");
		puts("  -h    Show help");
		return EXIT_SUCCESS;
	}

	if (options.path == NULL && !options.stdin) {
		fprintf(stderr, "%s: ERR no image file given\n", argv[0]);
		return EXIT_FAILURE;
	}

	// open file (or read directly from stdin)
	if (options.stdin) {
		image = stdin;
	} else {
		image = fopen(options.path, "r");
	}

	if (image == NULL) {
		fprintf (
			stderr,
			"%s: ERR could not open file %s\n", argv[0],
			options.path);
		return EXIT_FAILURE;
	}

	// read file into buffer
	loadFile(image);

	// run CPU
	if (options.minecraft) {
		runWithMinecraftSet();
	} else {
		runWithLegacySet();
	}

	return EXIT_SUCCESS;
}

// parseCommandLineArgs
// This function parses all command line arguments into the options struct. On
// success, it returns 0. If an error was encountered, it returns 1.
int parseCommandLineArgs (int argc, char **argv) {
	for (int i = 1, getSwitches = 1; i < argc; i++) {
		char *ch = argv[i];
		if (*ch == '-' && getSwitches) {
			// this arg has 1 or more switches
			while (*(++ch) != 0) switch (*ch) {
				case '-': getSwitches       = 0; break;
				case 'm': options.minecraft = 1; break;
				case 'c': options.altch     = 1; break;
				case 'x': options.stdin     = 1; break;
				case 'd': options.debug     = 1; break;
				case 'h': options.help      = 1; break;
			}
		}
		else if (options.path == NULL) {
			// we have a filepath
			options.path = ch;
		} else {
			fprintf (
				stderr,
				"%s: ERR multiple image files given\n",
				argv[0]);
			return 1;
		}
	}

	return 0;
}

// runWithLegacySet
// Runs the cpu with the legacy instruction set found in the textbook.
void runWithLegacySet (void) {
	while (machine.counter < MEM_SIZE) {
		machine.opcode  = machine.memory[machine.counter] >> 12;
		machine.address = machine.memory[machine.counter] & 0xFFF;
		debugCPUState();

		switch (machine.opcode) {
		case 0x0:
			// load value at address to register
			machine.reg = machine.memory[machine.address]; 
			break;
		case 0x1:
			// store value of register at address
			machine.memory[machine.address] = machine.reg; 
			break;
		case 0x2:
			// set vaue at address to zero
			machine.memory[machine.address] = 0;
			break;
		case 0x3:
			// add vaue at address to register
			machine.reg += machine.memory[machine.address];
			break;
		case 0x4:
			// increment value at address
			machine.memory[machine.address] ++;
			break;
		case 0x5:
			// subtract value at address from register
			machine.reg -= machine.memory[machine.address];
			break;
		case 0x6:
			// decrement value at address
			machine.memory[machine.address] --;
			break;
		case 0x7:
			// compare value at address against the register, and
			// set flags accordingly. the results of this operation
			// are used by the conditional jump operations.
			machine.flag_gt = machine.memory[machine.address] >  machine.reg;
			machine.flag_eq = machine.memory[machine.address] == machine.reg;
			machine.flag_lt = machine.memory[machine.address] <  machine.reg;
			break;
		case 0x8:
			// unconditionally jump to the address
			machine.counter = machine.address - 1;
			break;
		case 0x9:
			// conditionally jump to the address if the greater than
			// flag is set
			if (machine.flag_gt) {
				machine.counter = machine.address - 1;
			}
			break;
		case 0xa:
			// conditionally jump to the address if the equal to
			// flag is set
			if (machine.flag_eq) {
				machine.counter = machine.address - 1;
			}
			break;
		case 0xb:
			// conditionally jump to the address if the less than
			// flag is set
			if (machine.flag_lt) {
				machine.counter = machine.address - 1;
			}
			break;
		case 0xc:
			// conditionally jump to the address if the equal to
			// flag is *not* set
			if (!machine.flag_eq) {
				machine.counter = machine.address - 1;
			}
			break;
		case 0xd:
			// read a single character from the input (stdin) and
			// store it at address. this pauses until a character is
			// available to read.
			machine.memory[machine.address] = readInput();
			break;
		case 0xe:
			// send the value at address to the output (stdout)
			putchar(machine.memory[machine.address]);
			break;
		case 0xf:
			// halt the program
			return;
		}

		machine.counter++;
	}
}

// runWithMinecraftSet
// Runs the cpu with the new instruction set.
void runWithMinecraftSet (void) {
	int ch;

	while (machine.counter < MEM_SIZE) {
		machine.opcode  = machine.memory[machine.counter] >> 12;
		machine.address = machine.memory[machine.counter] & 0xFFF;
		debugCPUState();

		if (machine.address == 0xFFF) { machine.address = machine.ptr; }
		if (machine.counter == 0xFFE) { return; }
		
		switch (machine.opcode) {
		case 0x0:
			// load value at address to pointer
			machine.ptr = machine.memory[machine.address] & 0xFFF;
			break;
		case 0x1:
			// load value at address to register
			machine.reg = machine.memory[machine.address];
			break;
		case 0x2:
			// store of register at address
			machine.memory[machine.address] = machine.reg;
			break;
		case 0x3:
			// set value at address to zero
			machine.memory[machine.address] = 0;
			break;
		case 0x4:
			// increment value at address
			machine.memory[machine.address] ++;
			break;
		case 0x5:
			// decrement value at address
			machine.memory[machine.address] --;
			break;
		case 0x6:
			// add value at address to register
			machine.reg += machine.memory[machine.address];
			break;
		case 0x7:
			// subtract value at address from register
			machine.reg -= machine.memory[machine.address];
			break;
		case 0x8:
			// read a single character from the input (stdin) and
			// store it at address. this pauses until a character is
			// available to read.
			ch = readInput();

			// convert ASCII to minecraft charset
			if (ch > 127) {
				ch = 0;
			} else if (ch >= 'a' && ch <= 'z') {
				ch -= 32;
			}
			machine.memory[machine.address] = (u_int16_t)(asciiToMc[ch]);
			if (options.debug) fprintf (
				stderr,
				"debug: got char %c which is %02x -> %02X\n",
				ch, ch, machine.memory[machine.address]);
			break;
		case 0x9:
			// send the value at address to the output (stdout)

			// convert minecraft charser codepoint to ASCII
			// character or ANSI escape code
			ch = machine.memory[machine.address] & 0x3F;
			if (ch < 6) {
				switch (ch) {
				case 0: putchar(0);   break;
				case 1: putchar(EOF); break;
				case 2: fputs("\033[1A", stdout); break;
				case 3: fputs("\033[1B", stdout); break;
				case 4: fputs("\033[1D", stdout); break;
				case 5: fputs("\033[1C", stdout); break;
				}
			} else if (ch < 256 ) {
				putchar(mcToAscii[ch]);
			} else {
				putchar(0);
			}
			break;
		case 0xa:
			// compare value at address against the register, and
			// set flags accordingly. the results of this operation
			// are used by the conditional jump operations.
			machine.flag_gt = machine.memory[machine.address] >  machine.reg;
			machine.flag_eq = machine.memory[machine.address] == machine.reg;
			machine.flag_lt = machine.memory[machine.address] <  machine.reg;
			break;
		case 0xb:
			// unconditionally jump to the address
			machine.counter = machine.address - 1;
			break;
		case 0xc:
			// conditionally jump to the address if the greater than
			// flag is set
			if (machine.flag_gt) {
				machine.counter = machine.address - 1;
			}
			break;
		case 0xd:
			// conditionally jump to the address if the less than
			// flag is set
			if (machine.flag_lt) {
				machine.counter = machine.address - 1;
			}
			break;
		case 0xe:
			// conditionally jump to the address if the equal to
			// flag is set
			if (machine.flag_eq) {
				machine.counter = machine.address - 1;
			}
			break;
		case 0xf:
			// conditionally jump to the address if the equal to
			// flag is *not* set
			if (!machine.flag_eq) {
				machine.counter = machine.address - 1;
			}
			break;
		}

		machine.counter++;
	}
}

// loadFile
// Loads 4096 big-endian 16 bit integers from file into memory cells, starting
// at address 0.
void loadFile (FILE *file) {
	int ch, i = 0;
	while (i < MEM_SIZE && (ch = fgetc(file)) != EOF) {
		u_int16_t largeEnd = (u_int16_t)(ch);
		u_int16_t smallEnd = (u_int16_t)(fgetc(file));
		
		machine.memory[i] = largeEnd * 256 + smallEnd;
		i ++;
	}
}

// readInput
// Reads one character of input from stdin. It disables line buffering so that
// if the user types a key, it is registered instantly.
u_int16_t readInput (void) {
	u_int16_t ch;
	
	struct termios old;
	tcgetattr(0, &old);
	old.c_lflag &= (unsigned int)(~ICANON);
	old.c_lflag &= (unsigned int)(~ECHO);
	old.c_cc[VMIN] = 1;
	old.c_cc[VTIME] = 0;
	tcsetattr(0, TCSANOW, &old);
	ch = (u_int16_t)(getchar());
	old.c_lflag |= ICANON;
	old.c_lflag |= ECHO;
	tcsetattr(0, TCSADRAIN, &old);
	
	return ch;
}

// debugCPUState
// Prints debug information about the state of the CPU if the debug option has
// been enabled by the user.
void debugCPUState (void) {
	if (options.debug) fprintf (
		stderr,
		"debug: %03X: %01X %03X = %04X r%04X *%04X >%01X =%01X <%01X\n",
		machine.counter, machine.opcode, machine.address,
		machine.memory[machine.address],
		machine.reg, machine.ptr,
		machine.flag_gt, machine.flag_eq, machine.flag_lt);
}
