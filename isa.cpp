#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <stdexcept>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <thread>

using namespace std;


/* Debugging Flags */
bool PRINT_REGISTERS_FLAG = false;
bool PRINT_MEMORY_FLAG = false;
bool PRINT_STATS_FLAG = false;


/* Constants */
const int SIZE_OF_INSTRUCTION_MEMORY = 256;     // size of the read-only instruction memory
const int SIZE_OF_DATA_MEMORY = 256;            // pretty much the heap and all

int amount_of_instruction_memory_to_output = 8;  // default = 8


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


/* Registers */
#pragma region Registers
enum Register { R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15, X }; // X acts a dummy regsiter - doesn't exist but acts as a way to have uniform structure to all instructions that the ISA uses
enum FP_Register {FP0, FP1, FP2, FP3};

/* State */
// Empty - nothing in the stage; Current - the stage is currently running; Next - the stage has completed and is ready to move to the next stage
enum State {Empty, Current, Next};

State IF_State = Empty;
State ID_State = Empty;
State EX_State = Empty;
State MA_State = Empty;
State WB_State = Empty;


/* "Register File" - currently just a bunch of variables */
std::array<int, 16> registerFile;    // All 16 general purpose registers
std::array<float, 4> floatingPointRegisterFile;

int PC;                     // Program Counter
std::string CIR;            // Current Instruction Register
int IMMEDIATE;              // Immediate register used for immediate addressing
    

// ALU registers
Instruction OpCodeRegister = NOP;       // Stores the decoded OpCode that was in the CIR
int  ALU0, ALU1, ALU_OUT;               // 2 input regsiters for the ALU
float ALU_FP0, ALU_FP1;                 // 2 input registers for the ALU where FP calculations are occuring
int HI, LO;                             // High and Low parts of integer multiplication
int ALUD;                               // Destination register for the output of the ALU


// MEMORY ACCESS Registers
int MEMD;                          // Destination address for the position in memory (STO operation) or the register in the register file (LD operation)
int MEM_OUT;                            // Output of the memory (only used in load operations)


// WRITE BACK registers
int WBD;                           // Write back destination - stores the destination register for the memory in the memory output to be stored/held

#pragma endregion Registers


/* System Flags */
bool systemHaltFlag = false;            // If true, the system halts

bool memoryReadFlag = false;            // Used pass on instruction information on whether an instruction is needed to READ from memory (used in the MEMORY ACCESS stage)
bool memoryWriteFlag = false;           // Used pass on instruction information on whether an instruction is needed to WRITE to memory (used in the MEMORY ACCESS stage)

bool MEM_writeBackFlag = false;
bool writeBackFlag = false;

bool branchFlag = false;                // Used to tell the fetch stage that we have branched and so we do not need to increment at this point


/* Memory */
std::array<std::string, SIZE_OF_INSTRUCTION_MEMORY> instrMemory;
std::array<int, SIZE_OF_DATA_MEMORY> dataMemory;


/* ISA Function headers */
void fetch();
void decode();  
void execute();
void memoryAccess();
void writeBack();

/* ISA helpers */
void flushPipeline();

/* Non-ISA function headers */
void loadProgramIntoMemory();
std::vector<std::string> split(std::string str, char deliminator);
Register strToRegister(std::string str);
bool handleProgramFlags(int count, char** arguments);

/* Debugging function headers*/
void outputAllMemory(int cutOff);
void printRegisterFile(int maxReg);
void outputStatistics(int numOfCycles);

/* Debugging/GUI for showing whch Instruction is in which stage */
string IF_inst = "EMPTY";
string ID_inst = "EMPTY";
string EX_inst = "EMPTY";
string MA_inst = "EMPTY";
string WB_inst = "EMPTY";


/* Stats variables */
int numOfCycles = 1;        // Counts the number of cycles (stats at cycle 1 not cycle 0)
int numOfBranches = 0;
int numOfStalls = 0;        // Counts the number of times the pipeline stalls

#pragma region debugging

void outputAllMemory(int cutOff){
    std::string emptyLine = "--------------------------------";     // 32 '-'s to show an empty line

    //std::cout << "\tInstruction Memory" << "              \t\t\t" << "Data Memory\n" << std::endl;
    std::cout << "\tInstruction Memory" << "              \t" << "Data Memory\n" << std::endl;
    for (int i = 0; i < SIZE_OF_INSTRUCTION_MEMORY || i < SIZE_OF_DATA_MEMORY; i++){
        if (i > cutOff) break;
        
        std::cout << i << "\t";
        if (i < instrMemory.size()){
            if (instrMemory.at(i).empty()){
                std::cout << emptyLine;
            } else {
                std::string line = instrMemory.at(i);
                line.insert(line.length(), 32 - line.length(), ' ');
                std::cout << line;
            }
        }
        std::cout << "\t";
        if (i < dataMemory.size()){
            std::cout << dataMemory.at(i);
        }
        std::cout << std::endl;
    } std::cout << std::endl;
}

void printRegisterFile(int maxReg){
    std::cout << std::endl;
    std::cout << "PC: " << PC << std::endl;
    std::cout << "CIR: " << CIR << std::endl;
    for (int i = 0; (i < 16) && (i < maxReg); i++){
        std::cout << "R" << i << ": " << registerFile.at(i) << std::endl;
    }
    std::cout << "IMMEDIATE: " << IMMEDIATE << std::endl;
    std::cout << "ALU0: " << ALU0 << std::endl;
    std::cout << "ALU1: " << ALU1 << std::endl;
    std::cout << "ALU_OUT: " << ALU_OUT << std::endl;
    std::cout << "ALUD: " << ALUD << std::endl;
    std::cout << "\nMEMD: " << MEMD << std::endl;
    std::cout << "MEM_OUT: " << MEM_OUT << std::endl;
    std::cout << "memoryReadFlag: " << memoryReadFlag << std::endl;
    std::cout << "memoryWriteFlag: " << memoryWriteFlag << std::endl;
    std::cout << "MEM_writeBackFlag: " <<MEM_writeBackFlag << std::endl;
    std::cout << "\nWBD: " << WBD << std::endl;
    std::cout << "writeBackFlag: " << writeBackFlag << std::endl;
}


// Outputs all stats here
void outputStatistics(int numOfCycles){
    if (!PRINT_STATS_FLAG) return;

    cout << "\n\n---------- STATISTICS ----------\n" << endl;
    cout << "Total number of cycles:\t\t" << numOfCycles << endl;
    cout << "Total number of branches:\t\t" << numOfBranches << endl;
    cout << "Total number of stalls:\t\t" << numOfStalls << endl;
    cout << "Total number of successfully predicted branches:\t\t" << "Not implemented " << endl;
    cout << "Percent of successfully predicted branches:\t\t" << "Not implemented " << endl;   
}

#pragma endregion debugging

#pragma region F/D/E/M/W/

void fushPipeline(){
    IF_State = Empty;
    IF_inst = "";

    ID_State = Empty;
    ID_inst = "";
}

// The main cycle of the processor
void cycle(){
    // Print memory before running the program
    if (PRINT_MEMORY_FLAG) outputAllMemory(amount_of_instruction_memory_to_output);

    while (!systemHaltFlag) {

        //if (numOfCycles == 26) outputAllMemory(amount_of_instruction_memory_to_output);
        std::cout << "---------- Cycle " << numOfCycles << " starting ----------"<< std::endl;
        //std::cout << "PC has current value: " << PC << std::endl;


        // Non-pipelined 
        //fetch(); decode(); execute(); memoryAccess(); writeBack();
        
    /*
        thread(fetch);
        thread(decode);
        thread(execute);
        thread(memoryAccess);
        thread(writeBack);
    */
        // Pipelined
        writeBack(); memoryAccess(); execute(); decode(); fetch();

        cout << "\nCurrent instruction in the IF: " << IF_inst << endl;
        cout << "Current instruction in the ID: " << ID_inst << endl;
        cout << "Current instruction in the EX: " << EX_inst << endl;
        cout << "Current instruction in the MA: " << MA_inst << endl;
        cout << "Current instruction in the WB: " << WB_inst << endl;
                

        if (PRINT_REGISTERS_FLAG) printRegisterFile(16);

        std::cout << "---------- Cycle " << numOfCycles << " completed. ----------\n"<< std::endl;
        numOfCycles++;
    }
    std::cout << "Program has been halted\n" << std::endl;

    // Print the memory after the program has been ran
    if (PRINT_MEMORY_FLAG) outputAllMemory(amount_of_instruction_memory_to_output);
    if (PRINT_STATS_FLAG) outputStatistics(numOfCycles);
}


// Fetches the next instruction that is to be ran, this instruction is fetched by taking the PCs index 
void fetch(){
    // Change the state of the IF such that it is "currently running"
    IF_State = Current;
    
    // Load the memory address that is in the instruction memory address that is pointed to by the PC
    CIR = instrMemory.at(PC);

    // Increment PC or don't (depending on whether we are on a branch or not)
    if (branchFlag){
        branchFlag = false;
    } else {
        PC++;
    }

    // Debugging/GUI to show the current instr in the processor
    IF_inst = CIR;

    if (CIR.empty()) {
        IF_State = Empty;
        return;
    }

    // INCORRECT \/\/
    /* We DO NOT UPDATE the PC here but instead we do it in the DECODE stage as this will help with pipelining branches later on */
    // Instead of incrementing the PC here we could use a NPC which is used by MIPS and stores the next sequential PC
    // Increment PC
    //pc++;

    std::cout << "CIR has current value: " << CIR << std::endl;
    //std::cout << "Fetched... ";
    
    // IF has ran and now we are ready to move to the next stage
    IF_State = Next;
}


// Takes current instruction that is being used and decodes it so that it can be understood by the computer (not a massively important part)
// Updates PC
void decode(){
    
    // State change for ID
    if (IF_State != Next) {
        // If IF is not ready to move on, then ID cannot progress (i.e. it is empty)
        ID_State = Empty;

        // Debugging/GUI to show that the current instruction is empty
        ID_inst = string("");

        // increments stall count
        numOfStalls += 1;
        return;
    } else {
        // Else it can run
        ID_State = Current;

        // Debugging/GUI to show the current instr in the processor
        ID_inst = IF_inst;
    }

    std::vector<std::string> splitCIR = split(CIR, ' '); // split the instruction based on ' ' and decode instruction like that
    
    // Throws error if there isn't any instruction to be loaded
    if (splitCIR.size() == 0) throw std::invalid_argument("No instruction loaded");
    
    // Load the register values into the ALU's input
    if (splitCIR.size() > 1) {
        // Get set first/destination register
        if      (splitCIR.at(1).substr(0 ,1).compare("r")  == 0 ) ALUD = strToRegister(splitCIR.at(1));
        else if (splitCIR.at(1).substr(0, 2).compare("FP") == 0)  ALUD = (FP_Register) stoi(splitCIR.at(1).substr(1, splitCIR.at(1).length()));

        if (splitCIR.size() > 2) {
            if      (splitCIR.at(2).substr(0, 1).compare("r")  == 0 ) ALU0 = registerFile.at(strToRegister(splitCIR.at(2)));
            else if (splitCIR.at(2).substr(0, 2).compare("FP") == 0)  ALU_FP0 = (FP_Register) stoi(splitCIR.at(2).substr(1, splitCIR.at(2).length()));
            
            if (splitCIR.size() > 3) {
                if      (splitCIR.at(3).substr(0,1).compare("r")  == 0 ) ALU1 = registerFile.at(strToRegister(splitCIR.at(3)));
                else if (splitCIR.at(3).substr(0,1).compare("FP") == 0 ) ALU_FP1 = (FP_Register) stoi(splitCIR.at(3).substr(1, splitCIR.at(3).length()));

            }
        }
    }

    // if statement for decoding all instructions
         if (splitCIR.at(0).compare("ADD")  == 0) OpCodeRegister = ADD;
    else if (splitCIR.at(0).compare("ADDI") == 0) { OpCodeRegister = ADDI; IMMEDIATE = stoi(splitCIR.at(3)); }
    else if (splitCIR.at(0).compare("ADDF") == 0) OpCodeRegister = ADDF;
    else if (splitCIR.at(0).compare("SUB")  == 0) OpCodeRegister = SUB;
    else if (splitCIR.at(0).compare("SUBF") == 0) OpCodeRegister = SUBF;
    else if (splitCIR.at(0).compare("MUL")  == 0) OpCodeRegister = MUL;
    else if (splitCIR.at(0).compare("MULO") == 0) OpCodeRegister = MULO;
    else if (splitCIR.at(0).compare("MULFO")== 0) OpCodeRegister = MULFO;
    else if (splitCIR.at(0).compare("DIV")  == 0) OpCodeRegister = DIV;
    else if (splitCIR.at(0).compare("DIVF") == 0) OpCodeRegister = DIVF;
    else if (splitCIR.at(0).compare("CMP")  == 0) OpCodeRegister = CMP;

    else if (splitCIR.at(0).compare("LD")   == 0) OpCodeRegister = LD;
    else if (splitCIR.at(0).compare("LDD")  == 0) { OpCodeRegister = LDD; IMMEDIATE = stoi(splitCIR.at(2)); } 
    else if (splitCIR.at(0).compare("LDI")  == 0) { OpCodeRegister = LDI; IMMEDIATE = stoi(splitCIR.at(2)); }
    else if (splitCIR.at(0).compare("LID")  == 0) OpCodeRegister = LID;
    else if (splitCIR.at(0).compare("LDA")  == 0) OpCodeRegister = LDA;
    
    else if (splitCIR.at(0).compare("STO")  == 0) OpCodeRegister = STO;
    else if (splitCIR.at(0).compare("STOI") == 0) { OpCodeRegister = STOI; IMMEDIATE = stoi(splitCIR.at(1)); }

    else if (splitCIR.at(0).compare("AND")  == 0) OpCodeRegister = AND;
    else if (splitCIR.at(0).compare("OR")   == 0) OpCodeRegister = OR;
    else if (splitCIR.at(0).compare("NOT")  == 0) OpCodeRegister = NOT;
    else if (splitCIR.at(0).compare("LSHFT")== 0) OpCodeRegister = LSHFT;       // IMMEDIATE = stoi(splitCIR.at(3)); }
    else if (splitCIR.at(0).compare("RSHFT")== 0) OpCodeRegister = RSHFT;       // IMMEDIATE = stoi(splitCIR.at(3)); }

    else if (splitCIR.at(0).compare("JMP")  == 0) OpCodeRegister = JMP;
    else if (splitCIR.at(0).compare("JMPI") == 0) OpCodeRegister = JMPI;
    else if (splitCIR.at(0).compare("BNE")  == 0) OpCodeRegister = BNE;
    else if (splitCIR.at(0).compare("BPO")  == 0) OpCodeRegister = BPO;
    else if (splitCIR.at(0).compare("BZ")   == 0) OpCodeRegister = BZ;

    else if (splitCIR.at(0).compare("HALT") == 0) OpCodeRegister = HALT;
    else if (splitCIR.at(0).compare("NOP")  == 0) OpCodeRegister = NOP;
    else if (splitCIR.at(0).compare("MV")   == 0) OpCodeRegister = MV;
    else if (splitCIR.at(0).compare("MVHI") == 0) OpCodeRegister = MVHI;
    else if (splitCIR.at(0).compare("MVLO") == 0) OpCodeRegister = MVLO;

    else throw std::invalid_argument("Unidentified Instruction: " + splitCIR.at(0));

    //std::cout << "Decoded... ";

    ID_State = Next;
}


// Executes the current instruction
void execute(){

    // Prepare state for EX
    if (ID_State != Next) {
        // If IF is not ready to move on, then ID cannot progress (i.e. it is empty)
        EX_State = Empty;

        // Debugging/GUI to show that the current instruction is empty
        EX_inst = string("");

        // increments stall count
        numOfStalls += 1;
        return;
    } else {
        // Else it can run
        EX_State = Current;

        // Debugging/GUI to show the current instr in the processor
        EX_inst = ID_inst;
    }

    // Passes the instruction destination register or address along
    MEMD = ALUD; 

    // Set flags to false
    MEM_writeBackFlag = false;
    memoryReadFlag = false;
    memoryWriteFlag = false;

    // Massivce switch/case for the OpCodeRegister
    switch (OpCodeRegister){
        case ADD:                   // #####################
            ALU_OUT = ALU0 + ALU1;   

            MEM_writeBackFlag = true;      
            break;
        case ADDI:
            ALU_OUT = ALU0 + IMMEDIATE;

            MEM_writeBackFlag = true;
            break;
        case SUB:
            ALU_OUT = ALU0 - ALU1;

            MEM_writeBackFlag = true;
            break;
        case MUL:
            ALU_OUT = ALU0 * ALU1;

            MEM_writeBackFlag = true;
            break;

        /*case MULO:
            long int tempResult = (long int) ALU0 * (long int) ALU1;    // Not a register, only used to simulate a multiplication w/ overflow
            long int HImask = (long int) (pow(2, 32) - 1) << 32;        // Again, not a register, only used to simulated multiplication w/ overflow

            HI = (int) ( (tempResult & HImask) >> 32);
            LO = (int) tempResult;
            break;*/

        case DIV:
            ALU_OUT = (int) ALU0 / ALU1;

            MEM_writeBackFlag = true;
            break;

        case CMP:
            if      (ALU0 < ALU1) ALU_OUT = -1;
            else if (ALU0 > ALU1) ALU_OUT =  1;
            else if (ALU0 == ALU1)ALU_OUT =  0;
            std::cout << "PAINFUL CMP" << endl;
            MEM_writeBackFlag = true;
            break;
        case LD:
            ALU_OUT = ALU0;
            
            MEM_writeBackFlag = true;
            memoryReadFlag = true;
            break;
        case LDD:
            ALU_OUT = IMMEDIATE;
            
            MEM_writeBackFlag = true;
            memoryReadFlag = true;
            break;
        case LDI:                   // #####################
            ALU_OUT = IMMEDIATE;
            
            MEM_writeBackFlag = true;
            break;
        /*case LID:                   // BROKEN ################################
            registerFile[ALUD] = dataMemory[dataMemory[ALU0]];
            break;*/
        case LDA:
            ALU_OUT = ALU0 + ALU1;

            MEM_writeBackFlag = true;
            memoryReadFlag = true;
            break;
        case STO:
            ALU_OUT = ALU0;
            MEMD = registerFile[ALUD];       // register file access here might be invalid - ask Simon and see what he says

            memoryWriteFlag = true;
            break;
        case STOI:                   // #####################
            ALU_OUT = ALU0;
            MEMD = IMMEDIATE;

            memoryWriteFlag = true;
            break;
        case AND:
            ALU_OUT = ALU0 & ALU1;

            MEM_writeBackFlag = true;
            break;
        case OR:
            ALU_OUT = ALU0 | ALU1;

            MEM_writeBackFlag = true;
            break;
        case NOT:
            ALU_OUT = ~ALU0;

            MEM_writeBackFlag = true;
            break;
        case LSHFT:
            ALU_OUT = ALU0 << ALU1;

            MEM_writeBackFlag = true;
            break;
        case RSHFT:
            ALU_OUT = ALU0 >> ALU1;

            MEM_writeBackFlag = true;
            break;
        case JMP:
            PC = registerFile[ALUD];      // Again as in STO, is accessing the register file at this point illegal?

            branchFlag = true;

            /* STATS */ numOfBranches++;
            cout << "BRANCH" << endl;
            break;
        case JMPI:
            PC = PC + registerFile[ALUD]; // WARNING ERROR HERE

            branchFlag = true;
            
            /* STATS */ numOfBranches++;
            cout << "BRANCH" << endl;
            break;
        case BNE:
            if (ALU0 < 0) {
                PC = registerFile[ALUD];
                
                branchFlag = true;

                /* STATS */ numOfBranches++;
                cout << "BRANCH" << endl;
            }
            break;
        case BPO:
            if (ALU0 > 0) {
                PC = registerFile[ALUD];
                
                branchFlag = true;

                /* STATS */ numOfBranches++;
                cout << "BRANCH" << endl;
            }
            break;
        case BZ:
            if (ALU0 == 0) {
                PC = registerFile[ALUD];
                
                branchFlag = true;

                /* STATS */ numOfBranches++;
                cout << "BRANCH" << endl; 
            }
            break;
        case HALT:                   // #####################
            systemHaltFlag = true;
            break;
        case NOP:
            break;
        case MV:
            ALU_OUT = ALU0;
            
            MEM_writeBackFlag = true;
            break;

        case MVHI:
            ALU_OUT = HI;
            MEMD = ALUD;

            MEM_writeBackFlag = true;
            break;

        case MVLO:
            ALU_OUT = LO;
            MEMD = ALUD;

            MEM_writeBackFlag = true;
            break;
        default:
            std::cout << "Instruction not understood!!" << std::endl;
            break;

    }

    //std::cout << "Executed... ";

    EX_State = Next;
}

// Memory access part of the pipeline: LD and STO operations access the memory here. Branches set the PC here
void memoryAccess(){

    // Prepare State for MA
    if (EX_State != Next) {
        // If IF is not ready to move on, then ID cannot progress (i.e. it is empty)
        MA_State = Empty;

        // Debugging/GUI to show that the current instruction is empty
        MA_inst = string("");

        // increments stall count
        numOfStalls += 1;
        return;
    } else {
        // Else it can run
        MA_State = Current;

        // Debugging/GUI to show the current instr in the processor
        MA_inst = EX_inst;
    }

    // IF BRANCH RETURN
    // Passes the instruction destination register or address along
    WBD = MEMD; 

    // We also need to pass along the writeBackFlag
    writeBackFlag = MEM_writeBackFlag;

    // Check if this instruction needs to access memory
         if (memoryReadFlag == true)  MEM_OUT = dataMemory[ALU_OUT];
    else if (memoryWriteFlag == true) dataMemory[MEMD] = ALU_OUT; 
    else                      MEM_OUT = ALU_OUT;            // Not really needed, just ensures that all instructions have a regular 5-stage pipeline. HERE we could drop it down ot 4 to speed tings up but that might cause some issues with the line.
    
    //std::cout << "Memory Accessed... ";
    MA_State = Next;
}

// Data written back into register file: Write backs don't occur on STO or HALT (or NOP)
void writeBack(){

    // Prepare State for WB
    if (MA_State != Next) {
        // If IF is not ready to move on, then ID cannot progress (i.e. it is empty)
        WB_State = Empty;

        // Debugging/GUI to show that the current instruction is empty
        WB_inst = string("");

        // increments stall count
        numOfStalls += 1;
        return;
    } else {
        // Else it can run
        WB_State = Current;

        // Debugging/GUI to show the current instr in the processor
        WB_inst = MA_inst;
    }

    if (writeBackFlag) registerFile[WBD] = MEM_OUT;
    //std::cout << "Written Back... " << std::endl;
}

#pragma endregion F/D/E/M/W/


#pragma region helperFunctions

// Convert std::string to a Register
Register strToRegister(std::string str){
    //std::cout << "did I fail? " << str << std::endl;
    Register temp = (Register) stoi(str.substr(1, str.length()));
    //std::cout << "did I fail? " << str << std::endl;
    return temp;
}

// Not part of the ISA, loads an I/O program stored in a text file into 
void loadProgramIntoMemory(std::string pathToProgram){
    std::ifstream program(pathToProgram);
    std::string line;
    int counter = 0;

    while (!program.eof()){
        if (counter >= 256){
            throw std::invalid_argument("Program is too large for the memory space. Solution: inscrease memory space or run a smaller program");
            break;
        }
        std::getline(program, line);
        if (( !line.substr(0, line.length() - 1).empty() ) && ( line.substr(0,2).compare("//") != 0) ) {
            instrMemory.at(counter) = line.substr(0, line.length() - 1);
            counter++;
        }
    }
    amount_of_instruction_memory_to_output = counter - 1;
}


// Splits a string by a delminiter and returns it as a std::vector<std::string>
std::vector<std::string> split(std::string str, char deliminator){
        int oldIndex = 0;

        std::vector<std::string> out;

        if (str.length() == 0) return out;

        for (int i = 0; i < str.length(); i++){
                if ((str[i] == deliminator) && (oldIndex != i)){
                        out.push_back(str.substr(oldIndex, i - oldIndex));
                        oldIndex = i + 1;
                }
        }
        std::string subString = str.substr(oldIndex, str.length() - oldIndex);
        out.push_back(subString);
        return out;
}


// Returns true if the syntax was successfully handled
bool handleProgramFlags(int c, char** arguments){
    vector<string> args;
    // Makes the arguments memory safe and easier to handle
    for (int i = 0; i < c; i++){
        args.push_back(string(arguments[i]));
    }
    if (count(args.begin(), args.end(), "-r") == 1 ) PRINT_REGISTERS_FLAG = true;
    if (count(args.begin(), args.end(), "-m") == 1 ) PRINT_MEMORY_FLAG = true;
    if (count(args.begin(), args.end(), "-s") == 1 ) PRINT_STATS_FLAG = true;

    return true;
}

#pragma endregion helperFunctions


int main(int argc, char** argv){
    if (!handleProgramFlags(argc, argv)) {
        std::cout << "Usage: ./isa <program_name> -r|m" << std::endl;
        return 0;
    }

    loadProgramIntoMemory(argv[1]);
    cycle();

    return 0;
}