#pragma once

#include <stdint.h>

namespace SettingsFlash {

struct Values {
    uint8_t freq;
    uint8_t band;
};

enum class SaveResult : uint8_t {
    Saved,
    NotNeeded,
    Error
};

void init(void);

bool load(Values& values);

// Сохраняет только если freq/band отличаются от уже сохранённых.
SaveResult save_if_needed(uint8_t freq, uint8_t band);

bool has_valid_values(void);
Values current_values(void);

}
