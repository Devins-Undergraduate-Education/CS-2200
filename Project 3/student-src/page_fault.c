#include "mmu.h"
#include "pagesim.h"
#include "swapops.h"
#include "stats.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

/**
 * --------------------------------- PROBLEM 6 --------------------------------------
 * Checkout PDF section 7 for this problem
 * 
 * Page fault handler.
 * 
 * When the CPU encounters an invalid address mapping in a page table, it invokes the 
 * OS via this handler. Your job is to put a mapping in place so that the translation 
 * can succeed.
 * 
 * @param addr virtual address in the page that needs to be mapped into main memory.
 * 
 * HINTS:
 *      - You will need to use the global variable current_process when
 *      altering the frame table entry.
 *      - Use swap_exists() and swap_read() to update the data in the 
 *      frame as it is mapped in.
 * ----------------------------------------------------------------------------------
 */
void page_fault(vaddr_t addr) {

   stats.page_faults++; // page fault found... increment

   // create entry attributes
   vpn_t vpn = vaddr_vpn(addr);
   pte_t *pte = ((pte_t*) (mem + (PTBR * PAGE_SIZE))) + vpn; // same logic used in mem_access
   pfn_t pfn = free_frame();

   // set page table entry attributes
   pte->pfn = pfn;
   pte->valid = 1;
   pte->dirty = 0;

   // set frame table attributes for page fault
   frame_table[pfn].mapped = 1;
   frame_table[pfn].vpn = vpn;
   frame_table[pfn].process = current_process;

   // create a new frame
   paddr_t* newFrame = (paddr_t*) (mem + (PAGE_SIZE * pfn));

   // place in memory... efficiently
   if(swap_exists(pte)){ 
      swap_read(pte, newFrame);
   } else {
      memset(newFrame, 0, PAGE_SIZE);
   }
}

#pragma GCC diagnostic pop
