# Instruction Set Architecture

#### To Compile: `g++ -o isa isa.cpp -std=c++11`
#### To Run: `./isa <program_name>`

### Definitions and acronyms:

| Acronym   | Definition                                                   |
| --------- | ------------------------------------------------------------ |
| ISA       | Instruction Set Architecture                                 |
| F/D/E/M/W | Fetch... Decode... Execute... Memory Access... Write Back... |
| RD        | Register Destination                                         |
| RS        | Register Source                                              |

---
## Instruction Set

The instruction set architecture will be a 3 operand architecture - this is done as it is more efficent than 2 operand instructions (however it is not any more powerful)

| Implemented | Instruction      | Definition                                                                               | Memory Access? | Write back? | Notes                                                                                         |
| ----------- | ---------------- | ---------------------------------------------------------------------------------------- | -------------- | ----------- | --------------------------------------------------------------------------------------------- |
| #           | ADD rd rs1 rs2   | Adds the values that are directly in                                                     |                | Y           |
| #           | ADDI rd rs1 n    | Adds the immediate value to rs and stores it in rd                                       |                | Y           |
| #           | SUB rd rs1, rs2  | Subtracts rs2 from rs1 and puts it in rd (rd= rs1 - rs2)                                 |                | Y           |
| #           | MUL rd rs1 rs2   | Multiples rs1 and rs2 and the value goes into rd (rd = rs1*rs2). Overflow IS truncated   |                | Y           | Multiple passes required but not currently implemented                                        |
| #           | DIV rd rs1 rs2   | Integer divsion of rs1 by rs2 with the result stored in rd (rd = rs1 // rs2)             |                | Y           |
| #           | CMP rd rs1 rs2   | Compares rs1 and rs2; if rs1 < rs2, rd = -1; if rs1 = rs2, rd = 0; if rs1 > rs2, rd = 1  |                | Y           |
|             |                  |                                                                                          |                |             |
| X           | LD rd rs         | Loads a value into rd from the address stored in rs                                      | Y              | Y           | Made redundent by LDA                                                                         | WARNING BUGGY!!! |
|             | LDD rd n         | Loads the value in the memory address pointed to by n                                    | Y              | Y           |
| #           | LDI rd n         | Loads an immediate value n into the register rd                                          |                | Y           |
| To Be Done  | LID rd rs        | Loads the value into rd from the address that is pointed to by the address in rd         | Y              | Y           | DOUBLE MEMORY ACCESS - currently not implemented                                              |
| #           | LDA rd rs1 rs2   | Loads the value indexed rs2 addresses away from rs1 into rd                              | Y              | Y           |
| NO          | LDS rd rs1 rs2   | Loads the value indexed                                                                  | Y              | Y           | NOT TO BE IMPLEMENTED                                                                         |
|             |                  |                                                                                          |                |             |
| #           | STO rd rs        | Stores the value that is in rs into the memory address that is found in rd               | Y              |             | Currently accessing the registerFile - feels illegal that it's being done in EXE but not sure |
| #           | STOI n rs        | Stores a value into an immediate address                                                 | Y              |             |
| To Be Done  | STOA rd rs1 rs2  | Store rs2 in the memory address rd + rs1                                                 |                |             | NOT YET IMPLEMENTED                                                                           |
|             |                  |                                                                                          |                |
| #           | AND rd rs1 rs2   | Bitwise logical and operation between rs1 and rs2 - result in rd                         |                | Y           |
| #           | OR rd rs1 rs2    | Bitwise logical OR operation between rs1 and rs2, result stored in rd                    |                | Y           |
| #           | NOT rd rs        | Bitwise logical NOT operation on rs, result stored in rd                                 |                | Y           |
| #           | LSHFT rd rs1 rs2 | Leftshift operation on rs1 by rs2 bits, result stored in rd                              |                | Y           | Maybe change so that the value is shifted by rs not an immediate                              |
| #           | RSHFT rd rs1 rs2 | Rightshift operation on rs1 by rs2 bits, results stored in rs                            |                | Y           | Maybe change so that the value is shifted by rs not an immediate                              |
|             |                  |                                                                                          |                |             |
| #           | JMP rd           | Unconditional branch to the absolute value stored in rd (loads this address into the PC) |                | Y           |
| #           | JMPI rd          | Unconditional branch to the address PC+rs                                                |                | Y           |
| #           | BNE rd rs        | Proceedes a CMP operation: Conditional branch to rd if rs is negative                    |                | Y           | CMP is only negative when rs1 < rs2                                                           |
| #           | BPO rd rs        | Proceeds a CMP operation: Conditional branch to rd if rs is positive                     |                | Y           | CMP is only positive when rs1 > rs2                                                           |
| #           | BZ rd rs         | Proceeds a CMP operation: Conditional branch to rd if rs is zero                         |                | Y           | CMP is only 0 when rs1 = rs2                                                                  |
|             |                  |                                                                                          |                |             |
| #           | HALT             | Ends the program                                                                         |                |             |
| #           | NOP              | No operation                                                                             |                |             |
| NO          | RET              | Loads the return address into the PC so that a procedure can be returned                 |                |             |

## Example Programs
Currently only have one example program (vector addition) but have some tests too.

# Everything below here is out of date/not complete!!!
---
## Registers

Registers for the ISA are defined below and are currently stored in seperate variables but will be stored as a single arrray which acts as register file:

### Special Registers

| Acronym | Register Name                | Use                                               | Notes                          |
| ------- | ---------------------------- | ------------------------------------------------- | ------------------------------ |
| CIR     | Current Instruction Register | holds the current instruction that is being F/D/E |                                |
| PC      | Program Counter              | Stores the address of the current instruction     |                                |
| Imm     | Immediate                    | Holds the value for the a current immediate       | Not currently used/implemented |

OUT OF DATE!!!

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

Memory Access:
    - 

Written Back:
    - 


