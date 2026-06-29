#include "app_settings.hpp"
#include "gt_rx.hpp"

// Должен подключаться после app_settings.hpp,
// потому что там задаётся GT_GD32_TIMER_CLOCK_HZ.
#include "GyverTransfer/Transfer_GD32L23.h"

namespace {

GyverTransfer<APP_GT_PIN, GT_RX, APP_GT_PERIOD_US, APP_GT_BUFFER_SIZE> gt;

}

namespace GtRx {

void timebase_init(void) {
    gt_gd32_timebase_init();
}

void timebase_irq_handler(void) {
    gt_gd32_timebase_irq_handler();
}

void begin(void) {
    gt.begin();
}

uint32_t available(void) {
    return gt.available();
}

bool read_data_crc(Packet& data) {
    return gt.readDataCRC(data);
}

void clear_buffer(void) {
    gt.clearBuffer();
}

bool tick_isr(void) {
    return gt.tickISR();
}

}
