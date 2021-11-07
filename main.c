#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>

#include "mccharmap.h"

// amount of 16 bit memory cells
#define MEM_SIZE 4096

u_int16_t _getch();

int main(int argc, char **argv) {
  // command line args
  struct {
    int minecraft:1;
    int altch:1;
    int stdin:1;
    int debug:1;
    int help:1;
    char *path;
  } args;

  args.minecraft = 0;
  args.altch     = 0;
  args.stdin     = 0;
  args.debug     = 0;
  args.help      = 0;
  args.path      = NULL;
  for (int i = 1, getSwitches = 1; i < argc; i++) {
    char *ch = argv[i];
    if (*ch == '-' && getSwitches) {
      // this arg has 1 or more switches
      while (*(++ch) != 0) switch (*ch) {
        case '-': getSwitches    = 0; break;
        case 'm': args.minecraft = 1; break;
        case 'c': args.altch     = 1; break;
        case 'x': args.stdin     = 1; break;
        case 'd': args.debug     = 1; break;
        case 'h': args.help      = 1; break;
      }
    }
    // we have a filepath
    else if (args.path == NULL) args.path = ch;
    else {
      fprintf(stderr, "%s: ERR multiple image files given\n", argv[0]);
      goto error;
    }
  }

  if (args.help) {
    printf("Usage: %s [options] [image]\n", argv[0]);
    puts("Options:");
    puts("  -m    Enable minecraft instruction set");
    puts("  -c    Enable minecraft charset");
    puts("  -x    Read image file from stdin");
    puts("  -d    Enable debug logging");
    puts("  -h    Show help");
    goto exit;
  }

  if (args.path == NULL && !args.stdin) {
    fprintf(stderr, "%s: ERR no image file given\n", argv[0]);
    goto error;
  }

  // open file (or use stdin)
  u_int8_t buffer[MEM_SIZE * 2] = {0};
  FILE *image = NULL;
  if (args.stdin) image = stdin;
  else            image = fopen(args.path, "r");

  if (image == NULL) {
    fprintf(stderr, "%s: ERR could not open file %s\n", argv[0], args.path);
    goto error;
  }

  // read file into buffer
  int ch, i = 0;
  while(i < MEM_SIZE && (ch = fgetc(image)) != EOF) {
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
    if (args.debug) printf (
      "debug: %03X: %01X %03X [%04X] %04X %04X %01X %01X %01X\n",
      counter, opcode, addr, memory[addr], reg, ptr, flag_gt, flag_eq, flag_lt
    );

    if (args.minecraft) {
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
          if (ch > 127) ch = 0;
          else if (ch >= 'a' && ch <= 'z') ch -= 32;
          if (args.debug) printf (
            "debug: got char %c which is %02x -> %02X\n", ch, ch, asciiToMc[ch]
          );
          switch (ch) {
            default:
              memory[addr] = asciiToMc[ch];
              break;
          }
        } break;
        case 0x9: {
          int ch = memory[addr] & 0x3F;
          if (ch < 6) {
            switch (ch) {
              case 1:
                fputs("\033[2J", stdout);
                break;
              case 2:
                fputs("\033[1A", stdout);
                break;
              case 3:
                fputs("\033[1B", stdout);
                break;
              case 4:
                fputs("\033[1D", stdout);
                break;
              case 5:
                fputs("\033[1C", stdout);
                break;
              default: putchar(0);
            }
          } else putchar(mcToAscii[ch]);
        } break;
        case 0xa:
          flag_gt = memory[addr] >  reg;
          flag_eq = memory[addr] == reg;
          flag_lt = memory[addr] <  reg;
          break;
        case 0xb: counter = addr - 1;  break;
        
        case 0xc: if(flag_gt)  counter = addr - 1; break;
        case 0xd: if(flag_eq)  counter = addr - 1; break;
        case 0xe: if(flag_lt)  counter = addr - 1; break;
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
        case 0x8:              counter = addr - 1; break;
        case 0x9: if(flag_gt)  counter = addr - 1; break;
        case 0xa: if(flag_eq)  counter = addr - 1; break;
        case 0xb: if(flag_lt)  counter = addr - 1; break;
        case 0xc: if(!flag_eq) counter = addr - 1; break;
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

