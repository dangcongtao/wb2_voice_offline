#ifndef IR_ELECTRA_H_
#define IR_ELECTRA_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>
typedef struct 
{
union {
  uint8_t raw[kElectraAcStateLength];   ///< The state of the IR remote
  struct {
    // Byte 0
    uint8_t         :8;
    // Byte 1
    uint8_t SwingV  :3;
    uint8_t Temp    :5;
    // Byte 2
    uint8_t         :5;
    uint8_t SwingH  :3;
    // Byte 3
    uint8_t              :6;
    uint8_t SensorUpdate :1;
    uint8_t              :1;
    // Byte 4
    uint8_t         :5;
    uint8_t Fan     :3;
    // Byte 5
    uint8_t         :6;
    uint8_t Turbo   :1;
    uint8_t Quiet   :1;
    // Byte 6
    uint8_t         :3;
    uint8_t IFeel   :1;
    uint8_t         :1;
    uint8_t Mode    :3;
    // Byte 7
    uint8_t SensorTemp :8;
    // Byte 8
    uint8_t         :8;
    // Byte 9
    uint8_t         :2;
    uint8_t Clean   :1;
    uint8_t         :2;
    uint8_t Power   :1;
    uint8_t         :2;
    // Byte 10
    uint8_t         :8;
    // Byte 11
    uint8_t LightToggle :8;
    // Byte 12
    uint8_t Sum     :8;
  };
};
} ElectraProtocol;

// Constants
const uint8_t kElectraAcMinTemp = 16;   // 16C
const uint8_t kElectraAcMaxTemp = 32;   // 32C
const uint8_t kElectraAcTempDelta = 8;
const uint8_t kElectraAcSwingOn =  0b000;
const uint8_t kElectraAcSwingOff = 0b111;

const uint8_t kElectraAcFanAuto =    0b101;
const uint8_t kElectraAcFanLow =     0b011;
const uint8_t kElectraAcFanMed =     0b010;
const uint8_t kElectraAcFanHigh =    0b001;

const uint8_t kElectraAcAuto =     0b000;
const uint8_t kElectraAcCool =     0b001;
const uint8_t kElectraAcDry =      0b010;
const uint8_t kElectraAcHeat =     0b100;
const uint8_t kElectraAcFan =      0b110;

const uint8_t kElectraAcLightToggleOn  = 0x15;
// Light has known ON values of 0x15 (0b00010101) or 0x19 (0b00011001)
//   Thus common bits ON are: 0b00010001 (0x11)
// We will use this for the getLightToggle() test.
const uint8_t kElectraAcLightToggleMask = 0x11;
// and known OFF values of 0x08 (0b00001000) & 0x05 (0x00000101)
const uint8_t kElectraAcLightToggleOff = 0x08;

// Re: Byte[7]. Or Delta == 0xA and Temperature are stored in last 6 bits,
// and bit 7 stores Unknown flag
const uint8_t kElectraAcSensorTempDelta = 0x4A;
const uint8_t kElectraAcSensorMinTemp = 0;    // 0C
const uint8_t kElectraAcSensorMaxTemp = 50;   // 50C


#endif