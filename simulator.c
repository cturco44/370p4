/**
 * Project 1
 * EECS 370 LC-2K Instruction-level simulator
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define NUMMEMORY 65536 /* maximum number of words in memory */
#define NUMREGS 8 /* number of machine registers */
#define MAXLINELENGTH 1000
#define MAX_BLOCK_SIZE 256
#define MAX_CACHE_SIZE 256

typedef struct blockStruct {
    int data[MAX_BLOCK_SIZE];
    bool isDirty;
    bool isValid;
    int lruLabel;
    int set;
    int tag;
} blockStruct;

typedef struct cacheStruct {
    blockStruct blocks[MAX_CACHE_SIZE];
    int blocksPerSet;
    int blockSize;
    int lru;
    int numSets;
    int blockBits;
    int setBits;
} cacheStruct;

cacheStruct cache;


typedef struct stateStruct {
    int pc;
    int mem[NUMMEMORY];
    int reg[NUMREGS];
    int numMemory;
} stateType;

enum actionType {
            cacheToProcessor,
            processorToCache,
            memoryToCache,
            cacheToMemory,
            cacheToNowhere
};

void printState(stateType *);
int convertNum(int);
int extractBits(int num, int position, int num_bits);
int extract_opcode(int num);
void process_opcode(stateType *statePtr, int opcode, int loaded);
void printAction(int address, int size, enum actionType type);
int log2_int(int num_in);
int load(int addr, stateType *statePtr);
void store(int addr, int data, stateType *statePtr);
void updateLRU(int start, int end, int idx);
int mask_last_n_bits(int n, int num);
void copy_to_cache(int cache_idx, int addr, stateType *statePtr);
int find_and_evict(int start, int end, stateType *statePtr);
int concatenate(int idx);

int
main(int argc, char *argv[])
{
    char line[MAXLINELENGTH];
    stateType state;
    FILE *filePtr;

    if (argc != 5) {
        printf("error: usage: %s <machine-code file>\n", argv[0]);
        exit(1);
    }

    filePtr = fopen(argv[1], "r");
    if (filePtr == NULL) {
        printf("error: can't open file %s", argv[1]);
        perror("fopen");
        exit(1);
    }
    cache.blockSize = atoi(argv[2]);
    cache.numSets = atoi(argv[3]);
    cache.blocksPerSet = atoi(argv[4]);
    cache.blockBits = log2_int(cache.blockSize);
    cache.setBits = log2_int(cache.numSets);
    
    int idx = 0;
    for(int i = 0; i < cache.numSets; ++i) {
        for(int j = 0; j < cache.blocksPerSet; ++j) {
            cache.blocks[idx].isValid = false;
            cache.blocks[idx].set = i;
            ++idx;
        }
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
        int loaded = load(state.pc, &state);
        int opcode = extract_opcode(loaded);
        if(opcode == 0b110) {
            printf("machine halted\n");
            ++state.pc;
            break;
        }
        else {
            process_opcode(&state, opcode, loaded);
        }
        ++num_instructions;
    }
    printf("total of %d instructions executed\n", num_instructions);
    printf("final state of machine:\n");
    printState(&state);
    return(0);
}
void process_opcode(stateType *statePtr, int opcode, int loaded) {
    int num = loaded;
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
        statePtr->reg[regB] = load(statePtr->reg[regA] + offset, statePtr);
        ++statePtr->pc;
    }
    //sw
    else if (opcode == 0b011) {
        int offset = extractBits(num, 0, 16);
        offset = convertNum(offset);
        int regA = extractBits(num, 19, 3);
        int regB = extractBits(num, 16, 3);
        store(statePtr->reg[regA] + offset, statePtr->reg[regB], statePtr);
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

void
printAction(int address, int size, enum actionType type)
{
    printf("@@@ transferring word [%d-%d] ", address, address + size - 1);
    if (type == cacheToProcessor) {
        printf("from the cache to the processor\n");
    } else if (type == processorToCache) {
        printf("from the processor to the cache\n");
    } else if (type == memoryToCache) {
        printf("from the memory to the cache\n");
    } else if (type == cacheToMemory) {
        printf("from the cache to the memory\n");
    } else if (type == cacheToNowhere) {
        printf("from the cache to nowhere\n");
    }
}
int log2_int(int num_in) {
    int answer = 0;
    while(num_in >>= 1) {
        ++answer;
    }
    return answer;
}
int load(int addr, stateType *statePtr) {
    int block_offset = extractBits(addr, 0, cache.blockBits);
    int set_index = extractBits(addr, cache.blockBits, cache.setBits);
    int tag_bit_start = cache.blockBits + cache.setBits;
    int num_tag_bits = 32 - cache.blockBits - cache.setBits;
    int tag = extractBits(addr, tag_bit_start, num_tag_bits);
    
    int start = cache.blocksPerSet * set_index;
    int end = start + cache.blocksPerSet;
    
    enum actionType processType;
    // Return if already in cache
    for(int i = start; i < end; ++i) {
        if(cache.blocks[i].isValid && cache.blocks[i].tag == tag) {
            processType = cacheToProcessor;
            printAction(addr, 1, processType);
            return cache.blocks[i].data[block_offset];
        }
    }
    
    int replace = find_and_evict(start, end, statePtr);
    copy_to_cache(replace, addr, statePtr);
    updateLRU(start, end, replace);
    printAction(addr, 1, cacheToProcessor);
    return cache.blocks[replace].data[block_offset];

}
void store(int addr, int data, stateType *statePtr) {
    int block_offset = extractBits(addr, 0, cache.blockBits);
    int set_index = extractBits(addr, cache.blockBits, cache.setBits);
    int tag_bit_start = cache.blockBits + cache.setBits;
    int num_tag_bits = 32 - cache.blockBits - cache.setBits;
    int tag = extractBits(addr, tag_bit_start, num_tag_bits);
    
    int start = cache.blocksPerSet * set_index;
    int end = start + cache.blocksPerSet;
    
    // Cache hit
    for(int i = start; i < end; ++i) {
        if(cache.blocks[i].isValid && cache.blocks[i].tag == tag) {
            cache.blocks[i].data[block_offset] = data;
            cache.blocks[i].isDirty = true;
            printAction(addr, 1, processorToCache);
            return;
        }
    }
    
    int replace = find_and_evict(start, end, statePtr);
    copy_to_cache(replace, addr, statePtr);
    updateLRU(start, end, replace);
    cache.blocks[replace].data[block_offset] = data;
    cache.blocks[replace].isDirty = true;
    printAction(addr, 1, processorToCache);
}
// Does not update LRU
void copy_to_cache(int cache_idx, int addr, stateType *statePtr) {
    int tag_bit_start = cache.blockBits + cache.setBits;
    int num_tag_bits = 32 - cache.blockBits - cache.setBits;
    int tag = extractBits(addr, tag_bit_start, num_tag_bits);
    
    cache.blocks[cache_idx].isValid = true;
    cache.blocks[cache_idx].isDirty = false;
    cache.blocks[cache_idx].tag = tag;
    
    int address_start = mask_last_n_bits(cache.blockBits, addr);
    // Load block into memory
    for(int j = 0; j < cache.blockSize; ++j) {
        cache.blocks[cache_idx].data[j] = statePtr->mem[address_start + j];
    }
    enum actionType processType;
    processType = memoryToCache;
    printAction(address_start, cache.blockSize, processType);
}

void updateLRU(int start, int end, int idx) {
    
    for(int i = start; i < end; ++i) {
        if(i == idx) {
            cache.blocks[i].lruLabel = 0;
        }
        else if(cache.blocks[i].isValid) {
            ++cache.blocks[i].lruLabel;
        }
        
    }


}
int mask_last_n_bits(int n, int num) {
    return num &= ~ ((1 << n) - 1);
}
// Looks for open spot. If there is one, returns the index. If not, evicts LRU and returns new idx
// Does not update LRU
int find_and_evict(int start, int end, stateType *statePtr) {
    // Look for empty slot if not in cache
    for(int i = start; i < end; ++i) {
        //Found an empty slot
        if(!cache.blocks[i].isValid) {
            return i;
        }
    }
    
    // Every spot was full, need to evict one
    
    // Find index to evict with LRU
    int max = cache.blocks[start].lruLabel;;
    int max_idx = start;
    for(int i = start; i < end; ++i) {
        if(cache.blocks[i].lruLabel > max) {
            max = cache.blocks[i].lruLabel;
            max_idx = i;
        }
    }
    
    
    int address_start = concatenate(max_idx);
    // If bit is not dirty
    if(!cache.blocks[max_idx].isDirty) {
        printAction(address_start, cache.blockSize, cacheToNowhere);
        return max_idx;
    }
    

    // LRU has dirty bit
    // Write to memory
    for(int i = 0; i < cache.blockBits; ++i) {
        statePtr->mem[address_start + i] = cache.blocks[max_idx].data[i];
    }
    printAction(address_start, 4, cacheToMemory);
    return max_idx;
}
int concatenate(int idx) {
    int tag = cache.blocks[idx].tag;
    int result = (tag << cache.setBits) | cache.blocks[idx].set;
    result = (result << cache.blockBits);
    return result;
    
}
