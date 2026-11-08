// Name: Miriam Valenzuela, Jesus Hernandez
// Red ID: 131786119, 131481477

#include <string>
#include <vector>
#include <limits>
#include <unistd.h>
#include <cerrno>
#include <iostream>
#include <cstdio>
#include <cstdlib>


#include "log.h"
#include "pagetable.h"
#include "translationlookasidebuffer.h"
#include "tracereader.h"

#define DECIMAL_BASE 10
#define MAX_PAGETABLE_BITS 28

using namespace std;

enum class OutputMode {
    BITMASKS, 
    VA2PA,
    VA2PA_TLB_PTWALK,
    VPN2PPN,
    OFFSET,
    SUMMARY
};

// Keeps the supported output modes easier to work with than raw strings.
struct ProgramOptions {
    string tracePath;
    vector<unsigned int> levelBits;

    bool hasAccessLimit = false;
    unsigned int accessLimit = 0; 
    
    unsigned int tlbCapacity = 0;

    OutputMode outputMode = OutputMode::SUMMARY;
};

// Stores the values collected from the command line.
bool parsePositiveNumber(const char* text, unsigned int& result) {
    errno = 0;

    char* end = nullptr; 

    long value = strtol(text, &end, DECIMAL_BASE);

    if (errno != 0 
        || end == text 
        || *end != '\0' 
        || value <= 0 
        || value > static_cast<long>(numeric_limits<unsigned int>::max())) {
            return false;
    }

    result = static_cast<unsigned int>(value);

    return true;
}

// Converts an argument to an unsigned int while still allowing zero. 
bool parseNonNegativeNumber(const char* text, unsigned int& result) {
    errno = 0; 

    char* end = nullptr;

    long value = strtol(text, &end, DECIMAL_BASE);

    if (errno != 0 ||
        end == text ||
        *end != '\0' ||
        value < 0 ||
        value > static_cast<long>(numeric_limits<unsigned int>::max())) {
            return false;
    }

    result = static_cast<unsigned int>(value);

    return true;
}

// Converts te -o string into one of the supported output modes. 
bool parseOutputMode(const string& text, OutputMode& mode) {
    struct ModeOption {
        const char* name;
        OutputMode mode;
    };

    static const ModeOption modes[] = {
        {"bitmasks", OutputMode::BITMASKS},
        {"va2pa", OutputMode::VA2PA},
        {"va2pa_tlb_ptwalk", OutputMode::VA2PA_TLB_PTWALK},
        {"vpn2ppn", OutputMode::VPN2PPN},
        {"offset", OutputMode::OFFSET},
        {"summary", OutputMode::SUMMARY}
    };

    for (const ModeOption& option : modes) {
        if (text == option.name) {
            mode = option.mode;
            return true; 
        }
    }

    return false;
}

// Reads the optional arguments followed by te trace path and page-level bit counts.
bool parseArguments(int argc, char* argv[], ProgramOptions& options) {
    int option; 

    // Prevents getopt from printing messages
    opterr = 0; 

    while ((option = getopt(argc, argv, "n:c:o:")) != -1) {
        switch (option) {
            case 'n':
                if (!parsePositiveNumber(optarg, options.accessLimit)) {
                    cout << "Number of KV-cache accesses must be a number, greater than 0\n";
                    return false;
                }

                options.hasAccessLimit = true;
                break;
            
            case 'c':
                if (!parseNonNegativeNumber(optarg, options.tlbCapacity)) {
                    cout << "TLB capacity must be a number, greater than or equal to 0\n";
                    return false;
                }

                break; 

            case 'o':
                if (!parseOutputMode(optarg, options.outputMode)) {
                    return false;
                }

                break;

            default:
                return false;
        }
    }

    // Trace path and at least one-page table level are mandatory.
    if (optind >= argc || optind + 1 >= argc) {
        return false; 
    }

    options.tracePath = argv[optind];

    unsigned int totalBits = 0; 
    unsigned int level = 0; 

    // Every remaining mandatory argument represents one page-table level.
    for (int index = optind + 1; index < argc; index++) {
        unsigned int bits; 

        if (!parsePositiveNumber(argv[index], bits)) {
            cout << "Level " << level
                 << " page table must be at least 1 bit\n";
            return false; 
        }

        options.levelBits.push_back(bits);

        // Prevent overflow with absurdly large values
        if (bits > MAX_PAGETABLE_BITS - totalBits) {
            cout << "Too many bits used in page tables\n";
            return false;
        }
        
        totalBits += bits;
        level++;
    }

    return true; 
}

// Logs the page-table indexes and mapped physical page. 
void logVpnMapping(const PageTable& pageTable, unsigned int virtualAddress, unsigned int ppn) {
    vector<uint32_t> pages(pageTable.levelCount);

    for (unsigned int level = 0; level < pageTable.levelCount; level++) {
        pages[level] = extractPageNumberFromAddress(virtualAddress, pageTable.bitMaskAry[level], pageTable.shiftAry[level]);
    }

    log_pagemapping(static_cast<int>(pageTable.levelCount), pages.data(), ppn);
}

// Processes trace addresses using TLB and demand paging. 
void processTrace(FILE* traceFile, PageTable& pageTable, TranslationLookasideBuffer tlbCache, const ProgramOptions& options) {
    p2AddrTr trace; 

    unsigned int addressesProcessed = 0; 
    unsigned int pageTableHits      = 0; 
    unsigned int tlbHits            = 0;

    while ((!options.hasAccessLimit || addressesProcessed < options.accessLimit) && NextAddress(traceFile, &trace)) {
        // Pull out the virtual address
        unsigned int virtualAddress = trace.addr; 

        bool tlbHit       = false;
        bool pageTableHit = false;

        // Look for VPN in TLB cache first (if one is specified)
        Map* mapping = tlbCache.fetch(virtualAddress, pageTable.offsetBits);

        // Found in TLB cache
        if (mapping != nullptr) 
        {
            tlbHit = true;
            tlbHits++;
        }

        // Not found in TLB cache; look in page table instead.
        else
        {
            mapping = tlbCache.fetch_from_pagetable(virtualAddress, pageTable.offsetBits, &pageTable);

            // Found in page table
            if (mapping != nullptr)
            {
                pageTableHit = true;
                pageTableHits++;
            }

            // ...or it was neither in TLB nor the page table, and it was inserted with the previous lookup.
            else mapping = lookup_vpn2ppn(&pageTable, virtualAddress);
        }

        // Keep only the offset portion of the virtual address. 
        unsigned int offset = virtualAddress & pageTable.offsetMask;

        unsigned int physicalAddress = (mapping->ppn << pageTable.offsetBits) | offset; 

        switch (options.outputMode) {
            case OutputMode::OFFSET:
                hexnum(offset);
                break;

            case OutputMode::VPN2PPN:
                logVpnMapping(pageTable, virtualAddress, mapping->ppn); 
                break;

            case OutputMode::VA2PA:
                log_virtualAddr2physicalAddr(virtualAddress, physicalAddress);
                break;

            case OutputMode::VA2PA_TLB_PTWALK:
                log_va2pa_TLB_PTwalk(virtualAddress, physicalAddress, tlbHit, pageTableHit);
                break;

            case OutputMode::SUMMARY:
                // Summary is printed once after processing the trace.
                break; 

            case OutputMode::BITMASKS:
                // Bitmask mode returns before trace processing.
                break;

            default:
                break;
        }

        addressesProcessed++;
    }

    if (options.outputMode == OutputMode::SUMMARY) {
        log_summary(pageTable.pageSize, tlbHits, pageTableHits, addressesProcessed, pageTable.nextPpn, pageTable.pageTableEntries);
    }
}

int main(int argc, char* argv[]) {
    ProgramOptions options;

    if (!parseArguments(argc, argv, options)) {
        return 0;
    }

    // Check that the mandatory trace file can actually be opened. 
    FILE* traceFile = fopen(options.tracePath.c_str(), "rb");

    if (traceFile == nullptr) {
        cout << "Unable to open <<" << options.tracePath << ">>\n";
        return 0;
    }

    PageTable pageTable;

    initializePageTable (
        pageTable,
        options.levelBits.data(),
        static_cast<unsigned int>(options.levelBits.size())
    );

    TranslationLookasideBuffer tlbCache(options.tlbCapacity);

    // Bitmaks mode only needs the page-table configuration. 
    if (options.outputMode == OutputMode::BITMASKS) {
        log_bitmasks(
            static_cast<int>(pageTable.levelCount),
            pageTable.bitMaskAry
        );

        fclose(traceFile);
        destroyPageTable(pageTable);

        return 0;
    }

    processTrace(traceFile, pageTable, tlbCache, options);

    fclose(traceFile);

    destroyPageTable(pageTable);

    return 0;

}


