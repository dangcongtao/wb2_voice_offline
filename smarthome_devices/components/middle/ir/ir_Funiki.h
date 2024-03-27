#ifndef __IR_FUNIKI_H_
#define __IR_FUNIKI_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

/*
Model: TYPE: RG10B(E)/BGEF
*/

#define kFunikiBits (kFunikiAcStateLength * 8)

#define kFunikiAcSectionLength     6

// #define kFunikiAcBitMark   (2200U)
// #define kFunikiAcHdrSpace kFunikiAcBitMark

typedef struct
{
    union
    {
        uint8_t raw[kFunikiAcStateLength]; ///< State in code form.
        struct
        { // Standard message map
            // Byte 0
            uint8_t : 8;
            // Byte 1
            uint8_t : 8;
            // Byte 2
            uint8_t : 8;
            // Byte 3
            uint8_t fan : 3;
            uint8_t  : 2;
            uint8_t power : 1;
            uint8_t  : 2;
            // Byte 4
            uint8_t : 8;
            // Byte 5
            uint8_t temp : 4;
            uint8_t mode : 4;
        };
        struct
        { // Extended message map
            // 1st Section
            // Byte 0
            uint8_t : 8;
            // Byte 1
            uint8_t : 4;
            uint8_t : 4;
            // Byte 2
            uint8_t : 4;
            uint8_t : 4;
            // Byte 3
            uint8_t : 8;
            // Byte 4
            uint8_t : 8;
            // Byte 5
        };
    } data;
} FunikiProtocol;

#define CMD_TURBO   0xAD52AF5045BA
#define CMD_SWING   0x4DB2D62907F8
#define CMD_LED     0xAD52AF50A55A



#endif