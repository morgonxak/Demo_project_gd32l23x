#include "app_settings.hpp"
#include "app_state.hpp"
#include "gt_rx.hpp"

// TIMER1 нужен библиотеке как micros()
extern "C" void TIMER1_IRQHandler(void) {
    GtRx::timebase_irq_handler();
}

// PB7 -> EXTI7 -> EXTI5_9_IRQHandler
extern "C" void EXTI5_9_IRQHandler(void) {
    if (exti_interrupt_flag_get(APP_GT_EXTI_LINE) != RESET) {
        exti_interrupt_flag_clear(APP_GT_EXTI_LINE);

        gtEdgeIrqCount++;

        // На каждый фронт отдаём управление декодеру GyverTransfer.
        // true вернётся только когда полностью принят 1 байт.
        if (GtRx::tick_isr()) {
            gtByteCount++;
        }
    }
}
