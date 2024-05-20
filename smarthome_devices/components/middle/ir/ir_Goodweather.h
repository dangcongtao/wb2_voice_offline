#ifndef IR_BOSCH_H_
#define IR_BOSCH_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>


typedef struct
{
    union
    {
       uint64_t raw;  ///< The state of the IR remote in IR code form.
        struct {
            // Byte 0
            uint8_t :8;
            // Byte 1
            uint8_t Light :1;
            uint8_t       :2;
            uint8_t Turbo :1;
            uint8_t       :0;
            // Byte 2
            uint8_t Command :4;
            uint8_t         :0;
            // Byte 3
            uint8_t Sleep   :1;
            uint8_t Power   :1;
            uint8_t Swing   :2;
            uint8_t AirFlow :1;
            uint8_t Fan     :2;
            uint8_t         :0;
            // Byte 4
            uint8_t Temp  :4;
            uint8_t       :1;
            uint8_t Mode  :3;
            uint8_t       :0;
        };
    };
} GoodweatherProtocol;


// Constants
// Timing
const uint16_t kGoodweatherBitMark = 580;
const uint16_t kGoodweatherOneSpace = 580;
const uint16_t kGoodweatherZeroSpace = 1860;
const uint16_t kGoodweatherHdrMark = 6820;
const uint16_t kGoodweatherHdrSpace = 6820;
const uint8_t  kGoodweatherExtraTolerance = 12;  // +12% extra

// Modes
const uint8_t kGoodweatherAuto = 0b000;
const uint8_t kGoodweatherCool = 0b001;
const uint8_t kGoodweatherDry =  0b010;
const uint8_t kGoodweatherFan =  0b011;
const uint8_t kGoodweatherHeat = 0b100;
// Swing
const uint8_t kGoodweatherSwingFast = 0b00;
const uint8_t kGoodweatherSwingSlow = 0b01;
const uint8_t kGoodweatherSwingOff =  0b10;
// Fan Control
const uint8_t kGoodweatherFanAuto = 0b00;
const uint8_t kGoodweatherFanHigh = 0b01;
const uint8_t kGoodweatherFanMed =  0b10;
const uint8_t kGoodweatherFanLow =  0b11;
// Temperature
const uint8_t kGoodweatherTempMin = 16;  // Celsius
const uint8_t kGoodweatherTempMax = 31;  // Celsius
// Commands
const uint8_t kGoodweatherCmdPower    = 0x00;
const uint8_t kGoodweatherCmdMode     = 0x01;
const uint8_t kGoodweatherCmdUpTemp   = 0x02;
const uint8_t kGoodweatherCmdDownTemp = 0x03;
const uint8_t kGoodweatherCmdSwing    = 0x04;
const uint8_t kGoodweatherCmdFan      = 0x05;
const uint8_t kGoodweatherCmdTimer    = 0x06;
const uint8_t kGoodweatherCmdAirFlow  = 0x07;
const uint8_t kGoodweatherCmdHold     = 0x08;
const uint8_t kGoodweatherCmdSleep    = 0x09;
const uint8_t kGoodweatherCmdTurbo    = 0x0A;
const uint8_t kGoodweatherCmdLight    = 0x0B;
// PAD EOF
const uint64_t kGoodweatherStateInit  = 0xD50000000000;


#endif