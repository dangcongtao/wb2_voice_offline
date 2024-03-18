#ifndef IR_CARRIER_H_
#define IR_CARRIER_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>


typedef struct
{
    union
    {
        uint64_t raw;  ///< The state of the IR remote.
        struct {
            // Byte 0
            uint8_t :8;
            // Byte 1
            uint8_t :8;
            // Byte 2
            uint8_t Sum:4;
            uint8_t Mode:2;
            uint8_t Fan:2;
            // Byte 3
            uint8_t Temp:4;
            uint8_t :1;
            uint8_t SwingV:1;
            uint8_t :2;
            // Byte 4
            uint8_t :4;
            uint8_t Power:1;
            uint8_t OffTimerEnable:1;
            uint8_t OnTimerEnable:1;
            uint8_t Sleep:1;
            // Byte 5
            uint8_t :8;
            // Byte 6
            uint8_t :4;
            uint8_t OnTimer:4;
            // Byte 7
            uint8_t :4;
            uint8_t OffTimer:4;
        };
    };
} CarrierProtocol;


// Constants

// CARRIER_AC64
const uint8_t kCarrierAc64ChecksumOffset = 16;
const uint8_t kCarrierAc64ChecksumSize = 4;
const uint8_t kCarrierAc64Heat = 0b01;  // 1
const uint8_t kCarrierAc64Cool = 0b10;  // 2
const uint8_t kCarrierAc64Fan =  0b11;  // 3
const uint8_t kCarrierAc64FanAuto =    0b00;  // 0
const uint8_t kCarrierAc64FanLow =     0b01;  // 1
const uint8_t kCarrierAc64FanMedium =  0b10;  // 2
const uint8_t kCarrierAc64FanHigh =    0b11;  // 3
const uint8_t kCarrierAc64MinTemp = 16;  // Celsius
const uint8_t kCarrierAc64MaxTemp = 30;  // Celsius
const uint8_t kCarrierAc64TimerMax = 9;  // Hours.
const uint8_t kCarrierAc64TimerMin = 1;  // Hours.


#endif