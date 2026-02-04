#include <stdint.h>
#include <stddef.h>
#include "multiboot2.h"
#include "pmm.h"

extern uint8_t _kernel_end;

static uint8_t *bitmap;
static uint64_t bitmap_bytes;
static uint64_t total_pages;
static uint64_t used_pages;

#define PAGE_SIZE 4096

static inline void set_bit(uint64_t idx)   { bitmap[idx >> 3] |=  (1u << (idx & 7)); }
static inline void clear_bit(uint64_t idx) { bitmap[idx >> 3] &= ~(1u << (idx & 7)); }
static inline int  test_bit(uint64_t idx)  { return (bitmap[idx >> 3] >> (idx & 7)) & 1u; }

static void mark_range_used(uint64_t addr, uint64_t len) {
    uint64_t start_page = addr / PAGE_SIZE;
    uint64_t end_page   = (addr + len + PAGE_SIZE - 1) / PAGE_SIZE;
    if (end_page > total_pages) end_page = total_pages;
    for (uint64_t p = start_page; p < end_page; ++p) {
        if (!test_bit(p)) {
            set_bit(p);
            used_pages++;
        }
    }
}

static void parse_mmap(uint64_t mb_info_addr, uint64_t *out_highest) {
    struct multiboot2_info_header *hdr = (struct multiboot2_info_header *)(uintptr_t)mb_info_addr;
    uint8_t *tag_ptr = (uint8_t *)(hdr + 1);
    uint8_t *end     = (uint8_t *)hdr + hdr->total_size;
    *out_highest = 0;

    while (tag_ptr < end) {
        struct multiboot2_tag *tag = (struct multiboot2_tag *)tag_ptr;
        if (tag->type == MULTIBOOT2_TAG_TYPE_END) break;

        if (tag->type == MULTIBOOT2_TAG_TYPE_MMAP) {
            struct multiboot2_tag_mmap *mmap_tag = (struct multiboot2_tag_mmap *)tag;
            uint8_t *entry_ptr = (uint8_t *)(mmap_tag + 1);
            uint8_t *entry_end = (uint8_t *)tag + tag->size;

            while (entry_ptr < entry_end) {
                struct multiboot2_mmap_entry *e = (struct multiboot2_mmap_entry *)entry_ptr;
                uint64_t last = e->addr + e->len;
                if (last > *out_highest)
                    *out_highest = last;
                entry_ptr += mmap_tag->entry_size;
            }
        }

        tag_ptr += (tag->size + 7) & ~7u;
    }
}

void pmm_init(uint64_t mb_info_addr) {
    uint64_t highest;
    parse_mmap(mb_info_addr, &highest);
    total_pages = (highest + PAGE_SIZE - 1) / PAGE_SIZE;

    bitmap_bytes = (total_pages + 7) / 8;
    uintptr_t bitmap_start = ((uintptr_t)&_kernel_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    bitmap = (uint8_t *)bitmap_start;

    // Initially mark all pages used
    for (uint64_t i = 0; i < bitmap_bytes; ++i) bitmap[i] = 0xFF;
    used_pages = total_pages;

    // Walk mmap again and mark AVAILABLE pages free
    struct multiboot2_info_header *hdr = (struct multiboot2_info_header *)(uintptr_t)mb_info_addr;
    uint8_t *tag_ptr = (uint8_t *)(hdr + 1);
    uint8_t *end     = (uint8_t *)hdr + hdr->total_size;

    while (tag_ptr < end) {
        struct multiboot2_tag *tag = (struct multiboot2_tag *)tag_ptr;
        if (tag->type == MULTIBOOT2_TAG_TYPE_END) break;

        if (tag->type == MULTIBOOT2_TAG_TYPE_MMAP) {
            struct multiboot2_tag_mmap *mmap_tag = (struct multiboot2_tag_mmap *)tag;
            uint8_t *entry_ptr = (uint8_t *)(mmap_tag + 1);
            uint8_t *entry_end = (uint8_t *)tag + tag->size;

            while (entry_ptr < entry_end) {
                struct multiboot2_mmap_entry *e = (struct multiboot2_mmap_entry *)entry_ptr;
                if (e->type == MULTIBOOT2_MEMORY_AVAILABLE) {
                    // Mark this range free
                    uint64_t start_page = e->addr / PAGE_SIZE;
                    uint64_t end_page   = (e->addr + e->len) / PAGE_SIZE;
                    if (end_page > total_pages) end_page = total_pages;
                    for (uint64_t p = start_page; p < end_page; ++p) {
                        if (test_bit(p)) {
                            clear_bit(p);
                            used_pages--;
                        }
                    }
                }
                entry_ptr += mmap_tag->entry_size;
            }
        }

        tag_ptr += (tag->size + 7) & ~7u;
    }

    // Reserve low memory (<1 MiB)
    mark_range_used(0, 0x100000);

    // Reserve kernel + bitmap region
    uintptr_t kernel_start = 0x00100000; // we link at 1M
    uintptr_t kernel_end   = (uintptr_t)bitmap + bitmap_bytes;
    mark_range_used(kernel_start, kernel_end - kernel_start);

    // Reserve multiboot info structure
    uint64_t mbi_size = ((struct multiboot2_info_header *)(uintptr_t)mb_info_addr)->total_size;
    mark_range_used(mb_info_addr, mbi_size);
}

void *pmm_alloc(void) {
    for (uint64_t p = 0; p < total_pages; ++p) {
        if (!test_bit(p)) {
            set_bit(p);
            used_pages++;
            return (void *)(uintptr_t)(p * PAGE_SIZE);
        }
    }
    return NULL;
}

void pmm_free(void *page) {
    uintptr_t addr = (uintptr_t)page;
    uint64_t p = addr / PAGE_SIZE;
    if (p >= total_pages) return;
    if (test_bit(p)) {
        clear_bit(p);
        used_pages--;
    }
}

uint64_t pmm_total_bytes(void) {
    return total_pages * PAGE_SIZE;
}

uint64_t pmm_free_bytes(void) {
    return (total_pages - used_pages) * PAGE_SIZE;
}
