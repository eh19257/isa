#include <iostream>
#include <fstream>
#include <array>
#include <string>
#include <stdexcept>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <thread>
#include <queue>
//#include <bits/stdc++.h>

//#include "EnumsAndConstants.hpp"
#include "ExecutionUnits.hpp"

using namespace std;


/* Debugging Flags */
bool PRINT_REGISTERS_FLAG = false;
bool PRINT_MEMORY_FLAG = false;
bool PRINT_STATS_FLAG = false;


int amount_of_instruction_memory_to_output = 8;  // default = 8

/* Registers */
#pragma region Registers

/* "Register File" - currently just a bunch of variables */
std::array<int, 16> registerFile;    // All 16 general purpose registers
std::array<float, 4> floatingPointRegisterFile;

int PC;                     // Program Counter

// IF/ID registers
std::string CIR;            // Current Instruction Register
int IMMEDIATE;              // Immediate register used for immediate addressing

NonDecodedInstruction IF_ID_Inst;   // WARNING THIS IS A NON decoded instruction that is ONLY USED ONCE (here in IF)


// ID/I registers
Instruction OpCodeRegister = NOP;       // Stores the decoded OpCode that was in the CIR
int  ALU0, ALU1, ALU_OUT;               // 2 input regsiters for the ALU

DecodedInstruction ID_I_Inst; 

//float ALU_FP0, ALU_FP1;                 // 2 input registers for the ALU where FP calculations are occuring
int HI, LO;                             // High and Low parts of integer multiplication
int ALUD;                               // Destination register for the output of the ALU

// I/EX registers
int ID;

DecodedInstruction I_EX_Inst;

// EX/WB
DecodedInstruction EX_WB_Inst;

// EX/C registers

// C/WB registers
int C_OUT;

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

/* Execution Units*/
//std::array<ExecutionUnit, 4> EUs = {ALU(), ALU(), BU(), LSU()};
std::array<ALU*, 2> ALUs = {new ALU(), new ALU()};
std::array<BU*, 1> BUs = {new BU()};
std::array<LSU*, 1> LSUs = {new LSU(&dataMemory)};

/* Reservation Stations */
std::vector<DecodedInstruction> ALU_RV;
std::vector<DecodedInstruction> BU_RV;
std::vector<DecodedInstruction> LSU_RV;

/* ISA Function headers */
void fetch();
void decode();  
void issue();
void execute();
void complete();
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
string IF_inst = "";
string ID_inst = "";
string I_inst = "";
string EX_inst = "";
//string C_inst = "";
//string MA_inst = "";
string WB_inst = "";


/* Stats variables */
int numOfCycles = 1;        // Counts the number of cycles (stats at cycle 1 not cycle 0)
int numOfBranches = 0;
int numOfStalls = 0;        // Counts the number of times the pipeline stalls
int numOfBubbles = 0;

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

void flushPipeline(){
    IF_ID_Inst.state = EMPTY;
    IF_inst = "";

    ID_I_Inst.state = EMPTY;
    ID_inst = "";

    I_EX_Inst.state = EMPTY;
    I_inst = "";

    EX_inst = "";

    ALUs.at(0)->Inst = "";
    ALUs.at(1)->Inst = "";
    BUs.at(0)->Inst = "";
    LSUs.at(0)->Inst = "";

    // Flush all EUs as well
    ALUs.at(0)->In.state = EMPTY;
    ALUs.at(1)->In.state = EMPTY;
    BUs.at(0)->In.state = EMPTY;
    LSUs.at(0)->In.state = EMPTY;

    // Clean RVs as well
    ALU_RV.clear();
    BU_RV.clear();
    LSU_RV.clear();
}


// A single cycle of the processor
void cycle(){
    //if (numOfCycles == 26) outputAllMemory(amount_of_instruction_memory_to_output);
    std::cout << "---------- Cycle " << numOfCycles << " starting ----------"<< std::endl;
    //std::cout << "PC has current value: " << PC << std::endl;


    // Non-pipelined 
    //fetch(); decode(); issue(); execute(); complete(); writeBack();

    // Pipelined
    writeBack(); /*complete();*/ execute(); issue(); decode(); fetch();

    cout << "\nCurrent instruction in the IF: " << IF_inst << endl;
    cout << "Current instruction in the ID: " << ID_inst << endl;
    cout << "Current instruction in the I:  " << I_inst << endl;
    cout << "Current instruction in ALU0: " << ALUs.at(0)->Inst << "\tALU1: " << ALUs.at(1)->Inst << "\tBU: " << BUs.at(0)->Inst << "\tLSU: " << LSUs.at(0)->Inst << endl;
    //cout << "Current instruciton in the C:  " << C_inst << endl;
    cout << "Current instruction in the WB: " << WB_inst << endl;
            

    cout << "\n\n" << endl;
    if (PRINT_REGISTERS_FLAG) printRegisterFile(16);
    if (PRINT_MEMORY_FLAG) outputAllMemory(amount_of_instruction_memory_to_output);

    std::cout << "---------- Cycle " << numOfCycles << " completed. ----------\n"<< std::endl;
    numOfCycles++;
}


// Running the processor
void run(){
    // Print memory before running the program
    if (PRINT_MEMORY_FLAG) outputAllMemory(amount_of_instruction_memory_to_output);

    while (!systemHaltFlag) {
        cycle();        
    }
    // This last cycle() is to finish the final execution of the
    cycle();

    std::cout << "Program has been halted\n" << std::endl;

    // Print the memory after the program has been ran
    //if (PRINT_MEMORY_FLAG) outputAllMemory(amount_of_instruction_memory_to_output);
    if (PRINT_STATS_FLAG) outputStatistics(numOfCycles);
}


// Fetches the next instruction that is to be ran, this instruction is fetched by taking the PCs index 
void fetch(){

    if (IF_ID_Inst.state == BLOCK || IF_ID_Inst.state == CURRENT){
        // We cannot collect any instructions from memory and pass it into IF_ID_Inst
        return;
    }

    IF_ID_Inst.state = CURRENT;
    
    // Load the memory address that is in the instruction memory address that is pointed to by the PC
    IF_ID_Inst.instruction = instrMemory.at(PC);

    PC++;

    // Debugging/GUI to show the current instr in the processor
    IF_inst = IF_ID_Inst.instruction;

    if (IF_ID_Inst.instruction.empty()) {
        IF_ID_Inst.state = EMPTY;
        return;
    } else {
        // If it is not empty then we have successfully fetched the instruction
        IF_ID_Inst.state = NEXT;

        std::cout << "CIR has current value: " << IF_ID_Inst.instruction << std::endl;
    }

    // INCORRECT \/\/
    /* We DO NOT UPDATE the PC here but instead we do it in the DECODE stage as this will help with pipelining branches later on */
    // Instead of incrementing the PC here we could use a NPC which is used by MIPS and stores the next sequential PC
    // Increment PC
    //pc++;    
}


// Takes current instruction that is being used and decodes it so that it can be understood by the computer (not a massively important part)
// Updates PC
void decode(){
    #pragma region State Setup
    // State managing for IF_ID_Inst and ID_I_Inst
    
    if (ID_I_Inst.state == BLOCK || ID_I_Inst.state == CURRENT){
        // If this happens then we cannot run the decode as there are no places in which the result can be stored
        IF_ID_Inst.state = BLOCK;

        numOfStalls += 1;
        return;
    } 
    else if (ID_I_Inst.state == EMPTY){// || ID_I_Inst.state == EMPTY){
        // If this happens then decoded is able to be ran aslong as IF_ID_Inst contains an instruction

        if (IF_ID_Inst.state == NEXT){
            
            IF_ID_Inst.state = CURRENT;
            
            // Debugging/GUI
            ID_inst = IF_inst;
        }
        else{
            // Decode() cannot be run with no inputs and so we return

            ID_inst = "";
            return;
        }        
    } else {
        cout << "ID_I_Inst has gone mad - illegal state as it's NEXT" << endl;
    }
    #pragma endregion State Setup

    std::vector<std::string> splitCIR = split(IF_ID_Inst.instruction, ' '); // split the instruction based on ' ' and decode instruction like that
    
    // Throws error if there isn't any instruction to be loaded
    if (splitCIR.size() == 0) throw std::invalid_argument("No instruction loaded");
    
    // Load the register values into the ALU's input
    if (splitCIR.size() > 1) {
        // Get set first/destination register
        if (splitCIR.at(1).substr(0 ,1).compare("r")  == 0 ) {
            ID_I_Inst.rd = strToRegister(splitCIR.at(1)); 

            ID_I_Inst.DEST = ID_I_Inst.rd;
            //ALUD = ID_I_Inst.rd;
        }
        //else if (splitCIR.at(1).substr(0, 2).compare("FP") == 0)  ALUD = (FP_Register) stoi(splitCIR.at(1).substr(1, splitCIR.at(1).length()));

        if (splitCIR.size() > 2) {
            if      (splitCIR.at(2).substr(0, 1).compare("r")  == 0 ) {
                ID_I_Inst.rs0 = strToRegister(splitCIR.at(2));

                ID_I_Inst.IN0 = registerFile.at(ID_I_Inst.rs0);
                //ALU0 = registerFile.at(ID_I_Inst.rs0);
            }
            //else if (splitCIR.at(2).substr(0, 2).compare("FP") == 0)  ALU_FP0 = (FP_Register) stoi(splitCIR.at(2).substr(1, splitCIR.at(2).length()));
            
            if (splitCIR.size() > 3) {
                if      (splitCIR.at(3).substr(0,1).compare("r")  == 0 ){
                    ID_I_Inst.rs1 = strToRegister(splitCIR.at(3));

                    ID_I_Inst.IN1 = registerFile.at(ID_I_Inst.rs1);
                    //ALU1 = registerFile.at(ID_I_Inst.rs1);
                }
                //else if (splitCIR.at(3).substr(0,1).compare("FP") == 0 ) ALU_FP1 = (FP_Register) stoi(splitCIR.at(3).substr(1, splitCIR.at(3).length()));

            }
        }
    }

    // if statement for decoding all instructions
         if (splitCIR.at(0).compare("ADD")  == 0) ID_I_Inst.OpCode = ADD;
    else if (splitCIR.at(0).compare("ADDI") == 0) { ID_I_Inst.OpCode = ADDI; ID_I_Inst.IMM = stoi(splitCIR.at(3)); }
    else if (splitCIR.at(0).compare("ADDF") == 0) ID_I_Inst.OpCode = ADDF;
    else if (splitCIR.at(0).compare("SUB")  == 0) ID_I_Inst.OpCode = SUB;
    else if (splitCIR.at(0).compare("SUBF") == 0) ID_I_Inst.OpCode = SUBF;
    else if (splitCIR.at(0).compare("MUL")  == 0) ID_I_Inst.OpCode = MUL;
    else if (splitCIR.at(0).compare("MULO") == 0) ID_I_Inst.OpCode = MULO;
    else if (splitCIR.at(0).compare("MULFO")== 0) ID_I_Inst.OpCode = MULFO;
    else if (splitCIR.at(0).compare("DIV")  == 0) ID_I_Inst.OpCode = DIV;
    else if (splitCIR.at(0).compare("DIVF") == 0) ID_I_Inst.OpCode = DIVF;
    else if (splitCIR.at(0).compare("CMP")  == 0) ID_I_Inst.OpCode = CMP;

    else if (splitCIR.at(0).compare("LD")   == 0) ID_I_Inst.OpCode = LD;
    else if (splitCIR.at(0).compare("LDD")  == 0) { ID_I_Inst.OpCode = LDD; ID_I_Inst.IMM = stoi(splitCIR.at(2)); } 
    else if (splitCIR.at(0).compare("LDI")  == 0) { ID_I_Inst.OpCode = LDI; ID_I_Inst.IMM = stoi(splitCIR.at(2)); }
    else if (splitCIR.at(0).compare("LID")  == 0) ID_I_Inst.OpCode = LID;
    else if (splitCIR.at(0).compare("LDA")  == 0) ID_I_Inst.OpCode = LDA;
    
    else if (splitCIR.at(0).compare("STO")  == 0) { ID_I_Inst.OpCode = STO; ID_I_Inst.DEST = registerFile.at(strToRegister(splitCIR.at(1)));}
    else if (splitCIR.at(0).compare("STOI") == 0) { ID_I_Inst.OpCode = STOI; ID_I_Inst.IMM = stoi(splitCIR.at(1)); }

    else if (splitCIR.at(0).compare("AND")  == 0) ID_I_Inst.OpCode = AND;
    else if (splitCIR.at(0).compare("OR")   == 0) ID_I_Inst.OpCode = OR;
    else if (splitCIR.at(0).compare("NOT")  == 0) ID_I_Inst.OpCode = NOT;
    else if (splitCIR.at(0).compare("LSHFT")== 0) ID_I_Inst.OpCode = LSHFT;       // IMMEDIATE = stoi(splitCIR.at(3)); }
    else if (splitCIR.at(0).compare("RSHFT")== 0) ID_I_Inst.OpCode = RSHFT;       // IMMEDIATE = stoi(splitCIR.at(3)); }

    else if (splitCIR.at(0).compare("JMP")  == 0) { ID_I_Inst.OpCode = JMP; ID_I_Inst.DEST = registerFile.at(strToRegister(splitCIR.at(1))); }
    else if (splitCIR.at(0).compare("JMPI") == 0) {ID_I_Inst.OpCode = JMPI; ID_I_Inst.DEST = registerFile.at(strToRegister(splitCIR.at(1))); }
    else if (splitCIR.at(0).compare("BNE")  == 0) {ID_I_Inst.OpCode = BNE; ID_I_Inst.DEST = registerFile.at(strToRegister(splitCIR.at(1))); }
    else if (splitCIR.at(0).compare("BPO")  == 0) {ID_I_Inst.OpCode = BPO; ID_I_Inst.DEST = registerFile.at(strToRegister(splitCIR.at(1))); }
    else if (splitCIR.at(0).compare("BZ")   == 0) {ID_I_Inst.OpCode = BZ; ID_I_Inst.DEST = registerFile.at(strToRegister(splitCIR.at(1))); }

    else if (splitCIR.at(0).compare("HALT") == 0) ID_I_Inst.OpCode = HALT;
    else if (splitCIR.at(0).compare("NOP")  == 0) ID_I_Inst.OpCode = NOP;
    else if (splitCIR.at(0).compare("MV")   == 0) ID_I_Inst.OpCode = MV;
    else if (splitCIR.at(0).compare("MVHI") == 0) ID_I_Inst.OpCode = MVHI;
    else if (splitCIR.at(0).compare("MVLO") == 0) ID_I_Inst.OpCode = MVLO;

    else throw std::invalid_argument("Unidentified Instruction: " + splitCIR.at(0));


    // Crush instruction into a DecodedInstruction dataType
    //ID_I_Inst.OpCode = OpCodeRegister;
    //ID_I_Inst.DEST = ALUD;
    //ID_I_Inst.IN0 = ALU0;
    //ID_I_Inst.IN1 = ALU1;
    //ID_I_Inst.IMM = IMMEDIATE;

    // SUPER IMPORTANT - THIS IS THE PREVIOUS VALUE OF THE PC TO USE THE CORRECT OLD VALUE OF IT AND NOT THE NEW VALUE
    ID_I_Inst.OUT = IF_ID_Inst.PC;     // It is only set to the PC for the single instruction JMPI (indexed JMP)

    // IF_ID has now been fully used by the ID stage and the output of decode() is now being held in ID_I which makes ready for the NEXT stage and IF_ID has now been used up making it EMPTY
    IF_ID_Inst.state = EMPTY;
    ID_I_Inst.state = NEXT;

    // Debuggin/GUI
    ID_I_Inst.asString = IF_ID_Inst.instruction;

    
    std::cout << "Instruction " << ID_I_Inst.asString << " in ID decoded: "; ID_I_Inst.print();
}


// Issues the current instruction to it's repsective EU
void issue(){
    #pragma region State Setup
    // State managing for ID_I and I_RVs
    // This is different as we need to check if 

    if (ID_I_Inst.state == CURRENT) {
        // Note: here it shouldnt be possible for ID_I_Inst to have a current state but we deal with it like this anyway
    
    } else if (ID_I_Inst.state == EMPTY) {
        cout << "I is empty" << endl;

        // Debugging/GUI
        I_inst = "";
    }
    else if (ID_I_Inst.state == NEXT){//|| ID_I_Inst.state == BLOCK){

        I_inst = ID_inst;
        
        // ALU
        if ( (ID_I_Inst.OpCode >= ADD && ID_I_Inst.OpCode <= CMP) || ID_I_Inst.OpCode == MV || (ID_I_Inst.OpCode >= AND && ID_I_Inst.OpCode <= RSHFT) ){

            // Check if there is any space in the RVs
            if (ALU_RV.size() >= MAX_RV_SIZE){
                // the ALU's RV is full and so we have to set the ID_I_Inst to BLOCKING
                ID_I_Inst.state = BLOCK;
            } else {
                // HERE THERE IS SPACE IN THE ALU_RV AND SO WE CAN ISSUE AN INSTRUCTION TO IT
                ID_I_Inst.state = NEXT;

                ALU_RV.push_back(ID_I_Inst);

                ID_I_Inst.state = EMPTY;

                cout << "Loaded instruction " << ID_I_Inst.asString << " into ALU_RV. Decoded instruction: "; ID_I_Inst.print();
            }            
        }
        // BU
        else if (ID_I_Inst.OpCode >= JMP && ID_I_Inst.OpCode <= BZ || ID_I_Inst.OpCode == HALT || ID_I_Inst.OpCode == NOP) {
            
            // Check if there is any space in the RVs
            if (BU_RV.size() >= MAX_RV_SIZE){
                // the ALU's RV is full and so we have to set the ID_I_Inst to BLOCKING
                ID_I_Inst.state = BLOCK;
            } else {
                // HERE THERE IS SPACE IN THE ALU_RV AND SO WE CAN ISSUE AN INSTRUCTION TO IT
                ID_I_Inst.state = NEXT;

                BU_RV.push_back(ID_I_Inst);

                ID_I_Inst.state = EMPTY;
                
                cout << "Loaded instruction " << ID_I_Inst.asString << " into BU_RV. Decoded instruction: "; ID_I_Inst.print();
            }       
        }

        // LSU
        else if (ID_I_Inst.OpCode >= LD && ID_I_Inst.OpCode <= STOI) {

             // Check if there is any space in the RVs
            if (LSU_RV.size() >= MAX_RV_SIZE){
                // the ALU's RV is full and so we have to set the ID_I_Inst to BLOCKING
                ID_I_Inst.state = BLOCK;
            } else {
                // HERE THERE IS SPACE IN THE ALU_RV AND SO WE CAN ISSUE AN INSTRUCTION TO IT
                ID_I_Inst.state = NEXT;

                LSU_RV.push_back(ID_I_Inst);

                ID_I_Inst.state = EMPTY;

                cout << "Loaded instruction " << ID_I_Inst.asString << " into LSU_RV. Decoded instruction: "; ID_I_Inst.print();
            }
        }  
        else {
            throw std::invalid_argument("Could no issue an unknown instruction: " + ID_I_Inst.OpCode);
        }
    } else {
        // BLOCKING
        cout << "INSTRUCTION IS CURRENTLY BLOCKING IN ID/I!!!" << endl;
    }


    //////////
    // If the EUs are free, then we can off load the RVs into the EUs
    for (ALU* a : ALUs){
        if (a->In.state == BLOCK || a->In.state == CURRENT){
            // If the In-instruction for this EU is BLOCKING or CURRENTLY running then we do not d anything
            
            continue;

        } else if (a->In.state == EMPTY ){//|| a->In.state == NEXT){
            
            // If the ALU is not EMPTY then we can check if the last instruction in there is ready to be NEXT 
            if (!ALU_RV.empty()){
                if (ALU_RV.back().state == NEXT){
                    // If the instruction in the RV is ready to be NEXT, then it is loaded into the ALU

                    // LOAD INTO ALU
                    a->In = ALU_RV.back();
                    ALU_RV.pop_back();

                    cout << "Instruction " << a->In.asString << " loaded into the ALU" << (int) (std::find(ALUs.begin(), ALUs.end(), a) - ALUs.begin()) << ". Decoded instruction: "; a->In.print();
                }
            }
        } else {
            cout << "Instruction in the ALU has gone mad!!! - INSTRUCTION HAS STATE NEXT!!!" << endl;
        }
    }

    // If the BUs are free, then we can off load the RVs into the BUs
    for (BU* b : BUs){
        if (b->In.state == BLOCK || b->In.state == CURRENT){
            // If the In-instruction for this EU is BLOCKING or CURRENTLY running then we do not d anything
            
            continue;

        } else if (b->In.state == EMPTY){// || b->In.state == NEXT){
            
            // If the BU is not EMPTY then we can check if the last instruction in there is ready to be NEXT 
            if (!BU_RV.empty()){
                if (BU_RV.back().state == NEXT){
                    
                    // If the instruction in the RV is ready to be NEXT, then it is loaded into the BU

                    // LOAD INTO BU
                    b->In = BU_RV.back();
                    BU_RV.pop_back();
                }
            }
        } else {
            cout << "Instruction in the BU has gone mad!!! - INSTRUCTION HAS STATE NEXT!!!" << endl;
        }
    }

    // If the LSUs are free, then we can off load the RVs into the LSUs
    for (LSU* l : LSUs){
        if (l->In.state == BLOCK || l->In.state == CURRENT){
            // If the In-instruction for this EU is BLOCKING or CURRENTLY running then we do not d anything
            
            continue;

        } else if (l->In.state == EMPTY){//|| l->In.state == NEXT){
            
            // If the LSU is not EMPTY then we can check if the last instruction in there is ready to be NEXT 
            if (!LSU_RV.empty()){
                if (LSU_RV.back().state == NEXT){

                    // If the instruction in the RV is ready to be NEXT, then it is loaded into the LSU
                    
                    // LOAD INTO LSU
                    l->In = LSU_RV.back();
                    LSU_RV.pop_back();
                }
            }
        } else {
            cout << "Instruction in the LSU has gone mad!!! - INSTRUCTION HAS STATE NEXT!!!" << endl;
        }
    }

    ID_I_Inst.state = EMPTY;
    //I_EX_Inst.state = NEXT;
}


// Executes the current instruction
void execute(){
    // Run all EUs
    for (ALU* a : ALUs) {
        if ( (a->Out.state == EMPTY || a->Out.state == NEXT) && (a->In.state == NEXT || a->In.state == CURRENT) ) {
            a->Inst = I_inst;
            a->cycle();
        }
        else a->Inst = "";
    }
    for (BU*  b : BUs ) {
        if ( (b->Out.state == EMPTY || b->Out.state == NEXT) && (b->In.state == NEXT || b->In.state == CURRENT) ) {
            b->Inst = I_inst;
            b->cycle();
        }
        else b->Inst = "";
    }
    for (LSU* l : LSUs) {
        if ( (l->Out.state == EMPTY || l->Out.state == NEXT) && (l->In.state == NEXT || l->In.state == CURRENT) ) {
            l->Inst = I_inst;
            l->cycle();
        }
        else l->Inst = "";
    } 
}

// Data written back into register file: Write backs don't occur on STO or HALT (or NOP)
void writeBack(){
  
    WB_inst = "";

    // unpack data from each ALU if there exists an instruciton in their output 
    for (ALU* a : ALUs){
        if (a->Out.state == NEXT){// || a->Out.state == BLOCK || a->Out.state == CURRENT){
            // It should be illegal for the instruction to BLOCK or be CURRENTLY running here but if it does then we would just ignore that write it back

            std::cout << "Write back to index: " << a->Out.DEST << " with value: " << a->Out.OUT << std::endl;
            registerFile[a->Out.DEST] = a->Out.OUT;

            a->Out.state = EMPTY;

            // Debugging/GUI
            WB_inst = a->Inst;
        }
        else ;// Do Nothing as the instruction is EMPTY
    }

    // upack data from each BU, if there exists an instruction in their output
    for (BU* b : BUs){
        if (b->Out.state == NEXT){// || b->Out.state == BLOCK || b->Out.state == CURRENT){
            // It should be illegal for the instruction to BLOCK or be CURRENTLY running here but if it does then we would just ignore that write it back
            
            // Set halt flag here
            systemHaltFlag = b->haltFlag;

            if (b->branchFlag) {
                PC = b->Out.OUT;

                flushPipeline();
                std::cout << "@Write back. PC now equals: " << PC << std::endl;
            }
             
            b->Out.state = EMPTY;

            // Debugging/GUI
            WB_inst = b->Inst;
        }
    }

    // upack data from each BU, if there exists an instruction in their output
    for (LSU* l : LSUs){
        if (l->Out.state == NEXT){// || l->Out.state == BLOCK || l->Out.state == CURRENT){
            // It should be illegal for the instruction to BLOCK or be CURRENTLY running here but if it does then we would just ignore that write it back
            
            if (l->writeBackFlag) {
                registerFile[l->Out.DEST] = l->Out.OUT;
                std::cout << "Write back to index: " << l->Out.DEST << " with value: " << l->Out.OUT << std::endl;
            }

            l->Out.state = EMPTY;

            // Debugging/GUI
            WB_inst = l->Inst;
        }
    }

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
    run();

    // Clean up some pointers
    for (ALU* a : ALUs) delete a;
    for (BU*  b : BUs)  delete b;
    for (LSU* l : LSUs) delete l;

    return 0;
}