#include "ir_Whirlpool.h"
#include "system_typedef.h"
#include "string.h"
#include "IRutils.h"


// Constants
const uint16_t kWhirlpoolAcHdrMark = 8950;
const uint16_t kWhirlpoolAcHdrSpace = 4484;
const uint16_t kWhirlpoolAcBitMark = 597;
const uint16_t kWhirlpoolAcOneSpace = 1649;
const uint16_t kWhirlpoolAcZeroSpace = 533;
const uint16_t kWhirlpoolAcGap = 7920;
const uint32_t kWhirlpoolAcMinGap = kDefaultMessageGap;  // Just a guess.
const uint8_t kWhirlpoolAcSections = 3;


static bool IRWhirlpoolAc_validChecksum(const uint8_t state[],
                                  const uint16_t length) {
  if (length > kWhirlpoolAcChecksumByte1 &&
      state[kWhirlpoolAcChecksumByte1] !=
          xorBytes(state + 2, kWhirlpoolAcChecksumByte1 - 1 - 2, 0)) {
    os_trace("%s DEBUG: First Whirlpool AC checksum failed.\r\n", __func__);
    return false;
  }
  if (length > kWhirlpoolAcChecksumByte2 &&
      state[kWhirlpoolAcChecksumByte2] !=
          xorBytes(state + kWhirlpoolAcChecksumByte1 + 1,
                   kWhirlpoolAcChecksumByte2 - kWhirlpoolAcChecksumByte1 - 1, 0)) {
    os_trace("%s DEBUG: Second Whirlpool AC checksum failed.\r\n", __func__);
    return false;
  }
  // State is too short to have a checksum or everything checked out.
  return true;
}

bool decodeWhirlpoolAC(decode_results *results, uint16_t offset,
                               const uint16_t nbits, const bool strict) {
  if (results->rawlen < 2 * nbits + 4 + kHeader + kFooter - 1 + offset)
    return false;  // Can't possibly be a valid Whirlpool A/C message.
  if (strict) {
    if (nbits != kWhirlpoolAcBits) return false;
  }

  uint8_t sectionSize[] = {6, 8, 7};

  // Header
  if (!matchLimit(results->rawbuf[offset++], kWhirlpoolAcHdrMark-2250, kWhirlpoolAcHdrMark+2250)) return false;
  if (!matchLimit(results->rawbuf[offset++], kWhirlpoolAcHdrSpace-1125, kWhirlpoolAcHdrSpace+1125))
    return false;

  // Data Sections
  uint16_t pos = 0;
  for (uint8_t section = 0; section < kWhirlpoolAcSections;
       section++) {
    uint16_t used;
    // Section Data
    used = matchGenericStateLimit(results->rawbuf + offset, results->state + pos,
                        results->rawlen - offset, sectionSize[section] * 8,
                        0, 0, 0, 0,
                        kWhirlpoolAcBitMark-450, kWhirlpoolAcBitMark+450, kWhirlpoolAcOneSpace-500, kWhirlpoolAcOneSpace+500,
                        kWhirlpoolAcBitMark-450, kWhirlpoolAcBitMark+450, kWhirlpoolAcZeroSpace- 433, kWhirlpoolAcZeroSpace+437,
                        kWhirlpoolAcBitMark-450, kWhirlpoolAcBitMark+450, kWhirlpoolAcGap-2000, kWhirlpoolAcGap+2000,
                        section >= kWhirlpoolAcSections - 1,
                        _tolerance, kMarkExcess, false);
    if (used == 0) return false;
    offset += used;
    pos += sectionSize[section];
  }

  // Compliance
  if (strict) {
    // Re-check we got the correct size/length due to the way we read the data.
    if (pos * 8 != nbits) return false;
    if (!IRWhirlpoolAc_validChecksum(results->state, nbits / 8)){
        os_trace("%s invalid checksum\r\n", __func__);
        return false;
    }
  }

    WhirlpoolProtocol _;
    memset(&_, 0, sizeof(WhirlpoolProtocol));
    memcpy(_.raw, results->state, kWhirlpoolAcStateLength);

//   state_ir_t ir_state;
//   IRWhirlpoolAc_setRaw(results->state, kWhirlpoolAcStateLength, &ir_state);
    if (_.isCasper && _.J191) {
        os_trace("%s It is Casper AC\r\n");
        return false;
    }

  os_trace("%s success\r\n", __func__);

  // Success
  results->decode_type = WHIRLPOOL_AC;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

static bool pre_power = true;

static int8_t getTempOffset(whirlpool_ac_remote_model_t _model){
  switch (_model) {
    case DG11J191:  return -2;
    default:        return 0;
  }
}

void IRWhirlpoolAc_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state)
{
    WhirlpoolProtocol _;
    memset(&_, 0, sizeof(WhirlpoolProtocol));
    memcpy(_.raw, new_code, min(length, kWhirlpoolAcStateLength));

    if (_.Power) {
        pre_power = !pre_power;
    }

    if (_.J191)
        ir_state->model = DG11J191;
    else
        ir_state->model = DG11J13A;

    ir_state->power = pre_power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp + kWhirlpoolAcMinTemp + getTempOffset(ir_state->model);
    ir_state->swingv = (_.Swing1 && _.Swing2);

}