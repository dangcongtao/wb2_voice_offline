#ifndef IR_BOSCH_H_
#define IR_BOSCH_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>


// Constants
const uint16_t kBoschHdrMark = 4366;
const uint16_t kBoschBitMark = 502;
const uint16_t kBoschHdrSpace = 4415;
const uint16_t kBoschOneSpace = 1645;
const uint16_t kBoschZeroSpace = 571;
const uint16_t kBoschFooterSpace = 5235;
const uint16_t kBoschFreq = 38000;  // Hz. (Guessing the most common frequency.)
const uint16_t kBosch144NrOfSections = 3;
const uint16_t kBosch144BytesPerSection = 6;
// Modes                Bit[0] to Section 3    Bit[1-2] to Section 1
//                                ModeS3                   ModeS1
const uint8_t kBosch144Cool = 0b000;
const uint8_t kBosch144Dry =  0b011;
const uint8_t kBosch144Auto = 0b101;
const uint8_t kBosch144Heat = 0b110;
const uint8_t kBosch144Fan =  0b010;

// Fan Control          Bit[0-5] to Section 3    Bit[6-8] to Section 1
//                                  FanS3                    FanS1
const uint16_t kBosch144Fan20 = 0b111001010;
const uint16_t kBosch144Fan40 = 0b100010100;
const uint16_t kBosch144Fan60 = 0b010011110;
const uint16_t kBosch144Fan80 = 0b001101000;
const uint16_t kBosch144Fan100 = 0b001110010;
const uint16_t kBosch144FanAuto = 0b101110011;
const uint16_t kBosch144FanAuto0 = 0b000110011;

// Temperature
const uint8_t kBosch144TempMin = 16;  // Celsius
const uint8_t kBosch144TempMax = 30;  // Celsius
const uint8_t kBosch144TempRange = kBosch144TempMax - kBosch144TempMin + 1;
const uint8_t kBosch144TempMap[] = {
    0b00001,  // 16C      // Bit[0] to Section 3    Bit[1-4] to Section 1
    0b00000,  // 17C      //           TempS3                   TempS1
    0b00010,  // 18c
    0b00110,  // 19C
    0b00100,  // 20C
    0b01100,  // 21C
    0b01110,  // 22C
    0b01010,  // 23C
    0b01000,  // 24C
    0b11000,  // 25C
    0b11010,  // 26C
    0b10010,  // 27C
    0b10000,  // 28C
    0b10100,  // 29C
    0b10110   // 30C
};

// "OFF" is a 96bit-message    the same as Coolix protocol
const uint8_t kBosch144Off[] = {0xB2, 0x4D, 0x7B, 0x84, 0xE0, 0x1F,
                                0xB2, 0x4D, 0x7B, 0x84, 0xE0, 0x1F};

// On, 25C, Mode: Auto
const uint8_t kBosch144DefaultState[kBosch144StateLength] = {
  0xB2, 0x4D, 0x1F, 0xE0, 0xC8, 0x37,
  0xB2, 0x4D, 0x1F, 0xE0, 0xC8, 0x37,
  0xD5, 0x65, 0x00, 0x00, 0x00, 0x3A};


typedef struct
{
    union
    {
       uint8_t raw[kBosch144StateLength];  ///< The state in IR code form.
        struct {
            uint8_t            :8;   // Fixed value 0b10110010 / 0xB2.   ############
            uint8_t InnvertS1_1:8;   // Invert byte  0b01001101 / 0x4D   #
            uint8_t            :5;   // not used (without timer use)     #
            uint8_t FanS1      :3;   // Fan speed bits in Section 1      #
            uint8_t InnvertS1_2:8;   // Invert byte                      #  Section 1 =
            uint8_t            :2;   // not used (without timer use)     #   Sektion 2
            uint8_t ModeS1     :2;   // Operation mode bits S1           #
            uint8_t TempS1     :4;   // Desired temperature (Celsius) S2 #
            uint8_t InnvertS1_3:8;   // Invert byte (without timer use)  ############

            uint8_t            :8;   // Fixed value 0b10110010 / 0xB2.   ############
            uint8_t InnvertS2_1:8;   // Invert byte  0b01001101 / 0x4D   #
            uint8_t            :5;   // not used (without timer use)     #
            uint8_t FanS2      :3;   // Fan speed bits in Section 2      #
            uint8_t InnvertS2_2:8;   // Invert byte                      #  Section 2 =
            uint8_t            :2;   // not used (without timer use)     #   Sektion 1
            uint8_t ModeS2     :2;   // Operation mode bits S2           #
            uint8_t TempS2     :4;   // Desired temperature (Celsius) S2 #
            uint8_t InnvertS2_3:8;   // Invert byte (without timer use)  ###########

            uint8_t            :8;   // Fixed value 0b11010101 / 0xD5    ###########
            uint8_t ModeS3     :1;   // ModeBit in Section 3             #
            uint8_t FanS3      :6;   // Fan speed bits in Section 3      #
            uint8_t            :1;   // Unknown                          #
            uint8_t            :7;   // Unknown                          #
            uint8_t Quiet      :1;   // Silent-Mode                      #    Section 3
            uint8_t            :4;   // Unknown                          #
            uint8_t TempS3     :1;   // Desired temp. Bit in Section3    #
            uint8_t            :3;   // Unknown                          #
            uint8_t            :8;   // Unknown                          #
            uint8_t ChecksumS3 :8;   // Checksum from byte 13-17         ###########
        };
    };
} Bosch144Protocol;



#endif