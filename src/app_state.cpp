#include "app_state.hpp"

Packet lastPacket {};

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

volatile uint32_t settingsFlashLoadOkCount = 0;
volatile uint32_t settingsFlashLoadBadCount = 0;
volatile uint32_t settingsFlashSaveCount = 0;
volatile uint32_t settingsFlashSaveSkipCount = 0;
volatile uint32_t settingsFlashSaveErrorCount = 0;
