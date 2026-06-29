#include "led.hpp"
#include "app_settings.hpp"

void led_init(void) {
    rcu_periph_clock_enable(APP_LED_GPIO_CLK);

    gpio_mode_set(
        APP_LED_GPIO_PORT,
        GPIO_MODE_OUTPUT,
        GPIO_PUPD_NONE,
        APP_LED_GPIO_PIN
    );

    gpio_output_options_set(
        APP_LED_GPIO_PORT,
        GPIO_OTYPE_PP,
        GPIO_OSPEED_50MHZ,
        APP_LED_GPIO_PIN
    );

    gpio_bit_reset(APP_LED_GPIO_PORT, APP_LED_GPIO_PIN);
}

void led_toggle(void) {
    static uint8_t state = 0;
    state = !state;

    if (state) {
        gpio_bit_set(APP_LED_GPIO_PORT, APP_LED_GPIO_PIN);
    } else {
        gpio_bit_reset(APP_LED_GPIO_PORT, APP_LED_GPIO_PIN);
    }
}
