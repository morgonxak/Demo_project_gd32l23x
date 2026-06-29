// #include "gd32l23x.h"

// #define GT_GD32_TIMER_CLOCK_HZ 64000000UL
// #include "GyverTransfer/Transfer_GD32L23.h"

// // ===================== LED PB1 =====================

// #define LED_GPIO_PORT   GPIOB
// #define LED_GPIO_PIN    GPIO_PIN_1
// #define LED_GPIO_CLK    RCU_GPIOB

// static void led_init(void) {
//     rcu_periph_clock_enable(LED_GPIO_CLK);

//     gpio_mode_set(
//         LED_GPIO_PORT,
//         GPIO_MODE_OUTPUT,
//         GPIO_PUPD_NONE,
//         LED_GPIO_PIN
//     );

//     gpio_output_options_set(
//         LED_GPIO_PORT,
//         GPIO_OTYPE_PP,
//         GPIO_OSPEED_50MHZ,
//         LED_GPIO_PIN
//     );

//     gpio_bit_reset(LED_GPIO_PORT, LED_GPIO_PIN);
// }

// static void led_toggle(void) {
//     static uint8_t state = 0;
//     state = !state;

//     if (state) {
//         gpio_bit_set(LED_GPIO_PORT, LED_GPIO_PIN);
//     } else {
//         gpio_bit_reset(LED_GPIO_PORT, LED_GPIO_PIN);
//     }
// }

// // ===================== GyverTransfer =====================

// struct Packet {
//     uint8_t freq;
//     uint8_t band;
//     uint8_t set;
// };

// // Arduino D2 -> GD32 PB7
// static constexpr uint8_t GT_PIN = GT_PB(7);

// // Arduino: GyverTransfer<2, GT_TX, 1000> tx;
// GyverTransfer<GT_PIN, GT_RX, 1000, 16> gt;

// Packet lastPacket;

// volatile uint8_t lastFreq = 0;
// volatile uint8_t lastBand = 0;
// volatile uint8_t lastSet = 0;

// volatile uint32_t mainLoopCount = 0;
// volatile uint32_t gtEdgeIrqCount = 0;
// volatile uint32_t gtByteCount = 0;
// volatile uint32_t gtAvailable = 0;
// volatile uint32_t gtPacketOkCount = 0;
// volatile uint32_t gtPacketBadCount = 0;

// // ===================== EXTI PB7 =====================

// static void pb7_exti_init(void) {
//     rcu_periph_clock_enable(RCU_GPIOB);
//     rcu_periph_clock_enable(RCU_SYSCFG);

//     // PB7 как вход с подтяжкой вверх
//     gpio_mode_set(
//         GPIOB,
//         GPIO_MODE_INPUT,
//         GPIO_PUPD_PULLUP,
//         GPIO_PIN_7
//     );

//     // Привязываем EXTI7 именно к GPIOB
//     syscfg_exti_line_config(EXTI_SOURCE_GPIOB, EXTI_SOURCE_PIN7);

//     // EXTI7 по обоим фронтам
//     exti_interrupt_flag_clear(EXTI_7);
//     exti_init(EXTI_7, EXTI_INTERRUPT, EXTI_TRIG_BOTH);
//     exti_interrupt_flag_clear(EXTI_7);

//     // PB7 -> EXTI7 -> EXTI5_9_IRQn
//     nvic_irq_enable(EXTI5_9_IRQn, 2U);

//     __enable_irq();
// }

// // TIMER1 нужен библиотеке как micros()
// extern "C" void TIMER1_IRQHandler(void) {
//     gt_gd32_timebase_irq_handler();
// }

// // PB7 -> EXTI7 -> EXTI5_9_IRQHandler
// extern "C" void EXTI5_9_IRQHandler(void) {
//     if (exti_interrupt_flag_get(EXTI_7) != RESET) {
//         exti_interrupt_flag_clear(EXTI_7);

//         gtEdgeIrqCount++;

//         // На каждый фронт отдаём управление декодеру GyverTransfer.
//         // true вернётся только когда полностью принят 1 байт.
//         if (gt.tickISR()) {
//             gtByteCount++;
//         }
//     }
// }
// void dac_config(void)
// {
//     /* initialize DAC */
//     dac_deinit(DAC0);
//     /* DAC trigger config */
//     dac_trigger_source_config(DAC0, DAC_OUT0, DAC_TRIGGER_T5_TRGO);
//     /* DAC trigger enable */
//     dac_trigger_enable(DAC0, DAC_OUT0);
//     /* DAC wave mode config */
//     dac_wave_mode_config(DAC0, DAC_OUT0, DAC_WAVE_MODE_LFSR);
//     dac_lfsr_noise_config(DAC0, DAC_OUT0, DAC_LFSR_BITS10_0);

//     /* DAC enable */
//     dac_enable(DAC0, DAC_OUT0);
//     dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, 0x7F0);
// }

// // ===================== main =====================

// int main(void) {
//     SystemCoreClockUpdate();

//     led_init();

//     // Запускаем микросекундный таймер
//     gt_gd32_timebase_init();

//     // Инициализируем внутреннее состояние библиотеки и PB7
//     gt.begin();

//     // Включаем EXTI на PB7
//     pb7_exti_init();

//     while (1) {
//         mainLoopCount++;

//         gtAvailable = gt.available();

//         // Packet = 3 байта
//         // CRC + ~CRC = 2 байта
//         // Итого ждём 5 байт
//         if (gtAvailable >= sizeof(Packet) + 2U) {
//             Packet data;

//             if ((gtAvailable == sizeof(Packet) + 2U) && gt.readDataCRC(data)) {
//                 lastPacket = data;

//                 lastFreq = data.freq;
//                 lastBand = data.band;
//                 lastSet = data.set;

//                 gtPacketOkCount++;

//                 // Светодиод мигает только при правильном CRC-пакете
//                 led_toggle();
//             } else {
//                 gtPacketBadCount++;
//                 gt.clearBuffer();
//             }
//         }
//     }
// }