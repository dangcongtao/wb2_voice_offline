#ifndef IR_FUJITSU_H_
#define IR_FUJITSU_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

typedef struct 
{
    union {
        struct {
            uint8_t longcode[kFujitsuAcStateLengthLong];  ///< The state of the IR remote.
            uint8_t shortcode[kFujitsuAcStateLengthShort];
        };
        struct {
            // Byte 0~1
            uint64_t           :16;  // Fixed header
            // Byte 2
            uint64_t            :4;
            uint64_t Id         :2;   // Device Number/Identifier
            uint64_t            :2;
            // Byte 3-4
            uint64_t            :16;
            // Byte 5
            uint64_t Cmd        :8;  // short codes:cmd; long codes:fixed value
            // Byte 6
            uint64_t RestLength :8;  // Nr. of bytes in the message after this byte.
            // Byte 7
            uint64_t Protocol   :8;  // Seems like a protocol version number. Not sure.
            // Byte 8
            uint64_t Power      :1;
            uint64_t Fahrenheit :1;
            uint64_t Temp       :6;  // Internal representation varies between models.
            // Byte 9
            uint64_t Mode       :3;
            uint64_t Clean      :1;  // Also 10C Heat in ARREW4E.
            uint64_t TimerType  :2;
            uint64_t            :2;
            // Byte 10
            uint64_t Fan    :3;
            uint64_t        :1;
            uint64_t Swing  :2;
            uint64_t        :2;
            // Byte 11~13
            uint64_t OffTimer       :11;  // Also is the sleep timer value
            uint64_t OffTimerEnable :1;
            uint64_t OnTimer        :11;
            uint64_t OnTimerEnable  :1;
            // Byte 14
            uint64_t              :3;
            uint64_t Filter       :1;
            uint64_t              :1;
            uint64_t unknown      :1;
            uint64_t              :1;
            uint64_t OutsideQuiet :1;
            // Byte 15
            uint64_t              :0;  // Checksum
        };

        struct {
            // Byte 0~1
            uint64_t           :16;  // Fixed header
            // Byte 2
            uint64_t            :4;
            uint64_t          :2;   // Device Number/Identifier
            uint64_t            :2;
            // Byte 3-4
            uint64_t            :16;
            // Byte 5
            uint64_t         :8;  // short codes:cmd; long codes:fixed value
            // Byte 6
            uint64_t  :8;  // Nr. of bytes in the message after this byte.
            // Byte 7
            uint64_t    :8;  // Seems like a protocol version number. Not sure.
            // Byte 8
            uint64_t       :1;
            uint64_t  :1;
            uint64_t        :6;  // Internal representation varies between models.
            // Byte 9
            uint64_t        :3;
            uint64_t       :1;  // Also 10C Heat in ARREW4E.
            uint64_t   :2;
            uint64_t            :2;
            // Byte 10
            uint64_t Fan_new    :4;
            uint64_t   :2;
            uint64_t        :2;
            // Byte 11~13
            uint64_t        :11;  // Also is the sleep timer value
            uint64_t  :1;
            uint64_t         :11;
            uint64_t   :1;
            // Byte 14
            uint64_t              :3;
            uint64_t        :1;
            uint64_t              :1;
            uint64_t       :1;
            uint64_t              :1;
            uint64_t  :1;
            // Byte 15
            uint64_t              :0;
            // Byte 16
            uint64_t              :0;
            // Byte 17
            uint64_t              :0;  // Checksum
        };
    };
} FujitsuProtocol;

typedef struct 
{
    union {
        struct {
            uint8_t longcode[kFujitsuAcStateLength];  ///< The state of the IR remote.
            uint8_t shortcode[kFujitsuAcStateLengthShort];
        };
        struct {
            // Byte 0~1
            uint64_t           :16;  // Fixed header
            // Byte 2
            uint64_t            :4;
            uint64_t Id         :2;   // Device Number/Identifier
            uint64_t            :2;
            // Byte 3-4
            uint64_t            :16;
            // Byte 5
            uint64_t Cmd        :8;  // short codes:cmd; long codes:fixed value
            // Byte 6
            uint64_t RestLength :8;  // Nr. of bytes in the message after this byte.
            // Byte 7
            uint64_t Protocol   :8;  // Seems like a protocol version number. Not sure.
            // Byte 8
            uint64_t Power      :1;
            uint64_t Fahrenheit :1;
            uint64_t Temp       :6;  // Internal representation varies between models.
            // Byte 9
            uint64_t Mode       :3;
            uint64_t Clean      :1;  // Also 10C Heat in ARREW4E.
            uint64_t TimerType  :2;
            uint64_t            :2;
            // Byte 10
            uint64_t Fan    :4;
            uint64_t Swing  :2;
            uint64_t        :2;
            // Byte 11~13
            uint64_t OffTimer       :11;  // Also is the sleep timer value
            uint64_t OffTimerEnable :1;
            uint64_t OnTimer        :11;
            uint64_t OnTimerEnable  :1;
            // Byte 14
            uint64_t              :3;
            uint64_t Filter       :1;
            uint64_t              :1;
            uint64_t unknown      :1;
            uint64_t              :1;
            uint64_t OutsideQuiet :1;
            // Byte 15
            uint64_t              :0;
            // Byte 16
            uint64_t              :0;
            // Byte 17
            uint64_t              :0;  // Checksum
        };
    };
} Fujitsu144Protocol;


// Constants
const uint8_t kFujitsuAcModeAuto =  0x0;  // 0b000
const uint8_t kFujitsuAcModeCool =  0x1;  // 0b001
const uint8_t kFujitsuAcModeDry =   0x2;  // 0b010
const uint8_t kFujitsuAcModeFan =   0x3;  // 0b011
const uint8_t kFujitsuAcModeHeat =  0x4;  // 0b100

const uint8_t kFujitsuAcCmdStayOn = 0x00;            // b00000000
const uint8_t kFujitsuAcCmdTurnOn = 0x01;            // b00000001
const uint8_t kFujitsuAcCmdTurnOff = 0x02;           // b00000010
const uint8_t kFujitsuAcCmdEcono = 0x09;             // b00001001
const uint8_t kFujitsuAcCmdPowerful = 0x39;          // b00111001
const uint8_t kFujitsuAcCmdStepVert = 0x6C;          // b01101100
const uint8_t kFujitsuAcCmdToggleSwingVert = 0x6D;   // b01101101
const uint8_t kFujitsuAcCmdStepHoriz = 0x79;         // b01111001
const uint8_t kFujitsuAcCmdToggleSwingHoriz = 0x7A;  // b01111010

const uint8_t kFujitsuAcFanAuto = 0x00;
const uint8_t kFujitsuAcFanHigh = 0x01;
const uint8_t kFujitsuAcFanMed = 0x02;
const uint8_t kFujitsuAcFanLow = 0x03;
const uint8_t kFujitsuAcFanQuiet = 0x04;

const float   kFujitsuAcMinHeat =     10;  // 10C
const float   kFujitsuAcMinTemp =     16;  // 16C
const float   kFujitsuAcMaxTemp =     30;  // 30C
const uint8_t kFujitsuAcTempOffsetC = kFujitsuAcMinTemp;
const float   kFujitsuAcMinHeatF =    50;  // 50F
const float   kFujitsuAcMinTempF =    60;  // 60F
const float   kFujitsuAcMaxTempF =    88;  // 88F
const uint8_t kFujitsuAcTempOffsetF = 44;

const uint8_t kFujitsuAcSwingOff = 0x00;
const uint8_t kFujitsuAcSwingVert = 0x01;
const uint8_t kFujitsuAcSwingHoriz = 0x02;
const uint8_t kFujitsuAcSwingBoth = 0x03;

const uint8_t kFujitsuAcStopTimers =                       0b00;  // 0
const uint8_t kFujitsuAcSleepTimer =                       0b01;  // 1
const uint8_t kFujitsuAcOffTimer =                         0b10;  // 2
const uint8_t kFujitsuAcOnTimer =                          0b11;  // 3
const uint16_t kFujitsuAcTimerMax = 12 * 60;  ///< Minutes.

// Legacy defines.
#define FUJITSU_AC_MODE_AUTO kFujitsuAcModeAuto
#define FUJITSU_AC_MODE_COOL kFujitsuAcModeCool
#define FUJITSU_AC_MODE_DRY kFujitsuAcModeDry
#define FUJITSU_AC_MODE_FAN kFujitsuAcModeFan
#define FUJITSU_AC_MODE_HEAT kFujitsuAcModeHeat
#define FUJITSU_AC_CMD_STAY_ON kFujitsuAcCmdStayOn
#define FUJITSU_AC_CMD_TURN_ON kFujitsuAcCmdTurnOn
#define FUJITSU_AC_CMD_TURN_OFF kFujitsuAcCmdTurnOff
#define FUJITSU_AC_CMD_STEP_HORIZ kFujitsuAcCmdStepHoriz
#define FUJITSU_AC_CMD_STEP_VERT kFujitsuAcCmdStepVert
#define FUJITSU_AC_FAN_AUTO kFujitsuAcFanAuto
#define FUJITSU_AC_FAN_HIGH kFujitsuAcFanHigh
#define FUJITSU_AC_FAN_MED kFujitsuAcFanMed
#define FUJITSU_AC_FAN_LOW kFujitsuAcFanLow
#define FUJITSU_AC_FAN_QUIET kFujitsuAcFanQuiet
#define FUJITSU_AC_MIN_TEMP kFujitsuAcMinTemp
#define FUJITSU_AC_MAX_TEMP kFujitsuAcMaxTemp
#define FUJITSU_AC_SWING_OFF kFujitsuAcSwingOff
#define FUJITSU_AC_SWING_VERT kFujitsuAcSwingVert
#define FUJITSU_AC_SWING_HORIZ kFujitsuAcSwingHoriz
#define FUJITSU_AC_SWING_BOTH kFujitsuAcSwingBoth

#endif