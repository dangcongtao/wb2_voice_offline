#include "ir_Neoclima.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"

// Constants
const uint16_t kNeoclimaHdrMark = 6112;
const uint16_t kNeoclimaHdrSpace = 7391;
const uint16_t kNeoclimaBitMark = 537;
const uint16_t kNeoclimaOneSpace = 1651;
const uint16_t kNeoclimaZeroSpace = 571;
const uint32_t kNeoclimaMinGap = kDefaultMessageGap;

static uint8_t calcChecksum(const uint8_t state[],
                                   const uint16_t length) {
  if (length == 0) return state[0];
  return sumBytesState(state, length - 1, 0);
}

static bool IRNeoclimaAc_validChecksum(const uint8_t state[], const uint16_t length) {
  if (length < 2)
    return true;  // No checksum to compare with. Assume okay.
  return (state[length - 1] == calcChecksum(state, length));
}

bool decodeNeoclima(decode_results *results, uint16_t offset,
                            const uint16_t nbits, const bool strict) {
  // Compliance
  if (strict && nbits != kNeoclimaBits)
    return false;  // Incorrect nr. of bits per spec.

  // Match Main Header + Data + Footer
  uint16_t used;
  used = matchGenericStateLimit(results->rawbuf + offset, results->state,
                      results->rawlen - offset, nbits,
                      kNeoclimaHdrMark-1500, kNeoclimaHdrMark+1500, kNeoclimaHdrSpace-1800, kNeoclimaHdrSpace+1800,
                      kNeoclimaBitMark-400, kNeoclimaBitMark+400, kNeoclimaOneSpace-500, kNeoclimaOneSpace+500,
                      kNeoclimaBitMark-400, kNeoclimaBitMark+400, kNeoclimaZeroSpace-400, kNeoclimaZeroSpace+400,
                      kNeoclimaBitMark-400, kNeoclimaBitMark+400, kNeoclimaHdrSpace-1800, kNeoclimaHdrSpace+1800, false,
                      _tolerance, 0, false);
  if (!used) return false;
  offset += used;
  // Extra footer.
  uint64_t unused;
  if (!matchGenericDataLimit(results->rawbuf + offset, &unused,
                    results->rawlen - offset, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                    kNeoclimaBitMark-400, kNeoclimaBitMark+400, kNeoclimaHdrSpace-1800, kNeoclimaHdrSpace+1800, true, kUseDefTol, kMarkExcess, true)) return false;

  // Compliance
  if (strict) {
    // Check we got a valid checksum.
    if (!IRNeoclimaAc_validChecksum(results->state, nbits / 8)) return false;
  }
    os_trace("%s success\r\n", __func__);
  // Success
  results->decode_type = NEOCLIMA;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  return true;
}

void IRNeoclimaAc_setRaw(const uint8_t new_code[], const uint16_t length, state_ir_t *ir_state) {
    NeoclimaProtocol _;
    memset(&_, 0, sizeof(NeoclimaProtocol));
    memcpy(_.raw, new_code, min(length, kNeoclimaStateLength));

    ir_state->power = _.Power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    uint8_t min_temp = (!_.UseFah) ? kNeoclimaMinTempC
                                          : kNeoclimaMinTempF;
    ir_state->degrees = _.Temp + min_temp;
    ir_state->swingv = (_.SwingV == kNeoclimaSwingVOn);
    ir_state->swingh = !_.SwingH;
}