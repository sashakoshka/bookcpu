# bookcpu

`bookcpu` is a minimalist 16 instruction emulated cpu that I found in a
textbook. This repository contains a program to load and run compatible
binaries, and a program that allows you to compile such binaries using a
symbolic language thats sort of kind of similar to assembly.

There are plans in the works to:

1. Support a slightly more powerful instruction set that will be derived from
   the current one that still has a size of 16
2. Port the derived instruction set to a redstone computer in Minecraft

## Opcodes

| Opcode | Operation   | Description
| :----: | :---------- | :----------
| 0      | LOAD X      | Load value of address to register
| 1      | STORE X     | Store value of register to address
| 2      | CLEAR X     | Set value of address to zero
| 3      | ADD X       | Adds the value of address to register
| 4      | INCREMENT X | Increments the value in address
| 5      | SUBTRACT X  | Subtracts the value of address from register
| 6      | DECREMENT X | Decrements the value in address
| 7      | COMPARE X   | Compares register and value of address
| 8      | JUMP X      | Jumps to address
| 9      | JUMPGT X    | Jumps if greater than flag is set
| a      | JUMPEQ X    | Jumps if equal flag is set
| b      | JUMPLT X    | Jumps if less than flag is set
| c      | JUMPNEQ X   | Jumps if equal flag isn't set
| d      | IN X        | Waits for keypress, and stores the value in address
| e      | OUT X       | Prints ascii char in address
| f      | HALT        | Ends the program

## Opcodes and Assembly Symbols

| Opcode | Symbol |
| :----: | :----- |
| 0      | <-     |
| 1      | ->     |
| 2      | xx     |
| 3      | +=     |
| 4      | ++     |
| 5      | -=     |
| 6      | --     |
| 7      | ??     |
| 8      | go     |
| 9      | if >   |
| a      | if =   |
| b      | if <   |
| c      | if !   |
| d      | >>     |
| e      | <<     |
| f      | HALT   |

`::` defines a label
