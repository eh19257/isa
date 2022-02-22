#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <stdexcept>
#include <vector>

/* Constants */
const int SIZE_OF_INSTRUCTION_MEMORY = 256;     // size of the read-only instruction memory
const int SIZE_OF_DATA_MEMORY = 256;            // pretty much the heap and all

/* ISA Function headers */
void fetch();
void decode();  
void execute();

/* Non-ISA function headers */
void loadProgramIntoMemory();
std::vector<std::string> split(std::string str, char deliminator);

/* Debugging function headers*/
void outputAllMemory();
void printRegisterFile();

/* Instructions */
enum Instruction {
    ADD,
    LDI,
    STO,
    HALT
};

#pragma region Registers
/* Registers */
enum Registers {
    R0,
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    R7,
    R8,
    PC,
    CIR
};

/* "Register File" - currently just a bunch of variables */
std::array<int, 16> registers;    // All 16 general purpose registers

int pc;                     // Program Counter
std::string cir;            // Current Instruction Register

#pragma endregion Registers


/* System Flags */
bool systemHaltFlag = false;

/* Memory */
std::array<std::string, SIZE_OF_INSTRUCTION_MEMORY> instrMemory;
std::array<int, SIZE_OF_DATA_MEMORY> dataMemory;

#pragma region debugging

void outputAllMemory(){
    std::string emptyLine = "--------------------------------";     // 32 '-'s to show an empty line

    std::cout << "\tInstruction Memory" << "              \t\t\t" << "Data Memory\n" << std::endl;
    for (int i = 0; i < SIZE_OF_INSTRUCTION_MEMORY || i < SIZE_OF_DATA_MEMORY; i++){
        std::cout << i << "\t";
        if (i < instrMemory.size()){
            if (instrMemory.at(i).empty()){
                std::cout << emptyLine;
            } else {
                std::cout << instrMemory.at(i);
            }
        }
        /*
        std::cout << "\t\t\t";
        if (i < dataMemory.size()){
            if (dataMemory.at(i)){
                std::cout << emptyLine;
            } else {
                std::cout << dataMemory.at(i);
            }
        }
        */
        std::cout << std::endl;
    }
}

void printRegisterFile(){

    //std::cout <<
}

#pragma endregion debugging

// The main cycle of the processor
void cycle(){
    //registers[R0] = 123;
    while (!systemHaltFlag) {
        fetch();
        decode();
        execute();
        //std::cout << "Value of general register 0: " << reg.at(ADD) << std::endl;
    }
}

#pragma region F/D/E/

// Fetches the next instruction that is to be ran, this instruction is fetched by taking the PCs index 
void fetch(){
    // Load the memory address that is in the instruction memory address that is pointed to by the PC
    cir = instrMemory.at(pc);

    /* We DO NOT UPDATE the PC here but instead we do it in the DECODE stage as this will help with pipelining branches later on */
    // Instead of incrementing the PC here we could use a NPC which is used by MIPS and stores the next sequential PC
    // Increment PC
    //pc++;

    std::cout << "Fetching instruction: " << cir << std::endl;
}


// Takes current instruction that is being used and decodes it so that it can be understood by the computer (not a massively important part)
// Updates PC
void decode(){
    std::cout << "Decoding instruction" << std::endl;
    std::vector<std::string> splitCIR = split(cir, ' '); // split the instruction based on ' ' and decode instruction like that
    
    // Throws error if there isn't any instruction to be loaded
    if (splitCIR.size() == 0) throw std::invalid_argument("No instruction loaded");

    // if statement for decoding all instructions
    if (splitCIR.at(0).compare("ADD")) {
        
    }
}


// Executes the current instruction
void execute(){
    std::cout << "Executing instruction" << std::endl;

    std::string instr = cir.substr(0, 4);

    /*
    // switch case wont work unless you convert them into enums - this is because c++ can't compare std::strings
    switch(instr){
        case std::string("HALT"){

        }
    }
    */
}
#pragma endregion F/D/E/


#pragma region helperFunctions
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
        instrMemory.at(counter) = line;
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
    outputAllMemory();
    /*
    pc = 0;
    cycle();
    */
   cycle();
}




/* TO DO

To do:
-	Find a way to properly decode the instructions
-   Properly instantiate register file and instruction memory.
*/