#ifndef IR_PANASONIC_H_
#define IR_PANASONIC_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

const uint16_t kPanasonicFreq = 36700;
const uint16_t kPanasonicAcExcess = 0;
// Much higher than usual. See issue #540.
const uint16_t kPanasonicAcTolerance = 40;

const uint8_t kPanasonicAcAuto = 0;  // 0b000
const uint8_t kPanasonicAcDry = 2;   // 0b010
const uint8_t kPanasonicAcCool = 3;  // 0b011
const uint8_t kPanasonicAcHeat = 4;  // 0b010
const uint8_t kPanasonicAcFan = 6;   // 0b110
const uint8_t kPanasonicAcFanMin = 0;
const uint8_t kPanasonicAcFanLow = 1;
const uint8_t kPanasonicAcFanMed = 2;
const uint8_t kPanasonicAcFanHigh = 3;
const uint8_t kPanasonicAcFanMax = 4;
const uint8_t kPanasonicAcFanAuto = 7;
const uint8_t kPanasonicAcFanDelta = 3;
const uint8_t kPanasonicAcPowerOffset = 0;
const uint8_t kPanasonicAcTempOffset = 1;  // Bits
const uint8_t kPanasonicAcTempSize = 5;  // Bits
const uint8_t kPanasonicAcMinTemp = 16;      // Celsius
const uint8_t kPanasonicAcMaxTemp = 30;      // Celsius
const uint8_t kPanasonicAcFanModeTemp = 27;  // Celsius
const uint8_t kPanasonicAcQuietOffset = 0;
const uint8_t kPanasonicAcPowerfulOffset = 5;   // 0b100000
// CKP & RKR models have Powerful and Quiet bits swapped.
const uint8_t kPanasonicAcQuietCkpOffset = kPanasonicAcPowerfulOffset;
const uint8_t kPanasonicAcPowerfulCkpOffset = kPanasonicAcQuietOffset;
const uint8_t kPanasonicAcSwingVHighest = 0x1;  // 0b0001
const uint8_t kPanasonicAcSwingVHigh = 0x2;     // 0b0010
const uint8_t kPanasonicAcSwingVMiddle = 0x3;   // 0b0011
const uint8_t kPanasonicAcSwingVLow = 0x4;      // 0b0100
const uint8_t kPanasonicAcSwingVLowest = 0x5;   // 0b0101
const uint8_t kPanasonicAcSwingVAuto = 0xF;     // 0b1111

const uint8_t kPanasonicAcSwingHMiddle = 0x6;     // 0b0110
const uint8_t kPanasonicAcSwingHFullLeft = 0x9;   // 0b1001
const uint8_t kPanasonicAcSwingHLeft = 0xA;       // 0b1010
const uint8_t kPanasonicAcSwingHRight = 0xB;      // 0b1011
const uint8_t kPanasonicAcSwingHFullRight = 0xC;  // 0b1100
const uint8_t kPanasonicAcSwingHAuto = 0xD;       // 0b1101
const uint8_t kPanasonicAcChecksumInit = 0xF4;
const uint8_t kPanasonicAc2ChecksumInit = 0xF2;
const uint8_t kPanasonicAcOnTimerOffset = 1;
const uint8_t kPanasonicAcOffTimerOffset = 2;
const uint8_t kPanasonicAcTimeSize = 11;  // Bits
const uint8_t kPanasonicAcTimeOverflowSize = 3;  // Bits
const uint16_t kPanasonicAcTimeMax = 23 * 60 + 59;  // Mins since midnight.
const uint16_t kPanasonicAcTimeSpecial = 0x600;

const uint8_t kPanasonicAcIonFilterByte = 22;  // Byte
const uint8_t kPanasonicAcIonFilterOffset = 0;  // Bit

const uint8_t kPanasonicKnownGoodState[kPanasonicAcStateLength] = {
    0x02, 0x20, 0xE0, 0x04, 0x00, 0x00, 0x00, 0x06, 0x02,
    0x20, 0xE0, 0x04, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00,
    0x00, 0x0E, 0xE0, 0x00, 0x00, 0x81, 0x00, 0x00, 0x00};

const uint8_t kPanasonic2KnownGoodState[kPanasonicAcStateLength] = {
    0x02, 0x20, 0xE0, 0x05, 0x00, 0x00, 0x00, 0x07, 0x02, 0x20, 0xE0, 0x05, 0x00, 0x39, 0x30, 0x80, 0xAF, 0x00, 0x00, 0x0E, 0xE0, 0x00, 0x00, 0x89, 0x00, 0x04, 0x1A};

typedef struct
{
    union {
    uint32_t raw;  ///< The state in IR code form.
    struct {
        // Byte 0
        uint8_t             :3;
        uint8_t SwingH      :1;
        uint8_t SwingV      :3;
        uint8_t             :1;  ///< Always appears to be set. (1)
        // Byte 1
        uint8_t             :8;  // Always seems to be 0x36.
        // Byte 2
        uint8_t Temp        :4;
        uint8_t Fan         :4;
        // Byte 3
        uint8_t Mode        :3;
        uint8_t PowerToggle :1;  // 0 means toggle, 1 = keep the same.
        uint8_t             :4;
    };
    };
} PanasonicAc32Protocol;

const uint8_t kPanasonicAc32Fan =  1;  // 0b001
const uint8_t kPanasonicAc32Cool = 2;  // 0b010
const uint8_t kPanasonicAc32Dry =  3;  // 0b011
const uint8_t kPanasonicAc32Heat = 4;  // 0b010
const uint8_t kPanasonicAc32Auto = 6;  // 0b110

const uint8_t kPanasonicAc32FanMin =  2;
const uint8_t kPanasonicAc32FanLow =  3;
const uint8_t kPanasonicAc32FanMed =  4;
const uint8_t kPanasonicAc32FanHigh = 5;
const uint8_t kPanasonicAc32FanMax =  6;
const uint8_t kPanasonicAc32FanAuto = 0xF;
const uint8_t kPanasonicAc32SwingVAuto = 0x7;   // 0b111
const uint32_t kPanasonicAc32KnownGood = 0x0AF136FC;  ///< Cool, Auto, 16C


#endif