#ifndef IR_KELVINATOR_H_
#define IR_KELVINATOR_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

typedef struct
{
    union
    {
        uint8_t raw[kKelvinatorStateLength];  ///< The state in IR code form.
        struct {
            // Byte 0
            uint8_t Mode      :3;
            uint8_t Power     :1;
            uint8_t BasicFan  :2;
            uint8_t SwingAuto :1;
            uint8_t           :1;  // Sleep Modes 1 & 3 (1 = On, 0 = Off)
            // Byte 1
            uint8_t Temp  :4;  // Degrees C.
            uint8_t       :4;
            // Byte 2
            uint8_t           :4;
            uint8_t Turbo     :1;
            uint8_t Light     :1;
            uint8_t IonFilter :1;
            uint8_t XFan      :1;
            // Byte 3
            uint8_t :4;
            uint8_t :2;  // (possibly timer related) (Typically 0b01)
            uint8_t :2;  // End of command block (B01)
            // (B010 marker and a gap of 20ms)
            // Byte 4
            uint8_t SwingV  :4;
            uint8_t SwingH  :1;
            uint8_t         :3;
            // Byte 5~6
            uint8_t pad0[2];  // Timer related. Typically 0 except when timer in use.
            // Byte 7
            uint8_t       :4;  // (Used in Timer mode)
            uint8_t Sum1  :4;  // checksum of the previous bytes (0-6)
            // (gap of 40ms)
            // (header mark and space)
            // Byte 8~10
            uint8_t pad1[3];  // Repeat of byte 0~2
            // Byte 11
            uint8_t :4;
            uint8_t :2;  // (possibly timer related) (Typically 0b11)
            uint8_t :2;  // End of command block (B01)
            // (B010 marker and a gap of 20ms)
            // Byte 12
            uint8_t       :1;  // Sleep mode 2 (1 = On, 0=Off)
            uint8_t       :6;  // (Used in Sleep Mode 3, Typically 0b000000)
            uint8_t Quiet :1;
            // Byte 13
            uint8_t     :8;  // (Sleep Mode 3 related, Typically 0x00)
            // Byte 14
            uint8_t     :4;  // (Sleep Mode 3 related, Typically 0b0000)
            uint8_t Fan :3;
            // Byte 15
            uint8_t       :4;
            uint8_t Sum2  :4;  // checksum of the previous bytes (8-14)
        };
    };
} KelvinatorProtocol;


// Constants
#define kKelvinatorAuto  0  // (temp  25C)
#define kKelvinatorCool  1
#define kKelvinatorDry  2  // (temp  25C, but not shown)
#define kKelvinatorFan  3
#define kKelvinatorHeat  4
#define kKelvinatorBasicFanMax  3
#define kKelvinatorFanAuto  0
#define kKelvinatorFanMin  1
#define kKelvinatorFanMax  5
#define kKelvinatorMinTemp  16   // 16C
#define kKelvinatorMaxTemp  30   // 30C
#define kKelvinatorAutoTemp  25  // 25C

#define kKelvinatorSwingVOff          0b0000  // 0
#define kKelvinatorSwingVAuto         0b0001  // 1
#define kKelvinatorSwingVHighest      0b0010  // 2
#define kKelvinatorSwingVUpperMiddle  0b0011  // 3
#define kKelvinatorSwingVMiddle       0b0100  // 4
#define kKelvinatorSwingVLowerMiddle  0b0101  // 5
#define kKelvinatorSwingVLowest       0b0110  // 6
#define kKelvinatorSwingVLowAuto      0b0111  // 7
#define kKelvinatorSwingVMiddleAuto   0b1001  // 9
#define kKelvinatorSwingVHighAuto     0b1011  // 11

// Legacy defines (Deprecated)
#define KELVINATOR_MIN_TEMP kKelvinatorMinTemp
#define KELVINATOR_MAX_TEMP kKelvinatorMaxTemp
#define KELVINATOR_HEAT kKelvinatorHeat
#define KELVINATOR_FAN_MAX kKelvinatorFanMax
#define KELVINATOR_FAN_AUTO kKelvinatorFanAuto
#define KELVINATOR_FAN kKelvinatorFan
#define KELVINATOR_DRY kKelvinatorDry
#define KELVINATOR_COOL kKelvinatorCool
#define KELVINATOR_BASIC_FAN_MAX kKelvinatorBasicFanMax
#define KELVINATOR_AUTO_TEMP kKelvinatorAutoTemp
#define KELVINATOR_AUTO kKelvinatorAuto

uint8_t IRKelvinatorAC_calcBlockChecksum(const uint8_t *block,
                                          const uint16_t length);
#endif