#ifndef IR_COOLIX_H_
#define IR_COOLIX_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>


typedef struct
{
    union
    {
        uint32_t raw;  ///< The state in IR code form.
        struct {  // Only 24 bits are used.
            // Byte
            uint32_t            :1;  // Unknown
            uint32_t ZoneFollow1:1;  ///< Control bit for Zone Follow mode.
            uint32_t Mode       :2;  ///< Operation mode.
            uint32_t Temp       :4;  ///< Desired temperature (Celsius)
            // Byte
            uint32_t SensorTemp :5;  ///< The temperature sensor in the IR remote.
            uint32_t Fan        :3;  ///< Fan speed
            // Byte
            uint32_t            :3;  // Unknown
            uint32_t ZoneFollow2:1;  ///< Additional control bit for Zone Follow mode.
            uint32_t            :4;  ///< Fixed value 0b1011 / 0xB.
        };
    } data;
} CoolixProtocol;

// Constants
// Modes
const uint8_t kCoolixCool = 0b000;
const uint8_t kCoolixDry = 0b001;
const uint8_t kCoolixAuto = 0b010;
const uint8_t kCoolixHeat = 0b011;
const uint8_t kCoolixFan = 0b100;                                 // Synthetic.
// const uint32_t kCoolixModeMask = 0b000000000000000000001100;  // 0xC
// const uint32_t kCoolixZoneFollowMask = 0b000010000000000000000010  0x80002
// Fan Control
const uint8_t kCoolixFanMin = 0b100;
const uint8_t kCoolixFanMed = 0b010;
const uint8_t kCoolixFanMax = 0b001;
const uint8_t kCoolixFanAuto = 0b101;
const uint8_t kCoolixFanAuto0 = 0b000;
const uint8_t kCoolixFanZoneFollow = 0b110;
const uint8_t kCoolixFanFixed = 0b111;
// Temperature
const uint8_t kCoolixTempMin = 17;  // Celsius
const uint8_t kCoolixTempMax = 30;  // Celsius
#define kCoolixTempRange  (kCoolixTempMax - kCoolixTempMin + 1)
const uint8_t kCoolixFanTempCode = 0b1110;  // Part of Fan Mode.

const uint8_t kCoolixSensorTempMax = 30;  // Celsius
const uint8_t kCoolixSensorTempIgnoreCode = 0b11111;  // 0x1F / 31 (DEC)
// kCoolixSensorTempMask = 0b000000000000111100000000;  // 0xF00
// Fixed states/messages.
const uint32_t kCoolixOff    = 0b101100100111101111100000;  // 0xB27BE0
const uint32_t kCoolixSwing  = 0b101100100110101111100000;  // 0xB26BE0
const uint32_t kCoolixSwingH = 0b101100101111010110100010;  // 0xB5F5A2
const uint32_t kCoolixSwingV = 0b101100100000111111100000;  // 0xB20FE0
const uint32_t kCoolixSleep  = 0b101100101110000000000011;  // 0xB2E003
const uint32_t kCoolixTurbo  = 0b101101011111010110100010;  // 0xB5F5A2
const uint32_t kCoolixLed    = 0b101101011111010110100101;  // 0xB5F5A5
const uint32_t kCoolixClean  = 0b101101011111010110101010;  // 0xB5F5AA
const uint32_t kCoolixCmdFan = 0b101100101011111111100100;  // 0xB2BFE4
// On, 25C, Mode: Auto, Fan: Auto, Zone Follow: Off, Sensor Temp: Ignore.
const uint32_t kCoolixDefaultState = 0b101100100001111111001000;  // 0xB21FC8



#endif