#pragma once
#include <stdint.h>

void pmm_init(uint64_t mb_info_addr);
void *pmm_alloc(void);
void pmm_free(void *page);

uint64_t pmm_total_bytes(void);
uint64_t pmm_free_bytes(void);
