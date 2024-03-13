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
                      kMitsubishiHeavyHdrMark - 400, kMitsubishiHeavyHdrMark+400, kMitsubishiHeavyHdrSpace+400,kMitsubishiHeavyHdrSpace-400,
                      kMitsubishiHeavyBitMark-270,kMitsubishiHeavyBitMark+330, kMitsubishiHeavyOneSpace-320,kMitsubishiHeavyOneSpace+380,
                      kMitsubishiHeavyBitMark-270,kMitsubishiHeavyBitMark+330, kMitsubishiHeavyZeroSpace-320, kMitsubishiHeavyZeroSpace+380,
                      kMitsubishiHeavyBitMark-270,kMitsubishiHeavyBitMark+330, kMitsubishiHeavyGap-2000, kMitsubishiHeavyGap+2000, true,
                      _tolerance, 0, false);
  if (used == 0) return false;
  offset += used;
  // Compliance
  switch (nbits) {
    case kMitsubishiHeavy88Bits:
      if (strict && !(IRMitsubishiHeavy88Ac_checkZjsSig(results->state) &&
                      IRMitsubishiHeavy152Ac_validChecksum(results->state, kMitsubishiHeavy88StateLength)))
        return false;
      results->decode_type = MITSUBISHI_HEAVY_88;
      break;
    case kMitsubishiHeavy152Bits:
      if (strict && !(IRMitsubishiHeavy152Ac_checkZmsSig(results->state) &&
                      IRMitsubishiHeavy152Ac_validChecksum(results->state, kMitsubishiHeavy152StateLength)))
        return false;
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