#pragma once
#include <stdint.h>

#define VMM_PRESENT  (1ULL << 0)
#define VMM_WRITABLE (1ULL << 1)
#define VMM_USER     (1ULL << 2)

// Virtual framebuffer address (high virtual address)
#define VMM_FRAMEBUFFER_VIRT 0xFFFF8000000B8000ULL

extern volatile uint16_t *vmm_framebuffer;

void vmm_init(void);
void vmm_map_page(uint64_t *pml4, uint64_t virt, uint64_t phys, uint64_t flags);
void vmm_identity_map(uint64_t *pml4, uint64_t start, uint64_t len, uint64_t flags);
void vmm_load_pml4(uint64_t *pml4);
uint64_t *vmm_get_pml4(void);
