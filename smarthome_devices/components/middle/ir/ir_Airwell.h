#ifndef IR_AIRWELL_H_
#define IR_AIRWELL_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

typedef struct 
{
    union {
    uint64_t raw;  // The state of the IR remote in native IR code form.
    struct {
        uint64_t            :19;
        uint64_t Temp       :4;
        uint64_t            :5;
        uint64_t Fan        :2;
        uint64_t Mode       :3;
        uint64_t PowerToggle:1;
        uint64_t            :0;
    };
    };
} AirwellProtocol;

// Constants
const uint64_t kAirwellKnownGoodState = 0x140500002;  // Mode Fan, Speed 1, 25C
// Temperature
const uint8_t kAirwellMinTemp = 16;  // Celsius
const uint8_t kAirwellMaxTemp = 30;  // Celsius
// Fan
const uint8_t kAirwellFanLow = 0;     // 0b00
const uint8_t kAirwellFanMedium = 1;  // 0b01
const uint8_t kAirwellFanHigh = 2;    // 0b10
const uint8_t kAirwellFanAuto = 3;    // 0b11
// Modes
const uint8_t kAirwellCool = 1;  // 0b001
const uint8_t kAirwellHeat = 2;  // 0b010
const uint8_t kAirwellAuto = 3;  // 0b011
const uint8_t kAirwellDry = 4;   // 0b100
const uint8_t kAirwellFan = 5;   // 0b101


#endif