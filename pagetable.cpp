// Name: Miriam Valenzuela
// Red ID: 1131786119

#include "pagetable.h"

// Creates one Level and allocates the correct kind of entry array.
static Level* createLevel(PageTable* pageTable, unsigned int depth) {
    Level* level = new Level; 

     level->pageTablePtr = pageTable; 
    level->depth = depth;

    unsigned int entryCount = pageTable->entryCount[depth];

    if (depth == pageTable->levelCount - 1) {
        level->mapPtr = new Map[entryCount];
    } else {
        // Parenthesis initialize every child pointer to nullptr
        level->nextLevelPtr = new Level*[entryCount]();
    }

    // Every allocated array slot counts as a page-table entry
    pageTable->pageTableEntries += entryCount; 

    return level;
}


// Applies whichever requested mask and moves the page index down to bit zero. 
unsigned int extractPageNumberFromAddress(
    unsigned int virtualAddress,
    unsigned int mask,
    unsigned int shift
) {
    return (virtualAddress & mask) >> shift;
}

// Creates the paging metadata needed by every Level in the tree.
void initializePageTable(
    PageTable& pageTable,
    const unsigned int levelBits[],
    unsigned int levelCount
) {
    constexpr unsigned int addressBits = 32; 

    pageTable.levelCount = levelCount;

    pageTable.bitMaskAry = new unsigned int [levelCount];
    pageTable.shiftAry = new unsigned int[levelCount];
    pageTable.entryCount = new unsigned int[levelCount];

    unsigned int usedBits = 0; 

    for (unsigned int level = 0; level < levelCount; level++) {
        usedBits += levelBits[level];

        // Any bits below the current level determine how far its index is shifted.
        unsigned int shift = addressBits - usedBits; 

        pageTable.shiftAry[level] = shift; 
    
        // Builds a mask containing the correct number of consecutive 1 bits. 
        unsigned int levelMask = (1u << levelBits[level]) -1u;

        // Moves the 1 bits into this level's position in the address. 
        pageTable.bitMaskAry[level] = levelMask << shift; 

        // Each level needs one possible entry for every index represente by its bits. 
        pageTable.entryCount[level] = 1u << levelBits[level]; 
    }

    pageTable.offsetBits = addressBits - usedBits;

    // Offset is everything below the full VPN.
    pageTable.offsetMask = (1u << pageTable.offsetBits) - 1u; 

    pageTable.pageSize = 1u << pageTable.offsetBits;

    pageTable.nextPpn = 0;
    pageTable.pageTableEntries = 0;

    // Every page table starts with one root level
    pageTable.rootNodePtr = createLevel(&pageTable, 0); 
}

// Searches the page table for the mapping belonging to a virtual address.
Map* lookup_vpn2ppn(PageTable* pageTable, unsigned int virtualAddress) {
    if (pageTable == nullptr || pageTable->rootNodePtr == nullptr) {
        return nullptr; 
    }

    Level* currentLevel = pageTable->rootNodePtr;

    for (unsigned int depth = 0; depth < pageTable->levelCount; depth++) {
        unsigned int index = extractPageNumberFromAddress(
                                virtualAddress,
                                pageTable->bitMaskAry[depth],
                                pageTable->shiftAry[depth]
                            );

        // The leaf contains the actual VPN to PPN mapping.
        if (depth == pageTable->levelCount - 1) {
            Map* mapping = &currentLevel->mapPtr[index];

            if (mapping->valid) {
                return mapping;
            }

            return nullptr;
        }

        // A missing child means this VPN has never been mapped.
        if (currentLevel->nextLevelPtr[index] == nullptr) {
            return nullptr;
        }

        currentLevel = currentLevel->nextLevelPtr[index];

    }

    return nullptr;

}

// Inserts a new VPN to PPN mapping and creates missing tree nodes. 
void insert_vpn2ppn(PageTable* pageTable, unsigned int virtualAddress, unsigned int ppn) {
    if (pageTable == nullptr || pageTable->rootNodePtr == nullptr) {
        return;
    }

    Level* currentLevel = pageTable->rootNodePtr;

    for(unsigned int depth = 0; depth < pageTable->levelCount; depth++) {
        unsigned int index = extractPageNumberFromAddress(virtualAddress, pageTable->bitMaskAry[depth], pageTable->shiftAry[depth]);
        if (depth == pageTable->levelCount -1) {
            Map& mapping = currentLevel->mapPtr[index];

            // Existing VPN should keep its original physical page.
            if (!mapping.valid) {
                mapping.ppn = ppn;
                mapping.valid = true;
            }

            return;
        }

        // Only create a child when this tree path does not already exist.
        if (currentLevel->nextLevelPtr[index] == nullptr) {
            currentLevel->nextLevelPtr[index] = createLevel(pageTable, depth + 1);
        }

        currentLevel = currentLevel->nextLevelPtr[index];
    }
}

static void destroyLevel(Level* level) {
    if (level == nullptr) {
        return;
    }

    PageTable* pageTable = level->pageTablePtr;

    unsigned int depth = level->depth;

    if (depth < pageTable->levelCount - 1) {
        unsigned int entryCount = pageTable->entryCount[depth]; 

        for (unsigned int index = 0; index < entryCount; index++) {
            if (level->nextLevelPtr[index] != nullptr) {
                destroyLevel(level->nextLevelPtr[index]);
            }
        }

        delete[] level->nextLevelPtr;
    } else {
        // Leaf levels own an array of Map entries
        delete [] level->mapPtr;
    }

    delete level;
}

// Releases the metadata allocated.
void destroyPageTable(PageTable& pageTable) {
    destroyLevel(pageTable.rootNodePtr);

    delete[] pageTable.bitMaskAry;
    delete[] pageTable.shiftAry;
    delete[] pageTable.entryCount;

    pageTable.rootNodePtr = nullptr;

    pageTable.bitMaskAry = nullptr;
    pageTable.shiftAry = nullptr;
    pageTable.entryCount = nullptr; 

    pageTable.levelCount = 0;
    pageTable.offsetBits = 0;
    pageTable.offsetMask = 0;
    pageTable.pageSize = 0;

    pageTable.nextPpn = 0;
    pageTable.pageTableEntries = 0;
}
