// Name: Miriam Valenzuela
// Red ID: 131786119

#ifndef PAGETABLE_H
#define PAGETABLE_H

// Represents one valid VPN to physical page mapping at a leaf node.
struct Map {
    unsigned int ppn = 0;
    bool valid = false; 
};

struct PageTable;

// Represents one node/level in the multi-level page table tree.
struct Level {
    PageTable* pageTablePtr = nullptr; 
    unsigned int depth = 0;

    // The interior levels will use this array to point to child level nodes.
    Level** nextLevelPtr = nullptr; 

    // Leaf levels will use this array to store VPN to PPN mappings.
    Map* mapPtr = nullptr;
};

// Will store configurations and shared information for the page table tree. 
struct PageTable {
    unsigned int levelCount = 0;

    unsigned int*  bitMaskAry = nullptr;
    unsigned int* shiftAry = nullptr;
    unsigned int* entryCount = nullptr;

    Level* rootNodePtr = nullptr;

    unsigned int offsetBits = 0;
    unsigned int offsetMask = 0;
    // The next physical page assigned during the demand paging. 
    unsigned int nextPpn = 0;

    // Used later for summary output.
    unsigned long pageTableEntries = 0;
};

// Builds the masks, shifts, and entry counts for the requested paging layout.
void initializePageTable(PageTable& pageTable, const unsigned int levelBits[], unsigned int levelCount);

// Applies a mask and shift to extract a page-table index.
unsigned int extractPageNumberFromAddress(unsigned int virtualAddress, unsigned int mask, unsigned int shift);

// Releases any metadata arrays that got allocated during initialization.
void destroyPageTable(PageTable& pageTable);

#endif