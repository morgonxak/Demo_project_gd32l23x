#pragma once

#include <stdint.h>
#include "packet.hpp"

namespace GtRx {

void timebase_init(void);
void timebase_irq_handler(void);

void begin(void);

uint32_t available(void);
bool read_data_crc(Packet& data);
void clear_buffer(void);

bool tick_isr(void);

}
