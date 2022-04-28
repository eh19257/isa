#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

#include "EnumsAndConstants.hpp"

/* Class prototypes */
class ExecutionUnit;
class ALU;
class BU;
class LSU;
class HDU;



// Hazard Dection Unit (HDU) - used for deteciton RAW hazards
class HDU {
    /*
    private:
        std::vector<std::tuple<Register, Optional<int>, bool>> RAW_Table;

        // Returns true if there is a RAW clash
        bool checkForRegisterClashInRAWTable(std::vector<std::tuple<Register, Optional<int>, bool>>* RAW, int* val, Register* reg){
            // returns false in the case an X is found - X is a dummy register that is only used to make the instructions follow a 3 operand form
            if (*reg == -1) {
                std::cout << "DUMMY REG X DETECTED" << std::endl;
                return false;
            }
            // Search through the RAW table for any destination regsiters that are in it
            for (std::tuple<Register, Optional<int>, bool> entry : *RAW){
                // Here we check if rs0 is trying to read from a rd that hasnt been fully updated yet
                if (std::get<0>(entry) == *reg){
                    if (std::get<1>(entry).HasValue()){
                        // The destination register has been found in the RAW_table and the result has been forwarded
                        *val = std::get<1>(entry).Value();
                        std::cout << "Value found in RAW table: " << *val << std::endl;
                        return false;

                    } else {
                        // The result hasnt been written back and so we need to block this instruction
                        return true;
                    }

                }
            }
            // if the rd isn't found in the RAW table then there is no RAW hazard
            return false;
        }
        
        void loadInstInToRAW_Table(DecodedInstruction inst){
            // Check if the inst is a write back instruction
            if (inst.IsWriteBack == true && inst.rd != -1){
                std::cout << "entry in RAW TABLE create for: " << inst.asString << std::endl;
                RAW_Table.push_back( std::make_tuple(inst.rd, Optional<int>(), false) );
            }
        }

    public:

        void ClearRAWTable(){
            this->RAW_Table = std::vector<std::tuple<Register, Optional<int>, bool>>();
        }

        void printRAW(){
            std::cout << "RAW table:\n\tRegister\t\trd" << std::endl;
                for (int i = 0; i < RAW_Table.size(); i++){
                    std::cout << i << "\t" << std::get<0>(RAW_Table.at(i)) << "\t";
                    if (std::get<1>(RAW_Table.at(i)).HasValue()){
                        std::cout << std::get<1>(RAW_Table.at(i)).Value();
                    }
                    std::cout << std::endl;
                } 
                std::cout << "=== RAW table end ==="<< std::endl;
        }


        // Checks if the current instruction is dependent on a write, it also loads the instruction into the RAW table if it also could cause a RAW hazard
        DecodedInstruction CheckForRAWAndLoad(DecodedInstruction inst){
            inst = CheckForRAW(inst);
            loadInstInToRAW_Table(inst);

            return inst;
        }

        // Used when an instruction is in the RVs, checks BLOCKED instructions for forwarded values and then outputs the updated instruction
        DecodedInstruction CheckForRAW(DecodedInstruction inst){

            bool Isrs0Block = checkForRegisterClashInRAWTable(&RAW_Table, &inst.IN0, &inst.rs0);
            bool Isrs1Block = checkForRegisterClashInRAWTable(&RAW_Table, &inst.IN1, &inst.rs1);

            // If either one of these are blocking then we set inst to BLOCK else it's ready to move on
            if (Isrs0Block || Isrs1Block) {
                inst.state = BLOCK;
                std::cout << "RAW hazard detected for instruction: " << inst.asString << " Isrs0Block: " << Isrs0Block << " Isrs1Block: " << Isrs1Block << std::endl;
            } else {
                inst.state = NEXT;               
            }

            return inst;
        }

        // Loads the result of the instruction into its respective entry within the RAW_Table
        void LoadDestinationValueIntoRAWTable(DecodedInstruction inst){
            for (int i = 0; i < RAW_Table.size(); i++){
                if (std::get<0>(RAW_Table.at(i)) == inst.rd){

                    RAW_Table.at(i) = std::make_tuple(std::get<0>(RAW_Table.at(i)), Optional<int>(inst.OUT), false);
                    //RAW_Table.at(i).second.Value(inst.OUT);
                    return;
                }
            }
        }

        // Removes any written back values - i.e. values that have the 3rd part of the RAW_Table entry = true
        void RemoveAnyWrittenBackValues(){
            for (int i = 0; i < RAW_Table.size(); i++){
                // If this rd has been written back - then we can remove it from the table
                if (std::get<2>(RAW_Table.at(i))){
                    RAW_Table.erase(RAW_Table.begin() + i);
                }
            }
        }

        // When an instruction is done with writeback, it then flags this entry in the RAW Table as "writtenBack" at the end of this clock cycle it will be removed from the table using RemoveAnyWrittenBackValues()
        void SetWritebackFlag(DecodedInstruction inst){
            for (int i = 0; i < RAW_Table.size(); i++){
                if (std::get<0>(RAW_Table.at(i)) == inst.rd){
                    RAW_Table.at(i) = std::make_tuple(std::get<0>(RAW_Table.at(i)), std::get<1>(RAW_Table.at(i)), true);
                }
            }
        }
        */
    
    private:
        std::vector<std::pair<int, int>> RAW_Table;

        // returns true is reg is still waiting (respective instruction should be blocked)
        bool CheckRAW_TableForIndividualValues(int* reg, int* val){
            if (*reg == -1) return false;

            for (int i = 0; i < RAW_Table.size(); i++){
                if (*reg == RAW_Table.at(i).first){
                    *val = RAW_Table.at(i).second;

                    return false;
                }
            }
            return true;
        }
    
    public:

        HDU(){

        }

        // Returns false if no entry for reg was found in the RAW table
        bool CheckRAW_TableForForwardedValues(int* reg, int* val){
            if (*reg == -1) return true;

            for (int i = 0; i < RAW_Table.size(); i++){
                if (*reg == RAW_Table.at(i).first) {
                    *val = RAW_Table.at(i).second;
                    return true;
                }
            }
            // if the entry was not found in the RAW table then we know that this register is not valid to use yet so we return false
            return false;
        }
    
        void ForwardResult(DecodedInstruction inst){
            if (inst.IsWriteBack) RAW_Table.push_back(std::pair<int, int>(inst.DEST, inst.OUT));
        }

        void ClearRAW_Table(){
            RAW_Table = std::vector<std::pair<int, int>>();
        }
};




// General class for all Components
class ExecutionUnit{
    
    public:
        std::string Inst = "EMPTY";
        DecodedInstruction currentInst;

        DecodedInstruction In;
        DecodedInstruction Out;

        HDU* HazardDetectionUnit;
    
    ExecutionUnit(HDU* haz) {
        HazardDetectionUnit = haz;
        //state = IDLE;
    }

    // Every component must be able to cycle
    void cycle(){
        return;
    }
};


// Implementation for an arithmetic logic unity (ALU)
class ALU : public ExecutionUnit{
    public:
    
    ALU(HDU* haz) : ExecutionUnit(haz){

    }

    void cycle(){
        // Move all info over to the output register and set it being empty so that it can be loaded with info
        this->Out = this->In;
        
        // Update the stats of In and Out such that the EUs can run without getting interference with the states
        this->Out.state = EMPTY;
        this->In.state = CURRENT;

        std::cout << "ALU cycle called" << std::endl;

        // Now properly update the output register such that the output values are the correct, calculated values
        switch(In.OpCode){
            case ADD:                   // #####################
                Out.OUT = In.IN0 + In.IN1;;     
                break;

            case ADDI:
                Out.OUT = In.IN0 + In.IMM;
                break;

            case SUB:
                Out.OUT = In.IN0 - In.IN1;
                break;

            case MUL:
                Out.OUT = In.IN0 * In.IN1;
                break;

            /*case MULO:
                long int tempResult = (long int) In.IN0 * (long int) In.IN1;    // Not a register, only used to simulate a multiplication w/ overflow
                long int HImask = (long int) (pow(2, 32) - 1) << 32;        // Again, not a register, only used to simulated multiplication w/ overflow

                HI = (int) ( (tempResult & HImask) >> 32);
                LO = (int) tempResult;
                break;*/

            case DIV:
                Out.OUT = (int) In.IN0 / In.IN1;
                break;

            case CMP:
                if      (In.IN0 < In.IN1) Out.OUT = -1;
                else if (In.IN0 > In.IN1) Out.OUT =  1;
                else                Out.OUT =  0;
                break;

            case AND:
                Out.OUT = In.IN0 & In.IN1;
                break;
            case OR:
                Out.OUT = In.IN0 | In.IN1;
                break;
            case NOT:
                Out.OUT = ~In.IN0;
                break;
            case LSHFT:
                Out.OUT = In.IN0 << In.IN1;
                break;
            case RSHFT:
                Out.OUT = In.IN0 >> In.IN1;
                break;
            
            case MV:
                Out.OUT = In.IN0;
                break;
            /*
            case MVHI:
                Out.OUT = HI;
                Out.OUT = In.DEST;
                this->writeBackFlag = true;
                break;
            case MVLO:
                ALU_OUT = LO;
                MEMD = ALUD;
                MEM_writeBackFlag = true;
                break;
            */
            
            default:
                throw std::invalid_argument("ALU cannot execute instruction: " + In.OpCode);
                break;
        }

        // Load the output into the correct row in the RAW table
        this->HazardDetectionUnit->ForwardResult(this->Out);

        this->Out.state = NEXT;
        this->In.state = EMPTY;

        // Debugging/GUI
        //this->Inst = "";
    }
};


// Implementaiton for a Branch unity (BU)
class BU : public ExecutionUnit{

    public:
        bool branchFlag = false;    // True is there is going to be a branch - default = no branch
        bool haltFlag = false;

    BU(HDU* haz) : ExecutionUnit(haz){
    }

    void cycle(){
        // Move all info over to the output register and set it being empty so that it can be loaded with info
        this->Out = this->In;
        
        // Update the stats of In and Out such that the EUs can run without getting interference with the states
        this->Out.state = EMPTY;
        this->In.state = CURRENT;

        branchFlag = false;

        switch(In.OpCode){
            case JMP:
            Out.OUT = In.DEST;//registerFile[BUD];      // Again as in STO, is accessing the register file at this point illegal?

            branchFlag = true;

            ///* STATS */ numOfBranches++;
            //cout << "BRANCH" << endl;
            break;

        case JMPI:
            Out.OUT = In.OUT + In.DEST;//registerFile[BUD]; // WARNING ERROR HERE

            branchFlag = true;
            
            ///* STATS */ numOfBranches++;
            //cout << "BRANCH" << endl;
            break;

        case BNE:
            if (In.IN0 < 0) {
                Out.OUT = In.DEST;//PC = registerFile[BUD];
                
                branchFlag = true;

                ///* STATS */ numOfBranches++;
                //cout << "BRANCH" << endl;
            }
            break;

        case BPO:
            if (In.IN0 > 0) {
                Out.OUT = In.DEST;//PC = registerFile[BUD];
                
                branchFlag = true;

                ///* STATS */ numOfBranches++;
                //cout << "BRANCH" << endl;
            }
            break;

        case BZ:
            if (In.IN0 == 0) {
                Out.OUT = In.DEST;//PC = registerFile[BUD];
                
                branchFlag = true;

                ///* STATS */ numOfBranches++;
                //cout << "BRANCH" << endl; 
            }
            break;
        
        case HALT:                   // #####################
            this->haltFlag = true;
            break;

        case NOP:
            std::cout << "NOP EXECUTED!!!" << std::endl;
            //int foo;
            //std::cin >> foo;
            break;
        
        default:
            throw std::invalid_argument("BU cannot execute instruction: " + In.OpCode);
        }

        // Load the output into the correct row in the RAW table
        this->HazardDetectionUnit->ForwardResult(this->Out);

        this->Out.state = NEXT;
        this->In.state = EMPTY;
        
        // Debugging/GUI
        //this->Inst = "";
    }

};


// Implementation for a load/store unit (LSU)
class LSU : public ExecutionUnit{
     private:
        int ldiCount = 0;
        
    public:
        std::array<int, SIZE_OF_DATA_MEMORY>* memoryData;

    LSU(HDU* haz, std::array<int, SIZE_OF_DATA_MEMORY>* memData) : ExecutionUnit(haz){
        memoryData = memData;
    }

    void cycle(){
        // Move all info over to the output register and set it being empty so that it can be loaded with info
        this->Out = this->In;
        
        // Update the stats of In and Out such that the EUs can run without getting interference with the states
        this->Out.state = EMPTY;
        this->In.state = CURRENT;

        std::cout << "LSU cycle called" << std::endl;

        switch(In.OpCode){
            case LD:
                Out.OUT = memoryData->at(In.IN0);
                Out.DEST = In.DEST;
                
                //this->writeBackFlag = true;
                break;

            case LDD:
                Out.OUT = memoryData->at(In.IMM);
                Out.DEST = In.DEST;

                //this->writeBackFlag = true;
                break;

            case LDI:                   // #####################
                ldiCount++;
                if (ldiCount < 0) return;

                ldiCount = 0;

                Out.OUT = In.IMM;
                Out.DEST = In.DEST;
        
                //this->writeBackFlag = true;
                //std::cout << "LDI IS RUNNING HERE!!! this->writeBackFlag: " << this->Out.IsWriteBack << std::endl;
                break;

            /*case LID:                   // BROKEN ################################
                registerFile[ALUD] = dataMemory[dataMemory[In.IN0]];
                break;*/
            case LDA:
                Out.OUT = memoryData->at(In.IN0 + In.IN1);
                Out.DEST = In.DEST;

                //this->writeBackFlag = true;
                break;

            case STO:
                memoryData->at(In.DEST) = In.IN0;

                //this->writeBackFlag = false;
                break;

            case STOI:                   // #####################
                memoryData->at(In.IMM) = In.IN0;

                //this->writeBackFlag = false;
                break;
            
            case STOA:
                memoryData->at(In.DEST + In.IN0) = In.IN1;

                break;

            default:
                throw std::invalid_argument("LSU cannot execute instruction: " + In.OpCode);
        }

        // Load the output into the correct row in the RAW table
        this->HazardDetectionUnit->ForwardResult(this->Out);

        this->Out.state = NEXT;
        this->In.state = EMPTY;

        // Debugging/GUI
        //this->Inst = "";
    }
};