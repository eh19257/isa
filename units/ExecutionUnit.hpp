#include <iostream>
#include <stdexcept>

#include "../EnumsAndConstants.hpp"

// General class for all Components
class ExecutionUnit{
    
    public:
        EUState state;
        Register OpCodeRegister;
    
    ExecutionUnit(){
        state = READY;
    }

    void multiplexer(){
        return;
    }

    // Every component must be able to cycle
    void cycle(){
        return;
    }
};

// Implementation for an arithmetic logic unity (ALU)
class ALU : public ExecutionUnit{

    public:
        int ALU0;
        int ALU1;
        int ALU_OUT;
    
    ALU(){
        //state = READY;//std::cout << state << std::endl;
        //std::cout << state << std::endl;
    }

    void cycle(){
        // set State
        state = RUNNING;

        switch(OpCodeRegister){
            case ADD:                   // #####################
                ALU_OUT = ALU0 + ALU1;   

                writeBackFlag = true;      
                break;

            case ADDI:
                ALU_OUT = ALU0 + IMMEDIATE;

                writeBackFlag = true;
                break;

            case SUB:
                ALU_OUT = ALU0 - ALU1;

                writeBackFlag = true;
                break;

            case MUL:
                ALU_OUT = ALU0 * ALU1;

                writeBackFlag = true;
                break;

            /*case MULO:
                long int tempResult = (long int) ALU0 * (long int) ALU1;    // Not a register, only used to simulate a multiplication w/ overflow
                long int HImask = (long int) (pow(2, 32) - 1) << 32;        // Again, not a register, only used to simulated multiplication w/ overflow

                HI = (int) ( (tempResult & HImask) >> 32);
                LO = (int) tempResult;
                break;*/

            case DIV:
                ALU_OUT = (int) ALU0 / ALU1;

                writeBackFlag = true;
                break;

            case CMP:
                if      (ALU0 < ALU1) ALU_OUT = -1;
                else if (ALU0 > ALU1) ALU_OUT =  1;
                else if (ALU0 == ALU1)ALU_OUT =  0;
            
                writeBackFlag = true;
                break;

            case AND:
                ALU_OUT = ALU0 & ALU1;

                writeBackFlag = true;
                break;
            case OR:
                ALU_OUT = ALU0 | ALU1;

                writeBackFlag = true;
                break;
            case NOT:
                ALU_OUT = ~ALU0;

                writeBackFlag = true;
                break;
            case LSHFT:
                ALU_OUT = ALU0 << ALU1;

                writeBackFlag = true;
                break;
            case RSHFT:
                ALU_OUT = ALU0 >> ALU1;

                writeBackFlag = true;
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

    void cycle(){
        switch(OpCodeRegister){
            case JMP:
            PC = registerFile[ALUD];      // Again as in STO, is accessing the register file at this point illegal?

            branchFlag = true;

            /* STATS */ numOfBranches++;
            cout << "BRANCH" << endl;
            break;

        case JMPI:
            PC = PC + registerFile[ALUD]; // WARNING ERROR HERE

            branchFlag = true;
            
            /* STATS */ numOfBranches++;
            cout << "BRANCH" << endl;
            break;

        case BNE:
            if (ALU0 < 0) {
                PC = registerFile[ALUD];
                
                branchFlag = true;

                /* STATS */ numOfBranches++;
                cout << "BRANCH" << endl;
            }
            break;

        case BPO:
            if (ALU0 > 0) {
                PC = registerFile[ALUD];
                
                branchFlag = true;

                /* STATS */ numOfBranches++;
                cout << "BRANCH" << endl;
            }
            break;

        case BZ:
            if (ALU0 == 0) {
                PC = registerFile[ALUD];
                
                branchFlag = true;

                /* STATS */ numOfBranches++;
                cout << "BRANCH" << endl; 
            }
            break;
        
        default:
            throw std::invalid_argument("BU cannot execute instruction: " + OpCodeRegister);
        }
    }

};


// Implementation for a load/store unit (LSU)
class LSU : public ExecutionUnit{
    public:

    void cycle(){
        switch(OpCodeRegister){
            case LD:
            ALU_OUT = ALU0;
            
            writeBackFlag = true;
            memoryReadFlag = true;
            break;
        case LDD:
            ALU_OUT = IMMEDIATE;
            
            writeBackFlag = true;
            memoryReadFlag = true;
            break;
        case LDI:                   // #####################
            ALU_OUT = IMMEDIATE;
            
            writeBackFlag = true;
            break;
        /*case LID:                   // BROKEN ################################
            registerFile[ALUD] = dataMemory[dataMemory[ALU0]];
            break;*/
        case LDA:
            ALU_OUT = ALU0 + ALU1;

            writeBackFlag = true;
            memoryReadFlag = true;
            break;
        case STO:
            ALU_OUT = ALU0;
            WBD = registerFile[ALUD];       // register file access here might be invalid - ask Simon and see what he says

            memoryWriteFlag = true;
            break;
        case STOI:                   // #####################
            ALU_OUT = ALU0;
            WBD = IMMEDIATE;

            memoryWriteFlag = true;
            break;

        default:
            throw std::invalid_argument("LSU cannot execute instruction: " + OpCodeRegister);

        }
    }
};