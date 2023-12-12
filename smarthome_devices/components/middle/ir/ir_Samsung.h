#ifndef IR_SAMSUNG_H_
#define IR_SAMSUNG_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

#define kSamsungAcMinTemp  16  // C   Mask 0b11110000
#define kSamsungAcMaxTemp  30  // C   Mask 0b11110000
#define kSamsungAcAutoTemp 25  // C   Mask 0b11110000
#define kSamsungAcAuto 0
#define kSamsungAcCool 1
#define kSamsungAcDry 2
#define kSamsungAcFan 3
#define kSamsungAcHeat 4
#define kSamsungAcFanAuto 0
#define kSamsungAcFanLow 2
#define kSamsungAcFanMed 4
#define kSamsungAcFanHigh 5
#define kSamsungAcFanAuto2 6
#define kSamsungAcFanTurbo 7
#define kSamsungAcSectionLength 7
#define kSamsungAcPowerSection 0x1D20F00000000

#define kSamsungAcExtendedStateLength 21


typedef struct
{
    union
    {

        uint8_t raw[kSamsungAcExtendedStateLength]; ///< State in code form.
        struct
        { // Standard message map
            // Byte 0
            uint8_t : 8;
            // Byte 1
            uint8_t : 4;
            uint8_t : 4; // Sum1Lower
            // Byte 2
            uint8_t : 4; // Sum1Upper
            uint8_t : 4;
            // Byte 3
            uint8_t : 8;
            // Byte 4
            uint8_t : 8;
            // Byte 5
            uint8_t : 4;
            uint8_t Sleep5 : 1;
            uint8_t Quiet : 1;
            uint8_t : 2;
            // Byte 6
            uint8_t : 4;
            uint8_t Power1 : 2;
            uint8_t : 2;
            // Byte 7
            uint8_t : 8;
            // Byte 8
            uint8_t : 4;
            uint8_t : 4; // Sum2Lower
            // Byte 9
            uint8_t : 4; // Sum1Upper
            uint8_t Swing : 3;
            uint8_t : 1;
            // Byte 10
            uint8_t : 1;
            uint8_t FanSpecial : 3; // Powerful, Breeze/WindFree, Econo
            uint8_t Display : 1;
            uint8_t : 2;
            uint8_t CleanToggle10 : 1;
            // Byte 11
            uint8_t Ion : 1;
            uint8_t CleanToggle11 : 1;
            uint8_t : 2;
            uint8_t Temp : 4;
            // Byte 12
            uint8_t : 1;
            uint8_t Fan : 3;
            uint8_t Mode : 3;
            uint8_t : 1;
            // Byte 13
            uint8_t : 2;
            uint8_t BeepToggle : 1;
            uint8_t : 1;
            uint8_t Power2 : 2;
            uint8_t : 2;
        };
        struct
        { // Extended message map
            // 1st Section
            // Byte 0
            uint8_t : 8;
            // Byte 1
            uint8_t : 4;
            uint8_t Sum1Lower : 4;
            // Byte 2
            uint8_t Sum1Upper : 4;
            uint8_t : 4;
            // Byte 3
            uint8_t : 8;
            // Byte 4
            uint8_t : 8;
            // Byte 5
            uint8_t : 8;
            // Byte 6
            uint8_t : 8;
            // 2nd Section
            // Byte 7
            uint8_t : 8;
            // Byte 8
            uint8_t : 4;
            uint8_t Sum2Lower : 4;
            // Byte 9
            uint8_t Sum2Upper : 4;
            uint8_t OffTimeMins : 3; // In units of 10's of mins
            uint8_t OffTimeHrs1 : 1; // LSB of the number of hours.
            // Byte 10
            uint8_t OffTimeHrs2 : 4; // MSBs of the number of hours.
            uint8_t OnTimeMins : 3;  // In units of 10's of mins
            uint8_t OnTimeHrs1 : 1;  // LSB of the number of hours.
            // Byte 11
            uint8_t OnTimeHrs2 : 4; // MSBs of the number of hours.
            uint8_t : 4;
            // Byte 12
            uint8_t OffTimeDay : 1;
            uint8_t OnTimerEnable : 1;
            uint8_t OffTimerEnable : 1;
            uint8_t Sleep12 : 1;
            uint8_t OnTimeDay : 1;
            uint8_t : 3;
            // Byte 13
            uint8_t : 8;
            // 3rd Section
            // Byte 14
            uint8_t : 8;
            // Byte 15
            uint8_t : 4;
            uint8_t Sum3Lower : 4;
            // Byte 16
            uint8_t Sum3Upper : 4;
            uint8_t : 4;
            // Byte 17
            uint8_t : 8;
            // Byte 18
            uint8_t : 8;
            // Byte 19
            uint8_t : 8;
            // Byte 20
            uint8_t : 8;
        };
    };
} SamsungProtocol;



#endif