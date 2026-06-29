#include "dac_noise.hpp"
#include "app_settings.hpp"
#include "app_state.hpp"

uint16_t freq_to_dac_value(uint8_t freq) {
    // 0..255 -> 0..2048
    // 0 -> 0, 255 -> 2048
    return (uint16_t)(((uint32_t)freq * APP_DAC_VALUE_MAX + 127U) / 255U);
}

uint8_t band_clamp(uint8_t band) {
    if (band > APP_DAC_BAND_MAX) {
        return APP_DAC_BAND_MAX;
    }

    return band;
}

uint32_t band_to_lfsr_mask(uint8_t band) {
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
    rcu_periph_clock_enable(APP_DAC_GPIO_CLK);
    gpio_mode_set(APP_DAC_GPIO_PORT, GPIO_MODE_ANALOG, GPIO_PUPD_NONE, APP_DAC_GPIO_PIN);
}

static void timer5_config(void) {
    timer_parameter_struct timer_initpara;

    rcu_periph_clock_enable(APP_DAC_TIMER_CLK);
    timer_deinit(APP_DAC_TIMER);

    timer_struct_para_init(&timer_initpara);

    timer_initpara.prescaler        = APP_DAC_TIMER_PRESCALER;
    timer_initpara.alignedmode      = TIMER_COUNTER_EDGE;
    timer_initpara.counterdirection = TIMER_COUNTER_UP;
    timer_initpara.period           = APP_DAC_TIMER_PERIOD;
    timer_initpara.clockdivision    = TIMER_CKDIV_DIV1;

    timer_init(APP_DAC_TIMER, &timer_initpara);

    // TIMER5 update event -> TRGO -> DAC trigger
    timer_master_output_trigger_source_select(APP_DAC_TIMER, TIMER_TRI_OUT_SRC_UPDATE);

    timer_enable(APP_DAC_TIMER);
}

static void dac_config(void) {
    rcu_periph_clock_enable(APP_DAC_PERIPH_CLK);

    dac_deinit(APP_DAC);

    dac_trigger_source_config(APP_DAC, APP_DAC_OUT, APP_DAC_TRIGGER);
    dac_trigger_enable(APP_DAC, APP_DAC_OUT);

    dac_wave_mode_config(APP_DAC, APP_DAC_OUT, DAC_WAVE_MODE_LFSR);
    dac_lfsr_noise_config(APP_DAC, APP_DAC_OUT, DAC_LFSR_BIT0);

    dac_enable(APP_DAC, APP_DAC_OUT);
    dac_data_set(APP_DAC, APP_DAC_OUT, APP_DAC_ALIGN, 0U);
}

void dac_apply_packet(const Packet& data) {
    const uint16_t dacValue = freq_to_dac_value(data.freq);
    const uint8_t bandValue = band_clamp(data.band);

    dac_lfsr_noise_config(APP_DAC, APP_DAC_OUT, band_to_lfsr_mask(bandValue));
    dac_data_set(APP_DAC, APP_DAC_OUT, APP_DAC_ALIGN, dacValue);

    lastDacValue = dacValue;
    lastBandClamped = bandValue;
}

void dac_hw_init(void) {
    dac_gpio_config();
    dac_config();
    timer5_config();
}
