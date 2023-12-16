#ifndef IR_MIDEA_H_
#define IR_MIDEA_H_


#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

typedef struct
{
    union
    {
        uint64_t remote_state;
        // only use 48bits
        struct
        { 
            // Byte 0
            uint8_t Sum;
            // Byte 1 (value=0xFF when not in use.)
            // This byte gets dual usage as Sensor Temp and On Timer
            // Depending on "Type" below.
            // When in "OnTimer", the nr of half hours is stored with mask 0b01111110
            // i.e.
            //  uint8_t :1;
            //  uint8_t OnTimerHalfHours:6;
            //  uint8_t :1;
            uint8_t SensorTemp:7;  ///< Degrees or OnTimer.
            uint8_t disableSensor:1;
            // Byte 2 (value=0xFF when not in use.)
            uint8_t :1;  // 0b1
            uint8_t OffTimer:6;  ///< Nr of Half hours. Off is 0b111111
            uint8_t BeepDisable:1;  ///< 0 = no beep in follow me messages, 1 = beep.
            // Byte 3
            uint8_t Temp:5;
            uint8_t useFahrenheit:1;
            uint8_t :2;
            // Byte 4
            uint8_t Mode:3;
            uint8_t Fan:2;
            /// @todo Find out what this bit controls.
            /// @see https://github.com/crankyoldgit/IRremoteESP8266/issues/1342#issuecomment-733721085
            uint8_t :1;  ///< Unknown, but set on _some_ Pioneer System A/Cs.
            uint8_t Sleep:1;
            uint8_t Power:1;
            // Byte 5
            uint8_t Type:3;    ///< Normal, Special, or FollowMe message type
            uint8_t Header:5;  ///< Typically 0b10100
        };
    };
} MideaProtocol;

// Constants
#define kMideaACMinTempF  62        ///< Fahrenheit
#define kMideaACMaxTempF  86        ///< Fahrenheit
#define kMideaACMinTempC  17        ///< Celsius
#define kMideaACMaxTempC  30        ///< Celsius
#define kMideaACMinSensorTempC  0   ///< Celsius
#define kMideaACMaxSensorTempC  37  ///< Celsius
#define kMideaACMinSensorTempF  32  ///< Fahrenheit
#define kMideaACMaxSensorTempF  99  ///< Fahrenheit (Guess only!)
#define kMideaACSensorTempOnTimerOff  0b1111111
#define kMideaACTimerOff  0b111111
#define kMideaACCool  0     // 0b000
#define kMideaACDry  1      // 0b001
#define kMideaACAuto  2     // 0b010
#define kMideaACHeat  3     // 0b011
#define kMideaACFan  4      // 0b100
#define kMideaACFanAuto  0  // 0b00
#define kMideaACFanLow  1   // 0b01
#define kMideaACFanMed  2   // 0b10
#define kMideaACFanHigh  3  // 0b11
#if KAYSUN_AC
  // For Kaysun AC units, Toggle SwingV is 0xA202FFFFFF7E
  #define kMideaACToggleSwingV  0xA202FFFFFF7E
  #define kMideaACSwingVStep    0xA201FFFFFF7C
#else  // KAYSUN_AC
  #define kMideaACToggleSwingV  0xA201FFFFFF7C
#endif  // KAYSUN_AC
#if DANBY_DAC
  // For Danby DAC unit, the Ionizer toggle is the same as ToggleSwingV
  // #define kMideaACToggleIonizer  0xA201FFFFFF7C
  kSwingVToggleStr  kIonStr
#endif  // DANBY_DAC
#define kMideaACToggleEcono             0xA202FFFFFF7E
#define kMideaACToggleLight             0xA208FFFFFF75
#define kMideaACToggleTurbo             0xA209FFFFFF74
// Mode must be Auto, Cool, or Dry
#define kMideaACToggleSelfClean         0xA20DFFFFFF70
// 8C Heat AKA Freeze Protection
#define kMideaACToggle8CHeat            0xA20FFFFFFF73  // Only in Heat
#define kMideaACQuietOn                 0xA212FFFFFF6E
#define kMideaACQuietOff                0xA213FFFFFF6F

#define kMideaACTypeCommand  0b001  ///< Message type
#define kMideaACTypeSpecial  0b010  ///< Message type
#define kMideaACTypeFollow   0b100  ///< Message type

// Legacy defines. (Deprecated)
#define MIDEA_AC_COOL kMideaACCool
#define MIDEA_AC_DRY kMideaACDry
#define MIDEA_AC_AUTO kMideaACAuto
#define MIDEA_AC_HEAT kMideaACHeat
#define MIDEA_AC_FAN kMideaACFan
#define MIDEA_AC_FAN_AUTO kMideaACFanAuto
#define MIDEA_AC_FAN_LOW kMideaACFanLow
#define MIDEA_AC_FAN_MED kMideaACFanMed
#define MIDEA_AC_FAN_HI kMideaACFanHigh
#define MIDEA_AC_POWER kMideaACPower
#define MIDEA_AC_SLEEP kMideaACSleep
#define MIDEA_AC_MIN_TEMP_F kMideaACMinTempF
#define MIDEA_AC_MAX_TEMP_F kMideaACMaxTempF
#define MIDEA_AC_MIN_TEMP_C kMideaACMinTempC
#define MIDEA_AC_MAX_TEMP_C kMideaACMaxTempC

#endif