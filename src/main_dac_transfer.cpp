#include "gd32l23x.h"

#define GT_GD32_TIMER_CLOCK_HZ 64000000UL
#include "GyverTransfer/Transfer_GD32L23.h"

// ===================== LED PB1 =====================

#define LED_GPIO_PORT   GPIOB
#define LED_GPIO_PIN    GPIO_PIN_1
#define LED_GPIO_CLK    RCU_GPIOB

static void led_init(void) {
    rcu_periph_clock_enable(LED_GPIO_CLK);

    gpio_mode_set(
        LED_GPIO_PORT,
        GPIO_MODE_OUTPUT,
        GPIO_PUPD_NONE,
        LED_GPIO_PIN
    );

    gpio_output_options_set(
        LED_GPIO_PORT,
        GPIO_OTYPE_PP,
        GPIO_OSPEED_50MHZ,
        LED_GPIO_PIN
    );

    gpio_bit_reset(LED_GPIO_PORT, LED_GPIO_PIN);
}

static void led_toggle(void) {
    static uint8_t state = 0;
    state = !state;

    if (state) {
        gpio_bit_set(LED_GPIO_PORT, LED_GPIO_PIN);
    } else {
        gpio_bit_reset(LED_GPIO_PORT, LED_GPIO_PIN);
    }
}

// ===================== GyverTransfer =====================

// freq: 0..256 -> DAC center/offset 0..2048
// band: 0..12, где 12 принудительно ограничивается до DAC_LFSR_BITS11_0
// set : пока только принимаем и сохраняем, не используем
struct __attribute__((packed)) Packet {
    uint8_t freq;
    uint8_t band;
    uint8_t set;
};

// static_assert(sizeof(Packet) == 2U, "Packet size must be 2 bytes: uint8_t + uint8_t + uint8_t");

// Arduino D2 -> GD32 PB7
static constexpr uint8_t GT_PIN = GT_PB(7);

// Arduino: GyverTransfer<2, GT_TX, 1000> tx;
GyverTransfer<GT_PIN, GT_RX, 1000, 16> gt;

Packet lastPacket;

volatile uint8_t lastFreq = 0;
volatile uint8_t lastBand = 0;
volatile uint8_t lastSet = 0;
volatile uint16_t lastDacValue = 0;
volatile uint8_t lastBandClamped = 0;

volatile uint32_t mainLoopCount = 0;
volatile uint32_t gtEdgeIrqCount = 0;
volatile uint32_t gtByteCount = 0;
volatile uint32_t gtAvailable = 0;
volatile uint32_t gtPacketOkCount = 0;
volatile uint32_t gtPacketBadCount = 0;

// ===================== DAC PA4 + TIMER5 TRGO =====================

static uint16_t freq_to_dac_value(uint8_t freq) {
    // 0..255 -> 0..2048
    // 0 -> 0, 255 -> 2048
    return (uint16_t)(((uint32_t)freq * 2048U + 127U) / 255U);
}

static uint8_t band_clamp(uint8_t band) {
    if (band > 11U) {
        return 11U;
    }
    return band;
}

static uint32_t band_to_lfsr_mask(uint8_t band) {
    static const uint32_t lfsr_table[12] = {
        DAC_LFSR_BIT0,
        DAC_LFSR_BITS1_0,
        DAC_LFSR_BITS2_0,
        DAC_LFSR_BITS3_0,
        DAC_LFSR_BITS4_0,
        DAC_LFSR_BITS5_0,
        DAC_LFSR_BITS6_0,
        DAC_LFSR_BITS7_0,
        DAC_LFSR_BITS8_0,
        DAC_LFSR_BITS9_0,
        DAC_LFSR_BITS10_0,
        DAC_LFSR_BITS11_0
    };

    return lfsr_table[band_clamp(band)];
}

static void dac_gpio_config(void) {
    // PA4 = DAC0_OUT0
    rcu_periph_clock_enable(RCU_GPIOA);
    gpio_mode_set(GPIOA, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, GPIO_PIN_4);
}

static void timer5_config(void) {
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(RCU_TIMER5);
    timer_deinit(TIMER5);

    timer_struct_para_init(&timer_initpara);
    timer_initpara.prescaler         = 0;       // 64 MHz / (63 + 1) = 1 MHz
    timer_initpara.alignedmode       = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection  = TIMER_COUNTER_UP;
    timer_initpara.period            = 1;      // 1 MHz / (999 + 1) = 1 kHz update
    timer_initpara.clockdivision     = TIMER_CKDIV_DIV1;

    timer_init(TIMER5, &timer_initpara);

    // TIMER5 update event -> TRGO -> DAC trigger
    timer_master_output_trigger_source_select(TIMER5, TIMER_TRI_OUT_SRC_UPDATE);

    timer_enable(TIMER5);
}

static void dac_config(void) {
    rcu_periph_clock_enable(RCU_DAC);

    dac_deinit(DAC0);

    dac_trigger_source_config(DAC0, DAC_OUT0, DAC_TRIGGER_T5_TRGO);
    dac_trigger_enable(DAC0, DAC_OUT0);

    dac_wave_mode_config(DAC0, DAC_OUT0, DAC_WAVE_MODE_LFSR);
    dac_lfsr_noise_config(DAC0, DAC_OUT0, DAC_LFSR_BIT0);

    dac_enable(DAC0, DAC_OUT0);
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, 0U);
}

static void dac_apply_packet(const Packet& data) {
    const uint16_t dacValue = freq_to_dac_value(data.freq);
    const uint8_t bandValue = band_clamp(data.band);

    dac_lfsr_noise_config(DAC0, DAC_OUT0, band_to_lfsr_mask(bandValue));
    dac_data_set(DAC0, DAC_OUT0, DAC_ALIGN_12B_R, dacValue);

    lastDacValue = dacValue;
    lastBandClamped = bandValue;
}

static void dac_hw_init(void) {
    dac_gpio_config();
    dac_config();
    timer5_config();
}

// ===================== EXTI PB7 =====================

static void pb7_exti_init(void) {
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_SYSCFG);

    // PB7 как вход с подтяжкой вверх
    gpio_mode_set(
        GPIOB,
        GPIO_MODE_INPUT,
        GPIO_PUPD_PULLUP,
        GPIO_PIN_7
    );

    // Привязываем EXTI7 именно к GPIOB
    syscfg_exti_line_config(EXTI_SOURCE_GPIOB, EXTI_SOURCE_PIN7);

    // EXTI7 по обоим фронтам
    exti_interrupt_flag_clear(EXTI_7);
    exti_init(EXTI_7, EXTI_INTERRUPT, EXTI_TRIG_BOTH);
    exti_interrupt_flag_clear(EXTI_7);

    // PB7 -> EXTI7 -> EXTI5_9_IRQn
    nvic_irq_enable(EXTI5_9_IRQn, 2U);

    __enable_irq();
}

// TIMER1 нужен библиотеке как micros()
extern "C" void TIMER1_IRQHandler(void) {
    gt_gd32_timebase_irq_handler();
}

// PB7 -> EXTI7 -> EXTI5_9_IRQHandler
extern "C" void EXTI5_9_IRQHandler(void) {
    if (exti_interrupt_flag_get(EXTI_7) != RESET) {
        exti_interrupt_flag_clear(EXTI_7);

        gtEdgeIrqCount++;

        // На каждый фронт отдаём управление декодеру GyverTransfer.
        // true вернётся только когда полностью принят 1 байт.
        if (gt.tickISR()) {
            gtByteCount++;
        }
    }
}

// ===================== main =====================

int main(void) {
    SystemCoreClockUpdate();

    led_init();

    // DAC0_OUT0 на PA4, LFSR noise, TIMER5 TRGO
    dac_hw_init();

    // Запускаем микросекундный таймер
    gt_gd32_timebase_init();

    // Инициализируем внутреннее состояние библиотеки и PB7
    gt.begin();

    // Включаем EXTI на PB7
    pb7_exti_init();

    while (1) {
        mainLoopCount++;

        gtAvailable = gt.available();

        // Packet = 4 байта: uint16_t freq + uint8_t band + uint8_t set
        // CRC + ~CRC = 2 байта
        // Итого ждём 6 байт
        if (gtAvailable >= sizeof(Packet) + 2U) {
            Packet data;

            if ((gtAvailable == sizeof(Packet) + 2U) && gt.readDataCRC(data)) {
                lastPacket = data;

                lastFreq = data.freq;
                lastBand = data.band;
                lastSet = data.set;

                // Применяем только freq и band, set пока не используем
                dac_apply_packet(data);

                gtPacketOkCount++;

                // Светодиод мигает только при правильном CRC-пакете
                led_toggle();
            } else {
                gtPacketBadCount++;
                gt.clearBuffer();
            }
        }
    }
}
