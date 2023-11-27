#include "ir_Samsung.h"
#include "IRutils.h"
#include "system_typedef.h"
#include "IRsend.h"

#define kSamsungTick 560
#define kSamsungHdrMarkTicks 8
#define kSamsungHdrMark kSamsungHdrMarkTicks * kSamsungTick
#define kSamsungHdrSpaceTicks 8
#define kSamsungHdrSpace kSamsungHdrSpaceTicks * kSamsungTick
#define kSamsungBitMarkTicks 1
#define kSamsungBitMark kSamsungBitMarkTicks * kSamsungTick
#define kSamsungOneSpaceTicks 3
#define kSamsungOneSpace kSamsungOneSpaceTicks * kSamsungTick
#define kSamsungZeroSpaceTicks 1
#define kSamsungZeroSpace kSamsungZeroSpaceTicks * kSamsungTick
#define kSamsungRptSpaceTicks 4
#define kSamsungRptSpace kSamsungRptSpaceTicks * kSamsungTick
#define kSamsungMinMessageLengthTicks 193
#define kSamsungMinMessageLength (kSamsungMinMessageLengthTicks * kSamsungTick)
#define kSamsungMinGapTicks (kSamsungMinMessageLengthTicks - (kSamsungHdrMarkTicks + kSamsungHdrSpaceTicks + kSamsungBits * (kSamsungBitMarkTicks + kSamsungOneSpaceTicks) +kSamsungBitMarkTicks))
#define kSamsungMinGap (kSamsungMinGapTicks * kSamsungTick)

#define kSamsungAcHdrMark 690
#define kSamsungAcHdrSpace 17844
#define kSamsungAcSections 2
#define kSamsungAcSectionMark 3086
#define kSamsungAcSectionSpace 8864
#define kSamsungAcSectionGap 2886
#define kSamsungAcBitMark 586
#define kSamsungAcOneSpace 1432
#define kSamsungAcZeroSpace 436

// Data from https://github.com/crankyoldgit/IRremoteESP8266/issues/1220
// Values calculated based on the average of ten messages.
#define kSamsung36HdrMark 4515  /// < uSeconds
#define kSamsung36HdrSpace 4438  /// < uSeconds
#define kSamsung36BitMark 512  /// < uSeconds
#define kSamsung36OneSpace 1468  /// < uSeconds
#define kSamsung36ZeroSpace 490  /// < uSeconds

// _.Swing
#define kSamsungAcSwingV        0b010
#define kSamsungAcSwingH        0b011
#define kSamsungAcSwingBoth     0b100
#define kSamsungAcSwingOff      0b111
// _.FanSpecial
#define kSamsungAcFanSpecialOff 0b000
#define kSamsungAcPowerfulOn    0b011
#define kSamsungAcBreezeOn      0b101
#define kSamsungAcEconoOn       0b111

// uint8_t _tolerance;


bool decodeSAMSUNG(decode_results *results, uint16_t offset,
                           const uint16_t nbits, const bool strict) {

  if (strict && nbits != kSamsungBits)
    return false;  // We expect Samsung to be 32 bits of message.

  uint64_t data = 0;

  // Match Header + Data + Footer
  if (!matchGeneric(results->rawbuf + offset, &data,
                    results->rawlen - offset, nbits,
                    kSamsungHdrMark, kSamsungHdrSpace,
                    kSamsungBitMark, kSamsungOneSpace,
                    kSamsungBitMark, kSamsungZeroSpace,
                    kSamsungBitMark, kSamsungMinGap, true, kUseDefTol, 0, true)) return false;
  // Compliance
  // According to the spec, the customer (address) code is the first 8
  // transmitted bits. It's then repeated. Check for that.
  uint8_t address = data >> 24;
  if (strict && address != ((data >> 16) & 0xFF)) return false;
  // Spec says the command code is the 3rd block of transmitted 8-bits,
  // followed by the inverted command code.
  uint8_t command = (data & 0xFF00) >> 8;
  if (strict && command != ((data & 0xFF) ^ 0xFF)) return false;

  // Success
  results->bits = nbits;
  results->value = data;
  results->decode_type = SAMSUNG;
  // command & address need to be reversed as they are transmitted LSB first,
  results->command = reverseBits(command, sizeof(command) * 8);
  results->address = reverseBits(address, sizeof(address) * 8);
  os_trace("decode samsung success\r\n");
  return true;
}

uint8_t getSectionChecksum(const uint8_t *section) {
  return ((GETBITS8(*(section + 2), kLowNibble, kNibbleSize) << kNibbleSize) +
          GETBITS8(*(section + 1), kHighNibble, kNibbleSize));
}

uint8_t calcSectionChecksum(const uint8_t *section) {
  uint8_t sum = 0;

  sum += countBits(*section, 8, true, 0);  // Include the entire first byte
  // The lower half of the second byte.
  sum += countBits(GETBITS8(*(section + 1), kLowNibble, kNibbleSize), 8, true, 0);
  // The upper half of the third byte.
  sum += countBits(GETBITS8(*(section + 2), kHighNibble, kNibbleSize), 8, true, 0);
  // The next 4 bytes.
  sum += countBits(section + 3, 4, true, 0);
  // Bitwise invert the result.
  return sum ^ UINT8_MAX;
}

static bool validChecksum(const uint8_t state[], const uint16_t length) {
  bool result = true;
  const uint16_t maxlength =
      (length > kSamsungAcExtendedStateLength) ? kSamsungAcExtendedStateLength
                                               : length;
  for (uint16_t offset = 0;
       offset + kSamsungAcSectionLength <= maxlength;
       offset += kSamsungAcSectionLength)
    result &= (getSectionChecksum(state + offset) ==
               calcSectionChecksum(state + offset));
  return result;
}

bool decodeSamsungAC(decode_results *results, uint16_t offset,
                             const uint16_t nbits, const bool strict) {
  if (results->rawlen < 2 * nbits + kHeader * 3 + kFooter * 2 - 1 + offset){
    os_trace("Can't possibly be a valid Samsung A/C message.\r\n");
    return false;  // Can't possibly be a valid Samsung A/C message.
  }
  os_trace("step 1\r\n");
  if (nbits != kSamsungAcBits && nbits != kSamsungAcExtendedBits){
    os_trace("step 1 failed\r\n");
    return false;
  }
  os_trace("step 2\r\n");

  // Message Header
  if (!matchMark(results->rawbuf[offset++], kSamsungAcBitMark, kUseDefTol, kMarkExcess)){
    os_trace("match mark failed\r\n");
    return false;
  }
  os_trace("step 3\r\n");
  if (!matchSpace(results->rawbuf[offset++], kSamsungAcHdrSpace, kUseDefTol, kMarkExcess)){
    os_trace("match space failed\r\n");
    return false;
  }
  os_trace("step 4\r\n");
  // Section(s)
  for (uint16_t pos = 0; pos <= (nbits / 8) - kSamsungAcSectionLength;
       pos += kSamsungAcSectionLength) {
    uint16_t used;
    // Section Header + Section Data (7 bytes) + Section Footer
    used = matchGeneric(results->rawbuf + offset, results->state + pos,
                        results->rawlen - offset, kSamsungAcSectionLength * 8,
                        kSamsungAcSectionMark, kSamsungAcSectionSpace,
                        kSamsungAcBitMark, kSamsungAcOneSpace,
                        kSamsungAcBitMark, kSamsungAcZeroSpace,
                        kSamsungAcBitMark, kSamsungAcSectionGap,
                        pos + kSamsungAcSectionLength >= nbits / 8,
                        _tolerance, 0, false);
    if (used == 0){
      os_trace("step 2 not used failed\r\n");
      return false;
    }
    offset += used;
  }
  os_trace("step 5\r\n");
  // Compliance
  if (strict) {
    // Is the checksum valid?
    if (!validChecksum(results->state, nbits / 8)) {
      os_trace("DEBUG: Checksum failed!");
      return false;
    }
  }
  // Success
  results->decode_type = SAMSUNG_AC;
  results->bits = nbits;
  // No need to record the state as we stored it as we decoded it.
  // As we use result->state, we don't record value, address, or command as it
  // is a union data type.
  os_trace("decode samsung ac success\r\n");
  return true;
}