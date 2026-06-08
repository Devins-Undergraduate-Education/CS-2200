#include "stats.h"

/* The stats. See the definition in stats.h. */
stats_t stats;

/**
 * --------------------------------- PROBLEM 10 --------------------------------------
 * Checkout PDF section 10 for this problem
 * 
 * Calulate the total average time it takes for an access
 * 
 * HINTS:
 * 		- You may find the #defines in the stats.h file useful.
 * 		- You will need to include code to increment many of these stats in
 * 		the functions you have written for other parts of the project.
 * -----------------------------------------------------------------------------------
 */
void compute_stats() {

    // Effective (Average) Memory Access TIME - EMAT

    uint64_t accessTime = stats.accesses * MEMORY_ACCESS_TIME;
    uint64_t pageFaultTime = stats.page_faults * DISK_PAGE_READ_TIME;
    uint64_t writebackTime = stats.writebacks * DISK_PAGE_WRITE_TIME;

    // as per textbook: EMAT = T_c + m*(T_m) = T_i + m_i * EMAT_(i+1)
    stats.amat = (accessTime + writebackTime + pageFaultTime) / ((double) stats.accesses);
}
