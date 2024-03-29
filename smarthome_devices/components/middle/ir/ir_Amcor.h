#ifndef IR_AMCOR_H_
#define IR_AMCOR_H_


#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

typedef struct
{
    union
    {
        uint8_t raw[kAmcorStateLength];  // The state of the IR remote.
        struct {
            // Byte 0
            uint8_t       :8;  // Typically 0x01
            // Byte 1
            uint8_t Mode  :3;
            uint8_t       :1;
            uint8_t Fan   :3;
            uint8_t       :1;
            // Byte 2
            uint8_t       :1;
            uint8_t Temp  :6;
            uint8_t       :1;
            // Byte 3
            uint8_t       :8;
            // Byte 4
            uint8_t       :8;
            // Byte 5
            uint8_t       :4;
            uint8_t Power :4;
            // Byte 6
            uint8_t Max   :2;
            uint8_t       :4;
            uint8_t Vent  :2;
            // Byte 7
            uint8_t Sum   :8;
        };
    };
} AmcorProtocol;


// Constants

// Fan Control
const uint8_t kAmcorFanMin =   0b001;
const uint8_t kAmcorFanMed =   0b010;
const uint8_t kAmcorFanMax =   0b011;
const uint8_t kAmcorFanAuto =  0b100;
// Modes
const uint8_t kAmcorCool =     0b001;
const uint8_t kAmcorHeat =     0b010;
const uint8_t kAmcorFan =      0b011;  // Aka "Vent"
const uint8_t kAmcorDry =      0b100;
const uint8_t kAmcorAuto =     0b101;

// Temperature
const uint8_t kAmcorMinTemp = 12;  // Celsius
const uint8_t kAmcorMaxTemp = 32;  // Celsius

// Power
const uint8_t kAmcorPowerOn =   0b0011;  // 0x3
const uint8_t kAmcorPowerOff =  0b1100;  // 0xC

// Max Mode (aka "Lo" in Cool and "Hi" in Heat)
const uint8_t kAmcorMax = 0b11;

// "Vent" Mode
const uint8_t kAmcorVentOn =  0b11;


#endif