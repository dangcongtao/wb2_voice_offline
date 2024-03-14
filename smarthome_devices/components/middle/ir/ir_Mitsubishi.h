#ifndef IR_MITSUBISHI_H_
#define IR_MITSUBISHI_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

typedef struct
{
    union
    {
        uint8_t raw[kMitsubishiACStateLength];  ///< The state in code form.
        struct {
            // Byte 0~4
            uint8_t pad0[5];
            // Byte 5
            uint8_t       :5;
            uint8_t Power :1;
            uint8_t       :2;
            // Byte 6
            uint8_t       :3;
            uint8_t Mode  :3;
            uint8_t ISee : 1;
            uint8_t       :1;
            // Byte 7
            uint8_t Temp       :4;
            uint8_t HalfDegree :1;
            uint8_t            :3;
            // Byte 8
            uint8_t         :4;
            uint8_t WideVane:4;  // SwingH
            // Byte 9
            uint8_t Fan     :3;
            uint8_t Vane    :3;  // SwingV or VaneRight
            uint8_t VaneBit :1;
            uint8_t FanAuto :1;
            // Byte 10
            uint8_t Clock   :8;
            // Byte 11
            uint8_t StopClock :8;
            // Byte 12
            uint8_t StartClock:8;
            // Byte 13
            uint8_t Timer       :3;
            uint8_t WeeklyTimer :1;
            uint8_t             :4;
            // Byte 14
            uint8_t               :5;
            uint8_t Ecocool       :1;
            uint8_t               :2;
            // Byte 15
            uint8_t DirectIndirect:2;
            uint8_t AbsenseDetect :1;
            uint8_t               :2;
            uint8_t iSave10C      :1;  // i-SAVE:mode=Heat & iSave=on AND 10C on remote
            uint8_t               :2;
            // Byte 16
            uint8_t               :1;
            uint8_t NaturalFlow   :1;
            uint8_t               :1;
            uint8_t VaneLeft      :3;  // SwingV(Left)
            uint8_t               :2;
            // Byte 17
            uint8_t Sum   :8;
        };
    };
} Mitsubishi144Protocol;

#define kMitsubishiAcAuto 0b100
#define kMitsubishiAcCool 0b011
#define kMitsubishiAcDry  0b010
#define kMitsubishiAcHeat 0b001
#define kMitsubishiAcFan  0b111
#define kMitsubishiAcFanAuto 0
#define kMitsubishiAcFanMax 5
#define kMitsubishiAcFanRealMax 4
#define kMitsubishiAcFanSilent 6
#define kMitsubishiAcFanQuiet kMitsubishiAcFanSilent
#define   kMitsubishiAcMinTemp 16.0f  // 16C
#define   kMitsubishiAcMaxTemp 31.0f  // 31C
#define kMitsubishiAcVaneAuto    0b000  // Vanes move when AC wants to.
#define kMitsubishiAcVaneHighest 0b001
#define kMitsubishiAcVaneHigh    0b010
#define kMitsubishiAcVaneMiddle  0b011
#define kMitsubishiAcVaneLow     0b100
#define kMitsubishiAcVaneLowest  0b101
#define kMitsubishiAcVaneSwing   0b111 // Vanes move all the time.
#define kMitsubishiAcVaneAutoMove kMitsubishiAcVaneSwing;  // Deprecated
#define kMitsubishiAcWideVaneLeftMax  0b0001  // 1
#define kMitsubishiAcWideVaneLeft     0b0010  // 2
#define kMitsubishiAcWideVaneMiddle   0b0011  // 3
#define kMitsubishiAcWideVaneRight    0b0100  // 4
#define kMitsubishiAcWideVaneRightMax 0b0101  // 5
#define kMitsubishiAcWideVaneWide     0b0110  // 6
#define kMitsubishiAcWideVaneAuto     0b1000  // 8
#define kMitsubishiAcDirectOff    0b00  // Vanes move when AC wants to.
#define kMitsubishiAcIndirect     0b01
#define kMitsubishiAcDirect       0b11
#define kMitsubishiAcNoTimer 0
#define kMitsubishiAcStartTimer 5
#define kMitsubishiAcStopTimer 3
#define kMitsubishiAcStartStopTimer 7

typedef struct
{
    union
    {
        uint8_t raw[kMitsubishi112StateLength]; ///< State in code form.
        struct
        { 
            // Byte 0~4
            uint8_t pad0[5];
            // Byte 5
            uint8_t       :2;
            uint8_t Power :1;
            uint8_t       :5;
            // Byte 6
            uint8_t Mode  :3;
            uint8_t       :5;
            // Byte 7
            uint8_t Temp  :4;
            uint8_t       :4;
            // Byte 8
            uint8_t Fan     :3;
            uint8_t SwingV  :3;
            uint8_t         :2;
            // Byte 9~11
            uint8_t pad1[3];
            // Byte 12
            uint8_t         :2;
            uint8_t SwingH  :4;
            uint8_t         :2;
            // Byte 13
            uint8_t Sum :8;
                };
    };
} Mitsubishi112Protocol;


#define kMitsubishi112Cool                         0b011
#define kMitsubishi112Heat                         0b001
#define kMitsubishi112Auto                         0b111
#define kMitsubishi112Dry                          0b010

#define kMitsubishi112MinTemp  16  // 16C
#define kMitsubishi112MaxTemp  31  // 31C

#define kMitsubishi112FanMin                      0b010
#define kMitsubishi112FanLow                      0b011
#define kMitsubishi112FanMed                      0b101
#define kMitsubishi112FanMax                      0b000
#define kMitsubishi112FanQuiet  kMitsubishi112FanMin
#define kMitsubishi112SwingVLowest                0b101
#define kMitsubishi112SwingVLow                   0b100
#define kMitsubishi112SwingVMiddle                0b011
#define kMitsubishi112SwingVHigh                  0b010
#define kMitsubishi112SwingVHighest               0b001
#define kMitsubishi112SwingVAuto                  0b111

#define kMitsubishi112SwingHLeftMax               0b0001
#define kMitsubishi112SwingHLeft                  0b0010
#define kMitsubishi112SwingHMiddle                0b0011
#define kMitsubishi112SwingHRight                 0b0100
#define kMitsubishi112SwingHRightMax              0b0101
#define kMitsubishi112SwingHWide                  0b1000
#define kMitsubishi112SwingHAuto                  0b1100

// Legacy defines (Deprecated)
#define MITSUBISHI_AC_VANE_AUTO_MOVE kMitsubishiAcVaneAutoMove
#define MITSUBISHI_AC_VANE_AUTO kMitsubishiAcVaneAuto
#define MITSUBISHI_AC_MIN_TEMP kMitsubishiAcMinTemp
#define MITSUBISHI_AC_MAX_TEMP kMitsubishiAcMaxTemp
#define MITSUBISHI_AC_HEAT kMitsubishiAcHeat
#define MITSUBISHI_AC_FAN_SILENT kMitsubishiAcFanSilent
#define MITSUBISHI_AC_FAN_REAL_MAX kMitsubishiAcFanRealMax
#define MITSUBISHI_AC_FAN_MAX kMitsubishiAcFanMax
#define MITSUBISHI_AC_FAN_AUTO kMitsubishiAcFanAuto
#define MITSUBISHI_AC_DRY kMitsubishiAcDry
#define MITSUBISHI_AC_COOL kMitsubishiAcCool
#define MITSUBISHI_AC_AUTO kMitsubishiAcAuto




typedef struct
{
    union
    {
        uint8_t raw[kMitsubishi136StateLength];  ///< The state in code form.
        struct {
            // Byte 0~4
            uint8_t pad[5];
            // Byte 5
            uint8_t       :6;
            uint8_t Power :1;
            uint8_t       :1;
            // Byte 6
            uint8_t Mode  :3;
            uint8_t       :1;
            uint8_t Temp  :4;
            // Byte 7
            uint8_t         :1;
            uint8_t Fan     :2;
            uint8_t         :1;
            uint8_t SwingV  :4;
        };
    };
} Mitsubishi136Protocol;


#define kMitsubishi136PowerByte 5
#define kMitsubishi136MinTemp 17  // 17C
#define kMitsubishi136MaxTemp 30  // 30C
#define kMitsubishi136Fan             0b000
#define kMitsubishi136Cool            0b001
#define kMitsubishi136Heat            0b010
#define kMitsubishi136Auto            0b011
#define kMitsubishi136Dry             0b101
#define kMitsubishi136SwingVLowest   0b0000
#define kMitsubishi136SwingVLow      0b0001
#define kMitsubishi136SwingVHigh     0b0010
#define kMitsubishi136SwingVHighest  0b0011
#define kMitsubishi136SwingVAuto     0b1100
#define kMitsubishi136FanMin          0b00
#define kMitsubishi136FanLow          0b01
#define kMitsubishi136FanMed          0b10
#define kMitsubishi136FanMax          0b11
#define kMitsubishi136FanQuiet kMitsubishi136FanMin

void IRMitsubishi112checksum(Mitsubishi112Protocol *_);

#endif