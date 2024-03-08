// Copyright 2018-2020 David Conran
/// @file
/// @brief Support for Hitachi A/C protocols.
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/417
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/453
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/973
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/1056
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/1060
/// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/1134

// Supports:
//   Brand: Hitachi,  Model: RAS-35THA6 remote
//   Brand: Hitachi,  Model: LT0541-HTA remote  (HITACHI_AC1)
//   Brand: Hitachi,  Model: Series VI A/C (Circa 2007) (HITACHI_AC1)
//   Brand: Hitachi,  Model: RAR-8P2 remote (HITACHI_AC424)
//   Brand: Hitachi,  Model: RAS-AJ25H A/C (HITACHI_AC424)
//   Brand: Hitachi,  Model: PC-LH3B (HITACHI_AC3)
//   Brand: Hitachi,  Model: KAZE-312KSDP A/C (HITACHI_AC1)
//   Brand: Hitachi,  Model: R-LT0541-HTA/Y.K.1.1-1 V2.3 remote (HITACHI_AC1)
//   Brand: Hitachi,  Model: RAS-22NK A/C (HITACHI_AC344)
//   Brand: Hitachi,  Model: RF11T1 remote (HITACHI_AC344)

#ifndef IR_HITACHI_H_
#define IR_HITACHI_H_

#include <stdint.h>
#include "IRremoteWB2.h"
#include "IRsend.h"

/// Native representation of a Hitachi 224-bit A/C message.
union HitachiProtocol{
  uint8_t raw[kHitachiAcStateLength];  ///< The state in native code.
  struct {
    // Byte 0~9
    uint8_t pad0[10];
    // Byte 10
    uint8_t Mode  :8;
    // Byte 11
    uint8_t Temp  :8;
    // Byte 12
    uint8_t       :8;
    // Byte 13
    uint8_t Fan   :8;
    // Byte 14
    uint8_t         :7;
    uint8_t SwingV  :1;
    // Byte 15
    uint8_t         :7;
    uint8_t SwingH  :1;
    // Byte 16
    uint8_t       :8;
    // Byte 17
    uint8_t Power :1;
    uint8_t       :7;
    // Byte 18~26
    uint8_t pad1[9];
    // Byte 27
    uint8_t Sum   :8;
  };
};

// Constants
#define kHitachiAcFreq      38000   // Hz.
#define kHitachiAcAuto      2
#define kHitachiAcHeat      3
#define kHitachiAcCool      4
#define kHitachiAcDry       5
#define kHitachiAcFan       0xC
#define kHitachiAcFanAuto   1
#define kHitachiAcFanLow    2
#define kHitachiAcFanMed    3
#define kHitachiAcFanHigh   5
#define kHitachiAcMinTemp   16  // 16C
#define kHitachiAcMaxTemp   32  // 32C
#define kHitachiAcAutoTemp  23  // 23C

/// Native representation of a Hitachi 53-byte/424-bit A/C message.
union Hitachi424Protocol{
  uint8_t raw[kHitachiAc424StateLength];  ///< The state in native code
  struct {
    // Byte 0~10
    uint8_t pad0[11];
    // Byte 11
    uint8_t Button  :8;
    // Byte 12
    uint8_t       :8;
    // Byte 13
    uint8_t       :2;
    uint8_t Temp  :6;
    // Byte 14~24
    uint8_t pad1[11];
    // Byte 25
    uint8_t Mode  :4;
    uint8_t Fan   :4;
    // Byte 26
    uint8_t       :8;
    // Byte 27
    uint8_t Power :8;
    // Byte 28~34
    uint8_t pad2[7];
    // Byte 35
    uint8_t SwingH  :3;
    uint8_t         :5;
    // Byte 36
    uint8_t         :8;
    // Byte 37
    uint8_t         :5;
    uint8_t SwingV  :1;
    uint8_t         :2;
  };
};

// HitachiAc424 & HitachiAc344
#define kHitachiAc424ButtonPowerMode    0x13
#define kHitachiAc424ButtonFan          0x42
#define kHitachiAc424ButtonTempDown     0x43
#define kHitachiAc424ButtonTempUp       0x44
#define kHitachiAc424ButtonSwingV       0x81
#define kHitachiAc424ButtonSwingH       0x8C
#define kHitachiAc344ButtonPowerMode    kHitachiAc424ButtonPowerMode
#define kHitachiAc344ButtonFan          kHitachiAc424ButtonFan
#define kHitachiAc344ButtonTempDown     kHitachiAc424ButtonTempDown
#define kHitachiAc344ButtonTempUp       kHitachiAc424ButtonTempUp
#define kHitachiAc344ButtonSwingV       kHitachiAc424ButtonSwingV
#define kHitachiAc344ButtonSwingH       kHitachiAc424ButtonSwingH

#define kHitachiAc424MinTemp    16  // 16C
#define kHitachiAc424MaxTemp    32  // 32C
#define kHitachiAc344MinTemp    kHitachiAc424MinTemp
#define kHitachiAc344MaxTemp    kHitachiAc424MaxTemp
#define kHitachiAc424FanTemp    27  // 27C

#define kHitachiAc424Fan        1
#define kHitachiAc424Cool       3
#define kHitachiAc424Dry        5
#define kHitachiAc424Heat       6
#define kHitachiAc344Fan        kHitachiAc424Fan
#define kHitachiAc344Cool       kHitachiAc424Cool
#define kHitachiAc344Dry        kHitachiAc424Dry
#define kHitachiAc344Heat       kHitachiAc424Heat

#define kHitachiAc424FanMin     1
#define kHitachiAc424FanLow     2
#define kHitachiAc424FanMedium  3
#define kHitachiAc424FanHigh    4
#define kHitachiAc424FanAuto    5
#define kHitachiAc424FanMax     6
#define kHitachiAc424FanMaxDry  2
#define kHitachiAc344FanMin     kHitachiAc424FanMin
#define kHitachiAc344FanLow     kHitachiAc424FanLow
#define kHitachiAc344FanMedium  kHitachiAc424FanMedium
#define kHitachiAc344FanHigh    kHitachiAc424FanHigh
#define kHitachiAc344FanAuto    kHitachiAc424FanAuto
#define kHitachiAc344FanMax     kHitachiAc424FanMax

#define kHitachiAc424PowerOn    0xF1
#define kHitachiAc424PowerOff   0xE1

#define kHitachiAc344SwingHAuto     0   // 0b000
#define kHitachiAc344SwingHRightMax 1   // 0b001
#define kHitachiAc344SwingHRight    2   // 0b010
#define kHitachiAc344SwingHMiddle   3   // 0b011
#define kHitachiAc344SwingHLeft     4   // 0b100
#define kHitachiAc344SwingHLeftMax  5   // 0b101


/// Native representation of a Hitachi 104-bit A/C message.
union Hitachi1Protocol{
  uint8_t raw[kHitachiAc1StateLength];  ///< The state in native code.
  struct {
    // Byte 0~2
    uint8_t pad[3];
    // Byte 3
    uint8_t        :6;
    uint8_t Model  :2;
    // Byte 4
    uint8_t        :8;
    // Byte 5
    uint8_t Fan    :4;
    uint8_t Mode   :4;
    // Byte 6
    uint8_t        :2;
    uint8_t Temp   :5;  // stored in LSB order.
    uint8_t        :1;
    // Byte 7
    uint8_t OffTimerLow :8;  // nr. of minutes
    // Byte 8
    uint8_t OffTimerHigh :8;  // & in LSB order.
    // Byte 9
    uint8_t OnTimerLow :8;  // nr. of minutes
    // Byte 10
    uint8_t OnTimerHigh :8;  // & in LSB order.
    // Byte 11
    uint8_t SwingToggle  :1;
    uint8_t Sleep        :3;
    uint8_t PowerToggle  :1;
    uint8_t Power        :1;
    uint8_t SwingV       :1;
    uint8_t SwingH       :1;
    // Byte 12
    uint8_t Sum :8;
  };
};

// HitachiAc1
// Model
#define kHitachiAc1Model_A      0b10
#define kHitachiAc1Model_B      0b01

// Mode & Fan
#define kHitachiAc1Dry          0b0010  // 2
#define kHitachiAc1Fan          0b0100  // 4
#define kHitachiAc1Cool         0b0110  // 6
#define kHitachiAc1Heat         0b1001  // 9
#define kHitachiAc1Auto         0b1110  // 14
#define kHitachiAc1FanAuto      1   // 0b0001
#define kHitachiAc1FanHigh      2   // 0b0010
#define kHitachiAc1FanMed       4   // 0b0100
#define kHitachiAc1FanLow       8   // 0b1000

// Temp
#define kHitachiAc1TempSize     5   // Mask 0b01111100
#define kHitachiAc1TempDelta    7
#define kHitachiAc1TempAuto     25  // Celsius
// Timer
#define kHitachiAc1TimerSize    16  // Mask 0b1111111111111111
// Sleep
#define kHitachiAc1SleepOff 0b000
#define kHitachiAc1Sleep1   0b001
#define kHitachiAc1Sleep2   0b010
#define kHitachiAc1Sleep3   0b011
#define kHitachiAc1Sleep4   0b100
// Checksum
#define kHitachiAc1ChecksumStartByte    5


#endif  // IR_HITACHI_H_
