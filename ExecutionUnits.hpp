#include <iostream>
#include <stdexcept>
#include <string>

#include "EnumsAndConstants.hpp"

// General class for all Components
class ExecutionUnit{
    
    public:
        EUState state;
        std::string Inst = "EMPTY";

        bool writeBackFlag = false;

        DecodedInstruction In;
        DecodedInstruction Out;
    
    ExecutionUnit(){
        state = IDLE;
    }

    // Every component must be able to cycle
    void cycle(){
        return;
    }
};


// Implementation for an arithmetic logic unity (ALU)
class ALU : public ExecutionUnit{
    public:
    
    ALU(){

    }

    void cycle(){
        // set State
        //state = RUNNING;
        this->writeBackFlag = true;

        this->In.state = CURRENT;
        // Update the second destination register 
        Out.DEST = In.DEST;

        std::cout << "ALU cycle called" << std::endl;

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

    BU(){
    }

    void cycle(){
        // Set state to RUNNING
        
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


        this->Out.state = NEXT;
        this->In.state = EMPTY;
        
        // Debugging/GUI
        //this->Inst = "";
    }

};


// Implementation for a load/store unit (LSU)
class LSU : public ExecutionUnit{
    public:
        std::array<int, SIZE_OF_DATA_MEMORY>* memoryData;

    LSU(std::array<int, SIZE_OF_DATA_MEMORY>* memData){
        memoryData = memData;
    }

    void cycle(){
        // Set state to RUNNING

        this->In.state = CURRENT;

        std::cout << "LSU cycle called" << std::endl;
        switch(In.OpCode){
            case LD:
                Out.OUT = memoryData->at(In.IN0);
                Out.DEST = In.DEST;
                
                this->writeBackFlag = true;
                break;

            case LDD:
                Out.OUT = memoryData->at(In.IMM);
                Out.DEST = In.DEST;

                this->writeBackFlag = true;
                break;

            case LDI:                   // #####################
                Out.OUT = In.IMM;
                Out.DEST = In.DEST;
        
                this->writeBackFlag = true;
                std::cout << "LDI IS RUNNING HERE!!! this->writeBackFlag: " << this->writeBackFlag << std::endl;
                break;

            /*case LID:                   // BROKEN ################################
                registerFile[ALUD] = dataMemory[dataMemory[In.IN0]];
                break;*/
            case LDA:
                Out.OUT = memoryData->at(In.IN0 + In.IN1);
                Out.DEST = In.DEST;

                this->writeBackFlag = true;
                break;

            case STO:
                memoryData->at(In.DEST) = In.IN0;

                this->writeBackFlag = false;
                break;

            case STOI:                   // #####################
                memoryData->at(In.IMM) = In.IN0;

                this->writeBackFlag = false;
                break;

            default:
                throw std::invalid_argument("LSU cannot execute instruction: " + In.OpCode);
        }

        // Announce the fact that the instruction has been completed

        this->Out.state = NEXT;
        this->In.state = EMPTY;

        // Debugging/GUI
        //this->Inst = "";
    }
};


// Hazard Dection Unit (HDU) - used for deteciton RAW hazards
class HDU {
    public:
        std::vector<std::pair<Register, int>> RAW_Table;

    // NEED RO IMPLEMENT INSTRUCTIOSTATE!!!!
    /*InstructionState RAW(DecodedInstruction inst){

    }*/

    void loadInstInToRAW_Table(DecodedInstruction inst){
        // Check if inst.rd is already in the table
    }
};