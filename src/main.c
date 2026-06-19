#include "gd32l23x.h"
#include "systick.h"

#define LED_GPIO_PORT    GPIOB
#define LED_GPIO_PIN     GPIO_PIN_1
#define LED_GPIO_CLK     RCU_GPIOB

#define BUT_GPIO_PORT    GPIOA
#define BUT_GPIO_PIN     GPIO_PIN_1
#define BUT_GPIO_CLK     RCU_GPIOA

int main(void)
{
    systick_config();

    rcu_periph_clock_enable(LED_GPIO_CLK);
    rcu_periph_clock_enable(BUT_GPIO_CLK);

    gpio_mode_set(LED_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, LED_GPIO_PIN);
    gpio_output_options_set(LED_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_2MHZ, LED_GPIO_PIN);

    gpio_mode_set(BUT_GPIO_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, BUT_GPIO_PIN);

    gpio_bit_set(LED_GPIO_PORT, LED_GPIO_PIN); // начальное состояние LED: выключен

    while(1)
    {
        if(gpio_input_bit_get(BUT_GPIO_PORT, BUT_GPIO_PIN) == RESET)
        {
            delay_1ms(50); // антидребезг при нажатии

            if(gpio_input_bit_get(BUT_GPIO_PORT, BUT_GPIO_PIN) == RESET)
            {
                gpio_bit_toggle(LED_GPIO_PORT, LED_GPIO_PIN);

                while(gpio_input_bit_get(BUT_GPIO_PORT, BUT_GPIO_PIN) == RESET)
                {
                    // ждём отпускания кнопки
                }

                delay_1ms(50); // антидребезг при отпускании
            }
        }
    }
}