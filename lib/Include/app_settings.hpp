#pragma once

#include <stdint.h>
#include "gd32l23x.h"

// Важно: должно быть задано ДО подключения Transfer_GD32L23.h
#ifndef GT_GD32_TIMER_CLOCK_HZ
#define GT_GD32_TIMER_CLOCK_HZ 64000000UL
#endif

#if !defined(GD32L233)
#warning "This project is configured for GD32L232. Check MCU define in compiler settings."
#endif

// ===================== LED PB1 =====================

#define APP_LED_GPIO_PORT      GPIOB
#define APP_LED_GPIO_PIN       GPIO_PIN_1
#define APP_LED_GPIO_CLK       RCU_GPIOB

// ===================== GyverTransfer PB7 =====================

// Arduino D2 -> GD32 PB7
// Макрос GT_PB() определён в GyverTransfer/Transfer_GD32L23.h,
// поэтому APP_GT_PIN используется только после подключения этой библиотеки.
#define APP_GT_PIN             GT_PB(7)

static constexpr uint16_t APP_GT_PERIOD_US       = 1000U;
static constexpr uint8_t  APP_GT_BUFFER_SIZE     = 16U;
static constexpr uint8_t  APP_GT_CRC_SIZE_BYTES  = 2U;

// ===================== EXTI PB7 =====================

#define APP_GT_GPIO_PORT       GPIOB
#define APP_GT_GPIO_PIN        GPIO_PIN_7
#define APP_GT_GPIO_CLK        RCU_GPIOB

#define APP_GT_EXTI_SOURCE_PORT    EXTI_SOURCE_GPIOB
#define APP_GT_EXTI_SOURCE_PIN     EXTI_SOURCE_PIN7
#define APP_GT_EXTI_LINE           EXTI_7
#define APP_GT_EXTI_IRQN           EXTI5_9_IRQn

static constexpr uint8_t APP_GT_EXTI_IRQ_PRIORITY = 2U;

// ===================== DAC PA4 + TIMER5 TRGO =====================

#define APP_DAC_GPIO_PORT      GPIOA
#define APP_DAC_GPIO_PIN       GPIO_PIN_4
#define APP_DAC_GPIO_CLK       RCU_GPIOA

#define APP_DAC_PERIPH_CLK     RCU_DAC
#define APP_DAC                DAC0
#define APP_DAC_OUT            DAC_OUT0
#define APP_DAC_ALIGN          DAC_ALIGN_12B_R
#define APP_DAC_TRIGGER        DAC_TRIGGER_T5_TRGO

#define APP_DAC_TIMER          TIMER5
#define APP_DAC_TIMER_CLK      RCU_TIMER5


// При 64 MHz: 64 MHz / (0 + 1) / (1 + 1) = 32 MHz update.
// Если нужна 1 kHz, поставь prescaler = 63, period = 999.
static constexpr uint16_t APP_DAC_TIMER_PRESCALER = 0U;
static constexpr uint16_t APP_DAC_TIMER_PERIOD    = 1U;

static constexpr uint16_t APP_DAC_VALUE_MAX = 2048U;
static constexpr uint8_t  APP_DAC_BAND_MAX  = 11U;

// ===================== Flash settings storage =====================

// Команда сохранения настроек: если Packet.set == 255, сохраняем freq/band во Flash.
static constexpr uint8_t APP_PACKET_SET_SAVE_TO_FLASH = 255U;

// GD32L232: Flash page = 4 KB.
static constexpr uint32_t APP_SETTINGS_FLASH_PAGE_SIZE = 0x1000U;

// ВНИМАНИЕ:
// Эта страница будет полностью стираться при сохранении настроек.
// Адрес должен быть свободен и не должен пересекаться с прошивкой.
//
// Для GD32L232 с 32 KB Flash последняя страница обычно: 0x08007000.
// Для GD32L232 с 64 KB Flash последняя страница обычно: 0x0800F000.
// Проверь точный размер Flash своего контроллера и linker script.
static constexpr uint32_t APP_SETTINGS_FLASH_PAGE_ADDR = 0x08007000U;

static constexpr uint32_t APP_SETTINGS_MAGIC = 0x47543332UL; // 'GT32'
static constexpr uint8_t  APP_SETTINGS_VERSION = 1U;
