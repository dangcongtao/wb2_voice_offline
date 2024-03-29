#ifndef IR_YORK_H_
#define IR_YORK_H_


#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>


typedef struct
{
    union
    {
        uint8_t raw[kYorkStateLength];  ///< The state of the IR remote.
        struct {
            // byte 0-5
            uint8_t preamble[6];  // unknown, fixed 0x08, 0x10, 0x07, 0x02, 0x40, 0x08
            // byte 6
            uint8_t Key1     :4;  // key pressed on the remote: 1 power,  2 temp up, 3
                                // temp down...
            uint8_t Key2     :4;  // only set when setting ontime/offtime:
                                // Key1 value is 0x6 (enter key) and Key2 is 0x3 for
                                // "start" and 0x2 for "stop"
            // byte 7
            uint8_t Fan      :4;  // Fan speed: 1 low, 2 mid, 3 max, 8 auto
            uint8_t Power    :1;  // main unit power: 1 on, 0 off
            uint8_t          :3;
            // byte 8
            uint8_t Mode     :4;  // 1 heat, 2 cool, 3 dry, 4 fan, 8 auto
            uint8_t          :4;
            // byte 9
            uint8_t          :2;
            uint8_t Temp     :6;  // Degrees Celsius
            // byte 10
            uint8_t OffTimer :8;  // Power off time: 10s of minutes from now
            // byte 11
            uint8_t OnTimer  :8;  // Power on time: 10s of minutes from now
            // byte 12
            uint8_t          :8;  // unknown, normally 0x00, could be 0x08 when ontime
                                // set, 0x88 if both on and offtime set, 0x60 if
                                // sleep mode set
            // byte 13
            uint8_t SwingV   :1;  // 0 off, 1 on
            uint8_t          :7;
            // byte 14
            uint8_t          :8;  // checksum preamble, fixed 0xEC
            // byte 15-16
            uint8_t Chk1     :8;  // checksum, algorithm CRC-16/ARC, first byte
            uint8_t Chk2     :8;  // checksum, algorithm CRC-16/ARC, second byte
        };
    };
} YorkProtocol;



#endif