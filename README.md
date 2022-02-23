# Instruction Set Architecture

#### To Compile: `g++ -o isa isa.cpp -std=c++11`

### Definitions and acronyms:

| Acronym | Definition                   |
| ------- | ---------------------------- |
| ISA     | Instruction Set Architecture |
| F/D/E/  | Fetch... Decode... Execute   |
| RD      | Register Destination         |
| RS      | Register Source              |


### Instruction Set

The instruction set architecture will be a 3 operand architecture - this is done as it is more efficent than 2 operand instructions (however it is not any more powerful)

| Implemented | Instruction     | Definition                                                                                  | Notes                                                            |
| ----------- | --------------- | ------------------------------------------------------------------------------------------- | ---------------------------------------------------------------- |
| #           | ADD rd rs1 rs2  | Adds the values that are directly in                                                        |                                                                  |
| #           | ADDI rd rs1 n   | Adds the immediate value to rs and stores it in rd                                          |                                                                  |
| #           | SUB rd rs1, rs2 | Subtracts rs2 from rs1 and puts it in rd (rd= rs1 - rs2)                                    |                                                                  |
| #           | MUL rd rs1 rs2  | Multiples rs1 and rs2 and the value goes into rd (rd = rs1*rs2)                             | Multiple passes required but not currently implemented           |
| #           | DIV rd rs1 rs2  | Integer divsion of rs1 by rs2 with the result stored in rd (rd = rs1 // rs2)                |                                                                  |
| #           | CMP rd rs1 rs2  | Compares rs1 and rs2; if rs1 < rs2, rd = 1; if rs1 = rs2, rd = 0; if rs1 > rs2, rd = 1      |                                                                  |
|             |                 |                                                                                             |                                                                  |
| #           | LD rd rs        | Loads a value into rd from the address stored in rs                                         | Made redundent by LDA                                            |
| #           | LDI rd n        | Loads an immediate value n into the register rd                                             |                                                                  |
| #           | LID rd rs       | Loads the value into rd from the address that is pointed to by the address in rd            | Multiple passes required but not currently implemented           |
| #           | LDA rd rs1 rs2  | Loads the value indexed rs2 addresses away from rs1 into rd                                 |                                                                  |
| ////        | LDS rd rs1 rs2  | Loads the value indexed                                                                     |                                                                  |
|             |                 |                                                                                             |                                                                  |
| #           | STO rd rs       | Stores the value that is in rs into the address that is found in rd                         |                                                                  |
| #           | STOI rd n       | Stores a value into an immediate address                                                    |                                                                  |
|             |                 |                                                                                             |                                                                  |
| #           | AND rd rs1 rs2  | Bitwise logical and operation between rs1 and rs2 - result in rd                            |                                                                  |
| #           | OR rd rs1 rs2   | Bitwise logical OR operation between rs1 and rs2, result stored in rd                       |                                                                  |
| #           | NOT rd rs       | Bitwise logical NOT operation on rs, result stored in rd                                    |                                                                  |
| #           | LSHFT rd rs n   | Leftshift operation on rs by n bits, result stored in rd                                    | Maybe change so that the value is shifted by rs not an immediate |
| #           | RSHFT rd rs n   | Rightshift operation on rs by n bits, results stored in rs                                  | Maybe change so that the value is shifted by rs not an immediate |
|             |                 |                                                                                             |                                                                  |
|             | B rs            | Branches to the absolute value stored in rs (loads this address into the PC)                |                                                                  |
|             | BI rs1 rs2      | Branches to the address stored rs2 addresses away from rs1 (loads this address into the PC) |                                                                  |
| #           | HALT            | Ends the program                                                                            |                                                                  |
| #           | NOP             | No operation                                                                                |                                                                  |
|             |                 |                                                                                             |                                                                  |


The list of instructions that have been implemented are:
    - ADDI rd, rs, n            - Adds the immediate value to rs and stores it in rd
    - ADDII rd, n, m            - Adds the 2 immediate values and saves it into the rd    // Might not be super useful
    
    - SUB rd, r1, r2            - Subtracts r2 from r1 (r1 - r2) and saves result in rd

    - MUL rd, rs, rs            - Multiply both rs and the rs together and store in rd
    - DIV rd, rs, rs            - 

    - LD rd, rs                 - Loads the value that is held in the address which is directly stored in rs
    - LDI rd, const             - Immediate loading of a constant into rd
    - LD                        - Indirect loading - loads the address which the address in rs is pointing to       // MIGHT BE ABLE TO COMBINE THE NORMAL LOAD WITH THIS ONE
    - LDS rd, rs, const         - Scalar loading - loads the value starting from rs with a scaled index const into rd

    - STO rd, rs                - Stores the value that is in rs into the address which is directly found in rd 

            - LDI rd, const             - Loads the immediate constant into the rd
            - LDA rd, address           - Load from the value in the address
            - LD rd, rs                 - Loads the address which is found value from the which is in the 
            - LDS rd, rs                - L
            - STOI const, rs            - Stores the 
            - STOA rd, 

    - AND rd, rs, rs            - Logical and between both rs - result in rd
    - OR                        - Logical or between both rs - result in rd
    - NOT rd, rs                - Logical NOT for a register - result in rd
    - LSHFTI rd, need           - Left shift by the immediate 
    - LSHIFT rd, rs             - Left shift with v

    - BRANCH



    - RET                       - Return from procedure
    - HALT                      - Stops the program (and rings a bell)


### Registers

Registers for the ISA are defined below and are currently stored in seperate variables but will be stored as a single arrray which acts as register file:

#### Special Registers

| Acronym | Register Name                | Use                                               | Notes                          |
| ------- | ---------------------------- | ------------------------------------------------- | ------------------------------ |
| CIR     | Current Instruction Register | holds the current instruction that is being F/D/E |                                |
| PC      | Program Counter              | Stores the address of the current instruction     |                                |
| Imm     | Immediate                    | Holds the value for the a current immediate       | Not currently used/implemented |

Might need to implement a NEXT Program counter - this would be used to keep track of the next address to run from with the PC keeping track of the Current instruction ONLY

    - General Purpose Registers
        - r0 - r15  - All general purpose registers


System flags to indentify what state the processor is currently in: - Not 100% sure if we need them but they might be super useful????



Memory
    - Currently the implementation for how memory works is quite simple, there is only an instruction memory and a data memory, the instruction memory is 



The below is all related to each phase of an instruction and how it runs in the pipeline


Fetch:
    - The fetch in this program will us the current address that is in the program counter PC - this means that we need to increment the PC after we have fetched the instuction; if we increment before we will miss the first instruction. The instruction that is in memory is loaded into the CIR


Decode:
    - Decodes the current instruction by splitting up the ASCII text into the instruction, RD, RS_1 and RS_2 the CIR and converting it into 
     
Execute:
    - Executes the decoded instruction


