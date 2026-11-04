// Name: Jesus Hernandez
// Red ID: 131481477

#ifndef TRANSLATIONLOOKASIDEBUFFER_H
#define TRANSLATIONLOOKASIDEBUFFER_H

#define RECENTLY_ACCESSED_CAP 8

#include <vector>
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

    unsigned int size = 0;
    unsigned int capacity;
    unsigned int recent_size = 0;
    unsigned int virtual_time = 0;
    std::vector<VP_Map> pages;

    /* If TLB cache misses, seek from page table */
    Map* fetch_pagetable(unsigned int page, unsigned int shift, PageTable* ptable);

    /* Add to list of most recently used */
    VP_Map add_recently_accessed(VP_Map& page);

  public:
    /* Initializes TLB Cache with provided data */
    TranslationLookasideBuffer() : capacity(0);
    TranslationLookasideBuffer(unsigned int cap) : capacity(cap);

    /* Attempt to fetch a page from the TLB */
    Map* fetch(unsigned int page);
};

#endif