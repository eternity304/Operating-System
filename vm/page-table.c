#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define PAGESIZE 4096 // size of a page 4096 KB
#define VPN_BITS 9    // bits for virtual page index
#define PT_LEVELS 3   // levels of page table i.e. L2, L1, L0 in Sv39
#define PTE_COUNT (1 << VPN_BITS) // 2e9 = 512
#define MAX_PROCESSES 256 

// PTE flags
#define PTE_V 0x001 // Valid
#define PTE_R 0x002 // Read
#define PTE_W 0x004 // Write
#define PTE_X 0x008 // Execute
#define PTE_U 0x010 // User
#define PTE_G 0x020 // Global
#define PTE_A 0x040 // Accessed
#define PTE_D 0x080 // Dirty
#define PTE_COW 0x100 // Copy-On-Write (Custom flag)

// Simulated physical memory
uint8_t *phys_mem;

// Process control block
typedef struct {
    uint64_t *satp; // Root page table
    int pid;        // pid 
} process_t;

process_t *process_table[MAX_PROCESSES]; // A List containing all processes 

// Functions to simulate MMU and paging
uint64_t *allocate_page_table() {
    // - Initialize a page table of size PTE_COUNT by sizeof(uint64_t) i.e. 4096 by 8 bytes
    uint64_t *pt = (uint64_t *)calloc(PTE_COUNT, sizeof(uint64_t));
    return pt;
}

uint64_t allocate_phys_page() {
    static uint64_t phys_page_counter = 0;
    uint64_t ppn = phys_page_counter++;
    return ppn;
}

void map_page(uint64_t *root_pt, uint64_t va, uint64_t pa, uint64_t flags) {
    uint64_t vpn[PT_LEVELS];
    uint64_t *pt = root_pt;

    // Extract VPNs
    vpn[0] = (va >> 12) & 0x1FF;
    vpn[1] = (va >> 21) & 0x1FF;
    vpn[2] = (va >> 30) & 0x1FF;

    for (int level = PT_LEVELS - 1; level >= 0; level--) {
        uint64_t idx = vpn[level];
        if (level == 0) {
            // Leaf PTE
            pt[idx] = (pa << 10) | flags | PTE_V;
        } else {
            if (!(pt[idx] & PTE_V)) {
                // Allocate next level page table
                uint64_t *next_pt = allocate_page_table();
                uint64_t ppn = allocate_phys_page();
                pt[idx] = (ppn << 10) | PTE_V;
                pt = next_pt;
            } else {
                // Follow the pointer
                uint64_t ppn = pt[idx] >> 10;
                pt = (uint64_t *)(phys_mem + ppn * PAGESIZE);
            }
        }
    }
}

void copy_page_table(uint64_t *src_pt, uint64_t *dst_pt, int level) {
    for (int i = 0; i < PTE_COUNT; i++) {
        uint64_t pte = src_pt[i];
        if (pte & PTE_V) {
            if ((pte & (PTE_R | PTE_W | PTE_X)) == 0) {
                // Intermediate level, allocate new page table
                uint64_t *new_pt = allocate_page_table();
                uint64_t ppn = allocate_phys_page();
                dst_pt[i] = (ppn << 10) | PTE_V;
                uint64_t *child_src_pt = (uint64_t *)(phys_mem + ((pte >> 10) * PAGESIZE));
                uint64_t *child_dst_pt = new_pt;
                copy_page_table(child_src_pt, child_dst_pt, level - 1);
            } else {
                // Leaf PTE, copy directly
                dst_pt[i] = pte;
            }
        }
    }
}

void cow_page_table(uint64_t *src_pt, uint64_t *dst_pt, int level) {
    for (int i = 0; i < PTE_COUNT; i++) {
        uint64_t pte = src_pt[i];
        if (pte & PTE_V) {
            if ((pte & (PTE_R | PTE_W | PTE_X)) == 0) {
                // Intermediate level, allocate new page table
                uint64_t *new_pt = allocate_page_table();
                uint64_t ppn = allocate_phys_page();
                dst_pt[i] = (ppn << 10) | PTE_V;
                uint64_t *child_src_pt = (uint64_t *)(phys_mem + ((pte >> 10) * PAGESIZE));
                uint64_t *child_dst_pt = new_pt;
                cow_page_table(child_src_pt, child_dst_pt, level - 1);
            } else {
                // Leaf PTE, set COW flag and mark read-only
                src_pt[i] &= ~PTE_W;
                src_pt[i] |= PTE_COW;
                dst_pt[i] = src_pt[i];
            }
        }
    }
}

void handle_cow_fault(process_t *proc, uint64_t va) {
    uint64_t vpn[PT_LEVELS];
    uint64_t *pt = proc->satp;

    vpn[0] = (va >> 12) & 0x1FF;
    vpn[1] = (va >> 21) & 0x1FF;
    vpn[2] = (va >> 30) & 0x1FF;

    for (int level = PT_LEVELS - 1; level >= 0; level--) {
        uint64_t idx = vpn[level];
        uint64_t pte = pt[idx];
        if (!(pte & PTE_V)) {
            printf("Segmentation fault\n");
            exit(1);
        }
        if (level == 0) {
            if (pte & PTE_COW) {
                // Perform the copy
                uint64_t old_ppn = pte >> 10;
                uint64_t new_ppn = allocate_phys_page();
                memcpy(phys_mem + new_ppn * PAGESIZE, phys_mem + old_ppn * PAGESIZE, PAGESIZE);
                pt[idx] = (new_ppn << 10) | (pte & ~PTE_COW) | PTE_W;
            } else {
                printf("Write fault\n");
                exit(1);
            }
        } else {
            uint64_t ppn = pte >> 10;
            pt = (uint64_t *)(phys_mem + ppn * PAGESIZE);
        }
    }
}

process_t *fork_process(process_t *parent, int cow) {
    process_t *child = malloc(sizeof(process_t));
    child->pid = parent->pid + 1;
    child->satp = allocate_page_table();

    if (cow) {
        cow_page_table(parent->satp, child->satp, PT_LEVELS - 1);
    } else {
        copy_page_table(parent->satp, child->satp, PT_LEVELS - 1);
    }

    process_table[child->pid] = child;
    return child;
}

// Test functions
void simulate() {
    // Initialize physical memory
    phys_mem = calloc(1 << 20, PAGESIZE); // Allocate 1MB of physical memory

    // Create initial process
    process_t *init_proc = malloc(sizeof(process_t));
    init_proc->pid = 0;
    init_proc->satp = allocate_page_table();
    process_table[init_proc->pid] = init_proc;

    // Map some pages
    map_page(init_proc->satp, 0x1000, allocate_phys_page(), PTE_R | PTE_W | PTE_U);
    map_page(init_proc->satp, 0x2000, allocate_phys_page(), PTE_R | PTE_W | PTE_U);

    // Fork without COW
    process_t *child_proc = fork_process(init_proc, 0);
    printf("Forked process %d without COW\n", child_proc->pid);

    // Fork with COW
    process_t *cow_child_proc = fork_process(init_proc, 1);
    printf("Forked process %d with COW\n", cow_child_proc->pid);

    // Simulate a write in the child process that triggers COW
    handle_cow_fault(cow_child_proc, 0x1000);
    printf("Handled COW fault for process %d at address 0x1000\n", cow_child_proc->pid);
}

int main() {
    simulate();
    return 0;
}
