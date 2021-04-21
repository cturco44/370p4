/**
 * Project 1
 * EECS 370 LC-2K Instruction-level simulator
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define NUMMEMORY 65536 /* maximum number of words in memory */
#define NUMREGS 8 /* number of machine registers */
#define MAXLINELENGTH 1000

typedef struct stateStruct {
    int pc;
    int mem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
} stateType;

void printState(stateType *);
int convertNum(int);
int extractBits(int num, int position, int num_bits);
int extract_opcode(int num);
void process_opcode(stateType *statePtr, int opcode);

int
main(int argc, char *argv[])
{
    char line[MAXLINELENGTH];
    stateType state;
    FILE *filePtr;

    if (argc != 2) {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }

    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", argv[1]);
        perror("fopen");
        exit(1);
    }

    /* read the entire machine-code file into memory */
    for (state.numMemory = 0; fgets(line, MAXLINELENGTH, filePtr) != NULL;
            state.numMemory++) {

        if (sscanf(line, "%d", state.mem+state.numMemory) != 1) {
            printf("error in reading address %d\n", state.numMemory);
            exit(1);
        }
        printf("memory[%d]=%d\n", state.numMemory, state.mem[state.numMemory]);
    }
    printf("\n");
    //Initialize registers to 0;
    for(int i = 0; i < NUMREGS; ++i) {
        state.reg[i] = 0;
    }
    int num_instructions = 0;
    while(1) {
        printState(&state);
        int opcode = extract_opcode(state.mem[state.pc]);
        if(state.pc == 31) {
            printf("Hello\n");
        }
        if(opcode == 0b110) {
            printf("machine halted\n");
            ++state.pc;
            break;
        }
        else {
            process_opcode(&state, opcode);
        }
        ++num_instructions;
    }
    printf("total of %d instructions executed\n", num_instructions);
    printf("final state of machine:\n");
    printState(&state);
    return(0);
}
void process_opcode(stateType *statePtr, int opcode) {
    int num = statePtr->mem[statePtr->pc];
    //add
    if(opcode == 0b000) {
        int destR = extractBits(num, 0, 3);
        int regB = extractBits(num, 16, 3);
        int regA = extractBits(num, 19, 3);
        
        statePtr->reg[destR] = statePtr->reg[regA] + statePtr->reg[regB];
        ++statePtr->pc;
    }
    //nor
    else if (opcode == 0b001){
        int destR = extractBits(num, 0, 3);
        int regB = extractBits(num, 16, 3);
        int regA = extractBits(num, 19, 3);
        
        statePtr->reg[destR] = ~(statePtr->reg[regA] | statePtr->reg[regB]);
        ++statePtr->pc;
    }
    //lw
    else if (opcode == 0b010) {
        int offset = extractBits(num, 0, 16);
        offset = convertNum(offset);
        int regA = extractBits(num, 19, 3);
        int regB = extractBits(num, 16, 3);
        statePtr->reg[regB] = statePtr->mem[statePtr->reg[regA] + offset];
        ++statePtr->pc;
    }
    //sw
    else if (opcode == 0b011) {
        int offset = extractBits(num, 0, 16);
        offset = convertNum(offset);
        int regA = extractBits(num, 19, 3);
        int regB = extractBits(num, 16, 3);
        statePtr->mem[statePtr->reg[regA] + offset] = statePtr->reg[regB];
        ++statePtr->pc;
    }
    //beq
    else if (opcode == 0b100) {
        int offset = extractBits(num, 0, 16);
        offset = convertNum(offset);
        int regA = extractBits(num, 19, 3);
        int regB = extractBits(num, 16, 3);
        if(statePtr->reg[regA] == statePtr->reg[regB]) {
            statePtr->pc = statePtr->pc + 1 + offset;
        }
        else {
            ++statePtr->pc;
        }
    }
    //jalr
    else if(opcode == 0b101) {
        int regA = extractBits(num, 19, 3);
        int regB = extractBits(num, 16, 3);
        statePtr->reg[regB] = statePtr->pc + 1;
        statePtr->pc = statePtr->reg[regA];
    }
    //noop
    else if(opcode == 0b111) {
        ++statePtr->pc;
    }
    
}
int extract_opcode(int num) {
    return extractBits(num, 22, 3);
}
int extractBits(int num, int position, int num_bits) {
    int mask = 1;
    mask = mask << num_bits;
    mask = mask - 1;
    
    num = num >> position;
    return num & mask;
}
void
printState(stateType *statePtr)
{
    int i;
    printf("\n@@@\nstate:\n");
    printf("\tpc %d\n", statePtr->pc);
    printf("\tmemory:\n");
    for (i=0; i<statePtr->numMemory; i++) {
              printf("\t\tmem[ %d ] %d\n", i, statePtr->mem[i]);
    }
    printf("\tregisters:\n");
    for (i=0; i<NUMREGS; i++) {
              printf("\t\treg[ %d ] %d\n", i, statePtr->reg[i]);
    }
    printf("end state\n");
}

int
convertNum(int num)
{
    /* convert a 16-bit number into a 32-bit Linux integer */
    if (num & (1<<15) ) {
        num -= (1<<16);
    }
    return(num);
}

