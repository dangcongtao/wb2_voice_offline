#ifndef IR_VESTEL_H_
#define IR_VESTEL_H_

#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>


typedef struct 
{
    union {
        struct {
            uint64_t cmdState;
            uint64_t timeState;
        };
        struct {
            // Command
            uint64_t Signature  :12;  // 0x201
            uint64_t CmdSum     :8;
            uint64_t Swing      :4;  // auto 0xA, stop 0xF
            uint64_t TurboSleep :4;  // normal 0x1, sleep 0x3, turbo 0x7
            uint64_t            :8;
            uint64_t Temp       :4;
            uint64_t Fan        :4;
            uint64_t Mode       :3;
            uint64_t            :3;
            uint64_t Ion        :1;
            uint64_t            :1;
            uint64_t Power      :2;
            uint64_t UseCmd     :1;
            uint64_t            :0;
            // Time
            uint64_t            :12;
            uint64_t TimeSum    :8;
            uint64_t OffTenMins :3;
            uint64_t OffHours   :5;
            uint64_t OnTenMins  :3;
            uint64_t OnHours    :5;
            uint64_t Hours      :5;
            uint64_t OnTimer    :1;
            uint64_t OffTimer   :1;
            uint64_t Timer      :1;
            uint64_t Minutes    :8;
            uint64_t            :0;
        };
    };
} VestelProtocol;


// Constants
const uint16_t kVestelAcHdrMark = 3110;
const uint16_t kVestelAcHdrSpace = 9066;
const uint16_t kVestelAcBitMark = 520;
const uint16_t kVestelAcOneSpace = 1535;
const uint16_t kVestelAcZeroSpace = 480;
const uint16_t kVestelAcTolerance = 30;

const uint8_t kVestelAcMinTempH = 16;
const uint8_t kVestelAcMinTempC = 18;
const uint8_t kVestelAcMaxTemp = 30;

const uint8_t kVestelAcAuto = 0;
const uint8_t kVestelAcCool = 1;
const uint8_t kVestelAcDry = 2;
const uint8_t kVestelAcFan = 3;
const uint8_t kVestelAcHeat = 4;

const uint8_t kVestelAcFanAuto = 1;
const uint8_t kVestelAcFanLow = 5;
const uint8_t kVestelAcFanMed = 9;
const uint8_t kVestelAcFanHigh = 0xB;
const uint8_t kVestelAcFanAutoCool = 0xC;
const uint8_t kVestelAcFanAutoHot = 0xD;

const uint8_t kVestelAcNormal = 1;
const uint8_t kVestelAcSleep = 3;
const uint8_t kVestelAcTurbo = 7;
const uint8_t kVestelAcIon = 4;
const uint8_t kVestelAcSwing = 0xA;

// Default states
const uint64_t kVestelAcStateDefault = 0x0F00D9001FEF201ULL;
const uint64_t kVestelAcTimeStateDefault = 0x201ULL;


#endif