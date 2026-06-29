#include "settings_flash.hpp"
#include "app_settings.hpp"

#include "gd32l23x.h"

namespace {

struct __attribute__((packed)) FlashRecord {
    uint32_t magic;
    uint8_t version;
    uint8_t freq;
    uint8_t band;
    uint8_t checksum;
};

static_assert(sizeof(FlashRecord) == 8U, "FlashRecord must be exactly 8 bytes");

static_assert(
    (APP_SETTINGS_FLASH_PAGE_ADDR % APP_SETTINGS_FLASH_PAGE_SIZE) == 0U,
    "APP_SETTINGS_FLASH_PAGE_ADDR must be aligned to flash page size"
);

bool s_valid = false;
SettingsFlash::Values s_values {0U, 0U};

uint8_t clamp_band_for_flash(uint8_t band) {
    if (band > APP_DAC_BAND_MAX) {
        return APP_DAC_BAND_MAX;
    }

    return band;
}

uint8_t calc_checksum(uint8_t freq, uint8_t band) {
    return (uint8_t)(
        0xA5U ^
        APP_SETTINGS_VERSION ^
        freq ^
        band ^
        (uint8_t)(APP_SETTINGS_MAGIC >> 0U) ^
        (uint8_t)(APP_SETTINGS_MAGIC >> 8U) ^
        (uint8_t)(APP_SETTINGS_MAGIC >> 16U) ^
        (uint8_t)(APP_SETTINGS_MAGIC >> 24U)
    );
}

FlashRecord make_record(uint8_t freq, uint8_t band) {
    band = clamp_band_for_flash(band);

    FlashRecord record {};
    record.magic = APP_SETTINGS_MAGIC;
    record.version = APP_SETTINGS_VERSION;
    record.freq = freq;
    record.band = band;
    record.checksum = calc_checksum(freq, band);

    return record;
}

bool record_is_valid(const FlashRecord& record) {
    if (record.magic != APP_SETTINGS_MAGIC) {
        return false;
    }

    if (record.version != APP_SETTINGS_VERSION) {
        return false;
    }

    if (record.band > APP_DAC_BAND_MAX) {
        return false;
    }

    if (record.checksum != calc_checksum(record.freq, record.band)) {
        return false;
    }

    return true;
}

FlashRecord read_record_raw(void) {
    const auto* ptr = reinterpret_cast<const volatile FlashRecord*>(
        APP_SETTINGS_FLASH_PAGE_ADDR
    );

    FlashRecord record {};
    record.magic = ptr->magic;
    record.version = ptr->version;
    record.freq = ptr->freq;
    record.band = ptr->band;
    record.checksum = ptr->checksum;

    return record;
}

void fmc_flags_clear_local(void) {
    fmc_flag_clear(FMC_FLAG_END);
    fmc_flag_clear(FMC_FLAG_WPERR);
    fmc_flag_clear(FMC_FLAG_PGAERR);
    fmc_flag_clear(FMC_FLAG_PGERR);
}

bool flash_program_record(const FlashRecord& record) {
    fmc_state_enum state = FMC_READY;

    union {
        FlashRecord record;
        uint32_t words[2];
    } data {};

    data.record = record;

    const uint32_t primask = __get_PRIMASK();
    __disable_irq();

    fmc_unlock();
    fmc_flags_clear_local();

    state = fmc_page_erase(APP_SETTINGS_FLASH_PAGE_ADDR);

    if (state == FMC_READY) {
        fmc_flags_clear_local();

        // GD32L232: пишем по 32 бита.
        state = fmc_word_program(
            APP_SETTINGS_FLASH_PAGE_ADDR,
            data.words[0]
        );

        if (state == FMC_READY) {
            state = fmc_word_program(
                APP_SETTINGS_FLASH_PAGE_ADDR + 4U,
                data.words[1]
            );
        }
    }

    fmc_flags_clear_local();
    fmc_lock();

    if (primask == 0U) {
        __enable_irq();
    }

    if (state != FMC_READY) {
        return false;
    }

    const FlashRecord verify = read_record_raw();

    return (
        record_is_valid(verify) &&
        verify.freq == record.freq &&
        verify.band == record.band
    );
}

}

namespace SettingsFlash {

void init(void) {
    Values values {};

    if (load(values)) {
        s_valid = true;
        s_values = values;
    } else {
        s_valid = false;
        s_values = {128U, 10U};
    }
}

bool load(Values& values) {
    const FlashRecord record = read_record_raw();

    if (!record_is_valid(record)) {
        return false;
    }

    values.freq = record.freq;
    values.band = record.band;

    return true;
}

SaveResult save_if_needed(uint8_t freq, uint8_t band) {
    band = clamp_band_for_flash(band);

    // Защита от повторной перезаписи:
    // если такие freq/band уже лежат во Flash — ничего не стираем и не пишем.
    if (s_valid && s_values.freq == freq && s_values.band == band) {
        return SaveResult::NotNeeded;
    }

    const FlashRecord record = make_record(freq, band);

    if (!flash_program_record(record)) {
        return SaveResult::Error;
    }

    s_valid = true;
    s_values.freq = freq;
    s_values.band = band;

    return SaveResult::Saved;
}

bool has_valid_values(void) {
    return s_valid;
}

Values current_values(void) {
    return s_values;
}

}
