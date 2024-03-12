#ifndef IR_NAGAKAWA_H_
#define IR_NAGAKAWA_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>

typedef struct
{
    union
    {
        uint8_t raw[kNagakawaStateLength]; ///< State in code form.
        struct
        { 
            // Byte 0~2
            uint8_t                 :8;
            uint8_t                 :8;
            uint8_t                 :8;
            // Byte 3
            uint8_t MsgType         :2;
            uint8_t                 :6;
            // Byte 4
            uint8_t                 :8;
            // Byte 5
            uint8_t                 :2;
            uint8_t Power           :1;
            uint8_t OffTimerEnabled :1;
            uint8_t OnTimerEnabled  :1;
            uint8_t Quiet           :1;
            uint8_t Light           :1;
            uint8_t Econo           :1;
            // Byte 6
            uint8_t Mode            :4;
            uint8_t Health          :1;
            uint8_t Turbo           :1;
            uint8_t                 :2;
            // Byte 7
            uint8_t Temp            :4;
            uint8_t                 :4;
            // Byte 8
            uint8_t Fan             :3;
            uint8_t SwingV          :3;
            uint8_t TimerIndicator  :1;
            uint8_t                 :1;
            // Byte 9
            uint8_t                 :1;  // 0
            uint8_t OffTimer        :6;
            uint8_t                 :1;  // 0
            // Byte 10
            uint8_t                 :1;  // 0
            uint8_t OnTimer         :6;
            uint8_t                 :1;  // 0
            // Byte 11
            uint8_t                 :8;  // 00000000
            // Byte 12
            uint8_t                 :3;
            uint8_t SwingH          :1;
            uint8_t                 :1;
            uint8_t HalfDegree      :1;
            uint8_t                 :2;
            // Byte 13
            uint8_t Sum             :8;
        };
    };
} NagakawaProtocol;

#define kNagakawaHdrMark 3630
#define kNagakawaHdrSpace  1450
#define kNagakawaBitMark  430
#define kNagakawaOneSpace  1400
#define kNagakawaZeroSpace  430
#define kNagakawaGap kDefaultMessageGap  // Just a guess.

#define kNagakawaHeat  1
#define kNagakawaDry   2
#define kNagakawaCool  3
#define kNagakawaFan   7
#define kNagakawaAuto  0

#define kNagakawaFanAuto  0b000
#define kNagakawaFanMin   0b010  // Aka. "Night"
#define kNagakawaFanMed   0b011
#define kNagakawaFanMax  0b101

#define   kNagakawaTempMax     31.0
#define   kNagakawaTempMin     16.0

#define kNagakawaSwingVOff      0b000
#define kNagakawaSwingVOn       0b111


#endif