#include "ir_MitsubishiHeavy.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"

// Constants
const uint16_t kMitsubishiHeavyHdrMark = 3140;
const uint16_t kMitsubishiHeavyHdrSpace = 1630;
const uint16_t kMitsubishiHeavyBitMark = 370;
const uint16_t kMitsubishiHeavyOneSpace = 420;
const uint16_t kMitsubishiHeavyZeroSpace = 1220;
const uint32_t kMitsubishiHeavyGap = kDefaultMessageGap;  // Just a guess.

static bool IRMitsubishiHeavy88Ac_checkZjsSig(const uint8_t *state) {
  for (uint8_t i = 0; i < kMitsubishiHeavySigLength; i++)
    if (state[i] != kMitsubishiHeavyZjsSig[i]) return false;
  return true;
}

static bool IRMitsubishiHeavy152Ac_checkZmsSig(const uint8_t *state) {
  for (uint8_t i = 0; i < kMitsubishiHeavySigLength; i++)
    if (state[i] != kMitsubishiHeavyZmsSig[i]) return false;
  return true;
}

static bool IRMitsubishiHeavy152Ac_validChecksum(const uint8_t *state,
                                           const uint16_t length) {
  // Assume anything too short is fine.
  if (length < kMitsubishiHeavySigLength) return true;
  const uint8_t kOffset = kMitsubishiHeavySigLength - 2;
  return checkInvertedBytePairs(state + kOffset, length - kOffset);
}

bool decodeMitsubishiHeavy(decode_results* results, uint16_t offset,
                                   const uint16_t nbits, const bool strict) {
  if (strict) {
    switch (nbits) {
      case kMitsubishiHeavy88Bits:
      case kMitsubishiHeavy152Bits:
        break;
      default:
        return false;  // Not what is expected
    }
  }

  uint16_t used;

  used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                      results->rawlen - offset, nbits,
                      kMitsubishiHeavyHdrMark - 400, kMitsubishiHeavyHdrMark+400, kMitsubishiHeavyHdrSpace-400,kMitsubishiHeavyHdrSpace+400,
                      kMitsubishiHeavyBitMark-270,kMitsubishiHeavyBitMark+330, kMitsubishiHeavyOneSpace-320,kMitsubishiHeavyOneSpace+380,
                      kMitsubishiHeavyBitMark-270,kMitsubishiHeavyBitMark+330, kMitsubishiHeavyZeroSpace-320, kMitsubishiHeavyZeroSpace+380,
                      kMitsubishiHeavyBitMark-270,kMitsubishiHeavyBitMark+330, kMitsubishiHeavyGap-25000, kMitsubishiHeavyGap+25000, true,
                      _tolerance, 0, false);
  if (used == 0) return false;
  offset += used;
  // Compliance
  switch (nbits) {
    case kMitsubishiHeavy88Bits:
      if (strict && !(IRMitsubishiHeavy88Ac_checkZjsSig(results->state) &&
                      IRMitsubishiHeavy152Ac_validChecksum(results->state, kMitsubishiHeavy88StateLength)))
                      {
                        os_trace("%s checksum 88 failed\r\n", __func__);
        return false;
                      }
      results->decode_type = MITSUBISHI_HEAVY_88;
      break;
    case kMitsubishiHeavy152Bits:
      if (strict && !(IRMitsubishiHeavy152Ac_checkZmsSig(results->state) &&
                      IRMitsubishiHeavy152Ac_validChecksum(results->state, kMitsubishiHeavy152StateLength)))
        {
          os_trace("%s checksum 152 failed\r\n", __func__);
        return false;
        }
      results->decode_type = MITSUBISHI_HEAVY_152;
      break;
    default:
      return false;
  }

  os_trace("%s success, decode_type: %d\r\n", __func__, results->decode_type);

  // Success
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

void IRMitsubishiHeavy88Ac_setInfo(const uint8_t *data, state_ir_t *ir_state)
{
    Mitsubishi88Protocol _;
    memset(&_, 0, sizeof(Mitsubishi88Protocol));
    memcpy(_.raw, data, kMitsubishiHeavy88StateLength);

    ir_state->power = _.Power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp + kMitsubishiHeavyMinTemp;
}

static void IRMitsubishiHeavy88Ac_checksum(Mitsubishi88Protocol *_) {
  const uint8_t kOffset = kMitsubishiHeavySigLength - 2;
  invertBytePairs(_->raw + kOffset, kMitsubishiHeavy88StateLength - kOffset);
}

static void sendMitsubishiHeavy88(const unsigned char data[],
                                   const uint16_t nbytes,
                                   const uint16_t repeat) {
  if (nbytes < kMitsubishiHeavy88StateLength)
    return;  // Not enough bytes to send a proper message.
    vTaskEnterCritical();
  sendGenericBytes(kMitsubishiHeavyHdrMark, kMitsubishiHeavyHdrSpace,
              kMitsubishiHeavyBitMark, kMitsubishiHeavyOneSpace,
              kMitsubishiHeavyBitMark, kMitsubishiHeavyZeroSpace,
              kMitsubishiHeavyBitMark, kMitsubishiHeavyGap,
              data, nbytes, 38000, false, repeat, kDutyDefault);
  vTaskExitCritical();
}

void MitsubishiHeavy88Ac_send(state_ir_t state_ir)
{
  Mitsubishi88Protocol _;

  memcpy(_.raw, kMitsubishiHeavyZjsSig, kMitsubishiHeavySigLength);
  for (uint8_t i = kMitsubishiHeavySigLength; i < kMitsubishiHeavy88StateLength;
       i++) _.raw[i] = 0;

  _.Power = state_ir.power;

  switch ((uint8_t)state_ir.mode) {
    case kMitsubishiHeavyCool:
    case kMitsubishiHeavyDry:
    case kMitsubishiHeavyFan:
    case kMitsubishiHeavyHeat:
    _.Mode = state_ir.mode;
      break;
    default:
      _.Mode = kMitsubishiHeavyAuto;
  }

  switch ((uint8_t)state_ir.fanspeed) {
    case kMitsubishiHeavy88FanLow:
    case kMitsubishiHeavy88FanMed:
    case kMitsubishiHeavy88FanHigh:
    case kMitsubishiHeavy88FanTurbo:
    case kMitsubishiHeavy88FanEcono: 
        _.Fan = state_ir.fanspeed;
    break;
    default: _.Fan = kMitsubishiHeavy88FanAuto;
  }

  uint8_t newtemp = state_ir.degrees;
  newtemp = min((uint8_t)newtemp, kMitsubishiHeavyMaxTemp);
  newtemp = max(newtemp, kMitsubishiHeavyMinTemp);
  _.Temp = newtemp - kMitsubishiHeavyMinTemp;

  IRMitsubishiHeavy88Ac_checksum(&_);
  sendMitsubishiHeavy88(_.raw, kMitsubishiHeavy88StateLength,
                                kMitsubishiHeavy88MinRepeat);
}

void IRMitsubishiHeavy152Ac_setInfo(const uint8_t *data, state_ir_t *ir_state) {
  Mitsubishi152Protocol _;
  memset(&_, 0, sizeof(Mitsubishi152Protocol));
  memcpy(_.raw, data, kMitsubishiHeavy152StateLength);

  ir_state->power = _.Power;
  ir_state->mode = _.Mode;
  ir_state->fanspeed = _.Fan;
  ir_state->degrees = _.Temp + kMitsubishiHeavyMinTemp;
}

static void IRMitsubishiHeavy152Ac_checksum(Mitsubishi152Protocol *_) {
  const uint8_t kOffset = kMitsubishiHeavySigLength - 2;
  invertBytePairs(_->raw + kOffset, kMitsubishiHeavy152StateLength - kOffset);
}

static void sendMitsubishiHeavy152(const unsigned char data[],
                                    const uint16_t nbytes,
                                    const uint16_t repeat) {
  if (nbytes < kMitsubishiHeavy152StateLength)
    return;  // Not enough bytes to send a proper message.
  sendMitsubishiHeavy88(data, nbytes, repeat);
}

void MitsubishiHeavy152Ac_send(state_ir_t state_ir)
{
  Mitsubishi152Protocol _;
  memset(&_, 0, sizeof(Mitsubishi152Protocol));
  memcpy(_.raw, kMitsubishiHeavyZmsSig, kMitsubishiHeavySigLength);
  for (uint8_t i = kMitsubishiHeavySigLength;
       i < kMitsubishiHeavy152StateLength - 3; i += 2) _.raw[i] = 0;
  _.raw[17] = 0x80;

  _.Power = state_ir.power;

  switch ((uint8_t)state_ir.mode) {
    case kMitsubishiHeavyCool:
    case kMitsubishiHeavyDry:
    case kMitsubishiHeavyFan:
    case kMitsubishiHeavyHeat:
      _.Mode = state_ir.mode;
      break;
    default:
      _.Mode = kMitsubishiHeavyAuto;
  }

  switch ((uint8_t)state_ir.fanspeed) {
    case kMitsubishiHeavy152FanLow:
    case kMitsubishiHeavy152FanMed:
    case kMitsubishiHeavy152FanHigh:
    case kMitsubishiHeavy152FanMax:
    case kMitsubishiHeavy152FanEcono:
    case kMitsubishiHeavy152FanTurbo: 
      _.Fan = state_ir.fanspeed;
    break;
    default: _.Fan = kMitsubishiHeavy152FanAuto;
  }

  uint8_t newtemp = (uint8_t)state_ir.degrees;
  newtemp = min(newtemp, kMitsubishiHeavyMaxTemp);
  newtemp = max(newtemp, kMitsubishiHeavyMinTemp);
  _.Temp = newtemp - kMitsubishiHeavyMinTemp;

  IRMitsubishiHeavy152Ac_checksum(&_);
  sendMitsubishiHeavy152(_.raw, kMitsubishiHeavy152StateLength,
                                 kMitsubishiHeavy152MinRepeat);
}
