#ifndef IR_SANYO_H_
#define IR_SANYO_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

typedef struct
{
  union
  {
    uint8_t raw[kSanyoAcStateLength]; ///< The state in IR code form.
    // Ref: https://docs.google.com/spreadsheets/d/1dYfLsnYvpjV-SgO8pdinpfuBIpSzm8Q1R5SabrLeskw/edit?usp=sharing
    struct
    {
      // Byte 0
      uint8_t : 8; // 0x6A (Fixed?)
      // Byte 1
      uint8_t Temp : 5;
      uint8_t : 3;
      // Byte 2
      uint8_t SensorTemp : 5;
      uint8_t Sensor : 1; ///< Sensor location (0 = remote, 1 = A/C)
      uint8_t Beep : 1;
      uint8_t : 1;
      // Byte 3
      uint8_t OffHour : 4;
      uint8_t : 4;
      // Byte 4
      uint8_t Fan : 2;
      uint8_t OffTimer : 1;
      uint8_t : 1;
      uint8_t Mode : 3;
      uint8_t : 1;
      // Byte 5
      uint8_t SwingV : 3;
      uint8_t : 3;
      uint8_t Power : 2;
      // Byte 6
      uint8_t : 3;
      uint8_t Sleep : 1;
      uint8_t Temp_change : 4;
      // Byte 7
      uint8_t : 8;
      // Byte 8
      uint8_t Sum : 8;
    };
  };
} SanyoProtocol;

// Constants

const uint8_t kSanyoAcTempMin = 16;  ///< Celsius
const uint8_t kSanyoAcTempMax = 30;  ///< Celsius
const uint8_t kSanyoAcTempDelta = 4; ///< Celsius to Native Temp difference.

const uint8_t kSanyoAcHourMax = 15; ///<          0b1111

const uint8_t kSanyoAcHeat = 1;      ///<       0b001
const uint8_t kSanyoAcCool = 2;      ///<       0b010
const uint8_t kSanyoAcDry = 3;       ///<       0b011
const uint8_t kSanyoAcAuto = 4;      ///<       0b100
const uint8_t kSanyoAcFanAuto = 0;   ///<            0b00
const uint8_t kSanyoAcFanHigh = 1;   ///<            0b01
const uint8_t kSanyoAcFanLow = 2;    ///<            0b10
const uint8_t kSanyoAcFanMedium = 3; ///<            0b11

// const uint8_t kSanyoAcPowerStandby =           0b00;  ///< Standby?
const uint8_t kSanyoAcPowerOff = 0b01;       ///< Off
const uint8_t kSanyoAcPowerOn = 0b10;        ///< On
const uint8_t kSanyoAcSwingVAuto = 0;        ///<     0b000
const uint8_t kSanyoAcSwingVLowest = 2;      ///<     0b010
const uint8_t kSanyoAcSwingVLow = 3;         ///<     0b011
const uint8_t kSanyoAcSwingVLowerMiddle = 4; ///<     0b100
const uint8_t kSanyoAcSwingVUpperMiddle = 5; ///<     0b101
const uint8_t kSanyoAcSwingVHigh = 6;        ///<     0b110
const uint8_t kSanyoAcSwingVHighest = 7;     ///<     0b111

const uint8_t kSanyoAc88Auto = 0;     ///< 0b000
const uint8_t kSanyoAc88FeelCool = 1; ///< 0b001
const uint8_t kSanyoAc88Cool = 2;     ///< 0b010
const uint8_t kSanyoAc88FeelHeat = 3; ///< 0b011
const uint8_t kSanyoAc88Heat = 4;     ///< 0b100
const uint8_t kSanyoAc88Fan = 5;      ///< 0b101

const uint8_t kSanyoAc88TempMin = 10; ///< Celsius
const uint8_t kSanyoAc88TempMax = 30; ///< Celsius

const uint8_t kSanyoAc88FanAuto = 0;   ///< 0b00
const uint8_t kSanyoAc88FanLow = 1;    ///< 0b11
const uint8_t kSanyoAc88FanMedium = 2; ///< 0b10
const uint8_t kSanyoAc88FanHigh = 3;   ///< 0b11

typedef struct
{
  union
  {
    uint8_t raw[kSanyoAc88StateLength]; ///< The state in IR code form.
    // Ref: https://docs.google.com/spreadsheets/d/1weUmGAsEpfX38gg5rlDN69Uchnbr6gQl9FqHffLBIRk/edit#gid=0
    struct
    {
      // Byte 0-1
      uint8_t : 8; // 0xAA (Fixed?)
      uint8_t : 8; // 0x55 (Fixed?)
      // Byte 2
      uint8_t Fan : 2;
      uint8_t : 2;
      uint8_t Mode : 3;
      uint8_t Power : 1;
      // Byte 3
      uint8_t Temp : 5;
      uint8_t Filter : 1;
      uint8_t SwingV : 1;
      uint8_t : 1;
      // Byte 4
      uint8_t ClockSecs : 8; // Nr. of Seconds
      // Byte 5
      uint8_t ClockMins : 8; // Nr. of Minutes
      // Byte 6
      uint8_t ClockHrs : 8; // Nr. of Hours
      // Byte 7-9  (Timer times?)
      uint8_t : 8;
      uint8_t : 8;
      uint8_t : 8;
      // Byte 10
      uint8_t : 3;
      uint8_t Turbo : 1;
      uint8_t EnableStartTimer : 1;
      uint8_t EnableStopTimer : 1;
      uint8_t Sleep : 1;
      uint8_t : 1;
    };
  };
} SanyoAc88Protocol;

const uint8_t kSanyoAc152TempMin = 16;  ///< Celsius
const uint8_t kSanyoAc152TempMax = 30;  ///< Celsius
const uint8_t kSanyoAc152TempDelta = 4; ///< Celsius to Native Temp difference.

const uint8_t kSanyoAc152FanClean = 0;   ///<       0b001
const uint8_t kSanyoAc152Heat = 1;       ///<       0b001
const uint8_t kSanyoAc152Cool = 2;       ///<       0b010
const uint8_t kSanyoAc152Dry = 3;        ///<       0b011
const uint8_t kSanyoAc152ClothesDry = 4; ///<       0b011
const uint8_t kSanyoAc152Auto = 5;       ///<       0b100

const uint8_t kSanyoAc152FanAuto = 0;   ///<            0b00
const uint8_t kSanyoAc152FanHigh = 1;   ///<            0b01
const uint8_t kSanyoAc152FanLow = 2;    ///<            0b10
const uint8_t kSanyoAc152FanMedium = 3; ///<            0b11
const uint8_t kSanyoAc152FanQuite = 6;  ///<            Min

// const uint8_t kSanyoAc152PowerStandby =           0b00;  ///< Standby?
const uint8_t kSanyoAc152PowerOff = 0b0; ///< Off
const uint8_t kSanyoAc152PowerOn = 0b1;  ///< On

const uint8_t kSanyoAc152SwingVSwing = 0;       ///<     0b111
const uint8_t kSanyoAc152SwingVAuto = 1;        ///<     0b000
const uint8_t kSanyoAc152SwingVLowest = 2;      ///<     0b010
const uint8_t kSanyoAc152SwingVLow = 3;         ///<     0b011
const uint8_t kSanyoAc152SwingVLowerMiddle = 4; ///<     0b100
const uint8_t kSanyoAc152SwingVUpperMiddle = 5; ///<     0b101
const uint8_t kSanyoAc152SwingVHigh = 6;        ///<     0b110
const uint8_t kSanyoAc152SwingVHighest = 7;     ///<     0b111
typedef struct
{
  union
  {
    uint8_t raw[kSanyoAc152StateLength]; ///< The state in IR code form.
    // Ref: https://docs.google.com/spreadsheets/d/1weUmGAsEpfX38gg5rlDN69Uchnbr6gQl9FqHffLBIRk/edit#gid=0
    struct
    {
      // Byte 0
      uint8_t : 8; // 0x40 (Fixed?)
      // Byte 1
      uint8_t : 8; // 0x00 (Fixed?)
      // Byte 2
      uint8_t : 8; // 0x14 (Fixed?)
      // Byte 3
      uint8_t : 8; // 0x80 (Fixed?)
      // Byte 4
      uint8_t : 8; // 0x4B (Fixed?)
      // Byte 5
      uint8_t Button : 2;
      uint8_t Fan : 2;
      uint8_t is_clothes_mode: 1;
      uint8_t : 1;
      uint8_t Mode : 2;
      // Byte 6

      uint8_t : 1;
      uint8_t Temp : 5;
      uint8_t : 1;
      uint8_t Auto : 1;
      // Byte 7
      uint8_t : 5;
      uint8_t not_heat : 1;
      uint8_t : 2;
      // Byte 8
      uint8_t : 4;
      uint8_t Quite : 1;
      uint8_t Power : 1;
      uint8_t : 2;
      // Byte 9
      uint8_t : 8;
      // Byte 10
      uint8_t : 8;
      // Byte 11
      uint8_t : 8;
      // Byte 12
      uint8_t : 8;
      // Byte 13
      uint8_t SwingV : 3;
      uint8_t : 1;
      uint8_t : 4;
      // Byte 14
      uint8_t : 8;
      // Byte 15
      uint8_t : 8;
      // Byte 16
      uint8_t : 8;
      // Byte 17
      uint8_t : 1;
      uint8_t Clean : 1;
      uint8_t : 2;
      uint8_t Degree_minus : 1;
      uint8_t Temp_change : 3;
      // Byte 18
      uint8_t Sum : 8;
    };
  };
} SanyoAc152Protocol;

#endif