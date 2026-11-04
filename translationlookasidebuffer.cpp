// Name: Jesus Hernandez
// Red ID: 131481477

#include "translationlookasidebuffer.h"
#include <memory>

using namespace std;

/* Attempt to fetch a page from the TLB */
Map* TranslationLookasideBuffer::fetch(unsigned int page)
{
    if (capacity == 0) return nullptr;

    Map* result = nullptr;
    virtual_time++;

    // Search most recently accessed first, then rest of TLB
    for (unsigned int i = 0; i < pages.size(); i++)
    {
        VP_Map mapping = pages[i];

        // Found within TLB
        if (mapping.virtual_page == page)
        {
            mapping.age = virtual_time;
            result = mapping.physical_page;

            // Move it to most recently accessed (and swap with least accessed) if not in it already
            if (i >= recent_size)
            {
                pages.erase(pages.begin() + i);
                pages.push_back(*(add_recently_accessed(mapping)));
            }
        }
    }

    return result;
}

/* If TLB cache misses, seek from page table */
Map* TranslationLookasideBuffer::fetch_pagetable(unsigned int vAddress, unsigned int shift, PageTable* ptable)
{
    // First look it up on pagetable
    Map* result = lookup_vpn2ppn(ptable, vAddress);
    // virtual_time++

    // Not in pagetable, insert it instead
    if (result == nullptr)
    {
        insert_vpn2ppn(ptable, vAddress, ptable.nextPpn++);
        result = lookup_vpn2ppn(ptable, vAddress);
    }

    // Create mapping and add it to TLB cache
    VP_Map new_entry{virtual_time, vAddress >> shift, result};
    add_recently_accessed(new_entry);

    return result;
}

/* Add to list of most recently used */
unique_ptr<VP_Map> TranslationLookasideBuffer::add_recently_accessed(VP_Map& page)
{
    // Room to grow recently accessed list
    if (recent_size < RECENTLY_ACCESSED_CAP)
    {
        pages.push_back(page);
        recent_size++;

        return nullptr;
    }
    
    // No room to grow recently accessed list, must replace existing one
    unsigned int oldest = 0;

    // Find index of oldest
    for (unsigned int i = 0; i < recent_size; i++)
        if (pages[i].age < pages[oldest].age) oldest = i;

    // Throw oldest to back if there is room
    if (size < capacity)
        pages.pop_back(pages[oldest]);

    unique_ptr<VP_Map> victim(new VP_Map(pages[oldest]));
    pages.erase(pages.begin() + oldest);

    return victim;
}