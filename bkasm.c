#include <stdio.h>
#include <stdlib.h>

#define MEM_SIZE 4096

typedef struct var {
  char name[16];
  u_int16_t addr;
  u_int16_t value;
} Var;

typedef struct oper {
  u_int8_t opcode;
  char var[16];
  u_int16_t addr;
} Oper;

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "%s: please provide input and output files\n", argv[0]);
    return EXIT_FAILURE;
  }

  printf("%s: compiling %s ===> %s\n", argv[0], argv[1], argv[2]);
  
  //u_int16_t memory[MEM_SIZE] = {0};
  size_t varcount = 0,
         varsize  = 4;
  Var *vars = malloc(varsize * sizeof(Var));

  FILE *in = fopen(argv[1], "r");

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
    for (int i = 0; i < 15; i++) {
      if (ch == EOF) goto premature_eof_err;
      if (ch == ' ') break;
      var->name[i] = ch;
      var->name[i + 1] = 0;
      ch = fgetc(in); // get next char
    }

    // skip whitespace
    while ((ch = fgetc(in)) == ' ' || ch == '\t');

    int mult =  256;
    var->value = 0;
    for (int i = 0; i < 3; i++) {
      // if we reach whitespace its time to stop!!!
      if(ch == ' ' || ch == '\t' || ch == '\n') break;
      // only accept valid hex chars
      if(ch < 47 || (ch > 57 && ch < 65 ) || (ch > 70 && ch < 97) || ch > 102) {
        goto invalid_hex_err;
        break;
      }
      
           if (ch <= 57) ch -= 48;
      else if (ch <= 90) ch -= 55;
      else               ch -= 87;
      ch *= mult;
      var->value += ch;
      
      mult /= 16;
      ch = fgetc(in);
    }

    // skip trailing stuff
    while (ch != '\n' && ch != EOF) ch = fgetc(in);
    
    printf("got variable:\t[%s]\t[%03x]\n", var->name, var->value);
  }

  // go to start of new line
  while (ch != '\n' && ch != EOF) ch = fgetc(in);
  printf("%s: data section terminated\n", argv[0]);

  // read operations

  int opercount = 0,
      opersize  = 16;
  Oper *opers = malloc(opersize * sizeof(Oper));

  while((ch = fgetc(in)) != EOF) {
    // skip beginning whitespace, if there is any.
    while((ch == ' ' || ch == '\t' || ch == '\n') && ch != EOF)
      ch = fgetc(in);

    // get opcode from symbol
    u_int8_t opcode;
    switch (ch) {
      case '<':
        if ((ch = fgetc(in)) == '-') {
          opcode = 0x0;
        } else if (ch == '<') {
          opcode = 0xe;
        } else goto invalid_oper_err;
        break;
      case '-':
        switch (fgetc(in)) {
          case '>': opcode = 0x1; break;
          case '=': opcode = 0x5; break;
          case '-': opcode = 0x6; break;
          default: goto invalid_oper_err;
        }
        break;
      case 'x':
        if ((ch = fgetc(in)) == 'x') opcode = 0x2;
        else goto invalid_oper_err;
        break;
      case '+':
        if ((ch = fgetc(in)) == '=') {
          opcode = 0x3;
        } else if (ch == '+') {
          opcode = 0x4;
        } else goto invalid_oper_err;
        break;
      case '?':
        if ((ch = fgetc(in)) == '?') opcode = 0x7;
        else goto invalid_oper_err;
        break;
      case 'g':
        if ((ch = fgetc(in)) == 'g') opcode = 0x8;
        else goto invalid_oper_err;
        break;
      case 'i':
        if ((ch = fgetc(in)) == 'f' && (ch = fgetc(in)) == ' ') {
          switch (fgetc(in)) {
            case '>': opcode = 0x9; break;
            case '=': opcode = 0xa; break;
            case '<': opcode = 0xb; break;
            case '!': opcode = 0xc; break;
            default: goto invalid_oper_err;
          }
        } else goto invalid_oper_err;
        break;
      case ':':
        if ((ch = fgetc(in)) == ':') opcode = 0x10;
        else goto invalid_oper_err;
        break;
      case '>':
        if ((ch = fgetc(in)) == '>') opcode = 0xd;
        else goto invalid_oper_err;
        break;
      case 'H':
        if (
          (ch = fgetc(in)) == 'A' &&
          (ch = fgetc(in)) == 'L' &&
          (ch = fgetc(in)) == 'T'
        ) opcode = 0xf;
        else goto invalid_oper_err;
        break;
    }
    
    // skip whitespace
    while ((ch = fgetc(in)) == ' ' || ch == '\t');

    if(opcode < 0x10) {
      if(++opercount > opersize) {
        opersize *= 2;
        opers = realloc(opers, opersize * sizeof(Oper));
      }
      Oper *oper = &(opers[opercount - 1]);
      oper->opcode = opcode;

      if (opcode == 0xf)
        oper->var[0] = 0;
      else {
        // get var name
        for (int i = 0; i < 15;) {
          if (ch == EOF) goto premature_eof_err;
          if (ch == ' ' || ch == '\t' || ch == '\n') break;
          oper->var[i]   = ch;
          oper->var[++i] = 0;
          ch = fgetc(in); // get next char
        }
      }
      
      printf("got operation:\t[%01x]\t[%s]\n", oper->opcode, oper->var);
    } else {
      
    }

    // skip trailing stuff
    while (ch != '\n' && ch != EOF) ch = fgetc(in);

  }
  
  printf("%s: program section terminated\n", argv[0]);
  
  return EXIT_SUCCESS;
  
  premature_eof_err:
  fprintf (
    stderr, "%s: ERR reached end of file before program start in %s\n",
    argv[0], argv[1]
  );
  return EXIT_FAILURE;

  invalid_hex_err:
  fprintf (
    stderr, "%s: ERR invalid hex digit in %s: [%c]\n",
    argv[0], argv[1], ch
  );
  return EXIT_FAILURE;

  invalid_oper_err:
  fprintf (
    stderr, "%s: ERR opcode in %s\n",
    argv[0], argv[1]
  );
  return EXIT_FAILURE;
}
