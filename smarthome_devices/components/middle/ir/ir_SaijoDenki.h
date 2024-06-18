#ifndef IR_TROTEC_H_
#define IR_TROTEC_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

typedef struct
{
    union
    {
        uint8_t raw[kSaijoDenkiStateLength];  ///< Remote state in IR code form.
        struct {
            // Byte 0
            uint8_t :8;  // fixed value
            // Byte 1
            uint8_t  :6;
            uint8_t Sleep:1;  // fixed value
            uint8_t Power:1;  // fixed value
            // Byte 2
            uint8_t Temp  :6;
            uint8_t   :2;
            // Byte 3
            uint8_t Time_hour  :5;
            uint8_t Fan  :3;    // auto: 0, min: 1, med: 2, max: 3
            // Byte 4
            uint8_t  Time_min  :8;
            // Byte 5
            uint8_t         :5;
            uint8_t    SwingV   :3; // auto: 0, bot->top: 1-5
            // Byte 6
            uint8_t  :6;
            uint8_t Mode :2; // 0: cool, 1: dry
            // Byte 7
            uint8_t   :4;
            uint8_t       :4;
            // Byte 8
            uint8_t Sum   :8;
        };
    };
} SaijoDenkiProtocol;


#endif