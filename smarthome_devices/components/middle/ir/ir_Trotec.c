#include "ir_Trotec.h"
#include "system_typedef.h"
#include "string.h"
#include "IRutils.h"


// Constants
const uint16_t kTrotecHdrMark = 5952;
const uint16_t kTrotecHdrSpace = 7364;
const uint16_t kTrotecBitMark = 592;
const uint16_t kTrotecOneSpace = 1560;
const uint16_t kTrotecZeroSpace = 592;
const uint16_t kTrotecGap = 6184;
const uint16_t kTrotecGapEnd = 1500;  // made up value

const uint16_t kTrotec3550HdrMark = 12000;
const uint16_t kTrotec3550HdrSpace = 5130;
const uint16_t kTrotec3550BitMark = 550;
const uint16_t kTrotec3550OneSpace = 1950;
const uint16_t kTrotec3550ZeroSpace = 500;

const uint16_t kTrotec3550TimerMax = 8 * 60;  ///< 8 hours in Minutes.


static uint8_t IRTrotecESP_calcChecksum(const uint8_t state[],
                                  const uint16_t length) {
  return sumBytesState(state + 2, length - 3, 0);
}

static bool IRTrotecESP_validChecksum(const uint8_t state[], const uint16_t length) {
  return state[length - 1] == IRTrotecESP_calcChecksum(state, length);
}

bool decodeTrotec(decode_results *results, uint16_t offset,
                          const uint16_t nbits, const bool strict) {
  if (results->rawlen < 2 * nbits + kHeader + 2 * kFooter - 1 + offset)
  {
    os_trace("%s rawlen: %u, other: %d\r\n", __func__, results->rawlen, (2 * nbits + kHeader + 2 * kFooter - 1 + offset));
    return false;  // Can't possibly be a valid Trotec A/C message.
  }
  if (strict && nbits != kTrotecBits) return false;

  uint16_t used;
  // Header + Data + Footer #1
  used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                      results->rawlen - offset, nbits,
                      kTrotecHdrMark-1500, kTrotecHdrMark+1500, kTrotecHdrSpace-1800, kTrotecHdrSpace+1800,
                      kTrotecBitMark-400, kTrotecBitMark+400, kTrotecOneSpace-500, kTrotecOneSpace+500,
                      kTrotecBitMark-400, kTrotecBitMark+400, kTrotecZeroSpace-400, kTrotecZeroSpace+400,
                      kTrotecBitMark-400, kTrotecBitMark+400, kTrotecGap-1500, kTrotecGap+1500, true,
                      _tolerance, 0, false);
  if (used == 0){
    return false;
  }
  offset += used;

  // Footer #2
  if (!matchLimit(results->rawbuf[offset++], kTrotecBitMark-400, kTrotecBitMark+400)){
    return false;
  }
  if (offset <= results->rawlen &&
      !matchAtLeastLimit(results->rawbuf[offset++], kTrotecGapEnd-375))
      {
        return false;
      }
  // Compliance
  // Verify we got a valid checksum.
  if (strict && !IRTrotecESP_validChecksum(results->state, kTrotecStateLength)){
    os_trace("%s invalid checksum\r\n", __func__);
    return false;
  }
  // Success
  os_trace("%s success\r\n", __func__);
  results->decode_type = TROTEC;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

void IRTrotec_setRaw(const uint8_t state[], state_ir_t *ir_state)
{
    TrotecProtocol _;
    memset(&_, 0, sizeof(TrotecProtocol));
    memcpy(_.raw, state, kTrotecStateLength);

    ir_state->power = _.Power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp + kTrotecMinTemp;
}

static uint8_t IRTrotec3550_calcChecksum(const uint8_t state[],
                                  const uint16_t length) {
  return length ? sumBytesState(state, length - 1, 0) : 0;
}

static bool IRTrotec3550_validChecksum(const uint8_t state[], const uint16_t length) {
  return state[length - 1] == IRTrotec3550_calcChecksum(state, length);
}


bool decodeTrotec3550(decode_results *results, uint16_t offset,
                              const uint16_t nbits, const bool strict) {
  if (strict && nbits != kTrotecBits) return false;

  // Header + Data + Footer
  if (!matchGenericStateLimit(results->rawbuf + offset, results->state,
                    results->rawlen - offset, nbits,
                    kTrotec3550HdrMark-3000, kTrotec3550HdrMark+3000, kTrotec3550HdrSpace-1280, kTrotec3550HdrSpace+1280,
                    kTrotec3550BitMark-400, kTrotec3550BitMark+400, kTrotec3550OneSpace-500, kTrotec3550OneSpace+500,
                    kTrotec3550BitMark-400, kTrotec3550BitMark+400, kTrotec3550ZeroSpace-350, kTrotec3550ZeroSpace+350,
                    kTrotec3550BitMark-400, kTrotec3550BitMark+400, kDefaultMessageGap-25000, kDefaultMessageGap+25000, false, kUseDefTol, kMarkExcess, true)) return false;
  // Compliance
  if (strict && !IRTrotec3550_validChecksum(results->state, nbits / 8))
  {
    os_trace("%s invalid checksum\r\n", __func__);
    return false;
  }
  // Success
  os_trace("%s success\r\n", __func__);
  results->decode_type = TROTEC_3550;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

void IRTrotec3550_setRaw(const uint8_t state[], state_ir_t *ir_state) {
    Trotec3550Protocol _;
    memset(&_, 0, sizeof(Trotec3550Protocol));
    memcpy(_.raw, state, kTrotecStateLength);

    ir_state->power = _.Power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->celsius = _.Celsius;
    ir_state->degrees = _.Celsius ? _.TempC + kTrotec3550MinTempC
                       : _.TempF + kTrotec3550MinTempF;
    ir_state->swingv = _.SwingV;

}



