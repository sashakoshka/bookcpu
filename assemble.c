#include <stdio.h>
#include <stdlib.h>

#define MEM_SIZE 4096

int main(int argc, char **argv) {
  if (argc < 3) {
    fprintf(stderr, "%s: please provide input and output files\n", argv[0]);
    return EXIT_FAILURE;
  }
  
  u_int16_t memory[MEM_SIZE] = {0};

  FILE *in = fopen(argv[1], "r");

  char symbol[2];
  int addr;
  int index = 0;
  while(1) {
    
  }
  
  return EXIT_SUCCESS;
}
