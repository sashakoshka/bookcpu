#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEM_SIZE 4096

typedef struct var {
	u_int16_t addr;
	u_int16_t size; // unused as of now
	u_int16_t value;
	char name[16];
	char pointsTo[16];
} Var;

typedef struct oper {
	u_int8_t opcode;
	char var[16];
	u_int16_t addr;
} Oper;

int readVarName (FILE *, int *, char *);

int main (int argc, char **argv) {
	// command line args
	struct {
		int minecraft;
		int stdin;
		int quiet;
		int help;
		int decimal;
		char *inPath;
		char *outPath;
	} args = { 0 };
	
	for (int i = 1, getSwitches = 1; i < argc; i++) {
		char *ch = argv[i];
		if (*ch == '-' && getSwitches) {
			// this arg has 1 or more switches
			while (*(++ch) != 0) switch (*ch) {
			case '-': getSwitches    = 0; break;
			case 'm': args.minecraft = 1; break;
			case 'x': args.stdin     = 1; break;
			case 'q': args.quiet     = 1; break;
			case 'h': args.help      = 1; break;
			case 'd': args.decimal   = 1; break;
			}
		}
		// we have a filepath
		else if (args.inPath == NULL) args.inPath = ch;
		else args.outPath = ch;
	}

	if (args.help) {
		printf("Usage: %s [options] [source] [output]\n", argv[0]);
		puts("Options:");
		puts("  -m    Enable minecraft instruction set");
		puts("  -x    Read source file from stdin");
		puts("  -q    Don't output anything");
		puts("  -h    Show help");
		puts("  -d    Write image as newline separated decimal numbers");
		return EXIT_SUCCESS;
	}

	if ((args.inPath == NULL && !args.stdin) || args.outPath == NULL) {
		fprintf (
			stderr, "%s: please provide input and output files\n",
			argv[0]);
		return EXIT_FAILURE;
	}

	if (!args.quiet) {
		printf (
			"%s: compiling %s ===> %s\n",
			argv[0], args.inPath, args.outPath);
	}

	size_t varcount = 0,
	varsize  = 4;
	Var *vars = malloc(varsize * sizeof(Var));

	// open file
	FILE *in = NULL;
	if (args.stdin) {
		in = stdin;
	} else {
		in = fopen(args.inPath, "r");
	}

	if (in == NULL) {
		fprintf (
			stderr, "%s: ERR could not open file %s\n", argv[0],
			args.inPath);
		return EXIT_FAILURE;
	}

	// get variables
	int ch;
	while ((ch = fgetc(in)) != '-') {
		// realloc variable list if necessary
		if(++varcount > varsize) {
			varsize *= 2;
			vars = realloc(vars, varsize * sizeof(Var));
		}

		// get var name
		Var *var = &(vars[varcount - 1]);
		if (readVarName(in, &ch, var->name)) goto premature_eof_err;
		if (var->name[0] == '.') var->name[0] = 0;
		var->size = 1;
		var->addr = 0xFFF;
		var->pointsTo[0] = 0;

		// skip whitespace
		while ((ch = fgetc(in)) == ' ' || ch == '\t');

		if (args.minecraft && ch == '&') {
			// this is a pointer
			ch = fgetc(in);
			if (readVarName(in, &ch, var->pointsTo)) goto premature_eof_err;
			var->value = 0;
		} else {
			// get hex value
			int mult = 4096;
			var->value = 0;
			for (int i = 0; i < 4; i++) {
				// if we reach whitespace its time to stop!!!
				if(ch == ' ' || ch == '\t' || ch == '\n') break;
				// only accept valid hex chars
				if(ch < 47 || (ch > 57 && ch < 65 ) || (ch > 70 && ch < 97) || ch > 102) {
					goto invalid_hex_err;
				}

				if (ch <= 57) ch -= 48;
				else if (ch <= 90) ch -= 55;
				else ch -= 87;
				ch *= mult;
				var->value += ch;

				mult /= 16;
				ch = fgetc(in);
			}
		}

		// skip trailing stuff
		while (ch != '\n' && ch != EOF) { ch = fgetc(in) };

		if (!args.quiet) {
			printf("got variable:\t[%s]\t", var->name);
			if (var->pointsTo[0] != 0) {
				printf("[&%s]", var->pointsTo);
			} else {
				printf("[%03x]", var->value);
			}
			putchar('\n');
		}
	}

	// go to start of new line
	while (ch != '\n' && ch != EOF) ch = fgetc(in);
	if (!args.quiet) printf("%s: data section terminated\n", argv[0]);

	// read operations

	size_t opercount = 0,
	opersize  = 16;
	Oper *opers = malloc(opersize * sizeof(Oper));

	while ((ch = fgetc(in)) != EOF) {
	// skip beginning whitespace, if there is any.
		while ((ch == ' ' || ch == '\t' || ch == '\n') && ch != EOF) {
			ch = fgetc(in);
		}

		// skip line if this is a comment
		if (ch == '#') {
			if (!args.quiet) { printf("comment: "); }
			while (ch != '\n' && ch != EOF) {
				if (!args.quiet) { putchar(ch); }
				ch = fgetc(in);
			}
			if (!args.quiet) { putchar('\n'); }
			continue;
		}

		// get opcode from symbol
		u_int8_t opcode;
		switch (ch) {
		case '*':
			if ((ch = fgetc(in)) == '=' && args.minecraft) {
				opcode = 0x0;
			} else {
				goto invalid_oper_err;
			}
			break;
		case '<':
			if ((ch = fgetc(in)) == '-') {
				opcode = args.minecraft ? 0x1 : 0x0;
			} else if (ch == '<') {
				opcode = args.minecraft ? 0x9 : 0xe;
			} else {
				goto invalid_oper_err;
			}
			break;
		case '-':
			switch (fgetc(in)) {
			case '>': opcode = args.minecraft ? 0x2 : 0x1; break;
			case '=': opcode = args.minecraft ? 0x7 : 0x5; break;
			case '-': opcode = args.minecraft ? 0x5 : 0x6; break;
			default: goto invalid_oper_err;
			}
			break;
		case 'x':
			if ((ch = fgetc(in)) == 'x') {
				opcode = args.minecraft ? 0x3 : 0x2;
			}
			else goto invalid_oper_err;
			break;
		case '+':
			if ((ch = fgetc(in)) == '=') {
				opcode = args.minecraft ? 0x6 : 0x3;
			} else if (ch == '+') {
				opcode = 0x4;
			} else goto invalid_oper_err;
			break;
		case '?':
			if ((ch = fgetc(in)) == '?') {
				opcode = args.minecraft ? 0xa : 0x7;
			}
			else goto invalid_oper_err;
			break;
		case 'g':
			if ((ch = fgetc(in)) == 'o') {
				opcode = args.minecraft ? 0xb : 0x8;
			}
			else goto invalid_oper_err;
			break;
		case 'i':
			if ((ch = fgetc(in)) == 'f' && (ch = fgetc(in)) == ' ') {
				switch (fgetc(in)) {
				case '>': opcode = args.minecraft ? 0xc : 0x9; break;
				case '=': opcode = args.minecraft ? 0xe : 0xa; break;
				case '<': opcode = args.minecraft ? 0xd : 0xb; break;
				case '!': opcode = args.minecraft ? 0xf : 0xc; break;
				default: goto invalid_oper_err;
				}
			} else goto invalid_oper_err;
			break;
		case ':':
			if ((ch = fgetc(in)) == ':') { opcode = 0x10; }
			else goto invalid_oper_err;
			break;
		case '>':
			if ((ch = fgetc(in)) == '>') {
				opcode = args.minecraft ? 0x8 : 0xd;
			}
			else goto invalid_oper_err;
			break;
		case 'H':
			if (
				!args.minecraft	 &&
				(ch = fgetc(in)) == 'A' &&
				(ch = fgetc(in)) == 'L' &&
				(ch = fgetc(in)) == 'T'
			) {
				opcode = 0xf;
			}
			else goto invalid_oper_err;
			break;
		default:
			goto invalid_oper_err;
		}

		// skip whitespace
		while ((ch = fgetc(in)) == ' ' || ch == '\t');

		if (opcode < 0x10) {
			if(++opercount > opersize) {
				opersize *= 2;
				opers = realloc(opers, opersize * sizeof(Oper));
			}
			Oper *oper = &(opers[opercount - 1]);
			oper->opcode = opcode;

			if (opcode == 0xf) {
				// HALT does not take an address
				oper->var[0] = 0;
			} else {
				// get var name
				if (readVarName(in, &ch, oper->var))
					goto premature_eof_err;
			}

			if (!args.quiet)
				printf ("got operation:\t[%01x]\t[%s]\n",
					oper->opcode, oper->var);
		} else {
			// label
			if(++varcount > varsize) {
				varsize *= 2;
				vars = realloc(vars, varsize * sizeof(Var));
			}
			Var *label = &(vars[varcount - 1]);
			if (readVarName(in, &ch, label->name))
				goto premature_eof_err;
			label->size = 1;
			label->addr = (u_int16_t)(opercount);

			if (!args.quiet)
				printf ("got label:\t[%s]\t[%03x]\n",
					label->name, label->addr);
		}

		// skip trailing stuff
		while (ch != '\n' && ch != EOF) ch = fgetc(in);

	}

	// close file
	fclose(in);

	if (!args.quiet)
		printf("%s: program section terminated\n", argv[0]);

	// figure out memory locations of variables
	for (size_t i = 0, index = opercount; i < varcount; i++) {
		Var *var = &vars[i];
		if (var->addr == 0xFFF) {
			var->addr = (u_int16_t)(index);
			index += var->size;
			if (!args.quiet)
				printf ("variable %s\tinhabits %03x",
					var->name, var->addr);
			// if var is a pointer, find what it points to
			if (var->pointsTo[0] != 0) {
				for (size_t j = 0; j < varcount; j++) {
					if (strcmp(var->pointsTo, vars[j].name) == 0) {
						if (vars[j].addr == 0xFFF)
							goto invalid_symbol_err;
						else
							var->value = vars[j].addr;
					}
				}
				printf(" and points to %03x", var->value);
			}
			putchar('\n');
		}
	}

	FILE *out = fopen(args.outPath, "w");
	if (out == NULL) {
		fprintf (stderr,
			"%s: ERR could not open file %s\n",
			argv[0], args.outPath);
		return EXIT_FAILURE;
	}

	u_int16_t memaddr = 0;

	// write program section
	for (size_t i = 0; i < opercount; i++) {
		Oper *oper = &opers[i];
		// figure out what address it references
		char *var = oper->var;
		oper->addr = 0;

		// special symbols
		if (args.minecraft && strcmp(var, "PTR") == 0) {
			oper->addr = 0xFFF;
		} else if (args.minecraft && strcmp(var, "HALT") == 0) {
			oper->addr = 0xFFE;
		}

		for (size_t j = 0; j < varcount; j++) {
			// find which var
			if (strcmp(var, vars[j].name) == 0) {
				oper->addr = vars[j].addr;
			}
		}
		u_int16_t cell = (opers[i].opcode & 0xF) << 12 | (oper->addr & 0xFFF);

		if (args.decimal) {
			fprintf(out, "%i\n", cell);
		} else {
			fputc(cell >> 8, out);
			fputc(cell & 0xFF, out);
			if (!args.quiet) {
				printf("memory[%04x]: %04x\n", memaddr++, cell); 
			}
		}
	}

	// write data section
	for (size_t i = 0; i < varcount; i++) {
		u_int16_t cell = vars[i].value;

		if (args.decimal) {
			fprintf(out, "%i\n", cell);
		} else {
			// swap values. some bizarre endianness stuff.
			fputc(cell >> 8, out);
			fputc(cell & 0xFF, out);
			if (!args.quiet) {
				printf("memory[%04x]: %04x\n", memaddr++, cell);
			}
		}
	}

	return EXIT_SUCCESS;

	premature_eof_err:
	fprintf (
		stderr, "%s: ERR reached end of file before program start in %s\n",
		argv[0], args.inPath);
	return EXIT_FAILURE;

	invalid_hex_err:
	fprintf (
		stderr, "%s: ERR invalid hex digit in %s: [%c]\n",
		argv[0], args.inPath, ch);
	return EXIT_FAILURE;

	invalid_oper_err:
	fprintf (
		stderr, "%s: ERR unknown opcode in %s\n",
		argv[0], args.inPath);

	invalid_symbol_err:
	fprintf (
		stderr, "%s: ERR unknown symbol in %s\n",
		argv[0], args.inPath);
	return EXIT_FAILURE;
}

int readVarName (FILE *src, int *ch, char *dest) {
	for (int i = 0; i < 15; i++) {
		if (*ch == EOF) { return 1; }
		if (*ch == ' ' || *ch == '\t' || *ch == '\n') { break; }
		dest[i] = (char)(*ch);
		dest[i + 1] = 0;
		*ch = fgetc(src); // get next char
	}
	return 0;
}
