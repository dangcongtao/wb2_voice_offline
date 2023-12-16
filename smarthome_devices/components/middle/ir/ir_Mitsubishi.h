#ifndef IR_MITSUBISHI_H_
#define IR_MITSUBISHI_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

#define   kMitsubishiAcMaxTemp  31.0

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

void IRMitsubishi112checksum(Mitsubishi112Protocol *_);

#endif