// Name: Jesus Hernandez
// Red ID: 131481477

#ifndef TRANSLATIONLOOKASIDEBUFFER_H
#define TRANSLATIONLOOKASIDEBUFFER_H

#define RECENTLY_ACCESSED_CAP 8

#include <memory>
#include <vector>
#include <utility>
#include "pagetable.h"

/* Define metadata of Translation Lookaside Buffer cache */
struct TranslationLookasideBuffer
{
  private:
    /* Stores VPN -> PPN mappings, as well as age. */
    struct VP_Map
    {
        unsigned int age = 0;
        unsigned int virtual_page = 0;
        Map* physical_page = nullptr;
    };

    unsigned int capacity;
    unsigned int recent_size  = 0;
    unsigned int virtual_time = 0;
    std::vector<VP_Map> pages;

    /** 
     * @brief Add to list of most recently used.
     * @returns Pointer to replaced mapping, if there is one.
     */
    std::unique_ptr<VP_Map> add_recently_accessed(VP_Map& page);

  public:
    /* Initializes TLB Cache with provided data */
    TranslationLookasideBuffer()                 : capacity(0)   {};
    TranslationLookasideBuffer(unsigned int tlb_size_cap) : capacity(tlb_size_cap) {};

    /* Attempt to fetch a page from the TLB */
    Map* fetch(unsigned int virtualAddress, unsigned int shift);

    /** 
     * @brief If TLB cache misses (or no cache exists), seek from page table.
     * @returns Pointer to Page Table mapping, if there is one. Returns null if it is inserted instead.
     */
    Map* fetch_from_pagetable(unsigned int virtualAddress, unsigned int shift, PageTable* pageTable);
};

#endif