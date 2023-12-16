#include "ir_Sharp.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"

// Constants
// period time = 1/38000Hz = 26.316 microseconds.
const uint16_t kSharpTick = 26;
const uint16_t kSharpBitMarkTicks = 10;
const uint16_t kSharpBitMark = kSharpBitMarkTicks * kSharpTick;
const uint16_t kSharpOneSpaceTicks = 70;
const uint16_t kSharpOneSpace = kSharpOneSpaceTicks * kSharpTick;
const uint16_t kSharpZeroSpaceTicks = 30;
const uint16_t kSharpZeroSpace = kSharpZeroSpaceTicks * kSharpTick;
const uint16_t kSharpGapTicks = 1677;
const uint16_t kSharpGap = kSharpGapTicks * kSharpTick;
// Address(5) + Command(8) + Expansion(1) + Check(1)
const uint64_t kSharpToggleMask =
    ((uint64_t)1 << (kSharpBits - kSharpAddressBits)) - 1;
const uint64_t kSharpAddressMask = ((uint64_t)1 << kSharpAddressBits) - 1;
const uint64_t kSharpCommandMask = ((uint64_t)1 << kSharpCommandBits) - 1;


uint8_t IRSharpAccalcChecksum(uint8_t state[], const uint16_t length) {
  uint8_t xorsum = xorBytes(state, length - 1, 0);
  xorsum ^= GETBITS8(state[length - 1], kLowNibble, kNibbleSize);
  xorsum ^= GETBITS8(xorsum, kHighNibble, kNibbleSize);
  return GETBITS8(xorsum, kLowNibble, kNibbleSize);
}

bool IRSharpAcvalidChecksum(uint8_t state[], const uint16_t length) {
  return GETBITS8(state[length - 1], kHighNibble, kNibbleSize) ==
      IRSharpAccalcChecksum(state, length);
}

bool decodeSharpAc(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict) {
  // Compliance
  if (strict && nbits != kSharpAcBits) return false;

  // Match Header + Data + Footer
  uint16_t used;
  used = matchGenericState(results->rawbuf + offset, results->state,
                      results->rawlen - offset, nbits,
                      kSharpAcHdrMark, kSharpAcHdrSpace,
                      kSharpAcBitMark, kSharpAcOneSpace,
                      kSharpAcBitMark, kSharpAcZeroSpace,
                      kSharpAcBitMark, kSharpAcGap, true,
                      45, kMarkExcess, false);
  if (used == 0) {
    os_trace("%s used failed\r\n", __func__);
    return false;
  }
  offset += used;
  // Compliance
  if (strict) {
    if (!IRSharpAcvalidChecksum(results->state, kSharpAcStateLength)) {
        os_trace("%s invalid checksum\r\n", __func__);
        return false;
    }
  }

  // Success
  results->decode_type = SHARP_AC;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.

  os_trace("%s success\r\n", __func__);

  return true;
}