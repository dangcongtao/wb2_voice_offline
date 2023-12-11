#ifndef __IR_AQUA_H_
#define __IR_AQUA_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>
#include <IRsend.h>

#define kFunikiBits (kAquaAcStateLength * 8)

#define kAquaAcSectionLength     14

typedef struct
{
    union
    {
        uint8_t raw[kAquaAcStateLength]; ///< State in code form.
        struct
        { // Standard message map
            // Byte 0
            uint8_t : 8;
            // Byte 1
            uint8_t reverse_temp : 4;
            uint8_t reverse_power : 4;
            // Byte 2
            uint8_t : 8;
            // Byte 3
            uint8_t : 8;
            // Byte 4
            uint8_t : 1;
            uint8_t power : 1;
            uint8_t : 6;
            // Byte 5
            uint8_t fan : 4;
            uint8_t : 4;
            // Byte 6
            uint8_t : 8;
            // Byte 7
            uint8_t mode : 4;
            uint8_t : 4;
            // Byte 8
            uint8_t : 8;
            // Byte 9
            uint8_t : 8;
            // Byte 10
            uint8_t : 8;
            // Byte 11
            uint8_t : 8;
            // Byte 12
            uint8_t : 8;
            // Byte 13
            uint8_t : 8;
        };
    } data;
} AquaProtocol;

#endif