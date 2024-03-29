#ifndef IR_NEOCLIMA_H_
#define IR_NEOCLIMA_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>


typedef struct 
{
    union {
        uint8_t raw[kNeoclimaStateLength];  ///< State of the remote in code.
        struct {
            // Byte 0
            uint8_t       :8;
            // Byte 1
            uint8_t       :1;
            uint8_t CHeat :1;
            uint8_t Ion   :1;
            uint8_t       :5;
            // Byte 2
            uint8_t       :8;
            // Byte 3
            uint8_t Light :1;
            uint8_t       :1;
            uint8_t Hold  :1;
            uint8_t Turbo :1;
            uint8_t Econo :1;
            uint8_t       :1;
            uint8_t Eye   :1;
            uint8_t       :1;
            // Byte 4
            uint8_t       :8;
            // Byte 5
            uint8_t Button  :5;
            uint8_t         :2;
            uint8_t Fresh   :1;
            // Byte 6
            uint8_t         :8;
            // Byte 7
            uint8_t Sleep   :1;
            uint8_t Power   :1;
            uint8_t SwingV  :2;
            uint8_t SwingH  :1;
            uint8_t Fan     :2;
            uint8_t UseFah  :1;
            // Byte 8
            uint8_t Follow  :8;
            // Byte 9
            uint8_t Temp  :5;
            uint8_t Mode  :3;
            // Byte 10
            uint8_t     :8;
            // Byte 11
            uint8_t Sum :8;
        };
    };
} NeoclimaProtocol;


// Constants

const uint8_t kNeoclimaButtonPower =    0x00;
const uint8_t kNeoclimaButtonMode =     0x01;
const uint8_t kNeoclimaButtonTempUp =   0x02;
const uint8_t kNeoclimaButtonTempDown = 0x03;
const uint8_t kNeoclimaButtonSwing =    0x04;
const uint8_t kNeoclimaButtonFanSpeed = 0x05;
const uint8_t kNeoclimaButtonAirFlow =  0x07;
const uint8_t kNeoclimaButtonHold =     0x08;
const uint8_t kNeoclimaButtonSleep =    0x09;
const uint8_t kNeoclimaButtonTurbo =    0x0A;
const uint8_t kNeoclimaButtonLight =    0x0B;
const uint8_t kNeoclimaButtonEcono =    0x0D;
const uint8_t kNeoclimaButtonEye =      0x0E;
const uint8_t kNeoclimaButtonFollow =   0x13;
const uint8_t kNeoclimaButtonIon =      0x14;
const uint8_t kNeoclimaButtonFresh =    0x15;
const uint8_t kNeoclimaButton8CHeat =   0x1D;
const uint8_t kNeoclimaButtonTempUnit = 0x1E;

const uint8_t kNeoclimaSwingVOn =                            0b01;
const uint8_t kNeoclimaSwingVOff =                           0b10;
const uint8_t kNeoclimaFanAuto =                          0b00;
const uint8_t kNeoclimaFanHigh =                          0b01;
const uint8_t kNeoclimaFanMed =                           0b10;
const uint8_t kNeoclimaFanLow =                           0b11;

const uint8_t kNeoclimaFollowMe = 0x5D;  // Also 0x5F

const uint8_t kNeoclimaMinTempC = 16;   // 16C
const uint8_t kNeoclimaMaxTempC = 32;   // 32C
const uint8_t kNeoclimaMinTempF = 61;   // 61F
const uint8_t kNeoclimaMaxTempF = 90;   // 90F
const uint8_t kNeoclimaAuto =     0b000;
const uint8_t kNeoclimaCool =     0b001;
const uint8_t kNeoclimaDry =      0b010;
const uint8_t kNeoclimaFan =      0b011;
const uint8_t kNeoclimaHeat =     0b100;


#endif