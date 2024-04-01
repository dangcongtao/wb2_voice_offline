#ifndef IR_VOLTAS_H_
#define IR_VOLTAS_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>


typedef struct
{
    union
    {
       uint8_t raw[kVoltasStateLength];  ///< The state in native IR code form
        struct {
            // Byte 0
            uint8_t SwingH         :1;
            uint8_t SwingHChange   :7;
            // Byte 1
            uint8_t Mode           :4;
            uint8_t                :1;  // Unknown/Unused
            uint8_t FanSpeed       :3;
            // Byte 2
            uint8_t SwingV         :3;
            uint8_t Wifi           :1;
            uint8_t                :1;  // Unknown/Unused
            uint8_t Turbo          :1;
            uint8_t Sleep          :1;
            uint8_t Power          :1;
            // Byte 3
            uint8_t Temp           :4;
            uint8_t                :2;  // Typically 0b01
            uint8_t Econo          :1;
            uint8_t TempSet        :1;
            // Byte 4
            uint8_t OnTimerMins    :6;  // 0-59
            uint8_t                :1;  // Unknown/Unused
            uint8_t OnTimer12Hr    :1;  // (Nr of Hours + 1) % 12.
            // Byte 5
            uint8_t OffTimerMins   :6;  // 0-59
            uint8_t                :1;  // Unknown/Unused
            uint8_t OffTimer12Hr   :1;  // (Nr of Hours + 1) % 12.
            // Byte 6
            uint8_t                :8;  // Typically 0b00111011(0x3B)
            // Byte 7
            uint8_t OnTimerHrs     :4;  // (Nr of Hours + 1) % 12.
            uint8_t OffTimerHrs    :4;  // (Nr of Hours + 1) % 12.
            // Byte 8
            uint8_t                :5;  // Typically 0b00000
            uint8_t Light          :1;
            uint8_t OffTimerEnable :1;
            uint8_t OnTimerEnable  :1;
            // Byte 9
            uint8_t Checksum       :8;
        };
    };
} VoltasProtocol;


// Constants
const uint8_t kVoltasFan   = 0b0001;  ///< 1
const uint8_t kVoltasHeat  = 0b0010;  ///< 2
const uint8_t kVoltasDry   = 0b0100;  ///< 4
const uint8_t kVoltasCool  = 0b1000;  ///< 8
const uint8_t kVoltasMinTemp = 16;  ///< Celsius
const uint8_t kVoltasDryTemp = 24;  ///< Celsius
const uint8_t kVoltasMaxTemp = 30;  ///< Celsius
const uint8_t kVoltasFanHigh = 0b001;  ///< 1
const uint8_t kVoltasFanMed =  0b010;  ///< 2
const uint8_t kVoltasFanLow =  0b100;  ///< 4
const uint8_t kVoltasFanAuto = 0b111;  ///< 7
const uint8_t kVoltasSwingHChange =   0b1111100;  ///< 0x7D
const uint8_t kVoltasSwingHNoChange = 0b0011001;  ///< 0x19


#endif