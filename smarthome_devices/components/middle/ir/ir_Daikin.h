#ifndef IR_DAIKIN_H_
#define IR_DAIKIN_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

typedef struct
{
    union
    {
        uint8_t raw[kDaikinStateLength]; ///< State in code form.
        struct
        { 
            // Byte 0~5
            uint64_t          :48;
            // Byte 6
            uint64_t          :4;
            uint64_t Comfort  :1;
            uint64_t          :3;
            // Byte 7
            uint64_t Sum1     :8;  // checksum of the first part

            // Byte 8~12
            uint64_t              :40;
            // Byte 13~14
            uint64_t CurrentTime  :11;  // Current time, mins past midnight
            uint64_t CurrentDay   :3;  // Day of the week (SUN=1, MON=2, ..., SAT=7)
            uint64_t              :2;
            // Byte 15
            uint64_t Sum2         :8;  // checksum of the second part

            // Byte 16~20
            uint64_t          :40;
            // Byte 21
            uint64_t Power    :1;
            uint64_t OnTimer  :1;
            uint64_t OffTimer :1;
            uint64_t          :1;  // always 1
            uint64_t Mode     :3;
            uint64_t          :1;
            // Byte 22
            uint64_t Temp     :8;  // Temp should be between 20 - 64 (10 C - 32 C)
            // Byte 23
            uint64_t          :8;

            // Byte 24
            uint64_t SwingV   :4;  // 0000 =  off, 1111 = on
            uint64_t Fan      :4;
            // Byte 25
            uint64_t SwingH   :4;  // 0000 =  off, 1111 = on
            uint64_t          :4;
            // Byte 26~28
            uint64_t OnTime   :12;  // timer mins past midnight
            uint64_t OffTime  :12;  // timer mins past midnight
            // Byte 29
            uint64_t Powerful :1;
            uint64_t          :4;
            uint64_t Quiet    :1;
            uint64_t          :2;
            // Byte 30~31
            uint64_t          :0;

            // Byte 32
            uint8_t             :1;
            uint8_t Sensor      :1;
            uint8_t Econo       :1;
            uint8_t             :4;
            uint8_t WeeklyTimer :1;
            // Byte 33
            uint8_t       :1;
            uint8_t Mold  :1;
            uint8_t       :6;
            // Byte 34
            uint8_t Sum3  :8;  // checksum of the third part
        };
    };
} DaikinProtocol;


// Constants
const uint8_t kDaikinAuto = 0b000;  // temp 25
const uint8_t kDaikinDry =  0b010;  // temp 0xc0 = 96 degrees c
const uint8_t kDaikinCool = 0b011;
const uint8_t kDaikinHeat = 0b100;  // temp 23
const uint8_t kDaikinFan =  0b110;  // temp not shown, but 25
const uint8_t kDaikinMinTemp = 10;  // Celsius
const uint8_t kDaikinMaxTemp = 32;  // Celsius
const uint8_t kDaikinFanMin = 1;
const uint8_t kDaikinFanMed = 3;
const uint8_t kDaikinFanMax = 5;
const uint8_t kDaikinFanAuto = 0b1010;  // 10 / 0xA
const uint8_t kDaikinFanQuiet = 0b1011;  // 11 / 0xB
const uint8_t kDaikinSwingOn =  0b1111;
const uint8_t kDaikinSwingOff = 0b0000;
const uint16_t kDaikinHeaderLength = 5;
const uint8_t kDaikinSections = 3;
const uint8_t kDaikinSection1Length = 8;
const uint8_t kDaikinSection2Length = 8;
const uint8_t kDaikinSection3Length = kDaikinStateLength - kDaikinSection1Length - kDaikinSection2Length;
const uint8_t kDaikinByteChecksum1 = 7;
const uint8_t kDaikinByteChecksum2 = 15;
// const uint8_t kDaikinBitEye = 0b10000000;
const uint16_t kDaikinUnusedTime = 0x600;
const uint8_t kDaikinBeepQuiet = 1;
const uint8_t kDaikinBeepLoud = 2;
const uint8_t kDaikinBeepOff = 3;
const uint8_t kDaikinLightBright = 1;
const uint8_t kDaikinLightDim = 2;
const uint8_t kDaikinLightOff = 3;
const uint8_t kDaikinCurBit = kDaikinStateLength;
const uint8_t kDaikinCurIndex = kDaikinStateLength + 1;
const uint8_t kDaikinTolerance = 35;
const uint16_t kDaikinMarkExcess = kMarkExcess;
const uint16_t kDaikinHdrMark = 3650;   // kDaikinBitMark * 8
const uint16_t kDaikinHdrSpace = 1623;  // kDaikinBitMark * 4
const uint16_t kDaikinBitMark = 428;
const uint16_t kDaikinZeroSpace = 428;
const uint16_t kDaikinOneSpace = 1280;
const uint16_t kDaikinGap = 29000;
// Note bits in each octet swapped so can be sent as a single value
const uint64_t kDaikinFirstHeader64 =
    0b1101011100000000000000001100010100000000001001111101101000010001;


typedef struct
{
    union
    {
        struct {
            uint8_t pad[3];
            uint8_t raw[kDaikin2StateLength]; ///< State in code form.
        };
        struct
        { 
            // Byte -3~4
            uint64_t :64;

            // Byte 5~6
            uint64_t CurrentTime  :12;
            uint64_t              :3;
            uint64_t Power2       :1;
            // Byte 7
            uint64_t              :4;
            uint64_t Light        :2;
            uint64_t Beep         :2;
            // Byte 8
            uint64_t FreshAir     :1;
            uint64_t              :2;
            uint64_t Mold         :1;
            uint64_t              :1;
            uint64_t Clean        :1;
            uint64_t              :1;
            uint64_t FreshAirHigh :1;
            // Byte 9~12
            uint64_t              :32;

            // Byte 13
            uint64_t          :7;
            uint64_t EyeAuto  :1;
            // Byte 14~16
            uint64_t          :24;
            // Byte 17
            uint64_t SwingH   :8;
            // Byte 18
            uint64_t SwingV   :4;
            uint64_t          :4;
            // Byte 19
            uint64_t Sum1     :8;
            // Byte 20
            uint64_t          :8;

            // Byte 21~24
            uint64_t          :32;
            // Byte 25
            uint64_t Power    :1;
            uint64_t OnTimer  :1;
            uint64_t OffTimer :1;
            uint64_t          :1;
            uint64_t Mode     :3;
            uint64_t          :1;
            // Byte 26
            uint64_t          :1;
            uint64_t Temp     :6;
            uint64_t HumidOn  :1;
            // Byte 27
            uint64_t Humidity :8;
            // Byte 28
            uint64_t          :4;
            uint64_t Fan      :4;

            // Byte 29
            uint64_t            :8;
            // Byte 30~32
            /// @see https://github.com/crankyoldgit/IRremoteESP8266/pull/1264
            uint64_t OnTime     :12;
            uint64_t OffTime    :12;
            // Byte 33
            uint64_t Powerful   :1;
            uint64_t            :4;
            uint64_t Quiet      :1;
            uint64_t            :2;
            // Byte 34~35
            uint64_t            :16;
            // Byte 36
            uint64_t            :1;
            uint64_t Eye        :1;
            uint64_t Econo      :1;
            uint64_t            :1;
            uint64_t Purify     :1;
            uint64_t SleepTimer :1;
            uint64_t            :2;

            // Byte 37
            uint8_t       :8;
            // Byte 38
            uint8_t Sum2  :8;
        };
    };
} Daikin2Protocol;


const uint16_t kDaikin2Freq = 36700;  // Modulation Frequency in Hz.
const uint16_t kDaikin2LeaderMark = 10024;
const uint16_t kDaikin2LeaderSpace = 25180;
const uint16_t kDaikin2Gap = kDaikin2LeaderMark + kDaikin2LeaderSpace;
const uint16_t kDaikin2HdrMark = 3500;
const uint16_t kDaikin2HdrSpace = 1728;
const uint16_t kDaikin2BitMark = 460;
const uint16_t kDaikin2OneSpace = 1270;
const uint16_t kDaikin2ZeroSpace = 420;
const uint16_t kDaikin2Sections = 2;
const uint16_t kDaikin2Section1Length = 20;
const uint16_t kDaikin2Section2Length = 19;
const uint8_t kDaikin2Tolerance = 5;  // Extra percentage tolerance
const uint8_t kDaikin2SwingVHighest =     0x1;
const uint8_t kDaikin2SwingVHigh =        0x2;
const uint8_t kDaikin2SwingVUpperMiddle = 0x3;
const uint8_t kDaikin2SwingVLowerMiddle = 0x4;
const uint8_t kDaikin2SwingVLow =         0x5;
const uint8_t kDaikin2SwingVLowest =      0x6;
const uint8_t kDaikin2SwingVBreeze =      0xC;
const uint8_t kDaikin2SwingVCirculate =   0xD;
const uint8_t kDaikin2SwingVOff =         0xE;
const uint8_t kDaikin2SwingVAuto =        0xF;  // A.k.a "Swing"
const uint8_t kDaikin2SwingVSwing =  kDaikin2SwingVAuto;


const uint8_t kDaikin2SwingHWide =     0xA3;
const uint8_t kDaikin2SwingHLeftMax =  0xA8;
const uint8_t kDaikin2SwingHLeft =     0xA9;
const uint8_t kDaikin2SwingHMiddle =   0xAA;
const uint8_t kDaikin2SwingHRight =    0xAB;
const uint8_t kDaikin2SwingHRightMax = 0xAC;
const uint8_t kDaikin2SwingHAuto =     0xBE;  // A.k.a "Swing"
const uint8_t kDaikin2SwingHOff =      0xBF;
const uint8_t kDaikin2SwingHSwing =  kDaikin2SwingHAuto;

// Ref:
//   https://github.com/crankyoldgit/IRremoteESP8266/issues/1535#issuecomment-882092486
//   https://docs.google.com/spreadsheets/d/1kxHgFqiUB9ETXYEkszAIN5gE-t2ykvnPCnOV-sPUE0A/edit?usp=sharing
const uint8_t kDaikin2HumidityOff        = 0x00;
const uint8_t kDaikin2HumidityHeatLow    = 0x28;  // Humidify (Heat) only (40%?)
const uint8_t kDaikin2HumidityHeatMedium = 0x2D;  // Humidify (Heat) only (45%?)
const uint8_t kDaikin2HumidityHeatHigh   = 0x32;  // Humidify (Heat) only (50%?)
const uint8_t kDaikin2HumidityDryLow     = 0x32;  // Dry only (50%?)
const uint8_t kDaikin2HumidityDryMedium  = 0x37;  // Dry only (55%?)
const uint8_t kDaikin2HumidityDryHigh    = 0x3C;  // Dry only (60%?)
const uint8_t kDaikin2HumidityAuto       = 0xFF;


const uint8_t kDaikin2MinCoolTemp = 18;  // Min temp (in C) when in Cool mode.


typedef struct
{
    union
    {
        uint8_t raw[kDaikin152StateLength];  ///< The state of the IR remote.
        struct
        { 
            // Byte 0~4
            uint8_t pad0[5];
            // Byte 5
            uint8_t Power :1;
            uint8_t       :3;
            uint8_t Mode  :3;
            uint8_t       :1;
            // Byte 6
            uint8_t       :1;
            uint8_t Temp  :7;
            // Byte 7
            uint8_t :8;
            // Byte 8
            uint8_t SwingV  :4;
            uint8_t Fan     :4;
            // Byte 9~12
            uint8_t pad1[4];
            // Byte 13
            uint8_t Powerful  :1;
            uint8_t           :4;
            uint8_t Quiet     :1;
            uint8_t           :2;
            // Byte 14~15
            uint8_t pad2[2];
            // Byte 16
            uint8_t         :1;
            uint8_t Comfort :1;
            uint8_t Econo   :1;
            uint8_t Sensor  :1;
            uint8_t         :4;
            // Byte 17
            uint8_t     :8;
            // Byte 18
            uint8_t Sum :8;
        };
    };
} Daikin152Protocol;


const uint16_t kDaikin152Freq = 38000;  // Modulation Frequency in Hz.
const uint8_t  kDaikin152LeaderBits = 5;
const uint16_t kDaikin152HdrMark = 3492;
const uint16_t kDaikin152HdrSpace = 1718;
const uint16_t kDaikin152BitMark = 433;
const uint16_t kDaikin152OneSpace = 1529;
const uint16_t kDaikin152ZeroSpace = kDaikin152BitMark;
const uint16_t kDaikin152Gap = 25182;

const uint8_t kDaikin152DryTemp = kDaikin2MinCoolTemp;  // Celsius
const uint8_t kDaikin152FanTemp = 0x60;  // 96 Celsius

typedef struct
{
    union
    {
        uint8_t raw[kDaikin160StateLength];  ///< The state of the IR remote.
        struct
        { 
            // Byte 0~5
            uint8_t pad0[6];
            // Byte 6
            uint8_t Sum1 :8;
            // Byte 7~11
            uint8_t pad1[5];
            // Byte 12
            uint8_t Power   :1;
            uint8_t         :3;
            uint8_t Mode    :3;
            uint8_t         :1;
            // Byte 13
            uint8_t         :4;
            uint8_t SwingV  :4;
            // Byte 14~15
            uint8_t pad2[2];
            // Byte 16
            uint8_t       :1;
            uint8_t Temp  :6;
            uint8_t       :1;
            // Byte 17
            uint8_t Fan   :4;
            uint8_t       :4;
            // Byte 18
            uint8_t       :8;
            // Byte 19
            uint8_t Sum2  :8;
        };
    };
} Daikin160Protocol;


const uint16_t kDaikin160Freq = 38000;  // Modulation Frequency in Hz.
const uint16_t kDaikin160HdrMark = 5000;
const uint16_t kDaikin160HdrSpace = 2145;
const uint16_t kDaikin160BitMark = 342;
const uint16_t kDaikin160OneSpace = 1786;
const uint16_t kDaikin160ZeroSpace = 700;
const uint16_t kDaikin160Gap = 29650;
const uint16_t kDaikin160Sections = 2;
const uint16_t kDaikin160Section1Length = 7;
const uint16_t kDaikin160Section2Length = kDaikin160StateLength -
                                          kDaikin160Section1Length;
const uint8_t kDaikin160SwingVLowest =  0x1;
const uint8_t kDaikin160SwingVLow =     0x2;
const uint8_t kDaikin160SwingVMiddle =  0x3;
const uint8_t kDaikin160SwingVHigh =    0x4;
const uint8_t kDaikin160SwingVHighest = 0x5;
const uint8_t kDaikin160SwingVAuto =    0xF;


typedef struct
{
    union
    {
        uint8_t raw[kDaikin216StateLength];  ///< The state of the IR remote.
        struct
        { 
            // Byte 0~6
            uint8_t pad0[7];
            // Byte 7
            uint8_t Sum1  :8;
            // Byte 8~12
            uint8_t pad1[5];
            // Byte 13
            uint8_t Power :1;
            uint8_t       :3;
            uint8_t Mode  :3;
            uint8_t       :1;
            // Byte 14
            uint8_t         :1;
            uint8_t Temp    :6;
            uint8_t         :1;
            // Byte 15
            uint8_t         :8;
            // Byte 16
            uint8_t SwingV  :4;
            uint8_t Fan     :4;
            // Byte 17
            uint8_t SwingH  :4;
            uint8_t         :4;
            // Byte 18~20
            uint8_t pad2[3];
            // Byte 21
            uint8_t Powerful  :1;
            uint8_t           :0;
            // Byte 22~25
            uint8_t pad3[4];
            // Byte 26
            uint8_t Sum2      :8;
        };
    };
} Daikin216Protocol;

const uint16_t kDaikin216Freq = 38000;  // Modulation Frequency in Hz.
const uint16_t kDaikin216HdrMark = 3440;
const uint16_t kDaikin216HdrSpace = 1750;
const uint16_t kDaikin216BitMark = 420;
const uint16_t kDaikin216OneSpace = 1300;
const uint16_t kDaikin216ZeroSpace = 450;
const uint16_t kDaikin216Gap = 29650;
const uint16_t kDaikin216Sections = 2;
const uint16_t kDaikin216Section1Length = 8;
const uint16_t kDaikin216Section2Length = kDaikin216StateLength - kDaikin216Section1Length;

const uint8_t kDaikin216SwingOn = 0b1111;
const uint8_t kDaikin216SwingOff = 0b0000;


typedef struct
{
    union
    {
        uint8_t raw[kDaikin128StateLength];  ///< The state of the IR remote.
        struct
        { 
            // Byte 0
            uint8_t       :8;
            // Byte 1
            uint8_t Mode  :4;
            uint8_t Fan   :4;
            // Byte 2
            uint8_t ClockMins   :8;
            // Byte 3
            uint8_t ClockHours  :8;
            // Byte 4
            uint8_t OnHours     :6;
            uint8_t OnHalfHour  :1;
            uint8_t OnTimer     :1;
            // Byte 5
            uint8_t OffHours    :6;
            uint8_t OffHalfHour :1;
            uint8_t OffTimer    :1;
            // Byte 6
            uint8_t Temp    :8;
            // Byte 7
            uint8_t SwingV  :1;
            uint8_t Sleep   :1;
            uint8_t         :1;  // always 1
            uint8_t Power   :1;
            uint8_t Sum1    :4;
            // Byte 8
            uint8_t         :8;
            // Byte 9
            uint8_t Ceiling :1;
            uint8_t         :1;
            uint8_t Econo   :1;
            uint8_t Wall    :1;
            uint8_t         :4;
            // Byte 10~14
            uint8_t pad[5];
            // Byte 15
            uint8_t Sum2    :8;
        };
    };
} Daikin128Protocol;


const uint16_t kDaikin128Freq = 38000;  // Modulation Frequency in Hz.
const uint16_t kDaikin128LeaderMark = 9800;
const uint16_t kDaikin128LeaderSpace = 9800;
const uint16_t kDaikin128HdrMark = 4600;
const uint16_t kDaikin128HdrSpace = 2500;
const uint16_t kDaikin128BitMark = 350;
const uint16_t kDaikin128OneSpace = 954;
const uint16_t kDaikin128ZeroSpace = 382;
const uint16_t kDaikin128Gap = 20300;
const uint16_t kDaikin128FooterMark = kDaikin128HdrMark;
const uint16_t kDaikin128Sections = 2;
const uint16_t kDaikin128SectionLength = 8;
const uint8_t kDaikin128Dry =             0b00000001;
const uint8_t kDaikin128Cool =            0b00000010;
const uint8_t kDaikin128Fan =             0b00000100;
const uint8_t kDaikin128Heat =            0b00001000;
const uint8_t kDaikin128Auto =            0b00001010;
const uint8_t kDaikin128FanAuto =         0b0001;
const uint8_t kDaikin128FanHigh =         0b0010;
const uint8_t kDaikin128FanMed =          0b0100;
const uint8_t kDaikin128FanLow =          0b1000;
const uint8_t kDaikin128FanPowerful =     0b0011;
const uint8_t kDaikin128FanQuiet =        0b1001;
const uint8_t kDaikin128MinTemp = 16;  // C
const uint8_t kDaikin128MaxTemp = 30;  // C
const uint8_t kDaikin128BitWall =         0b00001000;
const uint8_t kDaikin128BitCeiling =      0b00000001;


typedef struct
{
    union
    {
        uint64_t raw;  ///< The state of the IR remote.
        struct {
            uint8_t             :8;
            uint8_t Mode        :4;
            uint8_t Fan         :4;
            uint8_t ClockMins   :8;
            uint8_t ClockHours  :8;
            uint8_t OnHours     :6;
            uint8_t OnHalfHour  :1;
            uint8_t OnTimer     :1;
            uint8_t OffHours    :6;
            uint8_t OffHalfHour :1;
            uint8_t OffTimer    :1;
            uint8_t Temp        :8;
            uint8_t SwingV      :1;
            uint8_t Sleep       :1;
            uint8_t             :1;
            uint8_t Power       :1;
            uint8_t Sum         :4;
        };
    };
} Daikin64Protocol;


const uint16_t kDaikin64HdrMark = kDaikin128HdrMark;
const uint16_t kDaikin64BitMark = kDaikin128BitMark;
const uint16_t kDaikin64HdrSpace = kDaikin128HdrSpace;
const uint16_t kDaikin64OneSpace = kDaikin128OneSpace;
const uint16_t kDaikin64ZeroSpace = kDaikin128ZeroSpace;
const uint16_t kDaikin64LdrMark = kDaikin128LeaderMark;
const uint16_t kDaikin64Gap = kDaikin128Gap;
const uint16_t kDaikin64LdrSpace = kDaikin128LeaderSpace;
const uint16_t kDaikin64Freq = kDaikin128Freq;  // Hz.
const uint8_t kDaikin64Overhead = 9;
const int8_t  kDaikin64ToleranceDelta = 5;  // +5%

const uint64_t kDaikin64KnownGoodState = 0x7C16161607204216;
const uint8_t kDaikin64Dry =  0b0001;
const uint8_t kDaikin64Cool = 0b0010;
const uint8_t kDaikin64Fan =  0b0100;
const uint8_t kDaikin64Heat =  0b1000;
const uint8_t kDaikin64FanAuto =  0b0001;
const uint8_t kDaikin64FanLow =   0b1000;
const uint8_t kDaikin64FanMed =   0b0100;
const uint8_t kDaikin64FanHigh =  0b0010;
const uint8_t kDaikin64FanQuiet = 0b1001;
const uint8_t kDaikin64FanTurbo = 0b0011;
const uint8_t kDaikin64MinTemp = 16;  // Celsius
const uint8_t kDaikin64MaxTemp = 30;  // Celsius
const uint8_t kDaikin64ChecksumOffset = 60;
const uint8_t kDaikin64ChecksumSize = 4;  // Mask 0b1111 << 59



typedef struct
{
    union
    {
        uint8_t raw[kDaikin176StateLength];  ///< The state of the IR remote.
        struct {
            // Byte 0~2
            uint8_t      :8;
            uint8_t      :8;
            uint8_t      :8;
            // Byte 3
            uint8_t Id1  :1;
            uint8_t      :7;
            // Byte 4
            uint8_t      :8;
            // Byte 5
            uint8_t      :8;
            // Byte 6
            uint8_t Sum1 :8;
            // Byte 7-9
            uint8_t      :8;
            uint8_t      :8;
            uint8_t      :8;
            // Byte 10
            uint8_t Id2  :1;
            uint8_t      :7;
            // Byte 11
            uint8_t      :8;
            // Byte 12
            uint8_t         :4;
            uint8_t AltMode :3;
            uint8_t         :1;
            // Byte 13
            uint8_t ModeButton  :8;
            // Byte 14
            uint8_t Power :1;
            uint8_t       :3;
            uint8_t Mode  :3;
            uint8_t       :1;
            // Byte 15~16
            uint8_t pad2[2];
            // Byte 17
            uint8_t       :1;
            uint8_t Temp  :6;
            uint8_t       :1;
            // Byte 18
            uint8_t SwingH  :4;
            uint8_t Fan     :4;
            // Byte 19~20
            uint8_t pad3[2];
            // Byte 21
            uint8_t Sum2  :8;
        };
    };
} Daikin176Protocol;


const uint16_t kDaikin176Freq = 38000;  // Modulation Frequency in Hz.
const uint16_t kDaikin176HdrMark = 5070;
const uint16_t kDaikin176HdrSpace = 2140;
const uint16_t kDaikin176BitMark = 370;
const uint16_t kDaikin176OneSpace = 1780;
const uint16_t kDaikin176ZeroSpace = 710;
const uint16_t kDaikin176Gap = 29410;
const uint16_t kDaikin176Sections = 2;
const uint16_t kDaikin176Section1Length = 7;
const uint16_t kDaikin176Section2Length = kDaikin176StateLength -
                                          kDaikin176Section1Length;
const uint8_t kDaikin176Fan =  0b000;  // 0
const uint8_t kDaikin176Heat = 0b001;  // 1
const uint8_t kDaikin176Cool = 0b010;  // 2
const uint8_t kDaikin176Auto = 0b011;  // 3
const uint8_t kDaikin176Dry =  0b111;  // 7
const uint8_t kDaikin176ModeButton = 0b00000100;
const uint8_t kDaikin176DryFanTemp = 17;  // Dry/Fan mode is always 17 Celsius.
const uint8_t kDaikin176FanMax = 3;
const uint8_t kDaikin176SwingHAuto =  0x5;
const uint8_t kDaikin176SwingHOff = 0x6;


const uint16_t kDaikin312HdrMark = 3518;
const uint16_t kDaikin312HdrSpace = 1688;
const uint16_t kDaikin312BitMark = 453;
const uint16_t kDaikin312ZeroSpace = 414;
const uint16_t kDaikin312OneSpace = 1275;
const uint16_t kDaikin312HdrGap = 25100;
const uint16_t kDaikin312SectionGap = 35512;
const uint16_t kDaikin312Sections = 2;
const uint16_t kDaikin312Section1Length = 20;
const uint16_t kDaikin312Section2Length = kDaikin312StateLength -
                                          kDaikin312Section1Length;



#endif