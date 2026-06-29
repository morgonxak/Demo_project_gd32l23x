#include "app_settings.hpp"
#include "app_state.hpp"
#include "led.hpp"
#include "dac_noise.hpp"
#include "gt_rx.hpp"
#include "exti_pb7.hpp"
#include "settings_flash.hpp"

int main(void) {
    SystemCoreClockUpdate();

    led_init();

    // DAC0_OUT0 на PA4, LFSR noise, TIMER5 TRGO
    dac_hw_init();

    // Загружаем сохранённые freq/band из Flash
    SettingsFlash::init();

    if (SettingsFlash::has_valid_values()) {
        const SettingsFlash::Values saved = SettingsFlash::current_values();

        Packet savedPacket {};
        savedPacket.freq = saved.freq;
        savedPacket.band = saved.band;
        savedPacket.set = 0U;

        lastPacket = savedPacket;

        lastFreq = savedPacket.freq;
        lastBand = savedPacket.band;
        lastSet = savedPacket.set;

        dac_apply_packet(savedPacket);

        settingsFlashLoadOkCount++;
    } else {
        settingsFlashLoadBadCount++;
    }

    // Запускаем микросекундный таймер
    GtRx::timebase_init();

    // Инициализируем внутреннее состояние библиотеки
    GtRx::begin();

    // Включаем EXTI на PB7
    pb7_exti_init();

    while (1) {
        mainLoopCount++;

        gtAvailable = GtRx::available();

        // Packet = 3 байта: uint8_t freq + uint8_t band + uint8_t set
        // CRC + ~CRC = 2 байта
        // Итого ждём sizeof(Packet) + 2 байта
        if (gtAvailable >= sizeof(Packet) + APP_GT_CRC_SIZE_BYTES) {
            Packet data {};

            if (
                (gtAvailable == sizeof(Packet) + APP_GT_CRC_SIZE_BYTES) &&
                GtRx::read_data_crc(data)
            ) {
                lastPacket = data;

                lastFreq = data.freq;
                lastBand = data.band;
                lastSet = data.set;

                // Применяем freq/band к DAC
                dac_apply_packet(data);

                // set == 255 — команда сохранить freq/band во Flash.
                // Если такие freq/band уже сохранены, Flash не стирается и не пишется.
                if (data.set == APP_PACKET_SET_SAVE_TO_FLASH) {
                    const SettingsFlash::SaveResult saveResult =
                        SettingsFlash::save_if_needed(data.freq, data.band);

                    if (saveResult == SettingsFlash::SaveResult::Saved) {
                        settingsFlashSaveCount++;
                    } else if (saveResult == SettingsFlash::SaveResult::NotNeeded) {
                        settingsFlashSaveSkipCount++;
                    } else {
                        settingsFlashSaveErrorCount++;
                    }
                }

                gtPacketOkCount++;

                // Светодиод мигает только при правильном CRC-пакете
                led_toggle();
            } else {
                gtPacketBadCount++;
                GtRx::clear_buffer();
            }
        }
    }
}
