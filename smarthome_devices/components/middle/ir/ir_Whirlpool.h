#ifndef IR_WHIRLPOOL_H_
#define IR_WHIRLPOOL_H_


#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>


typedef struct
{
    union
    {
        uint8_t raw[kWhirlpoolAcStateLength];  ///< The state in IR code form
        struct {
            // Byte 0~1
            uint8_t pad0[2];
            // Byte 2
            uint8_t Fan     :2;
            uint8_t Power   :1;
            uint8_t Sleep   :1;
            uint8_t         :3;
            uint8_t Swing1  :1;
            // Byte 3
            uint8_t Mode  :3;
            uint8_t       :1;
            uint8_t Temp  :4;
            // Byte 4
            uint8_t       :8;
            // Byte 5
            uint8_t         :4;
            uint8_t Super1  :1;
            uint8_t         :2;
            uint8_t Super2  :1;
            // Byte 6
            uint8_t ClockHours  :5;
            uint8_t LightOff    :1;
            uint8_t             :2;
            // Byte 7
            uint8_t ClockMins       :6;
            uint8_t                 :1;
            uint8_t OffTimerEnabled :1;
            // Byte 8
            uint8_t OffHours  :5;
            uint8_t           :1;
            uint8_t Swing2    :1;
            uint8_t           :1;
            // Byte 9
            uint8_t OffMins         :6;
            uint8_t                 :1;
            uint8_t OnTimerEnabled  :1;
            // Byte 10
            uint8_t OnHours :5;
            uint8_t         :3;
            // Byte 11
            uint8_t OnMins  :6;
            uint8_t         :2;
            // Byte 12
            uint8_t       :8;
            // Byte 13
            uint8_t Sum1  :8;
            // Byte 14
            uint8_t       :8;
            // Byte 15
            uint8_t Cmd   :8;
            // Byte 16~17
            uint8_t pad1[2];
            // Byte 18
            uint8_t             :3;
            uint8_t J191        :1;
            uint8_t             :4;
            // Byte 19
            uint8_t       :8;
            // Byte 20
            uint8_t Sum2  :8;
        };
        struct {
            // Byte 0~1
            uint8_t       :8;
            uint8_t       :8;
            // Byte 2
            uint8_t      :2;
            uint8_t    :1;
            uint8_t    :1;
            uint8_t         :3;
            uint8_t   :1;
            // Byte 3
            uint8_t   :3;
            uint8_t       :1;
            uint8_t   :4;
            // Byte 4
            uint8_t       :8;
            // Byte 5
            uint8_t         :4;
            uint8_t   :1;
            uint8_t         :2;
            uint8_t   :1;
            // Byte 6
            uint8_t   :5;
            uint8_t     :1;
            uint8_t             :2;
            // Byte 7
            uint8_t        :6;
            uint8_t                 :1;
            uint8_t  :1;
            // Byte 8
            uint8_t   :5;
            uint8_t           :1;
            uint8_t     :1;
            uint8_t           :1;
            // Byte 9
            uint8_t          :6;
            uint8_t                 :1;
            uint8_t   :1;
            // Byte 10
            uint8_t  :5;
            uint8_t         :3;
            // Byte 11
            uint8_t   :6;
            uint8_t         :2;
            // Byte 12
            uint8_t       :8;
            // Byte 13
            uint8_t   :8;
            // Byte 14
            uint8_t       :8;
            // Byte 15
            uint8_t    :8;
            // Byte 16~17
            uint8_t :8;
            uint8_t :1;
            uint8_t Fan1    :1;
            uint8_t :6;
            // Byte 18
            uint8_t             :4;
            uint8_t Power1            :1;
            uint8_t isCasper    :1;
            uint8_t             :2;
            // Byte 19
            uint8_t       :8;
            // Byte 20
            uint8_t   :8;
        };
    };
} WhirlpoolProtocol;


// Constants
const uint8_t kWhirlpoolAcChecksumByte1 = 13;
const uint8_t kWhirlpoolAcChecksumByte2 = kWhirlpoolAcStateLength - 1;
const uint8_t kWhirlpoolAcHeat = 0;
const uint8_t kWhirlpoolAcAuto = 1;
const uint8_t kWhirlpoolAcCool = 2;
const uint8_t kWhirlpoolAcDry = 3;
const uint8_t kWhirlpoolAcFan = 4;
const uint8_t kWhirlpoolAcFanAuto = 0;
const uint8_t kWhirlpoolAcFanHigh = 1;
const uint8_t kWhirlpoolAcFanMedium = 2;
const uint8_t kWhirlpoolAcFanLow = 3;
const uint8_t kWhirlpoolAcMinTemp = 18;     // 18C (DG11J1-3A), 16C (DG11J1-91)
const uint8_t kWhirlpoolAcMaxTemp = 32;     // 32C (DG11J1-3A), 30C (DG11J1-91)
const uint8_t kWhirlpoolAcAutoTemp = 23;    // 23C
const uint8_t kWhirlpoolAcCommandLight = 0x00;
const uint8_t kWhirlpoolAcCommandPower = 0x01;
const uint8_t kWhirlpoolAcCommandTemp = 0x02;
const uint8_t kWhirlpoolAcCommandSleep = 0x03;
const uint8_t kWhirlpoolAcCommandSuper = 0x04;
const uint8_t kWhirlpoolAcCommandOnTimer = 0x05;
const uint8_t kWhirlpoolAcCommandMode = 0x06;
const uint8_t kWhirlpoolAcCommandSwing = 0x07;
const uint8_t kWhirlpoolAcCommandIFeel = 0x0D;
const uint8_t kWhirlpoolAcCommandFanSpeed = 0x11;
const uint8_t kWhirlpoolAcCommand6thSense = 0x17;
const uint8_t kWhirlpoolAcCommandOffTimer = 0x1D;


#endif