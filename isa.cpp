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
#include <deque>
#include <map>


//#include "EnumsAndConstants.hpp"
#include "ExecutionUnits.hpp"

using namespace std;


/* Debugging Flags */
bool PRINT_REGISTERS_FLAG = false;
bool PRINT_MEMORY_FLAG = false;
bool PRINT_STATS_FLAG = false;
bool SINGLE_STEP_FLAG = false;


int amount_of_instruction_memory_to_output = 8;  // default = 8

/* Registers */
#pragma region Registers

/* Register Files */
std::array<int, SIZE_OF_REGISTER_FILE> ArchRegisterFile;        // Architecural register file for 16 general purpose registers
std::array<std::pair<int, bool>, SIZE_OF_REGISTER_FILE * 4> PhysRegisterFile;   // Physcial regsiter file for n*16 general purpose registers
std::map<int, int> ARF_To_PRF;                             // Map between ARF regstiers and PRF registers
//std::array<float, 4> floatingPointRegisterFile;

int PC;                     // Program Counter

// IF/IDI registers
std::string CIR;            // Current Instruction Register
int IMMEDIATE;              // Immediate register used for immediate addressing

NonDecodedInstruction IF_ID_Inst;   // WARNING THIS IS A NON decoded instruction that is ONLY USED ONCE (here in IF)
std::array<NonDecodedInstruction, SUPERSCALAR_WIDTH> IF_IDI;

// IDI/ALU_RVS


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

/* Hazard Detection Units */
HDU* HazardDetectionUnit = new HDU();

/* Execution Units*/
std::array<ALU*, 2> ALUs = {new ALU(HazardDetectionUnit), new ALU(HazardDetectionUnit)};
std::array<BU*, 1> BUs = {new BU(HazardDetectionUnit)};
std::array<LSU*, 1> LSUs = {new LSU(HazardDetectionUnit, &dataMemory)};

/* Reservation Stations */
std::deque<std::pair<DecodedInstruction, int>> ALU_RV;
std::deque<std::pair<DecodedInstruction, int>> BU_RV;
std::deque<std::pair<DecodedInstruction, int>> LSU_RV;

/* ISA Function headers */
void fetch();
void decodeIssue();
void decode();  
void issue();
void execute();
void complete();
void memoryAccess();
void writeBack();

/* ISA helpers */
void flushPipeline();
bool AllEUsAndRVsEmpty();
void initialisePRF();

/* DecodedIssue helpers */
void ForwardResultsToRVs();
void OffLoadingReservationStations();

bool CheckIfWriteOnly(DecodedInstruction inst);
int GetUnusedRegisterInPRF();
bool IsRegisterValidForExecution(int* reg, int* val);
DecodedInstruction RegisterRenameValidityCheck(DecodedInstruction inst);


/* Non-ISA function headers */
void loadProgramIntoMemory();
std::vector<std::string> split(std::string str, char deliminator);
Register strToRegister(std::string str);
bool handleProgramFlags(int count, char** arguments);

/* Debugging function headers*/
void outputAllMemory(int cutOff);
void printArchRegisterFile(int maxReg);
void outputStatistics(int numOfCycles);

/* Debugging/GUI for showing whch Instruction is in which stage */
array<string, SUPERSCALAR_WIDTH> IF_inst;
array<string, SUPERSCALAR_WIDTH> IDI_inst;
string ID_inst = "";
string I_inst = "";
string EX_inst = "";
string WB_inst = "";

array<DecodedInstruction, SUPERSCALAR_WIDTH> IF_gui;
array<DecodedInstruction, SUPERSCALAR_WIDTH> IDI_gui;
DecodedInstruction EX_gui;
DecodedInstruction WB_gui;

/* Stats variables */
int numOfCycles = 1;        // Counts the number of cycles (stats at cycle 1 not cycle 0)
int numOfBranches = 0;
int numOfStalls = 0;        // Counts the number of times the pipeline stalls
int numOfBubbles = 0;

/* Tracking Variables */
int IndexOfLastRegisterUsedInPRF = -1;
int currentSideOfBranch = 0;            // A global variable that keeps track of which side of the branch the instruction is on
const int maxNumberOfBranchSides = 2;         // In the case of a branch taken, then we need to flush out all of the incorrect instructions in the pipeline
                                        // We can keep track of which side of the branch we are in by using this currentSideOfBranch variable. On intialisation, we give each of
                                        // the instructions a number associated with the side of the branch. If the instruction is a branch then we increment this global value.
                                        // When we flush the pipline, we remove the instructions which have a different sideOfBranch as the BRANCH instruction
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

void printArchRegisterFile(int maxReg){
    std::cout << std::endl;
    std::cout << "PC: " << PC << std::endl;
    std::cout << "CIR: " << CIR << std::endl;
    for (int i = 0; /*(i < 16) &&*/ (i < maxReg); i++){
        std::cout << "PRF" << i << ": " << PhysRegisterFile.at(i).first << "\t\t" << PhysRegisterFile.at(i).second << std::endl;
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

void initialisePRF(){
    for (int i = 0; i < PhysRegisterFile.size(); i++){
        PhysRegisterFile.at(i) = std::pair<int, bool>(0, true);
    }
}

void RemoveFromPRF(DecodedInstruction inst){
    if (inst.IsWriteBack && inst.rd != -1){
        PhysRegisterFile.at(inst.rd).second = true;
    }
}

// Returns true if all the execution units are empty
bool AllEUsAndRVsEmpty(){    
    for (ALU* a : ALUs) {
        if (a->In.state  != EMPTY) return false;
        if (a->Out.state != EMPTY) return false;
    }
    for (BU* b : BUs) {
        if (b->In.state  != EMPTY) return false;
        if (b->Out.state != EMPTY) return false;
    }
    for (LSU* l : LSUs) {
        if (l->In.state  != EMPTY) return false;
        if (l->Out.state != EMPTY) return false;
    }

    // Check RVs
    if (!ALU_RV.empty()) return false;
    if (!BU_RV.empty() ) return false;
    if (!LSU_RV.empty()) return false;

    return true;
}


// Flush reservation station if the instruction is on the incorrent sideOfTheBranch
void flushRV(std::deque<std::pair<DecodedInstruction, int>>* RV, int thisSideOfBranch){
    for (int i = 0; i < RV->size(); i++){
        if (RV->at(i).first.sideOfBranch != thisSideOfBranch){
            // If instruction is writing back to a specific register then we need to remove it from the raw table and also remove the validity flag on the PRF
            HazardDetectionUnit->RemoveFromRAWTable(RV->at(i).first);
            RemoveFromPRF(RV->at(i).first);

            RV->erase(RV->begin() + i);
        }
    }
}

void flushPipeline(int thisSideOfBranch){
    for (int i = 0; i < IF_IDI.size(); i++){
        IF_IDI[i].state = EMPTY;
    }

    IF_inst = array<string, SUPERSCALAR_WIDTH>();

    IDI_inst = array<string, SUPERSCALAR_WIDTH>();

    for (int i = 0; i < i; i++){
        if (ALUs.at(i)->In.sideOfBranch != thisSideOfBranch){
            ALUs.at(i)->In.state = EMPTY;
            ALUs.at(i)->currentInst = DecodedInstruction();
        }
    }
    
    if (BUs.at(0)->In.sideOfBranch != thisSideOfBranch){
        BUs.at(0)->In.state = EMPTY;
        BUs.at(0)->currentInst = DecodedInstruction();
    }

    if (LSUs.at(0)->In.sideOfBranch != thisSideOfBranch){
        LSUs.at(0)->In.state = EMPTY;
        LSUs.at(0)->currentInst = DecodedInstruction();
    }

    // Clean RVs as well and removes any of the instructions from the RAW table and PRF
    flushRV(&ALU_RV, thisSideOfBranch);
    flushRV(&BU_RV, thisSideOfBranch);
    flushRV(&LSU_RV, thisSideOfBranch);
}

// Checks whether or not an instruction uses a register to write to and only write to - used for register renaming
bool CheckIfWriteOnly(DecodedInstruction inst){
    //if (inst.rd == -1 || !inst.IsWriteBack || inst.rd == inst.rs0 || inst.rd == inst.rs1) return false;
    if (inst.rd == -1 || !inst.IsWriteBack) return false;
    return true;
}

// Gets the index of an unused regsiter in the physicla register file
int GetUnusedRegisterInPRF(){
    int foo = IndexOfLastRegisterUsedInPRF;

    // iterate through the x - n registers in the PRF
    cout << "size of PRF: " << PhysRegisterFile.size() << endl;
    cout << "IndexOfLastRegisterUsedInPRF: " << IndexOfLastRegisterUsedInPRF << endl;
    
    while (foo < ((int) PhysRegisterFile.size() - 1)) {
        foo = foo + 1;
        cout << "foo: " << foo << endl;
        if (PhysRegisterFile.at(foo).second) {
            IndexOfLastRegisterUsedInPRF = foo;
            return IndexOfLastRegisterUsedInPRF;
        }
    }
    // iterate through 0-x registers in the PRF
    int bar = -1;
    while (bar < IndexOfLastRegisterUsedInPRF){
        bar = bar + 1;
        if (PhysRegisterFile.at(bar).second) {
            IndexOfLastRegisterUsedInPRF = bar;
            return IndexOfLastRegisterUsedInPRF;
        }
    }
    cout << "This is run" << endl;
    // if there aren't any registers avaliable (i.e. they are all in use which is very unlikely), then return -1
    IndexOfLastRegisterUsedInPRF = -1;
    return IndexOfLastRegisterUsedInPRF;
}

// Require so that the PRF doesnt get filled with falsely invalid registers on an instruction BLOCK
void UndoDestinationRegisterValidity(DecodedInstruction* inst){
    if (inst->IsWriteBack && inst->rd != -1){
        PhysRegisterFile.at(inst->rd).second = true;

        IndexOfLastRegisterUsedInPRF = IndexOfLastRegisterUsedInPRF - 1;
    }
}

// Returns true if this reg is valid for execution/use
bool IsRegisterValidForExecution(int* reg, int* val){
    if (*reg == -1) return true;

    // If the valid bit is true
    if (PhysRegisterFile.at(*reg).second){
        return true;
    } else {
        // If the valid bit is not true then we can check the RAW/result forwarding table for an entry
        // If an entry is found, then it will return true (as it is valid) and then also update the value for the instruction
        return HazardDetectionUnit->CheckRAW_TableForForwardedValues(reg, val);
    }
}

// Checks if an instruction's validity based on the validity of the 2 source registers (returned inst can BLOCK or NEXT)
DecodedInstruction CheckBothRegistersAreValidForFurtherExecution(DecodedInstruction inst){
    bool IsRs0Valid = IsRegisterValidForExecution(&inst.rs0, &inst.IN0);
    bool IsRs1Valid = IsRegisterValidForExecution(&inst.rs1, &inst.IN1);

    if (IsRs0Valid && IsRs1Valid){
        inst.state = NEXT;
    } else {
        inst.state = BLOCK;
    }

    return inst;
}

// Checks both source registers to see if they are valid for further execution
DecodedInstruction RegisterRenameValidityCheck(DecodedInstruction inst){
    bool IsRs0Valid = true;
    bool IsRs1Valid = true;

    // if RS0 is a valid register then we can rename rs0
    if (inst.rs0 != -1){
        inst.rs0 = ARF_To_PRF[inst.rs0];    // renames rs0
        inst.IN0 = PhysRegisterFile.at(inst.rs0).first;     // preload in the value in for this register before we check it's validity

        // If this register is valid to be used then it will return true and update the value if necerssary
        IsRs0Valid = IsRegisterValidForExecution(&inst.rs0, &inst.IN0);
    }

    // if RS0 is a valid register then we can rename rs0
    if (inst.rs1 != -1){
        inst.rs1 = ARF_To_PRF[inst.rs1];    // renames rs0
        inst.IN1 = PhysRegisterFile.at(inst.rs1).first;     // preload in the value in for this register before we check it's validity

        // If this register is valid to be used then it will return true and update the value if necerssary
        IsRs1Valid = IsRegisterValidForExecution(&inst.rs1, &inst.IN1);
    }

    //if one of them is not valid then we MUST BLOCK this instruction
    if ( IsRs0Valid && IsRs1Valid ){
        inst.state = NEXT;
    } else {
        inst.state = BLOCK;
    }

    return inst;
}

// A single cycle of the processor
void cycle(){
    //if (numOfCycles == 26) outputAllMemory(amount_of_instruction_memory_to_output);
    std::cout << "---------- Cycle " << numOfCycles << " starting ----------"<< std::endl;
    //std::cout << "PC has current value: " << PC << std::endl;


    // Non-pipelined 
    //fetch(); decode(); issue(); execute(); complete(); writeBack();

    // Pipelined
    //writeBack(); /*complete();*/ execute(); issue(); decode(); fetch();

    writeBack(); execute();  decodeIssue(); fetch();

    //HazardDetectionUnit->printRAW();

    cout << "\nCurrent instruction in the IF: " << IF_inst[0] << std::endl;
    cout << "Current instruction in the IDI: "; IDI_gui[0].printHuman(); std::cout << std::endl;
    //cout << "Current instruction in the I:  " << I_inst << endl;
    cout << "Current instruction in ALU0: "; ALUs.at(0)->currentInst.printHuman(); std::cout << "\tALU1: "; ALUs.at(1)->currentInst.printHuman(); std::cout << "\tBU: "; BUs.at(0)->currentInst.printHuman(); std::cout << "\tLSU: "; LSUs.at(0)->currentInst.printHuman(); std::cout << std::endl;
    cout << "Current instruction in the WB: ";  WB_gui.printHuman(); std::cout << std::endl; 
            
    cout << "\n\tALU_RV\t\tBU_RV \t\tLSU_RV" << endl;
    for (int i = 0; i < MAX_RV_SIZE; i++){
        cout << i << "\t";
        if (ALU_RV.size() <= i) cout << "\t\t";
        else { ALU_RV.at(i).first.printHuman(); cout << " ::" << ALU_RV.at(i).first.state << "\t"; }

        if (BU_RV.size() <= i) cout << "\t\t";
        else { BU_RV.at(i).first.printHuman(); cout << " ::" << BU_RV.at(i).first.state << "\t"; }
    
        if (LSU_RV.size() <= i) cout << "\t\t";
        else { LSU_RV.at(i).first.printHuman(); cout << " ::" << LSU_RV.at(i).first.state << "\t"; }
        
        cout << endl;
    }

    cout << "\n\n" << endl;
    if (PRINT_REGISTERS_FLAG) printArchRegisterFile(26);
    if (PRINT_MEMORY_FLAG) outputAllMemory(amount_of_instruction_memory_to_output);

    std::cout << "---------- Cycle " << numOfCycles << " completed. ----------\n"<< std::endl;
    numOfCycles++;
}

// Running the processor
void run(){
    // Print memory before running the program
    if (PRINT_MEMORY_FLAG) outputAllMemory(amount_of_instruction_memory_to_output);

    outputAllMemory(amount_of_instruction_memory_to_output);

    std::string foo;
    while (!(systemHaltFlag && AllEUsAndRVsEmpty())) {
        cycle();        
        if (SINGLE_STEP_FLAG) std::cin >> foo;
    }

    outputAllMemory(amount_of_instruction_memory_to_output);

    std::cout << "Program has been halted\n" << std::endl;

    // Print the memory after the program has been ran
    //if (PRINT_MEMORY_FLAG) outputAllMemory(amount_of_instruction_memory_to_output);
    if (PRINT_STATS_FLAG) outputStatistics(numOfCycles);
}

// Fetches the next instruction that is to be ran, this instruction is fetched by taking the PCs index 
void fetch(){

    // Run through every channel of the processor
    for (int i = 0; i < IF_IDI.size(); i++){

        if (IF_IDI[i].state == BLOCK || IF_IDI[i].state == CURRENT){
                // We cannot collect any instructions from memory and pass it into IF_ID_Inst
                cout << "Fetch() BLOCKED" << endl;
                continue;
            }
        // else if IF_ID_Inst.state = NEXT or EMPTY - it shouldnt be equal to NEXT here but we check anyway

        IF_IDI[i].state = CURRENT;
        
        // Load the memory address that is in the instruction memory address that is pointed to by the PC
        IF_IDI[i].instruction = instrMemory.at(PC);

        PC++;

        // Debugging/GUI to show the current instr in the processor
        IF_inst[i] = IF_IDI[i].instruction;

        if (IF_IDI[i].instruction.empty()) {
            IF_IDI[i].state = EMPTY;
            return;
        } else {
            // If it is not empty then we have successfully fetched the instruction
            IF_IDI[i].state = NEXT;

            std::cout << "CIR has current value: " << IF_IDI[i].instruction << std::endl;
        }

        // INCORRECT \/\/
        /* We DO NOT UPDATE the PC here but instead we do it in the DECODE stage as this will help with pipelining branches later on */
        // Instead of incrementing the PC here we could use a NPC which is used by MIPS and stores the next sequential PC
        // Increment PC
        //pc++; 


    }
}

void ForwardResultsToRVs(){
    for (int i = 0; i < ALU_RV.size(); i++) {
        ALU_RV.at(i).first = CheckBothRegistersAreValidForFurtherExecution(ALU_RV.at(i).first);
        ALU_RV.at(i).second++;      // Increases the "age" of the instruction being in the RV
    }

    for (int i = 0; i < BU_RV.size(); i++)   {
        BU_RV.at(i).first = CheckBothRegistersAreValidForFurtherExecution(BU_RV.at(i).first);
        BU_RV.at(i).second++;       // Increases the "age" of the instruction being in the RV
    }
    for (int i = 0; i < LSU_RV.size(); i++) {
        LSU_RV.at(i).first = CheckBothRegistersAreValidForFurtherExecution(LSU_RV.at(i).first); 
        LSU_RV.at(i).second++;      // Increases the "age" of the instruction being in the RV
    }
}

DecodedInstruction GetOldestValidInstruction(DecodedInstruction inst, std::deque<std::pair<DecodedInstruction, int>>* RV){
    if (RV->empty()) return inst;

    int indexOfOldestValidInst = -1;
    int oldestAge = -1;

    for (int i = 0; i < RV->size(); i++){
        // Is this inst valid
        if (RV->at(i).first.state != BLOCK) {
            // Is this inst
            if (RV->at(i).second > oldestAge){
                oldestAge = RV->at(i).second;
                indexOfOldestValidInst = i;
            }
        }
    }
    if (indexOfOldestValidInst == -1) return inst;

    inst = RV->at(indexOfOldestValidInst).first;
    RV->erase(RV->begin() + indexOfOldestValidInst);
    
    return inst;
}

void OffLoadingReservationStations(){
    ///////////////////////////////////
    ////////// RV OFF LOADING /////////
    ///////////////////////////////////

    // If the EUs are free, then we can off load the RVs into the EUs
    for (ALU* a : ALUs){
        if (a->In.state == BLOCK){
            // We run through ever entry in the RV, if it is blocking there might be an entry in the RAW table which can help us 
            
            //a->In = HazardDetectionUnit->CheckForRAW(a->In);
        }

        if (a->In.state == CURRENT){
            // If the In-instruction for this EU is BLOCKING or CURRENTLY running then we do not d anything
            
            continue;

        } else if (a->In.state == EMPTY ){//|| a->In.state == NEXT){
            a->In = GetOldestValidInstruction(a->In, &ALU_RV);

        } else {
            cout << "Instruction in the ALU has gone mad!!! - INSTRUCTION HAS STATE NEXT!!!" << endl;
        }
    }

    // If the BUs are free, then we can off load the RVs into the BUs
    for (BU* b : BUs){
        if (b->In.state == BLOCK){
            // We run through ever entry in the RV, if it is blocking there might be an entry in the RAW table which can help us 
            //b->In = HazardDetectionUnit->CheckForRAW(b->In);
        }

        if (b->In.state == CURRENT){
            // If the In-instruction for this EU is BLOCKING or CURRENTLY running then we do not d anything
            
            continue;

        } else if (b->In.state == EMPTY){// || b->In.state == NEXT){
            b->In = GetOldestValidInstruction(b->In, &BU_RV);

        } else {
            cout << "Instruction in the BU has gone mad!!! - INSTRUCTION HAS STATE NEXT!!!" << endl;
        }
    }

    // If the LSUs are free, then we can off load the RVs into the LSUs
    for (LSU* l : LSUs){
        if (l->In.state == BLOCK){
            // We run through ever entry in the RV, if it is blocking there might be an entry in the RAW table which can help us 
            //l->In = HazardDetectionUnit->CheckForRAW(l->In);
        }

        if (l->In.state == CURRENT){
            // If the In-instruction for this EU is BLOCKING or CURRENTLY running then we do not d anything
            
            continue;

        } else if (l->In.state == EMPTY){//|| l->In.state == NEXT){
            l->In = GetOldestValidInstruction(l->In, &LSU_RV);
            /*
            // If the LSU is not EMPTY then we can check if the last instruction in there is ready to be NEXT 
            if (!LSU_RV.empty()){
                if (LSU_RV.front().state == NEXT){

                    // If the instruction in the RV is ready to be NEXT, then it is loaded into the LSU
                    
                    // LOAD INTO LSU
                    l->In = LSU_RV.front();
                    LSU_RV.pop_front();
                }
            }*/
        } else {
            cout << "Instruction in the LSU has gone mad!!! - INSTRUCTION HAS STATE NEXT!!!" << endl;
        }
    }   
}


// Combined Instruction Decode with Issue
void decodeIssue(){
    // Cycle through every channel
    for (int i = 0; i < IF_IDI.size(); i++){

        // State checking for previous registers
        if (IF_IDI[i].state == NEXT || IF_IDI[i].state == BLOCK || IF_IDI[i].state == CURRENT){
                IF_IDI[i].state = CURRENT;
        }
        else {
            // Decode() cannot be run with no inputs and so we return

            IDI_gui[i] = DecodedInstruction();
            continue;
        }
        
        // Make new inst instance for decoded inst
        DecodedInstruction parsedInst;
        parsedInst.state = CURRENT;
        
        std::vector<std::string> splitCIR = split(IF_IDI[i].instruction, ' '); // split the instruction based on ' ' and decode instruction like that
        
        // Throws error if there isn't any instruction to be loaded
        if (splitCIR.size() == 0) throw std::invalid_argument("No instruction loaded");
        
        // Debugging/GUI
        parsedInst.SplitInst = splitCIR;

        // Load the register values into the ALU's input - Setting RD
        if (splitCIR.size() > 1) {
            // Get set first/destination register
            if (splitCIR.at(1).substr(0 ,1).compare("r")  == 0 ) {
                parsedInst.rd = strToRegister(splitCIR.at(1)); 
            }

            // Setting RS0
            if (splitCIR.size() > 2) {
                if (splitCIR.at(2).substr(0, 1).compare("r")  == 0 ) {
                    parsedInst.rs0 = strToRegister(splitCIR.at(2));
                }
                
                // Setting RS1
                if (splitCIR.size() > 3) {
                    if (splitCIR.at(3).substr(0,1).compare("r")  == 0 ){
                        parsedInst.rs1 = strToRegister(splitCIR.at(3));
                    } 

                }
            }
        }




        // SET WRTEBACK TO TRUE AS DEFAULT
        parsedInst.IsWriteBack = true;

        // Sets which branch the instruction follows
        parsedInst.sideOfBranch = currentSideOfBranch;

        // if statement for decoding all instructions
            if (splitCIR.at(0).compare("ADD")  == 0) parsedInst.OpCode = ADD;
        else if (splitCIR.at(0).compare("ADDI") == 0) { parsedInst.OpCode = ADDI; parsedInst.IMM = stoi(splitCIR.at(3)); }
        else if (splitCIR.at(0).compare("ADDF") == 0) parsedInst.OpCode = ADDF;
        else if (splitCIR.at(0).compare("SUB")  == 0) parsedInst.OpCode = SUB;
        else if (splitCIR.at(0).compare("SUBF") == 0) parsedInst.OpCode = SUBF;
        else if (splitCIR.at(0).compare("MUL")  == 0) parsedInst.OpCode = MUL;
        else if (splitCIR.at(0).compare("MULO") == 0) parsedInst.OpCode = MULO;
        else if (splitCIR.at(0).compare("MULFO")== 0) parsedInst.OpCode = MULFO;
        else if (splitCIR.at(0).compare("DIV")  == 0) parsedInst.OpCode = DIV;
        else if (splitCIR.at(0).compare("DIVF") == 0) parsedInst.OpCode = DIVF;
        else if (splitCIR.at(0).compare("CMP")  == 0) parsedInst.OpCode = CMP;

        else if (splitCIR.at(0).compare("LD")   == 0) parsedInst.OpCode = LD;
        else if (splitCIR.at(0).compare("LDD")  == 0) { parsedInst.OpCode = LDD; parsedInst.IMM = stoi(splitCIR.at(2)); } 
        else if (splitCIR.at(0).compare("LDI")  == 0) { parsedInst.OpCode = LDI; parsedInst.IMM = stoi(splitCIR.at(2)); }
        else if (splitCIR.at(0).compare("LID")  == 0) parsedInst.OpCode = LID;
        else if (splitCIR.at(0).compare("LDA")  == 0) parsedInst.OpCode = LDA;
        
        else if (splitCIR.at(0).compare("STO")  == 0) { parsedInst.OpCode = STO; parsedInst.IsWriteBack = false; }//ArchRegisterFile.at(strToRegister(splitCIR.at(1))); parsedInst.IsWriteBack = false; }
        else if (splitCIR.at(0).compare("STOI") == 0) { parsedInst.OpCode = STOI; parsedInst.IMM = stoi(splitCIR.at(1)); parsedInst.IsWriteBack = false; }
        else if (splitCIR.at(0).compare("STOA") == 0) { parsedInst.OpCode = STOA; parsedInst.IsWriteBack = false; }

        else if (splitCIR.at(0).compare("AND")  == 0) parsedInst.OpCode = AND;
        else if (splitCIR.at(0).compare("OR")   == 0) parsedInst.OpCode = OR;
        else if (splitCIR.at(0).compare("NOT")  == 0) parsedInst.OpCode = NOT;
        else if (splitCIR.at(0).compare("LSHFT")== 0) parsedInst.OpCode = LSHFT;       // IMMEDIATE = stoi(splitCIR.at(3)); }
        else if (splitCIR.at(0).compare("RSHFT")== 0) parsedInst.OpCode = RSHFT;       // IMMEDIATE = stoi(splitCIR.at(3)); }

        else if (splitCIR.at(0).compare("JMP")  == 0) { parsedInst.OpCode = JMP; parsedInst.IsWriteBack = false; currentSideOfBranch = (currentSideOfBranch + 1) % maxNumberOfBranchSides; }
        else if (splitCIR.at(0).compare("JMPI") == 0) { parsedInst.OpCode = JMPI; parsedInst.IsWriteBack = false; currentSideOfBranch = (currentSideOfBranch + 1) % maxNumberOfBranchSides; }
        else if (splitCIR.at(0).compare("BNE")  == 0) { parsedInst.OpCode = BNE; parsedInst.IsWriteBack = false; currentSideOfBranch = (currentSideOfBranch + 1) % maxNumberOfBranchSides; }
        else if (splitCIR.at(0).compare("BPO")  == 0) { parsedInst.OpCode = BPO; parsedInst.IsWriteBack = false; currentSideOfBranch = (currentSideOfBranch + 1) % maxNumberOfBranchSides; }
        else if (splitCIR.at(0).compare("BZ")   == 0) { parsedInst.OpCode = BZ; parsedInst.IsWriteBack = false; currentSideOfBranch = (currentSideOfBranch + 1) % maxNumberOfBranchSides; }

        else if (splitCIR.at(0).compare("HALT") == 0) { parsedInst.OpCode = HALT; parsedInst.IsWriteBack = false; }
        else if (splitCIR.at(0).compare("NOP")  == 0) { parsedInst.OpCode = NOP; parsedInst.IsWriteBack = false; }
        else if (splitCIR.at(0).compare("MV")   == 0) parsedInst.OpCode = MV;
        else if (splitCIR.at(0).compare("MVHI") == 0) parsedInst.OpCode = MVHI;
        else if (splitCIR.at(0).compare("MVLO") == 0) parsedInst.OpCode = MVLO;

        else throw std::invalid_argument("Unidentified Instruction: " + splitCIR.at(0));


        
        //////////////////////////////
        ////////// RENAMING //////////
        //////////////////////////////

        parsedInst.state = NEXT;
        
        // Rename and get the values for the new instruction
        parsedInst = RegisterRenameValidityCheck(parsedInst);

        // Here we apply register renaming FOR THE DESTINATIONS REGISTER
        if (parsedInst.rd != -1){
            if (parsedInst.IsWriteBack){
                if (parsedInst.rd != parsedInst.rs0 && parsedInst.rd != parsedInst.rs1){
                    int Prd = GetUnusedRegisterInPRF();

                    cout << "Prd: " << Prd << endl;

                    // If there are no usable registers in the PRF then we cannot continue and so the IF/IDI register must block
                    if (Prd == -1){
                        cout << "This is happening" << endl;
                        IF_IDI[i].state = BLOCK;

                        IDI_gui[i] = DecodedInstruction();
                        continue;
                        
                    } else {
                        // Create entry in ARF_To_PRF
                        ARF_To_PRF[parsedInst.rd] = Prd;

                        // Make the register nonvalid as it is currently being written too
                        PhysRegisterFile.at(Prd).second = false;

                        // Replace the ARF register in the instruction with the new PRF one
                        parsedInst.rd = Prd;               
                    }
                } else {
                    // In the case where it is not ONLY a write to the destination register - we use the map from the ARF to the PRF as the PRF reg
                    parsedInst.rd = ARF_To_PRF[parsedInst.rd];
                }

                // Both use the DEST as just the straight rd
                parsedInst.DEST = parsedInst.rd;


            } else {
                // This covers the register renaming cases for when the instruction doesn't write back to the destination register but instead uses it in another way
                parsedInst.rd = ARF_To_PRF[parsedInst.rd];
                parsedInst.DEST = PhysRegisterFile.at(parsedInst.rd).first;


                if (!IsRegisterValidForExecution(&parsedInst.rd, &parsedInst.DEST)){
                    parsedInst.state = BLOCK;
                } else; // We either BLOCK or NEXT depending on the validity of the other registers
            }
        }  




        // SUPER IMPORTANT - THIS IS THE PREVIOUS VALUE OF THE PC TO USE THE CORRECT OLD VALUE OF IT AND NOT THE NEW VALUE
        parsedInst.OUT = IF_IDI[i].PC;     // It is only set to the PC for the single instruction JMPI (indexed JMP)
           
        // Debuggin/GUI
        parsedInst.asString = IF_IDI[i].instruction;
        IDI_gui[i] = parsedInst;

        // instruction set to NEXT here as the RegisterRenameValidityCheck(.) line updates it to BLOCK if required
        //parsedInst.state = NEXT;


        std::cout << "Instruction " << ID_I_Inst.asString << " in IDI decoded: "; ID_I_Inst.print();


        ////////////////////////////
        ///// ISSUING INTO RVS /////
        ///////////////////////////

        // ALU
        if ( (parsedInst.OpCode >= ADD && parsedInst.OpCode <= CMP) || parsedInst.OpCode == MV || (parsedInst.OpCode >= AND && parsedInst.OpCode <= RSHFT) ){

            // Check if there is any space in the RVs
            if (ALU_RV.size() >= MAX_RV_SIZE){
                // the ALU's RV is full and so we have to set the ID_I_Inst to BLOCKING
                IF_IDI[i].state = BLOCK;
                parsedInst.state = BLOCK;

                // Needed so that the PRF doesnt get filled with falsely invalid physical registers
                UndoDestinationRegisterValidity(&parsedInst);
                
                // NOTE: Blocking parsedInst wont really do much here but it is done to ensure behaviour follows in any case

            } else {
                // HERE THERE IS SPACE IN THE ALU_RV AND SO WE CAN ISSUE AN INSTRUCTION TO IT
                // Here we push out new parsed/decoded inst into the RV and we can set the previously undecoded instruction to EMPTY
                //parsedInst.state = NEXT;

                ALU_RV.push_back(std::pair<DecodedInstruction, int>(parsedInst, -1));

                IF_IDI[i].state = EMPTY;

                cout << "Loaded instruction " << parsedInst.asString << " into ALU_RV. Decoded instruction: "; parsedInst.print();
            }            
        }
        // BU
        else if (parsedInst.OpCode >= JMP && parsedInst.OpCode <= BZ || parsedInst.OpCode == HALT || parsedInst.OpCode == NOP) {
            
            // Check if there is any space in the RVs
            if (BU_RV.size() >= MAX_RV_SIZE){
                // the ALU's RV is full and so we have to set the ID_I_Inst to BLOCKING
                IF_IDI[i].state = BLOCK;
                parsedInst.state = BLOCK;

                // Needed so that the PRF doesnt get filled with falsely invalid physical registers
                UndoDestinationRegisterValidity(&parsedInst);

            } else {
                // HERE THERE IS SPACE IN THE ALU_RV AND SO WE CAN ISSUE AN INSTRUCTION TO IT
                // Here we push out new parsed/decoded inst into the RV and we can set the previously undecoded instruction to EMPTY
                //parsedInst.state = NEXT;

                BU_RV.push_back(std::pair<DecodedInstruction, int>(parsedInst, -1));

                IF_IDI[i].state = EMPTY;
                
                cout << "Loaded instruction " << parsedInst.asString << " into BU_RV. Decoded instruction: "; parsedInst.print();
            }       
        }

        // LSU
        else if (parsedInst.OpCode >= LD && parsedInst.OpCode <= STOA) {

             // Check if there is any space in the RVs
            if (LSU_RV.size() >= MAX_RV_SIZE){
                // the ALU's RV is full and so we have to set the ID_I_Inst to BLOCKING
                IF_IDI[i].state = BLOCK;
                parsedInst.state = BLOCK;

                // Needed so that the PRF doesnt get filled with falsely invalid physical registers
                UndoDestinationRegisterValidity(&parsedInst);

            } else {
                // HERE THERE IS SPACE IN THE ALU_RV AND SO WE CAN ISSUE AN INSTRUCTION TO IT
                // Here we push out new parsed/decoded inst into the RV and we can set the previously undecoded instruction to EMPTY
                //parsedInst.state = NEXT;

                LSU_RV.push_back(std::pair<DecodedInstruction, int>(parsedInst, -1));

                IF_IDI[i].state = EMPTY;

                cout << "Loaded instruction " << parsedInst.asString << " into LSU_RV. Decoded instruction: "; parsedInst.print();
            }
        }  
        else {
            throw std::invalid_argument("Could no issue an unknown instruction: " + ID_I_Inst.OpCode);
        }

    }

    // Any forwarded results are given to the instructions that are in the RVs
    ForwardResultsToRVs();
    // Offloads an instruciton from one RV into their respective EU
    OffLoadingReservationStations();

    // Now we can remove any writtenback entries in the RAW_Table
    HazardDetectionUnit->ClearRAW_Table();
}

// Executes the current instruction
void execute(){
    // Running and setting the debugging values for the execution of the ALU
    for (ALU* a : ALUs) {
        if (a->Out.state == EMPTY || a->Out.state == NEXT) {
            if (a->In.state == NEXT || a->In.state == BLOCK) {
                // Set state of the instruction
                a->In.state = CURRENT;

                // Debugging/GUI
                a->currentInst = a->In;

                // Actually run the bad boi
                a->cycle();
            } 
            else if (a->In.state == CURRENT) a->cycle();
            else if (a->In.state == EMPTY)  a->currentInst = DecodedInstruction();
        }
        else a->currentInst = DecodedInstruction();
    }

    // Running and setting the debugging values for the execution of the BU
    for (BU*  b : BUs ) {
        if (b->Out.state == EMPTY || b->Out.state == NEXT){
            if (b->In.state == NEXT || b->In.state == BLOCK){
                // Set state of the instruction
                b->In.state = CURRENT;

                // Debugging/GUI
                b->currentInst = b->In;

                // Actually run the bad boi
                b->cycle();
            }
            else if (b->In.state == CURRENT) b->cycle();
            else if (b->In.state == EMPTY) b->currentInst = DecodedInstruction();
        }
        else b->currentInst = DecodedInstruction();
    }
    
    // Running and setting the debugging values for the execution of the LSU
    for (LSU* l : LSUs) {
        if (l->Out.state == EMPTY || l->Out.state == NEXT){
            if (l->In.state == NEXT){
                // Set state of the instruction
                l->In.state = CURRENT;

                // Debugging/GUI
                l->currentInst = l->In;

                // Actually run the bad boi
                l->cycle();
            }
            else if (l->In.state == CURRENT) l->cycle();
            else if (l->In.state == EMPTY) l->currentInst = DecodedInstruction();
        }
        else l->currentInst = DecodedInstruction();
    } 
}


// Data written back into register file: Write backs don't occur on STO or HALT (or NOP)
void writeBack(){
    
    // Need to handle the correct writeback order so that WAWs dont occur

    WB_gui = DecodedInstruction();

    // unpack data from each ALU if there exists an instruciton in their output 
    for (ALU* a : ALUs){
        if (a->Out.state == NEXT || a->Out.state == BLOCK) {// a->Out.state == CURRENT){
            // It should be illegal for the instruction to be CURRENTLY running here but if it does then we would just ignore that write it back

            // We first set the state here to CURRENT out of principle (and just in case we exapnd this any further in the future)
            a->Out.state = CURRENT;

            std::cout << "Write back to index: " << a->Out.DEST << " with value: " << a->Out.OUT << std::endl;
            PhysRegisterFile.at(a->Out.DEST).first = a->Out.OUT;
            PhysRegisterFile.at(a->Out.DEST).second = true;
            //HazardDetectionUnit->SetWritebackFlag(a->Out);

            a->Out.state = EMPTY;

            // Debugging/GUI
            WB_gui = a->Out;
            WB_gui.state = NEXT;    // LEGIT ONLY USED TO MAKE THE GUI SHOW UP
        }
        else ;// Do Nothing as the instruction is EMPTY
    }

    // upack data from each BU, if there exists an instruction in their output
    for (BU* b : BUs){
        if (b->Out.state == NEXT || b->Out.state == BLOCK){// || b->Out.state == CURRENT){
            // It should be illegal for the instruction to be CURRENTLY running here but if it does then we would just ignore that write it back
            
            // We first set the state here to CURRENT out of principle (and just in case we exapnd this any further in the future)
            b->Out.state = CURRENT;

            // Set halt flag here
            systemHaltFlag = b->haltFlag;

            if (b->branchFlag) {
                PC = b->Out.OUT;

                flushPipeline(b->Out.sideOfBranch);
                std::cout << "@Write back. PC now equals: " << PC << std::endl;
            }
             
            b->Out.state = EMPTY;

            // Debugging/GUI
            WB_gui = b->Out;
            WB_gui.state = NEXT;    // LEGIT ONLY USED TO MAKE THE GUI SHOW UP
        }
    }

    // upack data from each BU, if there exists an instruction in their output
    for (LSU* l : LSUs){
        if (l->Out.state == NEXT || l->Out.state == BLOCK){// || l->Out.state == CURRENT){
            // It should be illegal for the instruction to be CURRENTLY running here but if it does then we would just ignore that write it back
            
            // We first set the state here to CURRENT out of principle (and just in case we exapnd this any further in the future)
            l->Out.state = CURRENT;


            if (l->Out.IsWriteBack) {
                PhysRegisterFile[l->Out.DEST].first = l->Out.OUT;
                PhysRegisterFile[l->Out.DEST].second = true;
                //HazardDetectionUnit->SetWritebackFlag(l->Out);
                std::cout << "Write back to index: " << l->Out.DEST << " with value: " << l->Out.OUT << std::endl;
            }

            l->Out.state = EMPTY;

            // Debugging/GUI
            WB_gui = l->Out;
            WB_gui.state = NEXT;    // LEGIT ONLY USED TO MAKE THE GUI SHOW UP
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
    if (count(args.begin(), args.end(), "--step") == 1) SINGLE_STEP_FLAG = true;

    return true;
}

#pragma endregion helperFunctions



int main(int argc, char** argv){
    if (!handleProgramFlags(argc, argv)) {
        std::cout << "Usage: ./isa <program_name> -r|m" << std::endl;
        return 0;
    }

    initialisePRF();
    loadProgramIntoMemory(argv[1]);
    run();

    // Clean up some pointers
    for (ALU* a : ALUs) delete a;
    for (BU*  b : BUs)  delete b;
    for (LSU* l : LSUs) delete l;

    return 0;
}