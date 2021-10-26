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
