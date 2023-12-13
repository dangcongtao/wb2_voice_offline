#ifndef IR_TOSHIBA_H_
#define IR_TOSHIBA_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

typedef struct
{
    union
    {

        uint8_t raw[kToshibaACStateLengthLong]; ///< State in code form.
        struct
        { 
            // Byte[0] - 0xF2
            uint8_t :8;
            // Byte[1] - 0x0D (inverted previous byte's value)
            uint8_t :8;
            // Byte[2] - The expected payload length (in bytes) past the Byte[4].
            ///< Known lengths are:
            ///<   1 (56 bit message)
            ///<   3 (72 bit message)
            ///<   4 (80 bit message)
            uint8_t Length   :8;
            // Byte[3] - The bit-inverted value of the "length" byte.
            uint8_t          :8;
            // Byte[4]
            uint8_t          :3;
            uint8_t LongMsg  :1;
            uint8_t          :1;
            uint8_t ShortMsg :1;
            uint8_t          :2;
            // Byte[5]
            uint8_t Swing    :3;
            uint8_t          :1;
            uint8_t Temp     :4;
            // Byte[6]
            uint8_t Mode     :3;
            uint8_t          :2;
            uint8_t Fan      :3;
            // Byte[7]
            uint8_t          :4;
            uint8_t Filter   :1;
            uint8_t          :3;

            // Byte[8]
            // (Checksum for 72 bit messages, Eco/Turbo for long 80 bit messages)
            uint8_t EcoTurbo :8;
            };
    };
} ToshibaProtocol;


#define kToshibaAcLengthByte 2  ///< Byte pos of the "length" attribute
#define kToshibaAcMinLength 6  ///< Min Nr. of bytes in a message.

#define kToshibaAcInvertedLength 4  ///< Nr. of leading bytes in
                                              ///< inverted pairs.

#define kToshibaAcSwingStep 0       ///<            0b000
#define kToshibaAcSwingOn 1         ///<            0b001
#define kToshibaAcSwingOff 2        ///<            0b010
#define kToshibaAcSwingToggle 4     ///<            0b100

#define kToshibaAcMinTemp 17  ///< 17C
#define kToshibaAcMaxTemp 30  ///< 30C

#define kToshibaAcAuto 0                // 0b000
#define kToshibaAcCool 1                // 0b001
#define kToshibaAcDry  2                // 0b010
#define kToshibaAcHeat 3                // 0b011
#define kToshibaAcFan  4                // 0b100
#define kToshibaAcOff  7                // 0b111
#define kToshibaAcFanAuto 0   //      0b000
#define kToshibaAcFanMin  1   //      0b001
#define kToshibaAcFanMed  3   //      0b011
#define kToshibaAcFanMax  5   //      0b101

#define kToshibaAcTurboOn 1       //            0b01
#define kToshibaAcEconoOn 3       //            0b11



#endif