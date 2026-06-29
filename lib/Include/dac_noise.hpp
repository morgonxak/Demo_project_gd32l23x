#pragma once

#include <stdint.h>
#include "packet.hpp"

uint16_t freq_to_dac_value(uint8_t freq);
uint8_t band_clamp(uint8_t band);
uint32_t band_to_lfsr_mask(uint8_t band);

void dac_hw_init(void);
void dac_apply_packet(const Packet& data);
