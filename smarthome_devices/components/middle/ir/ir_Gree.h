#ifndef IR_GREE_H_
#define IR_GREE_H_


#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

typedef struct
{
    union
    {
        uint8_t remote_state[kToshibaACStateLengthLong]; ///< State in code form.
        struct
        { 
            // Byte 0
            uint8_t Mode      :3;
            uint8_t Power     :1;
            uint8_t Fan       :2;
            uint8_t SwingAuto :1;
            uint8_t Sleep     :1;
            // Byte 1
            uint8_t Temp        :4;
            uint8_t TimerHalfHr :1;
            uint8_t TimerTensHr :2;
            uint8_t TimerEnabled:1;
            // Byte 2
            uint8_t TimerHours:4;
            uint8_t Turbo     :1;
            uint8_t Light     :1;
            uint8_t ModelA    :1;  // model==YAW1F
            uint8_t Xfan      :1;
            // Byte 3
            uint8_t :2;
            uint8_t TempExtraDegreeF:1;
            uint8_t UseFahrenheit   :1;
            uint8_t unknown1        :4;  // value=0b0101
            // Byte 4
            uint8_t SwingV      :4;
            uint8_t SwingH      :3;
            uint8_t             :1;
            // Byte 5
            uint8_t DisplayTemp :2;
            uint8_t IFeel       :1;
            uint8_t unknown2    :3;  // value = 0b100
            uint8_t WiFi        :1;
            uint8_t             :1;
            // Byte 6
            uint8_t             :8;
            // Byte 7
            uint8_t             :2;
            uint8_t Econo       :1;
            uint8_t             :1;
            uint8_t Sum         :4;
        };
    };
} GreeProtocol;

#define kGreeAuto 0
#define kGreeCool 1
#define kGreeDry  2
#define kGreeFan  3
#define kGreeHeat 4
#define kGreeEcono 5

#define kGreeFanAuto 0
#define kGreeFanMin 1
#define kGreeFanMed 2
#define kGreeFanMax 3

#define kGreeMinTempC 16  // Celsius
#define kGreeMaxTempC 30  // Celsius
#define kGreeMinTempF 61  // Fahrenheit
#define kGreeMaxTempF 86  // Fahrenheit
#define kGreeTimerMax (24 * 60)

#define kGreeSwingLastPos   0b0000  // 0
#define kGreeSwingAuto      0b0001  // 1
#define kGreeSwingUp        0b0010  // 2
#define kGreeSwingMiddleUp  0b0011  // 3
#define kGreeSwingMiddle    0b0100  // 4
#define kGreeSwingMiddleDown 0b0101  // 5
#define kGreeSwingDown      0b0110  // 6
#define kGreeSwingDownAuto  0b0111  // 7
#define kGreeSwingMiddleAuto 0b1001  // 9
#define kGreeSwingUpAuto    0b1011  // 11

#define kGreeSwingHOff       0b000  // 0
#define kGreeSwingHAuto      0b001  // 1
#define kGreeSwingHMaxLeft   0b010  // 2
#define kGreeSwingHLeft      0b011  // 3
#define kGreeSwingHMiddle    0b100  // 4
#define kGreeSwingHRight     0b101  // 5
#define kGreeSwingHMaxRight  0b110  // 6

#define kGreeDisplayTempOff    0b00  // 0
#define kGreeDisplayTempSet    0b01  // 1
#define kGreeDisplayTempInside 0b10  // 2
#define kGreeDisplayTempOutside 0b11  // 3

// Legacy defines.
#define GREE_AUTO kGreeAuto
#define GREE_COOL kGreeCool
#define GREE_DRY kGreeDry
#define GREE_FAN kGreeFan
#define GREE_HEAT kGreeHeat
#define GREE_MIN_TEMP kGreeMinTempC
#define GREE_MAX_TEMP kGreeMaxTempC
#define GREE_FAN_MAX kGreeFanMax
#define GREE_SWING_LAST_POS kGreeSwingLastPos
#define GREE_SWING_AUTO kGreeSwingAuto
#define GREE_SWING_UP kGreeSwingUp
#define GREE_SWING_MIDDLE_UP kGreeSwingMiddleUp
#define GREE_SWING_MIDDLE kGreeSwingMiddle
#define GREE_SWING_MIDDLE_DOWN kGreeSwingMiddleDown
#define GREE_SWING_DOWN kGreeSwingDown
#define GREE_SWING_DOWN_AUTO kGreeSwingDownAuto
#define GREE_SWING_MIDDLE_AUTO kGreeSwingMiddleAuto
#define GREE_SWING_UP_AUTO kGreeSwingUpAuto


#endif