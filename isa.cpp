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
int  ALU0, ALU1; //ALU_OUT
Register ALU_OutPointer;

#pragma endregion Registers

/* Instructions */
enum Instruction {
    ADD,
    LDI,
    STO,
    HALT,
    NOP
};

/* System Flags */
bool systemHaltFlag = false;
Instruction operationTypeFlag = NOP; 

/* Memory */
std::array<std::string, SIZE_OF_INSTRUCTION_MEMORY> instrMemory;
std::array<int, SIZE_OF_DATA_MEMORY> dataMemory;


/* ISA Function headers */
void fetch();
void decode();  
void execute();

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
    std::cout << "ALU_OutPointer: " << ALU_OutPointer << std::endl;
}

#pragma endregion debugging

#pragma region F/D/E/

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
        if (splitCIR.at(1).substr(0,1).compare("r") == 0 ) ALU_OutPointer = strToRegister(splitCIR.at(1));

        if (splitCIR.size() > 2) {
            if (splitCIR.at(2).substr(0,1).compare("r") == 0 ) ALU0 = registerFile.at(strToRegister(splitCIR.at(2)));
            
            if (splitCIR.size() > 3) {
                if (splitCIR.at(3).substr(0,1).compare("r") == 0 ) ALU1 = registerFile.at(strToRegister(splitCIR.at(3)));

            }
        }
    }
    // if statement for decoding all instructions
         if (splitCIR.at(0).compare("ADD") == 0) operationTypeFlag = ADD;
    else if (splitCIR.at(0).compare("LDI") == 0) { operationTypeFlag = LDI; IMMEDIATE = stoi(splitCIR.at(2)); }
    else if (splitCIR.at(0).compare("STO") == 0) { operationTypeFlag = STO; IMMEDIATE = stoi(splitCIR.at(1)); }
    else if (splitCIR.at(0).compare("HALT") == 0) operationTypeFlag = HALT;
    else if (splitCIR.at(0).compare("NOP") == 0) operationTypeFlag = NOP;

    // Increment PC
    PC++;

    std::cout << "Decoded... ";
}


// Executes the current instruction
void execute(){ 
    switch (operationTypeFlag){
        case ADD:
            registerFile[ALU_OutPointer] = ALU0 + ALU1;
            break;
        case LDI:
            registerFile[ALU_OutPointer] = IMMEDIATE;
            break;
        case STO:
            dataMemory[IMMEDIATE] = ALU0;
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

    std::cout << "Executed... " << std::endl;
}
#pragma endregion F/D/E/


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
    std::cout << "Program has been halted" << std::endl;
    outputAllMemory(8);
}




/* TO DO

To do:
-	Find a way to properly decode the instructions
-   Properly instantiate register file and instruction memory.
*/