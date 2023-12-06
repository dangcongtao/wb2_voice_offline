#ifndef __IR_AQUA_H_
#define __IR_AQUA_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>
#include <IRsend.h>

#define kCasperBits (kCasperAcStateLength * 8)

#define kCasperAcSectionLength     6


typedef struct
{
    union
    {
        uint8_t raw[kCasperAcStateLength]; ///< State in code form.
        struct
        { // Standard message map
            // Byte 0
            uint8_t mode : 8;
            // Byte 1
            uint8_t reverse_temp : 4;
            uint8_t : 4;
            // Byte 2
            uint8_t reverse_mode : 8;
            // Byte 3
            uint8_t : 8;
            // Byte 4
            uint8_t : 8;
            // Byte 5
            uint8_t : 8;
            // Byte 6
            uint8_t : 8;
            // Byte 7
            uint8_t : 8;
        };
    } data;
} CasperProtocol;


#endif