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
    ir_state->swingv = _.SwingV5 | (_.SwingV7 << kMitsubishiHeavy88SwingVByte5Size);
    ir_state->swingh = _.SwingH1 | (_.SwingH2 << kMitsubishiHeavy88SwingHSize);
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
  ir_state->swingv = _.SwingV;
  ir_state->swingh = _.SwingH;
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

const uint16_t kMitsubishiHeavy160HdrMark = 6000;
const uint16_t kMitsubishiHeavy160HdrSpace = 7500;
const uint16_t kMitsubishiHeavy160BitMark = 550;
const uint16_t kMitsubishiHeavy160OneSpace = 1500;
const uint16_t kMitsubishiHeavy160ZeroSpace = 3450;
const uint32_t kMitsubishiHeavy160Gap = 7500;  // Just a guess.

static void sendMitsubishiHeavy160(const unsigned char data[],
                                   const uint16_t nbytes,
                                   const uint16_t repeat) {
  if (nbytes < kMitsubishiHeavy160StateLength)
    return;  // Not enough bytes to send a proper message.
  vTaskEnterCritical();
  sendGenericBytes(kMitsubishiHeavy160HdrMark, kMitsubishiHeavy160HdrSpace,
              kMitsubishiHeavy160BitMark, kMitsubishiHeavy160OneSpace,
              kMitsubishiHeavy160BitMark, kMitsubishiHeavy160ZeroSpace,
              kMitsubishiHeavy160BitMark, kMitsubishiHeavy160Gap,
              data, nbytes, 38000, false, repeat, kDutyDefault);

  mark(kMitsubishiHeavy160BitMark);
  vTaskExitCritical();
}

static bool IRMitsubishiHeavy160Ac_validChecksum(const uint8_t *state,
                                           const uint16_t length) {

  for (uint8_t i = 0; i < 4; i++) {
    if (state[i] != (uint8_t)(~state[i+4])) {
      os_trace("%s failed i = %u\r\n", __func__, i);
      return false;
    }
  }

  for (uint8_t i = 8; i < 12; i++) {
    if (state[i] != (uint8_t)(~state[i+4])) {
      os_trace("%s failed i = %u, val= %u %u\r\n", __func__, i, (uint8_t)(~state[i]), (uint8_t)(~state[i+4]));
      return false;
    }
  }
  return true;
}

bool decodeMitsubishiHeavy160(decode_results* results, uint16_t offset,
                                   const uint16_t nbits, const bool strict) {
  
  uint16_t used;

  used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                      results->rawlen - offset, nbits,
                      kMitsubishiHeavy160HdrMark - 1500, kMitsubishiHeavy160HdrMark+1500, kMitsubishiHeavy160HdrSpace-1800,kMitsubishiHeavy160HdrSpace+1800,
                      kMitsubishiHeavy160BitMark-400,kMitsubishiHeavy160BitMark+400, kMitsubishiHeavy160OneSpace-500,kMitsubishiHeavy160OneSpace+500,
                      kMitsubishiHeavy160BitMark-400,kMitsubishiHeavy160BitMark+400, kMitsubishiHeavy160ZeroSpace-800, kMitsubishiHeavy160ZeroSpace+800,
                      kMitsubishiHeavy160BitMark-400,kMitsubishiHeavy160BitMark+400, kMitsubishiHeavy160Gap-900, kMitsubishiHeavy160Gap+900, true,
                      _tolerance, 0, false);
  if (used == 0) {
    return false;
  }
  offset += used;

  if (strict) {
    if (!IRMitsubishiHeavy160Ac_validChecksum(results->state, kMitsubishiHeavy160StateLength)) {
      return false;
    }
  }

  os_trace("%s success, decode_type: %d\r\n", __func__, results->decode_type);

  // Success
  results->bits = nbits;
  results->decode_type = MITSUBISHI_HEAVY_160;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}


void MitsubishiHeavy160_setInfo(const uint8_t state[], state_ir_t *ir_state)
{
  Mitsubishi160Protocol _;
  memset(&_, 0, sizeof(kMitsubishiHeavy160StateLength));
  memcpy(_.raw, state, kMitsubishiHeavy160StateLength);

  ir_state->protocol = MITSUBISHI_HEAVY_160;
  ir_state->power = _.Power;
  ir_state->mode = _.Mode;
  ir_state->fanspeed = _.Fan;
  ir_state->degrees = _.Temp + 16;
  ir_state->swingv = _.SwingV + _.Sw_auto;
}

#define kMitsubishiHeavy160AcMinTemp  18
#define kMitsubishiHeavy160AcMaxTemp  30

#define kitsubishiHeavy160ModeCool  0x02
#define kitsubishiHeavy160ModeHeat  0x04
#define kitsubishiHeavy160ModeDry  0x01
#define kitsubishiHeavy160ModeFan  0x03
#define kitsubishiHeavy160ModeAuto  0x00

void MitsubishiHeavy160_send(state_ir_t state_ir)
{
  Mitsubishi160Protocol _;
  memset(&_, 0, sizeof(kMitsubishiHeavy160StateLength));
  const uint8_t kReset[kMitsubishiHeavy160StateLength] = {0xF2, 0xBF, 0x5B, 0xCF, 0x0D, 0x40, 0xA4, 0x30, 0xFB, 0xFE, 0xEB, 0xFF, 0x04, 0x01, 0x14, 0x00, 0xFD, 0x02, 0xFF, 0xFF};
  memcpy(_.raw, kReset, kMitsubishiHeavy160StateLength);

  // power
  _.Power = state_ir.power;

  // temp
  uint8_t degrees = max((uint8_t)state_ir.degrees, kMitsubishiHeavy160AcMinTemp);
  degrees = min(degrees, kMitsubishiHeavy160AcMaxTemp);
  _.Temp = degrees - 16;

  // mode
  switch ((uint8_t)state_ir.mode) {
    case kitsubishiHeavy160ModeCool:
    case kitsubishiHeavy160ModeHeat:
    case kitsubishiHeavy160ModeDry:
    case kitsubishiHeavy160ModeFan:
    case kitsubishiHeavy160ModeAuto:
      _.Mode = state_ir.mode;
      break;
    default:
      _.Mode = kitsubishiHeavy160ModeCool;
  }

  // fan: auto: 3
  if ((uint8_t)state_ir.fanspeed > 7 || (uint8_t)state_ir.fanspeed < 3)
    _.Fan = 3;
  else
    _.Fan = state_ir.fanspeed;

  // SwingV auto: 3, highest: 0, high: 1, mid: 2, lowest: 3
  if (state_ir.swingv != -1) {
    if ((uint8_t)state_ir.swingv > 3) {
      _.SwingV = 3;
      _.Sw_auto = 1;
    } else {
      _.SwingV = state_ir.swingv;
      _.Sw_auto = 0;
    }
  }

  for (uint8_t i = 0; i < 4; i++) {
    _.raw[i] = (uint8_t)(~_.raw[i+4]);
  }

  for (uint8_t i = 8; i < 12; i++) {
    _.raw[i+4] = (uint8_t)(~_.raw[i]);
  }

  #if DEBUG_ENABLE == 1
    os_trace("MITSUBISHI_HEAVY_send: 0x ");
    for(uint8_t i = 0; i < kMitsubishiHeavy160StateLength; i++) {
        os_trace("%02X-", _.raw[i]);
    }
    os_trace("\r\n");
  #endif

  sendMitsubishiHeavy160(_.raw, kMitsubishiHeavy160StateLength, kMitsubishiHeavy160MinRepeat);
}

