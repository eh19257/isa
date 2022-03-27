#include <iostream>
#include <stdexcept>
#include <string>

#include "EnumsAndConstants.hpp"

// General class for all Components
class ExecutionUnit{
    
    public:
        EUState state;
        std::string typeOfEU = "DefaultEU";

        Instruction OpCodeRegister;

        int IN0;
        int IN1;
        int IMMEDIATE;

        int OUT;
    
    ExecutionUnit(){
        state = READY;
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
        //state = READY;//std::cout << state << std::endl;
        //std::cout << state << std::endl;
    }

    void cycle(){
        // set State
        state = RUNNING;

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
        int BUD;            // Destination address for the jump
        int BU0;            // Input for the BU

        bool BranchFlag = false;    // True is there is going to be a branch - default = no branch

    BU(){
        typeOfEU = "BU";
    }

    void cycle(){
        switch(OpCodeRegister){
            case JMP:
            PC = registerFile[BUD];      // Again as in STO, is accessing the register file at this point illegal?

            branchFlag = true;

            ///* STATS */ numOfBranches++;
            cout << "BRANCH" << endl;
            break;

        case JMPI:
            PC = PC + registerFile[BUD]; // WARNING ERROR HERE

            branchFlag = true;
            
            ///* STATS */ numOfBranches++;
            cout << "BRANCH" << endl;
            break;

        case BNE:
            if (BU0 < 0) {
                PC = registerFile[BUD];
                
                branchFlag = true;

                ///* STATS */ numOfBranches++;
                cout << "BRANCH" << endl;
            }
            break;

        case BPO:
            if (BU0 > 0) {
                PC = registerFile[BUD];
                
                branchFlag = true;

                ///* STATS */ numOfBranches++;
                cout << "BRANCH" << endl;
            }
            break;

        case BZ:
            if (BU0 == 0) {
                PC = registerFile[BUD];
                
                branchFlag = true;

                ///* STATS */ numOfBranches++;
                cout << "BRANCH" << endl; 
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
        int LSU0;
        int LSU1;
        int LSUD;
        
        int LSU_OUT;

    LSU(){
        typeOfEU = "LSU";
    }

    void cycle(){
        switch(OpCodeRegister){
            case LD:
            OUT = IN0;
            
            writeBackFlag = true;
            memoryReadFlag = true;
            break;
        case LDD:
            OUT = IMMEDIATE;
            
            writeBackFlag = true;
            memoryReadFlag = true;
            break;
        case LDI:                   // #####################
            OUT = IMMEDIATE;
            
            writeBackFlag = true;
            break;
        /*case LID:                   // BROKEN ################################
            registerFile[ALUD] = dataMemory[dataMemory[IN0]];
            break;*/
        case LDA:
            OUT = IN0 + IN1;

            writeBackFlag = true;
            memoryReadFlag = true;
            break;
        case STO:
            OUT = IN0;
            WBD = registerFile[ALUD];       // register file access here might be invalid - ask Simon and see what he says

            memoryWriteFlag = true;
            break;
        case STOI:                   // #####################
            OUT = IN0;
            WBD = IMMEDIATE;

            memoryWriteFlag = true;
            break;

        default:
            throw std::invalid_argument("LSU cannot execute instruction: " + OpCodeRegister);

        }

        state = DONE;
    }
};

class MISC : public ExecutionUnit{

    MISC(){
        typeOfEU = "MISC";
    }

    void cycle(){
        std::cout << "NOT IMPLEMENTED MISC EU YET" << std::endl;
        
        state = DONE;
    }
};