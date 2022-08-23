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
	u_int16_t *memory;
	u_int16_t reg, ptr;
	int PADDING; // delete this if another 4 bytes are added
} machine = { 0 };

// function prototypes
u_int16_t readInput(void);
int  parseCommandLineArgs(int, char**);
void loadFile(FILE*);

int main (int argc, char **argv) {
	FILE *image = NULL;
	
	// parse command line options
	if (parseCommandLineArgs(argc, argv)) { goto error; }

	if (options.help) {
		printf("Usage: %s [options] [image]\n", argv[0]);
		puts("Options:");
		puts("  -m    Enable minecraft instruction set");
		puts("  -c    Enable minecraft charset");
		puts("  -x    Read image file from stdin");
		puts("  -d    Enable debug logging");
		puts("  -h    Show help");
		goto exit;
	}

	if (options.path == NULL && !options.stdin) {
		fprintf(stderr, "%s: ERR no image file given\n", argv[0]);
		goto error;
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
		goto error;
	}

	// read file into buffer
	loadFile(image);

	// run CPU
	while (machine.counter < MEM_SIZE) {
		int opcode = machine.memory[machine.counter] >> 12;
		int addr   = machine.memory[machine.counter] & 0xFFF;
		if (options.debug) printf (
			"debug: %03X: %01X %03X = %04X r%04X *%04X >%01X =%01X <%01X\n",
			machine.counter, opcode, addr, machine.memory[addr],
			machine.reg, machine.ptr,
			machine.flag_gt, machine.flag_eq, machine.flag_lt);

		if (options.minecraft) {
			if (addr == 0xFFF) addr = machine.ptr;
			if (machine.counter == 0xFFE) goto exit;
			
			switch (opcode) {
			case 0x0: machine.ptr = machine.memory[addr] & 0xFFF; break;
			case 0x1: machine.reg = machine.memory[addr];         break;
			case 0x2: machine.memory[addr] = machine.reg; break;
			case 0x3: machine.memory[addr] = 0;           break;

			case 0x4: machine.memory[addr] ++;             break;
			case 0x5: machine.memory[addr] --;             break;
			case 0x6: machine.reg += machine.memory[addr]; break;
			case 0x7: machine.reg -= machine.memory[addr]; break;

			case 0x8: {
				int ch = readInput();

				// convert ASCII to minecraft charset
				if (ch > 127) {
					ch = 0;
				} else if (ch >= 'a' && ch <= 'z') {
					ch -= 32;
				}
				machine.memory[addr] = (u_int16_t)(asciiToMc[ch]);
				if (options.debug) printf (
					"debug: got char %c which is %02x -> %02X\n",
					ch, ch, machine.memory[addr]);
			} break;
			case 0x9: {
				int ch = machine.memory[addr] & 0x3F;

				// convert minecraft charser codepoint to ASCII
				// character or ANSI escape code
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
			} break;
			case 0xa:
				machine.flag_gt = machine.memory[addr] >  machine.reg;
				machine.flag_eq = machine.memory[addr] == machine.reg;
				machine.flag_lt = machine.memory[addr] <  machine.reg;
				break;
			case 0xb: machine.counter = addr - 1;  break;

			case 0xc: if(machine.flag_gt)  machine.counter = addr - 1; break;
			case 0xd: if(machine.flag_lt)  machine.counter = addr - 1; break;
			case 0xe: if(machine.flag_eq)  machine.counter = addr - 1; break;
			case 0xf: if(!machine.flag_eq) machine.counter = addr - 1; break; 
			}
		} else {
			switch (opcode) {
			case 0x0: machine.reg = machine.memory[addr];  break;
			case 0x1: machine.memory[addr] = machine.reg;  break;
			case 0x2: machine.memory[addr] = 0;            break;
			case 0x3: machine.reg += machine.memory[addr]; break;
			case 0x4: machine.memory[addr] ++;             break;
			case 0x5: machine.reg -= machine.memory[addr]; break;
			case 0x6: machine.memory[addr] --;             break;
			case 0x7:
				machine.flag_gt = machine.memory[addr] >  machine.reg;
				machine.flag_eq = machine.memory[addr] == machine.reg;
				machine.flag_lt = machine.memory[addr] <  machine.reg;
				break;
			case 0x8:                      { machine.counter = addr - 1; } break;
			case 0x9: if(machine.flag_gt)  { machine.counter = addr - 1; } break;
			case 0xa: if(machine.flag_eq)  { machine.counter = addr - 1; } break;
			case 0xb: if(machine.flag_lt)  { machine.counter = addr - 1; } break;
			case 0xc: if(!machine.flag_eq) { machine.counter = addr - 1; } break;
			case 0xd: machine.memory[addr] = readInput();     break;
			case 0xe: putchar(machine.memory[addr]);       break;
			case 0xf: goto exit;
			}
		}

		machine.counter++;
	}

	exit:
	return EXIT_SUCCESS;

	error:
	return EXIT_FAILURE;
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

// loadFile
// Loads 4096 big-endian 16 bit integers from file into memory cells, starting
// at address 0.
void loadFile (FILE *file) {
	// TODO: have buffer be 16 bits by default, use bit shifting or
	// multiplication to put things in memory here. do not cast!
	
	u_int8_t buffer[MEM_SIZE * 2] = { 0 };
	int ch, i = 0;
	while (i < MEM_SIZE && (ch = fgetc(file)) != EOF) {
		// for some reason they need to be swapped? idfk
		buffer[i++] = (u_int8_t)(fgetc(file));
		buffer[i++] = (u_int8_t)(ch);
	}

	// cast buffer to u_int_16
	machine.memory = (u_int16_t *)(buffer);
}

// readInput
// Reads one character of input from stdin. It disables line buffering so that
// if the user types a key, it is registered instantly.
u_int16_t readInput() {
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
