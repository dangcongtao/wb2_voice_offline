#include "ir_Delonghi.h"
#include "system_typedef.h"
#include "string.h"
#include "IRutils.h"


const uint16_t kDelonghiAcHdrMark = 8984;
const uint16_t kDelonghiAcBitMark = 572;
const uint16_t kDelonghiAcHdrSpace = 4200;
const uint16_t kDelonghiAcOneSpace = 1558;
const uint16_t kDelonghiAcZeroSpace = 510;
const uint32_t kDelonghiAcGap = kDefaultMessageGap;  // A totally made-up guess.
const uint16_t kDelonghiAcFreq = 38000;  // Hz. (Guess: most common frequency.)
const uint16_t kDelonghiAcOverhead = 3;

static uint8_t IRDelonghiAc_calcChecksum(const uint64_t state) {
  uint8_t sum = 0;
  // Add up all the 8 bit chunks except for Most-significant 8 bits.
  for (uint8_t offset = 0; offset < kDelonghiAcChecksumOffset; offset += 8) {
    sum += GETBITS64(state, offset, 8);
  }
  return sum;
}

static bool IRDelonghiAc_validChecksum(const uint64_t state) {
  DelonghiProtocol dp;
  dp.raw = state;
  return (dp.Sum == IRDelonghiAc_calcChecksum(state));
}

bool decodeDelonghiAc(decode_results *results, uint16_t offset,
                              const uint16_t nbits, const bool strict) {
  if (results->rawlen < 2 * nbits + kDelonghiAcOverhead - offset)
    return false;  // Too short a message to match.
  if (strict && nbits != kDelonghiAcBits)
    return false;

  uint64_t data = 0;

  // Header + Data + Footer
  if (!matchGenericDataLimit(results->rawbuf + offset, &data,
                    results->rawlen - offset, nbits,
                    kDelonghiAcHdrMark-2250, kDelonghiAcHdrMark+2250, kDelonghiAcHdrSpace-1000, kDelonghiAcHdrSpace+1000,
                    kDelonghiAcBitMark-420, kDelonghiAcBitMark+420, kDelonghiAcOneSpace-500, kDelonghiAcOneSpace+500,
                    kDelonghiAcBitMark-420, kDelonghiAcBitMark+420, kDelonghiAcZeroSpace-400, kDelonghiAcZeroSpace+500,
                    kDelonghiAcBitMark-420, kDelonghiAcBitMark+420, kDelonghiAcGap-25000, kDelonghiAcGap+25000, true,
                    _tolerance, kMarkExcess, false)) return false;

  // Compliance
  if (strict && !IRDelonghiAc_validChecksum(data)){
    os_trace("%s invalid checksum\r\n", __func__);
    return false;
  }
    os_trace("%s success\r\n", __func__);
  // Success
  results->decode_type = DELONGHI_AC;
  results->bits = nbits;
  results->value = data;
  results->command = 0;
  results->address = 0;
  return true;
}

void IRDelonghiAc_setRaw(const uint64_t state, state_ir_t *ir_state)
{ 
    DelonghiProtocol _;
    memset(&_, 0, sizeof(DelonghiProtocol));
    _.raw = state;

    ir_state->power = _.Power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->celsius = !_.Fahrenheit;
    ir_state->degrees = _.Temp + (_.Fahrenheit ? kDelonghiAcTempMinF
                                : kDelonghiAcTempMinC) - 1;
    if (_.Fahrenheit) {
        ir_state->degrees = fahrenheitToCelsius(ir_state->degrees);
    }
    ir_state->turbo = _.Boost;
}
