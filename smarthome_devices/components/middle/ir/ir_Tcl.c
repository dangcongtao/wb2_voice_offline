#include "ir_Tcl.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"
#include "ir_Mitsubishi.h"
#include "ir_Nagakawa.h"

static void Tcl112StateReset(Tcl112Protocol *_)
{
  const uint8_t reset[kTcl112AcStateLength] = {
      0x23, 0xCB, 0x26, 0x01, 0x00, 0x24, 0x03, 0x07, 0x30, 0x00, 0x00, 0x00, 0x00, 0x03};
  memcpy(_->raw, reset, kTcl112AcStateLength);
}

static opmode_t Tcl112toCommonMode(const uint8_t mode)
{
  switch (mode)
  {
  case kTcl112AcCool:
    return opmode_kCool;
  case kTcl112AcHeat:
    return opmode_kHeat;
  case kTcl112AcDry:
    return opmode_kDry;
  case kTcl112AcFan:
    return opmode_kFan;
  default:
    return opmode_kAuto;
  }
}

static fanspeed_t Tcl112toCommonFanSpeed(const uint8_t speed)
{
  switch (speed)
  {
  case kTcl112AcFanHigh:
    return fanspeed_kMax;
  case kTcl112AcFanMed:
    return fanspeed_kMedium;
  case kTcl112AcFanLow:
    return fanspeed_kLow;
  case kTcl112AcFanMin:
    return fanspeed_kMin;
  default:
    return fanspeed_kAuto;
  }
}

static float IRTcl112AcgetTemp(Tcl112Protocol _)
{
  float result = kTcl112AcTempMax - _.Temp;
  // if (_.HalfDegree && !_.isAkito)
  //   result += 0.5;
  return result;
}

void Tcl112_setRaw(const uint8_t *newState, const decode_type_t protocol, state_ir_t *ir_state)
{
  // Tcl112Protocol _;
  Tcl112Protocol _;
  memset(&_, 0, sizeof(Tcl112Protocol));
  memcpy(_.raw, newState, kTcl112AcStateLength);

  ir_state->protocol = MITSUBISHI112;
  // We can only detect the difference between models when the power is on.

  ir_state->power = _.Power;

  ir_state->mode = Tcl112toCommonMode(_.Mode);
  ir_state->degrees = IRTcl112AcgetTemp(_);
  ir_state->fanspeed = Tcl112toCommonFanSpeed(_.Fan);

  // Toshiba_toCommon(_, ir_state);
}
void Tcl112_setInfo(const uint8_t *newState, const decode_type_t protocol, state_ir_t *ir_state)
{
  Tcl112Protocol _;
  memset(&_, 0, sizeof(Tcl112Protocol));

  memcpy(_.raw, newState, kTcl112AcStateLength);
  os_trace("%s halftemp: %u\r\n", __func__, _.HalfDegree);
  if ((_.raw[12] >> 4))
  {
    ir_state->protocol = AKITO;
  }
  else
  {
    ir_state->protocol = TCL112AC;
  }
  // We can only detect the difference between models when the power is on.

  ir_state->power = _.Power;

  ir_state->mode = _.Mode;
  ir_state->degrees = IRTcl112AcgetTemp(_);
  ir_state->fanspeed = _.Fan;
  ir_state->swingv = _.SwingV;
  ir_state->swingh = _.SwingH;

  // Toshiba_toCommon(_, ir_state);
}

static void sendTcl112Ac(const unsigned char data[], const uint16_t nbytes,
                         const uint16_t repeat)
{
  vTaskEnterCritical();
  sendGenericBytes(kTcl112AcHdrMark, kTcl112AcHdrSpace,
                   kTcl112AcBitMark, kTcl112AcOneSpace,
                   kTcl112AcBitMark, kTcl112AcZeroSpace,
                   kTcl112AcBitMark, kTcl112AcGap,
                   data, nbytes, 38000, false, repeat, 50);
  vTaskExitCritical();
}

static void IRTcl112AcsetTemp(Tcl112Protocol *_, const float celsius)
{
  // Make sure we have desired temp in the correct range.
  float safecelsius = max(celsius, kTcl112AcTempMin);
  safecelsius = min(safecelsius, kTcl112AcTempMax);
  // Convert to integer nr. of half degrees.
  uint8_t nrHalfDegrees = safecelsius * 2;
  // Do we have a half degree celsius?
  // if (!_->isAkito)
  // {
  //   _->HalfDegree = nrHalfDegrees & 1;
  // }
  _->Temp = (uint8_t)(kTcl112AcTempMax - nrHalfDegrees / 2);
}

void Tcl112_send(state_ir_t state_ir)
{
  os_trace("%s\r\n", __func__);
  Tcl112Protocol _;
  Tcl112StateReset(&_);
  _.SwingV = kTcl112AcSwingVOn; // swingv auto
  if (state_ir.protocol == AKITO)
  {
    os_trace("%s isAkito AC\r\n", __func__);
    _.HalfDegree = 1;

    if (state_ir.mode == kTcl112AcHeat)
    {
      _.isAkito = 0;
    }
    else
    {
      _.isAkito = 1;
    }
  }

  _.Mode = (uint8_t)state_ir.mode;

  _.Fan = (uint8_t)state_ir.fanspeed;

  IRTcl112AcsetTemp(&_, state_ir.degrees);

  _.Power = state_ir.power;

  if (state_ir.swingv != swingv_kOff)
  {
    _.SwingV = (uint8_t)state_ir.swingv;
  }

  if (state_ir.swingh != swingh_kOff)
  {
    _.SwingH = (uint8_t)state_ir.swingh;
  }

  IRMitsubishi112checksum(&_);
#if DEBUG_ENABLE == 1
  os_trace("%s: 0x ", __func__);
  for (uint8_t i = 0; i < kTcl112AcStateLength; i++)
  {
    os_trace("%02X-", _.raw[i]);
  }
  os_trace("\r\n");
#endif

  sendTcl112Ac(_.raw, kTcl112AcStateLength, kNoRepeat);
}