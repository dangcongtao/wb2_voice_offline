#ifndef IR_TCL_H_
#define IR_TCL_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>


typedef struct
{
    union
    {
        uint8_t raw[kTcl112AcStateLength]; ///< State in code form.
        struct
        { 
            // Byte 0~2
            uint8_t                 :8;
            uint8_t                 :8;
            uint8_t                 :8;
            // Byte 3
            uint8_t MsgType         :2;
            uint8_t                 :6;
            // Byte 4
            uint8_t                 :8;
            // Byte 5
            uint8_t                 :2;
            uint8_t Power           :1;
            uint8_t OffTimerEnabled :1;
            uint8_t OnTimerEnabled  :1;
            uint8_t Quiet           :1;
            uint8_t Light           :1;
            uint8_t Econo           :1;
            // Byte 6
            uint8_t Mode            :4;
            uint8_t Health          :1;
            uint8_t Turbo           :1;
            uint8_t                 :2;
            // Byte 7
            uint8_t Temp            :4;
            uint8_t                 :4;
            // Byte 8
            uint8_t Fan             :3;
            uint8_t SwingV          :3;
            uint8_t TimerIndicator  :1;
            uint8_t                 :1;
            // Byte 9
            uint8_t                 :1;  // 0
            uint8_t OffTimer        :6;
            uint8_t                 :1;  // 0
            // Byte 10
            uint8_t                 :1;  // 0
            uint8_t OnTimer         :6;
            uint8_t                 :1;  // 0
            // Byte 11
            uint8_t                 :8;  // 00000000
            // Byte 12
            uint8_t                 :3;
            uint8_t SwingH          :1;
            uint8_t                 :1;
            uint8_t HalfDegree      :1;
            uint8_t                 :1;
            uint8_t isTcl           :1;
            // Byte 13
            uint8_t Sum             :8;
        };
    };
} Tcl112Protocol;

// Constants
#define kTcl112AcHdrMark  3000
#define kTcl112AcHdrSpace  1650
#define kTcl112AcBitMark  500
#define kTcl112AcOneSpace  1050
#define kTcl112AcZeroSpace  325
#define kTcl112AcGap  kDefaultMessageGap  // Just a guess.
// Total tolerance percentage to use for matching the header mark.
#define kTcl112AcHdrMarkTolerance  6
#define kTcl112AcTolerance  5  // Extra Percentage for the rest.

#define kTcl112AcHeat  1
#define kTcl112AcDry   2
#define kTcl112AcCool  3
#define kTcl112AcFan   7
#define kTcl112AcAuto  8

#define kTcl112AcFanAuto  0b000
#define kTcl112AcFanMin   0b001  // Aka. "Night"
#define kTcl112AcFanLow   0b010
#define kTcl112AcFanMed   0b011
#define kTcl112AcFanHigh  0b101
#define kTcl112AcFanNight  kTcl112AcFanMin
#define kTcl112AcFanQuiet  kTcl112AcFanMin

#define   kTcl112AcTempMax     31.0
#define   kTcl112AcTempMin     16.0

#define kTcl112AcSwingVOff      0b000
#define kTcl112AcSwingVHighest  0b001
#define kTcl112AcSwingVHigh     0b010
#define kTcl112AcSwingVMiddle   0b011
#define kTcl112AcSwingVLow      0b100
#define kTcl112AcSwingVLowest   0b101
#define kTcl112AcSwingVOn       0b111
// MsgType
#define kTcl112AcNormal   0b01
#define kTcl112AcSpecial  0b1

#endif