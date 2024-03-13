#include "ir_Airwell.h"
#include "system_typedef.h"
#include "string.h"

const uint8_t kAirwellOverhead = 4;
const uint16_t kAirwellHalfClockPeriod = 950;  // uSeconds
const uint16_t kAirwellHdrMark = 3 * kAirwellHalfClockPeriod;  // uSeconds
const uint16_t kAirwellHdrSpace = 3 * kAirwellHalfClockPeriod;  // uSeconds
const uint16_t kAirwellFooterMark = 5 * kAirwellHalfClockPeriod;  // uSeconds


bool decodeAirwell(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict) {
  if (results->rawlen < nbits + kAirwellOverhead - offset)
    return false;  // Too short a message to match.

  // Compliance
  if (strict && nbits != kAirwellBits)
    return false;  // Doesn't match our protocol defn.

  // Header #1 + Data #1 + Footer #1 (There are total of 3 sections)
  uint16_t used = matchManchester(results->rawbuf + offset, &results->value,
                                  results->rawlen - offset, nbits,
                                  kAirwellHdrMark, kAirwellHdrMark,
                                  kAirwellHalfClockPeriod,
                                  kAirwellHdrMark, kAirwellHdrSpace,
                                  true, kUseDefTol, kMarkExcess, true, false);
  if (used == 0) return false;
  offset += used;

    os_trace("%s success\r\n", __func__);
  // Success
  results->decode_type = AIRWELL;
  results->bits = nbits;
  results->address = 0;
  results->command = 0;
  return true;
}

static bool airwell_prev_power = true;
void AirwellAc_setInfo(const uint64_t state, state_ir_t *ir_state) {
    AirwellProtocol _;
    _.raw = state;

    if (_.PowerToggle) {
        airwell_prev_power = !airwell_prev_power;
    }
    ir_state->power = airwell_prev_power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp + kAirwellMinTemp - 1;
}