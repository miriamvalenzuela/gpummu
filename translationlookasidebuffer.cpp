// Name: Jesus Hernandez
// Red ID: 131481477

#include "translationlookasidebuffer.h"

using namespace std;

/* Attempt to fetch a page from the TLB */
Map* TranslationLookasideBuffer::fetch(unsigned int virtualAddress, unsigned int shift)
{
    // No (valid) TLB
    if (capacity == 0) return nullptr;

    Map* result = nullptr;
    virtual_time++;

    // Get page from address
    unsigned int virtual_page = virtualAddress >> shift;

    // Search most recently accessed first, then rest of TLB
    for (unsigned int curr_tlb_index = 0; curr_tlb_index < pages.size(); curr_tlb_index++)
    {
        VP_Map& mapping = pages[curr_tlb_index];

        // Found within TLB
        if (mapping.virtual_page == virtual_page)
        {
            mapping.age = virtual_time;
            result = mapping.physical_page;

            // Move it to most recently accessed (and swap with least accessed) if not in it already
            if (curr_tlb_index >= recent_size)
            {
                unsigned int oldest = 0;

                // Find oldest within recently accessed list
                for (unsigned int curr_recent_index = 0; curr_recent_index < recent_size; curr_recent_index++)
                {
                    if (pages[curr_recent_index].age < pages[oldest].age) oldest = curr_recent_index;
                }

                swap(pages[curr_tlb_index], pages[oldest]);
            }
        }
    }

    return result;
}

/** 
 * @brief If TLB cache misses (or no cache exists), seek from page table.
 * @returns Pointer to Page Table mapping, if there is one. Returns null if it is inserted instead.
 */
Map* TranslationLookasideBuffer::fetch_from_pagetable(unsigned int virtualAddress, unsigned int shift, PageTable* pageTable)
{
    // First look it up on pagetable
    Map* result = lookup_vpn2ppn(pageTable, virtualAddress);

    // Not in pagetable, insert it instead
    if (result == nullptr)
    {
        insert_vpn2ppn(pageTable, virtualAddress, pageTable->nextPpn++);
    }

    // Create mapping and add it to TLB cache
    VP_Map new_entry{virtual_time, virtualAddress >> shift, lookup_vpn2ppn(pageTable, virtualAddress)};
    add_recently_accessed(new_entry);

    return result;
}

/** 
 * @brief Add to list of most recently used.
 * @returns Pointer to replaced mapping, if there is one.
 */
unique_ptr<TranslationLookasideBuffer::VP_Map> TranslationLookasideBuffer::add_recently_accessed(VP_Map& page)
{
    // No (valid) TLB
    if (capacity == 0) return nullptr;

    // Room to grow recently accessed list
    if (recent_size < RECENTLY_ACCESSED_CAP && recent_size < capacity)
    {
        pages.push_back(page);
        recent_size++;

        return nullptr;
    }
    
    // No room to grow recently accessed list, must replace existing one
    unsigned int oldest = 0;

    // Find index of oldest
    for (unsigned int curr_tlb_index = 0; curr_tlb_index < recent_size; curr_tlb_index++)
        if (pages[curr_tlb_index].age < pages[oldest].age) oldest = curr_tlb_index;

    // Throw oldest to back if there is room
    if (pages.size() < capacity)
    {
        pages.push_back(pages[oldest]);
    }

    unique_ptr<VP_Map> victim(new VP_Map(pages[oldest]));
    pages.erase(pages.begin() + oldest);
    pages.insert(pages.begin() + oldest, page);

    return victim;
}