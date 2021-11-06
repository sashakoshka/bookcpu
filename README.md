# bookcpu

`bookcpu` is a minimalist 16 instruction emulated cpu that I found in a
textbook. This repository contains a program to load and run compatible
binaries, and a program that allows you to compile such binaries using a
symbolic language thats sort of kind of similar to assembly.

There are plans in the works to:

1. Support a slightly more powerful instruction set that will be derived from
   the current one that still has a size of 16
2. Port the derived instruction set to a redstone computer in Minecraft

## Legacy Instruction Set

### Opcodes

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

### Opcodes and Assembly Symbols

| Opcode | Symbol 
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

## New Instruction Set
The main reason for this new set is the inability of the current set to perform
iterative operations. Basically, memory addresses have to be hardcoded - you
can't access or manipulate the value at a memory address that is stored within
another memory address. This makes it very difficult to do things like print out
strings. The only way to accomplish this is to programatically overwrite
instructions.

To solve this issue, the HALT instruction was removed. In its place was put a
load to pointer instruction. Basically, the machine now has a new register: the
pointer. Any operation that deals with a memory address can specify the address
as `FFF` (4095), and it will instead use the memory address specified by the
pointer register.

The instruction set was also reorganized into a sort of binary tree to make it
easier for a Minecraft redstone computer to execute.

To replace the functionality of the halt instruction, the machine should
interpret a jump to `FFE` (4094) as a halt.

It is my belief that the ability to create programs that can iterate over
memory makes up for the loss of two memory cells.

Aside from iteration, this opens up a whole new world of possibilites. Since
jump instructions can also use the pointer register, a primitive operating
system that utilizes cooperative multitasking to run more than one program at a
time is theoretically possible.

### New Opcodes

| Opcode | Symbol | Description
| :----: | :----- | :----------
| 0      | *=     | Load x to pointer
| 1      | <-     | Load x to register
| 2      | ->     | Store register in x
| 3      | xx     | Set x to 0
| 4      | ++     | Increment x
| 5      | --     | Decrement x
| 6      | +=     | Add x to register
| 7      | -=     | Subtract x from register
| 8      | >>     | Read into x
| 9      | <<     | Write x out
| a      | ??     | Compare register and x
| b      | go     | Jump to x
| c      | if >   | Jump to x if greater than flag is set
| d      | if <   | Jump to x if less than flag is set
| e      | if =   | Jump to x if equals flag is set
| f      | if !   | Jump to x if equals flag is unset

- `::` defines a label
- Putting `*` as the symbol name uses the address in the pointer register
- Putting a `HALT` as the symbol name uses the address `FFE` (4094)
