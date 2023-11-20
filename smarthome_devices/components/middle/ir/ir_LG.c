#include "ir_LG.h"
#include "IRutils.h"

const uint16_t kLgBitMark = 550;              ///< uSeconds.
const uint16_t kLgOneSpace = 1600;            ///< uSeconds.
const uint16_t kLgZeroSpace = 550;            ///< uSeconds.
const uint16_t kLgRptSpace = 2250;            ///< uSeconds.
const uint16_t kLgMinGap = 39750;             ///< uSeconds.
const uint32_t kLgMinMessageLength = 108050;  ///< uSeconds.
// LG (28 Bit)
const uint16_t kLgHdrMark = 8500;             ///< uSeconds.
const uint16_t kLgHdrSpace = 4250;            ///< uSeconds.
// LG (32 Bit)
const uint16_t kLg32HdrMark = 4500;           ///< uSeconds.
const uint16_t kLg32HdrSpace = 4450;          ///< uSeconds.
const uint16_t kLg32RptHdrMark = 8950;        ///< uSeconds.
// LG2 (28 Bit)
const uint16_t kLg2HdrMark = 3200;            ///< uSeconds.
const uint16_t kLg2HdrSpace = 9900;           ///< uSeconds.
const uint16_t kLg2BitMark = 480; 

const uint32_t kLgAcAKB74955603DetectionMask = 0x0000080;
const uint8_t  kLgAcChecksumSize = 4;  ///< Size in bits.
// Signature has the checksum removed, and another bit to match both Auto & Off.
const uint8_t  kLgAcSwingHOffsetSize = kLgAcChecksumSize + 1;
const uint32_t kLgAcSwingHSignature  = kLgAcSwingHOff >> kLgAcSwingHOffsetSize;
const uint32_t kLgAcVaneSwingVBase = 0x8813200;

bool decodeLG(decode_results *results, uint16_t offset,
                      const uint16_t nbits, const bool strict) {
  if (nbits >= kLg32Bits) {
    if (results->rawlen <= 2 * nbits + 2 * (kHeader + kFooter) - 1 + offset)
      return false;  // Can't possibly be a valid LG32 message.
  } else {
    if (results->rawlen <= 2 * nbits + kHeader - 1 + offset)
      return false;  // Can't possibly be a valid LG message.
  }
  // Compliance
  if (strict && nbits != kLgBits && nbits != kLg32Bits)
    return false;  // Doesn't comply with expected LG protocol.

  // Header (Mark)
  uint32_t kHdrSpace;
  if (matchMark(results->rawbuf[offset], kLgHdrMark, kUseDefTol, kMarkExcess))
    kHdrSpace = kLgHdrSpace;
  else if (matchMark(results->rawbuf[offset], kLg2HdrMark, kUseDefTol, kMarkExcess))
    kHdrSpace = kLg2HdrSpace;
  else if (matchMark(results->rawbuf[offset], kLg32HdrMark, kUseDefTol, kMarkExcess))
    kHdrSpace = kLg32HdrSpace;
  else
    return false;
  offset++;

  // Set up the expected data section values.
  const uint16_t kBitmark = (kHdrSpace == kLg2HdrSpace) ? kLg2BitMark
                                                        : kLgBitMark;
  // Header Space + Data + Footer
  uint64_t data = 0;
  uint16_t used = matchGeneric(results->rawbuf + offset, &data,
                               results->rawlen - offset, nbits,
                               0,  // Already matched the Header mark.
                               kHdrSpace,
                               kBitmark, kLgOneSpace, kBitmark, kLgZeroSpace,
                               kBitmark, kLgMinGap, true, kUseDefTol, 0, true);
  if (!used) return false;
  offset += used;

  // Repeat
  if (nbits >= kLg32Bits) {
    // If we are expecting the LG 32-bit protocol, there is always
    // a repeat message. So, check for it.
    uint64_t unused;
    if (!matchGeneric(results->rawbuf + offset, &unused,
                      results->rawlen - offset, 0,  // No Data bits to match.
                      kLg32RptHdrMark, kLgRptSpace,
                      kBitmark, kLgOneSpace, kBitmark, kLgZeroSpace,
                      kBitmark, kLgMinGap, true, kUseDefTol, kMarkExcess, true)) return false;
  }

  // The 16 bits before the checksum.
  uint16_t command = (data >> kLgAcChecksumSize);

  // Compliance
  if (strict && (data & 0xF) != sumNibbles(command, 4, 0, true))
    return false;  // The last 4 bits sent are the expected checksum.
  // Success
  if (kHdrSpace == kLg2HdrSpace)  // Was it an LG2 message?
    results->decode_type = LG2;
  else
    results->decode_type = LG;
  results->bits = nbits;
  results->value = data;
  results->command = command;
  results->address = data >> 20;  // The bits before the command.
  return true;
}