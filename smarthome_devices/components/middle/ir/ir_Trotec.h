#ifndef IR_TROTEC_H_
#define IR_TROTEC_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

typedef struct
{
    union
    {
        uint8_t raw[kTrotecStateLength];  ///< Remote state in IR code form.
        struct {
            // Byte 0
            uint8_t Intro1:8;  // fixed value
            // Byte 1
            uint8_t Intro2:8;  // fixed value
            // Byte 2
            uint8_t Mode  :2;
            uint8_t       :1;
            uint8_t Power :1;
            uint8_t Fan   :2;
            uint8_t       :2;
            // Byte 3
            uint8_t Temp  :4;
            uint8_t       :3;
            uint8_t Sleep :1;
            // Byte 4
            uint8_t       :8;
            // Byte 5
            uint8_t       :6;
            uint8_t Timer :1;
            uint8_t       :1;
            // Byte 6
            uint8_t Hours :8;
            // Byte 7
            uint8_t       :8;
            // Byte 8
            uint8_t Sum   :8;
        };
    };
} TrotecProtocol;


// Constants
const uint8_t kTrotecIntro1 = 0x12;
const uint8_t kTrotecIntro2 = 0x34;

const uint8_t kTrotecAuto = 0;
const uint8_t kTrotecCool = 1;
const uint8_t kTrotecDry = 2;
const uint8_t kTrotecFan = 3;

const uint8_t kTrotecFanLow = 1;
const uint8_t kTrotecFanMed = 2;
const uint8_t kTrotecFanHigh = 3;

const uint8_t kTrotecMinTemp = 18;
const uint8_t kTrotecDefTemp = 25;
const uint8_t kTrotecMaxTemp = 32;

const uint8_t kTrotecMaxTimer = 23;


typedef struct
{
    union
    {
        uint8_t raw[kTrotecStateLength];  ///< Remote state in IR code form.
        struct {
            // Byte 0
            uint8_t Intro:    8;  // fixed value (0x55)
            // Byte 1
            uint8_t SwingV   :1;
            uint8_t Power    :1;
            uint8_t          :1;  // Unknown
            uint8_t TimerSet :1;
            uint8_t TempC    :4;  // Temp + kTrotec3550MinTempC for degC)
            // Byte 2
            uint8_t TimerHrs :4;
            uint8_t          :4;  // Unknown
            // Byte 3
            uint8_t TempF    :5;  // Temp + kTrotec3550MinTempF for degF)
            uint8_t          :3;  // Unknown
            // Byte 4
            uint8_t          :8;  // Unknown
            // Byte 5
            uint8_t          :8;  // Unknown
            // Byte 6
            uint8_t Mode     :2;
            uint8_t          :2;  // Unknown
            uint8_t Fan      :2;
            uint8_t          :2;  // Unknown
            // Byte 7
            uint8_t          :7;  // Unknown
            uint8_t Celsius  :1;  // DegC or DegF
            // Byte 8
            uint8_t Sum      :8;
        };
    };
} Trotec3550Protocol;


const uint8_t kTrotec3550MinTempC = 16;
const uint8_t kTrotec3550MaxTempC = 30;
const uint8_t kTrotec3550MinTempF = 59;
const uint8_t kTrotec3550MaxTempF = 86;


#endif