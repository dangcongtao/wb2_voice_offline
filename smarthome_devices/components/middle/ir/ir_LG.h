#ifndef __IR_LG_H_
#define __IR_LG_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

typedef struct
{
    union
    {
        uint32_t raw; ///< The state of the IR remote in IR code form.
        struct
        {
            uint32_t Sum : 4;
            uint32_t Fan : 4;
            uint32_t Temp : 4;
            uint32_t Mode : 3;
            uint32_t BitA : 1;
            uint32_t : 2;
            uint32_t Power : 2;
            uint32_t Sign : 8;
        };
    } data;
} LGProtocol;

#define kLg32Bits 32
#define kLgBits 28

#define kLgAcFanLowest 0 // 0b0000
#define kLgAcFanLow 1    // 0b0001
#define kLgAcFanMedium 2 // 0b0010
#define kLgAcFanMax 4    // 0b0100
#define kLgAcFanAuto 5   // 0b0101
#define kLgAcFanLowAlt 9 // 0b1001
#define kLgAcFanHigh 10  // 0b1010
// Nr. of slots in the look-up table
#define kLgAcFanEntries kLgAcFanHigh + 1
#define kLgAcTempAdjust 15
#define kLgAcMinTemp 16 // Celsius
#define kLgAcMaxTemp 30 // Celsius
#define kLgAcCool 0     // 0b000
#define kLgAcDry 1      // 0b001
#define kLgAcFan 2      // 0b010
#define kLgAcAuto 3     // 0b011
#define kLgAcHeat 4     // 0b100
#define kLgAcPowerOff 3 // 0b11
#define kLgAcPowerOn 0  // 0b00
#define kLgAcSignature 0x88

#define kLgAcOffCommand 0x88C0051
#define kLgAcLightToggle 0x88C00A6
#define kLgAcJetMode 0x8810089

#define kLgAcSwingVToggle 0x8810001
#define kLgAcSwingSignature 0x8813
#define kLgAcSwingVLowest 0x8813048
#define kLgAcSwingVLow 0x8813059
#define kLgAcSwingVMiddle 0x881306A
#define kLgAcSwingVUpperMiddle 0x881307B
#define kLgAcSwingVHigh 0x881308C
#define kLgAcSwingVHighest 0x881309D
#define kLgAcSwingVSwing 0x8813149
#define kLgAcSwingVAuto kLgAcSwingVSwing
#define kLgAcSwingVOff 0x881315A
#define kLgAcSwingVLowest_Short 0x04
#define kLgAcSwingVLow_Short 0x05
#define kLgAcSwingVMiddle_Short 0x06
#define kLgAcSwingVUpperMiddle_Short 0x07
#define kLgAcSwingVHigh_Short 0x08
#define kLgAcSwingVHighest_Short 0x09
#define kLgAcSwingVSwing_Short 0x14
#define kLgAcSwingVAuto_Short kLgAcSwingVSwing_Short
#define kLgAcSwingVOff_Short 0x15

// AKB73757604 Constants
// SwingH
#define kLgAcSwingHAuto 0x881316B
#define kLgAcSwingHOff 0x881317C
// SwingV
#define kLgAcVaneSwingVHighest 1     ///< 0b001
#define kLgAcVaneSwingVHigh 2        ///< 0b010
#define kLgAcVaneSwingVUpperMiddle 3 ///< 0b011
#define kLgAcVaneSwingVMiddle 4      ///< 0b100
#define kLgAcVaneSwingVLow 5         ///< 0b101
#define kLgAcVaneSwingVLowest 6      ///< 0b110
#define kLgAcVaneSwingVSize 8
#define kLgAcSwingVMaxVanes 4 ///< Max Nr. of Vanes

#endif