#include "vmm.h"
#include "pmm.h"
#include <stddef.h>

volatile uint16_t *vmm_framebuffer = NULL;

static uint64_t *kernel_pml4 = NULL;

// Page table entry helpers
static inline int pte_present(uint64_t pte) {
    return (pte & VMM_PRESENT) != 0;
}

static inline uint64_t pte_addr(uint64_t pte) {
    return pte & 0x000FFFFFFFFFF000ULL;
}

static inline uint64_t *pte_to_ptr(uint64_t pte) {
    return (uint64_t *)(uintptr_t)pte_addr(pte);
}

// Get page table indices from virtual address
static inline uint64_t pml4_index(uint64_t virt) {
    return (virt >> 39) & 0x1FF;
}

static inline uint64_t pdpt_index(uint64_t virt) {
    return (virt >> 30) & 0x1FF;
}

static inline uint64_t pd_index(uint64_t virt) {
    return (virt >> 21) & 0x1FF;
}

static inline uint64_t pt_index(uint64_t virt) {
    return (virt >> 12) & 0x1FF;
}

static inline uint64_t page_offset(uint64_t virt) {
    return virt & 0xFFF;
}

void vmm_map_page(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags) {
    uint64_t pml4_idx = pml4_index(virt);
    uint64_t pdpt_idx = pdpt_index(virt);
    uint64_t pd_idx = pd_index(virt);
    uint64_t pt_idx = pt_index(virt);
    
    // Get or create PML4 entry
    if (!pte_present(pml4[pml4_idx])) {
        void *pdpt_page = pmm_alloc();
        if (!pdpt_page) return; // Out of memory
        pml4[pml4_idx] = (uint64_t)(uintptr_t)pdpt_page | VMM_PRESENT | VMM_WRITABLE;
        // Zero the new page
        uint64_t *pdpt = (uint64_t *)pdpt_page;
        for (int i = 0; i < 512; i++) pdpt[i] = 0;
    }
    uint64_t *pdpt = pte_to_ptr(pml4[pml4_idx]);
    
    // Get or create PDPT entry
    if (!pte_present(pdpt[pdpt_idx])) {
        void *pd_page = pmm_alloc();
        if (!pd_page) return;
        pdpt[pdpt_idx] = (uint64_t)(uintptr_t)pd_page | VMM_PRESENT | VMM_WRITABLE;
        uint64_t *pd = (uint64_t *)pd_page;
        for (int i = 0; i < 512; i++) pd[i] = 0;
    }
    uint64_t *pd = pte_to_ptr(pdpt[pdpt_idx]);
    
    // Get or create PD entry
    if (!pte_present(pd[pd_idx])) {
        void *pt_page = pmm_alloc();
        if (!pt_page) return;
        pd[pd_idx] = (uint64_t)(uintptr_t)pt_page | VMM_PRESENT | VMM_WRITABLE;
        uint64_t *pt = (uint64_t *)pt_page;
        for (int i = 0; i < 512; i++) pt[i] = 0;
    }
    uint64_t *pt = pte_to_ptr(pd[pd_idx]);
    
    // Set page table entry
    pt[pt_idx] = phys | flags;
    
    // Invalidate TLB for this page
    __asm__ __volatile__("invlpg (%0)" : : "r"(virt) : "memory");
}

void vmm_identity_map(uint64_t *pml4, uint64_t start, uint64_t len, uint64_t flags) {
    uint64_t end = start + len;
    for (uint64_t addr = start; addr < end; addr += 4096) {
        vmm_map_page(pml4, addr, addr, flags);
    }
}

void vmm_load_pml4(uint64_t *pml4) {
    uint64_t cr3 = (uint64_t)(uintptr_t)pml4;
    __asm__ __volatile__("mov %0, %%cr3" : : "r"(cr3) : "memory");
}

uint64_t *vmm_get_pml4(void) {
    return kernel_pml4;
}

void vmm_init(void) {
    // Allocate PML4
    kernel_pml4 = (uint64_t *)pmm_alloc();
    if (!kernel_pml4) return;
    
    // Zero PML4
    for (int i = 0; i < 512; i++) {
        kernel_pml4[i] = 0;
    }
    
    // Identity map first 4MB (kernel code, data, stack, etc.)
    vmm_identity_map(kernel_pml4, 0x00000000, 0x00400000, VMM_PRESENT | VMM_WRITABLE);
    
    // Map framebuffer (physical 0xB8000) to high virtual address
    vmm_map_page(kernel_pml4, VMM_FRAMEBUFFER_VIRT, 0xB8000, VMM_PRESENT | VMM_WRITABLE);
    
    // Load the new PML4
    vmm_load_pml4(kernel_pml4);
    
    // Update framebuffer pointer to use virtual address
    vmm_framebuffer = (volatile uint16_t *)VMM_FRAMEBUFFER_VIRT;
}
