#ifndef IR_ARGO_H_
#define IR_ARGO_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

typedef struct
{
    union
    {
        uint8_t raw[kArgoStateLength];  ///< The state in native IR code form
        struct {
            // Byte 0
            uint64_t Pre1     :8;  // Typically 0b00110101
            // Byte 1
            uint64_t Pre2     :8;  // Typically 0b10101111
            // Byte 2~4
            uint64_t          :3;
            uint64_t Mode     :3;
            uint64_t Temp     :5;  // straddle byte 2 and 3
            uint64_t Fan      :2;
            uint64_t RoomTemp :5;  // straddle byte 3 and 4
            uint64_t Flap     :3;  // SwingV
            uint64_t          :3;  // OnTimer, maybe hours
            // Byte 5
            uint64_t          :8;  // OnTimer, maybe minutes
            // Byte 6
            uint64_t          :8;  // OffTimer, maybe minutes
            // Byte 7
            uint64_t          :3;  // OffTimer, maybe hours
            uint64_t          :5;  // Time
            // Byte 8
            uint32_t          :6;  // Time
            uint32_t          :1;  // Timer On/Off
            uint32_t          :1;  // Timer Program
            // Byte 9
            uint32_t          :1;  // Timer Program
            uint32_t          :1;  // Timer 1h
            uint32_t Night    :1;
            uint32_t Max      :1;
            uint32_t          :1;  // Filter
            uint32_t Power    :1;
            uint32_t          :1;  // const 0
            uint32_t iFeel    :1;
            // Byte 10~11
            uint32_t Post     :2;
            uint32_t Sum      :8;  // straddle byte 10 and 11
            uint32_t          :6;
        };
        struct {
            // Byte 0-1
            uint8_t           :8;
            uint8_t           :8;
            // Byte 2-3
            uint8_t CheckHi   :3;
            uint8_t SensorT   :5;
            uint8_t Fixed     :3;  // Typically 0b011
            uint8_t CheckLo   :5;
        };
    };
} ArgoProtocol;


typedef struct
{
    union
    {
        uint8_t raw[kArgoStateLength];  ///< The state in native IR code form
        struct {
            // Byte 0 (same definition across the union)
            uint8_t Pre1           :4;  /// Preamble: 0b1011 @ref kArgoWrem3Preamble
            uint8_t IrChannel      :2;  /// 0..3 range
            uint8_t IrCommandType  :2;  /// @ref argoIrMessageType_t
            // Byte 1
            uint8_t RoomTemp       :5;  // in Celsius, range:  4..35 (offset by -4[*C])
            uint8_t Mode           :3;  /// @ref argoMode_t
            // Byte 2
            uint8_t Temp           :5;  // in Celsius, range: 10..32 (offset by -4[*C])
            uint8_t Fan            :3;  /// @ref argoFan_t
            // Byte3
            uint8_t Flap           :3;  /// SwingV @ref argoFlap_t
            uint8_t Power          :1;
            uint8_t iFeel          :1;
            uint8_t Night          :1;
            uint8_t Eco            :1;
            uint8_t Max            :1;  ///< a.k.a. Turbo
            // Byte4
            uint8_t Filter         :1;
            uint8_t Light          :1;
            uint8_t Post1          :6;  /// Unknown, always 0b110000 (TempScale?)
            // Byte5
            uint8_t Sum            :8;  /// Checksum
        };
        struct {
            // Byte 0 (same definition across the union)
            uint8_t                :8;  // {Pre1 | IrChannel | IrCommandType}
            // Byte 1
            uint8_t SensorT        :5;  // in Celsius, range:  4..35 (offset by -4[*C])
            uint8_t CheckHi        :3;  // Checksum (short)
        };
        struct Timer {
            // Byte 0 (same definition across the union)
            uint8_t                   : 8;  // {Pre1 | IrChannel | IrCommandType}
            // Byte 1
            uint8_t IsOn              : 1;
            uint8_t TimerType         : 3;
            uint8_t CurrentTimeLo     : 4;
            // Byte 2
            uint8_t CurrentTimeHi     : 7;
            uint8_t CurrentWeekdayLo  : 1;
            // Byte 3
            uint8_t CurrentWeekdayHi  : 2;
            uint8_t DelayTimeLo       : 6;
            // Byte 4
            uint8_t DelayTimeHi       : 5;
            uint8_t TimerStartLo      : 3;
            // Byte 5
            uint8_t TimerStartHi      : 8;
            // Byte 6
            uint8_t TimerEndLo        : 8;
            // Byte 7
            uint8_t TimerEndHi        : 3;
            uint8_t TimerActiveDaysLo : 5;  // Bitmap (LSBit is Sunday)
            // Byte 8
            uint8_t TimerActiveDaysHi : 2;  // Bitmap (LSBit is Sunday)
            uint8_t Post1             : 1;  // Unknown, always 1
            uint8_t Checksum          : 5;
        } timer;
        struct Config {
            uint8_t           :8;  // Byte 0 {Pre1 | IrChannel | IrCommandType}
            uint8_t Key       :8;  // Byte 1
            uint8_t Value     :8;  // Byte 2
            uint8_t Checksum  :8;  // Byte 3
        } config;
    };
} ArgoProtocolWREM3;

typedef enum {
  AC_CONTROL = 0b00,
  IFEEL_TEMP_REPORT = 0b01,
  TIMER_COMMAND = 0b10,  // WREM-3 only (WREM-2 has it under AC_CONTROL)
  CONFIG_PARAM_SET = 0b11  // WREM-3 only
} argoIrMessageType_t;

typedef enum {
  COOL = 0b001,
  DRY  = 0b010,
  HEAT = 0b011,
  FAN  = 0b100,
  AUTO = 0b101
} argoMode_t;

typedef enum {
  FAN_AUTO = 0b000,
  FAN_LOWEST = 0b001,
  FAN_LOWER = 0b010,
  FAN_LOW  = 0b011,
  FAN_MEDIUM = 0b100,
  FAN_HIGH  = 0b101,
  FAN_HIGHEST  = 0b110
} argoFan_t;

typedef enum {
  FLAP_AUTO = 0,
  FLAP_1 = 1,  // Highest
  FLAP_2 = 2,
  FLAP_3 = 3,
  FLAP_4 = 4,
  FLAP_5 = 5,
  FLAP_6 = 6,  // Lowest
  FLAP_FULL = 7
} argoFlap_t;

typedef enum {
  NO_TIMER         = 0b000,
  DELAY_TIMER      = 0b001,
  SCHEDULE_TIMER_1 = 0b010,
  SCHEDULE_TIMER_2 = 0b011,
  SCHEDULE_TIMER_3 = 0b100
} argoTimerType_t;

typedef enum {
  SUNDAY    = 0b000,
  MONDAY    = 0b001,
  TUESDAY   = 0b010,
  WEDNESDAY = 0b011,
  THURSDAY  = 0b100,
  FRIDAY    = 0b101,
  SATURDAY  = 0b110
} argoWeekday;




// Constants (WREM-2). Store MSB left.
const uint8_t  kArgoHeatBit    = 0b00100000;
const uint8_t  kArgoPreamble1  = 0b10101100;
const uint8_t  kArgoPreamble2  = 0b11110101;
const uint8_t  kArgoPost       = 0b00000010;

// Constants (generic)
const uint16_t kArgoFrequency  = 38000;  // Hz
// Temp
const uint8_t kArgoTempDelta   = 4;
const uint8_t kArgoMaxRoomTemp = 35;  // Celsius
const uint8_t kArgoMinTemp     = 10;  // Celsius delta +4
const uint8_t kArgoMaxTemp     = 32;  // Celsius
const uint8_t kArgoMaxChannel  = 3;


// Raw mode definitions for WREM-2 remote
// (not wraped into a ns nor enum for backwards-compat.)
const uint8_t kArgoCool =           0b000;
const uint8_t kArgoDry =            0b001;
const uint8_t kArgoAuto =           0b010;
const uint8_t kArgoOff =            0b011;
const uint8_t kArgoHeat =           0b100;
const uint8_t kArgoHeatAuto =       0b101;
// ?no idea what mode that is
const uint8_t kArgoHeatBlink =      0b110;

// Raw fan speed definitions for WREM-2 remote
// (not wraped into a ns nor enum for backwards-compat.)
const uint8_t kArgoFanAuto = 0;      // 0b00
const uint8_t kArgoFan1 = 1;         // 0b01
const uint8_t kArgoFan2 = 2;         // 0b10
const uint8_t kArgoFan3 = 3;         // 0b11

// Raw Flap/SwingV definitions for WREM-2 remote
// (not wraped into a ns nor enum for backwards-compat.)
const uint8_t kArgoFlapAuto = 0;
const uint8_t kArgoFlap1 = 1;
const uint8_t kArgoFlap2 = 2;
const uint8_t kArgoFlap3 = 3;
const uint8_t kArgoFlap4 = 4;
const uint8_t kArgoFlap5 = 5;
const uint8_t kArgoFlap6 = 6;
const uint8_t kArgoFlapFull = 7;

#endif