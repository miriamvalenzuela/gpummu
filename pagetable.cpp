// Name: Miriam Valenzuela
// Red ID: 1131786119

#include "pagetable.h"

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

    pageTable.nextPpn = 0;
    pageTable.pageTableEntries = 0;

    pageTable.rootNodePtr = nullptr; 
}

// Releases the metadata allocated.
void destroyPageTable(PageTable& pageTable) {
    delete[] pageTable.bitMaskAry;
    delete[] pageTable.shiftAry;
    delete[] pageTable.entryCount;

    pageTable.bitMaskAry = nullptr;
    pageTable.shiftAry = nullptr;
    pageTable.entryCount = nullptr; 
}
