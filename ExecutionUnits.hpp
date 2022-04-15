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
    private:
        std::vector<std::pair<Register, Optional<int>>> RAW_Table;

        // Returns true if there is a RAW clash
        bool checkForRegisterClashInRAWTable(std::vector<std::pair<Register, Optional<int>>>* RAW, int* val, Register* reg){
            // Search through the RAW table for any destination regsiters that are in it
            for (std::pair<Register, Optional<int>> entry : *RAW){
                // Here we check if rs0 is trying to read from a rd that hasnt been fully updated yet
                if (entry.first == *reg){
                    if (entry.second.HasValue()){
                        // The destination register has been found in the RAW_table but the result has been forwarded
                        *val = entry.second.Value();
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
            if (inst.IsWriteBack == true){
                RAW_Table.push_back(std::pair<Register, Optional<int>>(inst.rd, Optional<int>()));
            }
        }

    public:
        DecodedInstruction checkForRAW(DecodedInstruction inst){
            Register rs0, rs1;

            bool Isrs0Block = checkForRegisterClashInRAWTable(&RAW_Table, &inst.IN0, &inst.rs0);
            bool Isrs1Block = checkForRegisterClashInRAWTable(&RAW_Table, &inst.IN1, &inst.rs1);

            // If either one of these are blocking then we set inst to BLOCK else it's ready to move on
            if (Isrs0Block || Isrs1Block) {
                inst.state = BLOCK;
            } else {
                inst.state = NEXT;

                loadInstInToRAW_Table(inst);
            }

            return inst;
        }

        void LoadDestinationValueIntoRAWTable(DecodedInstruction inst){

        }


        HDU(){

        }
};




// General class for all Components
class ExecutionUnit{
    
    public:
        EUState state;
        std::string Inst = "EMPTY";

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
        // Update output's writeback status
        this->Out.IsWriteBack = this->In.IsWriteBack;

        this->In.state = CURRENT;

        // Move all info over to the output register 
        this->Out = this->In;

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
        this->HazardDetectionUnit->LoadDestinationValueIntoRAWTable(this->Out);

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
        // Update output's writeback status
        this->Out.IsWriteBack = this->In.IsWriteBack;

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
            break;
        
        default:
            throw std::invalid_argument("BU cannot execute instruction: " + In.OpCode);
        }

        // Load the output into the correct row in the RAW table
        this->HazardDetectionUnit->LoadDestinationValueIntoRAWTable(this->Out);

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
        // Update output's writeback status
        this->Out.IsWriteBack = this->In.IsWriteBack;

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
                if (ldiCount < 3) return;

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

            default:
                throw std::invalid_argument("LSU cannot execute instruction: " + In.OpCode);
        }

        // Load the output into the correct row in the RAW table
        this->HazardDetectionUnit->LoadDestinationValueIntoRAWTable(this->Out);

        this->Out.state = NEXT;
        this->In.state = EMPTY;

        // Debugging/GUI
        //this->Inst = "";
    }
};