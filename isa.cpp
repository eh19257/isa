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

//#include "EnumsAndConstants.hpp"
#include "ExecutionUnits.hpp"

using namespace std;


/* Debugging Flags */
bool PRINT_REGISTERS_FLAG = false;
bool PRINT_MEMORY_FLAG = false;
bool PRINT_STATS_FLAG = false;


int amount_of_instruction_memory_to_output = 8;  // default = 8

/* States */
StageState IF_State = Empty;
StageState ID_State = Empty;
StageState I_State = Empty;
StageState EX_State = Empty;
StageState C_State = Empty;
StageState MA_State = Empty;
StageState WB_State = Empty;


/* Registers */
#pragma region Registers

/* "Register File" - currently just a bunch of variables */
std::array<int, 16> registerFile;    // All 16 general purpose registers
std::array<float, 4> floatingPointRegisterFile;

int PC;                     // Program Counter

// IF/ID registers
std::string CIR;            // Current Instruction Register
int IMMEDIATE;              // Immediate register used for immediate addressing


// ID/I registers
Instruction OpCodeRegister = NOP;       // Stores the decoded OpCode that was in the CIR
int  ALU0, ALU1, ALU_OUT;               // 2 input regsiters for the ALU

DecodedInstruction ID_I_Inst; 

//float ALU_FP0, ALU_FP1;                 // 2 input registers for the ALU where FP calculations are occuring
int HI, LO;                             // High and Low parts of integer multiplication
int ALUD;                               // Destination register for the output of the ALU

// I/EX registers
int ID;
//Instruction I_EX__OpCodeRegister = NOP;

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
string IF_inst = "EMPTY";
string ID_inst = "EMPTY";
string I_inst = "EMPTY";
string EX_inst = "EMPTY";
string C_inst = "EMPTY";
string MA_inst = "EMPTY";
string WB_inst = "EMPTY";


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

void fushPipeline(){
    IF_State = Empty;
    IF_inst = "";

    ID_State = Empty;
    ID_inst = "";

    I_State = Empty;
    I_inst = "";

    EX_State = Empty;
    EX_inst = "";

    // Flush all EUs as well
    ALUs.at(0)->state = IDLE;
    ALUs.at(1)->state = IDLE;
    BUs.at(0)->state = IDLE;
    LSUs.at(0)->state = IDLE;

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
    cout << "Current instruction in the EX: " << EX_inst << endl;
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

    // Change the state of the IF such that it is "currently running"
    if (ID_State == Block || ID_State == Current) {
        IF_State = Block;
        return;
    }

    IF_State = Current;
    
    // Load the memory address that is in the instruction memory address that is pointed to by the PC
    CIR = instrMemory.at(PC);

    /*
    // Increment PC or don't (depending on whether we are on a branch or not)
    if (branchFlag){
        branchFlag = false;
    } else {
        PC++;
    }*/

    PC++;

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
    #pragma region State Setup
    // State change for ID
    if (I_State == Block || I_State == Current) {
        ID_State = Block;
        return;
    }
    if (ID_State == Current); // Do not change anything about the state of this stage and let it continue to run

    else if (IF_State != Next) {
        // Case if the instruction in the Fetch is not moving over

        ID_State = Empty;

        // Debugging/GUI to show that the current instruction is empty
        ID_inst = string("");

        // increments stall count
        numOfStalls += 1;
        return;
    }
    else if (IF_State == Next) {
        // If the previous stage has ran then we can move that instruction into the next stage along the pipeline

        ID_State = Current;

        // Debugging/GUI to show the current instr in the processor
        ID_inst = IF_inst; 
    }
    #pragma endregion State Setup

    std::vector<std::string> splitCIR = split(CIR, ' '); // split the instruction based on ' ' and decode instruction like that
    
    // Throws error if there isn't any instruction to be loaded
    if (splitCIR.size() == 0) throw std::invalid_argument("No instruction loaded");
    
    // Load the register values into the ALU's input
    if (splitCIR.size() > 1) {
        // Get set first/destination register
        if      (splitCIR.at(1).substr(0 ,1).compare("r")  == 0 ) {
            ID_I_Inst.rd = strToRegister(splitCIR.at(1)); 
            ALUD = ID_I_Inst.rd;
        }
        //else if (splitCIR.at(1).substr(0, 2).compare("FP") == 0)  ALUD = (FP_Register) stoi(splitCIR.at(1).substr(1, splitCIR.at(1).length()));

        if (splitCIR.size() > 2) {
            if      (splitCIR.at(2).substr(0, 1).compare("r")  == 0 ) {
                ID_I_Inst.rs0 = strToRegister(splitCIR.at(2));
                ALU0 = registerFile.at(ID_I_Inst.rs0);
            }
            //else if (splitCIR.at(2).substr(0, 2).compare("FP") == 0)  ALU_FP0 = (FP_Register) stoi(splitCIR.at(2).substr(1, splitCIR.at(2).length()));
            
            if (splitCIR.size() > 3) {
                if      (splitCIR.at(3).substr(0,1).compare("r")  == 0 ){
                    ID_I_Inst.rs1 = strToRegister(splitCIR.at(3));
                    ALU1 = registerFile.at(ID_I_Inst.rs1);
                }
                //else if (splitCIR.at(3).substr(0,1).compare("FP") == 0 ) ALU_FP1 = (FP_Register) stoi(splitCIR.at(3).substr(1, splitCIR.at(3).length()));

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
    
    else if (splitCIR.at(0).compare("STO")  == 0) { OpCodeRegister = STO; ALUD = registerFile.at(strToRegister(splitCIR.at(1)));}
    else if (splitCIR.at(0).compare("STOI") == 0) { OpCodeRegister = STOI; IMMEDIATE = stoi(splitCIR.at(1)); }

    else if (splitCIR.at(0).compare("AND")  == 0) OpCodeRegister = AND;
    else if (splitCIR.at(0).compare("OR")   == 0) OpCodeRegister = OR;
    else if (splitCIR.at(0).compare("NOT")  == 0) OpCodeRegister = NOT;
    else if (splitCIR.at(0).compare("LSHFT")== 0) OpCodeRegister = LSHFT;       // IMMEDIATE = stoi(splitCIR.at(3)); }
    else if (splitCIR.at(0).compare("RSHFT")== 0) OpCodeRegister = RSHFT;       // IMMEDIATE = stoi(splitCIR.at(3)); }

    else if (splitCIR.at(0).compare("JMP")  == 0) { OpCodeRegister = JMP; ALUD = registerFile.at(strToRegister(splitCIR.at(1))); }
    else if (splitCIR.at(0).compare("JMPI") == 0) {OpCodeRegister = JMPI; ALUD = registerFile.at(strToRegister(splitCIR.at(1))); }
    else if (splitCIR.at(0).compare("BNE")  == 0) {OpCodeRegister = BNE; ALUD = registerFile.at(strToRegister(splitCIR.at(1))); }
    else if (splitCIR.at(0).compare("BPO")  == 0) {OpCodeRegister = BPO; ALUD = registerFile.at(strToRegister(splitCIR.at(1))); }
    else if (splitCIR.at(0).compare("BZ")   == 0) {OpCodeRegister = BZ; ALUD = registerFile.at(strToRegister(splitCIR.at(1))); }

    else if (splitCIR.at(0).compare("HALT") == 0) OpCodeRegister = HALT;
    else if (splitCIR.at(0).compare("NOP")  == 0) OpCodeRegister = NOP;
    else if (splitCIR.at(0).compare("MV")   == 0) OpCodeRegister = MV;
    else if (splitCIR.at(0).compare("MVHI") == 0) OpCodeRegister = MVHI;
    else if (splitCIR.at(0).compare("MVLO") == 0) OpCodeRegister = MVLO;

    else throw std::invalid_argument("Unidentified Instruction: " + splitCIR.at(0));


    // Crush instruction into a DecodedInstruction dataType
    ID_I_Inst.OpCode = OpCodeRegister;
    ID_I_Inst.DEST = ALUD;
    ID_I_Inst.IN0 = ALU0;
    ID_I_Inst.IN1 = ALU1;
    ID_I_Inst.IMM = IMMEDIATE;
    ID_I_Inst.OUT = PC;     // It is only set to the PC for the single instruction JMPI (indexed JMP)

    ID_State = Next;
}


// Issues the current instruction to it's repsective EU
void issue(){
    #pragma region State Setup
    // State change for I
    if (EX_State == Block || EX_State == Current) {
        I_State = Block;
        return;
    }

    if (I_State == Current); // Do not change anything about the state of this stage and let it continue to run
    else if (ID_State != Next) {
        // If IF is not ready to move on, then ID cannot progress (i.e. it is empty)
        I_State = Empty;

        // Debugging/GUI to show that the current instruction is empty
        I_inst = string("");

        // increments stall count
        numOfStalls += 1;
        return;
    } else {
        // Else it can run
        I_State = Current;

        // Debugging/GUI to show the current instr in the processor
        I_inst = ID_inst;
    }
    #pragma endregion State Setup

    // ALUs
    if (ID_I_Inst.OpCode >= ADD && ID_I_Inst.OpCode <= CMP || ID_I_Inst.OpCode == MV){
        ALU_RV.push_back(ID_I_Inst);
        
    }
    // ALU
    else if (ID_I_Inst.OpCode >= AND && ID_I_Inst.OpCode <= RSHFT) {
        ALU_RV.push_back(ID_I_Inst);

    }
    // BU
    else if (ID_I_Inst.OpCode >= JMP && ID_I_Inst.OpCode <= BZ || ID_I_Inst.OpCode == HALT || ID_I_Inst.OpCode == NOP) {
        BU_RV.push_back(ID_I_Inst);

    }
    // LSU
    else if (ID_I_Inst.OpCode >= LD && ID_I_Inst.OpCode <= STOI) {
        LSU_RV.push_back(ID_I_Inst);
        
    } else {
        throw std::invalid_argument("Could no issue an unknown instruction: " + ID_I_Inst.OpCode);
    }

    I_State = Next;
}


// Executes the current instruction
void execute(){
    #pragma region State Setup
    // Prepare state for EX
    if (WB_State == Block || WB_State == Current) {
        EX_State = Block;
        return;
    }

    if (EX_State == Current); // Do not change anything about the state of this stage and let it continue to run
    else if (I_State != Next) {
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
        EX_inst = I_inst;
    }
    #pragma endregion State Setup

    // Set flags to false
    MEM_writeBackFlag = false;
    memoryReadFlag = false;
    memoryWriteFlag = false;

    // FILLING IN THE RESERVATION STATIONS
    if      (ALUs.at(0)->state == IDLE && ALU_RV.size() > 0) {
        ALUs.at(0)->loadInInstruction(ALU_RV.back());// LOAD IN ID_I_Inst into the ALU
        ALU_RV.pop_back();

        ALUs.at(0)->state = READY;
    }
    else if (BUs.at(0)->state == IDLE && BU_RV.size() > 0) {

        DecodedInstruction foo = BU_RV.back();
        std::cout << "PRINTING FOO: ";
        foo.print();
        BUs.at(0)->loadInInstruction(foo);
        BU_RV.pop_back();

        BUs.at(0)->state = READY;
    }
    else if (LSUs.at(0)->state == IDLE && LSU_RV.size() > 0){
        LSUs.at(0)->loadInInstruction(LSU_RV.back());
        LSU_RV.pop_back();

        LSUs.at(0)->state = READY;
    }
    else {
        cout << "Nothing to exectue" << endl;
        numOfBubbles++;
    }
    // REPEAT FOR ALL OTHER EUs

    // Run all EUs
    for (ALU* a : ALUs) if (a->state == READY || a->state == RUNNING) a->cycle();
    for (BU*  b : BUs ) if (b->state == READY || b->state == RUNNING) b->cycle();
    for (LSU* l : LSUs) if (l->state == READY || l->state == RUNNING) l->cycle();
  
    EX_State = Next;
}


// Multiplexes the output of the EUs into a single line that the can then be written back
void complete(){
    #pragma region State Setup
    // Prepare state for EX
    if (WB_State == Block || WB_State == Current) {
        C_State = Block;
        return;
    }

    if (C_State == Current); // Do not change anything about the state of this stage and let it continue to run
    else if (EX_State != Next) {
        // If IF is not ready to move on, then ID cannot progress (i.e. it is empty)
        C_State = Empty;

        // Debugging/GUI to show that the current instruction is empty
        C_inst = string("");

        // increments stall count
        numOfStalls += 1;
        return;
    } else {
        // Else it can run
        C_State = Current;

        // Debugging/GUI to show the current instr in the processor
        C_inst = EX_inst;
    }
    #pragma endregion State Setup

    bool foundOutputFlag = false;
    writeBackFlag = false;

    // ALU
    for (ALU* a : ALUs){
        if (a->state == DONE){
            C_OUT = a->OUT;
            writeBackFlag = true;
            WBD = a->DEST_OUT;

            a->state = IDLE;
            
            foundOutputFlag = true;
            break;
        }
    }
    // BU
    if (!foundOutputFlag) for (BU* b : BUs){
        if (b->state == DONE){
            if (b->branchFlag){
                PC = b->OUT;
                branchFlag = b->branchFlag;

                fushPipeline();
            }
            systemHaltFlag = b->haltFlag;
            b->state = IDLE;            
            
            foundOutputFlag = true;
            break;
        }
    }
    // LSU
    if (!foundOutputFlag) for (LSU* l : LSUs){
        cout << "State of the LSU in COmplete(): " << l->state << endl;
        if (l->state == DONE){
            C_OUT = l->OUT;
            WBD = l->DEST_OUT;
            std::cout << "In Complete(), writeBackFlag: " << l-> writeBackFlag << endl;
            writeBackFlag = l->writeBackFlag;
            l->state = IDLE;
            
            foundOutputFlag = true;
            break;
        }
    }

    C_State = Next;
}

// Data written back into register file: Write backs don't occur on STO or HALT (or NOP)
void writeBack(){
    #pragma region State Setup
    // Prepare State for WB
    if (WB_State == Current); // Do not change anything about the state of this stage and let it continue to run
    else if (EX_State != Next) {
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
        WB_inst = EX_inst;
    }
    #pragma endregion State Setup

    int writeOut;
    int destination;

    bool foundOutputFlag = false;
    writeBackFlag = false;

    // Unpack data from ALUs
    for (ALU* a : ALUs){
        if (a->state == DONE){
            writeOut = a->OUT;
            writeBackFlag = true;
            destination = a->DEST_OUT;

            a->state = IDLE;
            
            foundOutputFlag = true;
            break;
        }
    }
    // Unpack data from BUs
    if (!foundOutputFlag) for (BU* b : BUs){
        if (b->state == DONE){
            if (b->branchFlag){
                PC = b->OUT;
                branchFlag = b->branchFlag;

                fushPipeline();
            }
            systemHaltFlag = b->haltFlag;
            b->state = IDLE;            
            
            foundOutputFlag = true;
            break;
        }
    }
    // Unpack data from LSUs
    if (!foundOutputFlag) for (LSU* l : LSUs){
        cout << "State of the LSU in writeBack(): " << l->state << endl;
        if (l->state == DONE){
            writeOut = l->OUT;
            destination = l->DEST_OUT;
            std::cout << "In writeBack(), writeBackFlag: " << l-> writeBackFlag << endl;
            writeBackFlag = l->writeBackFlag;
            l->state = IDLE;
            
            foundOutputFlag = true;
            break;
        }
    }


    cout << "WRITE BACK" << endl;
    if (writeBackFlag) {
        std::cout << "Write back to index: " << destination << " with value: " << writeOut << std::endl;
        registerFile[destination] = writeOut;
    }
    
    WB_State = Next;
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