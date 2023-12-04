#ifndef IRSEND_H_
#define IRSEND_H_

#include <stdint.h>
#include <stdbool.h>
#include "IRremoteWB2.h"

#define kNoTempValue -100.0

typedef enum {
  GE6711AR2853M = 1,  // (1) LG 28-bit Protocol (default)
  AKB75215403,        // (2) LG2 28-bit Protocol
  AKB74955603,        // (3) LG2 28-bit Protocol variant
  AKB73757604,        // (4) LG2 Variant of AKB74955603
  LG6711A20083V,      // (5) Same as GE6711AR2853M, but only SwingV toggle.
} lg_ac_remote_model_t;

typedef enum {
  kControlCommand = 0,
  kSensorTempReport = 1,
  kTimerCommand = 2,
  kConfigCommand = 3,
  // Add new entries before this one, and update it to point to the last entry
  kLastAcCommandEnum = kConfigCommand,
} ac_command_t;

typedef enum {
  opmode_kOff  = -1,
  opmode_kAuto =  0,
  opmode_kCool =  1,
  opmode_kHeat =  2,
  opmode_kDry  =  3,
  opmode_kFan  =  4,
  // Add new entries before this one, and update it to point to the last entry
  kLastOpmodeEnum = opmode_kFan,
} opmode_t;

typedef enum {
  fanspeed_kAuto =       0,
  fanspeed_kMin =        1,
  fanspeed_kLow =        2,
  fanspeed_kMedium =     3,
  fanspeed_kHigh =       4,
  fanspeed_kMax =        5,
  fanspeed_kMediumHigh = 6,
  // Add new entries before this one, and update it to point to the last entry
  kLastFanspeedEnum = fanspeed_kMediumHigh,
} fanspeed_t;

typedef enum {
  swingh_kOff =     -1,
  swingh_kAuto =     0,  // a.k.a. On.
  swingh_kLeftMax =  1,
  swingh_kLeft =     2,
  swingh_kMiddle =   3,
  swingh_kRight =    4,
  swingh_kRightMax = 5,
  swingh_kWide =     6,  // a.k.a. left & right at the same time.
  // Add new entries before this one, and update it to point to the last entry
  kLastSwinghEnum = swingh_kWide,
} swingh_t;

typedef enum {
  swingv_kOff =    -1,
  swingv_kAuto =    0,
  swingv_kHighest = 1,
  swingv_kHigh =    2,
  swingv_kMiddle =  3,
  swingv_kLow =     4,
  swingv_kLowest =  5,
  swingv_kUpperMiddle = 6,
  // Add new entries before this one, and update it to point to the last entry
  kLastSwingvEnum = swingv_kUpperMiddle,
} swingv_t;

typedef struct {
  decode_type_t protocol;// = decode_type_t::UNKNOWN;
  int16_t model;// = -1;  // `-1` means unused.
  bool power;// = false;
  opmode_t mode;// = opmode_t::kOff;
  float degrees;// = 25;
  bool celsius;// = true;
  fanspeed_t fanspeed;// = fanspeed_t::kAuto;
  swingv_t swingv;// = swingv_t::kOff;
  swingh_t swingh;// = swingh_t::kOff;
  bool quiet;// = false;
  bool turbo;// = false;
  bool econo;// = false;
  bool light;// = false;
  bool filter;// = false;
  bool clean;// = false;
  bool beep;// = false;
  int16_t sleep;// = -1;  // `-1` means off.
  int16_t clock;// = -1;  // `-1` means not set.
  ac_command_t command;// = ;
  bool iFeel;// = false;
  float sensorTemperature;// = kNoTempValue;  // `kNoTempValue` means not set.
} state_ir_t; // = {UNKNOWN, -1, false, opmode_kOff, 25, true, fanspeed_kAuto, swingv_kOff, swingh_kOff, false, false, false, false, false, false, false, -1, -1, kControlCommand, false, kNoTempValue};

// typedef struct state_ss state_ir_t;

void IR_logCommon(state_ir_t ir_state);

#endif