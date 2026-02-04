#pragma once
#include <stdint.h>

#define MULTIBOOT2_MAGIC 0x36d76289

struct multiboot2_tag {
    uint32_t type;
    uint32_t size;
};

#define MULTIBOOT2_TAG_TYPE_END         0
#define MULTIBOOT2_TAG_TYPE_MMAP        6

#define MULTIBOOT2_MEMORY_AVAILABLE     1

struct multiboot2_tag_mmap {
    uint32_t type;      // 6
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    // struct multiboot2_mmap_entry entries[];
};

struct multiboot2_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t reserved;
};

struct multiboot2_info_header {
    uint32_t total_size;
    uint32_t reserved;
};
