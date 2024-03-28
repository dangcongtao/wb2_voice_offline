#ifndef IR_TECO_H_
#define IR_TECO_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>


typedef struct 
{
    union {
        uint64_t raw;  ///< The state of the IR remote in IR code form.
        struct {
            uint8_t Mode  :3;
            uint8_t Power :1;
            uint8_t Fan   :2;
            uint8_t Swing :1;
            uint8_t Sleep :1;
            uint8_t Temp  :4;
            uint8_t HalfHour  :1;
            uint8_t TensHours :2;  // number of 10hours
            uint8_t TimerOn   :1;
            uint8_t UnitHours :4;  // unit, not thenth
            uint8_t Humid :1;
            uint8_t Light :1;
            uint8_t :1;  // "Tree with bubbles" / Filter?? (Not Implemented)
            uint8_t Save  :1;
            uint8_t :8;  // Cst 0x50
            uint8_t :8;  // Cst 0x02
        };
    };
} TecoProtocol;


// Constants.
const uint8_t kTecoAuto = 0;  // temp = 25C
const uint8_t kTecoCool = 1;
const uint8_t kTecoDry = 2;  // temp = 25C, but not shown
const uint8_t kTecoFan = 3;
const uint8_t kTecoHeat = 4;
const uint8_t kTecoFanAuto = 0;  // 0b00
const uint8_t kTecoFanLow = 1;   // 0b01
const uint8_t kTecoFanMed = 2;   // 0b10
const uint8_t kTecoFanHigh = 3;  // 0b11
const uint8_t kTecoMinTemp = 16;   // 16C
const uint8_t kTecoMaxTemp = 30;   // 30C

const uint64_t kTecoReset =      0b01001010000000000000010000000000000;




#endif