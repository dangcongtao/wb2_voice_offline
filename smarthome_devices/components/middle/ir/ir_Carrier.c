#include "ir_Carrier.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"

const uint16_t kCarrierAc64HdrMark = 8940;
const uint16_t kCarrierAc64HdrSpace = 4556;
const uint16_t kCarrierAc64BitMark = 503;
const uint16_t kCarrierAc64OneSpace = 1736;
const uint16_t kCarrierAc64ZeroSpace = 615;
const uint32_t kCarrierAc64Gap = kDefaultMessageGap;  // A guess.

static uint8_t IRCarrierAc64_calcChecksum(const uint64_t state) {
  uint64_t data = GETBITS64(state,
      kCarrierAc64ChecksumOffset + kCarrierAc64ChecksumSize, kCarrierAc64Bits -
      (kCarrierAc64ChecksumOffset + kCarrierAc64ChecksumSize));
  uint8_t result = 0;
  for (; data; data >>= 4)  // Add each nibble together.
    result += GETBITS64(data, 0, 4);
  return result & 0xF;
}

/// Verify the checksum is valid for a given state.
/// @param[in] state The array to verify the checksum of.
/// @return true, if the state has a valid checksum. Otherwise, false.
static bool IRCarrierAc64_validChecksum(const uint64_t state) {
  // Validate the checksum of the given state.
  return (GETBITS64(state, kCarrierAc64ChecksumOffset,
                    kCarrierAc64ChecksumSize) == IRCarrierAc64_calcChecksum(state));
}

bool decodeCarrierAC64(decode_results *results, uint16_t offset,
                               const uint16_t nbits, const bool strict) {
  if (results->rawlen < 2 * nbits + kHeader + kFooter - 1 + offset)
    return false;  // Can't possibly be a valid Carrier message.
  if (strict && nbits != kCarrierAc64Bits)
    return false;  // We expect Carrier to be 64 bits of message.

  if (!matchGeneric(results->rawbuf + offset, &(results->value),
                    results->rawlen - offset, nbits,
                    kCarrierAc64HdrMark, kCarrierAc64HdrSpace,
                    kCarrierAc64BitMark, kCarrierAc64OneSpace,
                    kCarrierAc64BitMark, kCarrierAc64ZeroSpace,
                    kCarrierAc64BitMark, kCarrierAc64Gap, true,
                    kUseDefTol, kMarkExcess, false)) return false;

  // Compliance
  if (strict && !IRCarrierAc64_validChecksum(results->value)){
    return false;
    os_trace("%s invalid checksum\r\n", __func__);
  }
    os_trace("%s success\r\n", __func__);
  // Success
  results->bits = nbits;
  results->decode_type = CARRIER_AC64;
  results->address = 0;
  results->command = 0;
  return true;
}

void IRCarrierAc64_setInfo(const uint64_t state, state_ir_t *ir_state) { 
    CarrierProtocol _;
    memset(&_, 0, sizeof(CarrierProtocol));
    _.raw = state; 
    ir_state->power = _.Power;
    ir_state->mode = _.Mode;
    ir_state->fanspeed = _.Fan;
    ir_state->degrees = _.Temp + kCarrierAc64MinTemp;
}

