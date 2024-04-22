#include "ir_Nagakawa.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"
#include "ir_Mitsubishi.h"
#include "ir_Tcl.h"

static void sendNagakawa(const unsigned char data[],
                         const uint16_t nbytes,
                         const uint16_t repeat)
{
  if (nbytes < kNagakawaStateLength)
    return; // Not enough bytes to send a proper message.
  vTaskEnterCritical();
  sendGenericBytes(kNagakawaHdrMark, kNagakawaHdrSpace,
                   kNagakawaBitMark, kNagakawaOneSpace,
                   kNagakawaBitMark, kNagakawaZeroSpace,
                   kNagakawaBitMark, kNagakawaGap,
                   data, nbytes, 38, false, repeat, 50);

  vTaskExitCritical();
}

static void NagakawaStateReset(Mitsubishi112Protocol *_)
{
  const uint8_t kReset[kMitsubishi112StateLength] = {0x23, 0xCB, 0x26, 0x01, 0x00, 0x24, 0x03, 0x07, 0x30, 0x00, 0x00, 0x00, 0x00, 0x03};
  memcpy(_->raw, kReset, kMitsubishi112StateLength);
}

static opmode_t NagakawatoCommonMode(const uint8_t mode)
{
  switch (mode)
  {
  case kNagakawaCool:
    return opmode_kCool;
  case kNagakawaHeat:
    return opmode_kHeat;
  case kNagakawaDry:
    return opmode_kDry;
  case kNagakawaFan:
    return opmode_kFan;
  default:
    return opmode_kAuto;
  }
}

static fanspeed_t NagakawatoCommonFanSpeed(const uint8_t speed)
{
  switch (speed)
  {
  case kNagakawaFanMax:
    return fanspeed_kMax;
  case kNagakawaFanMed:
    return fanspeed_kMedium;
  case kNagakawaFanMin:
    return fanspeed_kMin;
  default:
    return fanspeed_kAuto;
  }
}

void Nagakawa_setRaw(const uint8_t *newState, const decode_type_t protocol, state_ir_t *ir_state)
{
  Mitsubishi112Protocol _;

  memcpy(_.raw, newState, kMitsubishi112StateLength);

  ir_state->protocol = NAGAKAWA;
  // We can only detect the difference between models when the power is on.

  ir_state->power = _.Power;

  ir_state->mode = NagakawatoCommonMode(_.Mode);
  // os_trace("%s %u\r\n", __func__, _.SwingV);
  // os_trace("%s temp: %u\r\n", __func__, _.Temp);
  // os_trace("%s fan: %u\r\n", __func__, _.Fan);
  ir_state->degrees = kMitsubishiAcMaxTemp - _.Temp;
  ir_state->fanspeed = NagakawatoCommonFanSpeed(_.Fan);

  // Toshiba_toCommon(_, ir_state);
}

void Nagakawa_setInfo(const uint8_t *newState, const decode_type_t protocol, state_ir_t *ir_state)
{
  Mitsubishi112Protocol _;

  memcpy(_.raw, newState, kMitsubishi112StateLength);

  ir_state->protocol = NAGAKAWA;
  // We can only detect the difference between models when the power is on.

  ir_state->power = _.Power;

  ir_state->mode = _.Mode;
  // os_trace("%s %u\r\n", __func__, _.SwingV);
  // os_trace("%s temp: %u\r\n", __func__, _.Temp);
  // os_trace("%s fan: %u\r\n", __func__, _.Fan);
  ir_state->degrees = kMitsubishiAcMaxTemp - _.Temp;
  ir_state->fanspeed = _.Fan;

  // Toshiba_toCommon(_, ir_state);
}

static void NagakawasetTemp(NagakawaProtocol *_, const float celsius)
{
  // Make sure we have desired temp in the correct range.
  float safecelsius = max(celsius, kNagakawaTempMin);
  safecelsius = min(safecelsius, kNagakawaTempMax);
  // Convert to integer nr. of half degrees.
  uint8_t nrHalfDegrees = safecelsius * 2;
  // Do we have a half degree celsius?
  _->HalfDegree = nrHalfDegrees & 1;
  _->Temp = (uint8_t)(kTcl112AcTempMax - nrHalfDegrees / 2);
}

void Nagakawa_send(state_ir_t state_ir)
{
  os_trace("%s\r\n", __func__);
  NagakawaProtocol _;
  NagakawaStateReset(&_);
  _.SwingV = kNagakawaSwingVOn; // swingv auto

  if (state_ir.swingv != swingv_kOff) {
    _.SwingV = state_ir.swingv;
  }

  _.Mode = (uint8_t)state_ir.mode;

  _.Fan = (uint8_t)state_ir.fanspeed;

  NagakawasetTemp(&_, state_ir.degrees);

  _.Power = state_ir.power;

  IRMitsubishi112checksum(&_);

  // os_trace("NAGAKAWA_SEND: 0x");
  // for(uint8_t i = 0; i < 14; i++) {
  //     os_trace("%02X-", _.raw[i]);
  // }
  // os_trace("\r\n");

  sendNagakawa(_.raw, kNagakawaStateLength, kNoRepeat);
}