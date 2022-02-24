#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <stdexcept>
#include <vector>
#include <iomanip>

/* Constants */
const int SIZE_OF_INSTRUCTION_MEMORY = 256;     // size of the read-only instruction memory
const int SIZE_OF_DATA_MEMORY = 256;            // pretty much the heap and all

/* Registers */
#pragma region Registers
enum Register { R0, R1, R2, R3, R4, R5, R6, R7, R8, R9, R10, R11, R12, R13, R14, R15 };

/* "Register File" - currently just a bunch of variables */
std::array<int, 16> registerFile;    // All 16 general purpose registers

int PC;                     // Program Counter
std::string CIR;            // Current Instruction Register
int IMMEDIATE;              // Immediate register used for immediate addressing
    
// ALU registers
Instruction opCodeRegister = NOP;       // Stores the decoded OpCode that was in the CIR
int  ALU0, ALU1, ALU_OUT;               // 2 input regsiters for the ALU
Register ALUD;                          // Destination register for the output of the ALU

// Memory Related Registers
Register MEMD;                          // Destination address for the position in memory (STO operation) or the register in the register file (LD operation)

#pragma endregion Registers

/* Instructions */
enum Instruction {
    ADD,
    ADDI,
    SUB,
    MUL,
    DIV,
    CMP,

    LD,
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
    NOP
};

/* System Flags */
bool systemHaltFlag = false;


/* Memory */
std::array<std::string, SIZE_OF_INSTRUCTION_MEMORY> instrMemory;
std::array<int, SIZE_OF_DATA_MEMORY> dataMemory;


/* ISA Function headers */
void fetch();
void decode();  
void execute();
void memoryAccess();
void writeBack();

/* Non-ISA function headers */
void loadProgramIntoMemory();
std::vector<std::string> split(std::string str, char deliminator);
Register strToRegister(std::string str);

/* Debugging function headers*/
void outputAllMemory(int cutOff);
void printRegisterFile(int maxReg);


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
    std::cout << "ALUD: " << ALUD << std::endl;
}

#pragma endregion debugging

#pragma region F/D/E/M/W/

// The main cycle of the processor
void cycle(){
    int numOfCycles = 1;
    //registers[R0] = 123;
    while (!systemHaltFlag) {
        std::cout << "---------- Cycle " << numOfCycles << " starting ----------"<< std::endl;
        std::cout << "PC has current value: " << PC << std::endl;

        fetch();
        decode();
        execute();
        memoryAccess();
        writeBack();

        printRegisterFile(5);

        std::cout << "---------- Cycle " << numOfCycles << " completed. ----------\n"<< std::endl;
        numOfCycles++;
    }
}


// Fetches the next instruction that is to be ran, this instruction is fetched by taking the PCs index 
void fetch(){
    // Load the memory address that is in the instruction memory address that is pointed to by the PC
    CIR = instrMemory.at(PC);

    /* We DO NOT UPDATE the PC here but instead we do it in the DECODE stage as this will help with pipelining branches later on */
    // Instead of incrementing the PC here we could use a NPC which is used by MIPS and stores the next sequential PC
    // Increment PC
    //pc++;

    std::cout << "CIR has current value: " << CIR << std::endl;
    std::cout << "Fetched... ";
}


// Takes current instruction that is being used and decodes it so that it can be understood by the computer (not a massively important part)
// Updates PC
void decode(){
    std::vector<std::string> splitCIR = split(CIR, ' '); // split the instruction based on ' ' and decode instruction like that
    
    // Throws error if there isn't any instruction to be loaded
    if (splitCIR.size() == 0) throw std::invalid_argument("No instruction loaded");
    
    // Load the register values into the ALU's input
    if (splitCIR.size() > 1) {
        // Get set first/destination register
        if (splitCIR.at(1).substr(0,1).compare("r") == 0 ) ALUD = strToRegister(splitCIR.at(1));

        if (splitCIR.size() > 2) {
            if (splitCIR.at(2).substr(0,1).compare("r") == 0 ) ALU0 = registerFile.at(strToRegister(splitCIR.at(2)));
            
            if (splitCIR.size() > 3) {
                if (splitCIR.at(3).substr(0,1).compare("r") == 0 ) ALU1 = registerFile.at(strToRegister(splitCIR.at(3)));

            }
        }
    }
    // if statement for decoding all instructions
         if (splitCIR.at(0).compare("ADD")  == 0) opCodeRegister = ADD;
    else if (splitCIR.at(0).compare("ADDI") == 0) { opCodeRegister = ADDI; IMMEDIATE = stoi(splitCIR.at(3)); }
    else if (splitCIR.at(0).compare("SUB")  == 0) opCodeRegister = SUB;
    else if (splitCIR.at(0).compare("MUL")  == 0) opCodeRegister = MUL;
    else if (splitCIR.at(0).compare("DIV")  == 0) opCodeRegister = DIV;
    else if (splitCIR.at(0).compare("CMP")  == 0) opCodeRegister = CMP;

    else if (splitCIR.at(0).compare("LD")   == 0) opCodeRegister = LD;
    else if (splitCIR.at(0).compare("LDI")  == 0) { opCodeRegister = LDI; IMMEDIATE = stoi(splitCIR.at(2)); }
    else if (splitCIR.at(0).compare("LID")  == 0) opCodeRegister = LID;
    else if (splitCIR.at(0).compare("LDA")  == 0) opCodeRegister = LDA;
    
    else if (splitCIR.at(0).compare("STO")  == 0) opCodeRegister = STO;
    else if (splitCIR.at(0).compare("STOI") == 0) { opCodeRegister = STOI; IMMEDIATE = stoi(splitCIR.at(1)); }

    else if (splitCIR.at(0).compare("AND")  == 0) opCodeRegister = AND;
    else if (splitCIR.at(0).compare("OR")   == 0) opCodeRegister = OR;
    else if (splitCIR.at(0).compare("NOT")  == 0) opCodeRegister = NOT;
    else if (splitCIR.at(0).compare("LSHFT")== 0) opCodeRegister = LSHFT;
    else if (splitCIR.at(0).compare("RSHFT")== 0) opCodeRegister = RSHFT;

    else if (splitCIR.at(0).compare("JMP")  == 0) opCodeRegister = JMP;
    else if (splitCIR.at(0).compare("JMPI") == 0) opCodeRegister = JMPI;
    else if (splitCIR.at(0).compare("BNE")  == 0) opCodeRegister = BNE;
    else if (splitCIR.at(0).compare("BPO")  == 0) opCodeRegister = BPO;
    else if (splitCIR.at(0).compare("BZ")   == 0) opCodeRegister = BZ;

    else if (splitCIR.at(0).compare("HALT") == 0) opCodeRegister = HALT;
    else if (splitCIR.at(0).compare("NOP")  == 0) opCodeRegister = NOP;
    else throw std::invalid_argument("Unidentified Instruction: " + splitCIR.at(0));

    // Increment PC
    PC++;

    std::cout << "Decoded... ";
}


// Executes the current instruction
void execute(){ 
    switch (opCodeRegister){
        case ADD:
            registerFile[ALUD] = ALU0 + ALU1;
            break;
        case ADDI:
            registerFile[ALUD] = ALU0 + IMMEDIATE;
            break;
        case SUB:
            registerFile[ALUD] = ALU0 - ALU1;
            break;
        case MUL:
            registerFile[ALUD] = ALU0 * ALU1;
            break;
        case DIV:
            registerFile[ALUD] = (int) ALU0 / ALU1;
            break;
        case CMP:
            if      (ALU0 < ALU1) registerFile[ALUD] = -1;
            else if (ALU0 > ALU1) registerFile[ALUD] =  1;
            else                  registerFile[ALUD] =  0;
            break;
        case LD:
            registerFile[ALUD] = dataMemory[ALU0];
        case LDI:
            registerFile[ALUD] = IMMEDIATE;
            break;
        case LID:
            registerFile[ALUD] = dataMemory[dataMemory[ALU0]];
            break;
        case LDA:
            registerFile[ALUD] = dataMemory[ALU0 + ALU1];
            break;
        case STO:
            dataMemory[ALUD] = ALU0;
            break;
        case STOI:
            dataMemory[IMMEDIATE] = ALU0;
            break;
        case AND:
            registerFile[ALUD] = ALU0 & ALU1;
            break;
        case OR:
            registerFile[ALUD] = ALU0 | ALU1;
            break;
        case NOT:
            registerFile[ALUD] = ~ALU0;
            break;
        case LSHFT:
            registerFile[ALUD] = ALU0 << ALU1;
            break;
        case RSHFT:
            registerFile[ALUD] = ALU0 >> ALU1;
            break;
        case JMP:
            break;
        case JMPI:
            break;
        case BNE:
            break;
        case BPO:
            break;
        case BZ:
            break;
        case HALT:
            systemHaltFlag = true;
            break;
        case NOP:
            break;
        default:
            std::cout << "Instruction not understood!!" << std::endl;
            break;

    }

    std::cout << "Executed... ";
}

// Memory access part of the pipeline: LD and STO operations access the memory here. Branches set the PC here
void memoryAccess(){
    
     std::cout << "Memory Accessed... ";
}

// Data written back into register file
void writeBack(){

     std::cout << "Written Back... " << std::endl;
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
        instrMemory.at(counter) = line.substr(0, line.length() - 1);
        counter++;
    }
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

#pragma endregion helperFunctions



int main(){
    loadProgramIntoMemory("program");
    outputAllMemory(8);
    cycle();
    std::cout << "Program has been halted\n" << std::endl;
    outputAllMemory(8);
}