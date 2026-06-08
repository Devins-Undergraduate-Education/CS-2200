#include "types.h"
#include "pagesim.h"
#include "mmu.h"
#include "swapops.h"
#include "stats.h"
#include "util.h"

pfn_t select_victim_frame(void);

pfn_t last_evicted = 0;

/**
 * --------------------------------- PROBLEM 7 --------------------------------------
 * Checkout PDF section 7 for this problem
 * 
 * Make a free frame for the system to use. You call the select_victim_frame() method
 * to identify an "available" frame in the system (already given). You will need to 
 * check to see if this frame is already mapped in, and if it is, you need to evict it.
 * 
 * @return victim_pfn: a phycial frame number to a free frame be used by other functions.
 * 
 * HINTS:
 *      - When evicting pages, remember what you checked for to trigger page faults 
 *      in mem_access
 *      - If the page table entry has been written to before, you will need to use
 *      swap_write() to save the contents to the swap queue.
 * ----------------------------------------------------------------------------------
 */
pfn_t free_frame(void) {

    // I hate these two lines... apparently "nested functions" are not allowed...
    pfn_t victim_pfn;
    victim_pfn = select_victim_frame();

    // calculate victim frame table entry
    fte_t *victim_fte = (fte_t*) (frame_table + victim_pfn);

    // if mapped, good! 
    if (frame_table[victim_pfn].mapped) {

        // calculate victim attributes
        pcb_t *victim_pcb = victim_fte->process;
        pte_t *victim_pte = ((pte_t*) (mem + (victim_pcb->saved_ptbr * PAGE_SIZE))) + victim_fte->vpn;

        // if page table entry is dirty...
        if (victim_pte->dirty) {
            // writeback!!
            swap_write(victim_pte, (uint8_t*) (mem + (victim_pfn * PAGE_SIZE)));
            stats.writebacks++;
        }

        // set page table entry 'good' stats
        victim_pte->valid = 0;
        victim_pte->dirty = 0;
    }

    // upmap since returning free frame
    victim_fte -> mapped = 0;
    
    return victim_pfn;
}

// Page Table: Virtual Address --> Physical Address (kernel memory) - one page table per process
// Frame Table: Reverse Mapping; given a frame number, return process ID and VPN, one frame table
// Dirty Bit: page entry that has been modified by the program isince brought from the disk
// Valid Bit: if the page is in actually in memory
// Protected Bit: kernel code, frame table, page table, OS code
// Referenced Bit: Page replacement // clocksweep or second chance replacement
// Physical memory is small, virtual memory is faster and is contiguous


/**
 * --------------------------------- PROBLEM 9 --------------------------------------
 * Checkout PDF section 7, 9, and 11 for this problem
 * 
 * Finds a free physical frame. If none are available, uses either a
 * randomized, FCFS, or clocksweep algorithm to find a used frame for
 * eviction.
 * 
 * @return The physical frame number of a victim frame.
 * 
 * HINTS: 
 *      - Use the global variables MEM_SIZE and PAGE_SIZE to calculate
 *      the number of entries in the frame table.
 *      - Use the global last_evicted to keep track of the pointer into the frame table
 * ----------------------------------------------------------------------------------
 */
pfn_t select_victim_frame() {

    /* See if there are any free frames first */
    size_t num_entries = MEM_SIZE / PAGE_SIZE;
    for (size_t i = 0; i < num_entries; i++) {
        if (!frame_table[i].protected && !frame_table[i].mapped) {
            return i;
        }
    }

    // RANDOM implemented for you... so kind!
    if (replacement == RANDOM) {
        /* Play Russian Roulette to decide which frame to evict */
        pfn_t unprotected_found = NUM_FRAMES;
        for (pfn_t i = 0; i < num_entries; i++) {
            if (!frame_table[i].protected) {
                unprotected_found = i;
                if (prng_rand() % 2) {
                    return i;
                }
            }
        }
        /* If no victim found yet take the last unprotected frame
           seen */
        if (unprotected_found < NUM_FRAMES) {
            return unprotected_found;
        }


    } else if (replacement == FIFO) {
        // start after the last removed
        pfn_t start = last_evicted + 1;

        // loop over entries and find one to evict
        for (size_t i = start; i < start + num_entries; i++) {

            pfn_t idx = i % num_entries; // Wrap around the frame table

            // if frame is NOT protected...
            if (!frame_table[idx].protected) {
                last_evicted = idx; // evicted is the WRAPPED index
                return idx; // return unprotected frame
            }
        }

    } else if (replacement == CLOCKSWEEP) {

        // start after the last removed
        pfn_t start = last_evicted + 1;

        // loop over the entries to find one to evict
        for (size_t i = start; i < start + num_entries; i++) {

            pfn_t idx = i % num_entries; // Wrap around the frame table

            // if frame table is protected, skip
            if(frame_table[idx].protected) {
                continue;
            }

            // set page table entry attributes to set referenced bit
            pcb_t *process = frame_table[idx].process;
            vpn_t vpn = frame_table[idx].vpn;
            pte_t *pte = ((pte_t*) (mem + (process->saved_ptbr * PAGE_SIZE))) + vpn; // used in mem_access

            // if page table entry has not been referenced...
            if (!pte->referenced) {
                last_evicted = idx; // we found one!
                return idx; // return unprotected frame
            }

            pte->referenced = 0; // set entry ref bit as 0... next time
        }

        // if all frames have their reference bit set, revert to FIFO behavior

        // start after the last removed
        pfn_t ret_pfn = last_evicted + 1;

        // loop over entries and find one to evict
        for (size_t i = ret_pfn; i < ret_pfn + num_entries; i++) {
            pfn_t idx = i % num_entries; // Wrap around the frame table

            // if frame is NOT protected...
            if (!frame_table[idx].protected) {
                last_evicted = idx; // evicted is the WRAPPED index
                return idx; // return unprotected frame
            }
        }
    }

    /* If every frame is protected, give up. This should never happen
       on the traces we provide you. */
    panic("System ran out of memory\n");
    exit(1);
}