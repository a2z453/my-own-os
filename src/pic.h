#pragma once
#include <stdint.h>

void pic_init(uint8_t offset_master, uint8_t offset_slave);
void pic_mask_all(void);
