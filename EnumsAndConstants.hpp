/* Instructions */
enum Instruction {
    ADD,
    ADDI,
    ADDF,
    SUB,
    SUBF,
    MUL,
    MULO,
    MULFO,
    DIV,
    DIVF,
    CMP,

    LD,
    LDD,
    LDI,
    LID,
    LDA,

    STO,
    STOI,

    AND,
    OR,
    NOT,
    LSHFT,
    RSHFT,

    JMP,
    JMPI,
    BNE,
    BPO,
    BZ,

    HALT,
    NOP,
    MV,
    MVHI,
    MVLO,
};

/* Instruction Struct - used for passing entire instructions between Reservation stations and EUs */
 struct DecodedInstruction {
     Instruction OpCode;
     int DEST;      // Destination for the instruction  (optional) 
     int IN0;       // Input 1
     int IN1;       // Input 2                          (optional) 
     int IMM;       // Immediate                        (optional) 
     int OUT;       // Output for the instruction       (optional) 

     void print(){
         std::cout << "DEST: " << DEST << " IN0: " << IN0 << " IN1: " << IN1 << " IMM: " << IMM << " OUT: " << OUT << std::endl;
     }
     
 };


/* Registers */
#pragma region Registers
enum Register { R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15, X }; // X acts a dummy regsiter - doesn't exist but acts as a way to have uniform structure to all instructions that the ISA uses
enum FP_Register {FP0, FP1, FP2, FP3};


/* States of a single pipeline stage */
// Empty - nothing in the stage; Current - the stage is currently running; Next - the stage has completed and is ready to move to the next stage
enum StageState {Empty, Current, Next, Block};

/* Execution States of a single EU */
enum EUState {IDLE, READY, RUNNING, DONE};

/* Constants */
const int SIZE_OF_INSTRUCTION_MEMORY = 256;     // size of the read-only instruction memory
const int SIZE_OF_DATA_MEMORY = 256;            // pretty much the heap and all