#ifndef IR_DELONGHI_H_
#define IR_DELONGHI_H_


#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

typedef struct
{
    union
    {
        uint64_t raw;  ///< The state of the IR remote.
        struct {
            uint8_t           :8;  // Header
            uint8_t Temp      :5;
            uint8_t Fan       :2;
            uint8_t Fahrenheit:1;
            uint8_t Power     :1;
            uint8_t Mode      :3;
            uint8_t Boost     :1;
            uint8_t Sleep     :1;
            uint8_t           :2;
            uint8_t OnTimer   :1;
            uint8_t OnHours   :5;
            uint8_t           :2;
            uint8_t OnMins    :6;
            uint8_t           :2;
            uint8_t OffTimer  :1;
            uint8_t OffHours  :5;
            uint8_t           :2;
            uint8_t OffMins   :6;
            uint8_t           :2;
            uint8_t Sum       :8;
        };
    };
} DelonghiProtocol;



// Constants
const uint8_t kDelonghiAcTempMinC = 18;  // Deg C
const uint8_t kDelonghiAcTempMaxC = 32;  // Deg C
const uint8_t kDelonghiAcTempMinF = 64;  // Deg F
const uint8_t kDelonghiAcTempMaxF = 90;  // Deg F
const uint8_t kDelonghiAcTempAutoDryMode = 0;
const uint8_t kDelonghiAcTempFanMode = 0b00110;
const uint8_t kDelonghiAcFanAuto =   0b00;
const uint8_t kDelonghiAcFanHigh =   0b01;
const uint8_t kDelonghiAcFanMedium = 0b10;
const uint8_t kDelonghiAcFanLow =    0b11;
const uint8_t kDelonghiAcCool =   0b000;
const uint8_t kDelonghiAcDry =    0b001;
const uint8_t kDelonghiAcFan =    0b010;
const uint8_t kDelonghiAcAuto =   0b100;
const uint16_t kDelonghiAcTimerMax = 23 * 60 + 59;
const uint8_t kDelonghiAcChecksumOffset = 56;


#endif