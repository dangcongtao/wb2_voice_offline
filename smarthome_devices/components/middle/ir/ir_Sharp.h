#ifndef IR_SHARP_H_
#define IR_SHARP_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

typedef struct
{
    union
    {

        uint8_t raw[kSharpAcStateLength]; ///< State in code form.
        struct
        {
            // Byte 0~3
            uint8_t pad[4];
            // Byte 4
            uint8_t Temp  :4;
            uint8_t Model :1;
            uint8_t Model_temp :3;
            // Byte 5
            uint8_t       :4;
            uint8_t PowerSpecial  :4;
            // Byte 6
            uint8_t Mode  :2;
            uint8_t       :1;
            uint8_t Clean :1;
            uint8_t Fan   :3;
            uint8_t       :1;
            // Byte 7
            uint8_t TimerHours  :4;
            uint8_t             :2;
            uint8_t TimerType   :1;
            uint8_t TimerEnabled:1;
            // Byte 8
            uint8_t Swing :3;
            uint8_t       :5;
            // Byte 9
            uint8_t       :8;
            // Byte 10
            uint8_t Special :8;
            // Byte 11
            uint8_t        :2;
            uint8_t Ion    :1;
            uint8_t        :1;
            uint8_t Model2 :1;
            uint8_t Model3 :3;
            // Byte 12
            uint8_t     :4;
            uint8_t Sum :4;
        };
    };
} SharpProtocol;


// Constants
#define kSharpAcHdrMark  3800
#define kSharpAcHdrSpace  1900
#define kSharpAcBitMark  470
#define kSharpAcZeroSpace  500
#define kSharpAcOneSpace  1400
#define kSharpAcGap  kDefaultMessageGap

#define kSharpAcByteTemp  4
#define kSharpAcMinTemp  15  // Celsius
#define kSharpAcMaxTemp  30  // Celsius

#define kSharpAcPowerUnknown  0                     // 0b0000
#define kSharpAcPowerOnFromOff  1                   // 0b0001
#define kSharpAcPowerOff  2                         // 0b0010
#define kSharpAcPowerOn  3                          // 0b0011 (Normal)
#define kSharpAcPowerSetSpecialOn  6                // 0b0110
#define kSharpAcPowerSetSpecialOff  7               // 0b0111
#define kSharpAcPowerTimerSetting  8                // 0b1000

#define kSharpAcAuto                              0b00  // A907 only
#define kSharpAcFan                               0b00  // A705 only
#define kSharpAcDry                               0b11
#define kSharpAcCool                              0b10
#define kSharpAcHeat                              0b01  // A907 only
#define kSharpAcFanAuto                      0b010  // 2
#define kSharpAcFanMin                       0b100  // 4 (FAN1)
#define kSharpAcFanMed                       0b011  // 3 (FAN2)
#define kSharpAcFanA705Low                   0b011  // 3 (A903 too)
#define kSharpAcFanHigh                      0b101  // 5 (FAN3)
#define kSharpAcFanA705Med                   0b101  // 5 (A903 too)
#define kSharpAcFanMax                       0b111  // 7 (FAN4)

#define kSharpAcTimerIncrement  30  // Mins
#define kSharpAcTimerHoursOff                              0b0000
#define kSharpAcTimerHoursMax                              0b1100  // 12
#define kSharpAcOffTimerType                            0b0
#define kSharpAcOnTimerType                             0b1

// Ref: https://github.com/crankyoldgit/IRremoteESP8266/discussions/1590#discussioncomment-1260213
#define kSharpAcSwingVIgnore  0b000  // Don't change the swing setting.
#define kSharpAcSwingVHigh    0b001  // 0° down. Similar to Cool Coanda.
#define kSharpAcSwingVOff     0b010  // Stop & Go to last fixed pos.
#define kSharpAcSwingVMid     0b011  // 30° down
#define kSharpAcSwingVLow     0b100  // 45° down
#define kSharpAcSwingVLast    0b101  // Same as kSharpAcSwingVOff.
// Toggles between last fixed pos & either 75° down (Heat) or 0° down (Cool)
// i.e. alternate between last pos <-> 75° down if in Heat mode, AND
//      alternate between last pos <-> 0° down if in Cool mode.
// Note: `setSwingV(kSharpAcSwingVLowest)` will only allow the Lowest setting in
//       Heat mode, it will default to `kSharpAcSwingVLow` otherwise.
//       If you want to set this value in other modes e.g. Cool, you must
//       use `setSwingV`s optional `force` parameter.
#define kSharpAcSwingVLowest  0b110
#define kSharpAcSwingVCoanda  kSharpAcSwingVLowest
#define kSharpAcSwingVToggle  0b111  // Toggle Constant swinging on/off.

#define kSharpAcSpecialPower               0x00
#define kSharpAcSpecialTurbo               0x01
#define kSharpAcSpecialTempEcono           0x04
#define kSharpAcSpecialFan                 0x05
#define kSharpAcSpecialSwing               0x06
#define kSharpAcSpecialTimer               0xC0
#define kSharpAcSpecialTimerHalfHour       0xDE


#endif