#pragma once

#include <stdint.h>

// freq: 0..255 -> DAC 0..2048
// band: 0..12, всё выше 11 зажимается до DAC_LFSR_BITS11_0
// set : 255 = сохранить freq/band во Flash, остальные значения только принимаются
struct __attribute__((packed)) Packet {
    uint8_t freq;
    uint8_t band;
    uint8_t set;
};

static_assert(sizeof(Packet) == 3U, "Packet size must be 3 bytes");
