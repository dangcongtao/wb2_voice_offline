#ifndef IR_CORONA_H_
#define IR_CORONA_H_


#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

typedef struct {
  uint8_t Header0;
  uint8_t Header1;
  uint8_t Label;
  uint8_t Data0;
  uint8_t Data0Inv;
  uint8_t Data1;
  uint8_t Data1Inv;
} CoronaSection;

#define kCoronaAcSections  3

typedef struct
{
    union
    {
        uint8_t raw[kCoronaAcStateLength];  ///< The state of the IR remote.
        CoronaSection sections[kCoronaAcSections];
        struct {
            // Byte 0
            uint8_t               :8;
            // Byte 1
            uint8_t               :8;
            // Byte 2
            uint8_t               :8;
            // Byte 3
            uint8_t Fan           :2;
            uint8_t               :1;
            uint8_t Econo         :1;
            uint8_t               :1;  // always on
            uint8_t               :1;
            uint8_t SwingVToggle  :1;
            uint8_t               :1;
            // Byte 4
            uint8_t               :8;
            // Byte 5
            uint8_t Temp          :4;
            uint8_t Power         :1;
            uint8_t PowerButton   :1;
            uint8_t Mode          :2;
        };
    };
} CoronaProtocol;


// CORONA_AC
const uint8_t kCoronaAcSectionBytes = 7;  // kCoronaAcStateLengthShort
const uint8_t kCoronaAcSectionHeader0 = 0x28;
const uint8_t kCoronaAcSectionHeader1 = 0x61;
const uint8_t kCoronaAcSectionLabelBase = 0x0D;  // 0b1101
const uint8_t kCoronaAcSectionData0Base = 0x10;  // D0 Pos 4 always on

const uint8_t kCoronaAcFanAuto =    0b00;  // 0
const uint8_t kCoronaAcFanLow =     0b01;  // 1
const uint8_t kCoronaAcFanMedium =  0b10;  // 2
const uint8_t kCoronaAcFanHigh =    0b11;  // 3

/* full auto mode not supported by this code yet
const uint8_t kCoronaAcAutoD0 = 0b00010100; // only combined with power save
const uint8_t kCoronaAcAutoD1 = 0b10000011; // only combined with power
*/
const uint8_t kCoronaAcMinTemp = 17;  // Celsius = 0b0001
const uint8_t kCoronaAcMaxTemp = 30;  // Celsius = 0b1110
const uint8_t kCoronaAcModeHeat = 0b00;  // 0
const uint8_t kCoronaAcModeDry =  0b01;  // 1
const uint8_t kCoronaAcModeCool = 0b10;  // 2
const uint8_t kCoronaAcModeFan =  0b11;  // 3

const uint8_t kCoronaAcSettingsSection = 0;
const uint8_t kCoronaAcOnTimerSection = 1;
const uint8_t kCoronaAcOffTimerSection = 2;
const uint16_t kCoronaAcTimerMax = 12 * 60;  // 12H in Minutes
// Min value on remote is 1 hour, actual sent value can be 2 secs
const uint16_t kCoronaAcTimerOff = 0xffff;
const uint16_t kCoronaAcTimerUnitsPerMin = 30;  // 30 units = 1 minute



#endif