#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

// amount of 16 bit memory cells
#define MEM_SIZE 4096

int main(int argc, char **argv) {
  // disable line buffering
  struct termios oldt, newt;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~ICANON;
  newt.c_lflag &= ~ECHO;
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  if (argc < 2) {
    fprintf(stderr, "%s: no image file given", argv[0]);
    goto error;
  }
  
  u_int8_t buffer[MEM_SIZE * 2] = {0};
  FILE *image = fopen(argv[1], "r");

  // read file into buffer
  int ch, i = 0;
  while(i < MEM_SIZE && (ch = fgetc(image)) != EOF) {
    // for some reason they need to be swapped? idfk
    buffer[i++] = fgetc(image);
    buffer[i++] = ch;  
  }

  // cast buffer to u_int_16
  u_int16_t *memory = (u_int16_t *)buffer;
  u_int16_t reg;
  int counter = 0;
  int flag_gt = 0, flag_eq = 0, flag_lt = 0;
  
  while (counter < MEM_SIZE) {
    int opcode = memory[counter] >> 12;
    int addr   = memory[counter] & 0x0FFF;
    
    switch (opcode) {
      case 0x0: reg = memory[addr]; break;
      case 0x1: memory[addr] =  reg; break;
      case 0x2: memory[addr] =  0;   break;
      case 0x3: reg += memory[addr]; break;
      case 0x4: memory[addr] ++;     break;
      case 0x5: reg -= memory[addr]; break;
      case 0x6: memory[addr] --;     break;
      case 0x7:
        // TODO: validate this
        flag_gt = reg >  memory[addr];
        flag_eq = reg == memory[addr];
        flag_lt = reg <  memory[addr];
        break;
      case 0x8:              counter = addr - 1; break;
      case 0x9: if(flag_gt)  counter = addr - 1; break;
      case 0xa: if(flag_eq)  counter = addr - 1; break;
      case 0xb: if(flag_lt)  counter = addr - 1; break;
      case 0xc: if(!flag_eq) counter = addr - 1; break;
      case 0xd: memory[addr] = getchar(); break;
      case 0xe: putchar(memory[addr]); break;
      case 0xf: goto exit;
    }
    counter++;
  }

  exit:
  tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
  return EXIT_SUCCESS;
  
  error:
  tcsetattr( STDIN_FILENO, TCSANOW, &oldt);
  return EXIT_FAILURE;
}
