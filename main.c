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
	int minecraft:1;
	int altch:1;
	int stdin:1;
	int debug:1;
	int help:1;
	char *path;
} options = { 0 };

// function prototypes
u_int16_t _getch();
int parseCommandLineArgs(int, char**);

int main (int argc, char **argv) {
	// parse command line options
	if (parseCommandLineArgs(argc, argv)) {
		goto error;
	}

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

	// open file (or use stdin)
	u_int8_t buffer[MEM_SIZE * 2] = {0};
	FILE *image = NULL;
	if (options.stdin) image = stdin;
	else            image = fopen(options.path, "r");

	if (image == NULL) {
		fprintf (
			stderr,
			"%s: ERR could not open file %s\n", argv[0],
			options.path);
		goto error;
	}

	// read file into buffer
	int ch, i = 0;
	while (i < MEM_SIZE && (ch = fgetc(image)) != EOF) {
		// for some reason they need to be swapped? idfk
		buffer[i++] = fgetc(image);
		buffer[i++] = ch;
	}

	// cast buffer to u_int_16
	u_int16_t *memory = (u_int16_t *)buffer;
	u_int16_t reg = 0, ptr = 0;
	int counter = 0;
	int flag_gt = 0, flag_eq = 0, flag_lt = 0;

	while (counter < MEM_SIZE) {
		int opcode = memory[counter] >> 12;
		int addr   = memory[counter] & 0xFFF;
		if (options.debug) printf (
			"debug: %03X: %01X %03X = %04X r%04X *%04X >%01X =%01X <%01X\n",
			counter, opcode, addr, memory[addr],
			reg, ptr, flag_gt, flag_eq, flag_lt);

		if (options.minecraft) {
			if (addr == 0xFFF) addr = ptr;
			if (counter == 0xFFE) goto exit;
			
			switch (opcode) {
			case 0x0: ptr = memory[addr] & 0xFFF; break;
			case 0x1: reg = memory[addr];         break;
			case 0x2: memory[addr] =  reg; break;
			case 0x3: memory[addr] =  0;   break;

			case 0x4: memory[addr] ++;     break;
			case 0x5: memory[addr] --;     break;
			case 0x6: reg += memory[addr]; break;
			case 0x7: reg -= memory[addr]; break;

			case 0x8: {
				int ch = _getch();

				// convert ASCII to minecraft charset
				if (ch > 127) {
					ch = 0;
				} else if (ch >= 'a' && ch <= 'z') {
					ch -= 32;
				}
				memory[addr] = asciiToMc[ch];
				if (options.debug) printf (
					"debug: got char %c which is %02x -> %02X\n",
					ch, ch, memory[addr]);
			} break;
			case 0x9: {
				int ch = memory[addr] & 0x3F;

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
				flag_gt = memory[addr] >  reg;
				flag_eq = memory[addr] == reg;
				flag_lt = memory[addr] <  reg;
				break;
			case 0xb: counter = addr - 1;  break;

			case 0xc: if(flag_gt)  counter = addr - 1; break;
			case 0xd: if(flag_lt)  counter = addr - 1; break;
			case 0xe: if(flag_eq)  counter = addr - 1; break;
			case 0xf: if(!flag_eq) counter = addr - 1; break; 
			}
		} else {
			switch (opcode) {
			case 0x0: reg = memory[addr];  break;
			case 0x1: memory[addr] =  reg; break;
			case 0x2: memory[addr] =  0;   break;
			case 0x3: reg += memory[addr]; break;
			case 0x4: memory[addr] ++;     break;
			case 0x5: reg -= memory[addr]; break;
			case 0x6: memory[addr] --;     break;
			case 0x7:
				flag_gt = memory[addr] >  reg;
				flag_eq = memory[addr] == reg;
				flag_lt = memory[addr] <  reg;
				break;
			case 0x8:              { counter = addr - 1; } break;
			case 0x9: if(flag_gt)  { counter = addr - 1; } break;
			case 0xa: if(flag_eq)  { counter = addr - 1; } break;
			case 0xb: if(flag_lt)  { counter = addr - 1; } break;
			case 0xc: if(!flag_eq) { counter = addr - 1; } break;
			case 0xd: memory[addr] = _getch(); break;
			case 0xe: putchar(memory[addr]);    break;
			case 0xf: goto exit;
			}
		}

		counter++;
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

u_int16_t _getch() {
	struct termios old;
	tcgetattr(0, &old);
	old.c_lflag &= ~ICANON;
	old.c_lflag &= ~ECHO;
	old.c_cc[VMIN] = 1;
	old.c_cc[VTIME] = 0;
	tcsetattr(0, TCSANOW, &old);
	u_int16_t ch = getchar();
	old.c_lflag |= ICANON;
	old.c_lflag |= ECHO;
	tcsetattr(0, TCSADRAIN, &old);
	return ch;
}

