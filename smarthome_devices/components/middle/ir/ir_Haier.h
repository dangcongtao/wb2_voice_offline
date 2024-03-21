#ifndef IR_HAIER_H_
#define IR_HAIER_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

// Constants

const uint8_t kHaierAcPrefix = 0b10100101;

const uint8_t kHaierAcMinTemp = 16;
const uint8_t kHaierAcDefTemp = 25;
const uint8_t kHaierAcMaxTemp = 30;
const uint8_t kHaierAcCmdOff =         0b0000;
const uint8_t kHaierAcCmdOn =          0b0001;
const uint8_t kHaierAcCmdMode =        0b0010;
const uint8_t kHaierAcCmdFan =         0b0011;
const uint8_t kHaierAcCmdTempUp =      0b0110;
const uint8_t kHaierAcCmdTempDown =    0b0111;
const uint8_t kHaierAcCmdSleep =       0b1000;
const uint8_t kHaierAcCmdTimerSet =    0b1001;
const uint8_t kHaierAcCmdTimerCancel = 0b1010;
const uint8_t kHaierAcCmdHealth =      0b1100;
const uint8_t kHaierAcCmdSwing =       0b1101;

const uint8_t kHaierAcSwingVOff =  0b00;
const uint8_t kHaierAcSwingVUp =   0b01;
const uint8_t kHaierAcSwingVDown = 0b10;
const uint8_t kHaierAcSwingVChg =  0b11;

const uint8_t kHaierAcAuto = 0;
const uint8_t kHaierAcCool = 1;
const uint8_t kHaierAcDry = 2;
const uint8_t kHaierAcHeat = 3;
const uint8_t kHaierAcFan = 4;

const uint8_t kHaierAcFanAuto = 0;
const uint8_t kHaierAcFanLow = 1;
const uint8_t kHaierAcFanMed = 2;
const uint8_t kHaierAcFanHigh = 3;

const uint16_t kHaierAcMaxTime = (23 * 60) + 59;

const uint8_t kHaierAcSleepBit = 0b01000000;

// Aqua

const uint8_t kHaierAcYrw02MinTempC = 16;
const uint8_t kHaierAcYrw02MaxTempC = 30;
const uint8_t kHaierAcYrw02MinTempF = 60;
const uint8_t kHaierAcYrw02MaxTempF = 86;
const uint8_t kHaierAcYrw02DefTempC = 25;

const uint8_t kHaierAcYrw02ModelA = 0xA6;
const uint8_t kHaierAcYrw02ModelB = 0x59;
const uint8_t kHaierAc176Prefix = 0xB7;
const uint8_t kHaierAc160Prefix = 0xB5;

const uint8_t kHaierAcYrw02SwingVOff = 0x0;
const uint8_t kHaierAcYrw02SwingVTop = 0x1;
const uint8_t kHaierAcYrw02SwingVMiddle = 0x2;  // Not available in heat mode.
const uint8_t kHaierAcYrw02SwingVBottom = 0x3;  // Only available in heat mode.
const uint8_t kHaierAcYrw02SwingVDown = 0xA;
const uint8_t kHaierAcYrw02SwingVAuto = 0xC;  // Airflow

const uint8_t kHaierAc160SwingVOff =     0b0000;
const uint8_t kHaierAc160SwingVTop =     0b0001;
const uint8_t kHaierAc160SwingVHighest = 0b0010;
const uint8_t kHaierAc160SwingVHigh =    0b0100;
const uint8_t kHaierAc160SwingVMiddle =  0b0110;
const uint8_t kHaierAc160SwingVLow =     0b1000;
const uint8_t kHaierAc160SwingVLowest =  0b0011;
const uint8_t kHaierAc160SwingVAuto =    0b1100;  // Airflow

const uint8_t kHaierAcYrw02SwingHMiddle = 0x0;
const uint8_t kHaierAcYrw02SwingHLeftMax = 0x3;
const uint8_t kHaierAcYrw02SwingHLeft = 0x4;
const uint8_t kHaierAcYrw02SwingHRight = 0x5;
const uint8_t kHaierAcYrw02SwingHRightMax = 0x6;
const uint8_t kHaierAcYrw02SwingHAuto = 0x7;

const uint8_t kHaierAcYrw02FanHigh = 0b001;
const uint8_t kHaierAcYrw02FanMed =  0b010;
const uint8_t kHaierAcYrw02FanLow =  0b011;
const uint8_t kHaierAcYrw02FanAuto = 0b101;  // HAIER_AC176 uses `0` in Fan2

const uint8_t kHaierAcYrw02Auto = 0b000;  // 0
const uint8_t kHaierAcYrw02Cool = 0b001;  // 1
const uint8_t kHaierAcYrw02Dry =  0b010;  // 2
const uint8_t kHaierAcYrw02Heat = 0b100;  // 4
const uint8_t kHaierAcYrw02Fan =  0b110;  // 5

const uint8_t kHaierAcYrw02ButtonTempUp =   0b00000;
const uint8_t kHaierAcYrw02ButtonTempDown = 0b00001;
const uint8_t kHaierAcYrw02ButtonSwingV =   0b00010;
const uint8_t kHaierAcYrw02ButtonSwingH =   0b00011;
const uint8_t kHaierAcYrw02ButtonFan =      0b00100;
const uint8_t kHaierAcYrw02ButtonPower =    0b00101;
const uint8_t kHaierAcYrw02ButtonMode =     0b00110;
const uint8_t kHaierAcYrw02ButtonHealth =   0b00111;
const uint8_t kHaierAcYrw02ButtonTurbo =    0b01000;
const uint8_t kHaierAcYrw02ButtonSleep =    0b01011;
const uint8_t kHaierAcYrw02ButtonTimer =    0b10000;
const uint8_t kHaierAcYrw02ButtonLock =     0b10100;
const uint8_t kHaierAc160ButtonLight =      0b10101;
const uint8_t kHaierAc160ButtonAuxHeating = 0b10110;
const uint8_t kHaierAc160ButtonClean =      0b11001;
const uint8_t kHaierAcYrw02ButtonCFAB =     0b11010;

const uint8_t kHaierAcYrw02NoTimers       = 0b000;
const uint8_t kHaierAcYrw02OffTimer       = 0b001;
const uint8_t kHaierAcYrw02OnTimer        = 0b010;
const uint8_t kHaierAcYrw02OnThenOffTimer = 0b100;
const uint8_t kHaierAcYrw02OffThenOnTimer = 0b101;


typedef struct
{
    union
    {
        uint8_t remote_state[kHaierACStateLength];
        struct {
            // Byte 0
            uint8_t Prefix;
            // Byte 1
            uint8_t Command:4;
            uint8_t Temp   :4;
            // Byte 2
            uint8_t CurrHours:5;
            uint8_t unknown  :1;  // value=1
            uint8_t SwingV   :2;
            // Byte 3
            uint8_t CurrMins:6;
            uint8_t OffTimer:1;
            uint8_t OnTimer :1;
            // Byte 4
            uint8_t OffHours:5;
            uint8_t Health  :1;
            uint8_t :0;
            // Byte 5
            uint8_t OffMins:6;
            uint8_t Fan    :2;
            // Byte 6
            uint8_t OnHours:5;
            uint8_t Mode   :3;
            // Byte 7
            uint8_t OnMins:6;
            uint8_t Sleep :1;
            uint8_t :0;
            // Byte 8
            uint8_t Sum;
        };
    };
} HaierProtocol;


typedef struct
{
    union
    {
        uint8_t raw[kHaierAC176StateLength];  ///< The state in native form
        struct {
            // Byte 0
            uint8_t Model       :8;
            // Byte 1
            uint8_t SwingV      :4;
            uint8_t Temp        :4;  // 16C~30C
            // Byte 2
            uint8_t             :5;
            uint8_t SwingH      :3;
            // Byte 3
            uint8_t             :1;
            uint8_t Health      :1;
            uint8_t             :3;
            uint8_t TimerMode   :3;
            // Byte 4
            uint8_t             :6;
            uint8_t Power       :1;
            uint8_t             :1;
            // Byte 5
            uint8_t OffTimerHrs :5;
            uint8_t Fan         :3;
            // Byte 6
            uint8_t OffTimerMins:6;
            uint8_t Turbo       :1;
            uint8_t Quiet       :1;
            // Byte 7
            uint8_t OnTimerHrs  :5;
            uint8_t Mode        :3;
            // Byte 8
            uint8_t OnTimerMins :6;
            uint8_t             :1;
            uint8_t Sleep       :1;
            // Byte 9
            uint8_t             :8;
            // Byte 10
            uint8_t ExtraDegreeF :1;
            uint8_t              :4;
            uint8_t UseFahrenheit:1;
            uint8_t              :2;
            // Byte 11
            uint8_t             :8;
            // Byte 12
            uint8_t Button      :5;
            uint8_t Lock        :1;
            uint8_t             :2;
            // Byte 13
            uint8_t Sum         :8;
            // Byte 14
            uint8_t Prefix2     :8;
            // Byte 15
            uint8_t             :8;
            // Byte 16
            uint8_t             :6;
            uint8_t Fan2        :2;
            // Byte 17
            uint8_t             :8;
            // Byte 18
            uint8_t             :8;
            // Byte 19
            uint8_t             :8;
            // Byte 20
            uint8_t             :8;
            // Byte 21
            uint8_t Sum2        :8;
        };
    };
} HaierAc176Protocol;


#endif