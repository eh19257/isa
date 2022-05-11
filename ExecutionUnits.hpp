#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>
#include <deque>

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

        void RemoveFromRAWTable(DecodedInstruction inst){
            if (inst.IsWriteBack && inst.rd != -1){
                for (int i = 0; i < RAW_Table.size(); i++){
                    // Here we find and remove it from the table
                    if (RAW_Table.at(i).first == inst.rd){
                        RAW_Table.erase(RAW_Table.begin() + i);
                    }
                }
            }
        }

        void ClearRAW_Table(){
            RAW_Table = std::vector<std::pair<int, int>>();
        }
};


class ROB {
    public:
        // deque used as it is like a circular buffer - DecodedInstruction stores, info about the RD, status of the instruction and the instruction itself
        //                                            - Optional<int> stores the value (if it exists) of the register
        std::deque<std::pair<DecodedInstruction, Optional<int>>> ReorderBuffer;


        // returns true if the result was successfully loaded into the ROB, returns false if not (note: it should NEVER return false)
        bool LoadCompletedInstructionIntoROB(DecodedInstruction inst){
            for (int i = 0; i < ReorderBuffer.size(); i++){
                std::pair<DecodedInstruction, Optional<int>> entry = ReorderBuffer.at(i);

                // If the instruction matches
                if (entry.first.uniqueInstructionIdentifer == inst.uniqueInstructionIdentifer){
                    //std::cout << "FOUND IN ROB: "; inst.printHuman(); std::cout << std::endl;
                    ReorderBuffer.at(i).first = inst;

                    // handle for when an instruction is writeback
                    if (entry.first.IsWriteBack && !entry.first.IsMemoryOperation){
                        ReorderBuffer.at(i).second.Value(inst.OUT);
                    }

                    // If it's a memory operation then it hasnt technically completed yet, we need to actually do the memory stuff before we even think about using these values
                    if (ReorderBuffer.at(i).first.IsMemoryOperation) {
                        ReorderBuffer.at(i).first.state = CURRENT;
                    } else {
                        ReorderBuffer.at(i).first.state = NEXT;
                    }

                    return true;
                }
            }
            throw std::invalid_argument("Instruction: " + inst.asString + " is not found in the ROB -- THIS IS ILLEGAL");
            return false;
        }

        // returns true if it was successfully loaded into the ROB, else returns false
        bool LoadInstructionIntoTheROB(DecodedInstruction inst){
            if (ReorderBuffer.size() >= MAX_NUMBER_OR_ROB_ENTRIES) {
                return false;
            } else {
                if (inst.IsMemoryOperation){
                    inst.state = BLOCK;
                } else {
                    inst.state = CURRENT;
                }
                
                // Position the inst in the correct place in the ROB
                for (int i = 0; i < ReorderBuffer.size(); i++){
                    if (ReorderBuffer.at(i).first.uniqueInstructionIdentifer > inst.uniqueInstructionIdentifer){
                        ReorderBuffer.insert(ReorderBuffer.begin() + i, std::pair<DecodedInstruction, Optional<int>>(inst, Optional<int>()));
                        return true;
                    }
                }
                // If we cannot find a specific place to insert the instuction, then we push it onto the back
                ReorderBuffer.push_back(std::pair<DecodedInstruction, Optional<int>>(inst, Optional<int>()));
                return true;
            }
        }
    
         // Returns false if an an entry was found in the ROB and it has not completed excution
        bool CheckROBForForwardedValues(int* reg, int* val, int uniqueIdentifierForCurrentInst){
            for (int i = 0; i < ReorderBuffer.size(); i++){
                if (ReorderBuffer.at(i).first.uniqueInstructionIdentifer == uniqueIdentifierForCurrentInst){
                    // This has to be done so we dont get an instruction waiting for itself
                    continue;
                }
                
                if (ReorderBuffer.at(i).first.rd == *reg && ReorderBuffer.at(i).first.IsWriteBack){
                    if (ReorderBuffer.at(i).first.state == NEXT && ReorderBuffer.at(i).second.HasValue()){
                        
                        // Update the value and then return true (it has been found and successfully updated and therefore it is valid)
                        int foo = ReorderBuffer.at(i).second.Value();
                        //std::cout << "The register " << *reg << " should have the new value "  << foo << std::endl;
                        
                        *val = foo;
                        return true;

                    } else {
                        //std::cout << *reg << " and val: " << *val << " are clashing with the instruction "; ReorderBuffer.at(i).first.printHuman();
                        //std::cout << std::endl;
                        //std::cout << "We actually get NO result" << std::endl;
                        return false;
                    }
                }
            }
            //std::cout << "RETURNED TRUE!!!" << std::endl;
            return true;
        }


        // Similar to the above but it returns false if it isn't found in the reg
        bool IsRegInROB(int* reg, int* val, int uniqueIdentifierForCurrentInst){
            int tempVal = *val;
            bool IsFowardedValueInROB = this->CheckROBForForwardedValues(reg, val, uniqueIdentifierForCurrentInst);

            if (IsFowardedValueInROB && tempVal == *val){
                return false;
            }
            return IsFowardedValueInROB;
        }

        DecodedInstruction BlockInstructionIfNotIsWriteBack(DecodedInstruction inst){
            // If this is a write back instruction then we can ignore this special case and use the ROB as it was intended
            /*if (!inst.IsWriteBack) {
                // If this instrucion isn't a writeback, then we must check that no other !isWriteBack instruction is going to 
                // -1 so we dont include the current instruction
                for (int i = 0; i < ReorderBuffer.size() - 1 && ReorderBuffer.at(i).first.uniqueInstructionIdentifer != inst.uniqueInstructionIdentifer; i++){
                    if (ReorderBuffer.at(i).first.IsBranchInst){
                        inst.state = BLOCK;
                        break;
                    }
                }
            }*/
            return inst;
        }

        // Cleans the ROB of any correctly completed instructions
        void CleanROB(){
            std::pair<DecodedInstruction, Optional<int>> top = ReorderBuffer.front();

            while (ReorderBuffer.front().first.state == NEXT || (ReorderBuffer.front().first.state == CURRENT && ReorderBuffer.front().first.IsMemoryOperation)){
                
                if (ReorderBuffer.front().first.IsMemoryOperation && ReorderBuffer.front().first.state == CURRENT) {
                    ReorderBuffer.at(0).first.state = NEXT;
                    break;
                } else {
                    ReorderBuffer.pop_front();
                }
            }
        }

        bool IsInstInROB(DecodedInstruction inst){
            for (int i = 0; i < ReorderBuffer.size(); i++){
                if (ReorderBuffer.at(i).first.uniqueInstructionIdentifer == inst.uniqueInstructionIdentifer){
                    return true;
                }
            }
            return false;
        }

        // returns true if the ROB is full
        bool full(){
            if (ReorderBuffer.size() >= MAX_NUMBER_OR_ROB_ENTRIES) return true;
            return false;
        }

        ROB(){

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
                Out.OUT = In.IN0 + In.IN1;     
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
        //this->HazardDetectionUnit->ForwardResult(this->Out);

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

        int* PC;

    BU(HDU* haz, int* pointToPC) : ExecutionUnit(haz){
        PC = pointToPC;
    }

    void cycle(){

        std::cout << "BU cycle called" << std::endl;
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
                *PC = Out.OUT;

                ///* STATS */ numOfBranches++;
                //cout << "BRANCH" << endl;
                break;

            case JMPI:
                Out.OUT = In.OUT + In.DEST;//registerFile[BUD]; // WARNING ERROR HERE

                branchFlag = true;
                *PC = Out.OUT;
                
                ///* STATS */ numOfBranches++;
                //cout << "BRANCH" << endl;
                break;

            case BNE:
                if (In.IN0 < 0) {
                    Out.OUT = In.DEST;//PC = registerFile[BUD];
                    
                    branchFlag = true;
                    *PC = Out.OUT;

                    ///* STATS */ numOfBranches++;
                    //cout << "BRANCH" << endl;
                }
                break;

            case BPO:
                if (In.IN0 > 0) {
                    Out.OUT = In.DEST;//PC = registerFile[BUD];
                    
                    branchFlag = true;
                    *PC = Out.OUT;

                    ///* STATS */ numOfBranches++;
                    //cout << "BRANCH" << endl;
                }
                break;

            case BZ:
                if (In.IN0 == 0) {
                    Out.OUT = In.DEST;//PC = registerFile[BUD];
                    
                    branchFlag = true;
                    *PC = Out.OUT;

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
        //this->HazardDetectionUnit->ForwardResult(this->Out);

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
        this->In.state = CURRENT;

        std::cout << "LSU cycle called" << std::endl;

        /*
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

            //// case LID:                   // BROKEN ################################
                registerFile[ALUD] = dataMemory[dataMemory[In.IN0]];
                break;
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
        }*/


        switch(In.OpCode){
            case LD:
                Out.OUT = In.IN0;
                
                //this->writeBackFlag = true;
                break;

            case LDD:
                Out.OUT = In.IMM;
                //Out.DEST = In.DEST;

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

            //// case LID:                   // BROKEN ################################
            //    registerFile[ALUD] = dataMemory[dataMemory[In.IN0]];
            //    break;
            case LDA:
                Out.OUT = In.IN0 + In.IN1;
                //Out.DEST = In.DEST;

                //this->writeBackFlag = true;
                break;

            case STO:
                Out.OUT = In.IN0;

                //this->writeBackFlag = false;
                break;

            case STOI:                   // #####################
                Out.DEST = In.IMM;
                Out.OUT = In.IN0;
                
                //memoryData->at(In.IMM) = In.IN0;

                //this->writeBackFlag = false;
                break;
            
            case STOA:
                Out.DEST = In.DEST + In.IN0;// + In.IN0;
                Out.OUT = In.IN1;
                
                //memoryData->at(In.DEST + In.IN0) = In.IN1;

                break;

            default:
                throw std::invalid_argument("LSU cannot execute instruction: " + In.OpCode);
        }

        // Load the output into the correct row in the RAW table
        //this->HazardDetectionUnit->ForwardResult(this->Out);

        this->Out.state = NEXT;
        this->In.state = EMPTY;

        // Debugging/GUI
        //this->Inst = "";
    }
};