#include <string>

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
    STOA,

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

/* Registers */
enum Register { R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15}; // X acts a dummy regsiter - doesn't exist but acts as a way to have uniform structure to all instructions that the ISA uses
enum FP_Register {FP0, FP1, FP2, FP3};


/* States of a single pipeline stage */
// Empty - nothing in the stage; Current - the stage is currently running; Next - the stage has completed and is ready to move to the next stage
//enum StageState {Empty, Current, Next, Block};

enum InstState {EMPTY, CURRENT, NEXT, BLOCK};

/* Execution States of a single EU */
//enum EUState {IDLE, READY, RUNNING, DONE};

/* NonDecodedInstruction - Only used in IF*/
struct NonDecodedInstruction {
    InstState state = EMPTY;
    int uniqueInstructionIdentifer; // Uniquely identifies an instruction in the pipeline so that it can be compared to other instructions later on - NOTE: an integer overflow on this is OK

    std::string instruction = "";

    int PC;
};

/* C++11 doesnt have std::optional so im gonna have to makemy own */
template <class T>
class Optional{
    private:
        T value;
        bool hasValue;

    public:

    Optional(){
        this->hasValue = false;
    }

    Optional(T val){
        this->hasValue = true;
        this->value = val;
    }

    T Value(){
        // Has no sanitising here because c++ seems to hate it
        
        return value;
    }

    void Value(T val){
        this->value = val;
        this->hasValue = true;
    }

    void RemoveValue(){
        this->hasValue = false;
    }

    bool HasValue(){
        return hasValue;
    }
};



/* Instruction Struct - used for passing entire instructions between Reservation stations and EUs */
struct DecodedInstruction {
    std::string asString = "";

    int uniqueInstructionIdentifer; // Uniquely identifies an instruction in the pipeline so that it can be compared to other instructions later on - NOTE: an integer overflow on this is OK

    InstState state = EMPTY;    // State of the instruction
    bool IsWriteBack = true;   // Default IS writeback
    bool IsBranchInst = false;
    bool IsMemoryOperation = false;

    bool branchFlag = false;    // Used to tell the processor that this instruction caused a branch

    Instruction OpCode;         // OP code of the instruction

    int sideOfBranch = 2147483647;

    int previousPhysDest = -1;
    
    std::vector<std::string> SplitInst;
    
    int Ard = -1;   // Im stupid so I have to now implement an architecture destination register attribute to this struct as I cannot be asked to re write my code in a way that I have to use rd for just arch registers again

    int rd = -1;    // Actual register used in instruction for destination       - Used to fight RAW hazards
    int rs0 = -1;   // Actual register used in instruction for source register 0 - Used to fight RAW hazards
    int rs1 = -1;   // Actual register used in instruction for source register 1 - Used to fight RAW hazards

    int DEST = -1;       // Destination for the instruction  (optional) 
    int IN0;        // Input 1                          (optional)
    int IN1;        // Input 2                          (optional) 
    int IMM;        // Immediate                        (optional) 
    int OUT;        // Output for the instruction       (optional) 

    void print(){
        std::cout << "RD: " << rd << " DEST: " << DEST << " RS0: " << rs0 << " IN0: " << IN0 << " RS1: " << rs1 << " IN1: " << IN1 << " IMM: " << IMM << " OUT: " << OUT << std::endl;
    }

    void printHuman(){
        if (this->state == EMPTY || this->asString.empty()){
            std::cout << "\t"; 
        } else {
            std::cout << SplitInst.at(0) << " ";
            for (int i = 1; i < SplitInst.size() && i < 4; i++){
                if (i == 1){
                    if (OpCode == STOI) {
                        std::cout << this->IMM << " ";
                    } else {
                        std::cout << "PR" << this->rd << " ";
                    }
                } 
                else if (i == 2){
                    if (this->rs0 == -1) {
                        std::cout << this->IMM << " ";
                    } else {
                        std::cout << "PR" << this->rs0 << " ";
                    }
                } else if (i == 3){
                    if (this->rs1 == -1){
                        std::cout << this->IMM << " ";
                    } else {
                        std::cout << "PR" << this->rs1;
                    }
                }
            }
        }
        std::cout << " :" << this->state << ":";

    }

};


/* Constants */
const int SIZE_OF_REGISTER_FILE = 16;           // Number of architectural register
const int PHYSICAL_REGISTER_FILE_SCALER = 4;
const int SIZE_OF_INSTRUCTION_MEMORY = 256;     // size of the read-only instruction memory
const int SIZE_OF_DATA_MEMORY = 256;            // pretty much the heap and all

const int MAX_RV_SIZE = 4;
const int MAX_NUMBER_OR_ROB_ENTRIES = 32;
const int MAX_NUMBER_OF_BRANCH_SIDES = 4;

const int NUMBER_OF_ALU = 2;
const int NUMBER_OF_BU = 1;
const int NUMBER_OF_LSU = 1;

const int SUPERSCALAR_WIDTH = 1;