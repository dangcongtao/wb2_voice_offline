#ifndef IR_MIRAGE_H_
#define IR_MIRAGE_H_


#include <stdint.h>
#include <stdbool.h>
#include <IRrecv.h>


typedef struct 
{
    union {
        uint8_t raw[kMirageStateLength];  ///< The state in code form.
        struct {  // Common
            // Byte 0
            uint8_t Header        :8;  // Header. (0x56)
            // Byte 1
            uint8_t Temp          :8;  // Celsius minus 0x5C.
            // Byte 2
            uint8_t               :8;  // Unknown / Unused.
            // Byte 3
            uint8_t               :8;  // Unknown / Unused.
            // Byte 4
            uint8_t Fan           :2;  // Fan Speed.
            uint8_t               :2;  // Unknown / Unused.
            uint8_t Mode          :4;  // Cool, Heat, Dry, Fan, Recycle
            // Byte 5
            uint8_t               :8;
            // Byte 6
            uint8_t               :8;
            // Byte 7
            uint8_t               :8;
            // Byte 8
            uint8_t               :8;
            // Byte 9
            uint8_t               :8;
            // Byte 10
            uint8_t               :8;
            // Byte 11
            uint8_t               :8;
            // Byte 12
            uint8_t               :8;
            // Byte 13
            uint8_t               :8;
            // Byte 14
            uint8_t Sum           :8;  // Sum of all the previous nibbles.
        };
        struct {  // KKG9AC1 remote
            // Byte 0
            uint8_t               :8;  // Header
            // Byte 1
            uint8_t               :8;  // Temp
            // Byte 2
            uint8_t               :8;  // Unknown / Unused.
            // Byte 3
            uint8_t               :3;  // Unknown / Unused.
            uint8_t Light_Kkg9ac1 :1;  // Aka. Display. Seems linked to Sleep mode.
            uint8_t               :4;  // Unknown / Unused.
            // Byte 4
            uint8_t               :8;  // Fan & Mode
            // Byte 5
            uint8_t               :1;  // Unknown
            uint8_t SwingAndPower :7;
            // Byte 6
            uint8_t               :7;  // Unknown / Unused.
            uint8_t Sleep_Kkg9ac1 :1;  // Sleep mode on or off.
            // Byte 7
            uint8_t               :3;  // Unknown / Unused.
            uint8_t Turbo_Kkg9ac1 :1;  // Turbo mode on or off. Only works in Cool mode.
            uint8_t               :4;  // Unknown / Unused.
            // Byte 8
            uint8_t               :8;  // Unknown / Unused.
            // Byte 9
            uint8_t               :8;  // Unknown / Unused.
            // Byte 10
            uint8_t               :8;  // Unknown / Unused.
            // Byte 11
            uint8_t Seconds       :8;  // Nr. of Seconds in BCD.
            // Byte 12
            uint8_t Minutes       :8;  // Nr. of Minutes in BCD.
            // Byte 13
            uint8_t Hours         :8;  // Nr. of Hours in BCD.
            // Byte 14
            uint8_t               :8;  // Sum
        };
        struct {  // KKG29A-C1 remote
            // Byte 0
            uint8_t                :8;  // Header
            // Byte 1
            uint8_t                :8;  // Temp
            // Byte 2
            uint8_t                :8;
            // Byte 3
            uint8_t Quiet          :1;
            uint8_t                :7;
            // Byte 4
            uint8_t                :2;  // Fan
            uint8_t OffTimerEnable :1;
            uint8_t OnTimerEnable  :1;
            uint8_t                :3;  // Mode
            uint8_t                :1;
            // Byte 5
            uint8_t SwingH         :1;
            uint8_t SwingV         :1;
            uint8_t LightToggle_Kkg29ac1 :1;  // Aka. Display Toggle.
            uint8_t                :3;
            uint8_t Power          :2;
            // Byte 6
            uint8_t                :1;
            uint8_t Filter         :1;  // Aka. UVC
            uint8_t                :1;
            uint8_t Sleep_Kkg29ac1 :1;  // Sleep mode on or off.
            uint8_t                :2;
            uint8_t RecycleHeat    :1;
            uint8_t                :1;
            // Byte 7
            uint8_t SensorTemp     :6;  // Temperature at the remote
            uint8_t CleanToggle    :1;
            uint8_t IFeel          :1;
            // Byte 8
            uint8_t OnTimerHours   :5;
            uint8_t                :2;
            uint8_t Turbo_Kkg29ac1 :1;  // Turbo mode on or off.
            // Byte 9
            uint8_t OnTimerMins    :6;
            uint8_t                :2;
            // Byte 10
            uint8_t OffTimerHours  :5;
            uint8_t                :3;
            // Byte 11
            uint8_t OffTimerMins   :6;
            uint8_t                :2;
            // Byte 12
            uint8_t                :8;
            // Byte 13
            uint8_t                :8;
            // Byte 14
            uint8_t                :8;  // Sum
        };
    };
} Mirage120Protocol;


// Constants
const uint8_t kMirageAcHeat =    0b001;  // 1
const uint8_t kMirageAcCool =    0b010;  // 2
const uint8_t kMirageAcDry =     0b011;  // 3
const uint8_t kMirageAcRecycle = 0b100;  // 4
const uint8_t kMirageAcFan =     0b101;  // 5

const uint8_t kMirageAcFanAuto = 0b00;  // 0
const uint8_t kMirageAcFanHigh = 0b01;  // 1
const uint8_t kMirageAcFanMed =  0b10;  // 2
const uint8_t kMirageAcFanLow =  0b11;  // 3
const uint8_t kMirageAcKKG29AC1FanAuto = 0b00;  // 0
const uint8_t kMirageAcKKG29AC1FanHigh = 0b01;  // 1
const uint8_t kMirageAcKKG29AC1FanLow =  0b10;  // 2
const uint8_t kMirageAcKKG29AC1FanMed =  0b11;  // 3

const uint8_t kMirageAcMinTemp = 16;  // 16C
const uint8_t kMirageAcMaxTemp = 32;  // 32C
const uint8_t kMirageAcTempOffset = 0x5C;
const uint8_t kMirageAcSensorTempOffset = 20;
const uint8_t kMirageAcSensorTempMax = 43;  // Celsius

const uint8_t kMirageAcPowerOff = 0x5F;
const uint8_t kMirageAcSwingVOff =      0b0000;  // 0
const uint8_t kMirageAcSwingVLowest =   0b0011;  // 3
const uint8_t kMirageAcSwingVLow =      0b0101;  // 5
const uint8_t kMirageAcSwingVMiddle =   0b0111;  // 7
const uint8_t kMirageAcSwingVHigh =     0b1001;  // 9
const uint8_t kMirageAcSwingVHighest =  0b1011;  // 11
const uint8_t kMirageAcSwingVAuto =     0b1101;  // 13



#endif