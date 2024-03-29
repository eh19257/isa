#include <iostream>
#include <stdexcept>
#include <string>

#include "EnumsAndConstants.hpp"

// General class for all Components
class ExecutionUnit{
    
    public:
        EUState state;
        std::string typeOfEU = "DefaultEU";

        bool writeBackFlag = false;

        Instruction OpCodeRegister;

        int IN0;
        int IN1;
        int IMMEDIATE;

        int DEST;
        int DEST_OUT;       // We need 2 destination registers - one between I/EX and one between EX/C
        int OUT;
    
    ExecutionUnit(){
        state = IDLE;
    }

    /*void multiplexer(){
        return;
    }*/

    // Every component must be able to cycle
    void cycle(){
        return;
    }
};


// Implementation for an arithmetic logic unity (ALU)
class ALU : public ExecutionUnit{
    public:
    
    ALU(){
        typeOfEU = "ALU";
        writeBackFlag = true;
        //state = READY;//std::cout << state << std::endl;
        //std::cout << state << std::endl;
    }

    void cycle(){
        // set State
        state = RUNNING;

        // Update the second destination register 
        DEST_OUT = DEST;

        std::cout << "ALU cycle called" << std::endl;

        switch(OpCodeRegister){
            case ADD:                   // #####################
                OUT = IN0 + IN1;;   

                //writeBackFlag = true;      
                break;

            case ADDI:
                OUT = IN0 + IMMEDIATE;

                //writeBackFlag = true;
                break;

            case SUB:
                OUT = IN0 - IN1;

                //writeBackFlag = true;
                break;

            case MUL:
                OUT = IN0 * IN1;

                //writeBackFlag = true;
                break;

            /*case MULO:
                long int tempResult = (long int) IN0 * (long int) IN1;    // Not a register, only used to simulate a multiplication w/ overflow
                long int HImask = (long int) (pow(2, 32) - 1) << 32;        // Again, not a register, only used to simulated multiplication w/ overflow

                HI = (int) ( (tempResult & HImask) >> 32);
                LO = (int) tempResult;
                break;*/

            case DIV:
                OUT = (int) IN0 / IN1;

                //writeBackFlag = true;
                break;

            case CMP:
                if      (IN0 < IN1) OUT = -1;
                else if (IN0 > IN1) OUT =  1;
                else                OUT =  0;
            
                //writeBackFlag = true;
                break;

            case AND:
                OUT = IN0 & IN1;

                //writeBackFlag = true;
                break;
            case OR:
                OUT = IN0 | IN1;

                //writeBackFlag = true;
                break;
            case NOT:
                OUT = ~IN0;

                //writeBackFlag = true;
                break;
            case LSHFT:
                OUT = IN0 << IN1;

                //writeBackFlag = true;
                break;
            case RSHFT:
                OUT = IN0 >> IN1;

                //writeBackFlag = true;
                break;
            
            default:
                throw std::invalid_argument("ALU cannot execute instruction: " + OpCodeRegister);
                break;
        }

        state = DONE;
    }
};


// Implementaiton for a Branch unity (BU)
class BU : public ExecutionUnit{

    public:
        bool branchFlag = false;    // True is there is going to be a branch - default = no branch

    BU(){
        typeOfEU = "BU";
    }

    void cycle(){
        // Set state to RUNNING
        state = RUNNING;

        std::cout << "BU cycle called" << std::endl;
        switch(OpCodeRegister){
            case JMP:
            OUT = DEST;//registerFile[BUD];      // Again as in STO, is accessing the register file at this point illegal?

            branchFlag = true;

            ///* STATS */ numOfBranches++;
            //cout << "BRANCH" << endl;
            break;

        case JMPI:
            OUT = OUT + DEST;//registerFile[BUD]; // WARNING ERROR HERE

            branchFlag = true;
            
            ///* STATS */ numOfBranches++;
            //cout << "BRANCH" << endl;
            break;

        case BNE:
            if (IN0 < 0) {
                OUT = DEST;//PC = registerFile[BUD];
                
                branchFlag = true;

                ///* STATS */ numOfBranches++;
                //cout << "BRANCH" << endl;
            }
            break;

        case BPO:
            if (IN0 > 0) {
                OUT = DEST;//PC = registerFile[BUD];
                
                branchFlag = true;

                ///* STATS */ numOfBranches++;
                //cout << "BRANCH" << endl;
            }
            break;

        case BZ:
            if (IN0 == 0) {
                OUT = DEST;//PC = registerFile[BUD];
                
                branchFlag = true;

                ///* STATS */ numOfBranches++;
                //cout << "BRANCH" << endl; 
            }
            break;
        
        default:
            throw std::invalid_argument("BU cannot execute instruction: " + OpCodeRegister);
        }

        state = DONE;
    }

};


// Implementation for a load/store unit (LSU)
class LSU : public ExecutionUnit{
    public:
        std::array<int, SIZE_OF_DATA_MEMORY>* memoryData;

    LSU(std::array<int, SIZE_OF_DATA_MEMORY>* memData){
        memoryData = memData;
        typeOfEU = "LSU";
    }

    void cycle(){
        // Set state to RUNNING
        state = RUNNING;

        std::cout << "LSU cycle called" << std::endl;
        switch(OpCodeRegister){
            case LD:
                OUT = memoryData->at(IN0);
                DEST_OUT = DEST;
                
                writeBackFlag = true;
                break;

            case LDD:
                OUT = memoryData->at(IMMEDIATE);
                DEST_OUT = DEST;

                writeBackFlag = true;
                break;

            case LDI:                   // #####################
                OUT = IMMEDIATE;
                DEST_OUT = DEST;

                writeBackFlag = true;
                break;

            /*case LID:                   // BROKEN ################################
                registerFile[ALUD] = dataMemory[dataMemory[IN0]];
                break;*/
            case LDA:
                OUT = memoryData->at(IN0 + IN1);
                DEST_OUT = DEST;

                writeBackFlag = true;
                break;

            case STO:
                memoryData->at(DEST) = IN0;
                break;

            case STOI:                   // #####################
                memoryData->at(IMMEDIATE) = IN0;
                break;

            default:
                throw std::invalid_argument("LSU cannot execute instruction: " + OpCodeRegister);
        }

        // Announce the fact that the instruction has been completed
        state = DONE;
    }
};

class MISC : public ExecutionUnit{

    MISC(){
        typeOfEU = "MISC";
    }

    void cycle(){
        // Set state to RUNNING
        state = RUNNING;
        std::cout << "NOT IMPLEMENTED MISC EU YET" << std::endl;
        
        state = DONE;
    }
};