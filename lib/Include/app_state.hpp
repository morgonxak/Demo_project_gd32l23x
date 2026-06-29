#pragma once

#include <stdint.h>
#include "packet.hpp"

extern Packet lastPacket;

extern volatile uint8_t lastFreq;
extern volatile uint8_t lastBand;
extern volatile uint8_t lastSet;
extern volatile uint16_t lastDacValue;
extern volatile uint8_t lastBandClamped;

extern volatile uint32_t mainLoopCount;
extern volatile uint32_t gtEdgeIrqCount;
extern volatile uint32_t gtByteCount;
extern volatile uint32_t gtAvailable;
extern volatile uint32_t gtPacketOkCount;
extern volatile uint32_t gtPacketBadCount;

extern volatile uint32_t settingsFlashLoadOkCount;
extern volatile uint32_t settingsFlashLoadBadCount;
extern volatile uint32_t settingsFlashSaveCount;
extern volatile uint32_t settingsFlashSaveSkipCount;
extern volatile uint32_t settingsFlashSaveErrorCount;
