#include "exti_pb7.hpp"
#include "app_settings.hpp"

void pb7_exti_init(void) {
    rcu_periph_clock_enable(APP_GT_GPIO_CLK);
    rcu_periph_clock_enable(RCU_SYSCFG);

    // PB7 как вход с подтяжкой вверх
    gpio_mode_set(
        APP_GT_GPIO_PORT,
        GPIO_MODE_INPUT,
        GPIO_PUPD_PULLUP,
        APP_GT_GPIO_PIN
    );

    // Привязываем EXTI7 именно к GPIOB
    syscfg_exti_line_config(APP_GT_EXTI_SOURCE_PORT, APP_GT_EXTI_SOURCE_PIN);

    // EXTI7 по обоим фронтам
    exti_interrupt_flag_clear(APP_GT_EXTI_LINE);
    exti_init(APP_GT_EXTI_LINE, EXTI_INTERRUPT, EXTI_TRIG_BOTH);
    exti_interrupt_flag_clear(APP_GT_EXTI_LINE);

    // PB7 -> EXTI7 -> EXTI5_9_IRQn
    nvic_irq_enable(APP_GT_EXTI_IRQN, APP_GT_EXTI_IRQ_PRIORITY);

    __enable_irq();
}
